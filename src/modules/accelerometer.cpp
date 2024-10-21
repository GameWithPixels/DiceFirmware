#include "accelerometer.h"

#include "drivers_hw/accel_chip.h"
#include "utils/int3_utils.h"
#include "core/ring_buffer.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "config/dice_variants.h"
#include "app_error.h"
#include "app_error_weak.h"
#include "nrf_log.h"
#include "config/settings.h"
#include "bluetooth/bluetooth_message_service.h"
#include "bluetooth/bluetooth_messages.h"
#include "bluetooth/bluetooth_stack.h"
#include "drivers_nrf/power_manager.h"
#include "drivers_nrf/gpiote.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/flash.h"
#include "drivers_nrf/scheduler.h"
#include "leds.h"
#include "validation_manager.h"
#include "malloc.h"


using namespace Modules;
using namespace Core;
using namespace DriversHW;
using namespace DriversNRF;
using namespace Config;
using namespace Bluetooth;

// This defines how frequently we try to read the accelerometer
#define MAX_FRAMEDATA_CLIENTS 1
#define MAX_ACC_CLIENTS 8
#define MAX_ACCELERATION_FRAMES 3

#define ABS(x) ((x) < 0 ? -(x) : (x))

namespace Modules::Accelerometer
{
    // This stores a few frames of acceleration data
    static AccelFrame frames[MAX_ACCELERATION_FRAMES];

    static DelegateArray<FrameDataClientMethod, MAX_FRAMEDATA_CLIENTS> frameDataClients;
    static DelegateArray<RollStateClientMethod, MAX_ACC_CLIENTS> rollStateClients;

    enum State {
        State_Unknown = 0,
        State_Initializing,
        State_Off,
        State_On,
        State_LowPower
    };
    State currentState = State_Unknown;

    void updateState();
    void pauseNotifications();
    void resumeNotifications();

    void calibrateHandler(const Message *msg);
    void calibrateFaceHandler(const Message *msg);
    void onSettingsProgrammingEvent(void *context, Flash::ProgrammingEventType evt);
    void readAccelerometer(int3 *acc);
    void accHandler(const int3 &acc);

    void update(void *context);

    // Given two vectors, return the absolute difference in the axis that changed the most.
    int agitation(int3 xyz0, int3 xyz_minus1) {
        int3 abs_diffs(
            ABS(xyz0.xTimes1000 - xyz_minus1.xTimes1000),
            ABS(xyz0.yTimes1000 - xyz_minus1.yTimes1000),
            ABS(xyz0.zTimes1000 - xyz_minus1.zTimes1000));
        return(MAX(abs_diffs.xTimes1000, MAX(abs_diffs.yTimes1000, abs_diffs.zTimes1000)));
    }

    void init(InitCallback callback) {
        static InitCallback _callback; // Don't initialize this static inline because it would only do it on first call!
        _callback = callback;

        currentState = State_Initializing;
        AccelChip::init([](bool result) {
            if (result) {
                MessageService::RegisterMessageHandler(Message::MessageType_Calibrate, calibrateHandler);
                MessageService::RegisterMessageHandler(Message::MessageType_CalibrateFace, calibrateFaceHandler);

                Flash::hookProgrammingEvent(onSettingsProgrammingEvent, nullptr);

                currentState = State_Off;
                start();
                NRF_LOG_DEBUG("Acc init");
            }
            _callback(result);
        });
    }

    void update(void *context) {
        int3 acc;
        readAccelerometer(&acc);
        accHandler(acc);
    }

    /// <summary>
    /// Crudely compares accelerometer readings passed in to determine the current face up
    /// Will return the last value if it cannot determine the current face up
    /// </summary>
    /// <returns>The face number, starting at 0</returns>
    int determineFace(int3 acc, int16_t *outConfidence, int previousFace) {
        // Compare against face normals stored in layout
        int faceCount = SettingsManager::getLayout()->faceCount;

        // Use calibrated normals, not canonical ones
        auto settings = SettingsManager::getSettings();
        auto &normals = settings->faceNormals;

        // First check that the acceleration is not too low
        int accMagTimes1000 = acc.magnitudeTimes1000();
        if (accMagTimes1000 < settings->fallingThresholdTimes1000) {
            // We return the previous face, but we have no real idea actually
            *outConfidence = 0;
            return previousFace;
        } else {
            // Compare the acceleration vector with the face normals
            int3 nacc = acc * 1000 / accMagTimes1000; // normalize

            // The starting "best value" should be -1000 as we're comparing this value with
            // the dot product of 2 normalized vectors, but is set a bit lower due to
            // imprecision of fixed point operations, which may return a value lower than -1000.
            int bestDotTimes1000 = -1100;    
            int bestFace = previousFace;
            for (int i = 0; i < faceCount; ++i) {
                int dotTimes1000 = int3::dotTimes1000(nacc, normals[i]);
                if (dotTimes1000 > bestDotTimes1000) {
                    // Found a better match
                    bestDotTimes1000 = dotTimes1000;
                    bestFace = i;
                }
            }
            // Return the best face
            *outConfidence = bestDotTimes1000;
            return bestFace;
        }
    }

    void accHandler(void *param, const int3 &acc) {
        auto settings = SettingsManager::getSettings();

        // Shift acceleration data back
        for (int i = MAX_ACCELERATION_FRAMES-1; i >= 1; --i) {
            frames[i] = frames[i-1];
        }

        // Now frame 0 will receive the new data
        frames[0].time = DriversNRF::Timers::millis();
        frames[0].acc = acc;
        frames[0].agitationTimes1000 = agitation(acc, frames[1].acc);
        frames[0].face = determineFace(acc, &frames[0].faceConfidenceTimes1000, frames[1].face);

        bool onFace = frames[0].faceConfidenceTimes1000 > settings->faceThresholdTimes1000
            || SettingsManager::getDieType() != DiceVariants::DieType_D4;
        // Calculate the estimated roll state
        if (frames[0].agitationTimes1000 < settings->lowerThresholdTimes1000) {
            frames[0].estimatedRollState = EstimatedRollState_OnFace;
        } else if (frames[0].agitationTimes1000 >= settings->lowerThresholdTimes1000 && frames[0].agitationTimes1000 < settings->middleThresholdTimes1000) {
            // Medium amount of agitation... we're handling (or finishing to roll)
            if (frames[1].estimatedRollState != EstimatedRollState_Rolling) {
                frames[0].estimatedRollState = EstimatedRollState_Handling;
            } else {
                frames[0].estimatedRollState = EstimatedRollState_Rolling;
            }
        } else {
            frames[0].estimatedRollState = EstimatedRollState_Rolling;
        }
        
        // If the time between the last and current time is zero, log it
        if(frames[0].time - frames[1].time == 0) {
            NRF_LOG_WARNING("Time diff between frames is 0, time: %d", frames[0].time);
        }      

        // Count how many onface states we estimated in the last 3 frames, same with handling and rolling
        int onFaceCount = 0;
        int handlingCount = 0;
        int rollingCount = 0;
        int agitationCount = 0;
        for (int i = 0; i < MAX_ACCELERATION_FRAMES; ++i) {
            switch (frames[i].estimatedRollState) {
                case RollState_OnFace:
                    onFaceCount++;
                    break;
                case RollState_Handling:
                    handlingCount++;
                    break;
                case RollState_Rolling:
                    rollingCount++;
                    break;
                default:
                    break;
            }
            if (frames[i].agitationTimes1000 > settings->upperThresholdTimes1000) {
                agitationCount++;
            }
        }

        frames[0].determinedRollState = frames[1].determinedRollState;
        if (onFaceCount == 3) {
            // Are we on a valid face?
            if (onFace) {
                // Is it a valid roll?
                if (frames[1].determinedRollState == RollState_Rolling) {
                    // We were rolling, and now we're on face, so we rolled
                    frames[0].determinedRollState = RollState_Rolled;
                } else {
                    frames[0].determinedRollState = RollState_OnFace;
                }
            } else {
                frames[0].determinedRollState = RollState_Crooked;
            }
        } else if (handlingCount == 3) {
            frames[0].determinedRollState = RollState_Handling;
        } else if ((rollingCount >= 2) && (agitationCount > 0)) {
            frames[0].determinedRollState = RollState_Rolling;
        }

        bool faceChanged = frames[0].face != frames[1].face;
        bool stateChanged = frames[0].determinedRollState != frames[1].determinedRollState;
        if (faceChanged || stateChanged) {
            for (int i = 0; i < rollStateClients.Count(); ++i) {
                rollStateClients[i].handler(rollStateClients[i].token, frames[1].determinedRollState, frames[1].face, frames[0].determinedRollState, frames[0].face);
            }
        }

        // Notify frame data clients
        for (int i = 0; i < frameDataClients.Count(); ++i) {
            frameDataClients[i].handler(frameDataClients[i].token, frames[0]);
        }
    }

    /// <summary>
    /// Initialize the acceleration system
    /// </summary>
    void start() {
        switch (currentState) {
            case State_Off:
            case State_LowPower:
                {
                    NRF_LOG_DEBUG("Starting accelerometer");

                    // Initialize the acceleration data
                    readAccelerometer(&frames[0].acc);
                    frames[0].face = determineFace(frames[0].acc, &frames[0].faceConfidenceTimes1000, 0);
                    frames[0].time = DriversNRF::Timers::millis();
                    frames[0].agitationTimes1000 = 0;
                    frames[0].estimatedRollState = EstimatedRollState_OnFace;
                    memcpy(&frames[1], &frames[0], sizeof(AccelFrame));
                    memcpy(&frames[2], &frames[0], sizeof(AccelFrame));

                    // Unhook first to avoid being hooked more than once if start() is called multiple times
                    AccelChip::unHook(accHandler);
                    AccelChip::hook(accHandler, nullptr);
                    AccelChip::disableInterrupt();
                    AccelChip::clearInterrupt();
                    AccelChip::enableDataInterrupt();

                    // Update current state
                    currentState = State_On;
                }
                break;
            default:
                NRF_LOG_WARNING("Accelerometer not in valid state to be started");
                break;
        }
    }

    /// <summary>
    /// Stop getting updated from the timer
    /// </summary>
    void stop() {
        switch (currentState) {
            case State_On:
                AccelChip::unHook(accHandler);
                AccelChip::disableDataInterrupt();
                AccelChip::clearInterrupt();

                // Update current state
                currentState = State_Off;
                NRF_LOG_DEBUG("Stopped accelerometer");
                break;
            default:
                NRF_LOG_WARNING("Accelerometer not in valid state to stop");
                break;
        }
    }

    void lowPower() {
        switch (currentState) {
            case State_Off:
                AccelChip::lowPower();
                currentState = State_LowPower;
                break;
            default:
                NRF_LOG_WARNING("Accelerometer not in valid state to go to low power");
                break;
        }
    }

    void wakeUp() {
        start();

        // Force override the roll state, since we most likely just woke up from motion
        frames[0].determinedRollState = RollState_Handling;

        // Notify frame data clients
        for (int i = 0; i < frameDataClients.Count(); ++i) {
            frameDataClients[i].handler(frameDataClients[i].token, frames[0]);
        }
    }


    /// <summary>
    /// Returns the currently stored up face!
    /// </summary>
    int currentFace() {
        return frames[0].face;
    }

    int currentFaceConfidenceTimes1000() {
        return frames[0].faceConfidenceTimes1000;
    }

    RollState currentRollState() {
        return frames[0].determinedRollState;
    }

    const char *getRollStateString(RollState state) {
#if defined(DEBUG)
        switch (state)
        {
        case RollState_Unknown:
        default:
            return "Unknown";
        case RollState_OnFace:
            return "OnFace";
        case RollState_Handling:
            return "Handling";
        case RollState_Rolling:
            return "Rolling";
        case RollState_Crooked:
            return "Crooked";
        }
#else
        return "";
#endif
    }

    /// <summary>
    /// Method used by clients to request timer callbacks when accelerometer readings are in
    /// </summary>
    void hookFrameData(Accelerometer::FrameDataClientMethod callback, void *parameter) {
        if (!frameDataClients.Register(parameter, callback)) {
            NRF_LOG_ERROR("Too many accelerometer hooks registered.");
        }
    }

    /// <summary>
    /// Method used by clients to stop getting accelerometer reading callbacks
    /// </summary>
    void unHookFrameData(Accelerometer::FrameDataClientMethod callback) {
        frameDataClients.UnregisterWithHandler(callback);
    }

    /// <summary>
    /// Method used by clients to stop getting accelerometer reading callbacks
    /// </summary>
    void unHookFrameDataWithParam(void *param) {
        frameDataClients.UnregisterWithToken(param);
    }

    void hookRollState(RollStateClientMethod method, void *param) {
        if (!rollStateClients.Register(param, method)) {
            NRF_LOG_ERROR("Too many accelerometer hooks registered.");
        }
    }

    void unHookRollState(RollStateClientMethod client) {
        rollStateClients.UnregisterWithHandler(client);
    }

    void unHookRollStateWithParam(void *param) {
        rollStateClients.UnregisterWithToken(param);
    }

    struct CalibrationNormals
    {
        int3 face1;
        int3 face5;
        int3 face10;
        int confidenceTimes1000;

        // Set to true if connection is lost to stop calibration
        bool calibrationInterrupted;

        CalibrationNormals() : calibrationInterrupted(false) {}
    };

    static CalibrationNormals *measuredNormals = nullptr;

    void onConnectionLost(void *param, bool connected) {
        if (!connected)
            measuredNormals->calibrationInterrupted = true;
    }

    void calibrateHandler(const Message *msg) {
        // Turn off state change notifications
        stop();
        Bluetooth::Stack::hook(onConnectionLost, nullptr);

        // Helper to restart accelerometer and clean up
        static auto restart = []()
        {
            Bluetooth::Stack::unHook(onConnectionLost);
            start();
        };

        // Start calibration!
        measuredNormals = (CalibrationNormals *)malloc(sizeof(CalibrationNormals));

#pragma GCC diagnostic push "-Wstack-usage="
#pragma GCC diagnostic ignored "-Wstack-usage="

        // Ask user to place die on face 1
        MessageService::NotifyUser("Place face 1 up", true, true, 30, [](bool okCancel)
        {
            if (okCancel) {
                // Die is on face 1
                // Read the normals
                readAccelerometer(&measuredNormals->face1);

                // Debugging
                //BLE_LOG_INFO("Face 1 Normal: %d, %d, %d", (int)(measuredNormals->face1.x * 100), (int)(measuredNormals->face1.y * 100), (int)(measuredNormals->face1.z * 100));

                // Place on face 5
                MessageService::NotifyUser("5 up", true, true, 30, [] (bool okCancel)
                {
                    if (okCancel) {
                        // Die is on face 5
                        // Read the normals
                        readAccelerometer(&measuredNormals->face5);

                        // Debugging
                        //BLE_LOG_INFO("Face 5 Normal: %d, %d, %d", (int)(measuredNormals->face5.x * 100), (int)(measuredNormals->face5.y * 100), (int)(measuredNormals->face5.z * 100));

                        // Place on face 10
                        MessageService::NotifyUser("10 up", true, true, 30, [] (bool okCancel)
                        {
                            if (okCancel) {
                                // Die is on face 10
                                // Read the normals
                                readAccelerometer(&measuredNormals->face10);

                                // From the 3 measured normals we can calibrate the accelerometer
                                auto l = SettingsManager::getLayout();

                                int3 newNormals[l->faceCount];
                                measuredNormals->confidenceTimes1000 = Utils::CalibrateNormals(
                                    0, measuredNormals->face1,
                                    4, measuredNormals->face5,
                                    9, measuredNormals->face10,
                                    newNormals, l->faceCount);

                                // And flash the new normals
                                SettingsManager::programCalibrationData(newNormals, l->faceCount, [] (bool result) {

                                    // Notify user that we're done, yay!!!
                                    // char text[256] = "";
                                    // snprintf(text, 256, "Calibrated, confidence = %d", (int)(measuredNormals->confidence * 100));
                                    MessageService::NotifyUser("Calibrated", true, false, 30, [] (bool okCancel) {
                                        // Restart notifications
                                        restart();
                                    });
                                });
                            } else {
                                // Process cancelled, restart notifications
                                restart();
                            }
                        });
                    } else {
                        // Process cancelled, restart notifications
                        restart();
                    }
                });
            } else {
                // Process cancelled, restart notifications
                restart();
            }
        });
    }

    void calibrateFaceHandler(const Message *msg) {
        const MessageCalibrateFace *faceMsg = (const MessageCalibrateFace *)msg;
        uint8_t face = faceMsg->face;

        // Copy current calibration data
        auto l = SettingsManager::getLayout();
        int normalCount = l->faceCount;
        int3 calibratedNormalsCopy[normalCount];
        memcpy(calibratedNormalsCopy, SettingsManager::getSettings()->faceNormals, normalCount * sizeof(int3));

        // Replace the face's normal with what we measured
        readAccelerometer(&calibratedNormalsCopy[face]);

        // And flash the new normals
        SettingsManager::programCalibrationData(calibratedNormalsCopy, normalCount, [](bool result)
                                                { MessageService::NotifyUser("Face calibrated", true, false, 5, nullptr); });
    }

#pragma GCC diagnostic pop "-Wstack-usage="

    void onSettingsProgrammingEvent(void *context, Flash::ProgrammingEventType evt) {
        if (evt == Flash::ProgrammingEventType_Begin) {
            NRF_LOG_DEBUG("Stopping axel from programming event");
            stop();
        } else {
            NRF_LOG_DEBUG("Starting axel from programming event");
            start();
        }
    }

    void readAccelerometer(int3 *acc) {
        AccelChip::read(acc);
    }

    // Interrupt Callback Storage
    static void* interruptParam = nullptr;
    static AccelerometerInterruptMethod interruptCallback = nullptr;
    void enableInterrupt(AccelerometerInterruptMethod callback, void* param) {
        ASSERT(interruptCallback == nullptr);
        ASSERT(interruptParam== nullptr);

        // Store the callback and param
        interruptCallback = callback;
        interruptParam = param;
        GPIOTE::enableInterrupt(
            BoardManager::getBoard()->accInterruptPin,
            NRF_GPIO_PIN_NOPULL,
            NRF_GPIOTE_POLARITY_HITOLO,
            [](uint32_t pin, nrf_gpiote_polarity_t action) {

                // Clear interrupt on the accelerometer
                AccelChip::clearInterrupt();

                // And the handler in the GPIOTE manager
                GPIOTE::disableInterrupt(BoardManager::getBoard()->accInterruptPin);

                // Trigger and clear the callback
                auto callback = interruptCallback;
                auto param = interruptParam;
                interruptParam = nullptr;
                interruptCallback = nullptr;

                // Callback!
                callback(param);
            });
        AccelChip::enableInterrupt();
    }

    void disableInterrupt() {
        GPIOTE::disableInterrupt(BoardManager::getBoard()->accInterruptPin);
        AccelChip::disableInterrupt();
    }
}

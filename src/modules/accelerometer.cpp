#include "accelerometer.h"

#include "drivers_hw/accel_chip.h"
#include "utils/float3_utils.h"
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

using namespace Modules;
using namespace Core;
using namespace DriversHW;
using namespace DriversNRF;
using namespace Config;
using namespace Bluetooth;

// This defines how frequently we try to read the accelerometer
#define MAX_FRAMEDATA_CLIENTS 1
#define MAX_ACC_CLIENTS 8

namespace Modules::Accelerometer
{
    static float3 handleStateNormal; // The normal when we entered the handled state, so we can determine if we've moved enough

    // This stores our current Acceleration Data
    static AccelFrame currentFrame;

    static DelegateArray<FrameDataClientMethod, MAX_FRAMEDATA_CLIENTS> frameDataClients;
    static DelegateArray<RollStateClientMethod, MAX_ACC_CLIENTS> rollStateClients;

    enum State
    {
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
    void readAccelerometer(float3 *acc);
    void accHandler(const float3 &acc);

    void update(void *context);

    bool init()
    {
        currentState = State_Initializing;
        if (!AccelChip::init()) {
            return false;
        }

        MessageService::RegisterMessageHandler(Message::MessageType_Calibrate, calibrateHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_CalibrateFace, calibrateFaceHandler);

        Flash::hookProgrammingEvent(onSettingsProgrammingEvent, nullptr);

        // Initialize the acceleration data
        readAccelerometer(&currentFrame.acc);
        currentFrame.face = 0;
        currentFrame.faceConfidence = 0.0f;
        currentFrame.time = DriversNRF::Timers::millis();
        currentFrame.jerk = float3::zero();
        currentFrame.sigma = 0.0f;
        currentFrame.smoothAcc = currentFrame.acc;

        currentState = State_Off;
        start();
        NRF_LOG_DEBUG("Acc init");

        return true;
    }

    void update(void *context)
    {
        float3 acc;
        readAccelerometer(&acc);
        accHandler(acc);
    }

    /// <summary>
    /// Crudely compares accelerometer readings passed in to determine the current face up
    /// Will return the last value if it cannot determine the current face up
    /// </summary>
    /// <returns>The face number, starting at 0</returns>
    int determineFace(float3 acc, float *outConfidence)
    {
        // Compare against face normals stored in board manager
        int faceCount = BoardManager::getBoard()->ledCount;
        auto settings = SettingsManager::getSettings();
        auto &normals = settings->faceNormals;
        float accMag = acc.magnitude();
        if (accMag < settings->fallingThreshold && accMag > 0)
        {
            if (outConfidence != nullptr)
            {
                *outConfidence = 0.0f;
            }
            return currentFrame.face;
        }
        else
        {
            float3 nacc = acc / accMag; // normalize
            float bestDot = -1.1f;      // Should be -1 as we're comparing this value with the dot product
                                        // of 2 normalized vectors, but is set a bit lower due to imprecision 
                                        // of floating point operations, which may return a value lower than -1.
            int bestFace = currentFrame.face;
            for (int i = 0; i < faceCount; ++i)
            {
                float dot = float3::dot(nacc, normals[i]);
                if (dot > bestDot)
                {
                    bestDot = dot;
                    bestFace = i;
                }
            }
            if (outConfidence != nullptr)
            {
                *outConfidence = bestDot;
            }
            return bestFace;
        }
    }

    void accHandler(void *param, const float3 &acc)
    {
        auto settings = SettingsManager::getSettings();

        uint32_t newTime = DriversNRF::Timers::millis();
        Core::float3 newJerk = ((acc - currentFrame.acc) * 1000.0f) / (float)(newTime - currentFrame.time);
        
        // If the time between the last and current time is zero, log it
        if(newTime - currentFrame.time == 0) {
            NRF_LOG_WARNING("Time diff between frames is 0, time: %d", newTime);
        }      

        float jerkMag = newJerk.sqrMagnitude();
        if (jerkMag > 10.f)
        {
            jerkMag = 10.f;
        }
        float newSigma = currentFrame.sigma * settings->sigmaDecay + jerkMag * (1.0f - settings->sigmaDecay);
        Core::float3 newSmoothAcc = currentFrame.smoothAcc * settings->accDecay + acc * (1.0f - settings->accDecay);
        float newFaceConfidence = 0.0f;
        uint8_t newFace = determineFace(acc, &newFaceConfidence);

        bool startMoving = newSigma > settings->startMovingThreshold;
        bool stopMoving = newSigma < settings->stopMovingThreshold;
        bool onFace = newFaceConfidence > settings->faceThreshold;
        bool zeroG = acc.sqrMagnitude() < (settings->fallingThreshold * settings->fallingThreshold);
        bool shock = acc.sqrMagnitude() > (settings->shockThreshold * settings->shockThreshold);

        RollState newRollState = currentFrame.rollState;
        switch (newRollState)
        {
        case RollState_Unknown:
        case RollState_OnFace:
        case RollState_Crooked:
            // We start rolling if we detect enough motion
            if (startMoving)
            {
                // We're at least being handled
                newRollState = RollState_Handling;
                handleStateNormal = acc.normalized();
            }
            break;
        case RollState_Handling:
            // Did we move enough?
            {
                bool rotatedEnough = float3::dot(acc.normalized(), handleStateNormal) < 0.5f;
                if (shock || zeroG || rotatedEnough)
                {
                    // Stuff is happening that we are most likely rolling now
                    newRollState = RollState_Rolling;
                }
                else if (stopMoving)
                {
                    // Just slid the dice around?
                    if (stopMoving)
                    {
                        if (BoardManager::getBoard()->ledCount == 6)
                        {
                            // We may be at rest
                            if (onFace)
                            {
                                // We're at rest
                                newRollState = RollState_OnFace;
                            }
                            else
                            {
                                newRollState = RollState_Crooked;
                            }
                        }
                        else
                        {
                            newRollState = RollState_OnFace;
                        }
                    }
                }
            }
            break;
        case RollState_Rolling:
            // If we stop moving we may be on a face
            if (stopMoving)
            {
                if (BoardManager::getBoard()->ledCount == 6)
                {
                    // We may be at rest
                    if (onFace)
                    {
                        // We're at rest
                        newRollState = RollState_OnFace;
                    }
                    else
                    {
                        newRollState = RollState_Crooked;
                    }
                }
                else
                {
                    newRollState = RollState_OnFace;
                }
            }
            break;
        default:
            break;
        }

        bool faceChanged = newFace != currentFrame.face;
        bool stateChanged = newRollState != currentFrame.rollState;
        if (faceChanged || stateChanged)
        {
            for (int i = 0; i < rollStateClients.Count(); ++i)
            {
                rollStateClients[i].handler(rollStateClients[i].token, newRollState, newFace);
            }
        }

        // Update last frame data
        currentFrame.acc = acc;
        currentFrame.time = newTime;
        currentFrame.jerk = newJerk;
        currentFrame.sigma = newSigma;
        currentFrame.smoothAcc = newSmoothAcc;
        currentFrame.rollState = newRollState;
        currentFrame.face = newFace;
        currentFrame.faceConfidence = newFaceConfidence;

        // Notify frame data clients
        for (int i = 0; i < frameDataClients.Count(); ++i)
        {
            frameDataClients[i].handler(frameDataClients[i].token, currentFrame);
        }
    }

    /// <summary>
    /// Initialize the acceleration system
    /// </summary>
    void start()
    {
        switch (currentState) {
            case State_Off:
            case State_LowPower:
                {
                    NRF_LOG_DEBUG("Starting accelerometer");

                    // Set initial value
                    readAccelerometer(&currentFrame.acc);
                    currentFrame.smoothAcc = currentFrame.acc;
                    currentFrame.time = Timers::millis();
                    currentFrame.jerk = Core::float3::zero();
                    currentFrame.sigma = 0.0f;
                    currentFrame.face = determineFace(currentFrame.acc, &currentFrame.faceConfidence);

                    // Determine what state we're in to begin with
                    auto settings = SettingsManager::getSettings();
                    bool onFace = currentFrame.faceConfidence > settings->faceThreshold;
                    if (onFace) {
                        currentFrame.rollState = RollState_OnFace;
                    } else {
                        currentFrame.rollState = RollState_Crooked;
                    }

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
    void stop()
    {
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

    void lowPower()
    {
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

    void wakeUp()
    {
        start();

        // Force override the roll state, since we most likely just woke up from motion
        currentFrame.rollState = RollState_Handling;

        // Notify frame data clients
        for (int i = 0; i < frameDataClients.Count(); ++i) {
            frameDataClients[i].handler(frameDataClients[i].token, currentFrame);
        }
    }


    /// <summary>
    /// Returns the currently stored up face!
    /// </summary>
    int currentFace()
    {
        return currentFrame.face;
    }

    float currentFaceConfidence()
    {
        return currentFrame.faceConfidence;
    }

    RollState currentRollState()
    {
        return currentFrame.rollState;
    }

    const char *getRollStateString(RollState state)
    {
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
    void hookFrameData(Accelerometer::FrameDataClientMethod callback, void *parameter)
    {
        if (!frameDataClients.Register(parameter, callback))
        {
            NRF_LOG_ERROR("Too many accelerometer hooks registered.");
        }
    }

    /// <summary>
    /// Method used by clients to stop getting accelerometer reading callbacks
    /// </summary>
    void unHookFrameData(Accelerometer::FrameDataClientMethod callback)
    {
        frameDataClients.UnregisterWithHandler(callback);
    }

    /// <summary>
    /// Method used by clients to stop getting accelerometer reading callbacks
    /// </summary>
    void unHookFrameDataWithParam(void *param)
    {
        frameDataClients.UnregisterWithToken(param);
    }

    void hookRollState(RollStateClientMethod method, void *param)
    {
        if (!rollStateClients.Register(param, method))
        {
            NRF_LOG_ERROR("Too many accelerometer hooks registered.");
        }
    }

    void unHookRollState(RollStateClientMethod client)
    {
        rollStateClients.UnregisterWithHandler(client);
    }

    void unHookRollStateWithParam(void *param)
    {
        rollStateClients.UnregisterWithToken(param);
    }

    struct CalibrationNormals
    {
        float3 face1;
        float3 face5;
        float3 face10;
        // float3 led0;
        // float3 led1;
        float confidence;

        // Set to true if connection is lost to stop calibration
        bool calibrationInterrupted;

        CalibrationNormals() : calibrationInterrupted(false) {}
    };

    static CalibrationNormals *measuredNormals = nullptr;

    void onConnectionLost(void *param, bool connected)
    {
        if (!connected)
            measuredNormals->calibrationInterrupted = true;
    }

    void calibrateHandler(const Message *msg)
    {
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
                                auto b = BoardManager::getBoard();

                                float3 newNormals[b->ledCount];
                                measuredNormals->confidence = Utils::CalibrateNormals(
                                    0, measuredNormals->face1,
                                    4, measuredNormals->face5,
                                    9, measuredNormals->face10,
                                    newNormals, b->ledCount);

                                // And flash the new normals
                                SettingsManager::programCalibrationData(newNormals, b->ledCount, [] (bool result) {

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

    void calibrateFaceHandler(const Message *msg)
    {
        const MessageCalibrateFace *faceMsg = (const MessageCalibrateFace *)msg;
        uint8_t face = faceMsg->face;

        // Copy current calibration data
        int normalCount = BoardManager::getBoard()->ledCount;
        float3 calibratedNormalsCopy[normalCount];
        memcpy(calibratedNormalsCopy, SettingsManager::getSettings()->faceNormals, normalCount * sizeof(float3));

        // Replace the face's normal with what we measured
        readAccelerometer(&calibratedNormalsCopy[face]);

        // And flash the new normals
        SettingsManager::programCalibrationData(calibratedNormalsCopy, normalCount, [](bool result)
                                                { MessageService::NotifyUser("Face calibrated", true, false, 5, nullptr); });
    }

    void onSettingsProgrammingEvent(void *context, Flash::ProgrammingEventType evt)
    {
        if (evt == Flash::ProgrammingEventType_Begin)
        {
            NRF_LOG_DEBUG("Stopping axel from programming event");
            stop();
        }
        else
        {
            NRF_LOG_DEBUG("Starting axel from programming event");
            start();
        }
    }

    void readAccelerometer(float3 *acc)
    {
        AccelChip::read(acc);
    }

    // Interrupt Callback Storage
    static void* interruptParam = nullptr;
    static AccelerometerInterruptMethod interruptCallback = nullptr;
    void enableInterrupt(AccelerometerInterruptMethod callback, void* param)
    {
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

    void disableInterrupt()
    {
        GPIOTE::disableInterrupt(BoardManager::getBoard()->accInterruptPin);
        AccelChip::disableInterrupt();
    }
}

#include "accelerometer.h"

#include "drivers_hw/lis2de12.h"
#include "drivers_hw/kxtj3-1057.h"
#include "drivers_hw/mxc4005xc.h"
#include "utils/utils.h"
#include "core/ring_buffer.h"
#include "config/board_config.h"
#include "config/settings.h"
#include "config/dice_variants.h"
#include "app_timer.h"
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

using namespace Modules;
using namespace Core;
using namespace DriversHW;
using namespace DriversNRF;
using namespace Config;
using namespace Bluetooth;

// This defines how frequently we try to read the accelerometer
#define TIMER2_RESOLUTION (1000)	// ms
#define JERK_SCALE (1000)		// To make the jerk in the same range as the acceleration
#define MAX_ACC_CLIENTS 8

namespace Modules
{
namespace Accelerometer
{
	APP_TIMER_DEF(accelControllerTimer);

	int face;
	float confidence;
	float sigma;
	float3 smoothAcc;
	RollState rollState = RollState_Unknown;
	bool moving = false;
	float3 handleStateNormal; // The normal when we entered the handled state, so we can determine if we've moved enough
	bool paused;
    Config::AccelerometerModel accelerometerModel;

	// This small buffer stores about 1 second of Acceleration data
	Core::RingBuffer<AccelFrame, ACCEL_BUFFER_SIZE> buffer;

	DelegateArray<FrameDataClientMethod, MAX_ACC_CLIENTS> frameDataClients;
	DelegateArray<RollStateClientMethod, MAX_ACC_CLIENTS> rollStateClients;

	void updateState();
	void pauseNotifications();
	void resumeNotifications();

    void CalibrateHandler(void* context, const Message* msg);
	void CalibrateFaceHandler(void* context, const Message* msg);
	void onSettingsProgrammingEvent(void* context, Flash::ProgrammingEventType evt);
	void onPowerEvent(void* context, nrf_pwr_mgmt_evt_t event);
    void readAccelerometer(float3* acc);
    bool checkIntPin();

	void update(void* context);

    void init() {
        MessageService::RegisterMessageHandler(Message::MessageType_Calibrate, nullptr, CalibrateHandler);
        MessageService::RegisterMessageHandler(Message::MessageType_CalibrateFace, nullptr, CalibrateFaceHandler);

		Flash::hookProgrammingEvent(onSettingsProgrammingEvent, nullptr);

        auto board = Config::BoardManager::getBoard();
        accelerometerModel = board->accModel;

		face = 0;
		confidence = 0.0f;
		smoothAcc = float3::zero();

		AccelFrame newFrame;
		readAccelerometer(&newFrame.acc);
		newFrame.time = DriversNRF::Timers::millis();
		newFrame.jerk = float3::zero();
		newFrame.sigma = 0.0f;
		newFrame.smoothAcc = newFrame.acc;
		buffer.push(newFrame);

		// Attach to the power manager, so we can wake the device up
		PowerManager::hook(onPowerEvent, nullptr);

		// Create the accelerometer timer
		ret_code_t ret_code = app_timer_create(&accelControllerTimer, APP_TIMER_MODE_REPEATED, update);
		APP_ERROR_CHECK(ret_code);

		if (!checkIntPin())
		{
			NRF_LOG_ERROR("Bad interrupt Pin");
			return;
		}

        enableInterrupt();

		start();
		NRF_LOG_INFO("Accelerometer initialized with accelerometerModel=%d", (int)accelerometerModel);
	}

	/// <summary>
	/// update is called from the timer
	/// </summary>
	void update(void* context) {

		auto settings = SettingsManager::getSettings();
		auto& lastFrame = buffer.last();


		AccelFrame newFrame;
        readAccelerometer(&newFrame.acc);
		newFrame.time = DriversNRF::Timers::millis();
		newFrame.jerk = ((newFrame.acc - lastFrame.acc) * 1000.0f) / (float)(newFrame.time - lastFrame.time);

		float jerkMag = newFrame.jerk.sqrMagnitude();
		if (jerkMag > 10.f) {
			jerkMag = 10.f;
		}
		sigma = sigma * settings->sigmaDecay + jerkMag * (1.0f - settings->sigmaDecay);
		newFrame.sigma = sigma;

		smoothAcc = smoothAcc * settings->accDecay + newFrame.acc * (1.0f - settings->accDecay);
		newFrame.smoothAcc = smoothAcc;
		newFrame.face = determineFace(newFrame.acc, &newFrame.faceConfidence);

		buffer.push(newFrame);

		// Notify clients
		for (int i = 0; i < frameDataClients.Count(); ++i)
		{
			frameDataClients[i].handler(frameDataClients[i].token, newFrame);
		}

		bool startMoving = sigma > settings->startMovingThreshold;
		bool stopMoving = sigma < settings->stopMovingThreshold;
		bool onFace = newFrame.faceConfidence > settings->faceThreshold;
		bool zeroG = newFrame.acc.sqrMagnitude() < (settings->fallingThreshold * settings->fallingThreshold);
        bool shock = newFrame.acc.sqrMagnitude() > (settings->shockThreshold * settings->shockThreshold);

        RollState newRollState = rollState;
        switch (rollState) {
            case RollState_Unknown:
            case RollState_OnFace:
            case RollState_Crooked:
                // We start rolling if we detect enough motion
                if (startMoving) {
                    // We're at least being handled
                    newRollState = RollState_Handling;
					handleStateNormal = newFrame.acc.normalized();
                }
                break;
            case RollState_Handling:
				// Did we move enough?
				{
					bool rotatedEnough = float3::dot(newFrame.acc.normalized(), handleStateNormal) < 0.5f;
					if (shock || zeroG || rotatedEnough) {
						// Stuff is happening that we are most likely rolling now
						newRollState = RollState_Rolling;
					} else if (stopMoving) {
						// Just slid the dice around?
						if (stopMoving) {
							if (BoardManager::getBoard()->ledCount == 6) {
								// We may be at rest
								if (onFace) {
									// We're at rest
									newRollState = RollState_OnFace;
								} else {
									newRollState = RollState_Crooked;
								}
							} else {
								newRollState = RollState_OnFace;
							}
						}
					}
				}
				break;
			case RollState_Rolling:
                // If we stop moving we may be on a face
                if (stopMoving) {
					if (BoardManager::getBoard()->ledCount == 6) {
						// We may be at rest
						if (onFace) {
							// We're at rest
							newRollState = RollState_OnFace;
						} else {
							newRollState = RollState_Crooked;
						}
					} else {
						newRollState = RollState_OnFace;
					}
                }
                break;
            default:
                break;
        }

		//if (newFrame.face != face || newRollState != rollState) {
		if (newFrame.face != face) {
			face = newFrame.face;
			confidence = newFrame.faceConfidence;

			//NRF_LOG_INFO("Face %d, confidence " NRF_LOG_FLOAT_MARKER, face, NRF_LOG_FLOAT(confidence));
		}

		if (newRollState != rollState) {

			// Debugging
			//BLE_LOG_INFO("Face Normal: %d, %d, %d", (int)(newFrame.acc.x * 100), (int)(newFrame.acc.y * 100), (int)(newFrame.acc.z * 100));

			if (newRollState != rollState) {
				NRF_LOG_INFO("State: %s", getRollStateString(newRollState));
				rollState = newRollState;
			}

			// Notify clients
			if (!paused) {
				for (int i = 0; i < rollStateClients.Count(); ++i) {
					rollStateClients[i].handler(rollStateClients[i].token, rollState, face);
				}
			}
		}
	}

	/// <summary>
	/// Initialize the acceleration system
	/// </summary>
	void start()
	{
		NRF_LOG_INFO("Starting accelerometer");
		// Set initial value
		float3 acc;
        readAccelerometer(&acc);
		face = determineFace(acc, &confidence);

		// Determine what state we're in to begin with
		auto settings = SettingsManager::getSettings();
		bool onFace = confidence > settings->faceThreshold;
        if (onFace) {
            rollState = RollState_OnFace;
        } else {
            rollState = RollState_Crooked;
        }

		ret_code_t ret_code = app_timer_start(accelControllerTimer, APP_TIMER_TICKS(TIMER2_RESOLUTION), NULL);
		APP_ERROR_CHECK(ret_code);
	}

	/// <summary>
	/// Stop getting updated from the timer
	/// </summary>
	void stop()
	{
		ret_code_t ret_code = app_timer_stop(accelControllerTimer);
		APP_ERROR_CHECK(ret_code);
		NRF_LOG_INFO("Stopped accelerometer");
	}

	/// <summary>
	/// Returns the currently stored up face!
	/// </summary>
	int currentFace() {
		return face;
	}

	float currentFaceConfidence() {
		return confidence;
	}

	RollState currentRollState() {
		return rollState;
	}

	const char* getRollStateString(RollState state) {
	#if defined(DEBUG)
		switch (state) {
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
	/// Crudely compares accelerometer readings passed in to determine the current face up
	/// </summary>
	/// <returns>The face number, starting at 0</returns>
	int determineFace(float3 acc, float* outConfidence)
	{
		// Compare against face normals stored in board manager
		int faceCount = BoardManager::getBoard()->ledCount;
		auto settings = SettingsManager::getSettings();
		auto& normals = settings->faceNormals;
		float accMag = acc.magnitude();
		if (accMag < settings->fallingThreshold) {
			if (outConfidence != nullptr) {
				*outConfidence = 0.0f;
			}
			return face;
		} else {
			float3 nacc  = acc / accMag; // normalize
			float bestDot = -1000.0f;
			int bestFace = -1;
			for (int i = 0; i < faceCount; ++i) {
				float dot = float3::dot(nacc, normals[i]);
				if (dot > bestDot) {
					bestDot = dot;
					bestFace = i;
				}
			}
			if (outConfidence != nullptr) {
				*outConfidence = bestDot;
			}
			return bestFace;
		}
	}

	/// <summary>
	/// Method used by clients to request timer callbacks when accelerometer readings are in
	/// </summary>
	void hookFrameData(Accelerometer::FrameDataClientMethod callback, void* parameter)
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
	void unHookFrameDataWithParam(void* param)
	{
		frameDataClients.UnregisterWithToken(param);
	}

	void hookRollState(RollStateClientMethod method, void* param)
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

	void unHookRollStateWithParam(void* param)
	{
		rollStateClients.UnregisterWithToken(param);
	}

	struct CalibrationNormals
	{
		float3 face1;
		float3 face5;
		float3 face10;
		float3 led0;
		float3 led1;

		// Set to true if connection is lost to stop calibration
		bool calibrationInterrupted;

		CalibrationNormals() : calibrationInterrupted(false) {}
	};
	CalibrationNormals* measuredNormals = nullptr;
	void onConnectionLost(void* param, bool connected)
	{
		if (!connected)
			measuredNormals->calibrationInterrupted = true;
	}

    void CalibrateHandler(void* context, const Message* msg) {

		// Turn off state change notifications
		stop();
		Bluetooth::Stack::hook(onConnectionLost, nullptr);

		// Helper to restart accelerometer and clean up
		static auto restart = []() {
			Bluetooth::Stack::unHook(onConnectionLost);
			start();
		};

		// Start calibration!
		measuredNormals = (CalibrationNormals*)malloc(sizeof(CalibrationNormals));

		// Ask user to place die on face 1
		MessageService::NotifyUser("Place face 1 up", true, true, 30, [] (bool okCancel)
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

								// Debugging
								//BLE_LOG_INFO("Face 10 Normal: %d, %d, %d", (int)(measuredNormals->face10.x * 100), (int)(measuredNormals->face10.y * 100), (int)(measuredNormals->face10.z * 100));
								LEDs::setPixelColor(0, 0x00FF00);

								// Place on led index 0
								MessageService::NotifyUser("Place lit face up", true, true, 30, [] (bool okCancel)
								{
									LEDs::setPixelColor(0, 0x000000);
									if (okCancel) {
										// Read the normals
										readAccelerometer(&measuredNormals->led0);


										LEDs::setPixelColor(9, 0x00FF00);
										// Place on led index 1
										MessageService::NotifyUser("Place lit face up", true, true, 30, [] (bool okCancel)
										{
											LEDs::setPixelColor(9, 0x000000);
											if (okCancel) {
												// Read the normals
												readAccelerometer(&measuredNormals->led1);

												// Now we can calibrate

												// From the 3 measured normals we can calibrate the accelerometer
												auto b = BoardManager::getBoard();
												auto l = DiceVariants::getLayouts(b->ledCount);

												float3 newNormals[b->ledCount];
												int layoutVersionIndex = Utils::CalibrateNormals(
													0, measuredNormals->face1,
													4, measuredNormals->face5,
													9, measuredNormals->face10,
													l, newNormals, b->ledCount);

												// Now figure out the base remapping based on how the electronics is rotated inside the dice
												auto ll = l->layouts[layoutVersionIndex];

												uint8_t newFaceToLEDLookup[b->ledCount];
												if (Utils::CalibrateInternalRotation(
													0, measuredNormals->led0,
													9, measuredNormals->led1,
													newNormals, ll, newFaceToLEDLookup, b->ledCount)) {

													// And flash the new normals
													SettingsManager::programCalibrationData(newNormals, layoutVersionIndex, newFaceToLEDLookup, b->ledCount, [] (bool result) {

														// Notify user that we're done, yay!!!
														MessageService::NotifyUser("Calibrated", false, false, 30, nullptr);

														// Restart notifications
														restart();
													});
												} else {
													// Notify user
													MessageService::NotifyUser("Not calibrated", false, false, 30, nullptr);

													// Restart notifications
													restart();
												}
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

	void CalibrateFaceHandler(void* context, const Message* msg) {
		const MessageCalibrateFace* faceMsg = (const MessageCalibrateFace*)msg;
		uint8_t face = faceMsg->face;

		// Copy current calibration data
		int normalCount = BoardManager::getBoard()->ledCount;
		float3 calibratedNormalsCopy[normalCount];
		memcpy(calibratedNormalsCopy, SettingsManager::getSettings()->faceNormals, normalCount * sizeof(float3));
		uint8_t ftlCopy[normalCount];
		memcpy(ftlCopy, SettingsManager::getSettings()->faceToLEDLookup, normalCount * sizeof(uint8_t));

		// Replace the face's normal with what we measured
		readAccelerometer(&calibratedNormalsCopy[face]);

		// And flash the new normals
		int fli = SettingsManager::getSettings()->faceLayoutLookupIndex;
		SettingsManager::programCalibrationData(calibratedNormalsCopy, fli, ftlCopy, normalCount, [] (bool result) {
			MessageService::NotifyUser("Face calibrated", true, false, 5, nullptr);
		});
	}

	void onSettingsProgrammingEvent(void* context, Flash::ProgrammingEventType evt){
		if (evt == Flash::ProgrammingEventType_Begin) {
			stop();
		} else {
			start();
		}
	}

	bool interruptTriggered = false;
	void accInterruptHandler(uint32_t pin, nrf_gpiote_polarity_t action) {
		// Acknowledge the interrupt and then disable it
        clearInterrupt();
//        disableInterrupt();
        Scheduler::push(nullptr, 0, [](void * p_event_data, uint16_t event_size) {
            NRF_LOG_INFO("Interrupt!!!");
        });
	}

	void onPowerEvent(void* context, nrf_pwr_mgmt_evt_t event) {
		if (event == NRF_PWR_MGMT_EVT_PREPARE_WAKEUP) {
			// Setup interrupt to wake the device up
			NRF_LOG_INFO("Setting accelerometer to trigger interrupt");

			// Set interrupt pin
            nrf_gpio_cfg_sense_input(BoardManager::getBoard()->accInterruptPin, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

            enableInterrupt();
		}
	}

    void readAccelerometer(float3* acc) {
        switch (accelerometerModel) {
            case Config::AccelerometerModel::LID2DE12:
                LIS2DE12::read(acc);
                break;
            case Config::AccelerometerModel::KXTJ3_1057:
                KXTJ3::read(acc);
                break;
            case Config::AccelerometerModel::MXC4005XC:
                MXC4005XC::read(acc);
                break;
            default:
                NRF_LOG_ERROR("Invalid Accelerometer Model");
                break;
        }
    }

    void enableInterrupt() {
        switch (accelerometerModel) {
            case Config::AccelerometerModel::LID2DE12:
                LIS2DE12::enableInterrupt();
                break;
            case Config::AccelerometerModel::KXTJ3_1057:
                KXTJ3::enableInterrupt();
                break;
            case Config::AccelerometerModel::MXC4005XC:
                MXC4005XC::enableInterrupt();
                break;
            default:
                NRF_LOG_ERROR("Invalid Accelerometer Model");
                break;
        }
    }

    void disableInterrupt() {
        switch (accelerometerModel) {
            case Config::AccelerometerModel::LID2DE12:
                LIS2DE12::disableInterrupt();
                break;
            case Config::AccelerometerModel::KXTJ3_1057:
                KXTJ3::disableInterrupt();
                break;
            case Config::AccelerometerModel::MXC4005XC:
                MXC4005XC::disableInterrupt();
                break;
            default:
                NRF_LOG_ERROR("Invalid Accelerometer Model");
                break;
        }
    }

    void clearInterrupt() {
        switch (accelerometerModel) {
            case Config::AccelerometerModel::LID2DE12:
                LIS2DE12::clearInterrupt();
                break;
            case Config::AccelerometerModel::KXTJ3_1057:
                KXTJ3::clearInterrupt();
                break;
            case Config::AccelerometerModel::MXC4005XC:
                MXC4005XC::clearInterrupt();
                break;
            default:
                NRF_LOG_ERROR("Invalid Accelerometer Model");
                break;
        }
    }

	bool checkIntPin() {
        nrf_gpio_cfg_input(BoardManager::getBoard()->accInterruptPin, NRF_GPIO_PIN_NOPULL);
        bool ret = nrf_gpio_pin_read(BoardManager::getBoard()->accInterruptPin) == 0;
        nrf_gpio_cfg_default(BoardManager::getBoard()->accInterruptPin);
		return ret;
	}
}
}

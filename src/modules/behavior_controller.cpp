#include "behavior_controller.h"
#include "bluetooth/bluetooth_stack.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/power_manager.h"
#include "modules/battery_controller.h"
#include "modules/accelerometer.h"
#include "config/settings.h"
#include "profile/profile_static.h"
#include "nrf_log.h"
#include "die.h"
#include "nrf_assert.h"

using namespace Bluetooth;
using namespace Animations;
using namespace Config;
using namespace DriversNRF;
using namespace Profile;

#define CONDITION_RECHECK_MAX 8
#define BATT_TOO_LOW_LEVEL 50 // 50%

namespace Modules::BehaviorController
{
    void onConnectionEvent(void* param, bool connected);
    void onBatteryStateChange(void* param, BatteryController::BatteryState newState);
    void onRollStateChange(void* param, Accelerometer::RollState newState, int newFace);

	void init() {

		// Hook up the behavior controller to all the events it needs to know about to do its job!
        Bluetooth::Stack::hook(onConnectionEvent, nullptr);
        BatteryController::hookBatteryState(onBatteryStateChange, nullptr);
        NRF_LOG_DEBUG("Behavior Controller init");
    }

    void onPixelInitialized() {

        auto data = Profile::Static::getData();

        // Trigger battery state rule
        auto battState = BatteryController::getBatteryState();
        if (battState == BatteryController::BatteryState_Charging) {
            onBatteryStateChange(nullptr, battState);
        } else {
            // Iterate the rules and look for one!
            for (int i = 0; i < data->getRuleCount(); ++i) {
                auto rule = data->getRule(i);
                auto condition = data->getCondition(rule->condition);
                if (condition->type == Behaviors::Condition_HelloGoodbye) {
                    // This is the right kind of condition, check it!
                    auto cond = static_cast<const Behaviors::ConditionHelloGoodbye*>(condition);
                    if (cond->checkTrigger(true)) {
                        // Go on, do the thing!
                        if (PowerManager::checkFromSysOff()) 
                        {
                            NRF_LOG_DEBUG("Skipping HelloGoodbye Condition");
                        }
                        else
                        {
                            NRF_LOG_DEBUG("Triggering a HelloGoodbye Condition");
                            data->TriggerActions(rule->actions, Animations::AnimationTag_Status);
                        }
                    }
                }
            }
        }
    }

	void onConnectionEvent(void* param, bool connected) {
        auto data = Profile::Static::getData();
        // Iterate the rules and look for one!
        for (int i = 0; i < data->getRuleCount(); ++i) {
            auto rule = data->getRule(i);
            auto condition = data->getCondition(rule->condition);
            if (condition->type == Behaviors::Condition_ConnectionState) {
                // This is the right kind of condition, check it!
                auto cond = static_cast<const Behaviors::ConditionConnectionState*>(condition);
                if (cond->checkTrigger(connected)) {
                    NRF_LOG_DEBUG("Triggering a Connection State Condition");
                    // Go on, do the thing!
                    data->TriggerActions(rule->actions, Animations::AnimationTag_BluetoothNotification);
                    // We're done!
                    break;
                }
            }
        }
    }

    bool processBatteryStateRule(int ruleIndex, BatteryController::BatteryState newState);

    void processBatteryStateRuleCallback(void* param) {
        // Recheck ourselves!
        BatteryController::BatteryState newState = BatteryController::getBatteryState();
        processBatteryStateRule((int)param, newState);
    }

    bool processBatteryStateRule(int ruleIndex, BatteryController::BatteryState newState) {
        auto data = Profile::Static::getData();
        auto rule = data->getRule(ruleIndex);
        auto condition = data->getCondition(rule->condition);
        ASSERT(condition->type == Behaviors::Condition_BatteryState);

        // This is the right kind of condition, check it!
        auto cond = static_cast<const Behaviors::ConditionBatteryState*>(condition);
        bool ret = cond->checkTrigger(newState);
        if (ret) {
            NRF_LOG_DEBUG("Triggering a Battery State Condition");
            
            // Setup a timer to repeat this check in a little bit if appropriate
            if (cond->repeatPeriodMs != 0) {
                // If we had any other battery-rule related delayed callback, cancel them
                Timers::cancelDelayedCallback(processBatteryStateRuleCallback);

                // And trigger ourselves to check this condition again!
                Timers::setDelayedCallback(processBatteryStateRuleCallback, (void*)ruleIndex, cond->repeatPeriodMs);
            }

            // Go on, do the thing!
            data->TriggerActions(rule->actions, Animations::AnimationTag_BatteryNotification);
        }
        return ret;
    }

    void onBatteryStateChange(void* param, BatteryController::BatteryState newState) {
        auto data = Profile::Static::getData();
        // Do we have a battery event condition?
        // Iterate the rules and look for one!
        for (int i = 0; i < data->getRuleCount(); ++i) {
            auto rule = data->getRule(i);
            auto condition = data->getCondition(rule->condition);
            if (condition->type == Behaviors::Condition_BatteryState) {

                if (processBatteryStateRule(i, newState)) {
                    break;
                }
            }
        }
    }

	void DisableAccelerometerRules() {
        Accelerometer::unHookRollState(onRollStateChange);
    }

	void EnableAccelerometerRules() {
        Accelerometer::hookRollState(onRollStateChange, nullptr);
    }

    void onRollStateChange(void* param, Accelerometer::RollState newState, int newFace) {

        auto data = Profile::Static::getData();
        // Do we have a roll state event condition?
        // Iterate the rules and look for one!
        for (int i = 0; i < data->getRuleCount(); ++i) {
            auto rule = data->getRule(i);
            auto condition = data->getCondition(rule->condition);

            // This is the right kind of condition, check it!
            bool conditionTriggered = false;
            switch (condition->type) {
                case Behaviors::Condition_Handling:
                    conditionTriggered = static_cast<const Behaviors::ConditionHandling*>(condition)->checkTrigger(newState, newFace);
                    break;
                case Behaviors::Condition_Rolling:
                    conditionTriggered = static_cast<const Behaviors::ConditionRolling*>(condition)->checkTrigger(newState, newFace);
                    break;
                case Behaviors::Condition_Crooked:
                    conditionTriggered = static_cast<const Behaviors::ConditionCrooked*>(condition)->checkTrigger(newState, newFace);
                    break;
                case Behaviors::Condition_FaceCompare:
                    conditionTriggered = static_cast<const Behaviors::ConditionFaceCompare*>(condition)->checkTrigger(newState, newFace);
                    break;
                default:
                    break;
            }

            if (conditionTriggered) {
                // do the thing
                data->TriggerActions(rule->actions, Animations::AnimationTag_Accelerometer);

                // We're done
                break;
            }
        }
    }
}

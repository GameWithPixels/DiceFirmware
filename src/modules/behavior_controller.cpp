#include "behavior_controller.h"
#include "bluetooth/bluetooth_stack.h"
#include "drivers_nrf/timers.h"
#include "drivers_nrf/power_manager.h"
#include "data_set/data_set.h"
#include "modules/battery_controller.h"
#include "modules/accelerometer.h"
#include "data_set/data_set.h"
#include "config/settings.h"
#include "nrf_log.h"
#include "die.h"
#include "nrf_assert.h"

using namespace Bluetooth;
using namespace Animations;
using namespace Config;
using namespace DriversNRF;
using namespace DataSet;

#define CONDITION_RECHECK_MAX 8
#define BATT_TOO_LOW_LEVEL 50 // 50%

namespace Modules::BehaviorController
{
    void onConnectionEvent(void* param, bool connected);
    void onBatteryStateChange(void* param, BatteryController::BatteryState newState);
    void onRollStateChange(void* param, Accelerometer::RollState prevState, int prevFace, Accelerometer::RollState newState, int newFace);

    int lastRollStateTimestamp;
    int disableAccelerometerRulesCount;
    int disableBatteryRulesCount;
    int disableConnectionRulesCount;

    void init(bool enableAccelerometerRules, bool enableBatteryRules, bool enableConnectionRules) {

        // Hook up the behavior controller to all the events it needs to know about to do its job!
        lastRollStateTimestamp = Timers::millis();
        disableAccelerometerRulesCount = 1;
        if (enableAccelerometerRules) {
            EnableAccelerometerRules();
        }
        disableBatteryRulesCount = 1;
        if (enableBatteryRules) {
            EnableBatteryRules();
        }
        disableConnectionRulesCount = 1;
        if (enableConnectionRules) {
            EnableConnectionRules();
        }

        NRF_LOG_DEBUG("Behavior Controller init");
    }

    void DisableAccelerometerRules() {
        disableAccelerometerRulesCount += 1;
        if (disableAccelerometerRulesCount == 1) {
            Accelerometer::unHookRollState(onRollStateChange);
        }
    }

    void EnableAccelerometerRules() {
        disableAccelerometerRulesCount -= 1;
        if (disableAccelerometerRulesCount == 0) {
            Accelerometer::hookRollState(onRollStateChange, nullptr);
        }
    }

    void DisableBatteryRules() {
        disableBatteryRulesCount += 1;
        if (disableBatteryRulesCount == 1) {
            BatteryController::unHookBatteryState(onBatteryStateChange);
        }
    }

    void EnableBatteryRules() {
        disableBatteryRulesCount -= 1;
        if (disableBatteryRulesCount == 0) {
            BatteryController::hookBatteryState(onBatteryStateChange, nullptr);
        }
    }

    void DisableConnectionRules() {
        disableConnectionRulesCount += 1;
        if (disableConnectionRulesCount == 1) {
            Bluetooth::Stack::unHook(onConnectionEvent);
        }
    }

    void EnableConnectionRules() {
        disableConnectionRulesCount -= 1;
        if (disableConnectionRulesCount == 0) {
            Bluetooth::Stack::hook(onConnectionEvent, nullptr);
        }
    }

    void onPixelInitialized() {

        // Do we have a hello goodbye condition
        auto bhv = DataSet::getBehavior();

        if (!forceCheckBatteryState()) {
            // Iterate the rules and look for one!
            for (int i = 0; i < bhv->rulesCount; ++i) {
                auto rule = DataSet::getRule(bhv->rulesOffset + i);
                auto condition = DataSet::getCondition(rule->condition);
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
                            Behaviors::triggerActions(rule->actionOffset, rule->actionCount, Animations::AnimationTag_Status);
                        }
                    }
                }
            }
        }
    }

    bool forceCheckBatteryState() {
        // Trigger battery state rule if applicable
        auto battState = BatteryController::getBatteryState();
        bool ret = battState == BatteryController::BatteryState_Charging;
        if (ret) {
            onBatteryStateChange(nullptr, battState);
        }
        return ret;
    }

    void onConnectionEvent(void* param, bool connected) {
        // Do we have a connection event condition?
        auto bhv = DataSet::getBehavior();

        // Iterate the rules and look for one!
        for (int i = 0; i < bhv->rulesCount; ++i) {
            auto rule = DataSet::getRule(bhv->rulesOffset + i);
            auto condition = DataSet::getCondition(rule->condition);
            if (condition->type == Behaviors::Condition_ConnectionState) {
                // This is the right kind of condition, check it!
                auto cond = static_cast<const Behaviors::ConditionConnectionState*>(condition);
                if (cond->checkTrigger(connected)) {
                    NRF_LOG_DEBUG("Triggering a Connection State Condition");
                    // Go on, do the thing!
                    Behaviors::triggerActions(rule->actionOffset, rule->actionCount, Animations::AnimationTag_BluetoothNotification);
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
        auto rule = DataSet::getRule(ruleIndex);
        auto condition = DataSet::getCondition(rule->condition);
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
            Behaviors::triggerActions(rule->actionOffset, rule->actionCount, Animations::AnimationTag_BatteryNotification);
        }
        return ret;
    }

    void onBatteryStateChange(void* param, BatteryController::BatteryState newState) {
        // Do we have a battery event condition?
        auto bhv = DataSet::getBehavior();

        // Iterate the rules and look for one!
        for (int i = 0; i < bhv->rulesCount; ++i) {
            auto rule = DataSet::getRule(bhv->rulesOffset + i);
            auto condition = DataSet::getCondition(rule->condition);
            if (condition->type == Behaviors::Condition_BatteryState) {

                processBatteryStateRule(i, newState);
            }
        }
    }

    void onRollStateChange(void* param, Accelerometer::RollState prevState, int prevFace, Accelerometer::RollState newState, int newFace) {

        // Do we have a roll state event condition?
        auto bhv = DataSet::getBehavior();

        // Iterate the rules and look for one!
        for (int i = 0; i < bhv->rulesCount; ++i) {
            auto rule = DataSet::getRule(bhv->rulesOffset + i);
            auto condition = DataSet::getCondition(rule->condition);

            // This is the right kind of condition, check it!
            bool conditionTriggered = false;
            switch (condition->type) {
                case Behaviors::Condition_Handling:
                    conditionTriggered = static_cast<const Behaviors::ConditionHandling*>(condition)->checkTrigger(newState, newFace);
                    break;
                case Behaviors::Condition_Rolling:
                    {
                        auto rollingCondition = static_cast<const Behaviors::ConditionRolling*>(condition);
                        int timestamp = Timers::millis();
                        if (timestamp - lastRollStateTimestamp > rollingCondition->repeatPeriodMs) {
                            conditionTriggered = rollingCondition->checkTrigger(newState, newFace);
                            if (conditionTriggered) {
                                lastRollStateTimestamp = timestamp;
                            }
                        }
                    }
                    break;
                case Behaviors::Condition_Crooked:
                    conditionTriggered = static_cast<const Behaviors::ConditionCrooked*>(condition)->checkTrigger(newState, newFace);
                    break;
                case Behaviors::Condition_FaceCompare:
                    conditionTriggered = static_cast<const Behaviors::ConditionFaceCompare*>(condition)->checkTrigger(newState, newFace);
                    break;
                case Behaviors::Condition_Rolled:
                    conditionTriggered = static_cast<const Behaviors::ConditionRolled*>(condition)->checkTrigger(prevState, prevFace, newState, newFace);
                    break;
                default:
                    break;
            }

            if (conditionTriggered) {
                // do the thing
                Behaviors::triggerActions(rule->actionOffset, rule->actionCount, Animations::AnimationTag_Accelerometer);
            }
        }
    }
}

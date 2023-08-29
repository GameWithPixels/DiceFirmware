#include "profile_data.h"
#include "config/board_config.h"
#include "utils/utils.h"
#include "drivers_nrf/flash.h"
#include "nrf_log.h"
#include "config/settings.h"

#include "animations/animations/animation_rainbow.h"
#include "animations/animations/animation_simple.h"
#include "behaviors/condition.h"
#include "behaviors/action.h"
#include "behaviors/behavior.h"

using namespace DriversNRF;
using namespace Config;
using namespace Animations;
using namespace Behaviors;

//#define USE_BINARY_BUFFER_IMAGE 1

namespace Profile
{
    #if USE_BINARY_BUFFER_IMAGE
    uint8_t defaultBytes[] =
    {
        0x0D, 0xF0, 0x0D, 0x60, 0x04, 0x00, 0x00, 0x00,
        0x42, 0x01, 0x7C, 0x00, 0x09, 0x15, 0x01, 0x09,
        0x0D, 0xF0, 0x0D, 0x60, 0x02, 0x01, 0xB8, 0x0B,
        0xFF, 0xFF, 0xFF, 0xFF, 0x03, 0x40, 0x80, 0x00,
        0x01, 0x00, 0xB8, 0x0B, 0x00, 0x00, 0x10, 0x00,
        0x00, 0x00, 0x08, 0x00, 0x01, 0xFF, 0x01, 0x00,
        0xDC, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
        0x08, 0x00, 0x03, 0xFF, 0x01, 0x00, 0xD0, 0x07,
        0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00,
        0x0A, 0xFF, 0x01, 0x00, 0x10, 0x27, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x08, 0x00, 0x00, 0x01, 0x20,
        0x01, 0x00, 0xE8, 0x03, 0xFF, 0xFF, 0xFF, 0xFF,
        0x08, 0x00, 0x00, 0x00, 0x01, 0xFF, 0x01, 0x00,
        0x64, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08, 0x08,
        0x08, 0x00, 0x01, 0x00, 0x01, 0x00, 0xB8, 0x0B,
        0xFF, 0xFF, 0xFF, 0xFF, 0x08, 0x08, 0x08, 0x00,
        0x01, 0x00, 0x01, 0x00, 0xE8, 0x03, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x06, 0x06, 0x00, 0x03, 0xFF,
        0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x06, 0x03,
        0x00, 0x00, 0x03, 0x00, 0xF4, 0x01, 0x04, 0x00,
        0x06, 0x00, 0x07, 0x02, 0x30, 0x75, 0x07, 0x04,
        0x88, 0x13, 0x07, 0x08, 0x88, 0x13, 0x07, 0x10,
        0x00, 0x00, 0x07, 0x20, 0xDC, 0x05, 0x01, 0x00,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xB2,
        0x00, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xBD, 0x00, 0x01, 0x00, 0x00, 0x01,
        0x52, 0x00, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x01,
        0x00, 0x00, 0x01, 0x60, 0x00, 0x00, 0x00, 0x00,
        0xD3, 0x00, 0x01, 0x00, 0x00, 0x01, 0x1A, 0x00,
        0x00, 0x00, 0x00, 0xDE, 0x00, 0x01, 0x00, 0x00,
        0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xE9, 0x00,
        0x01, 0x00, 0x00, 0x01, 0x36, 0x00, 0x00, 0x00,
        0x00, 0xF4, 0x00, 0x01, 0x00, 0x00, 0x01, 0x28,
        0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x01, 0x00,
        0x00, 0x01, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x0A,
        0x01, 0x8E, 0x00, 0xBB, 0x00, 0x01, 0x96, 0x00,
        0xD1, 0x00, 0x01, 0x9A, 0x00, 0xDC, 0x00, 0x01,
        0xA2, 0x00, 0xF2, 0x00, 0x01, 0xAA, 0x00, 0x08,
        0x01, 0x01, 0x9E, 0x00, 0xE7, 0x00, 0x01, 0xA6,
        0x00, 0xFD, 0x00, 0x01, 0x92, 0x00, 0xC6, 0x00,
        0x01, 0xAE, 0x00, 0x13, 0x01, 0x01, 0xE9, 0x21,
        0xFC, 0xE0, 0xF1, 0xA0
    };

    uint8_t* Data::CreateDefaultProfile(uint32_t* outSize) {
        *outSize = sizeof(defaultBytes);
        return defaultBytes;
    }

    void Data::DestroyDefaultProfile(uint8_t* ptr) {
        // Do nothing
    }

    #else

    class ProfileBufferAllocator
    {
        uint8_t* buffer;
        uint32_t allocSize;
        uint16_t currentOffset;

    public:
        ProfileBufferAllocator(uint8_t* memory, uint32_t size)
        : buffer(memory)
        , allocSize(size)
        , currentOffset(0)
        {
        }

        BufferDescriptor getDescriptor() {
            BufferDescriptor ret;
            ret.start = buffer;
            ret.size = allocSize;
            return ret;
        }

        uint16_t currentSize() const {
            return currentOffset;
        }

        template <typename T>
        T* get(Pointer<T> ptr) {
            uint8_t* ret = buffer + ptr.offset;
            return reinterpret_cast<T*>(ret);
        }

        template <typename T>
        T* getAt(Array<T> ptr, uint8_t index) {
            uint8_t* ret = buffer + ptr.offset + index * sizeof(T);
            return reinterpret_cast<T*>(ret);
        }

        template <typename T>
        void setAt(Array<T> ptr, uint8_t index, const T& value) {
            T* item = reinterpret_cast<T*>(buffer + ptr.offset + index * sizeof(T));
            *item = value;
        }

        template <typename T>
        Pointer<T> allocatePtr() {
            Pointer<T> ret;
            if (currentOffset + sizeof(T) <= allocSize) {
                ret.offset = currentOffset;
                new (buffer + currentOffset) T();
                currentOffset += sizeof(T);
            } else {
                NRF_LOG_ERROR("Not enough memory");
                ret.offset = 0;
            }
            return ret;
        }

        template <typename T>
        Array<T> allocateArray(int length) {
            Array<T> ret;
            if (currentOffset + sizeof(T) * length <= allocSize) {
                ret.offset = currentOffset;
                ret.length = length;
                new (buffer + currentOffset) T[length];
                currentOffset += sizeof(T) * length;
            } else {
                NRF_LOG_ERROR("Not enough memory");
                ret.offset = 0;
                ret.length = 0;
            }
            return ret;
        }
    };

	uint8_t* Data::CreateDefaultProfile(uint32_t* outSize) {

        // Create empty profile for now
        uint32_t mallocSize = 512; // <-- make sure this is big enough to hold the data we put in!
        uint8_t* ret = (uint8_t*)malloc(mallocSize);
        uint8_t* buffer = ret + sizeof(Header);
        ProfileBufferAllocator allocator(buffer, mallocSize);
        uint32_t allLedMask = 0xFFFFFFFF;
        uint8_t topFaceIndex = DiceVariants::getLayout()->faceCount - 1;
        uint32_t topLedMask = 1 << (BoardManager::getBoard()->ledCount - 1);

        // Create the global that indicates the current face
        auto currentFaceScalarPtr = allocator.allocatePtr<DScalarGlobal>();
        auto currentFaceScalar = allocator.get(currentFaceScalarPtr);
        currentFaceScalar->type = ScalarType_Global;
        currentFaceScalar->globalType = GlobalType_NormalizedCurrentFace;

        // Create the gradient that we will lookup into
        auto rainbowGradientPtr = allocator.allocatePtr<DGradientRainbow>();
        auto rainbowGradient = allocator.get(rainbowGradientPtr);
        rainbowGradient->type = GradientType_Rainbow;
        
        // Create the lookup color
        auto lookupGradientFromFacePtr = allocator.allocatePtr<DColorLookup>();
        auto lookupGradientFromFace = allocator.get(lookupGradientFromFacePtr);
        lookupGradientFromFace->type = ColorType_Lookup;
        lookupGradientFromFace->lookupGradient = rainbowGradientPtr;
        lookupGradientFromFace->parameter = currentFaceScalarPtr;

        // Create red color
        auto redColorPtr = allocator.allocatePtr<DColorRGB>();
        auto redColor = allocator.get(redColorPtr);
        redColor->type = ColorType_RGB;
        redColor->rValue = 8;
        redColor->gValue = 0;
        redColor->bValue = 0;

        // Create green color
        auto greenColorPtr = allocator.allocatePtr<DColorRGB>();
        auto greenColor = allocator.get(greenColorPtr);
        greenColor->type = ColorType_RGB;
        greenColor->rValue = 0;
        greenColor->gValue = 8;
        greenColor->bValue = 0;

        // Create blue color
        auto blueColorPtr = allocator.allocatePtr<DColorRGB>();
        auto blueColor = allocator.get(blueColorPtr);
        blueColor->type = ColorType_RGB;
        blueColor->rValue = 0;
        blueColor->gValue = 8;
        blueColor->bValue = 0;

        // Create yellow color
        auto yellowColorPtr = allocator.allocatePtr<DColorRGB>();
        auto yellowColor = allocator.get(yellowColorPtr);
        yellowColor->type = ColorType_RGB;
        yellowColor->rValue = 6;
        yellowColor->gValue = 6;
        yellowColor->bValue = 0;

        // Allocate our Hello animation
        auto animationRainbowPtr = allocator.allocatePtr<AnimationRainbow>();
        auto animationRainbow = allocator.get(animationRainbowPtr);
        animationRainbow->type = AnimationType_Rainbow;
        animationRainbow->traveling = 1;
        animationRainbow->count = 3;
        animationRainbow->duration = 3000;
        animationRainbow->faceMask = allLedMask;
        animationRainbow->fade = 64;
        animationRainbow->intensity = 128;

        // Charging
        auto animationChargingPtr = allocator.allocatePtr<AnimationSimple>();
        auto animationCharging = allocator.get(animationChargingPtr);
        animationCharging->type = AnimationType_Simple;
        animationCharging->traveling = 0;
        animationCharging->duration = 3000;
        animationCharging->color = redColorPtr;
        animationCharging->count = 1;
        animationCharging->fade = 255;
        animationCharging->faceMask = topLedMask;

        // Low battery
        auto animationLowBatteryPtr = allocator.allocatePtr<AnimationSimple>();
        auto animationLowBattery = allocator.get(animationLowBatteryPtr);
        animationLowBattery->type = AnimationType_Simple;
        animationLowBattery->traveling = 0;
        animationLowBattery->duration = 1500;
        animationLowBattery->color = redColorPtr;
        animationLowBattery->count = 3;
        animationLowBattery->fade = 255;
        animationLowBattery->faceMask = allLedMask;

        // Charging Problem
        auto animationChargingProblemPtr = allocator.allocatePtr<AnimationSimple>();
        auto animationChargingProblem = allocator.get(animationChargingProblemPtr);
        animationChargingProblem->type = AnimationType_Simple;
        animationChargingProblem->traveling = 0;
        animationChargingProblem->duration = 2000;
        animationChargingProblem->color = redColorPtr;
        animationChargingProblem->count = 10;
        animationChargingProblem->fade = 255;
        animationChargingProblem->faceMask = topLedMask;

        // Fully charged
        auto animationFullyChargedPtr = allocator.allocatePtr<AnimationSimple>();
        auto animationFullyCharged = allocator.get(animationFullyChargedPtr);
        animationFullyCharged->type = AnimationType_Simple;
        animationFullyCharged->traveling = 0;
        animationFullyCharged->duration = 10000;
        animationFullyCharged->color = greenColorPtr;
        animationFullyCharged->count = 1;
        animationFullyCharged->fade = 32;
        animationFullyCharged->faceMask = topLedMask;

        // Connection
        auto animationConnectionPtr = allocator.allocatePtr<AnimationSimple>();
        auto animationConnection = allocator.get(animationConnectionPtr);
        animationConnection->type = AnimationType_Simple;
        animationConnection->traveling = 0;
        animationConnection->duration = 1000;
        animationConnection->color = blueColorPtr;
        animationConnection->count = 1;
        animationConnection->fade = 255;
        animationConnection->faceMask = allLedMask;

        // Rolling
        auto animationRollingPtr = allocator.allocatePtr<AnimationSimple>();
        auto animationRolling = allocator.get(animationRollingPtr);
        animationRolling->type = AnimationType_Simple;
        animationRolling->traveling = 0;
        animationRolling->duration = 500;
        animationRolling->color = lookupGradientFromFacePtr;
        animationRolling->count = 1;
        animationRolling->fade = 0;
        animationRolling->faceMask = topLedMask;

        // On Face
        auto animationOnFacePtr = allocator.allocatePtr<AnimationSimple>();
        auto animationOnFace = allocator.get(animationOnFacePtr);
        animationOnFace->type = AnimationType_Simple;
        animationOnFace->traveling = 0;
        animationOnFace->duration = 3000;
        animationOnFace->color = lookupGradientFromFacePtr;
        animationOnFace->count = 1;
        animationOnFace->fade = 0;
        animationOnFace->faceMask = allLedMask;

        // Error while charging (temperature)
        auto animationTempErrorPtr = allocator.allocatePtr<AnimationSimple>();
        auto animationTempError = allocator.get(animationTempErrorPtr);
        animationTempError->type = AnimationType_Simple;
        animationTempError->traveling = 0;
        animationTempError->duration = 1000;
        animationTempError->color = yellowColorPtr;
        animationTempError->count = 3;
        animationTempError->fade = 255;
        animationTempError->faceMask = topLedMask;

        // Allocate the array
        auto animationArrayPtr = allocator.allocateArray<AnimationPtr>(9);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationRainbowPtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationChargingPtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationLowBatteryPtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationChargingProblemPtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationFullyChargedPtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationConnectionPtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationRollingPtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationOnFacePtr);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationTempErrorPtr);

        // Allocate a condition
        auto conditionHelloPtr = allocator.allocatePtr<ConditionHelloGoodbye>();
        auto conditionHello = allocator.get(conditionHelloPtr);
        conditionHello->type = Condition_HelloGoodbye;
        conditionHello->flags = ConditionHelloGoodbye_Hello;

        // Add New Connection condition (index 1)
        auto conditionConnectionPtr = allocator.allocatePtr<ConditionConnectionState>();
        auto connected = allocator.get(conditionConnectionPtr);
        connected->type = Condition_ConnectionState;
        connected->flags = ConditionConnectionState_Connected | ConditionConnectionState_Disconnected;

        // Add Rolling condition (index 2)
        auto rollingPtr = allocator.allocatePtr<ConditionRolling>();
        auto rolling = allocator.get(rollingPtr);
        rolling->type = Condition_Rolling;
        rolling->repeatPeriodMs = 500;

        // Add OnFace condition (index 3)
        auto facePtr = allocator.allocatePtr<ConditionFaceCompare>();
        auto face = allocator.get(facePtr);
        face->type = Condition_FaceCompare;
        face->flags = ConditionFaceCompare_Equal | ConditionFaceCompare_Greater;
        face->faceIndex = 0;

        // Add Low Battery condition (index 4)
        auto low_battPtr = allocator.allocatePtr<ConditionBatteryState>();
        auto low_batt = allocator.get(low_battPtr);
        low_batt->type = Condition_BatteryState;
        low_batt->flags = ConditionBatteryState_Flags::ConditionBatteryState_Low;
        low_batt->repeatPeriodMs = 30000; // 30s

        // Add Charging condition (index 5)
        auto charge_battPtr = allocator.allocatePtr<ConditionBatteryState>();
        auto charge_batt = allocator.get(charge_battPtr);
        charge_batt->type = Condition_BatteryState;
        charge_batt->flags = ConditionBatteryState_Flags::ConditionBatteryState_Charging;
        charge_batt->repeatPeriodMs = 5000; //s

        // Add Done charging condition (index 6)
        auto done_chargePtr = allocator.allocatePtr<ConditionBatteryState>();
        auto done_charge = allocator.get(done_chargePtr);
        done_charge->type = Condition_BatteryState;
        done_charge->flags = ConditionBatteryState_Done;
        done_charge->repeatPeriodMs = 5000; //s

        // Add Bad charging condition (index 7)
        auto bad_chargePtr = allocator.allocatePtr<ConditionBatteryState>();
        auto bad_charge = allocator.get(bad_chargePtr);
        bad_charge->type = Condition_BatteryState;
        bad_charge->flags = ConditionBatteryState_BadCharging;

        // Add error during charging (usually temperature) condition (index 8)
        auto error_chargePtr = allocator.allocatePtr<ConditionBatteryState>();
        auto error_charge = allocator.get(error_chargePtr);
        error_charge->type = Condition_BatteryState;
        error_charge->flags = ConditionBatteryState_Error;
        error_charge->repeatPeriodMs = 1500; //s


        // And matching action
        auto playHelloPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playHello = allocator.get(playHelloPtr);
        playHello->type = Action_PlayAnimation;
        playHello->animation = animationRainbowPtr;
        playHello->faceIndex = topFaceIndex;
        playHello->loopCount = 1;
        playHello->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playHelloArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playHelloArrayPtr, 0, (ActionPtr)playHelloPtr);

        auto playConnectedPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playConnected = allocator.get(playConnectedPtr);
        playConnected->type = Action_PlayAnimation;
        playConnected->animation = animationConnectionPtr;
        playConnected->faceIndex = topFaceIndex;
        playConnected->loopCount = 1;
        playConnected->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playConnectedArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playConnectedArrayPtr, 0, (ActionPtr)playConnectedPtr);

        auto playRollingPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playRolling = allocator.get(playRollingPtr);
        playRolling->type = Action_PlayAnimation;
        playRolling->animation = animationRollingPtr;
        playRolling->faceIndex = FACE_INDEX_CURRENT_FACE;
        playRolling->loopCount = 1;
        playRolling->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playRollingArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playRollingArrayPtr, 0, (ActionPtr)playRollingPtr);

        auto playFacePtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playFace = allocator.get(playFacePtr);
        playFace->type = Action_PlayAnimation;
        playFace->animation = animationOnFacePtr;
        playFace->faceIndex = topFaceIndex;
        playFace->loopCount = 1;
        playFace->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playFaceArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playFaceArrayPtr, 0, (ActionPtr)playFacePtr);

        auto playLow_battPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playLow_batt = allocator.get(playLow_battPtr);
        playLow_batt->type = Action_PlayAnimation;
        playLow_batt->animation = animationLowBatteryPtr;
        playLow_batt->faceIndex = topFaceIndex;
        playLow_batt->loopCount = 1;
        playLow_batt->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playLow_battArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playLow_battArrayPtr, 0, (ActionPtr)playLow_battPtr);

        auto playCharge_battPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playCharge_batt = allocator.get(playCharge_battPtr);
        playCharge_batt->type = Action_PlayAnimation;
        playCharge_batt->animation = animationChargingPtr;
        playCharge_batt->faceIndex = topFaceIndex;
        playCharge_batt->loopCount = 1;
        playCharge_batt->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playCharge_battArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playCharge_battArrayPtr, 0, (ActionPtr)playCharge_battPtr);

        auto playDone_charge_battPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playDone_charge_batt = allocator.get(playDone_charge_battPtr);
        playDone_charge_batt->type = Action_PlayAnimation;
        playDone_charge_batt->animation = animationFullyChargedPtr;
        playDone_charge_batt->faceIndex = topFaceIndex;
        playDone_charge_batt->loopCount = 1;
        playDone_charge_batt->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playDone_charge_battArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playDone_charge_battArrayPtr, 0, (ActionPtr)playDone_charge_battPtr);

        auto playBad_charge_battPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playBad_charge_batt = allocator.get(playBad_charge_battPtr);
        playBad_charge_batt->type = Action_PlayAnimation;
        playBad_charge_batt->animation = animationChargingProblemPtr;
        playBad_charge_batt->faceIndex = topFaceIndex;
        playBad_charge_batt->loopCount = 1;
        playBad_charge_batt->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playBad_charge_battArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playBad_charge_battArrayPtr, 0, (ActionPtr)playBad_charge_battPtr);

        auto playError_charge_battPtr = allocator.allocatePtr<ActionPlayAnimation>();
        auto playError_charge_batt = allocator.get(playError_charge_battPtr);
        playError_charge_batt->type = Action_PlayAnimation;
        playError_charge_batt->animation = animationTempErrorPtr;
        playError_charge_batt->faceIndex = topFaceIndex;
        playError_charge_batt->loopCount = 1;
        playError_charge_batt->overrides = Array<ParameterOverride>::emptyArray();
        // Allocate action array
        auto playError_charge_battArrayPtr = allocator.allocateArray<ActionPtr>(1);
        allocator.setAt(playError_charge_battArrayPtr, 0, (ActionPtr)playError_charge_battPtr);

        // Create the rule
        Rule helloRule;
        helloRule.condition = conditionHelloPtr;
        helloRule.actions = playHelloArrayPtr;

        Rule rollingRule;
        rollingRule.condition = rollingPtr;
        rollingRule.actions = playRollingArrayPtr;

        Rule onFaceRule;
        onFaceRule.condition = facePtr;
        onFaceRule.actions = playFaceArrayPtr;

        Rule chargingRule;
        chargingRule.condition = charge_battPtr;
        chargingRule.actions = playCharge_battArrayPtr;

        Rule badChargingRule;
        badChargingRule.condition = bad_chargePtr;
        badChargingRule.actions = playBad_charge_battArrayPtr;

        Rule lowBattRule;
        lowBattRule.condition = low_battPtr;
        lowBattRule.actions = playLow_battArrayPtr;

        Rule chargingDoneRule;
        chargingDoneRule.condition = done_chargePtr;
        chargingDoneRule.actions = playDone_charge_battArrayPtr;

        Rule connectedRule;
        connectedRule.condition = conditionConnectionPtr;
        connectedRule.actions = playConnectedArrayPtr;

        Rule tempRule;
        tempRule.condition = error_chargePtr;
        tempRule.actions = playError_charge_battArrayPtr;

        // Allocate rule array
        auto ruleArrayPtr = allocator.allocateArray<Rule>(9);
        allocator.setAt(ruleArrayPtr, 0, helloRule);
        allocator.setAt(ruleArrayPtr, 1, rollingRule);
        allocator.setAt(ruleArrayPtr, 2, onFaceRule);
        allocator.setAt(ruleArrayPtr, 3, chargingRule);
        allocator.setAt(ruleArrayPtr, 4, badChargingRule);
        allocator.setAt(ruleArrayPtr, 5, lowBattRule);
        allocator.setAt(ruleArrayPtr, 6, chargingDoneRule);
        allocator.setAt(ruleArrayPtr, 7, connectedRule);
        allocator.setAt(ruleArrayPtr, 8, tempRule);

        // Compute total size including hash and rounding to proper multiple
        uint16_t bufferSize = allocator.currentSize();
        uint32_t profileSize = sizeof(Header) + bufferSize + sizeof(uint32_t); // Header + empty buffer + hash
        *outSize = Flash::getProgramSize(profileSize);

        auto header = reinterpret_cast<Header*>(ret);
        header->headerStartMarker = PROFILE_VALID_KEY;
        header->version = PROFILE_VERSION;
        header->bufferSize = bufferSize;
        header->animations = animationArrayPtr;
        header->rules = ruleArrayPtr;
        header->headerEndMarker = PROFILE_VALID_KEY;

        // Compute and write hash
        uint32_t hash = Utils::computeHash(ret, sizeof(Header) + bufferSize);
        uint32_t hashAddr = (uint32_t)ret + sizeof(Header) + bufferSize;
        *((uint32_t*)hashAddr) = hash;

        NRF_LOG_INFO("Generated Default Profile, size=%d, hash=0x%08x", profileSize, hash);
        //NRF_LOG_HEXDUMP_INFO(ret, 32);

        return ret;
	}
	
    void Data::DestroyDefaultProfile(uint8_t* ptr) {
        free(ptr);
    }

    #endif
}
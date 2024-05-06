#include "profile_data.h"
#include "config/board_config.h"
#include "utils/utils.h"
#include "drivers_nrf/flash.h"
#include "nrf_log.h"
#include "config/settings.h"

#include "animations/animation_rainbow.h"
#include "animations/animation_flashes.h"
#include "behaviors/condition.h"
#include "behaviors/action.h"
#include "behaviors/behavior.h"
#include "drivers_nrf/watchdog.h"

using namespace DriversNRF;
using namespace Config;
using namespace Animations;
using namespace Behaviors;

#define USE_BINARY_BUFFER_IMAGE 1

namespace Profile
{
    #if USE_BINARY_BUFFER_IMAGE
    uint8_t defaultBytes[] = {
        0x0D,0xF0,0x0D,0x60,0x09,0x00,0x00,0x00,0x85,0x01,0xBC,0x00,0x0A,0xD0,0x00,0x0A,
        0x0D,0xF0,0x0D,0x60,0x03,0x01,0x01,0x03,0x02,0x00,0x00,0x00,0x01,0x03,0x03,0x02,
        0x15,0x03,0x0A,0x00,0x08,0x00,0x02,0x00,0x01,0x15,0x08,0x0C,0x00,0x12,0x00,0x03,
        0x02,0x00,0x15,0x00,0x02,0x08,0x00,0x00,0x02,0x00,0x08,0x00,0x02,0x00,0x00,0x10,
        0x02,0x06,0x06,0x00,0x01,0x01,0xB8,0x0B,0xFF,0xFF,0xFF,0xFF,0x20,0x00,0x1B,0x00,
        0x01,0x00,0x01,0x02,0xB8,0x0B,0xFF,0xFF,0xFF,0xFF,0x20,0x00,0x20,0x00,0x01,0xFF,
        0x01,0x00,0xDC,0x05,0xFF,0xFF,0xFF,0xFF,0x20,0x00,0x20,0x00,0x03,0xFF,0x01,0x02,
        0xD0,0x07,0xFF,0xFF,0xFF,0xFF,0x20,0x00,0x20,0x00,0x0A,0xFF,0x01,0x02,0x10,0x27,
        0xFF,0xFF,0xFF,0xFF,0x20,0x00,0x24,0x00,0x01,0x20,0x01,0x00,0xE8,0x03,0xFF,0xFF,
        0xFF,0xFF,0x20,0x00,0x28,0x00,0x01,0xFF,0x01,0x02,0xE8,0x03,0xFF,0xFF,0xFF,0xFF,
        0x20,0x01,0x03,0x00,0x01,0xFF,0x01,0x02,0xF4,0x01,0xFF,0xFF,0xFF,0xFF,0x20,0x01,
        0x03,0x00,0x01,0xFF,0x01,0x00,0xB8,0x0B,0xFF,0xFF,0xFF,0xFF,0x20,0x01,0x03,0x00,
        0x01,0xFF,0x01,0x02,0xE8,0x03,0xFF,0xFF,0xFF,0xFF,0x20,0x00,0x2C,0x00,0x03,0xFF,
        0x30,0x00,0x3E,0x00,0x4C,0x00,0x5A,0x00,0x68,0x00,0x76,0x00,0x84,0x00,0x92,0x00,
        0xA0,0x00,0xAE,0x00,0x02,0x01,0x0C,0x01,0x01,0x0E,0x01,0x17,0x01,0x01,0x19,0x01,
        0x24,0x01,0x01,0x26,0x01,0x31,0x01,0x01,0x33,0x01,0x3F,0x01,0x01,0x41,0x01,0x4D,
        0x01,0x01,0x4F,0x01,0x5B,0x01,0x01,0x5D,0x01,0x69,0x01,0x01,0x6B,0x01,0x75,0x01,
        0x01,0x77,0x01,0x83,0x01,0x01,0x01,0x01,0x01,0xFE,0x01,0x30,0x00,0x00,0x00,0x00,
        0x04,0x01,0x02,0x01,0xFF,0x01,0x84,0x00,0x00,0x00,0x00,0x0F,0x01,0x03,0xF4,0x01,
        0x01,0xFF,0x01,0x92,0x00,0x00,0x00,0x00,0x1C,0x01,0x04,0x00,0x06,0x01,0xFE,0x01,
        0xA0,0x00,0x00,0x00,0x00,0x29,0x01,0x07,0x04,0x88,0x13,0x01,0xFE,0x01,0x3E,0x00,
        0x00,0x00,0x00,0x37,0x01,0x07,0x10,0x00,0x00,0x01,0xFE,0x01,0x5A,0x00,0x00,0x00,
        0x00,0x45,0x01,0x07,0x02,0x30,0x75,0x01,0xFE,0x01,0x4C,0x00,0x00,0x00,0x00,0x53,
        0x01,0x07,0x08,0x88,0x13,0x01,0xFE,0x01,0x68,0x00,0x00,0x00,0x00,0x61,0x01,0x06,
        0x03,0x01,0xFE,0x01,0x76,0x00,0x00,0x00,0x00,0x6D,0x01,0x07,0x20,0xDC,0x05,0x01,
        0xFE,0x01,0xAE,0x00,0x00,0x00,0x00,0x7B,0x01,0x5B,0x26,0x1F,0x82,0xB4,0x01,0x00,
    };

    uint8_t* Data::createDefaultProfile(uint32_t* outSize) {
        *outSize = sizeof(defaultBytes);
        return defaultBytes;
    }

    void Data::destroyDefaultProfile(uint8_t* ptr) {
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

        Pointer<DScalarGlobal> addGlobal(GlobalName name) {
            auto ret = allocatePtr<DScalarGlobal>();
            auto g = get(ret);
            g->type = ScalarType_Global;
            g->name = name;
            return ret;
        }

        Pointer<DScalarUInt8> addConstantU8(uint8_t value) {
            auto ret = allocatePtr<DScalarUInt8>();
            auto g = get(ret);
            g->type = ScalarType_UInt8;
            g->value = value;
            return ret;
        }

        Pointer<DScalarUInt16> addConstantU16(uint16_t value) {
            auto ret = allocatePtr<DScalarUInt16>();
            auto g = get(ret);
            g->type = ScalarType_UInt16;
            g->value = value;
            return ret;
        }

        Pointer<DOperationScalar> addOperation(OperationOneOperand op, DScalarPtr val) {
            auto ret = allocatePtr<DOperationScalar>();
            auto t = get(ret);
            t->type = ScalarType_OperationScalar;
            t->operation = op;
            t->operand = val;
            return ret;
        }

        Pointer<DOperationTwoScalars> addOperation(OperationTwoOperands op, DScalarPtr val1, DScalarPtr val2) {
            auto ret = allocatePtr<DOperationTwoScalars>();
            auto t = get(ret);
            t->type = ScalarType_OperationTwoScalars;
            t->operation = op;
            t->operand1 = val1;
            t->operand2 = val2;
            return ret;
        }

        // Create the gradient that we will lookup into
        Pointer<ColorCurve> addRainbow() {
            auto rainbowPtr = allocatePtr<ColorCurve>();
            auto rainbow = get(rainbowPtr);
            rainbow->type = ColorCurveType_Rainbow;
            return rainbowPtr;
        }
        
        Pointer<DColorLookup> addLookup(ColorCurvePtr g, DScalarPtr p) {
            auto lookupPtr = allocatePtr<DColorLookup>();
            auto lookup = get(lookupPtr);
            lookup->type = ColorType_Lookup;
            lookup->lookupCurve = g;
            lookup->parameter = p;
            return lookupPtr;
        }

        // Create red color
        Pointer<DColorRGB> addRGB(uint8_t r, uint8_t g, uint8_t b) {
            auto colorPtr = allocatePtr<DColorRGB>();
            auto color = get(colorPtr);
            color->type = ColorType_RGB;
            color->rValue = r;
            color->gValue = g;
            color->bValue = b;
            return colorPtr;
        }

        Pointer<AnimationRainbow> addAnimRainbow(uint8_t count, uint16_t duration) {
            auto animPtr = allocatePtr<AnimationRainbow>();
            auto anim = get(animPtr);
            anim->type = AnimationType_Rainbow;
            anim->animFlags = AnimationFlags_UseLedIndices;
            anim->duration = duration;
            anim->faceMask = ANIM_FACEMASK_ALL_LEDS;
            anim->intensity = 32;
            anim->count = count;
            anim->fade = 128;
            anim->cyclesTimes16 = 16;
            anim->traveling = true;
            return animPtr;
        }

        Pointer<AnimationFlashes> addAnimFlashes(uint8_t count, uint16_t duration, DColorPtr color, uint8_t fade, uint32_t animFlags = (uint8_t)AnimationFlags_None, uint8_t colorFlags = (uint8_t)AnimationFlashesFlags_None) {
            auto animPtr = allocatePtr<AnimationFlashes>();
            auto anim = get(animPtr);
            anim->type = AnimationType_Flashes;
            anim->animFlags = animFlags;
            anim->duration = duration;
            anim->faceMask = ANIM_FACEMASK_ALL_LEDS;
            anim->intensity = 32;
            anim->color = color;
            anim->count = count;
            anim->colorFlags = colorFlags;
            anim->fade = fade;
            return animPtr;
        }

        // Allocate a condition
        Pointer<ConditionHelloGoodbye> addCondHello(uint8_t flags) {
            auto helloGoodbyePtr = allocatePtr<ConditionHelloGoodbye>();
            auto helloGoodbye = get(helloGoodbyePtr);
            helloGoodbye->type = Condition_HelloGoodbye;
            helloGoodbye->flags = flags;
            return helloGoodbyePtr;
        }

        Pointer<ConditionConnectionState> addCondConn(uint8_t flags) {
            auto connectionPtr = allocatePtr<ConditionConnectionState>();
            auto connection = get(connectionPtr);
            connection->type = Condition_ConnectionState;
            connection->flags = flags;
            return connectionPtr;
        }

        Pointer<ConditionHandling> addCondHandling() {
            auto handlingPtr = allocatePtr<ConditionHandling>();
            auto handling = get(handlingPtr);
            handling->type = Condition_Handling;
            return handlingPtr;
        }

        Pointer<ConditionRolling> addCondRolling(uint16_t repeatPeriod) {
            auto rollingPtr = allocatePtr<ConditionRolling>();
            auto rolling = get(rollingPtr);
            rolling->type = Condition_Rolling;
            rolling->repeatPeriodMs = repeatPeriod;
            return rollingPtr;
        }

        Pointer<ConditionFaceCompare> addCondFace(uint8_t flags, uint8_t index) {
            auto facePtr = allocatePtr<ConditionFaceCompare>();
            auto face = get(facePtr);
            face->type = Condition_FaceCompare;
            face->flags = flags;
            face->faceIndex = index;
            return facePtr;
        }

        // Add Low Battery condition
        Pointer<ConditionBatteryState> addCondBatt(uint8_t flags, uint16_t repeat) {
            auto batteryPtr = allocatePtr<ConditionBatteryState>();
            auto battery = get(batteryPtr);
            battery->type = Condition_BatteryState;
            battery->flags = flags;
            battery->repeatPeriodMs = repeat;
            return batteryPtr;
        }

        Array<ActionPtr> addPlayAnimActionAsArray(AnimationPtr animation, uint8_t face) {
            // And matching action
            auto playHelloPtr = allocatePtr<ActionPlayAnimation>();
            auto playHello = get(playHelloPtr);
            playHello->type = Action_PlayAnimation;
            playHello->animation = animation;
            playHello->faceIndex = face;
            playHello->loopCount = 1;
            playHello->overrides = Array<ParameterOverride>::emptyArray();
            // Allocate action array
            auto playHelloArrayPtr = allocateArray<ActionPtr>(1);
            setAt(playHelloArrayPtr, 0, (ActionPtr)playHelloPtr);
            return playHelloArrayPtr;
        }

        void setRuleAt(Array<Rule> rules, uint8_t index, Pointer<Condition> condition, Array<ActionPtr> actions) {
            Rule r;
            r.condition = condition;
            r.actions = actions;
            setAt(rules, index, r);
        }
    };

    uint8_t* Data::createDefaultProfile(uint32_t* outSize) {

        // Create empty profile for now
        const uint32_t mallocSize = 512; // <-- make sure this is big enough to hold the data we put in!
        uint8_t* const ret = (uint8_t*)malloc(mallocSize);
        uint8_t* const buffer = ret + sizeof(Header);
        ProfileBufferAllocator allocator(buffer, mallocSize - sizeof(Header) + sizeof(uint32_t)); // - Header - hash;

        // Create the global that indicates the current face
        const auto currentFaceScalarPtr = allocator.addGlobal(GlobalName_NormalizedCurrentFace);
        const auto rainbowGradientPtr = allocator.addRainbow();
        const auto lookupGradientFromFacePtr = allocator.addLookup(rainbowGradientPtr, currentFaceScalarPtr);
        const auto scalar3Ptr = allocator.addConstantU8(3);
        const auto animTimePtr = allocator.addGlobal(GlobalName_NormalizedAnimationTime);
        const auto scaledTimePtr = allocator.addOperation(OperationTwoOperands_Mul, animTimePtr, scalar3Ptr);
        const auto scalar256Ptr = allocator.addConstantU16(256);
        const auto travelingTimePtr = allocator.addOperation(OperationTwoOperands_Traveling, scaledTimePtr, scalar256Ptr);
        const auto travelingRainbowPtr = allocator.addLookup(rainbowGradientPtr, travelingTimePtr);
        const auto redColorPtr = allocator.addRGB(8,0,0);
        const auto greenColorPtr = allocator.addRGB(0,8,0);
        const auto blueColorPtr = allocator.addRGB(0,0,16);
        const auto yellowColorPtr = allocator.addRGB(6,6,0);

        // Allocate our Hello animation
        //const auto animationRainbowPtr = allocator.addAnimRainbow(3, 3000);
        const auto animationRainbowPtr = allocator.addAnimFlashes(1, 3000, travelingRainbowPtr, 0, AnimationFlags_UseLedIndices);
        const auto animationChargingPtr = allocator.addAnimFlashes(1, 3000, redColorPtr, 255, AnimationFlags_HighestLed);
        const auto animationLowBatteryPtr = allocator.addAnimFlashes(3, 1500, redColorPtr, 255);
        const auto animationChargingProblemPtr = allocator.addAnimFlashes(10, 2000, redColorPtr, 255, AnimationFlags_HighestLed);
        const auto animationFullyChargedPtr = allocator.addAnimFlashes(1, 10000, greenColorPtr, 32, AnimationFlags_HighestLed);
        const auto animationConnectionPtr = allocator.addAnimFlashes(1, 1000, blueColorPtr, 255);
        const auto animationHandlingPtr = allocator.addAnimFlashes(1, 1000, lookupGradientFromFacePtr, 255, AnimationFlags_HighestLed, AnimationFlashesFlags_CaptureColor);
        const auto animationRollingPtr = allocator.addAnimFlashes(1, 500, lookupGradientFromFacePtr, 255, AnimationFlags_HighestLed, AnimationFlashesFlags_CaptureColor);
        const auto animationOnFacePtr = allocator.addAnimFlashes(1, 3000, lookupGradientFromFacePtr, 255, AnimationFlags_None, AnimationFlashesFlags_CaptureColor);
        const auto animationTempErrorPtr = allocator.addAnimFlashes(3, 1000, yellowColorPtr, 255, AnimationFlags_HighestLed);

        // Allocate animation array
        const auto animationArrayPtr = allocator.allocateArray<AnimationPtr>(10);
        allocator.setAt(animationArrayPtr, 0, (AnimationPtr)animationRainbowPtr);
        allocator.setAt(animationArrayPtr, 1, (AnimationPtr)animationChargingPtr);
        allocator.setAt(animationArrayPtr, 2, (AnimationPtr)animationLowBatteryPtr);
        allocator.setAt(animationArrayPtr, 3, (AnimationPtr)animationChargingProblemPtr);
        allocator.setAt(animationArrayPtr, 4, (AnimationPtr)animationFullyChargedPtr);
        allocator.setAt(animationArrayPtr, 5, (AnimationPtr)animationConnectionPtr);
        allocator.setAt(animationArrayPtr, 6, (AnimationPtr)animationHandlingPtr);
        allocator.setAt(animationArrayPtr, 7, (AnimationPtr)animationRollingPtr);
        allocator.setAt(animationArrayPtr, 8, (AnimationPtr)animationOnFacePtr);
        allocator.setAt(animationArrayPtr, 9, (AnimationPtr)animationTempErrorPtr);

        // Allocate rule array
        const auto ruleArrayPtr = allocator.allocateArray<Rule>(10);
        allocator.setRuleAt(ruleArrayPtr, 0, 
            allocator.addCondHello(ConditionHelloGoodbye_Hello),
            allocator.addPlayAnimActionAsArray(animationRainbowPtr, FACE_INDEX_HIGHEST_FACE));
        allocator.setRuleAt(ruleArrayPtr, 1,
            allocator.addCondHandling(),
            allocator.addPlayAnimActionAsArray(animationHandlingPtr, FACE_INDEX_CURRENT_FACE));
        allocator.setRuleAt(ruleArrayPtr, 2,
            allocator.addCondRolling(500),
            allocator.addPlayAnimActionAsArray(animationRollingPtr,FACE_INDEX_CURRENT_FACE));
        allocator.setRuleAt(ruleArrayPtr, 3,
            allocator.addCondFace(ConditionFaceCompare_Equal | ConditionFaceCompare_Greater, 0),
            allocator.addPlayAnimActionAsArray(animationOnFacePtr, FACE_INDEX_HIGHEST_FACE));
        allocator.setRuleAt(ruleArrayPtr, 4,
            allocator.addCondBatt(ConditionBatteryState_Flags::ConditionBatteryState_Charging, 5000),
            allocator.addPlayAnimActionAsArray(animationChargingPtr, FACE_INDEX_HIGHEST_FACE));
        allocator.setRuleAt(ruleArrayPtr, 5,
            allocator.addCondBatt(ConditionBatteryState_BadCharging, 0),
            allocator.addPlayAnimActionAsArray(animationChargingProblemPtr, FACE_INDEX_HIGHEST_FACE));
        allocator.setRuleAt(ruleArrayPtr, 6,
            allocator.addCondBatt(ConditionBatteryState_Flags::ConditionBatteryState_Low, 30000),
            allocator.addPlayAnimActionAsArray(animationLowBatteryPtr, FACE_INDEX_HIGHEST_FACE));
        allocator.setRuleAt(ruleArrayPtr, 7,
            allocator.addCondBatt(ConditionBatteryState_Done, 5000),
            allocator.addPlayAnimActionAsArray(animationFullyChargedPtr, FACE_INDEX_HIGHEST_FACE));
        allocator.setRuleAt(ruleArrayPtr, 8,
            allocator.addCondConn(ConditionConnectionState_Connected | ConditionConnectionState_Disconnected),
            allocator.addPlayAnimActionAsArray(animationConnectionPtr, FACE_INDEX_HIGHEST_FACE));
        allocator.setRuleAt(ruleArrayPtr, 9,
            allocator.addCondBatt(ConditionBatteryState_Error, 1500),
            allocator.addPlayAnimActionAsArray(animationTempErrorPtr, FACE_INDEX_HIGHEST_FACE));

        // Compute total size including hash and rounding to proper multiple
        const uint16_t bufferSize = allocator.currentSize();
        const uint32_t profileSize = sizeof(Header) + bufferSize + sizeof(uint32_t); // Header + empty buffer + hash
        *outSize = Flash::getProgramSize(profileSize);

        auto const header = reinterpret_cast<Header*>(ret);
        header->headerStartMarker = PROFILE_VALID_KEY;
        header->version = PROFILE_VERSION;
        header->bufferSize = bufferSize;
        header->animations = animationArrayPtr;
        header->rules = ruleArrayPtr;
        header->headerEndMarker = PROFILE_VALID_KEY;

        // Compute and write hash
        const uint32_t hash = Utils::computeHash(ret, sizeof(Header) + bufferSize);
        const uint32_t hashAddr = (uint32_t)ret + sizeof(Header) + bufferSize;
        *((uint32_t*)hashAddr) = hash;

        // NRF_LOG_INFO("Generated Default Profile, size=%d, hash=0x%08x", profileSize, hash);

        NRF_LOG_RAW_INFO("uint8_t defaultBytes[] = {\r\n");
        NRF_LOG_RAW_INFO("\t");
        uint32_t offset = 0;
        for (; offset < *outSize; offset++) {
            NRF_LOG_RAW_INFO("0x%02x,", ret[offset]);
            if ((offset % 16) == 15) {
                Watchdog::feed();
                NRF_LOG_RAW_INFO("\r\n\t")
            }
        }
        NRF_LOG_RAW_INFO("};\r\n");

        return ret;
    }
    
    void Data::destroyDefaultProfile(uint8_t* ptr) {
        free(ptr);
    }

    #endif
}

#include "data_set.h"
#include "data_set_data.h"
#include "config/board_config.h"
#include "animations/animation_simple.h"
#include "animations/animation_rainbow.h"
#include "behaviors/action.h"
#include "behaviors/behavior.h"
#include "behaviors/condition.h"
#include "utils/utils.h"
#include "drivers_nrf/flash.h"
#include "nrf_log.h"
#include "config/settings.h"

using namespace Utils;
using namespace DriversNRF;
using namespace Config;
using namespace Modules;
using namespace Animations;
using namespace Behaviors;

//#define USE_BINARY_BUFFER_IMAGE

namespace DataSet
{
	void ProgramDefaultDataSet(const Settings& settingsPackAlong, DataSetWrittenCallback callback) {
        NRF_LOG_INFO("Programming default data set");

        static DataSetWrittenCallback _setWrittenCallback;
		_setWrittenCallback = callback;
#ifdef USE_BINARY_BUFFER_IMAGE
        // Doesn't quite work yet...
        static const uint8_t writeBuffer[] __attribute__ ((aligned (4))) = {
            0x10, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
            0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00,
            0x18, 0x00, 0x24, 0x00, 0x30, 0x00, 0x3C, 0x00,
            0x48, 0x00, 0x54, 0x00, 0x01, 0x00, 0xB8, 0x0B,
            0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0xFF,
            0x01, 0x00, 0xD0, 0x07, 0x00, 0x00, 0x08, 0x00,
            0x00, 0x00, 0x0A, 0xFF, 0x01, 0x00, 0xDC, 0x05,
            0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x03, 0xFF,
            0x01, 0x00, 0x88, 0x13, 0x00, 0x00, 0x08, 0x00,
            0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0xE8, 0x03,
            0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x00, 0x02, 0xFF,
            0x01, 0x00, 0x64, 0x00, 0x00, 0x00, 0x08, 0x00,
            0x7F, 0x00, 0x01, 0xFF, 0x01, 0x00, 0xB8, 0x0B,
            0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x00, 0x01, 0xFF,
            0x02, 0x00, 0xD0, 0x07, 0xFF, 0xFF, 0xFF, 0xFF,
            0x02, 0xC8, 0x01, 0x80, 0x00, 0x00, 0x04, 0x00,
            0x08, 0x00, 0x0C, 0x00, 0x10, 0x00, 0x14, 0x00,
            0x18, 0x00, 0x1C, 0x00, 0x01, 0x07, 0x13, 0x01,
            0x01, 0x04, 0x00, 0x01, 0x01, 0x05, 0xFF, 0x01,
            0x01, 0x06, 0xFF, 0x01, 0x01, 0x02, 0x00, 0x01,
            0x01, 0x00, 0x13, 0x01, 0x01, 0x03, 0x13, 0x01,
            0x01, 0x01, 0x13, 0x01, 0x00, 0x00, 0x04, 0x00,
            0x08, 0x00, 0x0C, 0x00, 0x10, 0x00, 0x14, 0x00,
            0x18, 0x00, 0x1C, 0x00, 0x01, 0x01, 0x00, 0x00,
            0x06, 0x03, 0x00, 0x00, 0x03, 0x00, 0xF4, 0x01,
            0x04, 0x00, 0x06, 0x00, 0x07, 0x02, 0x30, 0x75,
            0x07, 0x04, 0x88, 0x13, 0x07, 0x88, 0xB8, 0x0B,
            0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x03, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x04, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x05, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x06, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x07, 0x00,
        };
        static const uint32_t bufferSize = sizeof(writeBuffer);

        static const uint8_t dataBuffer[] __attribute__ ((aligned (4))) = {
            0x0C, 0xF0, 0x0D, 0x60, 0x03, 0x00, 0x00, 0x00,
            0xD0, 0xE1, 0x02, 0x00, 0x09, 0x00, 0x00, 0x00,
            0xDC, 0xE1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xDC, 0xE1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xDC, 0xE1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xDC, 0xE1, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
            0xDC, 0xE1, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00,
            0xEC, 0xE1, 0x02, 0x00, 0x60, 0x00, 0x00, 0x00,
            0x7C, 0xE2, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00,
            0x8C, 0xE2, 0x02, 0x00, 0x1C, 0x00, 0x00, 0x00,
            0x4C, 0xE2, 0x02, 0x00, 0x08, 0x00, 0x00, 0x00,
            0x5C, 0xE2, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00,
            0xA8, 0xE2, 0x02, 0x00, 0x07, 0x00, 0x00, 0x00,
            0xE0, 0xE2, 0x02, 0x00, 0x0C, 0xF0, 0x0D, 0x60,
        };

		static const Data* newData = (const Data*)dataBuffer;
#else
        static void* writeBuffer;
        static uint32_t bufferSize;
		static Data* newData; 

		int paletteCount = 4;
        int paletteSize = Utils::roundUpTo4(paletteCount * 3);
		int rgbKeyframeCount = 0;
        int rgbTrackCount = 0;
		int keyframeCount = 0;
        int trackCount = 0;
        int simpleAnimCount = 8;
		int animCount = simpleAnimCount + 1;
        int animOffsetSize = Utils::roundUpTo4(animCount * sizeof(uint16_t));
        int animSize = sizeof(AnimationSimple) * simpleAnimCount + sizeof(AnimationRainbow);
        int actionCount = 9;
        int actionOffsetSize = Utils::roundUpTo4(actionCount * sizeof(uint16_t));
        int actionSize = sizeof(ActionPlayAnimation) * actionCount;
        int conditionCount = 9;
        int conditionOffsetSize = Utils::roundUpTo4(conditionCount * sizeof(uint16_t));
        uint32_t conditionsSize =
            sizeof(ConditionHelloGoodbye) + 
            sizeof(ConditionConnectionState) +
            sizeof(ConditionRolling) +
            sizeof(ConditionFaceCompare) + 
            sizeof(ConditionBatteryState) +
            sizeof(ConditionBatteryState) +
            sizeof(ConditionBatteryState) +
            sizeof(ConditionBatteryState) +
            sizeof(ConditionBatteryState);
        int ruleCount = 9;
        int behaviorCount = 1;

		// Compute the size of the needed buffer to store all that data!
		bufferSize =
            paletteSize * sizeof(uint8_t) +
			rgbKeyframeCount * sizeof(RGBKeyframe) +
			rgbTrackCount * sizeof(RGBTrack) +
			keyframeCount * sizeof(Keyframe) +
			trackCount * sizeof(Track) +
			animOffsetSize + animSize +
            actionOffsetSize + actionSize +
            conditionOffsetSize + conditionsSize + 
            ruleCount * sizeof(Rule) +
            behaviorCount * sizeof(Behavior);

		uint32_t dataAddress = Flash::getDataSetDataAddress();

        // Allocate a buffer for all the data we're about to create
        // We'll write the data in the buffer and then program it into flash!
        writeBuffer = malloc(bufferSize);
        uint32_t writeBufferAddress = (uint32_t)writeBuffer;

        // Allocate a new data object
        // We need to fill it with pointers as if the data it points to is located in flash already.
        // That means we have to compute addresses by hand, we can't just point to the data buffer We
        // just created above. Instead we make the pointers point to where the data WILL be.
		newData = (Data*)malloc(sizeof(Data));
		
        int currentOffset = 0;
        newData->headMarker = ANIMATION_SET_VALID_KEY;
		newData->version = ANIMATION_SET_VERSION;

		newData->animationBits.palette = (const uint8_t*)(dataAddress + currentOffset);
        auto writePalette = (uint8_t*)(writeBufferAddress + currentOffset);
        currentOffset += paletteSize;
		newData->animationBits.paletteSize = paletteCount * 3;

		newData->animationBits.rgbKeyframes = (const RGBKeyframe*)(dataAddress + currentOffset);
        //auto writeKeyframes = (RGBKeyframe*)(writeBufferAddress + currentOffset);
        currentOffset += rgbKeyframeCount * sizeof(RGBKeyframe);
		newData->animationBits.rgbKeyFrameCount = rgbKeyframeCount;

		newData->animationBits.rgbTracks = (const RGBTrack*)(dataAddress + currentOffset);
        //auto writeRGBTracks = (RGBTrack*)(writeBufferAddress + currentOffset);
		currentOffset += rgbTrackCount * sizeof(RGBTrack);
        newData->animationBits.rgbTrackCount = rgbTrackCount;

		newData->animationBits.keyframes = (const Keyframe*)(dataAddress + currentOffset);
        //auto writeKeyframes = (Keyframe*)(writeBufferAddress + currentOffset);
        currentOffset += keyframeCount * sizeof(Keyframe);
		newData->animationBits.keyFrameCount = keyframeCount;

		newData->animationBits.tracks = (const Track*)(dataAddress + currentOffset);
        //auto writeRGBTracks = (Track*)(writeBufferAddress + currentOffset);
		currentOffset += trackCount * sizeof(Track);
        newData->animationBits.trackCount = trackCount;

		newData->animationBits.animationOffsets = (const uint16_t*)(dataAddress + currentOffset);
        auto writeAnimationOffsets = (uint16_t*)(writeBufferAddress + currentOffset);
        currentOffset += animOffsetSize;
		newData->animationBits.animationCount = animCount;

		newData->animationBits.animations = (const uint8_t*)(dataAddress + currentOffset);
        auto writeSimpleAnimations = (AnimationSimple*)(writeBufferAddress + currentOffset);
        auto writeRainbowAnimation = (AnimationRainbow*)(writeBufferAddress + currentOffset + sizeof(AnimationSimple) * simpleAnimCount);
        currentOffset += animSize;
		newData->animationBits.animationsSize = animSize;
		
        newData->actionsOffsets = (const uint16_t*)(dataAddress + currentOffset);
        auto writeActionsOffsets = (uint16_t*)(writeBufferAddress + currentOffset);
        currentOffset += actionOffsetSize;
		newData->actionCount = actionCount;
		
        newData->actions = (const Action*)(dataAddress + currentOffset);
        auto writeActions = (ActionPlayAnimation*)(writeBufferAddress + currentOffset);
        currentOffset += actionSize;
		newData->actionsSize = actionSize;
		
        newData->conditionsOffsets = (const uint16_t*)(dataAddress + currentOffset);
        auto writeConditionsOffsets = (uint16_t*)(writeBufferAddress + currentOffset);
        currentOffset += conditionOffsetSize;
		newData->conditionCount = conditionCount;
		
        newData->conditions = (const Condition*)(dataAddress + currentOffset);
        auto writeConditions = (Condition*)(writeBufferAddress + currentOffset);
        currentOffset += conditionsSize;
		newData->conditionsSize = conditionsSize;
        
        newData->rules = (const Rule*)(dataAddress + currentOffset);
        auto writeRules = (Rule*)(writeBufferAddress + currentOffset);
        currentOffset += ruleCount * sizeof(Rule);
        newData->ruleCount = ruleCount;
		
        newData->behavior = (const Behavior*)(dataAddress + currentOffset);
        auto writeBehaviors = (Behavior*)(writeBufferAddress + currentOffset);
        currentOffset += sizeof(Behavior);

		newData->tailMarker = ANIMATION_SET_VALID_KEY;

        // Cute way to create Red Green Blue colors in palette
        writePalette[0] = 8;
        writePalette[1] = 0;
        writePalette[2] = 0;
        writePalette[3] = 0;
        writePalette[4] = 8;
        writePalette[5] = 0;
        writePalette[6] = 0;
        writePalette[7] = 0;
        writePalette[8] = 8;
        writePalette[9] = 6;
        writePalette[10] = 6;
        writePalette[11] = 0;

		// Create animations
		for (int c = 0; c < simpleAnimCount; ++c) {
            writeSimpleAnimations[c].type = Animation_Simple;
            writeSimpleAnimations[c].animFlags = 0;
            writeSimpleAnimations[c].fade = 255;
		}

        // 0 Charging
        writeSimpleAnimations[0].count = 1;
        writeSimpleAnimations[0].duration = 3000;
        writeSimpleAnimations[0].colorIndex = 0; // Red
	    writeSimpleAnimations[0].faceMask = DiceVariants::getTopFaceMask();

        // 1 Charging Problem
        writeSimpleAnimations[1].count = 10;
        writeSimpleAnimations[1].duration = 2000;
        writeSimpleAnimations[1].colorIndex = 0; // Red
	    writeSimpleAnimations[1].faceMask = ANIM_FACEMASK_ALL_LEDS;

        // 2 Low battery
        writeSimpleAnimations[2].count = 3;
        writeSimpleAnimations[2].duration = 1500;
        writeSimpleAnimations[2].colorIndex = 0; // Red
	    writeSimpleAnimations[2].faceMask = DiceVariants::getTopFaceMask();

        // 3 Fully charged
        writeSimpleAnimations[3].count = 1;
        writeSimpleAnimations[3].duration = 3000;
        writeSimpleAnimations[3].colorIndex = 1; // Green
	    writeSimpleAnimations[3].faceMask = DiceVariants::getTopFaceMask();

        // 4 Connection
        writeSimpleAnimations[4].count = 2;
        writeSimpleAnimations[4].duration = 1000;
        writeSimpleAnimations[4].colorIndex = 2; // Blue
	    writeSimpleAnimations[4].faceMask = ANIM_FACEMASK_ALL_LEDS;

        // 5 Rolling
        writeSimpleAnimations[5].count = 1;
        writeSimpleAnimations[5].duration = 100;
        writeSimpleAnimations[5].colorIndex = PALETTE_COLOR_FROM_FACE; // We'll override based on face
	    writeSimpleAnimations[5].faceMask = DiceVariants::getTopFaceMask();

        // 6 On Face
        writeSimpleAnimations[6].count = 1;
        writeSimpleAnimations[6].duration = 3000;
        writeSimpleAnimations[6].colorIndex = PALETTE_COLOR_FROM_FACE; // We'll override based on face
	    writeSimpleAnimations[6].faceMask = ANIM_FACEMASK_ALL_LEDS;

        // 7 error while charging (temperature)
        writeSimpleAnimations[7].count = 1;
        writeSimpleAnimations[7].duration = 1000;
        writeSimpleAnimations[7].colorIndex = 3; // yellow
	    writeSimpleAnimations[7].faceMask = DiceVariants::getTopFaceMask();

        // 8 Rainbow
        writeRainbowAnimation->type = Animation_Rainbow;
        writeRainbowAnimation->animFlags = AnimationFlags_Traveling | AnimationFlags_UseLedIndices;
        writeRainbowAnimation->duration = 2000;
		writeRainbowAnimation->faceMask = ANIM_FACEMASK_ALL_LEDS;
        writeRainbowAnimation->count = 2;
        writeRainbowAnimation->fade = 200;
        writeRainbowAnimation->intensity = 0x80;
        writeRainbowAnimation->cyclesTimes10 = 10;

		// Create offsets
		for (int i = 0; i < simpleAnimCount; ++i) {
			writeAnimationOffsets[i] = i * sizeof(AnimationSimple);
		}

        // Offset for rainbow anim
        writeAnimationOffsets[simpleAnimCount] = simpleAnimCount * sizeof(AnimationSimple);

        // Create conditions
        uint32_t address = reinterpret_cast<uint32_t>(writeConditions);
        uint16_t offset = 0;

        // Add Hello condition (index 0)
        ConditionHelloGoodbye* hello = reinterpret_cast<ConditionHelloGoodbye*>(address);
        hello->type = Condition_HelloGoodbye;
        hello->flags = ConditionHelloGoodbye_Hello;
        writeConditionsOffsets[0] = offset;
        offset += sizeof(ConditionHelloGoodbye);
        address += sizeof(ConditionHelloGoodbye);
        // And matching action
        writeActions[0].type = Action_PlayAnimation;
        writeActions[0].animIndex = 8; // Rainbow
        writeActions[0].faceIndex = FACE_INDEX_CURRENT_FACE; // doesn't really matter
        writeActions[0].loopCount = 1;

        // Add New Connection condition (index 1)
        ConditionConnectionState* connected = reinterpret_cast<ConditionConnectionState*>(address);
        connected->type = Condition_ConnectionState;
        connected->flags = ConditionConnectionState_Connected | ConditionConnectionState_Disconnected;
        writeConditionsOffsets[1] = offset;
        offset += sizeof(ConditionConnectionState);
        address += sizeof(ConditionConnectionState);
        // And matching action
        writeActions[1].type = Action_PlayAnimation;
        writeActions[1].animIndex = 4; // All LEDs blue
        writeActions[1].faceIndex = 0; // doesn't matter
        writeActions[1].loopCount = 1;

        // Add Rolling condition (index 2)
        ConditionRolling* rolling = reinterpret_cast<ConditionRolling*>(address);
        rolling->type = Condition_Rolling;
        rolling->repeatPeriodMs = 500;
        writeConditionsOffsets[2] = offset;
        offset += sizeof(ConditionRolling);
        address += sizeof(ConditionRolling);
        // And matching action
        writeActions[2].type = Action_PlayAnimation;
        writeActions[2].animIndex = 5; // face based on color
        writeActions[2].faceIndex = FACE_INDEX_CURRENT_FACE;
        writeActions[2].loopCount = 1;

        // Add OnFace condition (index 3)
        ConditionFaceCompare* face = reinterpret_cast<ConditionFaceCompare*>(address);
        face->type = Condition_FaceCompare;
        face->flags = ConditionFaceCompare_Equal | ConditionFaceCompare_Greater;
        face->faceIndex = 0;
        writeConditionsOffsets[3] = offset;
        offset += sizeof(ConditionFaceCompare);
        address += sizeof(ConditionFaceCompare);
        // And matching action
        writeActions[3].type = Action_PlayAnimation;
        writeActions[3].animIndex = 6; // face led green
        writeActions[3].faceIndex = FACE_INDEX_CURRENT_FACE; // Doesn't actually matter
        writeActions[3].loopCount = 1;

        // Add Low Battery condition (index 4)
        ConditionBatteryState* low_batt = reinterpret_cast<ConditionBatteryState*>(address);
        low_batt->type = Condition_BatteryState;
        low_batt->flags = ConditionBatteryState_Flags::ConditionBatteryState_Low;
        low_batt->repeatPeriodMs = 30000; // 30s
        writeConditionsOffsets[4] = offset;
        offset += sizeof(ConditionBatteryState);
        address += sizeof(ConditionBatteryState);
        // And matching action
        writeActions[4].type = Action_PlayAnimation;
        writeActions[4].animIndex = 2; // face led red
        writeActions[4].faceIndex = 0;
        writeActions[4].loopCount = 1;

        // Add Charging condition (index 5)
        ConditionBatteryState* charge_batt = reinterpret_cast<ConditionBatteryState*>(address);
        charge_batt->type = Condition_BatteryState;
        charge_batt->flags = ConditionBatteryState_Flags::ConditionBatteryState_Charging;
        charge_batt->repeatPeriodMs = 5000; //s
        writeConditionsOffsets[5] = offset;
        offset += sizeof(ConditionBatteryState);
        address += sizeof(ConditionBatteryState);
        // And matching action
        writeActions[5].type = Action_PlayAnimation;
        writeActions[5].animIndex = 0; // face led red
        writeActions[5].faceIndex = DiceVariants::getTopFace();
        writeActions[5].loopCount = 1;

        // Add Done charging condition (index 6)
        ConditionBatteryState* done_charge = reinterpret_cast<ConditionBatteryState*>(address);
        done_charge->type = Condition_BatteryState;
        done_charge->flags = ConditionBatteryState_Done;
        done_charge->repeatPeriodMs = 5000; //s
        writeConditionsOffsets[6] = offset;
        offset += sizeof(ConditionBatteryState);
        address += sizeof(ConditionBatteryState);
        // And matching action
        writeActions[6].type = Action_PlayAnimation;
        writeActions[6].animIndex = 3; // face led green
        writeActions[6].faceIndex = DiceVariants::getTopFace();
        writeActions[6].loopCount = 1;

        // Add Bad charging condition (index 7)
        ConditionBatteryState* bad_charge = reinterpret_cast<ConditionBatteryState*>(address);
        bad_charge->type = Condition_BatteryState;
        bad_charge->flags = ConditionBatteryState_BadCharging;
        writeConditionsOffsets[7] = offset;
        offset += sizeof(ConditionBatteryState);
        address += sizeof(ConditionBatteryState);
        // And matching action
        writeActions[7].type = Action_PlayAnimation;
        writeActions[7].animIndex = 1; // face led red
        writeActions[7].faceIndex = DiceVariants::getTopFace();
        writeActions[7].loopCount = 1;

        // Add error during charging (usually temperature) condition (index 8)
        ConditionBatteryState* error_charge = reinterpret_cast<ConditionBatteryState*>(address);
        error_charge->type = Condition_BatteryState;
        error_charge->flags = ConditionBatteryState_Error;
        error_charge->repeatPeriodMs = 1500; //s
        writeConditionsOffsets[8] = offset;
        offset += sizeof(ConditionBatteryState);
        address += sizeof(ConditionBatteryState);
        // And matching action
        writeActions[8].type = Action_PlayAnimation;
        writeActions[8].animIndex = 7; // face led red fast
        writeActions[8].faceIndex = DiceVariants::getTopFace();
        writeActions[8].loopCount = 1;

        // Create action offsets
		for (int i = 0; i < actionCount; ++i) {
            writeActionsOffsets[i] = i * sizeof(ActionPlayAnimation);
		}

        // Add Rules
        for (int i = 0; i < ruleCount; ++i) {
            writeRules[i].condition = i;
            writeRules[i].actionOffset = i;
            writeRules[i].actionCount = 1;
        }

        // Add Behavior
        writeBehaviors[0].rulesOffset = 0;
        writeBehaviors[0].rulesCount = ruleCount;

        // NRF_LOG_INFO("Default Dataset Buffer size: %d bytes", bufferSize);
        // NRF_LOG_HEXDUMP_INFO(writeBuffer, bufferSize);
        // NRF_LOG_INFO("Dataset size: %d bytes", sizeof(Data));
        // NRF_LOG_HEXDUMP_INFO(newData, sizeof(Data));
#endif
        static auto programDefaultsToFlash = [](Flash::ProgramFlashFuncCallback callback) {
            Flash::write(nullptr, Flash::getDataSetDataAddress(), writeBuffer, bufferSize, callback);
        };

        Flash::programFlash(*newData, settingsPackAlong, programDefaultsToFlash, _setWrittenCallback);
	}
}
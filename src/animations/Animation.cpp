#include "animation.h"
#include "data_set/data_animation_bits.h"

#include "assert.h"
#include "../utils/utils.h"
#include "nrf_log.h"

#include "animation_simple.h"
#include "animation_gradient.h"
#include "animation_keyframed.h"
#include "animation_rainbow.h"
#include "animation_gradientpattern.h"
#include "animation_noise.h"
#include "animation_cycle.h"
#include "animation_blinkid.h"
#include "animation_normals.h"
#include "config/board_config.h"


// Define new and delete
void* operator new(size_t size) { return malloc(size); }
void operator delete(void* ptr) { free(ptr); }
void operator delete(void* ptr, unsigned int) { free(ptr); }


#define MAX_LEVEL (256)

using namespace Utils;
using namespace DataSet;
using namespace Config;

namespace Animations
{
	/// Dims the passed in color by the passed in intensity (normalized 0 - 255)
	/// </summary>
	uint32_t scaleColor(uint32_t refColor, uint8_t intensity)
	{
		uint8_t r = getRed(refColor);
		uint8_t g = getGreen(refColor);
		uint8_t b = getBlue(refColor);
		return toColor(r * intensity / MAX_LEVEL, g * intensity / MAX_LEVEL, b * intensity / MAX_LEVEL);
	}

	AnimationInstance::AnimationInstance(const Animation* preset, const AnimationBits* bits) 
		: animationPreset(preset)
		, animationBits(bits)
		, tag(AnimationTag_Unknown)
	{
	}

	AnimationInstance::~AnimationInstance() {
	}

	void AnimationInstance::setTag(AnimationTag _tag) {
		tag = _tag;
	}

	void AnimationInstance::start(int _startTime, uint8_t _remapFace, bool _loop) {
		startTime = _startTime;
		remapFace = _remapFace;
		forceFadeTime = -1;
		loop = _loop;
	}

	int AnimationInstance::setColor(uint32_t color, uint32_t faceMask, int retIndices[], uint32_t retColors[]) {
		auto b = BoardManager::getBoard();
		int c = b->ledCount;
		int retCount = 0;
		for (int i = 0; i < c; ++i) {
			if ((faceMask & (1 << i)) != 0) {
				retIndices[retCount] = i;
				retColors[retCount] = color;
				retCount++;
			}
		}
		return retCount;
	}

	int AnimationInstance::setIndices(uint32_t faceMask, int retIndices[]) {
		auto b = BoardManager::getBoard();
		int c = b->ledCount;
		int retCount = 0;
		for (int i = 0; i < c; ++i) {
			if ((faceMask & (1 << i)) != 0) {
				retIndices[retCount] = i;
				retCount++;
			}
		}
		return retCount;
	}

	void AnimationInstance::forceFadeOut(int fadeOutTime) {
		loop = false;
		if (forceFadeTime < startTime + animationPreset->duration) {
			forceFadeTime = fadeOutTime;
		}
		// Otherwise the anim will end sooner than the force fade out time, so
		// making it not loop is enough.
	}

	AnimationInstance* createAnimationInstance(const Animation* preset, const AnimationBits* bits) {
		AnimationInstance* ret = nullptr;
		switch (preset->type) {
			case Animation_Simple:
				// Maybe we'll pass an allocator at some point, this is the only place I've ever used a new in the firmware...
				ret = new AnimationInstanceSimple(static_cast<const AnimationSimple*>(preset), bits);
				break;
			case Animation_Gradient:
				ret = new AnimationInstanceGradient(static_cast<const AnimationGradient*>(preset), bits);
				break;
			case Animation_Rainbow:
				ret = new AnimationInstanceRainbow(static_cast<const AnimationRainbow*>(preset), bits);
				break;
			case Animation_Keyframed:
				ret = new AnimationInstanceKeyframed(static_cast<const AnimationKeyframed*>(preset), bits);
				break;
			case Animation_GradientPattern:
				ret = new AnimationInstanceGradientPattern(static_cast<const AnimationGradientPattern*>(preset), bits);
				break;
			case Animation_Noise:
				ret = new AnimationInstanceNoise(static_cast<const AnimationNoise*>(preset), bits);
				break;
			case Animation_Cycle:
				ret = new AnimationInstanceCycle(static_cast<const AnimationCycle *>(preset), bits);
				break;
			case Animation_BlinkId:
				ret = new AnimationInstanceBlinkId(static_cast<const AnimationBlinkId*>(preset), bits);
				break;
			case Animation_Normals:
				ret = new AnimationInstanceNormals(static_cast<const AnimationNormals*>(preset), bits);
				break;
			default:
				NRF_LOG_ERROR("Unknown animation preset type");
				break;
		}
		return ret;
	}

	void destroyAnimationInstance(AnimationInstance* animationInstance) {
		// Eventually we might use an allocator
		delete animationInstance;
	}
}


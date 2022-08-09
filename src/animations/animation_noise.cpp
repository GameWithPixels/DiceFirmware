#include "animation_noise.h"
#include "data_set/data_set.h"
#include "data_set/data_animation_bits.h"
#include "nrf_log.h"


namespace Animations
{
	/// <summary>
	/// constructor for rainbow animations
	/// Needs to have an associated preset passed in
	/// </summary>
	AnimationInstanceNoise::AnimationInstanceNoise(const AnimationNoise* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits) {
	}

	/// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceNoise::~AnimationInstanceNoise() {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceNoise::animationSize() const {
		return sizeof(AnimationNoise);
	}

	/// <summary>
	/// (re)Initializes the instance to animate leds. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceNoise::start(int _startTime, uint8_t _remapFace, bool _loop) {
		AnimationInstance::start(_startTime, _remapFace, _loop);
        //curRand = (uint16_t)(_startTime % (1 << 16));
	}

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceNoise::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        int time = ms - startTime;
		srand(time);
        //auto preset = getPreset();
		
		int delay = 70;				// delay in between LED updates
		int numFaces = rand()%2+1; 	// noticed that 1-3 or 1-4 faces is more or less how many faces we need to light up to mimic the noise pattern on the app

		uint32_t color = 0x110000;//gradient.evaluateColor(animationBits, gradientTime);
		
		int retcount = 0;
		
		if(time < past_time){
			past_time = 0;
			
		}
		if(time - past_time <= delay){
			for(int i = 0; i < numFaces; i++){
				retColors[i] = color;//rand()%0xFFFFFF;
				retIndices[i] = rand()%20;
			}
			retcount = numFaces;
		} else {
			past_time = time;
		}

		// Figure out the color from the gradient
        //auto& gradient = animationBits->getRGBTrack(preset->gradientTrackOffset);

        //int gradientTime = time * 1000 / preset->duration;
        // Fill the indices and colors for the anim controller to know how to update leds

		return retcount;
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceNoise::stop(int retIndices[]) {
		auto preset = getPreset();
		return setIndices(preset->faceMask, retIndices);
	}

	const AnimationNoise* AnimationInstanceNoise::getPreset() const {
        return static_cast<const AnimationNoise*>(animationPreset);
    }
}

#include "animation_noise.h"
#include "data_set/data_animation_bits.h"
#include "utils/Utils.h"
// #include "nrf_crypto.h"


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
        
		// initializing random number generator
		curRand = (uint16_t)(_startTime % (1 << 16));

		// initializing the durations and times of each blink
		for(int i = 0; i < 20; i++){
			individualFlashTimes[i] = 0;
			flashDurations[i] = 0;
		}
	}

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceNoise::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
		
        auto preset = getPreset();
		int time = ms - startTime;
		 
		int numFaces = curRand%2+1;

		// LEDs will pick an initial color from the overall gradient (generally black to white)
		auto& gradientOverall = animationBits->getRGBTrack(preset->overallGradientTrackOffset); 			
		// they will then fade according to the individual gradient
		auto& gradientIndividual = animationBits->getRGBTrack(preset->individualGradientTrackOffset);	

		// gradient time is an x-axis variable used to progress along a gradient and is normalized to range from 0-1000
		// eg: if we have a gradient that goes r->g->b 
        int gradientTime = time*1000/preset->duration;
		uint32_t firstColor = gradientOverall.evaluateColor(animationBits, gradientTime);

		int retCount = 0; // number that indicates how many LEDS to light up in ther current cycle

		// setting which faces to turn on setting the start of the individualFlashtimes of the faces that need to be turned on to the current time
		if(time - previousFlashTime >= preset->duration/preset->flashCount){
			for(int i = 0; i < numFaces; i++){
				int faceIndex = curRand%20;
				curRand = Utils::nextRand(curRand);
				individualFlashTimes[faceIndex] = time;
				// causes stretching of the duration of the flash based onthe duration of the actual animation
				flashDurations[faceIndex] = preset->duration * preset->flashDuration / 255 + curRand % 20;
			}
			
			previousFlashTime = time; 
		}
		
		// Setting the colors for each of the faces that we randomly selected in the previous loop according to the mix between the overall and individual gradient
		for(int i = 0; i < 20; i++){
			int timeFlash = time - individualFlashTimes[i];
			if(timeFlash < flashDurations[i]){
				int fadeTime = (flashDurations[i] * preset->fade) / (255 * 2);

				// the flash will fade according to the individual gradient
				uint32_t secondColor = gradientIndividual.evaluateColor(animationBits, timeFlash*1000/flashDurations[i] );

				// mixing the color acquired from the general gradient with the individual gradient
				uint32_t mixedColor = Utils::toColor((Utils::getRed(firstColor) * Utils::getRed(secondColor))/0xFF, // component-wise mixing of colors
									(Utils::getGreen(firstColor) * Utils::getGreen(secondColor))/0xFF, 				
									(Utils::getBlue(firstColor) * Utils::getBlue(secondColor))/0xFF);				

				// determining whether we should increase/decrease the intensity based on how far each individual flash has progressed (timeFlash) so as to mimic a fade
				if(timeFlash <= fadeTime){
					retColors[retCount] = Utils::modulateColor(mixedColor, timeFlash * 255 / fadeTime );
				} else if(timeFlash >= flashDurations[i] - fadeTime){
					retColors[retCount] = Utils::modulateColor(mixedColor, ((flashDurations[i] - timeFlash) * 255 / fadeTime));
				} else {
					retColors[retCount] = mixedColor;
				}
				retIndices[retCount] = i;
				retCount++;					
			}
		}
		return retCount;
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceNoise::stop(int retIndices[]) {
		auto preset = getPreset();
		return setIndices(ANIM_FACEMASK_ALL_LEDS, retIndices);
	}

	const AnimationNoise* AnimationInstanceNoise::getPreset() const {
        return static_cast<const AnimationNoise*>(animationPreset);
    }
}

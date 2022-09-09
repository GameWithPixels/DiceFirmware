#include "animation_noise.h"
#include "data_set/data_animation_bits.h"
#include "utils/Utils.h"
#include "config/board_config.h"

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
        ledCount = Config::BoardManager::getBoard()->ledCount;
		// initializing random number generator
		curRand = (uint16_t)(_startTime % (1 << 16));

		// initializing the durations and times of each blink
		for(int i = 0; i < MAX_LED_COUNT; i++){
			individualBlinkTimes[i] = 0;
			blinkDurations[i] = 0;
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

		// setting which faces to blink as well as the start time of each of their blinks in individualBlinkTimes, also adding some variation to each of their durations
		if(time - previousBlinkTime >= preset->duration/preset->blinkCount){
			for(int i = 0; i < numFaces; i++){
				curRand = Utils::nextRand(curRand);
				int faceIndex = curRand%ledCount;
				
				individualBlinkTimes[faceIndex] = time;
				// causes stretching of the duration of the blink based onthe duration of the actual animation
				curRand = Utils::nextRand(curRand);
				blinkDurations[faceIndex] = preset->duration * preset->blinkDuration / 255 + curRand % 20;
			}
			
			previousBlinkTime = time; 
		}
		
		// then setting the colors for each of the faces that we randomly selected in the previous loop according to the mix of color/intensity between the overall and individual gradient
		for(int i = 0; i < ledCount; i++){
			int timeBlink = time - individualBlinkTimes[i];
			if(timeBlink < blinkDurations[i]){
				int fadeTime = (blinkDurations[i] * preset->fade) / (255 * 2);

				// the blink will fade according to the individual gradient
				uint32_t secondColor = gradientIndividual.evaluateColor(animationBits, timeBlink*1000/blinkDurations[i] );

				// mixing the color acquired from the general gradient with the individual gradient
				uint32_t mixedColor = Utils::toColor((Utils::getRed(firstColor) * Utils::getRed(secondColor))/0xFF, // component-wise mixing of colors
									(Utils::getGreen(firstColor) * Utils::getGreen(secondColor))/0xFF, 				
									(Utils::getBlue(firstColor) * Utils::getBlue(secondColor))/0xFF);				

				// determining whether we should increase/decrease the intensity based on how far each individual blink has progressed (timeBlink) so as to mimic a fade
				if(timeBlink <= fadeTime){
					retColors[retCount] = Utils::modulateColor(mixedColor, timeBlink * 255 / fadeTime );
				} else if(timeBlink >= blinkDurations[i] - fadeTime){
					retColors[retCount] = Utils::modulateColor(mixedColor, ((blinkDurations[i] - timeBlink) * 255 / fadeTime));
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
		return setIndices(ANIM_FACEMASK_ALL_LEDS, retIndices);
	}

	const AnimationNoise* AnimationInstanceNoise::getPreset() const {
        return static_cast<const AnimationNoise*>(animationPreset);
    }
}

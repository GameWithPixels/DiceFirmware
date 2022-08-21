#include "animation_noise.h"
#include "data_set/data_set.h"
#include "data_set/data_animation_bits.h"
#include "utils/Utils.h"
#include "nrf_log.h"
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
        //curRand = (uint16_t)(_startTime % (1 << 16));
		// nrf_crypto_init();
		// nrf_crypto_rng_init(nullptr, nullptr);


		animationBits.palette = animPalette; // point to animpalette
		animationBits.paletteSize = 21;

		rgbKeyframes[0].setTimeAndColorIndex(0, 0);
		rgbKeyframes[1].setTimeAndColorIndex(1000, 1);
		rgbKeyframes[2].setTimeAndColorIndex(0, 3);
		rgbKeyframes[3].setTimeAndColorIndex(500, 2);
		rgbKeyframes[4].setTimeAndColorIndex(1000, 4);

		animationBits.rgbKeyframes = rgbKeyframes;
		animationBits.rgbKeyFrameCount = 5;

		rgbTracks[0] = {.keyframesOffset = 0, .keyFrameCount = 2,  .padding = 0, .ledMask = 0x000FFFFF};
		rgbTracks[1] = {.keyframesOffset = 2, .keyFrameCount = 3,  .padding = 0, .ledMask = 0x000FFFFF};
		animationBits.rgbTracks = rgbTracks;
		animationBits.rgbTrackCount = 2;

		keyframes[0].setTimeAndIntensity(0, 0);
		keyframes[1].setTimeAndIntensity(250, 255);
		keyframes[2].setTimeAndIntensity(500, 0);
		keyframes[3].setTimeAndIntensity(750, 255);
		
		intensityTracks = {.keyframesOffset = 0, .keyFrameCount = 2, .padding = 0, .ledMask = 0x000FFFFF};
		animationBits.keyframes = keyframes;
		animationBits.keyFrameCount = 4;
		animationBits.tracks = &intensityTracks;
		animationBits.trackCount = 1;
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
		// uint8_t* buffer = new uint8_t;
		// nrf_crypto_rng_vector_generate(buffer, 1);
		srand(time);
		
		int numFaces = rand()%2+1;//(*buffer)%2+1; 	// noticed that 1-2/3 faces is more or less how many faces we need to light up per cycle to mimic the noise pattern on the app

		// overall gradient color management
		auto& gradientOverall = animationBits.getRGBTrack(preset->gradientTrackOffset); 
		auto& gradientPersonal = animationBits.getRGBTrack(preset->gradientTrackOffset+1);
		auto& gradientIntensity = animationBits.getTrack(preset->gradientTrackOffset);
		// gradient time initialization
        int gradientTime = time*1000/preset->duration;
		uint32_t firstColor = gradientOverall.evaluateColor(&animationBits, gradientTime);

		if(time < past_time){
			past_time = 0;
		}
		
		int retCount = 0;
		if(time - past_time >= preset->flashDelay){
			for(int i = 0; i < numFaces; i++){
				if ( (preset->faceMask & (1 << i)) != 0 ){
					int retIndex = rand()%20;
					retIndices[i] = retIndex;
					retColors[i] = firstColor;
					intensityTrackTimes[retIndex] = 255;
					retCount++;
				}
			}
			past_time = time;
		}

		for(int i = 0; i < 20; i++){
			if(intensityTrackTimes[i] != 0 && intensityTrackTimes[i] != 255){
				uint32_t secondColor = gradientPersonal.evaluateColor(&animationBits, intensityTrackTimes[i]*1000/255 );
				uint32_t mixedColor = Utils::toColor((Utils::getRed(firstColor) * Utils::getRed(secondColor))/0xFF, (Utils::getGreen(firstColor) * Utils::getGreen(secondColor))/0xFF, (Utils::getBlue(firstColor) * Utils::getBlue(secondColor))/0xFF);
				//NRF_LOG_INFO("firstColor: %d,  mixedColor: %d, secondColor: %d,", firstColor, mixedColor, secondColor);
				retColors[retCount] = gradientIntensity.modulateColor(&animationBits, mixedColor, intensityTrackTimes[i]*1000/255);
				retIndices[retCount] = i;
				retCount++;
			}
		}

		for(int i = 0; i < 20; i++){
			if(intensityTrackTimes[i] <= preset->flashSpeed){
				intensityTrackTimes[i] = 0;
			} else {
				intensityTrackTimes[i] -= preset->flashSpeed;
			}
		}
		return retCount;
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

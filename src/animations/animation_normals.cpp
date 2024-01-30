#include "animation_normals.h"
#include "data_set/data_animation_bits.h"
#include "board_config.h"
#include "dice_variants.h"
#include "utils/Utils.h"
#include "nrf_log.h"
#include "utils/Rainbow.h"
#include "modules/accelerometer.h"
#include "drivers_nrf/rng.h"

using namespace DriversNRF;
using namespace Modules;

namespace Animations
{
	/// <summary>
	/// </summary>
	AnimationInstanceNormals::AnimationInstanceNormals(const AnimationNormals* preset, const DataSet::AnimationBits* bits)
		: AnimationInstance(preset, bits) {
	}

	/// <summary>
	/// destructor
	/// </summary>
	AnimationInstanceNormals::~AnimationInstanceNormals() {
	}

	/// <summary>
	/// Small helper to return the expected size of the preset data
	/// </summary>
	int AnimationInstanceNormals::animationSize() const {
		return sizeof(AnimationNormals);
	}

	/// <summary>
	/// (re)Initializes the instance to animate leds. This can be called on a reused instance.
	/// </summary>
	void AnimationInstanceNormals::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
		AnimationInstance::start(_startTime, _remapFace, _loopCount);

        // Grab the die normals
        auto layout = Config::DiceVariants::getLayout();
        normals = layout->baseNormals;

        // Grab the orientation normal, based on the current face
        uint8_t face = Config::DiceVariants::getTopFace();
        faceNormal = &normals[face];
        int backFaceOffset = 1;
        Core::int3 backVectorNormal = normals[(face + backFaceOffset) % layout->faceCount];
        while (abs(Core::int3::dotTimes1000(*faceNormal, backVectorNormal)) > 800 && backFaceOffset < layout->faceCount) {
            backFaceOffset += 1;
            backVectorNormal = normals[(face + backFaceOffset) % layout->faceCount];
        }
        
        // Compute our base vectors, up is aligned with current face, and
        // a back is at 90 degrees from that.
        auto cross = Core::int3::cross(*faceNormal, backVectorNormal);
        cross.normalize();
        backVector = Core::int3::cross(cross, *faceNormal);

        // For color override, precompute parameter
        auto preset = getPreset();
        switch (preset->mainGradientColorType) {
            case NormalsColorOverrideType_FaceToGradient:
        		baseColorParam = (Accelerometer::currentFace() * 1000) / Config::DiceVariants::getLayout()->faceCount;
                break;
            case NormalsColorOverrideType_FaceToRainbowWheel:
        		baseColorParam = (Accelerometer::currentFace() * 256) / Config::DiceVariants::getLayout()->faceCount;
                break;
            case NormalsColorOverrideType_None:
            default:
                break;
        }
	}

	/// <summary>
	/// Computes the list of LEDs that need to be on, and what their intensities should be.
	/// </summary>
	/// <param name="ms">The animation time (in milliseconds)</param>
	/// <param name="retIndices">the return list of LED indices to fill, max size should be at least 21, the max number of leds</param>
	/// <param name="retColors">the return list of LED color to fill, max size should be at least 21, the max number of leds</param>
	/// <returns>The number of leds/intensities added to the return array</returns>
	int AnimationInstanceNormals::updateLEDs(int ms, int retIndices[], uint32_t retColors[]) {
        int time = ms - startTime;
        auto preset = getPreset();
		int fadeTime = preset->duration * preset->fade / (255 * 2);

		uint8_t intensity = 255;
		if (time <= fadeTime) {
			// Ramp up
			intensity = (uint8_t)(time * 255 / fadeTime);
		} else if (time >= (preset->duration - fadeTime)) {
			// Ramp down
			intensity = (uint8_t)((preset->duration - time) * 255 / fadeTime);
		}

        int axisScrollTime = time * preset->axisScrollSpeedTimes1000 / preset->duration;
        int angleScrollTime = time * preset->angleScrollSpeedTimes1000 / preset->duration;
        int gradientTime = time * 1000 / preset->duration;

        // Figure out the color from the gradient
        auto& gradient = animationBits->getRGBTrack(preset->gradientOverTime);
        auto& axisGradient = animationBits->getRGBTrack(preset->gradientAlongAxis);
        auto& angleGradient = animationBits->getRGBTrack(preset->gradientAlongAngle);
        auto layout = Config::DiceVariants::getLayout();
        for (int i = 0; i < layout->ledCount; ++i) {
            int face = Config::DiceVariants::getFaceForLEDIndex(i);

            // Compute color relative to up/down angle (based on the angle to axis)
            // We'll extract the angle from the dot product of the face's normal and the axis
            int dotAxisTimes1000 = Core::int3::dotTimes1000(*faceNormal, normals[face]);

            // remap the [-1000, 1000] range to an 8 bit value usable by acos8
            uint8_t dotAxis8 = (dotAxisTimes1000 * 1275 + 1275000) / 10000;

            // Use lookup acos table
            int angleToAxis8 = Utils::acos8(dotAxis8);

            // remap 8 bit value to [-1000, 1000] range
            int angleToAxisNormalized = (angleToAxis8 - 128) * 1000 / 128;

            // Scale / Offset the value so we can use a smaller subset of the gradient
            int axisGradientBaseTime = angleToAxisNormalized * 1000 / preset->axisScaleTimes1000 + preset->axisOffsetTimes1000;

            // Add motion
            int axisGradientTime = axisGradientBaseTime + axisScrollTime;

            // Compute color along axis
            uint32_t axisColor = axisGradient.evaluateColor(animationBits, axisGradientTime);

            // Compute color relative to up/down angle (angle to axis), we'll use the dot product to the back vector

            // Start by getting a properly normalized in-plane direction vector
            Core::int3 inPlaneNormal = normals[face] - *faceNormal * dotAxisTimes1000;
            inPlaneNormal.normalize();

            // Compute dot product and extract angle
            int dotBackTimes1000 = Core::int3::dotTimes1000(backVector, inPlaneNormal);
            int dotBack8 = (dotBackTimes1000 * 1275 + 1275000) / 10000;
            int angleToBack8 = Utils::acos8(dotBack8);

            // Oops, we need full range so check cross product with axis to swap the sign as needed
            if (Core::int3::dotTimes1000(Core::int3::cross(backVector, normals[face]), *faceNormal) < 0) {
                // Negate the angle
                angleToBack8 = 255 - angleToBack8;
            }

            // Remap to proper range
            int angleToBackTimes1000 = (angleToBack8 - 128) * 1000 / 128;
            int angleGradientNormalized = (angleToBackTimes1000 + 1000) / 2;

            // Angle is animated and wrapped around
            int angleGradientTime = (angleGradientNormalized + angleScrollTime) % 1000;

            // Compute color along angle
            uint32_t angleColor = angleGradient.evaluateColor(animationBits, angleGradientTime);

            // Compute color over time
			uint32_t gradientColor = 0;
			switch (preset->mainGradientColorType) {
				case NormalsColorOverrideType_FaceToGradient:
                    {
                        // use the current face (set at start()) + variance
                        int gradientParam = baseColorParam + angleToAxisNormalized * preset->mainGradientColorVar / 1000;
                        gradientColor = gradient.evaluateColor(animationBits, gradientParam);
                    }
					break;
				case NormalsColorOverrideType_FaceToRainbowWheel:
                    {
                        // use the current face (set at start()) + variance
                        int rainbowParam = (baseColorParam + angleToAxisNormalized * preset->mainGradientColorVar * 256 / 1000000) % 256;
                        gradientColor = Rainbow::wheel(rainbowParam);
                    }
					break;
				case NormalsColorOverrideType_None:
				default:
					gradientColor = gradient.evaluateColor(animationBits, gradientTime);
					break;
			}

            retIndices[i] = i;
            retColors[i] = Utils::modulateColor(Utils::mulColors(gradientColor, Utils::mulColors(axisColor, angleColor)), intensity);
        }
        return layout->ledCount;
	}

	/// <summary>
	/// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
	/// </summary>
	int AnimationInstanceNormals::stop(int retIndices[]) {
		return setIndices(ANIM_FACEMASK_ALL_LEDS, retIndices);
	}

	const AnimationNormals* AnimationInstanceNormals::getPreset() const {
        return static_cast<const AnimationNormals*>(animationPreset);
    }
}

#include "animation_normals.h"
#include "data_set/data_animation_bits.h"
#include "board_config.h"
#include "dice_variants.h"
#include "utils/Utils.h"

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
	void AnimationInstanceNormals::start(int _startTime, uint8_t _remapFace, bool _loop) {
		AnimationInstance::start(_startTime, _remapFace, _loop);

        // Grab the die normals
        auto layout = Config::DiceVariants::getLayout();
        normals = layout->baseNormals;

        // Grab the orientation normal, based on the current face
        uint8_t face = _remapFace;
        if (face == 0xFF) {
            face = Config::DiceVariants::getTopFace();
        }
        faceNormal = &normals[face];
        int backFaceOffset = 1;
        Core::int3 backVectorNormal = normals[(face + backFaceOffset) % layout->faceCount];
        while (abs(Core::int3::dotTimes1000(*faceNormal, backVectorNormal)) > 800 && backFaceOffset < layout->faceCount) {
            backFaceOffset += 1;
            backVectorNormal = normals[(face + backFaceOffset) % layout->faceCount];
        }
        
        auto cross = Core::int3::cross(*faceNormal, backVectorNormal);
        cross.normalize();
        backVector = Core::int3::cross(cross, *faceNormal);
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

        int axisScrollTime = time * preset->axisScrollSpeedTimes1000 / preset->duration;
        int angleScrollTime = time * preset->angleScrollSpeedTimes1000 / preset->duration;
        int gradientTime = time * 1000 / preset->duration;

        // Figure out the color from the gradient
        auto& gradient = animationBits->getRGBTrack(preset->gradient);
        auto& axisGradient = animationBits->getRGBTrack(preset->gradientAlongAxis);
        auto& angleGradient = animationBits->getRGBTrack(preset->gradientAlongAngle);
        auto layout = Config::DiceVariants::getLayout();
        for (int i = 0; i < layout->faceCount; ++i) {
            // Compute color relative to up/down angle (angle to axis)
            int dotAxisTimes1000 = Core::int3::dotTimes1000(*faceNormal, normals[i]);
            int dotAxis8 = (dotAxisTimes1000 * 1275 + 1275000) / 10000;
            int angleToAxis8 = Utils::acos8(dotAxis8);
            int angleToAxisNormalized = (angleToAxis8 - 128) * 1000 / 128;
            int axisGradientBaseTime = angleToAxisNormalized;
            int axisGradientTime = (axisGradientBaseTime + axisScrollTime) % 1000;
            uint32_t axisColor = axisGradient.evaluateColor(animationBits, axisGradientTime);

            // Compute color relative to up/down angle (angle to axis)
            int dotBackTimes1000 = Core::int3::dotTimes1000(backVector, normals[i]);
            int dotBack8 = (dotBackTimes1000 * 1275 + 1275000) / 10000;
            int angleToBack8 = Utils::acos8(dotBack8);
            if (Core::int3::dotTimes1000(Core::int3::cross(backVector, normals[i]), *faceNormal) > 0) {
                // Negate the angle
                angleToBack8 = 255 - angleToBack8;
            }
            int angleToBackTimes1000 = (angleToBack8 - 128) * 1000 / 128;
            int angleGradientBaseTime = (angleToBackTimes1000 + 1000) / 2;
            int angleGradientTime = (angleGradientBaseTime + angleScrollTime) % 1000;
            uint32_t angleColor = angleGradient.evaluateColor(animationBits, angleGradientTime);

            uint32_t gradientColor = gradient.evaluateColor(animationBits, gradientTime);

            //int gradientTime = (gradientBaseTime) % 1000;
            retIndices[i] = i;
            retColors[i] = Utils::mulColors(gradientColor, Utils::mulColors(axisColor, angleColor));
        }
        return layout->faceCount;
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

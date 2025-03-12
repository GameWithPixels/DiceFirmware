#include "animation_blinkid.h"
#include "modules/anim_controller.h"
#include "pixel.h"
#include "settings.h"

#define HEADER_BITS_COUNT 3
#define DEVICE_BITS_COUNT (8 * sizeof(Pixel::getDeviceID()))
#define CRC_BITS_COUNT 3
#define CRC_DIVISOR 0xB // = 1011
#define CRC_MASK 0x7

using namespace Config;

namespace Animations
{

    /// <summary>
    /// Update the animation duration based on the passed preamble duration
    /// and number of frames per blink.
    /// </summary>
    void AnimationBlinkId::setDuration(uint16_t preambleDuration)
    {
        // Add preamble duration to time it takes to blink the header and the device id
        duration = preambleDuration + ANIM_FRAME_DURATION_MS * framesPerBlink * (HEADER_BITS_COUNT + DEVICE_BITS_COUNT);
    }

    /// <summary>
    /// constructor for simple animations
    /// Needs to have an associated preset passed in
    /// </summary>
    AnimationInstanceBlinkId::AnimationInstanceBlinkId(const AnimationBlinkId *preset, const DataSet::AnimationBits *bits)
        : AnimationInstance(preset, bits), message(AnimationInstanceBlinkId::getMessage())
    {
    }

    uint64_t AnimationInstanceBlinkId::getMessage()
    {
        // 3-bit CRC
        // https://en.wikipedia.org/wiki/Cyclic_redundancy_check#Computation
        const uint64_t shiftedValue = (uint64_t)Pixel::getDeviceID() << CRC_BITS_COUNT;
        const uint64_t mask = (uint64_t)(-1) ^ CRC_MASK;
        uint64_t div = (uint64_t)CRC_DIVISOR << DEVICE_BITS_COUNT;
        uint64_t crc = shiftedValue;
        uint64_t firstBit = (uint64_t)1 << (DEVICE_BITS_COUNT + CRC_BITS_COUNT);
        do
        {
            while ((crc & firstBit) == 0)
            {
                firstBit >>= 1;
                div >>= 1;
            }
            crc ^= div;
        } while ((crc & mask) != 0);
        return (shiftedValue | crc) << HEADER_BITS_COUNT; // Shift the results to add the initial "RGB" header
    }

    /// <summary>
    /// destructor
    /// </summary>
    AnimationInstanceBlinkId::~AnimationInstanceBlinkId()
    {
    }

    /// <summary>
    /// Small helper to return the expected size of the preset data
    /// </summary>
    int AnimationInstanceBlinkId::animationSize() const
    {
        return sizeof(AnimationBlinkId);
    }

    /// <summary>
    /// (re)Initializes the instance to animate leds. This can be called on a reused instance.
    /// </summary>
    void AnimationInstanceBlinkId::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount)
    {
        AnimationInstance::start(_startTime, _remapFace, _loopCount);
    }

    /// <summary>
    /// Computes the list of LEDs that need to be on, and what their intensities should be.
    /// </summary>
    void AnimationInstanceBlinkId::updateDaisyChainLEDs(int ms, uint32_t* outDaisyChainColors)
    {
        auto preset = getPreset();

        // Compute color
        uint32_t color = 0;
        const uint32_t brightness = (uint32_t)preset->brightness;
        const uint32_t frameCounter = (ms - startTime) / ANIM_FRAME_DURATION_MS;
        const uint32_t tick = frameCounter / preset->framesPerBlink;
        const uint32_t totalTicks = preset->duration / preset->framesPerBlink / ANIM_FRAME_DURATION_MS;
        const uint32_t preambleNumTicks = totalTicks - DEVICE_BITS_COUNT - HEADER_BITS_COUNT - CRC_BITS_COUNT;

        if (tick < preambleNumTicks || tick >= totalTicks)
        {
            // Show white for the preamble (and possibly the last frame)
            auto whiteBrightness = brightness / 2;
            color = (whiteBrightness << 16) | (whiteBrightness << 8) | whiteBrightness;
        }
        else
        {
            // Find the color index of the current bit of our message
            // Send lower order bit first (which is the header,
            // then CRC and finally the device id)
            uint64_t msg = message;
            uint32_t colorIndex = -1;
            for (uint32_t i = preambleNumTicks; i <= tick; ++i)
            {
                // Skip a color when getting a 1
                colorIndex += 1 + (msg & 1);
                msg >>= 1;
            }

            // We use 3 colors
            colorIndex %= 3;

            // colorIndex = 0 => red, 1 => green, 2 => blue
            color = brightness << (16 - 8 * colorIndex);
        }

        auto layout = SettingsManager::getLayout();
        for (int i = 0; i < layout->ledCount; ++i)
        {
            outDaisyChainColors[i] = color;
        }
    }

    /// <summary>
    /// Clear all LEDs controlled by this animation, for instance when the anim gets interrupted.
    /// </summary>
    int AnimationInstanceBlinkId::stop(int retIndices[])
    {
        return setIndices(ANIM_FACEMASK_ALL_LEDS, retIndices);
    }

    const AnimationBlinkId* AnimationInstanceBlinkId::getPreset() const
    {
        return static_cast<const AnimationBlinkId*>(animationPreset);
    }
}

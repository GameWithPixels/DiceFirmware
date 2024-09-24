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
#include "animation_sequence.h"
#include "animation_worm.h"
#include "config/settings.h"
#include "config/dice_variants.h"


// Define new and delete
void* operator new(size_t size) { return malloc(size); }
void operator delete(void* ptr) { free(ptr); }
void operator delete(void* ptr, unsigned int) { free(ptr); }


#define MAX_LEVEL (256)
#define MAX_BLENDED_COLORS (8)

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

    void AnimationInstance::start(int _startTime, uint8_t _remapFace, uint8_t _loopCount) {
        startTime = _startTime;
        remapFace = _remapFace;
        forceFadeTime = -1;
        loopCount = _loopCount;
    }

    int AnimationInstance::setColor(uint32_t color, uint32_t faceMask, int retIndices[], uint32_t retColors[]) {
        int c = SettingsManager::getLayout()->faceCount;
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
        int c = SettingsManager::getLayout()->faceCount;
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
        loopCount = 1;
        if (forceFadeTime < startTime + animationPreset->duration) {
            forceFadeTime = fadeOutTime;
        }
        // Otherwise the anim will end sooner than the force fade out time, so
        // making it not loop is enough.
    }

    int animIndices[MAX_COUNT];
    uint32_t animColors[MAX_COUNT];
    void AnimationInstance::updateLEDs(int ms, uint32_t* outDaisyChainColors) {

        // Update the (derived) animation instance
        int animColorCount = update(ms, animIndices, animColors);

        // Flatten the colors
        uint32_t colors[MAX_COUNT];
        memset(colors, 0, sizeof(uint32_t) * MAX_COUNT);
        for (int i = 0; i < animColorCount; ++i) {
            int c = animIndices[i];
            if (c >= 0 && c < MAX_COUNT) {
                colors[c] = animColors[i];
            }
        }

        // Do a bunch of remapping / blending based on the animation flags and layout
        auto layout = SettingsManager::getLayout();

        // Now figure out what color each LED needs
        // Remap "electrical" index (daisy chain index) to "logical" led index
        for (int l = 0; l < layout->ledCount; ++l) {

            // Remap logical led index to face indices (there may be more than one)
            int animIndices[MAX_BLENDED_COLORS];
            int animIndexCount = 1;

            if (animationPreset->getIndexType() == AnimationIndexType_Face || animationPreset->getIndexType() == AnimationIndexType_Led) {
                int ll = layout->LEDIndexFromDaisyChainIndex(l);
                if (animationPreset->getIndexType() == AnimationIndexType_Face) {
                    int faces[MAX_BLENDED_COLORS];
                    animIndexCount = layout->faceIndicesFromLEDIndex(ll, faces);
                    // Animation specifies face indices, meaning if there are more than one led on a face,
                    // they will all get the same color from the animation.
                    for (int f = 0; f < animIndexCount; ++f) {
                        animIndices[f] = layout->remapFaceIndexBasedOnUpFace(remapFace, faces[f]);
                    }
                } else {
                    // Remap LED Indices instead
                    animIndexCount = layout->remapLEDIndexBasedOnUpFace(remapFace, ll, animIndices);
                }
            } else {
                // Daisy chain - no remapping
                animIndices[0] = l;
            }

            // Blend the colors together
            if (animIndexCount == 0) {
                // No face, no color
                outDaisyChainColors[l] = 0;
            } else if (animIndexCount == 1) {
                // One face, copy the color
                outDaisyChainColors[l] = colors[animIndices[0]];
            } else {
                // Multiple faces, average the colors
                uint32_t r = 0;
                uint32_t g = 0;
                uint32_t b = 0;
                for (int i = 0; i < animIndexCount; ++i) {
                    uint32_t faceColor = colors[animIndices[i]];
                    r += getRed(faceColor);
                    g += getGreen(faceColor);
                    b += getBlue(faceColor);
                }
                r /= animIndexCount;
                g /= animIndexCount;
                b /= animIndexCount;

                // Set the led color
                outDaisyChainColors[l] = toColor(r, g, b);
            }
        }
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
            case Animation_Sequence:
                ret = new AnimationInstanceSequence(static_cast<const AnimationSequence*>(preset), bits);
                break;
            case Animation_Worm:
                ret = new AnimationInstanceWorm(static_cast<const AnimationWorm*>(preset), bits);
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


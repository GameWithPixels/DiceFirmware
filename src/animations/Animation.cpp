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

    /*virtual*/ 
    int AnimationInstance::update(int ms, int retIndices[], uint32_t retColors[]) {
        // Base doesn't set any LED. Derived classes should override this.
        return 0;
    }

    /*virtual*/ 
    void AnimationInstance::updateFaces(int ms, uint32_t* outFaces) {

        auto layout = SettingsManager::getLayout();

        // Update the (derived) animation instance
        int animIndices[MAX_COUNT];
        uint32_t animColors[MAX_COUNT];
        int animColorCount = update(ms, animIndices, animColors);

        // Flatten the colors
        memset(outFaces, 0, sizeof(uint32_t) * MAX_COUNT);
        for (int i = 0; i < animColorCount; ++i) {
            int face = animIndices[i];

            // Remap the faces as necessary
            int remappedFace = layout->remapFaceIndexBasedOnUpFace(remapFace, face);
            outFaces[remappedFace] = animColors[i];
        }
    }

    /*virtual*/ 
    void AnimationInstance::updateLEDs(int ms, uint32_t* outLEDs) {

        uint32_t faceColors[MAX_COUNT];
        memset(faceColors, 0, sizeof(uint32_t) * MAX_COUNT);
        updateFaces(ms, faceColors);

        // Do a bunch of remapping / blending based on the animation flags and layout
        auto layout = SettingsManager::getLayout();

        // Now figure out what color each LED needs
        // Remap "electrical" index (daisy chain index) to "logical" led index
        for (int l = 0; l < layout->ledCount; ++l) {

            // Animation specifies face indices, meaning if there are more than one led on a face,
            // they will all get the same color from the animation.
            int faces[MAX_BLENDED_COLORS];
            int faceCount = layout->faceIndicesFromLEDIndex(l, faces);

            // Blend the colors together
            if (faceCount == 0) {
                // No face, no color
                outLEDs[l] = 0;
            } else if (faceCount == 1) {
                // One face, copy the color
                outLEDs[l] = faceColors[faces[0]];
            } else {
                // Multiple faces, average the colors
                uint32_t r = 0;
                uint32_t g = 0;
                uint32_t b = 0;
                for (int i = 0; i < faceCount; ++i) {
                    uint32_t faceColor = faceColors[faces[i]];
                    r += getRed(faceColor);
                    g += getGreen(faceColor);
                    b += getBlue(faceColor);
                }
                r /= faceCount;
                g /= faceCount;
                b /= faceCount;

                // Set the led color
                outLEDs[l] = toColor(r, g, b);
            }
        }
    }

    /*virtual*/ 
    void AnimationInstance::updateDaisyChainLEDs(int ms, uint32_t* outDaisyChainColors) {


        // Do a bunch of remapping / blending based on the animation flags and layout
        auto layout = SettingsManager::getLayout();

        uint32_t ledColors[MAX_COUNT];
        memset(ledColors, 0, sizeof(uint32_t) * MAX_COUNT);
        updateLEDs(ms, ledColors);

        // Remap "electrical" index (daisy chain index) to "logical" led index
        for (int l = 0; l < layout->ledCount; ++l) {

            int ll = layout->LEDIndexFromDaisyChainIndex(l);
            outDaisyChainColors[l] = (ll != 255) ? ledColors[ll] : 0;
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


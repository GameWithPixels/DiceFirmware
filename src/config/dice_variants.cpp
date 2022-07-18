#include "dice_variants.h"
#include "board_config.h"

using namespace Core;

namespace Config
{
namespace DiceVariants
{
	uint8_t Layout::animIndexToLEDIndex(int animFaceIndex, int remapFace) {
		// The transformation is:
		// animFaceIndex (what face the animation says it wants to light up)
		//	-> rotatedAnimFaceIndex (based on remapFace and remapping table, i.e. what actual
		//	   face should light up to "retarget" the animation around the current up face)
		//		-> ledIndex (based on pcb face to led mapping, i.e. to account for the internal rotation
		//		   of the PCB and the fact that the LEDs are not accessed in the same order as the number of the faces)
		int rotatedAnimFaceIndex = faceRemap[remapFace * faceCount + animFaceIndex];
		return faceToLedLookup[rotatedAnimFaceIndex];
	}
}
}
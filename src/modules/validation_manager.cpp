#include "validation_manager.h"
#include "animations/animation.h"
#include "config/board_config.h"
#include "nrf_log.h"
#include "modules/anim_controller.h"
#include "animations/animation_name.h"
#include "animations/animation_simple.h"

using namespace Animations;
using namespace Modules;
using namespace Config;


namespace Modules::ValidManager
{
	void skipFeed(void* token);
	static AnimationName nameAnim;

	// Initializes validation animation objects and hooks AnimController callback
	void init() {
		// Determine which board is running
		int ledCount = BoardManager::getBoard()->ledCount;

		// Name animation object
        nameAnim.type = Animation_Name;
        switch (ledCount) 	// Change duration and count by board type
        {
			// D20
            case 20:
                nameAnim.preamble_count = 4;
                nameAnim.duration = 1188;
                break;
			
			// D6
			case 6:
                nameAnim.preamble_count = 6;
                nameAnim.duration = 1254;
                break;
        }

		// Assign callback for AnimController to skip PowerManager::feed
		AnimController::hook(skipFeed, nullptr);

		NRF_LOG_INFO("Validation Manager Initialized");
	}

	// Function for playing validation animations
	void onDiceInitialized() {
		NRF_LOG_INFO("Loop name animation");

		// Play preamble/name animation
        AnimController::play(&nameAnim, nullptr, 0, true); 		// Loop forever!
	}

	// Callback for skipping PowerManager::feed call in AnimController
	void skipFeed(void* token) {
		return;
	}
}

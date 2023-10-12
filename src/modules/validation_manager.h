#pragma once

/// <summary>
/// The validation manager repeatedly plays an AnimationBlinkId
/// instance, until there is BLE connection.
/// It also resumes playing the animation when disconnected.
/// The manager should be initialized only when inValidation()
/// returns true, which is when the least significant bit of
/// the UICR customer[0] register is set to 0 but the second
/// LSB is not set 0.
/// As a consequence, "validation mode" can be turned on once by
/// setting the LSB to 0 and turned back off by setting the 2nd
/// LSB to 0, which is what leaveValidation() does.
/// A BLE central may send an "ExitValidation" to turn off validation
/// mode and go to sleep.
/// </summary>
namespace Modules::ValidationManager
{
    void init();
    void onPixelInitialized();

    void leaveValidation();
    bool inValidation();
}

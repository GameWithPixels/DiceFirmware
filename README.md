# DiceFirmware

Firmware for the Bluetooth Pixel dice, based on Nordic's nRF5 SDK (available
[here](https://www.nordicsemi.com/Products/Development-software/nRF5-SDK/Download#infotabs)).

## Programming a Pixel's electronic board via USB

### Hardware Setup

We use a J-Link debug probe from Segger to connect a computer to a die's electronic board.

![J-Link connected to a die electronic board with a flat ribbon cable](images/board+jlink.jpg "Let the light show begin!")

Plug one end of the flat ribbon cable into the J-Link and the other end into custom adapter.
On the small side of the adapter, pull the black end of the connector away from the board to open it.

![Custom Pixel adapter with a flat ribbon cable connector on one end and a tiny connector for the electronic board on the other end](images/connector.jpg)

Then insert the flexible dice board with the electric lines facing down, and close the connector.

## Programming

[Download](https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools/Download#infotabs)
and install nRF Command Line Tools for Win32 (version 10.12.2 at the time of writing).
During the installation process, you should be prompted to also install the J-Link drivers.

Flashing a die is done in 3 steps: erase, program and reset:

```
nrfjprog -f nrf52 --eraseall
nrfjprog -f nrf52 --program firmware.hex --sectorerase
nrfjprog -f nrf52 --reset
```

Firmware releases are available on the releases
[page](https://github.com/GameWithPixels/DiceFirmware/releases) of this repository.
To program the board, download the .hex file from the latest release,
and run the commands above from the folder where the .hex file was saved
(be sure to change `firmware.hex` in the command to the correct filename).

## Device Firmware Upgrade (DFU)

Nordic provides the tools to update the firmware through Bluetooth.
This is the only way to update our electronic boards once encased in a Pixel die.

You may use Nordic's [nRF Toolbox](https://www.nordicsemi.com/Products/Development-tools/nRF-Toolbox)
app to push a firmware update to a Pixel. Once the app is started, scroll down and tap on
"Device Firmware Upgrade (DFU)" and then "Connect".

![Connect screen listing nearby Bluetooth devices](images/connect_screen.jpg)

This page shows the scanned Bluetooth Low Energy devices. The name of any nearby Pixel should appear.
This name is advertised by the die firmware.
But in order to proceed with a DFU update, we need to connect to the *bootloader*.
The latter is run when the die is turned on and stay active for just 3 seconds.
Then, if no DFU request was made during that time, the die transition to running the firmware.

Turn the dice off and back on and immediately tap on the circling arrow located on the top right corner
of the app to refresh the list of Bluetooth devices.

You should see a new entry named "DiceDfuTarg" which is the name that the *bootloader* is advertising.
Upon selecting it, a list of available packages is displayed. Those are zip files containing the firmware
and some settings.

![Package screen listing available updates](images/package_screen.jpg)

You must first import into the app the package containing the firmware update.
Such packages are available on the releases
[page](https://github.com/GameWithPixels/DiceFirmware/releases) of this repository.
Once that done, select the package in the list and tap on the "Update" button located at the bottom of the screen.

At this point the die will most likely be done running the *bootloader* as it only stays active for 3 seconds.
Turn the dice off and back on again to let the app connect to the *bootloader* and proceed with the update.
Tap on "Retry" if you get a "Device failed to connect" error.

The update should then proceed and the app will let you know when it's done. The die automatically reboots
at the end of the process and runs the updated firmware.

## Building The Firmware

### Environment Setup on Windows

The requirements are the same than for building the dice *bootloader*.
Check out the instructions on the *bootloader*'s GitHub
[page](https://github.com/GameWithPixels/DiceBootloader#readme).

### Building

Make sure that the *Makefile* `SDK_ROOT` variable is pointing to the correct folder.

Open a command line and go the folder where this repository is cloned and run `make`.

The output files are placed in the `_builds` folder, by default those are debug files (not release).
The one that we want to program to the flash memory is the `.hex` file
(more about this format [here](https://en.wikipedia.org/wiki/Intel_HEX)) .

## Programming a Pixel electronic board with *make*

Using the project's *Makefile* you may:

* `reset`: restart the device
* `erase`: entirely erase the flash memory
* `flash_softdevice`: program the *SoftDevice* into the die's memory and reboot the device
* `flash_bootloader`: program the bootloader into the die's memory and reboot the device

**For debug builds:**

* `firmware_debug` (default): produce a debug build of the firmware => `firmware_d.hex`
* `settings_debug`: generate the bootloader settings page for a debug build
* `flash`: program the firmware into the die's memory and reboot the device
* `reflash`:call `erase`, `flash_softdevice` and `flash` in a sequence

*Note:* debug builds being quite bigger than release ones,
we usually don't have enough memory to flash the *bootloader* with them.

**For release builds:**

* `firmware_release`: produce a release build of the firmware => `firmware.hex`
* `settings_release`: generate the bootloader settings page for a release build
* `flash_release`: program the firmware into the die's memory and reboot the device
* `reflash_release`: call `erase`, `flash_softdevice` and `flash_release` in a sequence
* `flash_board`: call `erase`, `flash_softdevice`, `flash_bootloader` and `flash_release` in a sequence
* `publish`: produce a zipped DFU package (also copied in the `binaries` folder)

Some commands requires `nRF Util` to run properly (see
[documentation](https://infocenter.nordicsemi.com/topic/ug_nrfutil/UG/nrfutil/nrfutil_intro.html)
about this tool).

The *Makefile* expects to find `nrfutil.exe` in the current folder or the `PATH`.
Search for `NRFUTIL` to set a different path.
We're using the 6.1.3 build that can be downloaded from
[GitHub](https://github.com/NordicSemiconductor/pc-nrfutil/releases/tag/v6.1.3).

The version of the firmware is defined by the variable `VERSION` in the *Makefile*.

## Output logs in Visual Studio Code

Install Arduino [extension](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.vscode-arduino)
from Microsoft.
It enables access to the serial port to the die's electronic board (through USB).

To connect to the die electronic board, run the following commands in VS Code:
* `Arduino: Select Port` and select SEGGER
* `Arduino: Open Serial Monitor`

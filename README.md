# DiceFirmware

Firmware for the Bluetooth Pixel dice, based on Nordic's nRF5 SDK (available [here](https://www.nordicsemi.com/Products/Development-software/nRF5-SDK/Download#infotabs)).

## Programming a Pixel's electronic board via USB

### Hardware Setup

We use a J-Link debug probe from Segger to connect a computer to a die's electronic board.

![J-Link connected to a die electronic board with a flat ribbon cable](images/board+jlink.jpg "Let the light show begin!")

Plug one end of the flat ribbon cable into the J-Link and the other end into custom adapter. On the small side of the adapter, pull the black end of the connector away from the board to open it.

![Custom Pixel adapter with a flat ribbon cable connector on one end and a tiny connector for the electronic board on the other end](images/connector.jpg)

Then insert the flexible dice board with the electric lines facing down, and close the connector.

## Programming

[Download](https://www.nordicsemi.com/Products/Development-tools/nRF-Command-Line-Tools/Download#infotabs) and install nRF Command Line Tools for Win32 (version 10.12.2 at the time of writing). During the installation process, you should be prompted to also install the J-Link drivers.

Flashing a die is done in 3 steps: erase, program and reset:

```
nrfjprog -f nrf52 --eraseall
nrfjprog -f nrf52 --program firmware.hex --sectorerase
nrfjprog -f nrf52 --reset
```

Firmware releases are available [here](https://github.com/GameWithPixels/DiceFirmware/releases). To program the board, download the .hex file from the latest release, and run the commands above from the folder where the .hex file was saved (be sure to change `firmware.hex` in the command to the correct filename).

## Building The Firmware

### Environment Setup on Windows

The requirements are the same than for building the dice _bootloader_.
Check out the instructions on the _bootloader_'s GitHub [page](https://github.com/GameWithPixels/DiceBootloader#readme).

### Building

Make sure that the _Makefile_ `SDK_ROOT` variable is pointing to the correct folder.

Open a command line and go the folder where this repository is cloned and run `make`.

The output files are placed in the `_builds` folder, by default those are debug files (not release). The one that we want to program to the flash memory is the `.hex` file (more about this format [here](https://en.wikipedia.org/wiki/Intel_HEX)) .

## Programming a Pixel electronic board with _make_

Using the project's _Makefile_ you may:

* `reset`: restart the device
* `erase`: entirely erase the flash memory
* `flash_softdevice`: program the _SoftDevice_ into the die's memory and reboot the device
* `flash_bootloader`: program the bootloader into the die's memory and reboot the device

__For debug builds:__

* `firmware_debug` (default): produce a debug build of the firmware => `firmware_d.hex`
* `settings_debug`: generate the bootloader settings page for a debug build
* `flash`: program the firmware into the die's memory and reboot the device
* `reflash`:call `erase`, `flash_softdevice` and `flash` in a sequence

_Note: debug builds being somewhat bigger then release ones, we usually don't flash the bootloader with with them to save memory._

__For release builds:__

* `firmware_release`: produce a release build of the firmware => `firmware.hex`
* `settings_release`: generate the bootloader settings page for a release build
* `flash_release`: program the firmware into the die's memory and reboot the device
* `reflash_release`: call `erase`, `flash_softdevice` and `flash_release` in a sequence
* `flash_board`: call `erase`, `flash_softdevice`, `flash_bootloader` and `flash_release` in a sequence
* `publish`: produce a zipped DFU package (also copied in the `binaries` folder)

The last command requires `nRF Util` to produce the DFU package (see documentation [here](https://infocenter.nordicsemi.com/topic/ug_nrfutil/UG/nrfutil/nrfutil_intro.html) about this tool). DFU packages can be pushed on dice via Bluetooth using Nordic's nRF Toolbox (available on mobile).

The _Makefile_ expects to find `nrfutil.exe`. Search for `NRFUTIL` to set a different path.
We're using the 5.2 build that can be downloaded from [GitHub](https://github.com/NordicSemiconductor/pc-nrfutil/releases/tag/v5.2.0).

The version of the firmware is defined by the variable `VERSION` in the _Makefile_.

## Output logs in Visual Studio Code

Install Arduino [extension](https://marketplace.visualstudio.com/items?itemName=vsciot-vscode.vscode-arduino) from Microsoft.
It enables access to the serial port to the die's electronic board (through USB).

To connect to the die electronic board, run the following commands in VS Code:
* `Arduino: Select Port` and select SEGGER
* `Arduino: Open Serial Monitor`

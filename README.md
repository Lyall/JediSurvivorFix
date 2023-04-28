# Star Wars Jedi: Survivor Fix
[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/W7W01UAI9)</br>
[![Github All Releases](https://img.shields.io/github/downloads/Lyall/JediSurvivorFix/total.svg)](https://github.com/Lyall/JediSurvivorFix/releases)

This is a fix to remove pillarboxing/letterboxing in Star Wars Jedi: Survivor.

## Features
- Removes pillarboxing/letterboxing cutscenes.

## Installation
- Grab the latest release of JediSurvivorFix from [here.](https://github.com/Lyall/JediSurvivorFix/releases)
- Extract the contents of the release zip in to the the Win64 folder.<br />(e.g. `"<GameDrive>\JediSurvivor"`).

### Linux/Steam Deck
- Make sure you set the Steam launch options to `WINEDLLOVERRIDES="xinput1_3=n,b" %command%`

## Configuration
- See **JediSurvivorFix.ini** to adjust settings for the fix.

## Known Issues
Please report any issues you see.

## Screenshots

| ![ezgif-4-434265f8f6](https://user-images.githubusercontent.com/695941/235080189-7e5b2166-6e9e-4190-9259-d9ec0b04c8df.gif) |
|:--:|
| Disabled pillarboxing/letterboxing in cutscenes. |

## Credits

[RERevHook](https://www.nexusmods.com/residentevilrevelations/mods/26) for the DLL proxy code.<br />
[inipp](https://github.com/mcmtroffaes/inipp) for ini reading. <br />
[Loguru](https://github.com/emilk/loguru) for logging. <br />
[length-disassembler](https://github.com/Nomade040/length-disassembler) for length disassembly.

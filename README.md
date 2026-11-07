# Snowrunner Manual Transmission

This mod disables SnowRunner's automatic shitbox transmission. \
Shifting can either be done with the game's ui shifter and keybinds to shift up/down in the auto position (IMMERSIVE MODE) or completely with keybinds (DISABLE GAME SHIFTING).

## Installation

1. Get the latest release [here.](https://github.com/drafty46/SMT/releases/latest)
2. Extract files from archive into game folder, next to SnowRunner.exe.
3. The mod will load on game launch.

#### Use [this subscribe mod](https://mod.io/g/snowrunner/m/manual-text-for-shifter#description) to change the shifter text from AUTO {gear} to GEAR {gear}
![alt text](image-1.png)

## Usage

The mod's menu, where all configuration is done, is bound to the delete key by default.

![alt text](image.png)

To change a keybind left click on it's blue box, press the desired key/keys and then right click on the box to confirm. \
**NOTE: For axis that return to one end like pedals you need to confirm before releasing them.**    \
To clear an already set keybind right click on it's box. \
Changes take effect immediately but aren't saved unless you use the save button.

### Paddle shift mode

Enable **PADDLE SHIFT MODE**, directly below **DISABLE GAME SHIFTING**, and bind
**GEAR UP** and **GEAR DOWN** to your paddles, keyboard, or supported controller.
The two options are mutually exclusive. Paddle mode wins if both are enabled in
an existing INI file.

Use the game's selector for A, L-, L, L+, H, N, and R, subject to the installed
gearbox's available modes. In A, automatic upshifts and downshifts are disabled;
the paddles step through reverse, neutral, and the truck's existing forward
gears. **SKIP NEUTRAL** still applies. Selecting N or R with the paddles keeps A
logically selected, allowing a paddle press to return to forward drive.

Outside A, paddle presses do nothing, including when **REQUIRE CLUTCH** is on.
**REQUIRE GEAR HELD** is ignored in paddle mode so releasing a paddle retains the
gear. **IMMERSIVE MODE** does not restrict paddle mode to forward gears.
Legacy mod L-/L/L+ bindings are inactive in paddle mode; select these ranges with
the game's shifter so the game initializes their values itself.

Paddle mode calls the game's original A-selection function to initialize the
native coefficient and entry gear, then clears the automatic-shifting flag.
Paddle gear changes call the original gear-selection function without setting
power coefficients. Native coefficient updates pass through unchanged, without
the old setter hook's extra gear-change call. No gearbox definitions or gear
ratios are edited. Changing modes takes effect immediately; use **Save config**
to persist the options.

### Coefficient investigation

The forced `1.05` normal-gear coefficient has been removed. Local commit
`eebc34a` changed `1.01` to `1.05` with the message "Small fix", without giving
a reason. The predecessor used `1.01` when first gear was involved. Neither
source supplies a demonstrated requirement for that override. The original
Ferrster implementation also initializes native A and disables automatic
shifting afterward, showing that this approach predates the override.

[Upstream issue #35](https://github.com/drafty46/SMT/issues/35) reports `1.0`
from the game's normal-gear functions versus SMT's forced `1.05`. This is an
upstream observation, not an in-game measurement made for this change. A
coefficient's exact effect on speed or wheel torque cannot be established from
the mod's source alone.

The older full-manual mode still uses its existing L-/L/L+ coefficient-based
range emulation. That behavior is isolated from paddle mode, which always uses
native range selection and never applies those constants. Removing the legacy
emulation would remove the old mode's distinct low-range bindings.

### Building and checking

Build **Release | x64** with Visual Studio's C++ tools (v145) and the Windows 10
SDK. Include and library paths are relative to the project; bundled OIS and
Detours libraries are used.

Run `tests\run.cmd` on Windows with the Visual Studio C++ tools installed. The
assert-based checks compile the actual configuration and shifting source with
test doubles for the game's entry points. They cover gear boundaries, neutral
and reverse, native range selection, exact coefficient forwarding, mode and
vehicle transitions, and config defaults/conflicts/save/reload. Test files are
written under `x64\tests`.

These checks do not verify SnowRunner's executable signatures, physical torque,
HUD behavior, or hardware input in the running game. Compare against an unmodded
session using the same truck, engine, gearbox, load and route before treating
physical behavior as verified.

**Ranges** - use this if you don't have a physical range toggle. There's LOW, NORMAL and HIGH.  \
Cycling up/down is done with 'RANGE LOW'/'RANGE HIGH' keybinds. (**Must be a single button**)   \
When selected, LOW/HIGH simulate their key being pressed, so bind the same button for the range + the actual button to press.

## Credits

### [Ferrster](https://github.com/Ferrster) for [Snowrunner-Manual-Gearbox-Mod](https://github.com/Ferrster/Snowrunner-Manual-Gearbox-Mod) - reverse engineered game functions and data structures
### [ThirteenAG](https://github.com/ThirteenAG) for [Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) - injector
### [microsoft](https://github.com/microsoft) for [Detours](https://github.com/microsoft/Detours) - library
### [ocornut](https://github.com/ocornut) for [DearImgui](https://github.com/ocornut/imgui) - library
### [Rookfighter](https://github.com/Rookfighter) for [inifile-cpp](https://github.com/Rookfighter/inifile-cpp) - library
### [Rebzzel](https://github.com/Rebzzel) for [kiero](https://github.com/Rebzzel/kiero) - library
### [rdbo](https://github.com/rdbo) for [ImGui-DirectX-11-Kiero-Hook](https://github.com/rdbo/ImGui-DirectX-11-Kiero-Hook) - Imgui+kiero integration
### [wgois](https://github.com/wgois) for [OIS](https://github.com/wgois/OIS) - library
### [Tessil](https://github.com/Tessil) for [ordered-map](https://github.com/Tessil/ordered-map) - library
### [nothings](https://github.com/nothings) for [stb](https://github.com/nothings/stb) - library

# Don't Be Awake with the Fisches

An original Nintendo 3DS survival-horror prototype about Jackstone, an isolated boat, and an ocean that never feels safe.

## Current gameplay

- **60-second preparation phase:** explore the deck and collect supplies before the first night.
- **Survival systems:** hunger, thirst, fatigue, fear, food, water, wood, an emergency beacon, a key and a lantern.
- **Night events:** nine different atmospheric events, including strange knocks, distant lights, heavy fog, a storm and unexplained movement under the boat.
- **Dual-screen layout:** the top screen renders the animated boat scene; the lower screen displays status, supplies and controls.
- **Animation:** moving water, drifting highlights, a flickering lantern and a bobbing player character.
- **Restart:** after a run ends, press START to begin a new 60-second preparation phase.

## Controls

- Circle Pad / D-pad: move around the deck
- A: collect a nearby item
- Y: eat, drink or use a signal flare
- X: rest briefly
- B: show a short deck hint
- START: leave the current run; after an ending, START restarts

## Build

The GitHub Actions workflow installs the devkitPro 3DS toolchain, builds the 3DSX/ELF, creates original icon and banner art, and attempts to package an installable CIA with bannertool and makerom. The workflow uploads the build artifacts when every step succeeds.

## Important

This is an actively developed homebrew project. A successful CI run and a real-device test are required before calling a build finished. Visuals are currently drawn procedurally in C; detailed hand-painted backgrounds, audio, save data and a larger story are future development work.

# Don't Be Awake with the Fisches

An original Nintendo 3DS survival-horror adventure inspired by the project's stated gameplay goals. This repository contains original code and original placeholder/procedural visuals; it does not include proprietary source code or extracted assets from another game.

## Target

- Nintendo 3DS / 3DS XL with custom firmware
- C and devkitARM + libctru + citro2d
- Dual-screen interface: exploration on the top screen, status and inventory on the bottom screen

## Planned gameplay

- Jackstone begins aboard an isolated boat
- Explore, collect useful supplies, and manage hunger, thirst, and fatigue
- Decide when to sleep; nights can pass quietly or trigger unpredictable events
- Investigate strange sounds and ocean creatures
- Inventory, story events, and multiple outcomes

## Build

The GitHub Actions workflow builds a 3DSX homebrew artifact using the official devkitPro devkitARM container. CIA packaging will be added after the initial build is verified and the title metadata/assets are finalized.

## Controls (initial prototype)

- D-pad: move
- A: interact / collect
- B: wait / close a prompt
- X: sleep when near the bed
- START: pause

# Don't Be Awake with the Fisches

A Nintendo 3DS survival-horror adaptation inspired by the gameplay structure of *Don't Sleep With The Fishes*. It uses original code and original placeholder artwork.

## Current rebuild milestone

1. **Evacuation phase:** a 60-second 3DS-friendly countdown starts on the damaged ship.
2. Search the ship's three areas for supplies.
3. Carry one item at a time back to the lifeboat and store it there.
4. Evacuate before the countdown ends, or leave early from the lifeboat area.
5. Choose one of three companions: Frederik, Row, or Laurel.
6. Begin the day-by-day survival section with the supplies that were actually stored.

The 60-second timer and simplified three-area layout are adaptations for this 3DS prototype, not a claim that every detail matches the PC game's exact timing or level layout.

## Current survival prototype

- Four selectable boat areas: outside deck, kitchen, upper deck, and bedroom.
- Hunger, thirst, three separate energy indicators, hull integrity, food, water, fish, and bait.
- Fishing, eating, rain collection, planting, and sleeping.
- Random night encounters with simple choices, including storms that can damage the hull.
- Row can secure the boat during storms and repair hull damage with the salvaged rope.

## Controls

### Ship evacuation
- **Left / Right:** switch between ship areas.
- **A:** pick up an item or store the carried item at the lifeboat.
- **X:** evacuate while at the lifeboat area.
- The countdown also ends the evacuation phase.

### Companion selection
- **Left / Right:** select a companion.
- **A:** confirm.
- **Touchscreen:** select a companion.

### Lifeboat
- **Touchscreen:** select an area.
- **Left / Right:** switch areas.
- **A:** perform the selected area's action.
- **B:** drink stored water.

## Build and testing

GitHub Actions builds the 3DSX and installable CIA with devkitPro. A successful workflow confirms compilation and packaging only. The CIA still needs to be tested on a real Nintendo 3DS.

## Day-loop rebuild (in progress)

- Time advances in two-hour steps when the player performs an action, rather than advancing every few seconds in real time.
- The three energy meters are now separate action reserves: fishing, work around the boat, and talking with the chosen shipmate use different meters.
- Sleeping is blocked while energy remains; it becomes available when all three meters are empty or the day reaches its limit.
- A new day starts after the night event resolves and restores the three energy reserves.
- The X button is used to talk with the selected shipmate.
- Supply locations are randomized for each new run; the bottom screen shows whether an item is still in its area, being carried, or already stored.

These energy categories and time values are a 3DS gameplay adaptation. They are not claimed to reproduce the PC game's exact numerical balance. The official game description confirms the broader loop of spending daytime energy on fishing, tasks and talking, then ending the day and sleeping.

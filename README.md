# Don't Be Awake with the Fisches

A first-person survival adventure for Nintendo 3DS, inspired by the original room-based boat survival loop.

## Game flow

1. Start aboard the large ship and collect food, drinking water, rope and compass.
2. Transfer to the small sailboat. This is your home for the survival journey.
3. Choose between four boat areas on the touchscreen:
   - **Outside deck:** fish for food. Each attempt uses bait, and you can only try once per in-game hour.
   - **Kitchen:** eat stored food or prepare a caught fish.
   - **Upper deck:** collect rainwater when it rains and plant food for later days.
   - **Bedroom:** sleep in the evening. Sleeping advances to morning and can lead to a quiet, bad or good night event.
4. Time advances while you play. Hunger and thirst decrease, so actions and supplies matter.

## Survival systems

- The clock advances one in-game hour every five real-time seconds.
- The sky changes between day and night based on the actual in-game hour.
- Fishing is limited by bait and a one-attempt-per-hour cooldown; catches are not guaranteed.
- Water can be collected during rain, not created freely.
- Planted food takes multiple days to grow.
- Sleeping is only available in the bedroom in the evening and triggers a randomized night event.
- Night events can be calm, harmful (a storm), or helpful (useful driftwood).

## Controls

### Large ship
- **Left / Right:** move between ship areas.
- **Up / Down:** move to the bridge / return to the bow.
- **A:** collect the item in the current area.

### Small sailboat
- **Touchscreen:** select Outside Deck, Kitchen, Upper Deck or Bedroom.
- **Left / Right:** switch between rooms.
- **A:** perform the selected room's action.
- **B:** drink stored water.

## Build and testing

GitHub Actions builds the 3DSX and installable CIA with devkitPro. A successful workflow confirms compilation and packaging only. The CIA still needs to be tested on a real Nintendo 3DS.

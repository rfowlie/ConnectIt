# Content folders

From the owner's 2026-10-20 notes after setting up the first presets: most assets are unique to one adventure level (level
config, match preset, loadouts, conditions, director), but they lived in folders by asset type, which made it hard to see
what a level needs and what's missing. The owner is doing the moves and choosing the naming; this note holds the agreed
direction and the gotchas.

## Direction

- **Per level:** `Levels/<LevelName>/` holds the map and everything only that level uses -- level config, match preset,
  loadouts, condition assets, director Blueprint(s), level-only strategies. At a glance: what the level needs, what's missing.
- **Shared:** `Shared/<Type>/` for anything two levels use -- Classic / versus presets, common goals, generic strategies
  (e.g. place-random), generic directors.
- **Prefixes** (owner's scheme; in use: `CILC_` level config, `CIMP_` match preset, `CIWC_` condition asset, `CIAL_` loadout).

## Moving assets safely

- Move in the Content Browser (never in Explorer while the editor might hold references), then **Fix Up Redirectors** on the
  folders.
- **Since 2026-10-21 a map names its board and default preset in its World Settings** (real asset references, so they follow a
  move). The ConnectIt project settings (and their path strings in `Config/DefaultGame.ini`) are gone, so nothing in config
  names a ConnectIt asset any more. See [level-world-settings](level-world-settings.md).

## Wishlist

- ~~A reusable **board data asset**~~ -- done 2026-10-21: the level config is now only the board, and any number of maps name
  it in their World Settings ([level-world-settings](level-world-settings.md)). A shared board belongs in `Shared/`.

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
- **`Config/DefaultGame.ini` names level configs by path as plain strings** (`DefaultLevelConfig`, `LevelConfigs`); they do
  not follow a move. Redirectors keep the old path working until fixed up, so update the ini when the configs have moved
  (Claude does this on request).
- Soft references in other config (e.g. `LevelCatalog`) are the same.

## Wishlist

- A reusable **board data asset** (grid definition, registries, piece class) so levels with the same board don't repeat
  those values -- see [stage-and-match-preset](stage-and-match-preset.md).

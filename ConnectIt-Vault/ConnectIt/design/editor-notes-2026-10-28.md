# Editor notes, 2026-10-28

The owner's notes from poking around the project, filed with Claude's take on each (checked against the code). Nothing
decided yet unless marked.

## 1. Bug: a new tile Blueprint isn't hovered or placeable by the player (the AI can place there)

**Cause (found):** nothing in C++ raises a tile's hover. `AGridTileBase::OnGridTileBeginCursorOver` is a delegate the
hover subsystem listens to (`UGridHoverSubsystem::RegisterTile`), but **no C++ code broadcasts it** -- the existing tile
Blueprint calls it from its own *Begin Cursor Over* event. A new tile Blueprint that doesn't make that call never
reports a hover, so the player's highlight and click (which go through the hovered tile) never see it. The AI works
from board positions, not hover, so it's unaffected. (A second thing to check on the new Blueprint: its mesh must block
the cursor's trace channel, or *Begin Cursor Over* never fires at all.)

**Fix direction:** broadcast the delegate in C++ (`AGridTileBase::NotifyActorBeginCursorOver`, with a Blueprint hook to
customise or veto it), so every tile class gets hover for free; the same for pieces (`AGridPieceBase` documents the same
Blueprint-only pattern). A small plugin change in `UnrealGridMechanics`.

## 2. World Settings validation

**Owner:** some levels (the hub, menus) don't need match data (catalog, preset, level config); validation warns anyway,
which is annoying; but match data on the map is very convenient for match levels. An in-between based on the game mode?

**Today:** `AConnectIt_WorldSettings::IsDataValid` warns on a missing Level Config, Tile Catalog (only with a Level
Config) and Default Match Preset -- for every map.

**Take:** key it on the map's **game mode**: World Settings already has a game mode override per map. If the map's
game mode (override, else the project default) is the match game mode (`AConnectIt_GameMode` or a subclass), warn about
missing match data; otherwise stay quiet. No new setting, and a hub map with the hub game mode is automatically exempt.
Fallback if that's too implicit: an explicit "Map Kind" (Match / Hub / Menu) on World Settings.

## 3. Map node actors

**Owner:** instead of a base actor, an interface (`GetMapNodeDataAsset()`); specific hover / selection functions in the
hub; reuse the grid setup (a world subsystem tracks hover / click, the player controller listens) -- which would mean a
node base actor; scale up on hover with a transform-lerp component; visual lines between nodes (particles or actors?).

**Take:**

- **A component, not a base actor or a bare interface.** `UConnectIt_MapNodeComponent` (holds the data asset, the
  node's state, hover / select events): artists can add it to *any* actor, and the shared behaviour lives in one place
  -- which an interface alone can't carry. An interface can still sit on top if Blueprint wants to ask "are you a node?".
- **Hover / select:** the grid pattern (a world subsystem relays hover / click, the player controller listens) is the
  right shape -- and item 1 shows the trap: raise hover **in C++** from the component (cursor events on its owner), never
  rely on each Blueprint to broadcast it. A small hub subsystem (or the hub manager from the hub plan's Stage 4) tracks
  hovered / selected. No grid base actor needed: the hub isn't a grid.
- **Scale on hover:** `UnrealGameMechanics` already has `UActorLerpComponent`, but it moves *locations* along waypoint
  batches. Hover wants a small **transform lerp** (start / end transform, time, play forward / reverse on hover / unhover)
  -- a new sibling component in the same `Animation/` area, game-agnostic. (The Fab listing the owner linked is an
  external asset -- worth comparing before building one.)
- **Lines between nodes:** a **spline mesh** actor per unlock link (a spline from node to node with a repeating mesh or a
  ribbon material) -- revealed with its destination node, restylable later (dashed / glowing for secret links). Niagara
  ribbons work too but are harder to place and keep in sync; actors are easier to dress and to toggle with the reveal
  walk. (The owner linked a video tutorial on the effect.)

These change the [adventure hub plan](adventure-hub-plan.md)'s Stage 4 (`AConnectIt_MapNodeActor` → a component).

## 4. Data asset naming

**Owner:** C++ data assets should always end in `DataAsset`.

**Today, not following it:** `UConnectIt_ActionShardTable`, `UConnectIt_TileCatalog`, `UConnectIt_BoardLayout`,
`UConnectIt_LevelCatalog`, `UConnectIt_MatchPreset`, `UConnectIt_MatchConditionAsset` (a primary data asset). Already
following it: `UConnectIt_MapNodeDataAsset`, `UConnectIt_LevelConfigDataAsset`, `UActionLoadoutDataAsset`,
`UItemActionInventoryDataAsset`.

**Take:** a convention for the vault (`_decisions` + the code schema), and a rename pass with **class redirects**
(`DefaultEngine.ini` `+ClassRedirects`) so existing assets keep loading -- one small plan, done at a quiet moment.

## 5. Tile catalog

**Owner:** the tile power struct repeats the gameplay tag (`Name`) that is already the catalog's map key -- a mismatch
risk; enforce it in C++ (making a power makes its tag); and why are all these values set per entry at all?

**Today:** `UConnectIt_TileCatalog::Tiles` is `TMap<FGameplayTag, FConnectItTileCatalogEntry>`; each entry holds a tile
class and a full `FConnectItTilePower` -- `Name` (tag), `Triggers`, `FireAtMultiplier`, `MultiplierFloor`,
`MultiplierCeiling`, `CooldownTurns`, `Effect`.

**Take:**

- **One source for the tag.** Each power's C++ effect struct declares its **native tag** (e.g. `Push` →
  `ConnectIt.TilePower.Push`, defined in C++ so it always exists). The catalog key is then checked against (or filled
  from) the effect's tag, and `Name` is dropped from the entry or set from the key when the catalog loads -- it can't
  disagree.
- **Most values belong to the power type, not the entry.** What a power *is* -- its trigger(s), and usually its gate --
  is part of the C++ effect (defaults there). The entry then only overrides **tunables** (cooldown, gate threshold,
  floors / ceilings where they matter), each shown only if the effect uses it. Fewer fields, fewer ways to set a Push up
  wrong.

## 6. Layout tool: preview vs builder tiles

**Owner:** editing a previewed tile should move the builder tile; flipping between them is annoying. Maybe make the
builder tile itself look like its power.

**Take:** agreed with the owner's second idea -- **make the builder tile WYSIWYG** and retire the preview mode: in the
editor (construction script / an editor-only visual), a builder tile shows its catalog entry's look (the tile class's
mesh, or a clear per-tag visual). One object to move and inspect; nothing to keep in sync.

## Round 2 (owner, 2026-10-28)

- **World Settings validation by game mode: decided.**
- **Components for map nodes: decided.** Owner asks whether one **hover component** could also serve grid tiles and
  pieces. *Claude:* yes, without much breakage -- `UnrealGridMechanics` already depends on `UnrealGameMechanics`, so a
  generic **hover component** there (raises hover / unhover / click in C++ from its owner's cursor events) can be used
  by tiles, pieces and map nodes alike. The grid hover subsystem keeps its API (`RegisterTile`, the hover-changed
  events) and binds to the component instead of each actor's Blueprint-broadcast delegate; the tile / piece Blueprints
  drop their manual broadcast (or it would fire twice). It fixes item 1 for every actor type at once.
- **Data asset rename: decided** -- rename for clarity (with class redirects).
- **Tile catalog keyed by tile class:** owner proposes the **tile Blueprint class as the key** and just an
  `FConnectItTilePower` as the value -- more intuitive for designers, and one class can then only stand for one power
  (today one actor could be used for every power). *Claude:* it works, with one constraint: the board (thread-safe,
  replicated, copied into the AI's search) can't hold a class reference -- so a tile's data would carry its **tile class
  as a soft path** (plain data) instead of the power tag; the rules look the power up by that path, the visuals spawn
  that class. Bonus: two classes can both be "Push" with different looks and tunings (visual variants as entries).
  Builder tiles and layout cells name a **tile class** (picked from the catalog's keys) rather than a tag; the power's
  tag still comes from its C++ effect (item 5).
- **Layout tool:** owner prefers keeping **preview mode, with edits synced**: moving a previewed tile moves / updates its
  builder tile. *Claude:* preview tiles spawned **transient** (never saved with the map), each linked to its builder; an
  editor move on a preview snaps its builder to the same cell; clearing previews before a save as a safety net. So
  saving in preview mode can't leave preview tiles in the map.
- **The push tile reparented to `CI_GridTileBase` still doesn't hover** -- see "Hover: what to check" below.

### Hover: what to check (Claude)

The C++ side looks right for a spawned catalog tile: `UConnectIt_TileRegistry::SwapBuilderTiles` spawns it, the
registry's actor-spawned listener adds it and registers it with the hover subsystem. So the gap is most likely in the
Blueprint or collision:

1. **The child overrides the cursor event.** If the push tile's event graph has its own *Event ActorBeginCursorOver*,
   it replaces the parent's -- and the parent's call to *On Grid Tile Begin Cursor Over* never runs. Remove it, or call
   *Parent: ActorBeginCursorOver* first.
2. **Collision.** Cursor-over only fires when a component of the actor **blocks the cursor trace channel** (the player
   controller's click / mouse-over channel, usually Visibility). A new mesh on the child (or the parent's collider
   hidden, scaled or set to No Collision) stops it. In PIE: `show collision`, or a line trace from the cursor.
3. **Which delegate the parent broadcasts.** `UGridTileComponent` also has an `OnGridTileBeginCursorOver`; the hover
   subsystem listens to the **actor's** delegate. If `CI_GridTileBase` broadcasts the component's, only tiles wired
   some other way would hover.
4. **Registered?** `UGridHoverSubsystem::IsTileRegistered(tile)` (Blueprint-callable) on the push tile in PIE tells
   whether registration or the cursor event is the problem.

Raising hover in C++ (the hover component above) removes 1 and 3 for good.

## Round 3 (owner, 2026-10-28)

- **Hover component: design drawn up** -- [hover-component](hover-component.md).
- **Tile catalog, the big picture (replaces "keyed by tile class" from round 2):** builders hold a **power tag**; the
  tag is looked up in the catalog. What matters: **each tag is automatically tied to the right effect**, and
  `FConnectItTilePower` only **exposes the values relevant to that power** -- no triggers exposed, no multiplier fields
  on Push. *Claude:* make the **effect struct carry the power**: each C++ effect declares its native tag and its
  triggers (fixed in C++), and holds **only its own tunable values** as properties (plus shared ones such as cooldown on
  the base, shown only where the effect uses them). A catalog entry = tile class + that effect; the map key is the
  effect's tag (validated / filled from it), so `Name` and the trigger mask leave the entry.
  - **Matching actors to powers** (one tile class used for several powers): left for later --
    [question](../_questions/tile-catalog-actor-matching.md).
  - **Considered and set aside:** builders referencing a per-tile data asset shaped like a catalog entry -- clear, but
    it drops what the catalog is for (one place to theme / retune a level's tiles).
- **Layout tool -- today the preview does not move the builder.** Checked: `PreviewTileCatalog` spawns each preview
  **transient** and hidden from the outliner, hides its builder, and keeps **no link** between the two; moving a
  preview changes nothing. Saving in preview mode is already safe (transient actors aren't saved). Syncing would be
  new: link each preview to its builder, react to the editor's actor-moved notification, move and snap the builder.
- **The reparented push tile:** registers properly, no Blueprint changes, still no hover -- **benched**; the hover
  component work may shake it loose.
- **New principle: actors refresh themselves from the board** -- recorded in [hover-component](hover-component.md)
  ("Related").

## Round 4 (owner, 2026-10-28)

- **Hover:** in `UnrealGameMechanics`, on tiles only, with a subsystem and a game-specific listener -- recorded in
  [hover-component](hover-component.md).
- **Tile catalog:** as in round 3, **except the trigger mask stays exposed** on the entry for now (useful while
  playtesting).
- **Layout tool:** **add the sync** -- moving a preview tile in the editor moves / snaps its builder tile.

## Round 5 (owner, 2026-10-28): plans

Hover revised (component in `UnrealGridMechanics`, reusing `UGridHoverSubsystem`; player actions are the listeners;
`RefreshFromBoard` on tiles and pieces by default) -- in [hover-component](hover-component.md). **Separate plans**
written -- **all approved by the owner 2026-10-28** (with the hub Stage 4 revision):

- [hover-component-plan](hover-component-plan.md) -- hover component + RefreshFromBoard.
- [world-settings-validation-plan](world-settings-validation-plan.md).
- [data-asset-naming-plan](data-asset-naming-plan.md).
- [tile-catalog-effects-plan](tile-catalog-effects-plan.md).
- [layout-preview-sync-plan](layout-preview-sync-plan.md).
- The adventure hub plan's **Stage 4 revised** (map node component, hover via the grid hover component, transform lerp,
  spline links) -- [adventure-hub-plan](adventure-hub-plan.md); it now `requires` the hover plan.

## Testing Push: reading a power's events (owner question, answered by Claude)

**Owner:** when the board resolves, the queue plays `ConnectIt.Event.TilePowerTriggered` then
`ConnectIt.Event.PieceMoved` -- how do we tell it was Push, and which piece and tiles?

**How it works today (checked in code):** each queued tag carries its event as payload; a listener reads it with the
typed getters in `UConnectIt_BoardEventLibrary`:

- **`GetActiveTilePowerTriggeredEvent`** → `TilePowerName` (e.g. `ConnectIt.TilePower.Push`), `Position` (the Push
  tile), `Faction`, `bNoEffect`, `ReadyTurn` (cooldown), `LevelWideIndex`.
- **`GetActivePieceMovedEvent`** → `From`, `To`, `Faction` (the piece moved, and where).
- Every event also has **`CauseIndex`**: the index, in the same change's event list, of the event that set it off -- the
  Piece Moved's cause is the Tile Power Triggered before it.

**How a visualizer should use it:** route by event, not by power --
- on Tile Power Triggered: find the tile actor at `Position` and let **it** play its "fired" effect (it knows it's a
  Push tile -- the visualizer doesn't need to);
- on Piece Moved: move the piece actor `From` → `To` (the same handler works for Push, Repel, Vacuum or a player's
  Nudge).

**Gap found:** Blueprint can read `CauseIndex` but can't follow it -- there is no way to get the active event's own
index or to fetch the event at an index. Listeners can only rely on play order (Triggered arrives just before its
moves). Two small helpers would close it: `GetActiveBoardEventIndex()` and `GetBoardEventCause()` (the active event's
cause, typed like the other getters) -- e.g. so a Piece Moved can ask "was I moved by a power, and which one?".

## Suggested follow-ups

| Item | Kind |
| --- | --- |
| A shared hover component (tiles, pieces, map nodes) raising hover in C++ (items 1, 3) | Bug -- small plugin plan |
| World Settings validation by game mode (2) | small plan |
| Map node component, hover subsystem, transform lerp, link lines (3) | revision of the adventure hub plan's Stage 4 |
| `DataAsset` naming + redirects (4) | small plan |
| Tile catalog: power = effect struct (native tag, only its own values; trigger mask stays exposed for now) (5) | small plan (tile powers area) |
| Layout tool: sync preview moves to their builder tiles (6, new behaviour) | small C++ (editor) |
| `RefreshFromBoard` on grid actors | with the hover component plan |
| Board event cause helpers (`GetActiveBoardEventIndex`, `GetBoardEventCause`) | small C++ |

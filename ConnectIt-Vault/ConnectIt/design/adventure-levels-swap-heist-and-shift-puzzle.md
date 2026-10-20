# Adventure levels 5 and 3: Swap Heist and Shift Puzzle

Setup checklist (2026-10-08) for the first two [Adventure levels](adventure-mode.md), chosen because each teaches one
action. Everything uses what exists today; the one thing missing is the on-screen teaching text (item T below).
Unverified assumptions are marked **(verify)** -- check them in PIE before building more levels on top.

Conventions used: X runs forward (Up = +X), Y runs right (Right = +Y); factions are slots 0 and 1, and below the
player is "faction 0" and the AI "faction 1" **(verify which slot the human gets and who moves first)**.

## Shared setup (do once, reuse for both)

- [ ] **S1. Copy a working level.** Duplicate `CIL_Sandbox` (or `CIL_NetworkTest`) twice and name the maps. Keep its
  GameMode, HUD, registries and camera.
- [ ] **S2. GameMode is Adventure.** The level's GameMode Blueprint has `MatchType = Adventure`, `NumFactions = 2`, and
  `AIControllerClass` set. Without that class the AI is never spawned (logged error, not a crash).
- [ ] **S3. Tile Blueprint carries designer data.** The tile Blueprint implements `IConnectIt_TileDataProvider`
  (`Get Initial Tile Data`) and exposes instance-editable variables for starting multiplier, active, can-shift and
  starting faction piece (-1 = empty). Every special tile in the maps below is just a per-instance override.
- [ ] **S4. One level config per level.** Duplicate `CI_LevelConfig_Test` twice. On each set `PlayerLoadout`,
  `AIProfile`, `Rules` (Lines, connect length, target score), `PieceActorClass`, pool size (32+), and the instanced
  `TileRegistry` / `PieceRegistry` / `GridDefinition` (copy them from the source asset).
- [ ] **S5. Register each level.** Project Settings > ConnectIt Level Config > `LevelConfigs`: map name -> level config
  asset. The map is empty today, so every level currently falls back to `DefaultLevelConfig` with a warning. Add the
  levels to the `LevelCatalog` if you want them in the main menu.
- [ ] **S6. Auto-end is on.** The player controller's actions component has `bAutoEndTurnOnAllRequiredActionsCompleted`
  enabled, so a satisfied turn-end tree ends the turn without a button. Otherwise the player has to end turns by hand.
- [ ] **S7. Loadouts validate.** Open each loadout and AI profile and fix anything in the Data Validation output (an
  action class in both arrays, a turn-end leaf for an action the loadout doesn't have, no root action on a human
  loadout). The AI profile validates its strategy too.
- [ ] **S8. Action presentation.** Each action Blueprint used has a display name, description and icon so the actions
  menu has something to show, plus the grid-state / hover / first-selection tags its tiles react to.
- [ ] **T. Teaching text (not built).** Nothing in the code shows "swap your piece with theirs" prompts. Decide where
  it lives (HUD widget on level start, or level Blueprint) and write it per level. This is the part that makes the
  levels *teach* rather than just be puzzles.

## Level 5: Swap Heist (teaches Swap)

**Idea:** a dense board of AI pieces; the player's tool is Swap. The first move is a guided win (swap to score), the
rest of the level teaches *defensive* swaps and the cost of a score.

### Rules and numbers
- [ ] Board ~5x5, all tiles active. Rules: Lines, connect length 4, target score 12 (three clean lines; scoring a line
  again later is worth more because multipliers rise).
- [ ] **Player loadout:** root + viewer system actions as in `CI_ActionLoadout_Test`; `PermanentActions`: Place (1 per
  turn); `NumberedActions`: Swap with `StartingMatchUses = 3`, `MaxHeldUses = 3`, `MaxUsesPerTurn = 1`. Turn-end tree:
  Any-group of {Place x1, Swap x1}, so a swap is an alternative to placing, not an extra.
- [ ] **AI profile:** loadout = Place only (copy `CI_ActionLoadout_AIClassic`), turn-end tree Place x1. Strategy
  MinMax, moderate (depth 3, `MistakeChance` ~0.25, `TopMovesConsidered` 3). The AI doesn't model swaps, so it won't
  see swap threats coming, which suits a teaching level. **(verify)** the search is still Place-only.
- [ ] Remember the two Swap rules the player will trip on: it needs one of *their* pieces and one of the *opponent's*
  (both tiles occupied), and **a swap that scores keeps nothing**: the swapped piece is cleared too.

### Map layout (suggested)
- [ ] **Opening puzzle (the guided win):** player pieces on (2,0), (2,1), (2,2); an AI piece on (2,3); one more player
  piece on (0,0). Swap (0,0) with (2,3): the player's piece lands on (2,3), completing a row of four. The AI piece
  lands on (0,0), where it must not complete anything for the AI.
- [ ] **Heist pressure:** scatter ~6 more AI pieces so no AI line of 4 is one move from complete at the start, but a
  couple of three-in-a-rows are *two* moves away. These are what the player learns to break with the remaining swaps.
- [ ] One or two x2 multiplier tiles near the middle so the later scores feel bigger.

### Check in PIE
- [ ] The opening swap scores and the whole row clears, including the swapped piece (the "keeps nothing" rule) and the
  Scored visuals despawn the right pieces.
- [ ] A swap with two of the player's own pieces, or two of the AI's, is rejected (log says "must own exactly one").
- [ ] After three swaps the Swap button is disabled / does nothing; no refills.
- [ ] The AI takes turns, only places, and does not stall.

## Level 3: Shift Puzzle (teaches Shift)

**Idea:** the player's loadout is Shift only. Pieces are almost in a line; one shift of the right line completes it.
A near-random AI places pieces but is no real threat.

### Rules and numbers
- [ ] Board 5x5 or 6x6. Rules: Lines, connect length 4, target score 4 to 8 (one or two lines).
- [ ] **Player loadout:** root + viewer system actions; `PermanentActions`: one Shift action class. Turn-end tree: Shift
  x1. **No Place.** Shift direction and steps are per action *class*: make the Blueprint subclass for the direction
  you teach first (e.g. Down), with `ShiftSteps = 1`. Offer one direction at first; more directions and a `ShiftSteps = 2`
  class are later variants.
- [ ] **AI profile:** Place-only loadout, MinMax set weak: depth 1, `TopMovesConsidered` 50, `MistakeChance` 1.0 (near-random
  placement). There is no "AI skips its turn" setting, so near-random is the closest to passive. **(verify)** it
  feels harmless and that an AI loadout with *no* actions is not an option (it would probably stall).

### Map layout (suggested)
- [ ] **The first puzzle:** player pieces on (2,0), (2,1), (2,2) and a fourth on (3,3), one tile off the row. Select any
  tile in column Y=3 and shift Down: the whole column moves one place toward -X, the piece lands on (2,3), and the row
  of four scores. Keep column Y=3 otherwise empty so the lesson is one piece moving.
- [ ] The AI placing in the way doesn't break it: a shift rotates whole tile data, so an AI piece on the target tile
  just moves along (e.g. to (1,3)).
- [ ] **Optional beats** (same board, or a second map): a piece two places away with a `ShiftSteps = 2` class; a pinned
  tile (`bCanShift = false`) in the column that the piece must be jumped over (steps count shiftable tiles only); a
  second row to build once the first clears.

### Check in PIE
- [ ] Hovering shows the whole column highlighted; selecting it moves tiles *and* pieces to the new places.
- [ ] The shift scores and one piece stays on the board. Which piece stays depends on the order the touched tiles are
  scored in **(verify)** and is fine either way for the lesson.
- [ ] The piece registry stays right afterwards: score a second line and confirm the correct pieces are removed.
- [ ] A turn ends automatically after the one shift; the AI then places and the player's pre-built row is still intact.

## Open / to decide while building
- Who moves first and which slot the human is (it changes which side the guided win suits).
- Whether the teaching prompt is shown once at start or again on the step that needs it (item T).
- Whether either level wants a win beyond a score target (a puzzle-style "solve it in N turns" doesn't exist as a rule).
- Follow-ups after these two: the other three loadout shapes in [adventure-mode](adventure-mode.md) for the 3-loadouts
  playtest checkpoint.

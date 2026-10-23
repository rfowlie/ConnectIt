---
schema: recipes
task: Add a tile power -- a kind of tile that does something when a piece arrives, leaves or changes owner, the tile changes, a line scores through it, or every settle pass (Sync) -- as an effect struct, a tag, and a tile catalog entry with its tile class; place it in a level with builder tiles; make it level-wide on the Level director.
touches:
  - Source/ConnectIt/Public/Board/Powers/ConnectIt_TilePower.h (subclass FConnectItTilePowerEffect)
  - Config/DefaultGameplayTags.ini (a ConnectIt.TilePower.* tag)
  - a UConnectIt_TileCatalog data asset (the entry -- tile class + power) named by the level config or Project Settings
  - a tile Blueprint (the catalog's tile class) and builder tiles placed in the level
  - (level-wide) the Level director's Level Wide Powers
reconciled: 2026-10-22
commit: f6d511d
---

# Recipe: add a tile power

## Goal

A kind of tile with a power, e.g. "when a line scores through it, it pushes a neighbouring piece away". How powers run (the
chain, triggers, once per action) is in [[tile-power-chain]]. Here: making one and putting it in a level.

A power is **defined once, by tag, in a tile catalog**: the trigger, the gate, the cooldown and what it does, with its
values. Placed tiles only name the tag. Decision:
[tile catalog and builder tiles](../../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md).

## Prerequisites

- A tile catalog (`ConnectIt Tile Catalog` data asset) set on the level config (**Tile Catalog**) or as Project Settings >
  Game > ConnectIt > **Default Tile Catalog**, with a **Plain Tile Class**.
- A builder tile Blueprint (parent `ConnectIt_BuilderTile`) with a blockout look, set as **Builder Tile Class** in the same
  settings.

## Steps

1. **The effect (C++)**, if no existing one does what you need: a struct deriving from `FConnectItTilePowerEffect`. Its
   `UPROPERTY`s are the power's tunable values. Override `MakeOperations`:
   ```cpp
   USTRUCT(meta = (DisplayName = "Push A Neighbour"))
   struct FConnectItTilePowerEffect_Push : public FConnectItTilePowerEffect
   {
       GENERATED_BODY()

       virtual void MakeOperations(const FConnectItTilePowerContext& Context,
           TArray<FInstancedStruct>& OutOperations) const override;
       // Context: Board (as it is now), Position (the powered tile), Faction (whose change set
       // it off), Random (the board's seeded stream -- the only randomness allowed)
   };
   ```
   - Add board operation structs (`FInstancedStruct::Make(Operation)`) -- the same operations player actions send.
   - The chain stamps each operation's Faction and applies it only if it `CanApply`. It applies them with `Apply`, never
     `CarryOutOperation`, so returning one that can't happen is fine.
   - Plain data only: it runs in the AI's search on a worker thread (no actors, no UObjects).
2. **The tag:** `ConnectIt.TilePower.<Name>` (Project Settings > Gameplay Tags, under `ConnectIt.TilePower`). No level
   suffix.
3. **The tile class:** a tile Blueprint deriving from `ConnectIt_TileBase` that shows this kind of tile. It reacts to the
   board events, e.g. **Power Triggered** (`Get Active Power Triggered Event`).
4. **The catalog entry:** in the tile catalog, add an entry to **Tiles**: key = the tag; **Tile Class** = the Blueprint;
   **Power**:
   - **Trigger:** Piece Enter / Piece Exit / Piece Changed / Tile Changed / Scoring / Sync.
   - **Min Multiplier:** the gate (0 = none).
   - **Cooldown Turns:** 0 = every time.
   - **Effect:** your effect struct, with its values.
   - Leave **Name** empty (the key names it).
5. **Put it in a level:** place a builder tile and set its **Tile Power Tag**. In the layout tool, **Preview Tile Catalog**
   shows the level as the catalog's tiles (never saved); **Clear** puts the builders back.
6. **Level-wide** (every tile has it, on top of its own): add the tag to the Level director's **Level Wide Powers**. It
   still needs a catalog entry, and cools down across the whole board.

## Verify

- Output Log: `ConnectIt_TileRegistry: N builder tile(s) swapped from tile catalog '<name>'` at match start;
  `InitialiseBoardState -- ... N with a power`.
- The tests: `ConnectIt.Board.TilePowerChain` shows the shape of a test with test-only effects
  (`ConnectIt_MinMaxTestTerms.h`). Test a new effect the same way: rules from `SetTilePowers`, a tag on a tile, carry
  out an operation, check the events and the board.
- In PIE: the power's Power Triggered event plays, then its operations' events (each with Power Triggered as its
  cause).

## Pitfalls

- **Once per action:** a power fires at most once per tile per action; that is what ends chains. A Sync power is the
  exception -- make it change things only when the board calls for it, or settle never quiets (a bug guard stops it at 64
  loops and logs).
- **A tag the catalog doesn't define** gives the tile no power. It is warned at board build and flagged by validation
  (builder tiles, layouts, the Level director).
- **The faction an effect acts for** is the change that set it off: the arriving piece's owner (Enter), the leaving one's
  (Exit), the new owner (Changed), the scoring faction (Scoring), the change's faction (Tile Changed -- -1 for nobody, and
  then an operation needing a faction can't apply). A Sync power acts for the action's faction.
- **The AI's search runs powers too:** an expensive effect slows every node. Run `ConnectIt.AI.MinMax.Throughput` and
  compare.
- **Random only from the context's stream:** anything else differs between the server and the AI's search.
- **No values on placed tiles:** the catalog is the one place a power is tuned.

## See also

- Flow: [[tile-power-chain]]
- Design: [tile-power-triggers](../../design/tile-power-triggers.md), [tile-power-setup](../../design/tile-power-setup.md),
  [tile-powers-plan](../../design/tile-powers-plan.md)
- Decisions: [tile powers in the after-move chain](../../_decisions/2026-10-22-tile-powers-in-the-after-move-chain.md),
  [tile catalog and builder tiles](../../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md)
- Recipes: [[script-a-level]], [[add-a-board-request-type]]

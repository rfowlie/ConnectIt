---
schema: recipes
task: Add a tile power -- a kind of tile that does something when a piece arrives, leaves or changes owner, the tile changes, a line scores through it, a turn starts (a countdown), or every settle pass (Sync) -- as an effect struct, a tag, and a tile catalog entry with its tile class and multiplier limits; animate it through the tile hooks; place it in a level with builder tiles; make it level-wide on the Level director.
touches:
  - Source/ConnectIt/Public/Board/Powers/ConnectIt_TilePowerEffects.h (subclass FConnectItTilePowerEffect; base in ConnectIt_TilePower.h)
  - Config/DefaultGameplayTags.ini (a ConnectIt.TilePower.* tag)
  - a UConnectIt_TileCatalogDataAsset data asset (the entry -- tile class + power) named by the map's World Settings or Project Settings
  - a tile Blueprint (the catalog's tile class, parent ConnectIt_TileBase; its tile hooks) and builder tiles placed in the level
  - (level-wide) the Level director's Level Wide Powers
reconciled: 2026-10-28
commit: 54c1284
---

# Recipe: add a tile power

## Goal

A kind of tile with a power, e.g. "when a line scores through it, it pushes a neighbouring piece away". How powers run (the
chain, triggers, once per action) is in [[tile-power-chain]]. Here: making one and putting it in a level.

A power is **defined once, by tag, in a tile catalog**: its triggers, gate, cooldown, the tile type's multiplier limits
and what it does, with its values. Placed tiles only name the tag. Decisions:
[tile catalog and builder tiles](../../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md),
[the first tile powers](../../_decisions/2026-10-23-the-first-tile-powers.md).

## Prerequisites

- A tile catalog (`ConnectIt Tile Catalog` data asset) set in the map's World Settings (**Tile Catalog**) or as Project Settings >
  Game > ConnectIt > **Default Tile Catalog**, with a **Default Tile Class**.
- A builder tile Blueprint (parent `ConnectIt_BuilderTile`) with a blockout look, set as **Builder Tile Class** in the same
  settings.

## Steps

1. **The effect (C++)**, if no existing one does what you need. The shipped ones are in `ConnectIt_TilePowerEffects.h`:
   **Push**, **Vacuum**, **Repel**, **Chained**. Add yours there: a struct deriving from `FConnectItTilePowerEffect`.
   Its `UPROPERTY`s are the power's tunable values -- only the ones it uses, since they're all a catalog entry shows for
   it. Override `GetPowerTag` (its native tag, step 2: the effect *is* the power's identity) and `MakeOperations`; for a
   multiplier gate, add an `FConnectItMultiplierGate Gate` value and override `PassesGate` to ask it:
   ```cpp
   USTRUCT(BlueprintType, meta = (DisplayName = "Push"))
   struct CONNECTIT_API FConnectItTilePowerEffect_Push : public FConnectItTilePowerEffect
   {
       GENERATED_BODY()

       // The powered tile's multiplier it needs to fire (0 = none)
       UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
       FConnectItMultiplierGate Gate;

       virtual FGameplayTag GetPowerTag() const override;   // { return ConnectIt_TilePower_Push; }
       virtual bool PassesGate(const FConnectItTilePowerContext& Context) const override { return Gate.Passes(Context); }
       virtual void MakeOperations(const FConnectItTilePowerContext& Context,
           TArray<FInstancedStruct>& OutOperations) const override;
       // Context: Board (as it is now), Position (the powered tile), Faction (whose change set
       // it off), Random (the board's seeded stream -- the only randomness allowed), Rules
   };
   ```
   - Add board operation structs (`FInstancedStruct::Make<FConnectItBoardOperation_MovePiece>(Op)` -- name the type,
     or it is sliced to the base) -- the same operations player actions send: **Move Piece**, **Adjust Multiplier**,
     Remove, Capture, ...
   - The chain stamps each operation's Faction and applies it only if the rules allow it (`CanCarryOut`: its
     `CanApply`, and no locked tile unless the operation may -- [[lock-a-tile-or-chain-tiles]]). It applies them with `Apply`, never
     `CarryOutOperation`, so returning one that can't happen is fine.
   - **Picking at random:** build the candidates first, keeping only the operations `Context.Rules->CanCarryOut`
     allows, then roll among them with `Context.Random`. A player who sees one valid option can then predict it, and
     the power acts whenever it can. Push, Vacuum and Repel share helpers for this in `ConnectIt_TilePowerEffects.cpp`.
   - Plain data only: it runs in the AI's search on a worker thread (no actors, no UObjects).
2. **The tag:** a native tag `ConnectIt.TilePower.<Name>` in `ConnectIt_GameplayTags.h/.cpp`
   (`UE_DECLARE_GAMEPLAY_TAG_EXTERN` / `UE_DEFINE_GAMEPLAY_TAG_COMMENT`), returned by the effect's `GetPowerTag`. Native,
   so the tag always exists for the effect that names it. No level suffix.
3. **The tile class:** a tile Blueprint deriving from `ConnectIt_TileBase` that shows this kind of tile. Animate it in
   its **tile hooks** (Blueprint events under ConnectIt | Tile Hooks):
   - **On Power Triggered** (its power fired), **On Piece Enter** / **On Piece Exit** (with the event's tag as Cause:
     placed, swapped, captured, moved, removed), **On Multiplier Changed**, **On Lock Changed**, **On Active Changed**,
     **On Chain Partner Changed**.
   - **On Refreshed From Board** (no reaction): the tile's data on the display board, after any change that touched it
     has played and at match start. Set the tile's resting look from it (multiplier, lock, chain, power) -- not timed
     effects, which belong in the hooks above.
   - **Hover is not the tile's job.** It needs nothing in the graph (the hover component raises it), and the player
     actions decide what a hover means (`HandleValidHover` / `HandleHoverCleared`), possibly calling a function on the
     tile. Don't broadcast *On Grid Tile Begin Cursor Over* (deprecated). Keep a collision that blocks the cursor
     trace (Visibility).
   - **Each hook is passed a Reaction. Call Reaction > Finish when the animation ends** (cache it in a variable if the
     animation is a timeline or a delay). The event queue waits until every reaction of the event has finished, so a
     chain plays in order. A hook you don't override finishes at once.
   - A Turn Start tile can show its countdown with **Get Power Turns Left (State, Position, Current Turn)**, Current
     Turn being the GameState's Active Turn Number.
4. **The catalog entry:** in the tile catalog, add an entry to **Entries**: **Tile Class** = the Blueprint; **Power**
   (the entry's tag is its effect's -- there is no key to set, so they can't disagree; validation reports a tag defined
   twice or an entry with no effect):
   - **Triggers** (one or more): Piece Enter / Piece Exit / Piece Changed / Tile Changed / Scoring / Turn Start, or
     **Sync** on its own.
   - **Cooldown Turns:** 0 = every time. For **Turn Start** it is the countdown: the power fires every N turns (all
     turns count), starting N turns after the tile joins the board.
   - **Multiplier Floor** (default 1) and **Multiplier Ceiling** (-1 = none; 0 = the tile never adds to a score, though
     it still forms lines): this tile type's limits, applied to scoring, Adjust, Destroy and the starting value.
   - **Effect:** your effect struct, with its own values (e.g. Push's **Gate > Fire At Multiplier**, 0 = none).
5. **Put it in a level:** place a builder tile and set its **Tile Power Tag**. In the layout tool, **Preview Tile Catalog**
   shows the level as the catalog's tiles (never saved); **Clear** puts the builders back.
6. **Level-wide** (every tile has it, on top of its own): add the tag to the Level director's **Level Wide Powers**. It
   still needs a catalog entry, and cools down across the whole board.

## Verify

- Output Log: `ConnectIt_TileRegistry: N builder tile(s) swapped from tile catalog '<name>'` at match start;
  `InitialiseBoardState -- ... N with a power`. Catalog validation flags a Sync power with other triggers, and a power
  with no trigger.
- The tests show the shape of a test: `ConnectIt.Board.PushAndVacuum`, `ConnectIt.Board.TurnStartPowers`,
  `ConnectIt.Board.MoveAndMultiplier`, and `ConnectIt.Board.TilePowerChain` with test-only effects
  (`ConnectIt_MinMaxTestTerms.h`). Test a new effect the same way: rules from `SetTilePowers`, a tag on a tile, carry
  out an operation, check the events and the board, and that a replay matches. Check that a single valid option is
  picked for every seed.
- In PIE: the power's Power Triggered event plays, then its operations' events (each with Power Triggered as its
  cause), each waiting for its tiles' hooks to finish. Hook dispatch needs a world: check it in PIE.

## Pitfalls

- **A hook that never calls Finish stalls the event queue** -- every later event waits. There is no timeout. Every
  path through an overridden hook must reach Finish (a branch that skips the animation too).
- **Once per action:** a power fires at most once per tile per action; that is what ends chains. A Sync power is the
  exception -- make it change things only when the board calls for it, or settle never quiets (a bug guard stops it at 64
  loops and logs). A Sync power has no other trigger.
- **Turn Start powers aren't run by the AI's search** (for now): it plans as if the countdown never ends. The board
  only commits a Turn Start change when some Turn Start power is ready.
- **A tag the catalog doesn't define** gives the tile no power. It is warned at board build and flagged by validation
  (builder tiles, layouts, the Level director).
- **The faction an effect acts for** is the change that set it off: the arriving piece's owner (Enter), the leaving one's
  (Exit), the new owner (Changed), the scoring faction (Scoring), the change's faction (Tile Changed -- -1 for nobody, and
  then an operation needing a faction can't apply), whose turn it is (Turn Start). A Sync power acts for the action's
  faction.
- **The AI's search runs powers too:** an expensive effect slows every node. Run `ConnectIt.AI.MinMax.Throughput` and
  compare.
- **Random only from the context's stream:** anything else differs between the server and the AI's search.
- **No values on placed tiles:** the catalog is the one place a power is tuned.

## See also

- Flow: [[tile-power-chain]]
- Design: [tile-power-triggers](../../design/tile-power-triggers.md), [tile-power-setup](../../design/tile-power-setup.md);
  plans: [tile-powers-plan](../../design/tile-powers-plan.md), [first-tile-powers-plan](../../design/first-tile-powers-plan.md)
- Decisions: [tile powers in the after-move chain](../../_decisions/2026-10-22-tile-powers-in-the-after-move-chain.md),
  [tile catalog and builder tiles](../../_decisions/2026-10-22-tile-catalog-and-builder-tiles.md),
  [the first tile powers](../../_decisions/2026-10-23-the-first-tile-powers.md)
- Recipes: [[script-a-level]], [[add-a-board-request-type]], [[lock-a-tile-or-chain-tiles]]

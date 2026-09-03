# Code Repetition & Overlap Catalogue

This file catalogues classes, subsystems, and code blocks across ConnectIt and its plugins that duplicate one another's responsibilities, overlap in confusing ways, or represent an unfinished refactor. It is a triage list, not a bug tracker — entries here are structural/architectural in nature (two things built to do the same job). Straightforward correctness bugs (wrong logic, not overlapping design) are tracked instead in the [Known Issues](README.md#known-issues) section of the top-level index; a couple of entries below cross-link to that section where a duplication candidate also happens to have a confirmed bug attached to it.

**Scope note**: see [Conventions.md](Conventions.md) for the companion document — naming/metadata drift (typos, stale category strings, inconsistent delegate style) lives there instead of here.

Each entry: **What / Where** → **Evidence** → **Hypothesis** → **Recommendation** → **Confidence**.

---

## #1: Legacy game-flow pipeline vs. live networked pipeline

**What / Where**: `Source/ConnectIt/Framework/Game State Machine/*` (`UConnectIt_State_Base` and its four subclasses), `Framework/Data/ConnectIt_GameFacade.h`, `Framework/Data/ConnectIt_GameViewModel.h`, `Framework/Data/ConnectIt_PlayerData.h`, and `Framework/Interface/IConnectIt_FacadeHandlerInterface.h` / `IConnectIt_GameStateHandlerInterface.h` — an entire non-networked state-machine/facade/view-model pipeline for board queries, player data, and turn flow — versus the live pipeline (`AConnectIt_BoardManager`, `UConnectIt_BoardStateComponent`, `UConnectIt_GameUtilityLibrary`, `AConnectIt_GameState`) that now does the same job with correct replication.

**Evidence**: Grepping the entire live pipeline (Board/, Action/, Framework/Controller/, Framework/GameMode/, Framework/GameState/, GameEvent/) for any reference to `UConnectIt_GameFacade`, `UConnectIt_GameViewModel`, `UConnectIt_PlayerData`, or `UConnectIt_State_*` turns up nothing — the legacy set is referenced only by itself. `UConnectIt_GameUtilityLibrary`'s own doc comment states it explicitly *"Replaces UConnectIt_GameFacade for the networked game."* Full historical account of what this pipeline was and how it worked: [LegacyPipeline.md](LegacyPipeline.md).

**Hypothesis**: predates the network-play rewrite. Superseded in place rather than deleted, likely because removal wasn't the priority while the networked pipeline was being built and validated.

**Recommendation**: Delete, once confirmed nothing external (level Blueprints, editor utility widgets, etc. — outside the C++ grep's reach) still references it. This is the single largest dead-weight candidate in the catalogue — an entire subfolder tree, not a class pair.

**Confidence**: High (confirmed via exhaustive grep of the live pipeline).

---

## #2: Two parallel MinMax tree-search implementations

**What / Where**: `Source/ConnectIt/MinMax/ConnectIt_MinMaxManager.h` (`UConnectIt_MinMaxManager` + a namespaced node type `ConnectIt::FMinMaxNode`, fields `Tiles: TMap<FGridPosition,FTileDescriptor>`, `ScoreBoard`, `FactionTurn`, `Children`, `Score`) vs. `Source/ConnectIt/MinMax/ConnectIt_MinMaxTreeBuilder.h` (`UConnectIt_MinMaxTreeBuilder` + a separate global-scope node type `FConnectItMinMaxNode`, near-identical shape: `Tiles: TMap<FGridPosition,FConnectItTileRepresentation>`, `ScoreBoard`, `FactionTurn`, `Children`) — plus a third, fully commented-out attempt, `Source/ConnectIt/MinMax/ConcreteMinMaxExample.h` (`TMinMaxManagerBase`/`TMinMaxManagerBase2` → `UMinMaxManagerBase` → `UConnectIt_MinMax`, using `UnrealGameIntelligence`'s template solvers).

**Evidence**: Both `UConnectIt_MinMaxManager` and `UConnectIt_MinMaxTreeBuilder` are live, compiling code (not commented out) representing the same domain concept — a min-max search node over board tiles + scoreboard + faction turn — with different field/type names, different owning classes, and no shared base or interface between them. `UConnectIt_MinMaxTreeBuilder` is the more complete of the two (async build/solve via `BuildTreeAsync`/`SolveTreeAsync`, thread safety via `FCriticalSection`, returns `TArray<FConnectItMoveOutcome>`); `UConnectIt_MinMaxManager` is more synchronous. Neither is consumed by `AConnectIt_AIController` in `Source/ConnectIt/Framework/Controller/` — its `BeginMakeDecision()` is a `BlueprintImplementableEvent` stub, so no C++ caller of either MinMax class was found in the module. `UnrealGameIntelligence`'s own generic MinMax template toolkit (`MinMaxAlgorithm.h`/`MinMaxABPruning.h`/`MinMaxABMoveOrder.h`) is a third, unrelated implementation of the same underlying algorithm family that neither ConnectIt class builds on.

**Hypothesis**: two unreconciled attempts at ConnectIt's AI search layer, likely written in separate sessions without the second author revisiting/removing the first. `ConcreteMinMaxExample.h` reads as an earlier, now-abandoned attempt to route ConnectIt's AI through the generic plugin templates before pivoting to a project-specific structure.

**Recommendation**: Needs an owner decision — pick one (`UConnectIt_MinMaxTreeBuilder` looks more complete) and either delete the other or document why both are needed. Actively-maintained duplicate AI logic is a real correctness/maintenance risk if the two diverge on scoring rules over time. This is a higher-severity item than most below because both classes are live, not dead code.

**Confidence**: High for the duplication itself; Medium on which one (if either) is actually the intended long-term implementation — that's a design call, not something derivable from the code alone.

---

## #3: `UGridTrackerSubsystem` vs. `UGridHoverSubsystem`

**What / Where**: `Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Subsystem/GridTrackerSubsystem.h` and `GridHoverSubsystem.h` — both `UWorldSubsystem`s that expose hover-state delegates.

**Evidence**: `UGridTrackerSubsystem` tracks all grid tiles/units plus the currently-hovered tile (`OnGridTileHoveredStart`/`OnGridTileHoveredStop`) and contains a fully commented-out "Unit hover" section (`OnGridUnitHoveredStart`/`OnGridUnitHoveredStop`, `GetGridUnitHovered`) — dead code within a subsystem that itself may be dead. `UGridHoverSubsystem` relays `OnGridTileHoverChanged` / `OnGridPieceHoverChanged` for the tiles/pieces the registry components register with it, and is consumed by `AGridCursorManagerBase` and `UTurnBasedAction`. Unlike `UGridTrackerSubsystem`, it does not own or enumerate the tile/piece lists — the registry components do.

**Hypothesis**: `UGridTrackerSubsystem` is an earlier or parallel implementation of the same "which tile is hovered" responsibility that was superseded by `UGridHoverSubsystem` without being removed.

**Recommendation**: Deprecate/remove `UGridTrackerSubsystem` once confirmed nothing (including level Blueprints) binds to it, or if it's kept, delete its dead "Unit hover" block and document why two subsystems with overlapping registries coexist.

**Confidence**: High (only one of the two has a confirmed external consumer).

---

## #4: `UGameMechanicsStateBase` vs. `UGameMechanicsStateSimple`

**What / Where**: `Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/State/GameMechanicsStateBase.h` and `GameMechanicsStateSimple.h` — two differently-patterned "state" base classes in the same folder.

**Evidence**: `UGameMechanicsStateBase` (Abstract) uses an `Enter()`/`Exit()` public-wrapper-calls-protected-`_Internal`-BlueprintNativeEvent pattern with `OnGameStateEnter`/`OnGameStateExit` delegates. `UGameMechanicsStateSimple` (not Abstract) instead makes `Enter()`/`Exit()` themselves direct `BlueprintNativeEvent`s with no wrapper indirection, and a single `OnStateComplete` delegate. ConnectIt's legacy state machine (`UConnectIt_State_Base`, see [#1](#1-legacy-game-flow-pipeline-vs-live-networked-pipeline) / [LegacyPipeline.md](LegacyPipeline.md)) derives from `UGameMechanicsStateSimple`, not the more elaborate `UGameMechanicsStateBase` — so the plugin's own two state-base classes are inconsistently the "chosen" one depending on which consumer you look at.

**Hypothesis**: `UGameMechanicsStateSimple` likely came first (simpler API), and `UGameMechanicsStateBase` was a later, more structured redesign that never replaced it because its only real consumer (ConnectIt's legacy state machine) was itself being phased out around the same time.

**Recommendation**: Pick one pattern as the plugin's canonical state-machine base and either fold the other's useful bits in or deprecate it. Lower urgency than #2/#3 since the only current consumer of either is the legacy pipeline slated for removal — but worth resolving before any future project reaches for "generic state machine" in this plugin and has to guess which base to use.

**Confidence**: High (both classes read directly, consumer confirmed via grep).

---

## #5: `IGridTileSelector` deprecating `UGridTileSelectorComponent`

**What / Where**: `Plugins/UnrealGridMechanics/Source/UnrealGridMechanics/Public/Tile/Selector/GridTileSelector.h` (interface) and `GridTileSelectorComponent.h` (component).

**Evidence**: `IGridTileSelector` offers a richer, dual-pattern API (Blueprint-friendly dynamic delegate register/unregister *and* C++-friendly direct multicast binding via `BindOnGridTileSelected`/`GetOnGridTileSelectedDelegate`), and its own `GetGridTileSelectorComponent()` accessor is explicitly marked `[Deprecated]` (`DeprecatedFunction` meta) pointing at `UGridTileSelectorComponent` — a much simpler component exposing just one dynamic multicast delegate.

**Hypothesis**: this is a self-documented, already-decided deprecation — not an ambiguous case like #2–#4. The interface-based pattern is the intended direction; the component is being phased out.

**Recommendation**: Lowest-ambiguity item in this catalogue — just finish the deprecation (remove `UGridTileSelectorComponent` and its `Deprecated`-tagged accessor once no consumer references either).

**Confidence**: High (self-documented in the source itself).

---

## #6: `AConnectIt_GridPiece`'s dead replication setup

**What / Where**: `Source/ConnectIt/Grid/ConnectIt_GridPiece.h/.cpp` — replicates `FactionID` (`ReplicatedUsing=OnRep_FactionID`) and `OccupiedPosition` (`Replicated`), with `bReplicates = true` set in its constructor.

**Evidence**: the tag-reactive interpreter that used to be the only class spawning `AConnectIt_GridPiece` actors (and explicitly guarded spawning with `if (GetOwner()->HasAuthority()) return;` -- i.e. the server never spawned one) has been removed project-wide, and its replacement (`AConnectIt_BoardManager`'s `CreateGameEventsFromBoardUpdate`/`ExecuteGameEvents`) doesn't spawn pieces yet either -- see [ConnectItModule.md](ConnectItModule.md#grid). This entry's original evidence (server never spawns one) needs re-confirming once that replacement pipeline is functional; until then, treat the client-only-spawning claim as historical rather than currently verified.

**Hypothesis**: not a duplicate class in the traditional sense — this is a network-model mismatch left over from when pieces may have been intended to be server-spawned/replicated actors, before the design moved to "client-local visual reaction to replicated board state" (see [SingleSourceOfTruth-Replication.md](Workflows/SingleSourceOfTruth-Replication.md)).

**Recommendation**: Either remove the now-pointless `Replicated`/`bReplicates` setup from `AConnectIt_GridPiece` to avoid misleading a future reader into assuming pieces are server-authoritative actors, or, if there's a future reason to spawn pieces server-side, document why the replication scaffolding is being kept ahead of that need. Re-confirm which side (if either) spawns this actor once the replacement spawn pipeline is functional, before acting on this entry.

**Confidence**: Medium (the original client-only-spawning evidence came from a class that's since been removed; not yet re-verified against its replacement).

---

## #7: Dead / commented-out code inventory

**What / Where**: several standalone blocks of commented-out code kept "just in case," bundled here as one entry since they share the same recommendation:
- `MinMaxAlgorithm.h` (`UnrealGameIntelligence`) — ~50 lines, a commented-out `TMinMaxAlgorithm` struct.
- `MinMaxUtility.h` (`UnrealGameIntelligence`) — ~135 lines, a commented-out `TMinMaxManagerBase` class; the same file's *live* `TMinMaxManager<TNode>` abstract template also has no concrete subclass anywhere in the plugin (orphaned scaffolding, not literally commented out, but effectively unused).
- `GridTrackerSubsystem.h` (`UnrealGridMechanics`) — commented-out "Unit hover" section (see [#3](#3-ugridtrackersubsystem-vs-ugridworldsubsystem)).
- `GridTileSelector.h` (`UnrealGridMechanics`) — a commented-out "Lifecycle" block.

**Evidence**: direct inspection of each file; none of these blocks are behind `#if 0`/compile guards with a stated reason, they're plain `//`-commented code.

**Hypothesis**: exploratory/abandoned-design leftovers kept out of habit rather than deliberate historical preservation.

**Recommendation**: per-block decision (delete, or restore and finish if still wanted) rather than a blanket rule — but all four should get that decision made rather than accumulating further. Lowest priority in this catalogue: none of these affect runtime behavior, they're purely a readability/maintenance cost for anyone trying to understand what's canonical in these files.

**Confidence**: High that the blocks are dead; Low/Medium on intent (only the original author can say whether any of these were meant to come back).

---

## Additional minor overlaps

Two smaller items surfaced while writing the per-plugin docs, noted here for completeness since those docs point back to this file for them:

- **`AGridTileBase` vs. `UGridTileComponent`** (`UnrealGridMechanics`) — both independently declare their own `OnGridTileBeginCursorOver`-style delegate for the same "cursor is over this tile" concept, one at the actor level (`AGridTileBase`) and one at the component level (`UGridTileComponent`, whose own header comment states "anything that is a grid tile will be required to have this component"). Not severe enough for its own ranked entry, but worth consolidating onto one of the two rather than both, the next time either is touched. Confidence: Medium (both declarations confirmed; whether both are actually bound-to in practice wasn't traced).
- **Redundant Public+Private module-dependency listings** — `UnrealTurnBasedMechanics.Build.cs` lists `UnrealGridMechanics` in both `PublicDependencyModuleNames` and `PrivateDependencyModuleNames` (harmless, since Public already implies the dependency is available, but redundant and worth trimming). Confidence: High (directly confirmed in the `.Build.cs`).

## Not included: the two board-manager-discovery mechanisms

`UConnectIt_GameUtilityLibrary::GetBoardManager()` (logs an error on miss, no fallback) and `UConnectIt_BoardManagerSubsystem::GetBoardManager()` (silent, pairs with an `OnBoardManagerReady` delegate) look at a glance like two ways to do the same thing, but this is a **deliberate two-mode API**, not accidental duplication — confirmed intentional per the design rationale captured in `Source/ConnectIt/Workflows/BoardManagerSubsystem_Workflow.txt`. It's documented in full, including when to use which, in [SubsystemDiscovery-DualAccessPattern.md](Workflows/SubsystemDiscovery-DualAccessPattern.md) rather than treated as a defect here.

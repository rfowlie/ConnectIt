# ConnectIt Documentation

ConnectIt is an Unreal Engine 5 "Connect Four"-style turn-based game built on a stack of generic, project-agnostic plugins. This is the entry point for its external documentation: what each plugin is for, what every class does, which patterns are worth reusing in other projects, the naming conventions in play, and the known duplication and correctness issues worth triaging.

**Start here if you're new to the project**, then follow the links below for whichever area you need.

## Architecture Map

ConnectIt (`Source/ConnectIt/`) is the game module — it doesn't implement grid math, turn logic, or event sequencing itself; it consumes four first-party plugins for that and adds the actual game rules (piece placement, scoring, win conditions) and networking on top:

```
Source/ConnectIt  (game module)
  ├─ depends on → UnrealGridMechanics        (2D grid/board framework)
  ├─ depends on → UnrealTurnBasedMechanics    (turn/action/participant framework)
  │                  └─ depends on → UnrealGridMechanics
  │                  └─ depends on → UnrealGameMechanics
  ├─ depends on → UnrealGameMechanics         (event sequencing, pooling, turn primitives)
  ├─ depends on → UnrealGameIntelligence      (MinMax toolkit, influence maps)
  └─ depends on → UnrealCodingUtils           (small grab-bag utilities; Private only)

  Not yet integrated:
  UnrealAIMechanics        (utility-AI plugin — exists, not referenced by ConnectIt.Build.cs)
  AdvancedSessions-5-5     (vendored third-party — exists, not referenced by ConnectIt.Build.cs;
                             OnlineSubsystem dependency is commented out, pending online-session work)
```

(Source: `ConnectIt.Build.cs` and each plugin's own `.uplugin`/`.Build.cs`.)

## Plugin Documentation

Each plugin's purpose, scope boundaries, and full class catalogue:

| Plugin | Purpose (one line) |
|---|---|
| [UnrealGridMechanics](../../../Plugins/UnrealGridMechanics/Docs/README.md) | The 2D grid/board framework — tiles, pieces, board-state replication, shift system, grid math. The foundation ConnectIt's board is built on. |
| [UnrealTurnBasedMechanics](../../../Plugins/UnrealTurnBasedMechanics/Docs/README.md) | Network-replicated turn-based game framework — action stack, match-phase state machine, turn-order strategies, controller mediation. The most mature and best-documented plugin in the project. |
| [UnrealGameMechanics](../../../Plugins/UnrealGameMechanics/Docs/README.md) | Generic gameplay scaffolding — gated/sequenced event tasks (the backbone of ConnectIt's visual sequencing), actor pooling, turn bookkeeping, a state-machine base. |
| [UnrealGameIntelligence](../../../Plugins/UnrealGameIntelligence/Docs/README.md) | AI/decision-support toolkit — a MinMax/alpha-beta solver template family and an influence-map debug-visualization subsystem. |
| [UnrealCodingUtils](../../../Plugins/UnrealCodingUtils/Docs/README.md) | Small, dependency-free grab-bag of general C++ helpers. Currently one function. |
| [UnrealAIMechanics](../../../Plugins/UnrealAIMechanics/Docs/README.md) | Generic weighted-scoring/utility-AI action picker. Not currently wired into ConnectIt. |
| [AdvancedSessions (Third-Party)](../../../Plugins/AdvancedSessions-5-5/Docs/README.md) | Vendored community plugin for online session/friends/identity management. Vendored in preparation for online play; not yet integrated. |

## ConnectIt Module

- [ConnectItModule.md](ConnectItModule.md) — class catalogue of the live, networked game module itself (Board/, Action/, Framework/, Grid/, GameEvent/, Library/, GameIntelligence/, MinMax/).
- [LegacyPipeline.md](LegacyPipeline.md) — full historical write-up of the superseded, non-networked Game State Machine / Facade / ViewModel / PlayerData pipeline. Not used by any live code; kept here for context rather than deleted from the record.
- [RuntimeStateAccess.md](RuntimeStateAccess.md) — reference for reading live board/turn/participant/action state (e.g. for debug or in-game UI): which accessor to use, whether it's Blueprint-safe, and where the gaps are. Includes the current turn-end/resolution sequencing design (`TurnEndEventTag` + `QueueTagContainer`), which replaced the earlier `ResolutionHoldCount`/`BoardSequencerComponent` mechanism.

## Reusable Workflows

Five design patterns from ConnectIt's networked pipeline, written up as portable, engine/project-agnostic patterns rather than ConnectIt-specific how-tos — see [Workflows/README.md](Workflows/README.md) for the index. Useful as a starting point for any other turn-based or event-sequenced multiplayer project:

- [Server-Authoritative Action Request](Workflows/ServerAuthoritative-ActionRequest.md)
- [Single-Source-of-Truth Replication](Workflows/SingleSourceOfTruth-Replication.md)
- [Gated Event Sequencing via Tags](Workflows/GameplayTag-EventSequencing.md) (turn-end is now just another instance of this pattern)
- [Dual-Mode Subsystem Discovery](Workflows/SubsystemDiscovery-DualAccessPattern.md)
- [One Debug Widget Per Tracked Class](Workflows/DebugWidgets.md) — also the class↔widget↔plugin location reference for the `DWidget_*` family

## Conventions & Quality

- [Conventions.md](Conventions.md) — naming and structural conventions in use, plus a **Discrepancies** section flagging where the codebase violates its own conventions (a stale "Chimera" naming residue from a predecessor project, a `BlueprintReadWrite` casing bug, inconsistent delegate declaration style, a filename inconsistency, and inconsistent `Category`/`ClassGroup` strings).
- [Duplication.md](Duplication.md) — seven ranked entries cataloguing classes/subsystems that overlap or duplicate each other's responsibilities, from the entire legacy pipeline down to a couple of small commented-out code blocks — each with evidence, a hypothesis for why it happened, and a recommendation.

## Known Issues

Two confirmed correctness bugs, surfaced here rather than only inside `Duplication.md`, because they're defects, not structural overlap, and are worth a new contributor's attention immediately:

- **`AConnectIt_PlayerController::ServerRouteBoardChangeRequest_Implementation`** (`Source/ConnectIt/Private/Framework/Controller/AConnectIt_PlayerController.cpp:92`) — only stamps the server-authoritative `FactionID` onto an incoming request when the client-sent value is negative/unset. Every current action already sends a valid `FactionID` itself, so in practice the server currently trusts whatever `FactionID` a client sends as long as it's `>= 0`, checking only that it's *that participant's turn* — not that the `FactionID` actually belongs to them. A modified client could submit a different faction's ID and have it accepted. Likely fix: stamp `FactionID` from the server-side participant unconditionally, not only when unset. See [ServerAuthoritative-ActionRequest.md](Workflows/ServerAuthoritative-ActionRequest.md#gotchas).
- **`PieceControlInfluenceMap::Update()`** (`Source/ConnectIt/Private/GameIntelligence/InfluenceMap/PieceControlInfluenceMap.cpp:26`) — scores each neighboring tile by comparing its `FactionPiece` against `Snapshot.FactionTurn`. `FConnectItBoardState::FactionTurn` defaults to `-1` and is only ever set inside MinMax/AI hypothetical-state code, never on the real live board — so on the actual board it's permanently `-1`, which is also the sentinel value for an *empty* tile. The practical effect: every empty neighboring tile currently scores as if it belonged to the active faction, and every occupied tile (including the active faction's own pieces) scores as opposing. The influence map is not measuring what it's meant to. Needs `Snapshot.FactionTurn` actually populated from the real active participant/faction before this comparison is meaningful.

These bullets are the record of the bugs as documented here; there's no separate issue tracker integration for this project at present.

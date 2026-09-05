# Naming & Structural Conventions

This document describes the naming and structural conventions actually in use across the ConnectIt project and its plugins, and calls out the places where the codebase currently violates its own conventions. It exists so that new code — whether a new plugin class, a new ConnectIt gameplay class, or a new gameplay tag — lands consistent with what's already here, and so the known inconsistencies have one place to be tracked instead of being rediscovered repeatedly.

**Scope note**: this document covers naming and metadata conventions (prefixes, function-name patterns, category strings, delegate declaration style). Structural/architectural overlap — two classes or subsystems doing the same job — is a different kind of problem and lives in [Duplication.md](Duplication.md) instead. If an entry could plausibly go in either file, the rule is: **naming/metadata drift → here; two things doing the same job → Duplication.md.**

## Type Prefixes

Standard Unreal Engine type prefixes are followed correctly and consistently across every plugin and the ConnectIt module:

| Prefix | Meaning | Example |
|---|---|---|
| `A` | Actor | `AConnectIt_PlayerController`, `AGridTileBase` |
| `U` | UObject / Component / interface *implementation* | `UConnectIt_BoardStateComponent`, `UTurnBasedAction` |
| `I` | Interface (always paired with a `U`-prefixed `UINTERFACE`) | `IConnectIt_ScoringRule`, `ITurnOrderInterface` |
| `F` | Struct | `FGridPosition`, `FConnectItBoardState` |
| `E` | Enum | `EMatchPhase`, `EMatchEndReason` |

On top of the engine convention, ConnectIt's own module prefixes nearly every class with `ConnectIt_` (e.g. `AConnectIt_PlayerController`, `UConnectIt_BoardStateComponent`, `IConnectIt_ScoringRule`), and each first-party plugin prefixes its classes with an abbreviation of its own name (`Grid*`/`AGridTileBase`, `TurnBased*`, `GameEvent*`/`GameTurn*`, `AI_Utility*`, `GI_*`).

**Documented exceptions** (see [Discrepancies](#discrepancies) #1 for one of these being more than a naming quirk):
- `UPieceControlInfluenceMap` / `UPieceControlMapVisualizer` (`Source/ConnectIt/GameIntelligence/InfluenceMap/`) — carry only the standard `U` type prefix, no `ConnectIt_` project prefix. The only two live ConnectIt classes without it.
- `ConcreteMinMaxExample` (`Source/ConnectIt/MinMax/`) — also unprefixed, but the entire file is commented out (dead reference/example code), so this is lower priority than the two classes above.

## Method / Property Conventions

| Pattern | Meaning | Example |
|---|---|---|
| `OnRep_<Property>` | Replication callback (`ReplicatedUsing=`) | `OnRep_BoardSnapshot`, `OnRep_FactionID`, `OnRep_MatchResult`, `OnRep_MatchPhase` |
| `Server<Verb>` | Server RPC | `ServerRouteBoardChangeRequest`, `ServerNotifyReady`, `ServerSubmitTurnEnd` |
| `Handle<Event>` | A function bound to a delegate, reacting to something that already happened | `HandleBoardStateChanged`, `HandleTurnEndTaskExecute`, `HandleGameOver` |
| `<Verb>_Implementation` | The native override point for a `BlueprintNativeEvent` | `Activate_Internal_Implementation`, `OnBoardStateChanged_Implementation` |
| `Get<Thing>` / `Is<Condition>` | `BlueprintPure`, `const` query | `GetCurrentState()`, `IsLocalPlayerTurn()` |

**Notable absence, worth preserving as a convention going forward**: no `Client<Verb>` or `Multicast<Verb>` RPCs exist anywhere in `Source/ConnectIt` today. Every "tell everyone" need is currently met by the single-authoritative-replicated-property pattern (see [SingleSourceOfTruth-Replication.md](Workflows/SingleSourceOfTruth-Replication.md)) rather than explicit multicast RPCs. If a future feature seems to need a `Client_`/`Multicast_` RPC, it's worth first checking whether it can instead be expressed as a replicated property change — that's the pattern the rest of the codebase already uses, and mixing both approaches for the same kind of event is exactly what caused the bug described in [GameplayTag-EventSequencing.md](Workflows/GameplayTag-EventSequencing.md)'s Problem section.

## GameplayTag Conventions

Gameplay tags are dot-hierarchical strings (`ConnectIt.Game.PlacePiece`, `ConnectIt.Tile.ValidHover`, `ConnectIt.Event.LineScored`), declared natively via `UE_DECLARE_GAMEPLAY_TAG_EXTERN` in `Source/ConnectIt/Public/ConnectIt_GameplayTags.h`. The C++ identifier for a tag replaces every `.` with `_`:

| Tag string | C++ identifier |
|---|---|
| `ConnectIt.Game.PlacePiece` | `ConnectIt_Game_PlacePiece` |
| `ConnectIt.Tile.ValidHover` | `ConnectIt_Tile_ValidHover` |
| `ConnectIt.Event.LineScored` | `ConnectIt_Event_LineScored` |

Three tag families are in use: `ConnectIt.Game.*` (match/turn phases and request-type routing — e.g. what `UConnectIt_BoardRequestMediator::ProcessRequest` dispatches on), `ConnectIt.Tile.*` (tile visual-feedback state), and `ConnectIt.Event.*` (board event triggers consumed by `UGameEventTaskSubsystem` — see [GameplayTag-EventSequencing.md](Workflows/GameplayTag-EventSequencing.md)).

## Delegate Declaration Conventions

`UnrealTurnBasedMechanics` consistently declares **every** delegate twice: once as a `BlueprintAssignable` dynamic multicast delegate (for Blueprint binding), and once more as a plain (non-dynamic) multicast delegate with a `_Native` suffix (for C++ binding — cheaper, supports lambdas/`TFunction`, no Blueprint marshalling overhead). For example `Public/TurnBasedMechanicsDelegates.h` declares both `FOnTurnNotification`/`FOnTurnNotification_Native` and `FOnOpponentTurnStarted`/`FOnOpponentTurnStarted_Native` side by side, with an explicit comment on why: *"DYNAMIC delegates cannot be declared like this, they do not compile properly"* for the native-only forms.

This is the most disciplined delegate convention in the codebase, but it is **not** followed elsewhere — `UnrealGridMechanics`, `UnrealGameMechanics`, and `UnrealGameIntelligence` all declare dynamic-only delegates. Whether the `_Native`-sibling pattern is meant to become the house style or was a deliberate one-off for `UnrealTurnBasedMechanics`'s performance-sensitive turn/participant events is listed as [Discrepancy #3](#discrepancies) below rather than assumed either way.

---

## Discrepancies

| # | Discrepancy | Location | Description | Suspected Cause |
|---|---|---|---|---|
| 1 | "Chimera" naming residue | `UnrealCodingUtils::IsAuthoritative` — `Category = "Chimera|Utils"` ([`CodingUtilsComponentLibrary.h:21`](../../../Plugins/UnrealCodingUtils/Source/UnrealCodingUtils/Public/Library/CodingUtilsComponentLibrary.h)) | A metadata string references "Chimera," a project name that predates "ConnectIt." Not user-facing (Blueprint editor category only), but a stale identifier from before the plugin suite was generalized/renamed. (A second example, `UnrealGridMechanics::UBoardStateInterpreter`'s `ClassGroup=(Chimera)`, no longer applies — that class has been removed.) | Copy-paste carried over from an earlier codebase (likely an earlier, differently-named project this plugin suite originated in) during extraction into standalone, project-agnostic plugins; the rename sweep missed this spot. |
| 2 | Casing bug | `UnrealGameMechanics::UGameTurnHandler` — [`GameTurnHandler.h:83`](../../../Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/GameTurn/GameTurnHandler.h) | `UPROPERTY(EditAnywhere, BlueprintreadWrite, Category = "Game Mechanics \| Turn")` — `BlueprintreadWrite` should be `BlueprintReadWrite`. UHT specifier matching is case-sensitive, so this is not just cosmetic: depending on UHT version this either fails to compile cleanly or silently doesn't grant the intended Blueprint read/write access. | Typo — a single dropped capital letter, most likely from manual typing rather than autocomplete/copy-paste (the rest of the specifier list is correctly cased). |
| 3 | Delegate declaration inconsistency | `UnrealTurnBasedMechanics` vs. `UnrealGridMechanics` / `UnrealGameMechanics` / `UnrealGameIntelligence` | Only `UnrealTurnBasedMechanics` pairs every dynamic multicast delegate with a native (`_Native`) sibling (see [above](#delegate-declaration-conventions)). The other three plugins declare dynamic-only delegates throughout. | Unclear whether this is meant to be the project-wide standard (with the other plugins simply not yet caught up) or a deliberate one-off optimization for `UnrealTurnBasedMechanics`'s especially hot turn/participant delegate paths. Needs an owner decision before being either standardized or left as an intentional exception. |
| 4 | Filename / type-prefix inconsistency | `Source/ConnectIt/Public/Framework/Controller/ConnectIt_PlayerController.h` vs. `Source/ConnectIt/Private/Framework/Controller/AConnectIt_PlayerController.cpp` | The header omits the `A` actor-type prefix from its filename (matching every other class file in the project); the implementation file includes it. This is the only header/cpp filename pair in the project that disagree on whether the type prefix is part of the filename. | Most likely an artifact of a rename — the class itself is correctly named `AConnectIt_PlayerController` in code either way, so this is a filename-only slip, probably from renaming one file and not the other during a refactor. |
| 5 | `Category` / `ClassGroup` string inconsistency | Spread across all plugins — e.g. `UGridMechanics_GridLibrary` uses `"Grid Mechanics \| Library \| ..."`-style categories, `UGridTileRegistryComponent` uses `"Grid\|Registry"` (no spaces around `\|`), `UGameTurnHandler` uses `"Game Mechanics \| Turn"` while `UGameMechanics_Framework` uses `"GameMechanics\|Framework"` (no space, different capitalization of the plugin name itself); `ClassGroup` values seen include `(Grid)`, `(Custom)`, `(Chimera)` (see #1), and `(TurnBased)` | No single canonical `Category`/`ClassGroup` prefix exists per plugin — spacing around `\|`, presence/absence of spaces in multi-word segments, and capitalization of the plugin name all vary class-to-class within the same plugin, which makes the Blueprint "Add Component" menu and function-library category tree inconsistent to browse. | Organic accretion — each class was categorized independently as it was written, with no style guide to converge on. Lowest-severity entry here (cosmetic, editor-browsing-only impact) but worth a mechanical cleanup pass once a canonical per-plugin category prefix is agreed on. |

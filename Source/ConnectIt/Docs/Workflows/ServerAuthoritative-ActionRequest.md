# Pattern: Server-Authoritative Action Request

## Problem

A client needs to trigger a gameplay mutation (place a piece, shift a row) that only the server is allowed to actually apply — but the code that *decides* to make the request (input handling, UI, targeting/validation feedback) naturally lives on the client, close to the player. Routing every possible mutation through its own hand-written RPC, each with its own parameter list and its own copy of "is it actually this player's turn," multiplies both the RPC surface area and the places a validation check can be forgotten.

## Pattern

Collapse every gameplay mutation into one **data-only request struct** with a type tag and a generic payload, one **single RPC** that carries that struct to the server, and one **dispatch point** on the server that routes by tag to the actual handler. The client-side code that decides *what* to request never talks to the network layer directly — it hands a fully-formed request to a routing layer that owns the one RPC.

```
Client-side "intent" code            Client-side routing         Server
──────────────────────────           ──────────────────         ──────
Action/UI builds a                → PlayerController      →      RPC        →   Authoritative
request struct                      .ServerRouteRequest()        boundary        handler,
(type tag + payload)                                                             dispatched
                                                                                  by type tag
```

## Participants / Classes Involved

| Role | ConnectIt class | Note |
|---|---|---|
| Request struct | `FTurnActionRequest` (`UnrealTurnBasedMechanics`) | `RequestType` (`FGameplayTag`) + `FactionID` + `FInstancedStruct Payload` — the payload is a generic extension point, not hardcoded per request type. |
| Concrete payloads | `FConnectItRequestPlacePiece`, `FConnectItRequestBoardShift` (see [ConnectItModule.md](../ConnectItModule.md)) | Unwrapped from `Payload` by tag at the dispatch point. |
| Intent-side classes | `UConnectIt_PlacePieceAction`, `UConnectIt_ShiftAction` (`UTurnBasedAction` subclasses) | Build the request, fire `OnChangeRequested` — have no knowledge of networking or server-side state mutation. |
| Routing layer | `AConnectIt_PlayerController::ServerRouteBoardChangeRequest` (`UFUNCTION(Server, Reliable)`) | The **one** RPC. Validates turn ownership (`ParticipantComponent->IsMyTurn()`) before forwarding. |
| Dispatch point | `AConnectIt_BoardManager::ProcessRequest` | Routes by `RequestType` tag to private, `HasAuthority()`-gated handlers (`HandlePlacePieceRequest`, `HandleShiftRequest`). |

## Sequence

1. Player interacts with a `UTurnBasedAction` (e.g. hovers and selects a tile). The action validates the interaction locally (is this a legal hover/selection?) and builds an `FTurnActionRequest`, then fires `OnChangeRequested`.
2. The player controller catches this locally and calls its one `Server`, `Reliable` RPC, handing the whole request struct across the network boundary unexamined.
3. The server-side RPC implementation performs the one check every request needs regardless of type — is it actually this participant's turn? — and (in ConnectIt's case) stamps the server-authoritative `FactionID` before forwarding.
4. The RPC hands the validated request to the dispatch point, which reads `RequestType` and routes to the matching private handler. Handlers are gated by `HasAuthority()` as a second layer of defense even though the RPC itself only runs server-side.
5. The handler mutates state and commits it through whatever the project's single-source-of-truth mechanism is (see [SingleSourceOfTruth-Replication.md](SingleSourceOfTruth-Replication.md)) — this pattern doesn't care how the mutation is applied, only that client code never applies it directly.

## Why It's Reusable

Everything above the "Participants" table is generic: a tagged request struct, one RPC, one tag-dispatched handler. Any turn-based or action-gated multiplayer game with more than one or two distinct mutation types benefits from this instead of one RPC per mutation type, because:
- New mutation types are additive (new tag + new payload struct + new `case` in the dispatch point) rather than requiring a new RPC declaration each time.
- The "is this actually your turn / your action to take" check has exactly one place to live, not one per RPC.
- Client-side intent code (`UTurnBasedAction` subclasses in ConnectIt's case) is testable and reasoned-about independent of networking — it only ever *builds* a request, never sends one.

The `FGameplayTag` + `FInstancedStruct` combination is engine-specific to Unreal (gameplay tags and instanced structs are both UE features), but the *shape* of the pattern — tagged envelope, single ingress RPC, tag-dispatched authoritative handler — ports to any engine with some form of variant/tagged-union payload and an RPC or request-channel mechanism.

## Gotchas

- **The validation the RPC performs is easy to get subtly wrong** — see the [Known Issues](../README.md#known-issues) entry for `ServerRouteBoardChangeRequest_Implementation`: it currently checks *whose turn it is* but not that the request's `FactionID` actually belongs to that participant, because it only stamps `FactionID` when the client sent a negative/unset value. When reusing this pattern, stamp the authoritative identity field **unconditionally** from server-side participant state, never trust-if-present from the client payload — "only fill in if missing" is a gap a modified client can walk through.
- The dispatch point's per-type handlers still need their own `HasAuthority()` guard even though only the server ever calls them through this path — defense in depth against any future code path that calls the dispatch point directly (e.g. from a debug console command) without going through the RPC.
- A generic payload (`FInstancedStruct` or equivalent) trades compile-time type safety for extensibility — the dispatch point is the one place that must correctly match `RequestType` to the right payload type; a mismatch here fails at runtime, not compile time.

## Source

This pattern is synthesized from the live request-flow implementation across `AConnectIt_PlayerController`, `AConnectIt_BoardManager::ProcessRequest`, and the `UConnectIt_PlacePieceAction`/`UConnectIt_ShiftAction` classes — see [ConnectItModule.md](../ConnectItModule.md) for the full class catalogue. There is no standalone `.txt` design note for this one (unlike the other four workflows); the closest primary source is the `AConnectIt_PlayerController.cpp:92` TODO discussed in [Known Issues](../README.md#known-issues).

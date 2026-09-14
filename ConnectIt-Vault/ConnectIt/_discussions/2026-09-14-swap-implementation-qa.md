---
date: 2026-09-14
topics:
  - bp-ui-local-controller-gating
  - player-state-lookup-local-player-index-bug
  - action-activation-by-tag-not-class
  - swap-uses-remaining-tamper-hardening
tags:
  - discussion
---

# 2026-09-14 — SWAP Implementation Q&A

## Summary

Five self-contained technical Q&A exchanges from active SWAP-action implementation work
(not one continuous conversation on a single theme): gating Blueprint UI creation by
`Is Local Controller` so it doesn't run server-side; a real bug in
`GetLocalConnectItPlayerState` (it passed a faction slot index into a UE API that expects
a local-player index); how to activate an action the UI selected by class rather than
constructing a fresh, uninitialized instance; and two related fixes to
`AConnectIt_PlayerState`'s SWAP-uses-remaining state — removing a client-writable
`SetSwapActionUsesRemaining` Server RPC (its `HasAuthority()` guard was dead code, not
real protection) and renaming `TryConsumeSwapUse()` away from its misleading "Try"
framing while keeping it a guarded write rather than an unguarded setter.

## Topics

- **BP UI local-controller gating** — first time this came up
- **PlayerState lookup: local-player-index bug** — first time this came up
- **Action activation by tag, not class** — first time this came up
- **SwapUsesRemaining tamper-hardening** — continues [2026-09-12 — Action State Architecture](2026-09-12-action-state-architecture.md)
  (its "action state authority placement" topic raised the PlayerState-vs-ActionsComponent
  tampering question in the abstract; this is that same question landing on a concrete
  bug in the actual `SwapUsesRemaining` implementation)

## Updates

- UI creation on the PlayerController is now gated behind `Is Local Controller` — stops
  the server (dedicated, and the non-owning side of a listen server) from trying to build
  widgets. Matches the guard pattern `AConnectIt_PlayerController::BeginPlay()` and its
  base already use (`if (!IsLocalController()) return;`) before any client-only wiring.
- Fixed a real bug in `UConnectIt_GameUtilityLibrary::GetLocalConnectItPlayerState`:
  `UGameplayStatics::GetPlayerState(WorldContextObject, PlayerIndex)` takes a
  **local-player index** (which local player on this machine), not a faction slot —
  `GetLocalPlayerSlotIndex()`'s return value was being fed into the wrong kind of index,
  which happened to work for the player in slot 0 by coincidence and silently returned
  null for the player in slot 1. Fixed to resolve via
  `World->GetFirstPlayerController()->PlayerState` directly, with no index translation.
- Action activation from the UI's selected action class redirected to
  `ActionsComponent->TryPushAction(ActionTag)` (resolving the already-cloned
  `RuntimeActions` instance from `InitialiseFromLoadout`) instead of constructing a new
  instance from the raw class — a fresh `NewObject` would have none of the loadout-authored
  tag properties, never run `PostInitialiseAction`, and not be wired to the component's
  completion/board-change delegates.
- `SetSwapActionUsesRemaining` — a client-callable `Server, Reliable` RPC that wrote an
  arbitrary client-supplied value straight into `SwapUsesRemaining` — identified as a live
  tampering hole, not a hypothetical one. Its `HasAuthority()` check inside
  `_Implementation` was dead code: a Server RPC's implementation always executes
  server-side regardless of who calls it, so the check can never be false there. Flagged
  for deletion in favor of the existing guarded `TryConsumeSwapUse()` / GameMode's
  match-start grant as the only legitimate writers.
- `TryConsumeSwapUse()` to be renamed (its "Try" framing and bool return misleadingly
  imply a live gate, when by the time it's called the board mutation has already
  succeeded and there's nothing left to gate) — staying a self-contained guarded write
  (never below zero, never more than a one-per-call decrement) rather than becoming a raw
  setter, since a setter would move that invariant out to every future caller instead of
  enforcing it in one place. Adding `HasAuthority()` here *is* meaningful, unlike on the
  RPC — this is a plain C++ method with no built-in call-origin restriction, so the check
  is real protection against a future client-reachable call path, not a redundant one.

## Workflows

- **Adopted convention: gate PlayerController-owned UI creation behind `Is Local
  Controller`**, not `Has Authority`/`Get Net Mode` — the only check that correctly
  distinguishes "my own controller" from "a controller merely replicated to this machine"
  across dedicated server, listen server, and client.
- **Adopted convention: activate an action via `ActionsComponent->TryPushAction(ActionTag)`**,
  never via constructing a new instance from a `TSubclassOf<UTurnBasedAction>` — the
  loadout-cloned `RuntimeActions` instance is the only one carrying its configured tags
  and delegate wiring.

## Transcript

Q1: I am getting some errors in BP when initiating UI in BP through the PC. The issue I believe is because we are trying to create UI on the server. How can I check if a player is either a client or a listen server in BP?

A1: That's `Is Local Controller` — a pure BP node on the PlayerController itself. It's the right check here, and it's actually already the pattern this codebase uses in C++: `AConnectIt_PlayerController::BeginPlay()` and its base `ATurnBasedPlayerControllerBase::BeginPlay()` both guard with `if (!IsLocalController()) return;` before doing anything client-only (action wiring, ready notification). Mirror that same guard around whatever's creating the widget.

Why this is the right check specifically (not `Has Authority` or `Get Net Mode`):

- Dedicated server: `Is Local Controller` is `false` for every PlayerController that exists there — there's no local human at all, so nothing ever creates UI. This is your crash.
- Listen server: the host's own PlayerController → `true`. The other player's PlayerController, replicated onto the host's machine so the server can see it → `false`. This is the distinction `Has Authority`/`Get Net Mode` can't make for you — both of those are true/same for every PlayerController on that machine, since they describe the session as a whole, not which specific controller belongs to the person sitting there. If your UI code checked authority instead of local-controller, a listen server would end up trying to build UI for the remote player's controller too.
- Client: `true` for its own PlayerController (there's only ever one PC on a pure client that's locally controlled), `false` for nothing since a client never gets another player's PC replicated to it in a normal 1v1 setup.

So the fix is: wrap the UI-creation node(s) in a `Branch` off `Is Local Controller`, called on `Self` from inside the PlayerController's own graph (usually right where you're currently calling `Create Widget` / `Add to Viewport`). If the errors are specifically about `Add to Viewport` failing, that's the tell — there's no local viewport to add to on a server-only instance, exactly what this guard prevents.

`Get Net Mode` is still useful if you ever need to distinguish dedicated-server-vs-listen-server-vs-client for something else, but for "should this controller build its own UI," `Is Local Controller` is the one check that's correct in every topology without combining anything.

---

Q2: I have made the updates. I am working out getting the uses remaining to show up in UI but I it is not working at the moment. This is the function the widget is calling to get the local player state:

```
AConnectIt_PlayerState* UConnectIt_GameUtilityLibrary::GetLocalConnectItPlayerState(const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;
    
    AConnectIt_GameState* GameState = GetConnectItGameState(WorldContextObject);
    if (!IsValid(GameState)) return nullptr;

    APlayerState* PlayerState = UGameplayStatics::GetPlayerState(
        WorldContextObject, GetLocalPlayerSlotIndex(WorldContextObject));
    return Cast<AConnectIt_PlayerState>(PlayerState);
}
```

A2: Found it — this is a real bug, and a classic Unreal gotcha: `UGameplayStatics::GetPlayerState(WorldContextObject, PlayerIndex)` does not take a faction/slot index. It takes a local player index — which local player on this machine (0 for the first, 1 for a second local player in split-screen, etc.). Internally it's just `GetPlayerController(WorldContextObject, PlayerIndex)->PlayerState`, and `GetPlayerController` resolves that index against `GEngine->GetGamePlayer(World, PlayerIndex)` — the local-player list, which has nothing to do with `GameState->PlayerArray` or faction slots at all.

`GetLocalPlayerSlotIndex()` returns this player's faction slot (0 or 1, whichever side they're playing) — a completely different number that happens to coincide with the local-player index only by accident. Trace what actually happens:

- Player in slot 0: `GetPlayerState(WCO, 0)` asks for local player 0, which exists (it's this client's own), so it returns this client's own PlayerState — correct, but for the wrong reason. It works despite the bug, not because of it.
- Player in slot 1: `GetPlayerState(WCO, 1)` asks for local player 1 — which doesn't exist on a non-split-screen client (there's only local player 0) — so `GetPlayerController` returns null, and the whole chain returns null. This is almost certainly the player you're testing as when it "isn't working."

The fix is to skip `UGameplayStatics::GetPlayerState` entirely and go straight through the local controller, the same way `GetLocalPlayerSlotIndex()` itself already does:

```cpp
AConnectIt_PlayerState* UConnectIt_GameUtilityLibrary::GetLocalConnectItPlayerState(
    const UObject* WorldContextObject)
{
    if (!IsValid(WorldContextObject)) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!IsValid(World)) return nullptr;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!IsValid(PC)) return nullptr;

    return Cast<AConnectIt_PlayerState>(PC->PlayerState);
}
```

`GetFirstPlayerController()` is always this client's own controller, and `PC->PlayerState` is always its correctly-replicated own PlayerState — no index translation, no room for the mismatch. This also drops the `GameState` lookup, which was fetched and validity-checked but never actually used for anything in the original — dead weight.

One more thing worth checking once this compiles: confirm `SwapUsesRemaining` is actually replicating to that client at all — `ReplicatedUsing` PlayerState properties only push to a client once that PlayerState is relevant to them, which for PlayerState is normally "all clients" by default, so this should just work once the lookup itself is fixed. But if the widget still reads 0/stale after this fix, that's the next place to look, not the getter.

---

Q3: Okay I have the UI passing on the class of the action that should start. The PC is receiving this action subclass but how do we tell the ActionsComponent to run this action? Push requires and actual instance of an action.

A3: Don't construct a new instance from the class — that's the trap here, and it would silently break more than it looks like. Push the existing instance instead.

Here's why: `UTurnBasedActionsComponent::InitialiseFromLoadout` already clones every action in the loadout's `Actions[]` array into `RuntimeActions` exactly once, at setup (`CloneActionsFromLoadout` → `DuplicateObject` per entry, then `InitialiseAction`/`PostInitialiseAction` runs on each clone — TileRegistry resolution, input binding, delegate wiring to the component). That cloned instance is what carries the loadout-authored properties (`Tag_ValidHover`, `Tag_Default`, `Tag_FirstSelected` on your SWAP action, its `ActionTag`), and it's the specific object the component's `OnActionCompleted`/`OnChangeRequested` handlers are bound to. If the PC instead does `NewObject<T>(ActionClass)` from the raw class the UI handed it, you get a blank instance with none of that — no tags set, `PostInitialiseAction` never ran so `TileRegistry` is null, and nothing wires it to the component's completion/board-change handling. It would push onto the stack and then do essentially nothing correctly.

The framework already has the lookup you want, built for exactly this: `UTurnBasedActionsComponent::TryPushAction(FGameplayTag ActionTag)` — `BlueprintCallable`, on the component. It does `FindActionByTag(Tag)` — which searches `RuntimeActions` (and `RootAction`) for the already-cloned instance with that `ActionTag` — then `TryPushActionByRef` on whatever it finds.

So: change the UI→PC contract from passing a class to passing the `FGameplayTag` (the action's `ActionTag` — the same `ConnectIt.Action.SwapPiece` / `ConnectIt.Action.PlacePiece` tags from the loadout). Then on the PC: `ActionsComponent->TryPushAction(SelectedActionTag)` — one node, no lookup logic of your own to write.

If the UI genuinely can only hand you the class (say the buttons are built from a data table keyed by `TSubclassOf`, not tags), you can still avoid constructing anything — resolve to the existing clone instead, iterating `GetAllRuntimeActions()` (BlueprintPure) for the entry whose class matches, then `TryPushActionByRef` on it. Doable entirely in the graph — but it's a worse version of what `TryPushAction(Tag)` already does for you in one call, so switch the UI to tags rather than build this.

---

Q4: Can you elaborate about the issue with SetSwapActionUsesRemaining?

```cpp
UFUNCTION(Server, Reliable, Category = "ConnectIt|PlayerState")
void SetSwapActionUsesRemaining(const int32 RemainingUses);
```

```cpp
void AConnectIt_PlayerState::SetSwapActionUsesRemaining_Implementation(const int32 RemainingUses)
{
    if (!HasAuthority()) return;
    SwapUsesRemaining = RemainingUses;
}
```

If it's gated behind a HasAuthority then there shouldn't be an issue right? Only the server can call?

A4: The `HasAuthority()` check isn't adding protection — it's checking something that's already unconditionally true, so it can't be false to stop anything.

Here's the mechanism: `UFUNCTION(Server, Reliable)` means the engine transports this call across the network to the server, and runs `_Implementation` there. That's the entire meaning of "Server RPC" — a client calls `SetSwapActionUsesRemaining(RemainingUses)`, nothing executes locally, the engine ships it to the server, and the server is where the body runs. Every single time, no exceptions. So `HasAuthority()` inside `_Implementation` is asking "am I the server?" in a function that, by construction, only ever runs on the server. It will return true 100% of the time. It's not a gate — it's checking a fact that's already guaranteed before the function is entered.

What `HasAuthority()` answers is "is this machine the server" — not "was the caller of this function trustworthy." Those are different questions, and the RPC mechanism already answers the first one for you. The problem was never where the code runs — it always ran on the server. The problem is what value it trusts: `RemainingUses` is a number a client typed, and the function writes it straight into `SwapUsesRemaining` with zero validation against anything the server itself knows to be true.

Concretely: any client owning this PlayerState can call `PlayerState->SetSwapActionUsesRemaining(999)` right now — legitimately, through the normal RPC path, no exploit needed, `HasAuthority()` and all — and the server will set it to 999. That's not a hypothetical hardening gap, that's "SWAP's 3-use budget doesn't exist" the moment any client code (malicious or just a future bug) calls this instead of going through `TryConsumeSwapUse()`.

The fix isn't a better guard on this function — it's that this function shouldn't take a client-supplied value at all. The server already has the one legitimate writer: `TryConsumeSwapUse()` (decrements, checks `<= 0` first, logged) plus the match-start grant in GameMode. Notice `TryConsumeSwapUse()` isn't even a `UFUNCTION`, let alone a `Server` RPC — it's a plain C++ method, because its only callers are other server-only code (the Mediator). Recommendation: delete it. If you later need something to reset the count (a rematch flow, say), that's a plain server-side function GameMode calls directly — never one a client invokes with a number it picked.

---

Q5: I feel TryConsumeSwapUse() is confusing, it checks to see if possible to decrement after all the other validations and a new board state are done. It's awkward. Can we change it to a setter function, remove the UFUNCTION to hide it from BP and then add the HasAuthority() check to prevent clients from calling it in C++?

A5: Let's take the two asks separately, because they land pretty differently once you trace what each guard is actually protecting against.

"Remove the UFUNCTION" — there's nothing to remove. `bool TryConsumeSwapUse();` already has no `UFUNCTION` macro at all. It's already plain C++ — invisible to Blueprint, no RPC machinery, nothing network-aware about it. That's `SetSwapActionUsesRemaining` (the one being deleted) — `TryConsumeSwapUse()` was never that.

"Add HasAuthority() to prevent clients from calling it" — this one's actually right, and it's a good catch. A Server RPC's `_Implementation` is guaranteed to run on the server by the RPC mechanism itself — that's what made `HasAuthority()` dead code there. A plain C++ method has no such guarantee. If a client-side code path ever called `PlayerState->TryConsumeSwapUse()` directly, it would just run — locally, on that client's own copy of the object, no network involved, no protection at all. So `HasAuthority()` here isn't redundant the way it was on the RPC — it's a real, load-bearing assertion against a mistake that's otherwise fully possible. Add it.

The setter question is where I'd actually stop you. Right now, "never go below zero, only ever decreases by exactly one, only from a successful swap" is enforced in exactly one place, next to the data it protects. Swap that for `SetSwapUsesRemaining(NewValue)` and the Mediator becomes responsible for computing the correct value and never getting it wrong — every future caller re-derives the invariant instead of the invariant enforcing itself. `HasAuthority()` only closes the network attack surface; it does nothing about an ordinary same-side bug — a badly-written debug command, a copy-paste mistake in a second mediator six months from now — writing a bad number straight through. A guarded consume method is immune to that class of bug by construction. A setter isn't.

I think the real complaint is narrower than "wrong shape" — it's that `TryConsumeSwapUse()`'s name and bool return imply it's a live gate, when by the time it's called (after the board's already mutated) there's nothing left to gate — the actual gating already happened earlier, in the read-only `<= 0` check. That mismatch is what reads as awkward, not the guarded-write pattern itself.

So: keep it self-contained and guarded, lose the "Try" framing. Rename to `ConsumeSwapUse()` (or similar) — no "Try," and its internal `<= 0` check becomes a defensive `ensure()`-style safety net rather than a real decision point, since the Mediator's pre-check already means it can't actually fire. Add `HasAuthority()`. Don't add a setter.

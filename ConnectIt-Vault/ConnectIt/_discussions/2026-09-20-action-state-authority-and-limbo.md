---
date: 2026-09-20
source: "This Claude Code conversation — compiled excerpts (the Stage 2 design questions and answers from 2026-09-18, and the delegate-binding question from 2026-09-20)"
topics:
  - turn-end-count-source-and-replication-race
  - action-state-authority-placement
  - post-completion-limbo
  - request-action-identity
  - playerstate-delegate-binding-lifetime
tags:
  - discussion
---

# 2026-09-20 — Action State Authority and the Post-Completion Limbo

## Summary

How the new loadout/turn-end system should count uses and decide when a turn can end without
racing replication. The owner chose a single authoritative counter on `PlayerState`, and answered
the resulting client/server timing problem with a "limbo" state in the actions component; the
owner also corrected the first written version of that design (the action must still *finish* the
moment the server confirms, so a player can't spam requests). A second exchange asks why the
component binds to the `PlayerState`'s delegate when a request is sent rather than once at
BeginPlay, and moves the bind into an idempotent helper.

## Topics

- **turn-end-count-source-and-replication-race** — first time this came up
- **action-state-authority-placement** — continues
  [2026-09-12 — Action State Architecture](2026-09-12-action-state-architecture.md)
- **post-completion-limbo** — first time this came up
- **request-action-identity** — first time this came up
- **playerstate-delegate-binding-lifetime** — first time this came up

## Deliverables

- Implemented and building (not PIE-verified): `ATurnBasedPlayerState` per-action state, the
  Mediator gate, `ActionTag` on `FTurnActionRequest`, and the limbo in
  `UTurnBasedActionsComponent` — see
  [server gate in Mediator and state revision](../_decisions/2026-09-18-server-gate-in-mediator-and-state-revision.md).
- `EnsureBoundToPlayerState()` in `UTurnBasedActionsComponent`, called from `InitialiseFromLoadout`
  and `NotifyTurnStarted`.

## Updates

- Decisions recorded from this discussion:
  [single authoritative counter with limbo](../_decisions/2026-09-18-action-state-single-authoritative-counter-with-limbo.md)
  and [server gate, revision counter, ProducesRequestType](../_decisions/2026-09-18-server-gate-in-mediator-and-state-revision.md).
- The limbo's timeout (`StateSyncTimeoutSeconds`, 3s) and a replicated revision counter were
  added by the assistant to make the exit ordering exact; the owner's stated design was the
  delegate-only exit.

## Workflows

- **Post-completion limbo.** After the server confirms a request the action completes as normal;
  the actions component then enters limbo and does nothing until
  `PlayerState::OnActionRuntimeStateUpdated` fires, and only then checks auto-end-turn against the
  updated state. The delegate is bound permanently and its handler ignores updates unless limbo is
  active.

## Transcript

> Compiled from this conversation. Whole messages only, in order; the assistant's long planning
> and implementation messages between these exchanges are omitted, and the first assistant entry
> below is a verbatim excerpt of one paragraph of a plan document (marked). Nothing that appears
> is trimmed or paraphrased.

**[Owner]**
Okay let's start pushing towards deprecating the old system and inserting the new. Let's break it down by; what is new, what changes, what gets removed.

**[Assistant]** (three design questions, asked with the question tool)

Question 1 — header "Turn counts":
Auto-end-turn is evaluated on the client right after an action completes, but PlayerState is server-authoritative and replicates later — so a tree reading PlayerState's UsesThisTurn could see a stale count and fail to end the turn. Where should the turn-end tree read its counts?
- Client action instance, PlayerState reconciles (Recommended): Tree keeps reading each action instance's CompletionsThisTurn (the client's prediction, as today). PlayerState holds the authoritative UsesThisTurn/match uses/cooldown that the server checks on every request and the UI reads. Two counters, but no replication race.
- PlayerState only: One counter, fully authoritative, but auto-end waits on replication and can lag or race the outcome RPC; would need the server to drive turn-end.

Question 2 — header "Request identity":
The server routes requests by RequestType (e.g. ConnectIt_Game_Shift), which is not the action's tag, so it can't tell which config entry a request should spend. How should a request identify its action?
- Add ActionClass/tag to FTurnActionRequest (Recommended): Base RequestBoardChange stamps the action's own tag automatically so actions can't forget it. Server resolves it against the player's loadout entries, then each Mediator handler asserts the RequestType matches the action.
- Fixed RequestType→action map in the Mediator: No request change, but a hardcoded ConnectIt-only table that must be updated per new action; defeats the generic loadout goal.

Question 3 — header "State home":
Where should the loadout reference and per-action runtime state live?
- Plugin's ATurnBasedPlayerState (Recommended): Generic in UnrealTurnBasedMechanics, since nothing about it is ConnectIt-specific (matches the 09-18 decision). ConnectIt_PlayerState keeps only game-specific extras.
- ConnectIt_PlayerState: Faster to build now but locks the system into this game and would have to be extracted later.

**[Owner]** (answers)
1 (turn counts): I think one counter, fully authoritative. But I understand the catch that we have to wait for the server. That being said, there is a natural fix occuring here. Every action will create a visual update. Generally we do not want the player doing things during this visual update. Now the visual update does not happen until the server returns with the updated board state (therefore the updated player state as well). We should implement a sort of pause state for the actions component. After Complete gets called on the root action. We should move into some limbo action state. The only way out of this state is to react to a ActionRuntimeState delegate firing. At which point we read the updated state and check for turn end etc.
2 (request identity): Add ActionClass/tag to FTurnActionRequest (Recommended)
3 (state home): Plugin's ATurnBasedPlayerState (Recommended)

**[Assistant]** (excerpt — decision 1 as first written in the plan document)
1. **One authoritative counter.** Use counts live on `PlayerState` only. To avoid the client/replication race, the actions component gets a **limbo state**: after the server confirms a request the component does not finish the action or evaluate turn-end until an `ActionRuntimeState`-changed delegate fires, then reads the updated state and checks turn-end.

**[Owner]**
Just to clarify this is slightly incorrect.
- One authoritative counter. Use counts live on PlayerState only. To avoid the client/replication race, the actions component gets a limbo state: after the server confirms a request the component does not finish the action or evaluate turn-end until an ActionRuntimeState-changed delegate fires, then reads the updated state and checks turn-end.

The action should finish, we do not want a player sending numerous requests by accident. The action should finish, then the actions component enters a limbo state, waiting on PlayerState::OnActionRuntimeStateUpdated. When it gets this delegate call it checks auto end turn.

**[Assistant]** (excerpt — decision 1 and the limbo bullet as corrected in the plan document)
1. **One authoritative counter.** Use counts live on `PlayerState` only. To avoid the client/replication race, the actions component gets a **limbo state**. The action still finishes normally the moment the server confirms (`Complete()` runs as today, so the player can't fire off repeat requests by accident). Immediately after, the component enters limbo and does nothing until `PlayerState::OnActionRuntimeStateUpdated` fires; only then does it check auto-end-turn against the updated state.

- **Limbo state in `UTurnBasedActionsComponent`** (extends the existing
  `bAwaitingRequestConfirmation` / `AwaitingConfirmationAction` machinery): on a confirmed
  outcome the action completes as today, then the component enters limbo (stack mutation and
  new pushes stay blocked). The only exit is `PlayerState::OnActionRuntimeStateUpdated`; on it
  the component leaves limbo, reads the updated state and runs `CheckAutoEndTurn` (tree with
  state-backed counts). Ordering guard: record the pre-request `UsesThisTurn` at send time; if
  the state has already advanced when limbo is entered (update arrived before the outcome
  RPC), exit immediately instead of waiting. Includes a timeout/failure path so a lost update
  can't freeze the turn.

**[Owner]**
I'm looking over the changes, one question I have is about this function


```
void UTurnBasedActionsComponent::HandleBoardChangeRequested(const FTurnActionRequest& Request)
{
    if (bAwaitingRequestConfirmation)
    {
        // Shouldn't be reachable -- pushing AwaitingConfirmationAction
        // force-deactivates the requesting action, tearing down its
        // hover/selection bindings, so it can't legitimately fire a second
        // request while still waiting on the first. Kept as defense-in-depth
        // for a future action type that might not route through the same
        // Activate/Deactivate lifecycle.
        UE_LOG(LogTurnBasedMechanics, Error,
            TEXT("TurnBasedActionsComponent: HandleBoardChangeRequested -- "
                 "already awaiting a request on %s, ignoring"),
            *GetOwner()->GetName());
        return;
    }

    bAwaitingRequestConfirmation = true;
    PendingRequest = Request;

    // Listen for the state update this request will cause, and remember the
    // revision it must beat (see EnterStateSyncLimbo). Harmless for legacy
    // loadouts -- the handler ignores updates unless limbo is active.
    if (ATurnBasedPlayerState* PS = GetOwnerPlayerState())
    {
        PS->OnActionRuntimeStateUpdated.AddUniqueDynamic(
            this, &UTurnBasedActionsComponent::HandleActionRuntimeStateUpdated);
        RevisionAtRequest = PS->GetActionStateRevision();
    }

    // Push before flipping the flag -- PushAction refuses to run while
    // bAwaitingRequestConfirmation is true
    if (IsValid(AwaitingConfirmationAction))
    {
        PushAction(AwaitingConfirmationAction);
    }    

    OnBoardChangeRequested.Broadcast(Request);
    OnBoardChangeRequested_Native.Broadcast(Request);
}
```


We are binding to the PlayerState delegate when we enter the limbo state, but then we are not unbinding when the limbo state is exited. I understand that when we bind we are using adduniquedynamic so it shouldn't create numerous bindings, but it feels a little confusing to read. What is the argument against, binding to this on begin play and then just gating the function UTurnBasedActionsComponent::HandleActionRuntimeStateUpdated so that undesirable calls don't come through?

**[Assistant]**
One correction first: the bind happens when a request is sent, not when limbo is entered. Limbo comes later, after the action completes. The two also do different jobs. The **revision snapshot** has to be taken at request time, but the **delegate bind** has no reason to happen there. I put them together because they were convenient, not because it's right.

**Why I didn't bind at BeginPlay**
- **The PlayerState may not exist yet.** The component is initialised from the controller's `BeginPlay`, and on clients the PlayerState replicates in afterwards. A BeginPlay bind would silently bind to nothing, and the stale-binding problem gets worse if the PlayerState is ever replaced (reconnect or seamless travel).
- **Lazy binding sidesteps that.** By the time a request is sent the PlayerState is definitely there, and `AddUniqueDynamic` makes rebinding harmless.

**Your alternative is sounder, done at the right moment.** The handler already ignores updates unless `bAwaitingRuntimeState` is true, so a permanent binding with the existing gate is safe and easier to read. The only requirement is binding once the PlayerState exists, not at BeginPlay. There's also no unbind problem: dynamic delegates skip destroyed objects, and a stale binding on a replaced PlayerState just points at a dead object.

**What I'd change**
1. Add `EnsureBoundToPlayerState()`, which does the `AddUniqueDynamic` and is a no-op if there's no PlayerState yet.
2. Call it from `InitialiseFromLoadout` and from `NotifyTurnStarted`, where the PlayerState is certain to exist. That leaves the request path with only the revision snapshot.
3. Delete the stale comment "Harmless for legacy loadouts". Legacy loadouts no longer exist after Stage 3.
4. Add a one-line comment on the handler: "bound permanently; ignored unless limbo is active".

Want me to make that change?

**[Owner]**
Yes, make that change

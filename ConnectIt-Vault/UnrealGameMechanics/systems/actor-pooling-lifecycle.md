---
schema: systems
spans:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Pooling/ActorPoolSubsystem.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/Pooling/ActorPoolSubsystem.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Pooling/ActorPool.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/Pooling/ActorPool.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Pooling/ActorPoolInterface.h
reconciled: 2026-09-06
commit: bcdbce3
---

# Actor pooling lifecycle

## What happens

[[UnrealGameMechanics/code/UActorPoolSubsystem|UActorPoolSubsystem]] keeps one
`UActorPool` per actor class. A caller **gets** actors (pool grows if short), then
**activates** them as a separate step, uses them, **deactivates** them, and **releases**
them back. Get/Release manage pool membership; Activate/Deactivate trigger the actor's
show/hide side effect (`IActorPoolInterface` if implemented, else a default
hide/collision/tick toggle). The two pairs are split so the caller can bind a completion
listener before an activation that might finish synchronously.

## Diagram

```mermaid
stateDiagram-v2
    [*] --> Inactive: CreatePool / lazy on first GetObjects
    Inactive --> Retrieved: GetObjects(Amount)  (grows pool if needed)
    Retrieved --> Active: ActivateObject(actor)  → IActorPoolInterface::ActivatePoolObject\n(or default show/collision/tick on)
    Active --> Deactivated: DeactivateObject(actor) → DeactivatePoolObject\n(or default hide/collision/tick off)
    Deactivated --> Inactive: ReleaseObject(actor)  (back to InactivePool)
    Retrieved --> Inactive: ReleaseObject(actor)  (release without ever activating)
    Inactive --> [*]: Deinitialize() (world teardown)
```

## Steps

1. **(optional) `CreatePool(Class, InitialPoolSize)`** to pre-warm; otherwise the pool is
   created lazily on first `GetObjects`.
2. **`GetObjects(Class, Amount)`** → actors moved from `InactivePool` to `ActivePool`
   (`TSet`), created via `CreatePooledActor` if the pool is short. **Not activated.**
3. **Bind** any listener that the actor's activation might synchronously fire.
4. **`ActivateObject(actor)`** → `IActorPoolInterface::ActivatePoolObject` if implemented,
   else `SetDefaultActivation(actor, true)`.
5. Use the actor.
6. **`DeactivateObject(actor)`** → the mirror.
7. **`ReleaseObject(actor)`** → back to `InactivePool`; reverse-lookup (`ActorToPool`)
   means the caller doesn't track which pool it came from.

## Gotchas

- **Get does not activate; Release does not deactivate.** Miss step 3/4 ordering and a
  synchronous activation completion is lost — this is the entire reason the calls are
  separate.
- `ReleaseObject` on an actor that didn't come from `GetObjects` logs/asserts via the
  reverse lookup.
- Pools never shrink or evict before world teardown.
- `ActivePool` is a `TSet` — retrieval order across many `GetObjects` calls is not stable.

## Cross-impact

Consumers relying on the split timing: `UnrealGridMechanics` `UGridPieceRegistryComponent`
(pool retrieve/release of pieces), project VFX/projectile systems. Changing the
get/activate contract or `IActorPoolInterface` affects all of them.

## See also

- In-repo: `Docs/Systems.md` → pooling section.
- [[UnrealGameMechanics/code/UActorPoolSubsystem|UActorPoolSubsystem]]

---
schema: code
kind: UCLASS
role: primary
source:
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Pooling/ActorPoolSubsystem.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Private/Pooling/ActorPoolSubsystem.cpp
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Pooling/ActorPool.h
  - Plugins/UnrealGameMechanics/Source/UnrealGameMechanics/Public/Pooling/ActorPoolInterface.h
reconciled: 2026-09-06
commit: bcdbce3
---

# UActorPoolSubsystem

`UWorldSubsystem`. A per-world, per-class actor pool. `GetObjects` hands out actors
(growing the pool as needed); `ReleaseObject` returns them. **Retrieval and activation
are deliberately separate calls** so the caller controls activation timing.

Covers `UActorPool` (the per-class pool object) and `IActorPoolInterface` (implement on a
pooled actor to customise activate/deactivate).

## When you touch this

- Spawning/despawning many short-lived actors (pieces, projectiles, VFX) without
  `SpawnActor`/`Destroy` churn.
- Giving a pooled actor custom show/hide behaviour.

## Entry points

- `GetObjects(TSubclassOf<AActor>, Amount = 1, InitialPoolSize = 1)` → `TArray<AActor*>`.
- `ReleaseObject(AActor*)` — return to pool (does **not** deactivate).
- `ActivateObject(AActor*)` / `DeactivateObject(AActor*)` — explicit, separate from
  get/release.
- `FindPool` / `CreatePool` (pre-warm), reverse lookup via `ActorToPool`.
- On `UActorPool`: `GetActive/Inactive/TotalPoolCount`.
- `IActorPoolInterface::ActivatePoolObject` / `DeactivatePoolObject`
  (BlueprintImplementableEvent) — override the default hide/collision/tick toggle.

## Collaborators

- `UActorPool` per class; `Pools` + `ActorToPool` maps on the subsystem.
- Consumers: `UnrealGridMechanics` piece registry (`UGridPieceRegistryComponent`),
  project effect systems.

## Gotchas

- **`GetObjects` does not activate; `ReleaseObject` does not deactivate.** If a pooled
  actor's activation can complete *synchronously* (a legitimate pattern for pieces with
  no visible effect), you must bind any completion listener *before* calling
  `ActivateObject`, or you miss the signal. This is the whole reason the calls are split.
- `ReleaseObject` requires the actor to have come from `GetObjects` (reverse-lookup
  asserts/logs otherwise).
- Pools live until `Deinitialize` (world teardown) — no per-pool eviction.
- `ActivePool` is a `TSet`, `InactivePool` a `TArray` — ordering is not guaranteed on
  retrieval.

## Cross-impact

Change the get/activate contract and also update every consumer that relies on the
split timing (grid piece registry especially), and `IActorPoolInterface` implementers.

## See also

- In-repo: `Docs/README.md` → *Pooling*; `Docs/Systems.md` → pooling section.
- [[UnrealGameMechanics/systems/actor-pooling-lifecycle|systems/actor-pooling-lifecycle]]

# Pattern: Dual-Mode Subsystem Discovery

## Problem

Code all over a project needs a reference to "the one important object in this world" (a board manager, a match director, a level's central coordinator) — but that object is an `AActor`, so it doesn't necessarily exist yet at the moment every caller wants it, and a full `TActorIterator` world scan on every lookup is wasteful when the answer never changes once the object has spawned. Compounding this: different callers have genuinely different needs — some want to fail loudly and immediately if the object isn't there yet (a real bug, this should always exist by now), while others are asking opportunistically and just want to know "do we have it yet, and if not, tell me when we do" (e.g. a UI widget that might construct before the world is fully set up).

## Pattern

Cache the reference in a per-world subsystem that always exists for the lifetime of the world (so it's always safe to query or bind to, regardless of timing), populated by the object registering itself once on spawn. Then expose **two distinct access modes** on top of that single cache, rather than trying to serve every caller through one API:

1. **A loud-fail convenience entry point** — for callers confident the object should already exist; logs an error and returns null on a miss instead of silently returning null, because a miss here usually indicates an actual bug.
2. **A silent accessor plus a "ready" event** — for callers that can't guarantee timing; check the silent accessor first, and if null, bind the ready event instead of polling every tick.

```
Object spawns
  → registers itself with the per-world subsystem (runs on server AND every client)

Caller type A: "should exist by now"     Caller type B: "might be early"
  → LoudFailGetX()                         → subsystem.SilentGetX()
       miss → logs error, returns null          null? → bind OnXReady, use the
       hit  → returns it                              reference it passes when
                                                        it fires
                                              non-null? → use it directly
```

## Participants / Classes Involved

| Role | ConnectIt class | Note |
|---|---|---|
| Per-world cache, always exists | `UConnectIt_BoardManagerSubsystem` (`UWorldSubsystem`) | See [ConnectItModule.md](../ConnectItModule.md). Deliberately lives in the *game* module, not the plugin's own world subsystem — see [Gotchas](#gotchas). |
| Registration | `AConnectIt_BoardManager::BeginPlay()` → `RegisterBoardManager(this)` | Runs **outside** any `HasAuthority()` branch, on both server and every client's local copy — most callers of the "loud-fail" mode below are client-side, so registration has to happen on clients too. Safe to call more than once; last caller wins (handles PIE re-entry). |
| Mode 1: loud-fail convenience entry point | `UConnectIt_GameUtilityLibrary::GetBoardManager(WorldContextObject)` | Checks the subsystem cache, returns it directly. No fallback world scan. Logs an error and returns null on a miss — a miss is treated as a bug to surface immediately, not a race to paper over silently. |
| Mode 2: silent accessor | `UConnectIt_BoardManagerSubsystem::GetBoardManager()` (`BlueprintPure`) | Same cached value, but returns null with no logging if not yet registered — for callers checking opportunistically. |
| Mode 2: ready event | `UConnectIt_BoardManagerSubsystem::OnBoardManagerReady` | `BlueprintAssignable`, fires with the reference every time registration runs (idempotent registration, so effectively once per level unless the cached object is replaced). Bind once (e.g. in a widget's `Construct`) instead of polling from `Tick`. |

## Sequence

1. The subsystem is created automatically at world init — before any gameplay actor, including the object it will eventually cache, has necessarily spawned.
2. The object spawns and registers itself with the subsystem in `BeginPlay`, unconditionally (both server and client).
3. A caller confident the object should already exist by the time it runs (e.g. server-side code running well after match start) uses the loud-fail entry point directly — a null here means something is actually wrong.
4. A caller that can't guarantee timing (e.g. a UI widget that might construct before the object has spawned) checks the silent accessor first; if null, it binds the ready event and proceeds once it fires instead of polling. This order matters: binding without checking first is a race, since a broadcast that already happened does not replay to a listener that binds after the fact -- it would wait forever for an event it missed.

## Why It's Reusable

This is a generic answer to "how do callers reliably get a reference to a singleton-per-world actor without racing its spawn order," and it applies to any engine/framework with (a) some notion of a per-world/per-session service locator and (b) actors/objects that spawn at unpredictable times relative to the code that wants them:

- **Two access modes, not one compromise API.** A single API trying to serve both "this should definitely exist" callers and "this might not exist yet" callers ends up either logging spurious errors for the second group or silently swallowing real bugs for the first. Splitting them lets each caller category get the right failure behavior.
- **Register unconditionally on every machine that might query it**, not just the authoritative one — a cache that's only populated server-side is useless to the client-side code that's usually the majority of its callers.
- **Provide a "became ready" event alongside the accessor**, not just the accessor alone — this is what lets opportunistic callers avoid polling entirely.
- **Keep the cache in the module/layer that owns the concrete type being cached**, not in a lower-level generic layer that would have to depend back upward to know the concrete type — see the next section for why this specifically avoided a circular dependency in ConnectIt's case.

## Gotchas

- **This is easy to mistake for accidental duplication** — two ways to get "the board manager" can look, at a glance, like someone built the same thing twice. It isn't: the two modes serve genuinely different caller needs, and collapsing them back into one API would just reintroduce the problem this pattern solves. See [Duplication.md](../Duplication.md#not-included-the-two-board-manager-discovery-mechanisms) for why this specific pair is explicitly *not* listed as a duplication candidate. When applying this pattern elsewhere, document the two-mode split clearly (as this file does) so a future reader doesn't "simplify" it back into one function.
- **Placement of the cache matters for dependency direction.** In ConnectIt, the cached object (`AConnectIt_BoardManager`) is a game-module class, but the generic grid plugin already has its own per-world subsystem (`UGridHoverSubsystem`). Caching the game-module type *there* would require the plugin to depend back on the game module — a circular dependency most build systems (including Unreal's) reject outright. Keeping the cache in a new subsystem inside the consuming module instead avoids this entirely. When reusing this pattern, put the cache in the layer that's allowed to know the concrete type, not the lowest layer just because it "feels" more central.
- **A silent accessor is only safe for callers who genuinely handle the null case** — don't let "returns null quietly" become "callers stopped checking for null, and it turns into a hard-to-diagnose silent failure. Route callers who should be treating a miss as a bug through the loud-fail mode instead, deliberately.
- Registration being idempotent ("last caller wins") is what makes this safe under editor re-entry (PIE) or any scenario where the object might be recreated within the same world lifetime — don't assume registration only ever happens once.
- **The ready event does not need "clearing" after it fires, and shouldn't be.** Registration being safe to call more than once means the event needs to stay live to notify listeners of a future re-registration too — clearing it after the first broadcast would silently break that. A listener that only needs the reference once should unbind itself (`RemoveDynamic`) inside its own handler once it's done; that's a per-listener decision the subsystem has no way to make on their behalf.

## Source

Portable rewrite of `Source/ConnectIt/Workflows/BoardManagerSubsystem_Workflow.txt`, which remains the ConnectIt-specific implementation reference (exact function names, exact file paths, and the two-mode quick-reference table for this specific project).

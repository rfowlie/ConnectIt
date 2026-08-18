# Pattern: One Debug Widget Per Tracked Class

## Problem

A single monolithic debug widget that caches "board state," "turn state,"
"action state," etc. as grouped categories has two costs that compound as a
project grows. First, its refresh footprint is coarse: any one delegate
firing tends to trigger a refresh of every category, not just the one that
actually changed, which both wastes work and — as happened here — makes it
easy to refresh the wrong category or miss one entirely (a delegate fires,
`OnDebugStateUpdated()` broadcasts, but the specific `Refresh<Category>()`
that should have run never gets called because the category groupings don't
line up 1:1 with the delegates that actually exist). Second, its class
boundary doesn't match the codebase's real class boundary: "Turn State" as
a debug-widget concept doesn't correspond to any one class, so when the
underlying types change shape there's no single, obvious place to update.

## Pattern

Give each concrete class worth observing its own widget, named after
exactly the class it tracks, subclassing a single shared, minimal base.
Each concrete widget resolves and caches a pointer to exactly one source
object, exposes one `BlueprintPure` getter per field it cares about (no
wrapper structs grouping unrelated data), and binds only the delegates that
source class itself exposes — nothing else. The base class supplies the
mechanical parts every subclass needs identically (resolve-on-construct,
unbind-on-destruct, a single coarse "state updated" event) and leaves
resolving the source and reading its fields to each subclass's own
`BindDelegates()`/`UnbindDelegates()`/`RefreshFields()` overrides.

```
UDWidgetBase (UnrealGameMechanics, generic)
  RefreshAll() -> RefreshFields() + OnDebugStateUpdated()
  BindDelegates() / UnbindDelegates() / RefreshFields()  (virtual, empty)
      │
      ├─ DWidget_ConnectIt_BoardStateComponent   (ConnectIt)
      ├─ DWidget_ConnectIt_GameState             (ConnectIt)
      ├─ DWidget_TurnBasedParticipantManagerComponent (UnrealTurnBasedMechanics)
      ├─ DWidget_TurnBasedActionsComponent            (UnrealTurnBasedMechanics)
      └─ DWidget_GameEventTaskSubsystem               (UnrealGameMechanics)
```

Because the refresh handler for every bound delegate just calls the
inherited `RefreshAll()` (never a category-specific partial refresh), the
"which delegate implies which category" mapping gap that caused the
monolithic widget's staleness bug structurally cannot recur here — there is
only one category per widget, and it's always the one that just changed.

## Class ↔ Widget ↔ Location Mapping

The base class lives in the most foundational plugin (`UnrealGameMechanics`)
so any plugin or the game module can build on it. Each concrete widget lives
in **the module that owns the class it tracks** — not the module that
happens to be using the widget — and, if that module isn't the game module
itself, in a `Debug/` subfolder kept separate from the tracked class's own
folder (so `Debug/` reads as "instrumentation for classes defined
elsewhere in this module," not as part of the class's own feature folder).

| Widget | Tracks | Lives In | Header |
|---|---|---|---|
| `UDWidgetBase` | — (shared base, no tracked class) | `UnrealGameMechanics` | `Public/Widget/DWidgetBase.h` |
| `UDWidget_ConnectIt_BoardStateComponent` | `UConnectIt_BoardStateComponent` | `ConnectIt` (game module) | `Public/UI/DWidget_ConnectIt_BoardStateComponent.h` |
| `UDWidget_ConnectIt_GameState` | `AConnectIt_GameState` | `ConnectIt` (game module) | `Public/UI/DWidget_ConnectIt_GameState.h` |
| `UDWidget_TurnBasedParticipantManagerComponent` | `UTurnBasedParticipantManagerComponent` | `UnrealTurnBasedMechanics` | `Public/Debug/DWidget_TurnBasedParticipantManagerComponent.h` |
| `UDWidget_TurnBasedActionsComponent` | `UTurnBasedActionsComponent` | `UnrealTurnBasedMechanics` | `Public/Debug/DWidget_TurnBasedActionsComponent.h` |
| `UDWidget_GameEventTaskSubsystem` | `UGameEventTaskSubsystem` | `UnrealGameMechanics` | `Public/Debug/DWidget_GameEventTaskSubsystem.h` |

The rule that decides where a widget lives: **the tracked class's own
module**, full stop — never the module of whatever happens to be consuming
the widget today. `UTurnBasedParticipantManagerComponent` and
`UTurnBasedActionsComponent` are generic `UnrealTurnBasedMechanics` types
with no ConnectIt-specific knowledge, so their debug widgets moved there
even though ConnectIt is currently the only consumer — a second project
built on the same plugin stack gets them for free. `UConnectIt_BoardStateComponent`
and `AConnectIt_GameState` are genuinely ConnectIt-specific, so their
widgets stay in the game module.

## Naming Convention

`DWidget_<ClassName>`, where `<ClassName>` is the tracked class's name with
its `U`/`A`/`F` type prefix stripped (matching this project's general
convention of dropping the prefix when a class name is reused elsewhere,
e.g. in `Category` strings). The widget class itself still gets its own `U`
prefix, since it's a `UUserWidget` subclass in its own right — e.g.
`UConnectIt_BoardStateComponent` is tracked by `UDWidget_ConnectIt_BoardStateComponent`.
The shared base has no tracked class to name itself after, so it's just
`UDWidgetBase`.

## Participants / Classes Involved

| Role | Class | Note |
|---|---|---|
| Shared scaffolding | `UDWidgetBase` (`UnrealGameMechanics`, `UUserWidget`) | `RefreshAll()`, `OnDebugStateUpdated()` (BlueprintImplementableEvent), and the three protected virtuals every subclass overrides. Has no dependency on anything project- or plugin-specific beyond `UMG`. |
| Concrete widgets | `DWidget_*` (see mapping table above) | Each owns exactly one `UPROPERTY() TObjectPtr<T> ResolvedSource`, one `bool bSourceValid`, and one cached field per getter. |
| Escape hatch | `RefreshAll()` (inherited, `BlueprintCallable`) | Callable from BP as a manual "something looks stale" refresh, independent of any delegate. |

## Sequence

1. `NativeConstruct()` (base class) calls `BindDelegates()` once, then `RefreshAll()`.
2. A concrete `BindDelegates()` resolves `ResolvedSource` (via a subsystem, `GetOwningPlayer()->FindComponentByClass`, a utility-library getter, or a "ready" delegate if the source may not exist yet) and binds every delegate that source exposes to a small set of `UFUNCTION()` handlers.
3. Every one of those handlers does exactly one thing: call the inherited `RefreshAll()`. No handler ever calls a bespoke partial-refresh function.
4. `RefreshAll()` calls the concrete `RefreshFields()` (re-reads every cached field from `ResolvedSource`, first setting `bSourceValid = IsValid(ResolvedSource)`) and then fires `OnDebugStateUpdated()`, which a BP subclass implements to redraw from the widget's own getters.
5. `NativeDestruct()` calls `UnbindDelegates()` once, mirroring step 2's binds exactly.

## Why It's Reusable

- **The base class has zero project knowledge.** `UDWidgetBase` doesn't know what a "board" or a "turn" is — it only knows the resolve/bind/refresh/unbind shape. Any project built on `UnrealGameMechanics` gets it for free.
- **A widget's home module is a mechanical decision, not a judgment call.** "Which module defines the tracked class" answers "where does the widget go" every time, which is also what keeps this pattern from silently drifting back toward a monolithic, game-module-only widget as new tracked classes get added.
- **Per-widget refresh footprint scales with the number of classes actually observed, not with the total state surface of the game.** Adding a new tracked class means adding one new small widget, never touching an existing one.
- **The "always call `RefreshAll()`, never a partial refresh" rule is itself portable** — it's what makes this pattern immune to the exact staleness bug that motivated it (see Gotchas).

## Gotchas

- **This pattern exists because of a real bug, not as a preemptive best practice.** The predecessor monolithic widget (`UConnectIt_DebugStateWidget`, still present and intentionally untouched — see below) mapped several delegates to category-specific `Refresh<Category>()` calls; some categories only had *dead* or *intermittently-firing* delegates backing them (an empty stub, a phase enum that can "settle back" to its prior value within one replication tick and thus never re-fires its `OnRep`), so that category silently stopped updating while an unrelated, reliably-firing delegate kept the widget's top-level `OnDebugStateUpdated()` broadcasting anyway — everything looked alive, one category just quietly went stale. Splitting into one-widget-per-class doesn't fix flaky source delegates by itself, but it does mean each widget only has one category to get right, and "just call `RefreshAll()`" removes the mapping step where the original bug lived.
- **A tracked class's own replication/notification reliability still matters.** If the source class's delegate for a given change doesn't fire reliably (e.g. a property with no `ReplicatedUsing` on a monotonic value that should always notify), the widget built on top of it inherits that gap — see `UTurnBasedParticipantManagerComponent::TurnNumber`'s `OnRep_TurnNumber` fix, which existed before this widget family did but is exactly the kind of source-side fix this pattern depends on.
- **Never fold two classes' data into one widget for convenience.** `DWidget_ConnectIt_GameState` deliberately does not surface `AConnectIt_GameState`'s board-derived convenience wrappers (`GetFactionScore`, etc.) even though they're one function call away — that data already has a home in `DWidget_ConnectIt_BoardStateComponent`, and duplicating it here would reintroduce the "which widget is the source of truth for this field" ambiguity this pattern exists to avoid.
- **The original monolithic widget stays as-is, on purpose.** `UConnectIt_DebugStateWidget` (`Source/ConnectIt/Public/UI/ConnectIt_DebugStateWidget.h`) is not deprecated or replaced by this family — it remains a working, single-file, cross-referenced example wiring up every accessor in [RuntimeStateAccess.md](../RuntimeStateAccess.md). The two are parallel options, not a migration in progress.
- **Cached mutable object pointers need the same discipline as the source class itself.** A couple of concrete widgets cache a live `UObject*` rather than only value data (e.g. the top/root action tag is cached instead of the action pointer itself, specifically to avoid exposing `Complete()`/`Cancel()` through a debug-only getter) — when adding a new tracked class, check whether any of its exposed methods have side effects before deciding what to cache.

## Source

Introduced as `ConnectIt_DebugWidget_*` inside the ConnectIt game module,
then relocated per-class to the plugin that actually owns each tracked type
and renamed to the shorter `DWidget_*` convention once the pattern proved
out — see the mapping table above for current locations.

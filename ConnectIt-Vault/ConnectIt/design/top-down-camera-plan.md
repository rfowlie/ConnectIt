---
type: plan
status: not-started
approved: true
approved-on: 2026-10-24
completed-on:
playtested: false
design: "[top-down-camera](top-down-camera.md)"
question:
decision:
---

# Plan: top-down camera (rig, behaviour stack, profiles)

Design: [top-down-camera](top-down-camera.md) (2026-10-24). Used by the [adventure-hub-plan](adventure-hub-plan.md)
(Stage 4 camera) and by matches. Written 2026-10-24 in a design-only session; **approved by the owner 2026-10-24**. Not built -- building happens in
its own session.

## Context

Settled with the owner on 2026-10-24:

- **A custom camera stack**, game-agnostic, in **`UnrealGameMechanics`** (a new `Camera/` area). Replaces the imported
  `AGigafireCombatPawn` (`Plugins/UnrealUnsorted`), which is only a reference.
- **The rig:** a **camera pawn** possessed by the player controller, with the behaviour stack in a **rig component**;
  movement replication off (each client moves its own camera). `SetViewTargetWithBlend` stays available for special
  shots.
- **Behaviours** propose changes; the rig owns the view. An always-on **base layer** (player input) and **override
  layers** pushed by requests (Focus, Frame, Follow) with priority, blend and "interruptible by input"; then
  **constraints** (bounds, zoom limits); then one smoothing step. Requests return a handle with complete / interrupted.
- **Camera profiles** (data assets) say which behaviours, constraints and settings a context uses; a level names its
  profile.
- **First inputs:** edge scroll, drag to pan, wheel zoom toward the cursor, keyboard pan. Gamepad later.
- **The match camera may move on its own** to bring a move into view on a large board -- driven by board events as they
  play, only when the move isn't already on screen, interruptible, and a player setting. **An event waits for its
  automatic focus** before it plays.
- Local only: never replicated, nothing in gameplay depends on it.

Each stage builds and passes the tests before the next; the editor must be closed for builds; the owner commits between
stages.

## Stage 1: the rig and the view

- **`FCameraRigView`** (plain struct): focus point on the ground plane, zoom (arm length or height), yaw, pitch.
- **`UCameraRigComponent`** (`UnrealGameMechanics/Public/Camera/`): holds the current and the desired view; each tick
  runs the stack (Stages 2-3), clamps by constraints, smooths toward the desired view (one exponential smoothing per
  field, rates as settings), and applies the view to its owner's spring arm and camera (pitch on the **arm**, not the
  camera -- the imported pawn's issue). Uses its **owner's controller**, never `GetFirstPlayerController`.
- **`ACameraRigPawn`**: spring arm + camera + rig component; no movement replication; a profile to start with.
- Helpers: screen point → ground point (for zoom toward the cursor and drag); fit a set of points into view (center +
  zoom from the camera's FOV and aspect) for Frame.
- Tests (no world needed where possible): smoothing converges; constraint clamping; fit-points maths for a few shapes.

## Stage 2: behaviours, layers and requests

- **`UCameraBehaviour`** (abstract, `EditInlineNew`, Blueprintable): `Propose(const FCameraRigContext&, FCameraRigView&
  InOutDesired)`; `IsFinished()`; settings as properties. Blueprint subclasses allowed for game-specific ones.
- **Base layer:** the profile's base behaviours, always running, applied in order.
- **Override layer:** a stack of **requests** -- each a behaviour instance + priority + blend in / out time +
  `bInterruptibleByInput`. The highest-priority active request drives the desired view (blending with the base by its
  weight); finished requests pop themselves.
- **Requests API** on the rig component: `RequestFocus(TargetPoint / Actor, Params)`, `RequestFrame(Points, Params)`,
  `RequestFollow(Actor, Params)` → `FCameraRequestHandle`; `Cancel(Handle)`; delegates **OnComplete** (once) /
  **OnInterrupted**. Player input while an interruptible request runs cancels it (OnInterrupted).
- **Constraints** (`UCameraConstraint`, instanced): **bounds** (a rectangle, or a set of points + margin that callers
  can update -- e.g. the hub's revealed nodes) and **zoom limits**; applied after every layer.
- Tests: priority order; a finished request pops and fires OnComplete once; input interrupts an interruptible request
  and not a non-interruptible one; bounds clamp a focus that targets outside them.

## Stage 3: input behaviours (base layer)

Input reaches the rig through the player controller (Enhanced Input; the plugin's input tag binder where it fits):

- **Edge scroll** -- pan when the mouse is within an edge band (percent of the screen); nothing when the mouse is
  outside the window.
- **Drag to pan** -- hold a mouse button; the ground point under the cursor stays under the cursor.
- **Wheel zoom toward the cursor** -- zoom steps between limits, keeping the ground point under the cursor fixed.
- **Keyboard pan** -- WASD / arrows, relative to the camera's yaw.
- Pan speed scales with zoom. Each behaviour can be switched off by a player setting (a settings struct the game
  passes in -- the plugin knows no save system).
- Tests: the maths of drag (point stays under the cursor) and zoom-to-cursor; edge band detection.

## Stage 4: profiles

- **`UCameraProfile`** (data asset): base behaviours (instanced), constraints (instanced), smoothing rates, starting
  view (pitch, zoom, yaw).
- The rig pawn takes its profile from the level (ConnectIt: a field on its World Settings, next to the board / preset
  ones) or a default.
- Two first profiles (owner, editor): **hub** (edge scroll + drag + wheel zoom + keyboard; bounds from revealed nodes);
  **match** (wheel zoom + drag + keyboard; bounds from the board).

## Stage 5: ConnectIt wiring

- The game's player controller possesses an `ACameraRigPawn` (GameMode default pawn class, or spawned locally); turn and
  action input keeps going through the controller as today.
- **Match framing:** on board seed and on a layout change (Layout Changed / Tiles Added / Removed), request a **Frame**
  of the board's tiles (or the layout's camera position, from the phased-levels layout asset, when it has one) and
  update the bounds constraint.
- **Automatic focus on moves:** a small ConnectIt component listens to board events as they play (the display board
  the visuals use) and, for the opponent's / AI's moves -- and power chains -- issues a low-priority, interruptible
  **Focus** on the positions involved **only if they're not comfortably on screen**. It registers as an **async task
  in an earlier phase** for that event's tag (`UGameEventTaskSubsystem::RegisterAsyncTask`), so the event's visuals
  wait for the camera; it completes at once when no move is needed. A player setting turns it off.
- **Hub** (when the adventure hub is built): select a node → `RequestFocus(node)`, its OnComplete opens the panel; on
  load `RequestFrame` the revealed nodes and set the bounds to them.
- Tests: the "already on screen" check; the focus task completes immediately when nothing needs to move.

## Stage 6: vault

`UnrealGameMechanics` domain: a `code/` page for the camera system (it has no `_logs/` -- log under ConnectIt, as for
earlier plugin changes); ConnectIt: decision note (custom camera stack: rig component, layers, requests, profiles),
recipe "add a camera behaviour / profile", update [top-down-camera](top-down-camera.md) and the adventure hub plan's
camera reference; note that `AGigafireCombatPawn` in `UnrealUnsorted` can be deleted once this is in (the owner's call).

## Not in this plan

- Gamepad input; rotation (90° snaps); pinch zoom.
- Cinematic shots and camera shake.
- Split screen.

## Verification

- Each stage: build (editor closed); `Automation RunTests` for the plugin's and ConnectIt's camera tests pass.
- Owner, PIE: in a match, edge scroll / drag / wheel zoom / keyboard all move the camera and stay in bounds; on a large
  board the AI's off-screen move is brought into view before it animates, and moving the mouse interrupts it; a layout
  change reframes the board; a 2-player client moves its own camera independently.

## Settled before approval (owner, 2026-10-24: all three as written)

1. The rig's profile comes from the level's World Settings (with a project default).
2. Automatic focus covers the opponent's / AI's moves and power chains, not your own.
3. The camera pawn is the game's default pawn class (the GameMode spawns it), not spawned separately.

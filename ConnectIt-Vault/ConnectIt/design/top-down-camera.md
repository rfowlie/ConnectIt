# Top-down camera: needs and a movement-strategy system

Design note (2026-10-24). **Settled the same day; nothing built.** Plan: [top-down-camera-plan](top-down-camera-plan.md) (**approved 2026-10-24**, not built). For the adventure hub map
([adventure-hub-plan](adventure-hub-plan.md), Stage 4 "camera") and ordinary gameplay. Starting point: the owner's old
pawn, imported into `Plugins/UnrealUnsorted/` (a holding area: not enabled, nothing may depend on it).

## What the imported pawn does (`AGigafireCombatPawn`)

- A pawn with a spring arm (1200 long, no collision) and a camera pitched -60°.
- **Three movement modes, one at a time**, each a handler function bound to a single delegate that `Tick` calls:
  - **follow cursor** -- edge scrolling: the mouse within 15 % / 10 % of a screen edge pans at a fixed speed, relative to
    the pawn's yaw, clamped to rectangular map bounds;
  - **follow target** -- interpolate toward an actor (`VInterpTo`), broadcast "complete" when close; falls back to
    cursor mode if the target dies;
  - **move to location** -- interpolate toward a point, broadcast "complete" when close.
- Modes are switched through an interface (`IGigafirePawnHandler`: `SetFollowCursor`, `SetFollowTarget`,
  `SetMoveToLocation`, `SetCursorCanTick`).

### Issues seen (design review, not a fix list)

- **It won't build in this project as is:** `GIGAFIRE_API`, the missing `Interfaces/GigafirePawnHandler.h`, an
  include path from the old project, `GameFramework/Actor.h` for a pawn.
- **Strategies are hard-coded and exclusive:** adding a mode means editing the pawn; only one runs, so "edge scroll
  while zooming" or "player pans while a focus finishes" can't happen; there's no zoom, drag, keyboard or gamepad.
- **Bounds only apply in cursor mode**; follow / move-to can leave the map.
- **"Complete" fires every frame** while within the threshold (no once-only), and move-to never hands control back.
  The threshold (0.25 units) with an easing interpolation can take a long time to reach.
- **`GetFirstPlayerController()`** instead of the pawn's own controller (wrong in split screen / some multiplayer); the
  mouse position's success isn't checked (mouse outside the window).
- A mode is bound in the **constructor** (it runs on the class default object too); the delegate isn't
  Blueprint-visible.
- Pitch is on the camera, not the arm, so changing arm length also moves the camera sideways.

## What the camera needs (Claude, to discuss)

**Contexts:** the adventure hub; a match (Classic, versus, vs AI, adventure levels); phase / layout changes; event
playback (a power chain, a score) if the camera should draw attention.

| Need | Hub | Match | Notes |
| --- | --- | --- | --- |
| **Pan**: edge scroll, keyboard, drag, gamepad stick | yes | maybe (big boards) | each input a setting the player can turn off |
| **Zoom**: wheel / pinch, min-max, toward the cursor | yes | yes | pan speed scales with zoom |
| **Focus**: move smoothly to a point / actor, report done once | select a node | layout change, maybe an event | interruptible by the player, or not, per request |
| **Frame**: fit a set of points (centre + zoom from bounds and FOV) | revealed nodes on load | the whole board; a new layout | the "camera position" on a layout can be a framing |
| **Bounds**: keep the view inside an area that can change | the revealed area + margin | the board + margin | applied to every behaviour, last |
| **Rotate** (snap 90°?) | no | maybe | for boards seen from a side |
| **Smoothing** | yes | yes | one place, not per behaviour |

## A movement-strategy system (Claude, to discuss)

- **A rig and behaviours, separated.** The rig (pawn or view-target actor: arm + camera) owns the current view (focus
  point, zoom, yaw). **Behaviours** are small objects that each *propose* a change; the rig never knows which exist.
- **Layers with priorities**: an always-on **base layer** (player input: pan, zoom, edge scroll) and **override
  layers** pushed on request (Focus, Frame, Follow) with a priority, an optional blend in / out, and an "interruptible
  by input" flag. Each tick: the highest active override wins (or blends with the base), then **constraints** (bounds,
  zoom limits) clamp the result, then smoothing. When a request finishes it pops itself and broadcasts "done" once.
- **Requests return a handle**: `RequestFocus(Target, Params) -> Handle` with `OnComplete` / `OnInterrupted`; callers
  can cancel. The hub's "select node → centre it, then open the panel" is a Focus request whose completion opens the
  panel.
- **Camera profiles** (data assets): which base behaviours and constraints a context uses and their settings (hub:
  edge scroll + drag + zoom, bounds from revealed nodes; match: zoom + optional pan, bounds from the board). A level (its
  world settings) names its profile.
- **Behaviours as instanced objects** (`EditInlineNew` UObjects or instanced structs) so a profile lists them in the
  Details panel and Blueprint can add new ones -- no edits to the rig.
- **Local only:** the camera is per local player and never replicated; gameplay never depends on it.
- **Precedents worth a look before building:** Lyra's camera-mode stack (blended modes, a similar idea), and UE's
  Gameplay Cameras plugin (camera rigs / nodes; experimental in 5.5 -- check its state before relying on it).
- **Home:** game-agnostic, so a plugin -- a camera area in `UnrealGameMechanics`, or its own plugin -- not ConnectIt;
  `UnrealUnsorted` is only the holding area.

## Decided (owner, 2026-10-24)

- **The match camera can move on its own** -- e.g. during the AI's (or the other player's) turn, to bring the move into
  view when the board is too large to show everything at once.
- **A custom camera stack** (the layered behaviour / request / profile system above), built ourselves -- not the
  Gameplay Cameras plugin, not a port of Lyra's (still worth reading for ideas).

### What "moves on its own" implies (Claude)

- **Driven by board events as they play**, not by the server: when the event queue plays a move (Piece Placed, Pieces
  Swapped, a shift, a power chain), the local camera may issue a Focus request on the positions involved -- using the
  same display board the visuals use, so the camera arrives as the animation plays.
- **Only when needed:** skip the focus if those positions are already comfortably on screen (no camera jitter on a
  small board).
- **Whose moves:** the opponent's / AI's moves by default (your own move is wherever you just clicked); a power chain
  could follow its events.
- **The player stays in charge:** an automatic focus is interruptible by input, has a low priority, and is a player
  setting that can be switched off.
- **Gating the event queue:** decide whether an event waits for the camera to arrive (the queue already waits for
  visuals) or plays while the camera moves.

### Pawn or view-target actor (Claude, 2026-10-24, answering the owner's question)

- **Possessed pawn:** the player controller possesses it and, by default, looks through it. The usual choice for
  top-down / RTS games, where the camera *is* the player's body. In multiplayer the server spawns and owns possessed
  pawns, so the pawn must not replicate its movement (the camera moves locally only).
- **View-target actor:** any actor the controller looks through (`SetViewTargetWithBlend`) without possessing it. Each
  client can spawn its own locally (no server involvement), and switching between cameras blends for free -- useful
  when the player already possesses something else.
- **For ConnectIt either works:** no C++ code possesses a pawn today and possession has no gameplay meaning; input
  goes through the player controller. The decision that matters more is **putting the behaviour stack in a component**
  (a camera rig component) -- then it can sit on a pawn or any actor, and the choice stays reversible.
- **Decided (owner, 2026-10-24): a camera pawn possessed by the player controller, the behaviour stack in a rig
  component, movement replication off.** `SetViewTargetWithBlend` stays available for special shots.

## Open

- ~~Pawn or view-target actor?~~ Camera pawn + rig component (decided).
- ~~Does the match camera move by itself?~~ Yes (decided).
- ~~Which inputs ship first?~~ **Edge scroll, drag to pan, wheel zoom (toward the cursor), keyboard pan**; gamepad
  later (decided).
- ~~Gameplay Cameras, Lyra's pattern, or a custom stack?~~ Custom stack (decided).
- ~~Does an event wait for the camera?~~ **Yes**: an automatic focus becomes part of the event queue's wait, so the
  move plays in view; focus times kept short (decided).
- ~~New plugin or an existing one?~~ **`UnrealGameMechanics`** -- a camera area in the suite base plugin (decided).

Nothing left open. Plan written: [top-down-camera-plan](top-down-camera-plan.md).

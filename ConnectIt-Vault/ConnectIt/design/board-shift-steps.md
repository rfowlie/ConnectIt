# Board Shift — N steps

Design + as-built note (2026-10-08), design/ is schema-less. Extends [board-shift](board-shift.md): a shift
used to rotate a line exactly one place; it now rotates it `Steps` places. Decision:
[n-steps fixed per action](../_decisions/2026-10-08-board-shift-n-steps-fixed-per-action.md).

## How it works

- `FConnectItBoardOperation_Shift` gained `Steps` (default 1). `Apply` rotates the shiftable subset of the line so
  each tile takes the data of the one `Steps` places before it, wrapping at the ends. Steps = 1 is exactly the old
  behavior.
- **Counted in shiftable tiles.** A pinned (`bCanShift = false`) tile is skipped, so a tile jumps over it without
  spending a step. This matches how a one-step shift already treated pinned tiles.
- **Refused:** `Steps < 1`, and a whole number of laps (`Steps` a multiple of the shiftable count) — nothing would
  move. `Steps` larger than a lap but not a multiple of it works (it equals `Steps % shiftable`).
- **Fixed per action.** `UConnectIt_BoardShiftAction::ShiftSteps` (EditDefaultsOnly) is what the action sends; a loadout
  offers "Shift 2" as its own action class. The Mediator rejects a request whose `Steps` differs from the sending
  action's `ShiftSteps` (`Steps` is client-supplied). `Direction` is not checked — it is runtime-settable.
- **One use** however large N is; balance through the N a loadout grants.
- **Visuals unchanged.** `FConnectItBoardEvent_BoardShifted` still carries `StartPositions[i] -> EndPositions[i]`
  (plus the new `Steps`); each tile slides straight to its end. The piece registry re-keys from those pairs, so it
  needed no change.
- **Scoring** is the shared after-move step (`ResolveBoardChange`) on every position in the line that now holds a
  piece, unchanged.

## Open / not done

- **Loop visuals.** With N > 1 more tiles wrap, and each crosses the board through the others in one slide. Still the
  open [board-shift-loop-visuals](../_questions/board-shift-loop-visuals.md) question; N sub-step animation would need
  the step count in the event (now there) plus per-step events.
- **Player-picked N or sign**, and a **destination preview** (hover still highlights the whole line).
- **Scoring order.** `ResolveBoardChange` scores touched positions one at a time and an earlier score can clear pieces
  a later position needed, so which line scores can depend on line order. It is shared by Place, Swap and the AI
  search, so changing it to snapshot-based scoring is a rules-wide decision, not part of this change. More tiles
  relocating per shift makes it likelier to matter.
- **Generic operation validation.** The Steps check is a special case in `UConnectIt_BoardRequestMediator::ProcessRequest`;
  a per-action hook is the cleanup if a second action needs one.

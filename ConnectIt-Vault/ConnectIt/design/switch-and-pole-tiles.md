# Switch and pole tiles

Board-mechanic idea (2026-09-21), design only. Two special tile roles that work together:

- **Switch tile** — an ordinary placeable tile that controls one or more poles.
- **Pole tile** — an **occupied** tile: players can never place a piece on it, whether or not it
  currently has a faction. It takes on the faction of the piece on its controlling switch, and it
  can be part of scoring lines like a piece.

Like [Crumble](tile-powers.md#crumble), it's a per-level mechanic, and it wants the layering in
[board-request-objects](board-request-objects.md) (a reaction, a semantic step, tile roles).

## Rules (owner's answers, 2026-09-21)

- **A pole mirrors its switch.** Piece on the switch → the pole takes that piece's faction. No piece
  on the switch → the pole is **neutral** and can't contribute to scoring. It is still unplaceable
  while neutral.
- **The pole changes only when the switch's piece changes** (a different owner, or removed). Being
  part of a scoring line does **not** change it — a pole keeps its faction after helping score.
- **The switch fires on every ownership change**, so it works after any move that puts a piece on
  it or changes who owns it: place, capture, swap, board shift, force-place. It is "on" whenever a
  piece occupies it and "off" when empty (one rule, no one-shot).
- **Poles carry no multiplier.** Depending on balance they might not contribute points at all —
  decided by playtesting, not up front.
- **Poles are shiftable.** Board Shift can move them like any tile; the switch link survives via the
  group tag (see below), so nothing needs pinning.

## How it maps onto what exists

- **Scoring only reads `FactionPiece`** (`UConnectIt_LineScoringRule::FindScoringLines`), so a pole
  that holds a faction already counts toward a line with no rule change.
- **Unplaceable already has a flag** — `bIsOccupied` is documented as "active but not placeable by
  players (non player piece occupying)" — but `SetFactionPiece` overwrites it from `FactionPiece`, and
  a *neutral* pole (`FactionPiece = -1`) would then read as empty and placeable. Poles need an explicit
  role marker rather than relying on `bIsOccupied`.
- **`ApplyScoringLine` must leave poles alone.** It empties every tile in a completed line except
  the completing one and adds 1.0 to each tile's multiplier. Poles neither get cleared nor gain a
  multiplier, so it needs a pole-aware exemption (and a decision on whether a pole's points count).
- **Model it as derived state, not a trigger.** A pole's faction is a function of its switch, so a
  "sync poles" reaction sets each pole from its switch — idempotent, and it scans the whole board.
  That single rule covers place, capture, swap and shift with no per-request trigger logic.
  **Timing is not a real problem** (owner's read, confirmed): a pole that turns from neutral to a
  faction after a placement can still complete a line, and a pole that turns neutral simply stops
  contributing, so neither ordering breaks scoring. The one implementation requirement is that
  scoring also evaluates the poles that just changed (they are touched positions, not only the
  placed piece) — the mutate → score → react loop in the request-object design. An alternative that
  avoids sync timing entirely is to compute a pole's effective faction on read (from its switch)
  and only emit a step when it changes for visuals.
- **Visuals** come from a `PolesSynced` step `{Subject = switch, Related = poles that changed,
  FactionA = new owner}`, emitted only when something changed, animated in Blueprint.

## Design implications

- **Tile role, not more bools.** Pole, switch and Crumble's lava are all "what kind of tile is
  this". A tile-role gameplay tag on `FConnectItTileData` (replicated with the tile) also answers
  the open lava-vs-toggled-off question.
- **Links by group tag, not by position.** Board Shift moves whole tile data, so a switch→pole
  link stored as positions would break after a shift. A shared group tag on the switch and its poles
  survives moves and allows many poles per switch. It fits the designer-set tile values path
  (`IConnectIt_TileDataProvider`).
- **Every piece-touching request needs a rule for poles.** Swap, Capture, Remove and Force-place
  validate with `bIsOccupied` and would treat a pole as a piece. Poles should be immune, in one
  shared predicate rather than per handler.
- **Holding a switch is strong.** Its poles stay a faction and stay usable in lines for as long as
  the piece is there, since scoring doesn't consume them. The counterplay is capturing or swapping
  the switch piece, or scoring the switch's own piece away.

## Later idea: several switches controlling one pole

Saved for later; the rules above assume exactly one controlling switch per pole. Two variants:

- **Unanimous control:** a pole takes a faction only if one faction controls *all* its switches
  (otherwise neutral).
- **Graded contribution:** the more of a pole's switches a faction controls, the more points the
  pole contributes to a scoring line for that faction.

Either way the group-tag link (many switches, many poles) already supports it.

## Open questions

- Do poles contribute points at all when they're in a scoring line, or only complete it? (Balance;
  playtest.)
- Flipping poles can also *break* an opponent's line that ran through them. Intended counterplay?
- The MinMax/AI move generation and the influence map read `FactionPiece` directly; a placement
  that changes distant poles changes what a simulated move does, so the AI needs the same reaction
  logic.

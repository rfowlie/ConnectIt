# Swap Piece (SWAP)

A limited-use trade: give up the position of one of your own pieces to take the position
of another occupied tile. The disruptive, resource-gated alternative to Place Piece.

## How it works

- Two-step selection: pick one of **your own** pieces first, then pick **any other
  occupied tile** — your own or an opponent's.
- The two tiles' occupying factions trade places.
- **Server rule, not obviously visible client-side:** exactly one of the two tiles must
  belong to you. Picking two of your own pieces (or, if it were ever possible, two
  belonging to someone else) is silently rejected by the server — the client-side hover
  filter only enforces "first pick is mine," not the full rule, so a player can select a
  second tile that also happens to be their own and have the request bounce.
- Limited use: each faction has a per-match SWAP budget (`SwapUsesRemaining` on
  PlayerState), consumed only once the server actually commits the swap — a rejected
  attempt costs nothing.
- Mandatory-alternate with Place Piece: completing a SWAP satisfies the turn on its own,
  the same way completing a placement does. A player never has to place a new piece on a
  turn where they SWAP instead.
- Scoring is re-checked for **both** resulting positions independently, since they now
  belong to two different factions. Either side, or both, can complete a line off the
  same swap.

## Gameplay ramifications

- **It's a double-edged tool.** Swapping one of your pieces into an opponent's near-complete
  line hands them the missing piece — the same move that repositions your own piece into a
  scoring spot can just as easily gift the opponent theirs. New players are especially
  likely to not think through the *opponent's* resulting position, only their own.
- **The turn-end-alternate framing means SWAP can substitute for placement entirely.** A
  match where both players lean on SWAP instead of placing could plausibly slow board
  fill-rate and pacing compared to a match of pure Place Piece turns — worth watching in
  playtesting, not just balancing the SWAP budget number itself.
- **The budget is the real balance lever**, not the swap itself — a generous budget makes
  SWAP close to a second Place Piece with extra flexibility; a tight budget makes each use
  a real strategic commitment. No number is set in stone here yet.
- Because the "must own exactly one side" rule isn't client-enforced on the second pick,
  the Actions UI should probably surface *why* a swap got rejected (or filter the second
  pick's valid targets down to only the legal set) once that polish pass happens — right
  now a rejected swap looks identical to any other rejected request.

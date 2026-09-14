---
created: 2026-09-13
question: "Should the action-state architecture question (ConnectIt/_discussions/2026-09-12-action-state-architecture.md — PlayerState vs. ActionsComponent as source of truth, ActionsComponent as a generic push/pop state machine) get real design time now, or wait?"
status: answered
tags:
  - question
---

# Should the action-state architecture question get real design time now, or wait?

## Why it matters

This follows directly from the 2026-09-12 consultation
(`2026-09-12-board-mediator-refactor-vs-demo-progress.md`), which recommended finishing
one concrete action-acquisition mechanic before revisiting Mediator/BoardRules
architecture. SWAP is now that mechanic — built, with its use-counter on `PlayerState`.
The owner is now raising a broader architecture question about *how* action state should
generally be modeled and owned, prompted by the same session's work, and pushed back
explicitly on the prior "don't refactor yet" guidance as potentially under-weighting a
real risk.

## What would answer it

Whether the current SWAP implementation actually exhibits the problem the discussion
worries about (PlayerState needing to sync from ActionsComponent), or whether the concern
is speculative — designed for a second action that doesn't exist yet.

## Current thinking

See Resolution — answered in this consultation.

## Resolution

### 1. The question
Should the action-state architecture question raised in
`ConnectIt/_discussions/2026-09-12-action-state-architecture.md` — where per-action state
should live, PlayerState vs. ActionsComponent, and whether ActionsComponent should be
redesigned as a generic push/pop state machine — get real design/implementation time now,
or wait?

### 2. My take

I read the discussion. Here's what I actually see: you built SWAP's use-counter already,
and you didn't fall into the trap the discussion is worried about — `SwapUsesRemaining`
lives directly on PlayerState, server-set, never synced-from-ActionsComponent. The "bad
setup" the note flags (PlayerState needing a round trip to stay current with
ActionsComponent) is a problem you're imagining for a *second* action, not one that exists
in the code you shipped yesterday.

That's the tell I want to name, hands-dirty: you finished one concrete implementation and,
same day, started designing the general case for it — before a second action exists to
check the shape against. `career-coach.md` calls this exact move "over-generalizing
because committing to the specific is scary, dressed up as building for what comes next."
I'm not saying that's what's happening — I don't get to decide that for you — but the
mechanical fact stands regardless of motive: you cannot design a good generic abstraction
from one example. You need two, minimum, before the real shared shape shows itself. Right
now you have SWAP and a hypothesis.

This isn't "don't do it." It's "you don't have the input data yet."

**Bottom line:** finish proving SWAP works before designing the framework SWAP is
supposedly an instance of.

### 3. Where I'd push back

Your own framing — "we shouldn't just shove any old implementation in right now, it could
cause massive issues later" — treats what's built as fragile. Is it? Walk through the
actual failure mode: a third action shows up needing similar state, and the fix is
extracting a pattern you can now see twice into a shared base. That's a Tuesday refactor,
not a landmine. "Not yet generic" and "will cause massive issues" are different risk
categories, and the discussion note argues for the second while only having evidence for
the first.

### 4. What I'd actually do

1. Finish what's already sitting open in `ConnectIt/_tasks/active.md` from the SWAP
   session — confirm `RootActionClass` isn't PlacePiece, confirm the `ActionsComponent`
   Class Defaults survived the component-class swap — and actually run SWAP in PIE.
   That's the real "give this proper care" move: verify the one thing you built, not
   redesign the system around it.
2. Leave the action-state-architecture note exactly where it is — an open Question in the
   discussion note, not acted on. It's real, it's not wasted thinking, it's just early.
3. The moment a second action (or a deliberate "let's pressure-test this" prototype)
   needs similar state, pull this note back out — now you're generalizing from two real
   shapes instead of one imagined failure mode.

**Don't:** start moving state off `ActionsComponent` or redesigning it as a push/pop
machine before SWAP has actually run once in PIE. You'd be refactoring a system to fix a
problem you haven't confirmed it has yet.

### 5. Log the consultation

Written to this file
(`Development/optimal-co-developer/_questions/2026-09-13-action-state-architecture-vs-demo-progress.md`).

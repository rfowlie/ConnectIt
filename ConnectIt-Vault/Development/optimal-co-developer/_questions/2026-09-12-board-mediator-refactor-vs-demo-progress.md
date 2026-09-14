---
created: 2026-09-12
question: "What do you think about the Board Request Mediator / BoardRules refactor and player-action-config redesign discussion (ConnectIt/_discussions/2026-09-12-board-mediator-and-action-config-design.md)?"
status: answered
tags:
  - question
---

# What do you think about the Board Request Mediator / BoardRules refactor and player-action-config redesign discussion?

## Why it matters

The discussion proposes re-architecting `UConnectIt_BoardRequestMediator` /
`UConnectIt_BoardRules` — a system `ConnectIt/CLAUDE.md` already documents as
post-refactor — in service of an explicitly unresolved game-design question (how players
earn/spend actions). Worth knowing whether this is warranted foundation-building or the
perfectionism/generalization pattern flagged in `career-coach.md`, before time goes into
it.

## What would answer it

Whether the proposed rework is grounded in a mechanic that's actually been built, or is
speculative flexibility for mechanics not yet chosen between.

## Current thinking

See Resolution — answered in this consultation.

## Resolution

### 1. The question
What's my take on `ConnectIt/_discussions/2026-09-12-board-mediator-and-action-config-design.md`
— the Board Request Mediator / `BoardRules` refactor and player-action-config redesign
discussion?

### 2. My take

Two items in there are cheap, real fixes: the `MustImplement` UPROPERTY syntax is just a
bug, and `IsTilePlaceable` as an actual rule instead of hardcoded logic is a small,
bounded gap-close. Do both, no debate needed.

The rest — folding the Mediator into GameMode, dissolving `BoardRules` into independent
interfaces, making requests swappable strategy types — is a different animal.
`ConnectIt/CLAUDE.md` already calls this system "post-refactor." This is a second pass at
architecture just finished, in service of a game-design question (how do players earn
actions — marker tiles, score thresholds, dice?) that's explicitly unresolved. The
transcript's own words were "we are trying to be flexible… just in case the core game
loop can be improved upon" — that phrase is the tell `career-coach.md` named directly:
generalizing to avoid committing to the specific, dressed up as future-proofing. Building
a maximally flexible rule-interface system for three hypothetical mechanics not yet
chosen between is the shape of perfectionism filling the vacuum where a decision should
be.

**Bottom line:** ship the two cheap fixes now; don't touch the Mediator/`BoardRules`
architecture until one action-acquisition mechanic has actually been picked and
prototyped.

### 3. Where I'd push back

The framing treats this as an architecture question — "where should the rule live." It
isn't. It's a design question wearing architecture clothes. You can't know whether
`BoardRules` should dissolve into independent interfaces until you know what the
interfaces actually need to express, and you don't know that until one real acquisition
mechanic is built end-to-end. Refactoring ahead of that is designing the shelf before you
know what's going on it.

### 4. What I'd actually do

1. Fix the `MustImplement` UPROPERTY syntax and add the `IsTilePlaceable` rule — both
   bounded, both real, do them this session.
2. Pick the single simplest action-acquisition mechanic to prototype for the white-box
   demo — "always available, limited uses" needs no board-scanning or marker-tile logic,
   so it's the cheapest one to build and test the loop with.
3. Build that one mechanic against the *current* Mediator/`BoardRules` shape, even if
   it's awkward — let the awkwardness reveal the real interface boundary.
4. Only then revisit whether `BoardRules` dissolves or the Mediator folds into GameMode —
   as a refactor grounded in one working mechanic, not a speculative rewrite for three
   undecided ones.

**Don't:** start restructuring `BoardRequestMediator`/`BoardRules` ownership before one
action-acquisition mechanic is actually working — that's re-architecting a system just
rebuilt, for requirements that don't exist yet.

### 5. Log the consultation

Written to this file
(`optimal-co-developer/_questions/2026-09-12-board-mediator-refactor-vs-demo-progress.md`).

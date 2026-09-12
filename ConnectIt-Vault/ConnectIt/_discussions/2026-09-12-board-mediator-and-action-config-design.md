---
date: 2026-09-12
topics:
  - board-request-mediator-architecture
  - board-rules-refactor
  - is-tile-placeable-rule
  - mustimplement-instanced-uproperty-syntax
  - action-granting-revoking-design
  - player-action-config-redesign
  - alternate-game-modes-for-playstyles
tags:
  - discussion
---

# 2026-09-12 — Board Request Mediator & Player Action Config Design

## Summary

Design pass on the Board Request Mediator and `UConnectIt_BoardRules`: whether the
Mediator should fold into GameMode or stay a swappable UObject with strategy-pattern
request types, and whether `BoardRules` should dissolve into independent rule interfaces
living on the Mediator (mirroring how `UConnectIt_LevelConfigDataAsset` already separates
rule interfaces). Also covers a missing `IsTilePlaceable` rule, the correct
`MustImplement` UPROPERTY syntax for interface-constrained config properties, the open
question of where/how players gain or lose actions during a request update, and a desire
to rethink the old rigid player action config.

## Topics

- **Board Request Mediator architecture** — first time this came up
- **BoardRules refactor** — first time this came up
- **IsTilePlaceable rule** — first time this came up
- **MustImplement Instanced UPROPERTY syntax** — first time this came up
- **Action granting/revoking design** — continues [2026-09-11 — SWAP Action & Actions UI Design](2026-09-11-swap-action-ui-design.md)
  (its Questions section already raised "how are actions earned — always available with a
  limited number of uses, or earned via a special randomly-spawned power tile" — this
  discussion revisits that same open question)
- **Player action config redesign** — continues [2026-09-11 — SWAP Action & Actions UI Design](2026-09-11-swap-action-ui-design.md)
  (that note's proposed GameMode-side "actions mediator," which computes and pushes
  per-level action availability to PlayerState, is the mechanism this discussion is now
  trying to redesign the config surface for)
- **Alternate game modes for playstyles** — first time this came up

## Tasks

- [ ] Add an `IsTilePlaceable` rule — current logic is hardcoded to "no other piece is
      there," but other reasons a tile can't be placed may come up
- [ ] Brainstorm other required board rules beyond `IsTilePlaceable`
- [ ] Fix config rule UPROPERTYs to use the correct `MustImplement` setup, e.g.
      `UPROPERTY(EditAnywhere, Instanced, Category=Mover, meta=(MustImplement="/Script/Mover.TurnGeneratorInterface")) TObjectPtr<UObject> TurnGenerator;`
      (`TSubclassOf<UObject>` also works) — see
      [unreal-garden.com/docs/uparam/#mustimplement](https://unreal-garden.com/docs/uparam/#mustimplement)

## Deliverables

- Rule interfaces living independently on the Mediator (replacing the `BoardRules`
  encapsulating object), matching how `UConnectIt_LevelConfigDataAsset` already separates
  rule interfaces
- An `IsTilePlaceable` rule interface, wired into placement-validation logic
- Config rule properties corrected to the proper `MustImplement`-constrained UPROPERTY
  form

## Questions

- Should the Board Request Mediator fold into GameMode, or stay a separately assignable
  UObject so the set of valid requests can be altered (e.g. per level)?
- Should all requests become strategy-pattern/interface types that can be added or
  removed via level config, rather than being fixed?
- Should `BoardRules` be dissolved entirely, with its rule interfaces living
  independently on the Mediator instead of encapsulated in one class? (Leaning yes —
  `UConnectIt_LevelConfigDataAsset` already does something like this and "feels better.")
- What other board rules are needed beyond `IsTilePlaceable`? Not yet brainstormed.
- Where should the check live for granting/revoking a player's actions during a request
  update — e.g. scoring a line via a tile marked as holding a skill grants a single use of
  that skill? Candidate: `BoardRules`, inside a custom `ApplyScoring`. Not settled — would
  rather keep it flexible in case the core game loop changes.
- How do players actually acquire actions, in general — scoring on marker tiles
  (Scrabble-style), unlocking a skill every X points, rolling a random die every other
  turn? Not decided; described as a "blocking design decision."
- Are acquired actions permanent, limited-use, or must be used immediately alongside
  placing a piece? Not decided.
- Should alternate play styles (different action-acquisition rules) be handled via
  separate game modes rather than one universal flexible system?
- What are the core requirements for a redesigned player action config, beyond the two
  named so far — "what actions are allowed for this level" and "what actions do players
  start with"? Explicitly open — the old player action config is considered "really
  rigid" and in need of rethinking.

## Updates

- `UConnectIt_LevelConfigDataAsset` is already partway toward the direction favored for
  `BoardRules`: it separates rule interfaces on the config rather than asking for one
  encapsulating board-rules object.

## Workflows

- **Correct `MustImplement`-constrained UPROPERTY pattern for config rules**, to replace
  the current (incorrect) setup:
  `UPROPERTY(EditAnywhere, Instanced, Category=Mover, meta=(MustImplement="/Script/Mover.TurnGeneratorInterface")) TObjectPtr<UObject> TurnGenerator;`
  — `TSubclassOf<UObject>` is also a valid form. Reference:
  [unreal-garden.com/docs/uparam/#mustimplement](https://unreal-garden.com/docs/uparam/#mustimplement).

## Transcript

BoardRequestMediator could just be folded into GameMode? No having it be a UObject that can potentially be assigned would allow us to alter the possible requests that are valid. Better yet, we should have all requests be strategy types or interfaces that can be added or removed depending on level config?

BoardRules is a mess of a class. All the interface rules should probably just live independently on the Mediator rather than trying to encapsulate them into some class?

UConnectIt_LevelConfigDataAsset is alreayd sort of doing this by separating the rule interfaces rather than asking for a board rules object. This feels better.

Need to make rules for 'IsTilePlaceable' (right now the logic is hard coded to if no other piece is there. But we might have new reasons a tile cannot be placed even if no other piece is there)
Need to brainstorm other required rules, but for now it would be ideal to just ensure the setup is solid so that adding another rule on the config and wiring it into execution logic is straightforward.

Config Rules are using the incorrect MustImplement Setup. Should be like this...
UPROPERTY(EditAnywhere, Instanced, Category=Mover, meta=(MustImplement="/Script/Mover.TurnGeneratorInterface"))  
TObjectPtr<UObject> TurnGenerator;

TSubClass<UObject> - also works

https://unreal-garden.com/docs/uparam/#mustimplement

Basically where would we check that actions get given or taken away from a player? What are all the game scenarios where this might happen. One could be if a player scores a line using a tile that is marked as holding a skill, the player gets a single use of the that skill. How do we handle that during the request update? It would fall under 'BoardRules' in the custom ApplyScoring we would have to check. That could be easy if we just make that the way the game is played, but we are trying to be flexible and prototype just in case the core game loop can be improved upon. We are still not sure about how player acquire other actions (scoring on tiles with markers like scrabble, getting to use a special skill every X points achieved, rolling a random die on every other turn?), whether they are permanent or have uses or must be used right away along side placing a piece. There are some blocking design decisions at play here.

We could create different game modes to accommodate these alternate play styles

Originally we had worked a bit with a player action config. This is really rigid and I want to think of ways to improve this. What are the core things we need?

What actions are allowed for this level?
What actions do the players start with?

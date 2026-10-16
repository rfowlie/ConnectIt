
- I am imagining a situation much like super smash bros, where we have a level that is designed for a specific adventure mode, but then we want to allow for players to use in multiplayer with custom rules. How do we go about supporting that?
- We will want to broadcast what checkpoint win condition has completed so that the same object can act differently for different check points reached. 
	- Not sure we should use tags though, it should really just be an index. We will want a way to check the state of win conditions for all players (int func(playerId));
- 

## Summary of your AI notes

### What you want

- **MinMax as a tool, not a hidden strategy.** Designers should be able to call the search from their own BP strategies with parameters.
- **A designer-facing pipeline.** They set up or reuse a strategy, set it on the controller, call `BeginDecision`, and the move flows through the normal checks to the server.
- **Mix-and-match strategies.** Strategies should be generic, parameterizable containers of behaviour. The controller coordinates them and swapping them is free.

### The flow you described

- **Pipeline:** `AIController` owns a `Strategy`. `BeginDecision(Context)` leads to `FinishDecision(Decision)`, which fires `OnDecisionFinished`. The controller’s `HandleDecisionFinished` runs its checks and calls `SubmitDecision`.
- **Proposed change:** make the checks in `HandleDecisionFinished` BP-overridable. You called it “templated”, but what you describe is a hook such as `HandleDecisionFinishedInternal` returning a bool, “should this decision be submitted?”. Its default would return true. This is a virtual or `BlueprintNativeEvent`, not a template.

### Concerns about loadouts

- **Where validation happens.** From BP it looks like you can create a move and send it with no loadout involved. In the code, the controller’s `SubmitDecision` only looks up which action produces the request type. The real check is the server’s Mediator gate, which rejects a move whose action isn’t in the player state’s loadout.
- **Who owns the loadout.** Should the controller own the loadout instead of the config? The case you raise is an AI that swaps loadouts every turn. Today the loadout lives on the PlayerState. `SetLoadout` can swap it at the next turn start. The controller reads it fresh on each decision.
- **Strategies and moves.** Do strategies discover moves from the loadout, or assume a fixed move set and decide accordingly? Should a strategy have to declare the moves it supports, so unavailable actions can be skipped up front?

### Strategy and controller roles

- **Strategies only return decisions.** The controller sends them.
- **Unchecked sends.** A controller could skip strategies and send a random move that isn’t in the loadout. The server would block it, which is confusing.
- **MinMax guards itself.** It limits its choices to its own loadout, so tampering isn’t a concern.

### MinMax specifically

- It currently reads move availability straight from the player state’s loadout through the decision context.
- **Parameterize it.** Pass the allowed move set in, so a designer can change it on the fly. Your example is giving the AI new moves for a comeback when the player is close to winning.

### Open questions

1. Is the strategy the whole AI behaviour, or a swappable part under a coordinating controller? You’re leaning toward the second.
2. Should the controller or the config own the loadout?
3. Should strategies declare their supported moves?
4. Should the controller be able to skip strategies at all?
5. How should MinMax take its move set and search settings as parameters?


## AI structure: organized notes

### Proposed architecture

- **Director**
    - It listens to game events and responds to them. Expect one subclass per adventure level, since each level has custom events to react to.
    - It decides three things: the **loadout**, the **strategies and how they’re initialized**, and the **decision selector**. It re-decides on game events, including level start.
    - It initializes and parameterizes the strategies, then hands them to the coordinator.
- **Coordinator**
    - It is not a strategy, but it has the same interface.
    - It is generic: it takes an array of strategies and runs them all, collecting every decision.
    - After the think time, it queries each strategy for its cached decisions. It then runs a passed-in selector and tries to submit the selected decisions to the server.
- **Strategy**
    - Strategies **cache their own decisions** so the coordinator can pull them when the think time is up.
    - Start with an **empty strategy that takes a delegate for `BeginDecision`**. Once patterns repeat across levels, encapsulate them as subclasses.
- **Selector**
    - A function the director passes in. It either receives the collected decisions, or receives the strategies and queries how many decisions each should contribute.
    - It returns an **array of decisions** that the coordinator submits.

### Consequences

- **AI profile** might be deprecated. The director now picks the loadout and strategies that the profile used to bundle.
- **Win conditions and checkpoints** get restructured. A director may or may not depend on them.
- **Win conditions** would move into **their own data assets**, so several systems (rules, director, BP scripting) can reference the same one.
- The cost is some editor-matching concerns, since multiple systems need to know the specifics of a win-condition asset. You expect this to be mostly avoidable.

### Open questions

1. **Where director events come from.**
    - Option A: win-condition and checkpoint events live in their own data asset, so scripting in BP shows exactly which events exist.
    - Option B: the director responds to generic subsystem events.
2. **Delegates vs. many strategies.**
    - Option A: one generic strategy plus delegates the director sets. This avoids many near-duplicate strategies.
    - Option B: encapsulated variants, which are easier to set in directors.
    - Your current lean is delegates first, with subclasses added as patterns emerge.
3. **Strategy responsibilities.** Should “run decisions” and “select a decision” be separated, or are they naturally intertwined?
4. **Selector shape.** Does it receive decisions, or receive strategies and query them?

### How this relates to what exists

- Today the controller owns one duplicated strategy from an AI profile, and the strategy decides _and_ picks one move.
- The controller submits one decision per call.
- These notes replace that with director, coordinator, strategies and selector. The coordinator submits several decisions, and the profile goes away.
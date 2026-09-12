# Jonas Tyroller — Synthesis Wiki

_Person slug: `jonas-tyroller` · Built from 6 raw sources · Last updated: 2026-09-05_
_Raw material: [_knowledge/raw/jonas-tyroller/](../raw/jonas-tyroller/)_
_Board context: [board-of-advisors.md](../../board-of-advisors.md) · [advisor-links.md](../../board-of-advisors-links.md)_

> Source 2 (`02-make-your-indie-game-go-viral.md`) is a podcast Jonas **hosts** with guest **Gavin Eisenbis** (Two Star Games). Most of its content is Gavin's virality/retention philosophy — attribute those ideas to Gavin. Jonas's own positions there come through in his questions, pushback, and asides, and are flagged below.

## Who he is (as it bears on the advice)

German indie developer, half of the two-man studio **Grizzly Games** with his partner **Paul**. Shipped **_Islanders_** (minimalist city-builder), **_Will You Snail?_** (solo platformer), and **_Thronefall_** (minimalist medieval base-defense / light-strategy hybrid — ~3 years, Early Access then 1.0 on Steam + Switch). Runs a YouTube channel (~200k subs) that teaches *how to think about game design* rather than tool tutorials, plus a game-dev podcast.

His authority for a solo dev pivoting into programming/design who wants a tight-loop turn-based strategy game ([career-coach.md](career-coach.md)): he ships small, legible strategy games on a tiny team, and has articulated an unusually complete method for **how to search for a good game without over-scoping or over-polishing** — the exact failure modes in the coaching doc.

---

## Core ideas

### 1. Design is a search algorithm
Making a game is running a search over an infinite, multi-dimensional space for the best possible game. **How you search — the order you try things, when, and what you experiment with — determines success**, more than raw effort. The teaching metaphor: you're a boat on a lake looking for the deepest spot; you can **move**, **measure** (playtest / prototype), or **teleport** to an already-explored position. The "gigabrain" third captain explores *all directions first* via teleport, then narrows from the two most promising points.

### 2. Go wide first, narrow later (speed vs. accuracy)
A search is either fast or accurate, never both. For indie, the reasonable shape is: **exploration phase** (slow, accurate, all directions) → **pre-production** (still searching, smaller radius) → **production** (sacrifice accuracy for speed — you have to finish). But **never stop searching completely**; keep doing occasional measurements/playtests so you can still correct course. Thronefall's exploration: "a crapload of ideas" from multiplayer climbing games to golfing flowers, before Paul's little-king prototype won.

### 3. Local minimum — dare bigger jumps
The illusion of having found the deepest spot: every small move is worse, so you conclude you're done — while a far deeper trench sits just past the hill. **You cannot tell when you're in one** (it feels like you're optimizing well), so deliberately give yourself the chance to break out even when things feel good. Take **big jumps when they're cheap**: new game modes (all systems already exist, just change the objective), full rebalances. Fortnite: base-building tower defense → a different game mode → one of the most-played games ever. "You might have a Fortnite slumbering right around the corner and not even know about it."

### 4. Search an infinite space by guessing from your database
You have to guess a direction, but not blindly. Your **database** = every existing game (you roughly know if it succeeded or failed) + your own past projects. Safest move: pick a direction that's worked before, then **search *around* those data points** so you don't just rebuild a game that exists. **"Nurture your database"** — play other games, read their reviews, gather data; don't do all the exploring yourself.

### 5. The Innovation Scale (not "unique selling point")
"USPs are overrated" — the framing pushes people toward too much, or pointless, innovation. Reframe as positioning on an **innovation scale**:
- **Too little** → "parking your ship on top of another ship" — a game that already exists.
- **Too much** → "a unique game for an alien species," artsy-fartsy, nobody's interested.
- **Sweet spot** → near other successful boats, with plenty of space between you.

### 6. Decide what you're optimizing for
Joy vs. portfolio vs. commercial success are *different games*. "What gets measured gets improved." Most devs optimize **revenue** — defensible because in indie, revenue is a good proxy for quality (better game → more people enjoy it → more revenue → win-win with players). Whatever the goal, split it into smaller derivative goals.

### 7. The revenue formula: Fun + Appeal + Scope — from day one
- **Fun** keeps players.
- **Appeal** acquires players.
- **Scope** makes sure you can finish.
Optimize all three **from the beginning, not as an afterthought.** ("I'm a bit of a formula lover.")

### 8. Fun = Flow
Csikszentmihalyi's flow: the channel between boredom and frustration where difficulty and skill are balanced. Applies to every testable skill — comprehension, reaction time, strategy, puzzle-solving — and even **story pacing** (spell everything out = boring; too complex = frustrating). Also cites the **Octalysis framework** (Yu-kai Chou) for motivation. Fun is well-covered and **easy to measure** — just playtest. "If you want to improve the fun, you have to measure the fun."

### 9. Appeal = (Presentation + Fantasy) × Readability — "a Jonas original formula"
- **Presentation**: art, sound, VFX, polish, **juiciness**. Achievable via low-poly (Islanders) or pure juice (One Finger Death Punch), not just AAA fidelity (Baldur's Gate).
- **Fantasy**: is it a role-play people actually *crave* but can't do in real life (too dangerous / impractical)? Superflight (wingsuit), PowerWash Simulator, Euro Truck Simulator — real-life fantasies grounded in reality, with the **negatives stripped out** (no angry customers, no bookkeeping). Childhood fantasies: king, pirate, theme-park builder, hero. Not all fantasies are equal — **pick carefully and live up to it** (immersion). Abstract games have no fantasy, so presentation must carry the whole load.
- **Readability**: can someone understand the game — the fantasy *and* the gameplay — from **one screenshot or a few seconds** of footage? It's the bridge between presentation, fantasy and gameplay. Resembling a known game can *help* readability even if the comparison stings. Thronefall's weakness: people read it as a classic RTS.

### 10. Scope = return per investment
Not revenue — **revenue ÷ investment**. Two equal pools of fish → go to the closer one; the far one might make you run out of fuel before you get there. **"Keep it small, keep it simple, keep it tiny."** Tiny games do extremely well in the indie arena. Quality over quantity.

### 11. Noisy measurements — become a scientist
One playtester (especially yourself) = clouded judgment; even many testers carry noise. **Measure twice where it matters. Let time pass, let the feeling cool down — "the poop might float away."**

### 12. Minimize the cost of exploration so you can explore *more*
Lack of exploration is far more expensive than exploration. The cheapest form is **prototyping** — a tiny fast scouting boat, not your big slow main boat. **Do NOT write clean, production-ready code in a prototype** — you only care about going fast and getting a roughly-correct measurement. Take shortcuts.

### 13. Prototype whatever is fastest
Rule of thumb. If the thing depends on lots of existing systems, prototype it **right into your game** — don't spin up a fresh Unity project to try one idea.

### 14. Prototype art and gameplay separately
Building both at once "is called making the game" — you lose every speed advantage of prototyping. Islanders: a visual-only prototype (move camera, toggle buildings, zero gameplay) let the artist explore fast and free; gameplay was prototyped separately with ugly blocks. **Rule: systems that are separate from each other should be prototyped separately from each other.** Bonus: cleaner measurements, less noise.

### 15. Parallelize scouts; stop over-discussing
**More scouting boats beat bigger scouting boats.** Idle teammate → into a scouting boat immediately. Teams love discussing because "if we talk we don't have to work" — the scout "would have arrived two days ago if they'd just sent it."

### 16. The multiple-captains problem
Two decision-makers each scout, each returns convinced their spot is best → endless emotional argument (sunk-cost fallacy on both sides, "I'm the better captain" subtext). Bad fixes: **"let's do both"** (clutter, scope creep, nobody happy) and **team votes** (don't address the emotion). **Good fix: the captains teleport into each other's positions and continue the other's search.** Removes the emotion (equal investment in each idea) and adds fresh perspective — "you'll be surprised how much of a rational argument people can suddenly have." Also **split final-say by domain** (one owns art decisions, one owns gameplay); crew act as mini-captains searching their own domains. Don't have too many captains — decisions must be fast.

### 17. Red flags that your search is broken
- **You never scrap any work** → you have no search branches; you're following your nose into a poor local minimum. (Will You Snail character controller: one attempt, no search, a 4-year project built on it. Thronefall economy: 1–2 balancing attempts, ended up extremely snowbally in both directions, now too late to fix because players like it.)
- **You constantly scrap work** → commitment issues, decision-making problems, or a flawed database (searching nonsense directions).
- **You scrap work way too late** → same as not exploring, then hitting a dead end. (Thronefall: 2 months on a card-game version before scrapping.)
- **Your game failed** → don't think *"my game failed"* — think *"my search algorithm failed; how do I improve it next time."*

### 18. Playtest embarrassingly early — "Cut your mouth. Don't say anything."
Devs avoid playtesting because they know testers will name the unfinished parts — wrong instinct. Don't defend, don't disclaim, don't explain while they play. Just **watch them get frustrated.** "Your ego can take it." **Play test early, play test often. Fail faster — the faster you fail, the less time you waste.** It's delusional to think your first design is right; playtesting is how it gets better and better.

### 19. Reduce first, then add
Most devs take an inspiration game and pile their own ideas **on top** → guaranteed feature creep (you now owe everything that game has *plus* your additions — impossible for a small team). Instead: **take the inspiration, remove things, and only then add your own.** Thronefall from Stronghold / Age of Empires: buildings only on select fields, the game decides which building goes where, few units, no mouse unit-control (units follow your character). That reduction *created room* for the perk system, upgrade system, enemy variety, campaign. Islanders from city-builders: one resource, buildings are free → room for procgen islands and many building types.

### 20. Find your "5 seconds of fun" before adding anything
The minimal repeatable core. Only once you have it do you start adding on top.

### 21. "Adding is very dangerous" — stop using "add more" as the universal fix
"Not interesting enough" / "not clear enough" / "this section is too short" — if your answer to every problem is to add content, you get feature creep. **Find other solutions.**

### 22. Constraints are a design asset
Thronefall's minimalist art was **necessity + embracing constraints** — a two-man team can't make complex RTS art; doing so would take 5× longer and wouldn't sell better. Graphics aren't the selling point, so most resources shouldn't go there. "Graphics aren't important, so make something you can do very quickly that still looks okay" — done tastefully (color palette, contrast). Method: Unity Asset Store toon shader + simple low-poly Blender shapes; **90% asset bashing** for levels (a few simple shapes clattered everywhere).

### 23. Every level (every design decision) is an experiment
"As a game designer every level you build is more or less an experiment." Expect some to fail; the point is the measurement.

### 24. "Sometimes doing nothing is actually best" — click undo
The villager-wandering saga and the enemy-tent saga: multiple solutions that **looked fine at first glance** but were distracting or killed the coziness once you lived with them → undo. Recurring line: *"at first glance this seems like a good solution, but when you spend more time with it..."* First impressions lie; you have to sit with a change.

### 25. Fantasy-first level/content design
The "wind" level failed because "it didn't feel cool from a fantasy perspective" → response was to build "an absolute Fortress that feels cool and epic to defend." Later he deliberately broke his own level-design rules (a graveyard in a forest, not an epic castle) once confident the *fantasy* would still land. **Memorable beats generic** — a memorable graveyard that also foreshadows the boss.

### 26. "If you can't build a big thing, start with building a small thing"
Stuck on the angle for a jungle level → started with one tiny island → unblocked the creative barrier, and it flowed from there. Ended up one of the best layouts in the game.

### 27. The "massive red tail" of every feature
A Thronefall map is built in ~2 days but takes ~a month to finish: polish, new enemies/buildings/weapon, testing, balancing, localization, out-of-bounds tracing, level-select integration, extra-mode integration. **Small tasks have a tendency to really add up.**

### 28. Nail it on one instance before scaling
He wanted to keep building level 2, but "it made more sense to nail the gameplay on **one** map before we try to do it on two." (The friends-and-family playtest confirmed this was the right call.)

### 29. Kill your darlings via co-ownership
Jonas loved the golfing-flowers prototype; he handed it to Paul, who hit hard roadblocks. "This was not my baby anymore, this was our baby." They talked it out and agreed it wouldn't work. Handing your darling to your partner converts an emotional attachment into a shared, testable decision.

### 30. YouTube / devlogs are a multi-year play or a hobby — not short-term marketing
After 5 years and ~200k subs the channel makes **~$300/month** — "not a lot compared to how much time I put in." Doing it full-time is "freaking stressful and sucks a lot of the joy out of it." Treat it as a hobby. (In the Gavin talk: if you *do* devlogs, treat it as a 5-year business decision; the short-term wish-list return usually isn't there. Thronefall's devlog series did drive ~half of that game's wishlists, but that's the exception, tied to a consistent genre/audience.)

### 31. Take real breaks; game dev isn't everything
After Thronefall he deliberately slowed both YouTube and game dev — BJJ ("the PvP version of bouldering"), swimming, moving in with his partner, guinea pigs — and was openly "a bit confused where to go from here." "Game Dev is not everything by any means. Remember that things take time."

### 32. Epistemic humility — "we have three data points"
He repeatedly discounts his own certainty: each developer has only a handful of shipped games, so strong universal claims are suspect. "Different things work for different people — it's not like everybody should make games how I make games."

---

## Vocabulary & catchphrases

| Term | Meaning |
|---|---|
| **Design is a search algorithm** | The core frame — you're searching an infinite space for the best possible game |
| **The boat on the lake / deepest spot** | The search metaphor: move, measure, teleport |
| **Scouting boat vs. main fishing boat** | A quick scrappy prototype vs. the actual game build |
| **Gigabrain captain / big brain cap** | Someone who explores widely first, then narrows |
| **Go wide first, narrow later** | The recommended indie search shape |
| **Local minimum** | The false sense you've found the best design; escape via big cheap jumps |
| **"A Fortnite slumbering right around the corner"** | A huge improvement one big jump away that you can't see |
| **Your database / nurture your database** | Existing games + your past projects, used to guess directions; feed it by playing and reading reviews |
| **The Innovation Scale** | Position between "parking on another ship" (too little) and "a game for an alien species" (too much) |
| **Fun, Appeal, Scope** | The revenue formula — keep players / get players / finish the game |
| **Appeal = (Presentation + Fantasy) × Readability** | "A Jonas original formula" |
| **Readability** | Understanding the game from one screenshot / a few seconds |
| **Fantasy** | A crave-able real-life role-play, with the boring/negative parts removed |
| **Juiciness** | Tactile feedback and polish in presentation |
| **Keep it small, keep it simple, keep it tiny** | Scope mantra |
| **Return per investment** | The real target — not raw revenue; "go to the closer pool of fish" |
| **Noisy measurements / measure twice / let the feeling cool down** | Distrust single playtests and first impressions |
| **"Minimize the cost of exploration so you can explore more, not less"** | Why you prototype scrappily |
| **"That's called making the game"** | What you're doing wrong if you prototype art and gameplay together |
| **"Whatever is fastest, that's what you do"** | Prototyping rule of thumb |
| **The multiple-captains problem / use the teleport ability, switch places** | How to break a co-founder deadlock without "doing both" or voting |
| **"Your search algorithm failed, not your game"** | Reframe of a commercial failure |
| **"Cut your mouth. Don't say anything."** | How to run a playtest |
| **Fail faster / play test early, play test often** | The faster you fail, the less time you waste |
| **Reduce first, then add** | Take the inspiration, remove things, *then* add your own |
| **5 seconds of fun / the repeatable thing** | The minimal core you must find before adding anything |
| **"Adding is very dangerous"** | "Add more stuff" is not a valid fix for every problem |
| **Embracing our constraints / necessity** | Small-team limitations treated as design direction |
| **Asset bashing** | Building levels from a few simple repeated shapes |
| **"Every level is an experiment"** | Expect failures; the point is the measurement |
| **"Sometimes doing nothing is actually best" / click undo** | The right move is often to revert |
| **"At first glance it seems good, but when you spend more time with it..."** | First impressions lie |
| **"If you can't build a big thing, start with a small thing"** | Unblock by shrinking the task |
| **Memorable, not generic** | The bar for content |
| **The massive red tail** | The long pile of finishing work behind every feature/map |
| **Capture lightning in a bottle** | The uncertainty that you can repeat a success |
| **"We have three data points"** | His standing disclaimer about sample size |
| **"Paul and I" / two-man team** | Grizzly Games; Paul is his Thronefall partner (mostly art/economy) |

---

## Stances (where he plants a flag)

- **A failed game is a failed *search process*** — debrief the process, not the game.
- **Explore widely before committing; keep a small residual search running forever.**
- **Deliberately break out of local minima even when things feel fine**; take cheap big jumps (game modes, full rebalances).
- **Innovate a moderate amount** — near proven successes, not on top of them, not in "alien" territory.
- **Be explicit about your goal** (joy / portfolio / money); they are different games.
- **For commercial success, optimize Fun + Appeal + Scope from day one**, not as an afterthought.
- **Appeal is as important as fun and badly under-discussed; readability is non-negotiable.**
- **Pick a fantasy people actually crave; strip out its real-life negatives.**
- **Keep scope tiny** — tiny games sell; think return-per-investment.
- **Prototype scrappily** — clean code in a prototype is a category error.
- **Prototype art and gameplay separately** (and separate systems separately).
- **More small scouts beat fewer big ones; stop over-discussing and send the scout.**
- **Break co-founder deadlocks by swapping sides**, never by "doing both" or voting.
- **Playtest embarrassingly early, stay silent, don't defend, fail faster.**
- **Reduce your inspiration before adding your own ideas.**
- **"Add more content" is not a valid fix for every problem.**
- **Measure twice; distrust single playtests and first impressions; let feelings cool.**
- **Sometimes the correct move is to undo and do nothing.**
- **Nail a mechanic on one instance before scaling it.**
- **Constraints (a tiny team) are a design asset, not just a limitation.**
- **YouTube/devlogs are a multi-year play or a hobby — not short-term marketing.**
- **Take real breaks; game dev isn't everything; be humble about your tiny sample size.**
- **Different processes work for different people.**

---

## Recurring stories (his canon)

- **The boat on the lake / three captains** — his central teaching metaphor for search.
- **Thronefall's exploration phase** — "a crapload of ideas": multiplayer climbing games, golfing flowers; Paul's little-king "defend your kingdom at night" prototype won and "felt like something very solid from the get-go."
- **The golfing-flowers prototype** he was in love with → handed to Paul → roadblocks → "not my baby, our baby" → killed after talking it out.
- **Thronefall almost became a turn-based card game** — ~2 months, "pretty horrible," tunnel vision, scrapped, restarted from scratch. One of his personal low points; "can we pull this off again, was this a fluke?"
- **Will You Snail character controller** — one attempt, zero search, a 4-year project built on top. "That is crazy."
- **Thronefall economy** — only 1–2 balancing attempts; extremely snowbally in both directions; too late to fix because players like it. "I wish I'd run more experiments early."
- **Fortnite** — base-building tower defense → Battle Royale game mode → one of the most-played games in history (the local-minimum / big-jump example).
- **Islanders** — visual-only prototype (camera + toggle buildings); gameplay prototyped separately with ugly blocks; **Steam-algorithm / impulse-buy success without much creator coverage**; reduced city-builders to one free resource, then added procgen islands and many building types.
- **Superflight / PowerWash Simulator / Euro Truck Simulator** — his standard "fantasy" examples (crave-able real-life role-play, negatives removed).
- **"A Difficult Game About Climbing" (Pontypants) / Snakebird / One Finger Death Punch / Baldur's Gate** — his readability & presentation examples.
- **Thronefall's art style** — necessity, two-man team, Unity Asset Store toon shader + simple Blender low-poly; Paul did most of the art.
- **The villager-wandering saga** and **the enemy-tents saga** — solutions that looked good at first glance, felt distracting once lived with, "click undo," "sometimes doing nothing is best."
- **The "wind" level failing on fantasy** → building an epic fortress instead.
- **"If you can't build a big thing, start small"** — the tiny-island unblock on the jungle level.
- **Traces of a fallen civilization** — the level-design lesson learned only at level 5 (Sturmklamm); applied to every level after and "always worked very well."
- **The Switch mini-map** built lightweight-first → "the most optimized piece of code we have in the PC version."
- **Accidentally wiped his hard drive**, lost devlog footage, lost track of the development day count.
- **Launch-day texture**: the duck he spotted; the guinea pigs (Lina, Lea, Lu); Brazilian jiu-jitsu ("the PvP version of bouldering"); office still in his dad's house; "I cry when I'm happy, not when I'm sad"; a gift left on his desk at launch.
- **Thronefall Early Access launch** — day 151 of dev when nobody had played it; top-10 most-played demo in a Steam Next Fest; more wishlists than all his previous games combined; "motivating beyond belief" turning straight into "panic"; hybrid AI + paid-human localization; feedback "coming in faster than I can read," sorted by priority with colour-coded task ownership.
- **YouTube: ~200k subs, ~$300/month after 5 years** → decided to treat it as a hobby.
- **"We have three data points"** — his standing epistemic disclaimer, delivered to Gavin.
- **Side research projects** — coding neural networks from scratch without training them, "not the first person to try this but I found it fascinating."

---

## How this applies to the flagship project & pivot

_(Bridge back to [career-coach.md](career-coach.md); see also [[tim-cain]] and [[tim-ruswick]].)_

- **"Design is a search algorithm" (#1–4, #17)** reframes the coaching doc's disorganization/perfectionism loop: the problem isn't that the project is messy, it's that there's **no search** — no prototypes scrapped, one economy/controller attempt built on for years. The white-box demo *is* a measurement; run more of them.
- **Reduce first, then add (#19–21)** and **"5 seconds of fun" (#20)** are the direct method for the Go/Connect-4 board game: take the tabletop inspiration, strip it, prove the minimal repeatable loop is fun, *then* add. "Add more stuff" is explicitly not allowed as the fix for "not interesting enough."
- **Prototype art and gameplay separately (#14)** endorses the white-box (no-art) plan — and warns that the moment art and gameplay merge, "that's called making the game."
- **Prototype scrappily / don't write clean code (#12)** is a useful counter-weight to the coaching doc's "every line must be a reusable foundation" reflex — at the search stage, that instinct is the bug. (Contrast [[tim-cain]], who wants *shipped* systems modular; both agree the prototype itself should be quick and dirty.)
- **Scope = return per investment, keep it tiny (#10, #22)** and **constraints as an asset**: a solo dev's limits point straight at the minimalist strategy game he already makes.
- **Fantasy-first (#9, #25)**: even an abstract board game needs presentation to carry it, and a Fire Emblem-like needs a fantasy people crave — pick it deliberately.
- **Playtest embarrassingly early, cut your mouth (#18)** and **noisy measurements (#11, #24)**: the demo's job is a fun-test with silent observation and a cooling-off period, not a portfolio reveal.
- **The multiple-captains problem (#16)** and **kill darlings via co-ownership (#29)** become relevant the moment collaborators join after the loop works.
- **YouTube is a multi-year play or a hobby (#30)** and **take real breaks / "we have three data points" (#31–32)** temper the "prove them wrong" drive and the motivation-cliff pattern: humility about sample size, and permission to rest.

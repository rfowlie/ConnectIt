# Reusable Workflows

Five design patterns extracted from ConnectIt's networked pipeline, written up as portable, project-agnostic patterns rather than ConnectIt-specific how-tos. Each links back to a `Source/ConnectIt/Workflows/*.txt` file (where one exists) as the ConnectIt-specific implementation reference — the `.md` files here are the "teach this pattern to someone building a different game" layer; the `.txt` files stay in place as ground truth for this project's exact implementation.

| Workflow | Problem It Solves | Source `.txt` |
|---|---|---|
| [Server-Authoritative Action Request](ServerAuthoritative-ActionRequest.md) | Client-side intent (input, UI, targeting) needs to trigger a server-only mutation without one hand-written RPC per mutation type, and without re-deriving the same validation checks in each one. | — (synthesized; see [Known Issues](../README.md#known-issues) for the related validation-gap bug) |
| [Single-Source-of-Truth Replication](SingleSourceOfTruth-Replication.md) | Piecemeal replication and server-only delegate broadcasts let client and server state drift, or silently never reach clients at all. | — (synthesized; see `GameEventSubsystem_Workflow.txt`'s "WHY IT EXISTS" section for the bug this fixed) |
| [Gated Event Sequencing via Tags](GameplayTag-EventSequencing.md) | Multiple systems reacting to the same event fire all at once instead of in order, and some reactions take real time that later steps need to actually wait for. Turn-end itself is now just another instance of this pattern -- see `GameEventSubsystem_Workflow.txt`'s "TURN SYSTEM NOW WAITS ON THIS" section. | `GameEventSubsystem_Workflow.txt` |
| [Dual-Mode Subsystem Discovery](SubsystemDiscovery-DualAccessPattern.md) | Callers need a reference to a per-world singleton actor that may not have spawned yet, and different callers need different failure behavior when it hasn't. | `BoardManagerSubsystem_Workflow.txt` |
| [One Debug Widget Per Tracked Class](DebugWidgets.md) | A single monolithic debug widget's category groupings don't map 1:1 to the delegates backing them, letting one category silently go stale while the widget as a whole still looks alive; its class boundary also doesn't match any real class in the codebase. | — (synthesized; also serves as the class↔widget↔plugin location reference) |

Each workflow doc follows the same shape: **Problem** → **Pattern** → **Participants / Classes Involved** → **Sequence** → **Why It's Reusable** → **Gotchas** → **Source**.

# Actor-Owned Combat Queues

## Status

Accepted

The engine represents delayed combat outcomes as combat events queued on the affected actor rather than in a global tick scheduler. Each actor processes the combat events that existed at the start of its update, decrements all of them, and runs ready events in queue order without blocking later short-delay events behind earlier long-delay events; callbacks may enqueue follow-up events, but those new events wait until the actor's next update. This keeps delayed damage and death tied to the actor state they affect while preserving deterministic per-actor timing.

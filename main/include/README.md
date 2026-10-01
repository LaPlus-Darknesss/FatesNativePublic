# include

No speculative game headers are created in Pass 1.

This is deliberate: the available Paragon schemas and Ghidra types are excellent starting evidence, but committing guessed class layouts before offset/constructor/caller validation would create expensive long-term debt.

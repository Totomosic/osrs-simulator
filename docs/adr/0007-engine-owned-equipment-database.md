# Engine-Owned Equipment Database

The equipment database lives inside the C++ engine and parses a versioned JSON document. This keeps equipment validation, equipment-piece identity, and composition construction authoritative across native and WASM consumers, even though browser-side JSON loading would be simpler for the web DPS calculator.

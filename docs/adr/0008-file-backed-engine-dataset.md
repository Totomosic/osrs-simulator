# File-Backed Engine Dataset

Engine reference data lives in bundled JSON files under `data/`, and `DatabaseService` atomically loads the built-in dataset from document contents rather than embedded C++ string literals. This keeps parsing, identity, and cross-database validation authoritative in the C++ engine while letting native callers read files from disk and web callers fetch static assets before passing their contents into WASM.

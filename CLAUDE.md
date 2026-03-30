# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Syft is a C++ tool for **reactive synthesis from a DFA game**. It takes a DFA (the specification) and an input/output variable partition, then solves a two-player reachability game to determine if the agent has a winning strategy and, if so, extracts it as a Transducer.

## Build

**Dependencies**: CLI11, CMake ≥ 3.5, C++17. CUDD is vendored under `third_party/cudd/`.

```bash
# macOS
brew install cli11

# Ubuntu
apt install libcli11-dev

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Executable: `build/bin/Syftmax`

## Running

```bash
# Check realizability and extract a single winning strategy
./build/bin/Syftmax -d spec.dfa -p spec.part

# Compute maximally permissive strategy instead
./build/bin/Syftmax -d spec.dfa -p spec.part --maxset

# Environment player moves first (default: Agent moves first)
./build/bin/Syftmax -d spec.dfa -p spec.part -e

# Alternative state encodings (may improve BDD performance)
./build/bin/Syftmax -d spec.dfa -p spec.part --fanin
./build/bin/Syftmax -d spec.dfa -p spec.part --fanout
```

## Input File Formats

**`.dfa` file** — the game specification:
```
# Lines starting with # are comments
.vars: X1 X2 Y1 Y2          # alphabet variables in order (inputs + outputs)
.states: 4                   # total state count (optional; inferred if omitted)
.initial: 0                  # initial state (0-indexed)
.accepting: 3                # accepting states, space-separated
.transitions:
# from_state  minterm  to_state
# minterm is a binary string of length |vars|:
#   character at position j = value of vars[j] (0 or 1)
0 0000 1
0 0001 1
0 0010 2
...
```

**`.part` file** — input/output variable partition:
```
.inputs: X1 X2
.outputs: Y1 Y2
```

## Architecture

The synthesis pipeline in `src/Main.cpp`:

1. **Parse** `.dfa` and `.part` files
2. **Build `ExplicitStateDfa`** via `from_explicit_table()` — constructs one CUDD ADD per state using Shannon expansion; the ADD maps an alphabet variable assignment to the successor state index
3. **Convert to `SymbolicStateDfa`** — log-encodes states in `ceil(log2(n))` BDD variables; three strategies: standard, `--fanin`, `--fanout`
4. **Solve game** via `ReachabilityMaxSetSynthesizer::run()` — backward BDD fixpoint
5. **Extract strategy**: `AbstractSingleStrategy()` → `Transducer`, or `AbstractMaxSet()` for maximally permissive strategies

### Key Classes

- **`VarMgr`** (`src/synthesis/header/VarMgr.h`): Central CUDD variable manager shared by everything. Tracks named input/output variables and per-automaton state variables.

- **`ExplicitStateDfa`** (`src/synthesis/header/ExplicitStateDfa.h`): DFA with explicit states and symbolic transitions (one CUDD ADD per state). Entry point: `from_explicit_table()`.

- **`SymbolicStateDfa`** (`src/synthesis/header/SymbolicStateDfa.h`): Fully symbolic DFA with log-encoded states. `product()` combines multiple DFAs.

- **`DfaGameSynthesizer`** (`src/synthesis/header/DfaGameSynthesizer.h`): Abstract base implementing `preimage()` and `synthesize_strategy()` over BDDs.

- **`ReachabilityMaxSetSynthesizer`** (`src/synthesis/header/ReachabilityMaxSetSynthesizer.h`): Concrete solver. `AbstractSingleStrategy()` returns a `Transducer`; `AbstractMaxSet()` returns deferring + non-deferring strategy sets.

- **`Transducer`** (`src/synthesis/header/Transducer.h`): Moore machine representing the winning strategy. Exports to `.dot` via `dump_dot()`.

- **`Quantification`** (`src/synthesis/header/Quantification.h`): Variable quantification variants (`Exists`, `Forall`, `ForallExists`, `ExistsForall`) used in preimage computation.

## CMake Modules

`CMakeModules/Findcudd.cmake` locates CUDD headers and libraries on the system.

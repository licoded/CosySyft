# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Syft is a C++ tool for **reactive synthesis from LTLf (Linear Temporal Logic on Finite traces) specifications**. It converts LTLf formulas to DFAs and solves two-player games to compute maximally permissive winning strategies.

## Build

**Dependencies** (must be installed first): CUDD, MONA, Flex, Bison, Boost, and the `lydia` submodule.

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Executable: `build/bin/Syftmax`

## Running

```bash
# Single strategy synthesis
./build/bin/Syftmax -f <formula.ltlf> -p <partition.part>

# Maximally permissive strategy
./build/bin/Syftmax -f <formula.ltlf> -p <partition.part> --maxset

# State encoding options
./build/bin/Syftmax -f <formula.ltlf> -p <partition.part> --fanin
./build/bin/Syftmax -f <formula.ltlf> -p <partition.part> --fanout
```

**Input formats:**
- `.ltlf` file: single line LTLf formula
- `.part` file: variable partition, e.g.:
  ```
  .inputs: X1 X2
  .outputs: Y1 Y2
  ```

## Architecture

The pipeline in `src/Main.cpp` flows through these stages:

1. **Parse** formula + partition files
2. **Build DFA** via `ExplicitStateDfaMona` (wraps Lydia/MONA)
3. **Convert DFA representations**:
   - `ExplicitStateDfaMona` → `ExplicitStateDfa` (explicit states, symbolic transitions as ADDs)
   - `ExplicitStateDfa` → `SymbolicStateDfa` (log-encoded states using BDDs)
4. **Solve game** via `ReachabilityMaxSetSynthesizer` (backward fixpoint over BDDs)
5. **Extract strategy**: `AbstractSingleStrategy()` → `Transducer`, or `AbstractMaxSet()` for permissive strategies

### Key Classes

- **`VarMgr`** (`src/synthesis/header/VarMgr.h`): Central BDD variable manager. Tracks named input/output variables and per-automaton state variables. Nearly every class holds a shared pointer to it.

- **`SymbolicStateDfa`** (`src/synthesis/header/SymbolicStateDfa.h`): Fully symbolic DFA with log-encoded states. Supports `product()` for combining multiple DFAs. Has three encoding strategies: standard, fanin, fanout.

- **`DfaGameSynthesizer`** (`src/synthesis/header/DfaGameSynthesizer.h`): Abstract base for game solvers. Implements `preimage()` and `synthesize_strategy()` using BDD quantification.

- **`ReachabilityMaxSetSynthesizer`** (`src/synthesis/header/ReachabilityMaxSetSynthesizer.h`): Concrete solver for reachability games. `run()` computes winning region; `AbstractMaxSet()` computes deferring + non-deferring strategy sets.

- **`Transducer`** (`src/synthesis/header/Transducer.h`): Moore machine representing the winning strategy. Can export to `.dot` format.

- **`Quantification`** (`src/synthesis/header/Quantification.h`): Strategy for variable quantification in preimage computation — `Exists`, `Forall`, `ForallExists`, `ExistsForall`.

- **`InputOutputPartition`** (`src/synthesis/header/InputOutputPartition.h`): Reads `.part` files; separates controllable (output) from uncontrollable (input) variables.

## CMake Modules

Custom finders in `CMakeModules/`: `Findcudd.cmake`, `Findmona.cmake`. These locate CUDD and MONA headers/libraries on the system.

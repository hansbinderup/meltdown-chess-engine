# SPSA tuning using OpenBench

This README will not explain how the SPSA tuning works, or why we use it. 
For that please read the WIKI from [OpenBench](https://github.com/AndyGrant/OpenBench/wiki/SPSA-Tuning-Workloads).

## How to use

### First step

Modify the `Makefile` on the test branch to include spsa option:

```diff
openbench:
+	meson setup .openbench --cross-file targets/linux-native.txt --wipe --buildtype=release -Dspsa=true
-	meson setup .openbench --cross-file targets/linux-native.txt --wipe --buildtype=release
	meson compile -C .openbench
	cp .openbench/meltdown-chess-engine $(EXE)
```

It's fine to commit this to your local SPSA branch as it will only be used for SPSA testing anyways.
I'm hoping to add the compilation flag to OpenBench at one point, but for now this should do.

### Second step: 

Compile meltdown locally with SPSA enabled: 
```
./scripts/build.sh --spsa -r
```
(`--spsa` will enable SPSA and `-r` will run the executable when built).

### Third step 

Now run `spsa` from meltdown itself:
```
spsa
fullDepthMove, int, 4.0, 0.0, 12.0, 1.0, 0.002
lmrReductionLimit, int, 3.0, 0.0, 12.0, 1.0, 0.002
rfpReductionLimit, int, 3.0, 0.0, 12.0, 1.0, 0.002
rfpMargin, int, 100.0, 0.0, 150.0, 10.0, 0.002
rfpEvaluationMargin, int, 120.0, 0.0, 150.0, 10.0, 0.002
razorReductionLimit, int, 3.0, 0.0, 12.0, 1.0, 0.002
razorMarginShallow, int, 125.0, 0.0, 250.0, 10.0, 0.002
razorMarginDeep, int, 175.0, 0.0, 250.0, 10.0, 0.002
razorDeepReductionLimit, int, 2.0, 0.0, 12.0, 1.0, 0.002
nullMoveReduction, int, 2.0, 0.0, 12.0, 1.0, 0.002
aspirationWindow, int, 50.0, 10.0, 100.0, 5.0, 0.002
timeManMovesToGo, int, 20.0, 10.0, 40.0, 2.0, 0.002
timeManHardLimit, int, 3.0, 1.0, 5.0, 1.0, 0.002
seePawnValue, int, 100.0, 50.0, 1000.0, 10.0, 0.002
seeKnightValue, int, 422.0, 50.0, 1000.0, 10.0, 0.002
seeBishopValue, int, 422.0, 50.0, 1000.0, 10.0, 0.002
seeRookValue, int, 642.0, 50.0, 1000.0, 10.0, 0.002
seeQueenValue, int, 1015.0, 50.0, 1500.0, 10.0, 0.002
```
The output will be the expected SPSA input for OpenBench.

### Fourth step

Start the tuning from OpenBench:

Pick `Create Tune` in the side menu.

Pick `Meltdown` in engine list.

Setup tuning (default should be a fine starting point).

Paste the SPSA input:
![image](https://github.com/user-attachments/assets/b945bc7d-db42-4e77-815f-507de9129ec7)

### Fifth step

Start the tuner and wait :)

The tuning will take a few days so please be patient!

## SPSA parameters

The content in `parameters.h` is grouped to have a single point of entry where we can add tuning parameters.
When a parameter has been added then it will be reflected in a few different places.

All parameters are packed in the `spsa` namespace where they can be found by the engine. Example:
```cpp
if (depth > spsa::nullMoveReduction && !isChecked && m_ply) {
    ...
}
```

### Which parameters belong in the list?

All "magic" constant should go in the list. Often we copy a value from the wiki or another engine. The chances are, that these values have been tuned with the rest of that engine's parameters. Therefore this value might not be as efficient for our engine, as it is for theirs. All these parameters might be differentials and effect each other in good and bad.

### Do I have to tune all parameters at once?

Absolutely not. It _probably_ doesn't make much sense to tune both search, time management, and eg SEE values at the same time.

It's most likely a better approach to tune each feature by itself.

### How to tune partial parameters

Instead of copying the entire SPSA input into OpenBench you can just paste the parameters you want to tune.

You don't have to change Meltdown's parameter list - only chose the parameters you're interested in tuning and copy those to the SPSA input of OpenBench. The rest will stay untouched.

### Example (SPSA enabled)

The list for this example looks like:
```cpp
#define TUNABLE_LIST(TUNABLE)                              \
    TUNABLE(fullDepthMove, uint8_t, 4, 0, 12, 1)           \
    TUNABLE(lmrReductionLimit, uint8_t, 3, 0, 12, 1)       \
    TUNABLE(rfpReductionLimit, uint8_t, 3, 0, 12, 1)       \
    TUNABLE(rfpMargin, Score, 100, 0, 150, 10)             \
    TUNABLE(rfpEvaluationMargin, Score, 120, 0, 150, 10)   \
    TUNABLE(razorReductionLimit, uint8_t, 3, 0, 12, 1)     \
    TUNABLE(razorMarginShallow, Score, 125, 0, 250, 10)    \
    TUNABLE(razorMarginDeep, Score, 175, 0, 250, 10)       \
    TUNABLE(razorDeepReductionLimit, uint8_t, 2, 0, 12, 1) \
    TUNABLE(nullMoveReduction, uint8_t, 2, 0, 12, 1)       \
    TUNABLE(aspirationWindow, uint8_t, 50, 10, 100, 5)     \
    TUNABLE(timeManMovesToGo, uint8_t, 20, 10, 40, 2)      \
    TUNABLE(timeManHardLimit, uint8_t, 3, 1, 5, 1)         \
    TUNABLE(seePawnValue, int32_t, 100, 50, 1000, 10)      \
    TUNABLE(seeKnightValue, int32_t, 422, 50, 1000, 10)    \
    TUNABLE(seeBishopValue, int32_t, 422, 50, 1000, 10)    \
    TUNABLE(seeRookValue, int32_t, 642, 50, 1000, 10)      \
    TUNABLE(seeQueenValue, int32_t, 1015, 50, 1500, 10)
```

We generate SPSA inputs, uci options and mutable parameter types when compiling for SPSA:

UCI options:
```
uci
id name Meltdown 0.0.0-dev
id author Hans Binderup
option name Ponder type check default false
option name Syzygy type string default <empty>
option name Hash type spin default 16 min 1 max 1024
option name Threads type spin default 1 min 1 max 128
option name fullDepthMove type spin default 4 min 0 max 12
option name lmrReductionLimit type spin default 3 min 0 max 12
option name rfpReductionLimit type spin default 3 min 0 max 12
option name rfpMargin type spin default 100 min 0 max 150
option name rfpEvaluationMargin type spin default 120 min 0 max 150
option name razorReductionLimit type spin default 3 min 0 max 12
option name razorMarginShallow type spin default 125 min 0 max 250
option name razorMarginDeep type spin default 175 min 0 max 250
option name razorDeepReductionLimit type spin default 2 min 0 max 12
option name nullMoveReduction type spin default 2 min 0 max 12
option name aspirationWindow type spin default 50 min 10 max 100
option name timeManMovesToGo type spin default 20 min 10 max 40
option name timeManHardLimit type spin default 3 min 1 max 5
option name seePawnValue type spin default 100 min 50 max 1000
option name seeKnightValue type spin default 422 min 50 max 1000
option name seeBishopValue type spin default 422 min 50 max 1000
option name seeRookValue type spin default 642 min 50 max 1000
option name seeQueenValue type spin default 1015 min 50 max 1500
uciok
```

SPSA inputs:
```
spsa
fullDepthMove, int, 4.0, 0.0, 12.0, 1.0, 0.002
lmrReductionLimit, int, 3.0, 0.0, 12.0, 1.0, 0.002
rfpReductionLimit, int, 3.0, 0.0, 12.0, 1.0, 0.002
rfpMargin, int, 100.0, 0.0, 150.0, 10.0, 0.002
rfpEvaluationMargin, int, 120.0, 0.0, 150.0, 10.0, 0.002
razorReductionLimit, int, 3.0, 0.0, 12.0, 1.0, 0.002
razorMarginShallow, int, 125.0, 0.0, 250.0, 10.0, 0.002
razorMarginDeep, int, 175.0, 0.0, 250.0, 10.0, 0.002
razorDeepReductionLimit, int, 2.0, 0.0, 12.0, 1.0, 0.002
nullMoveReduction, int, 2.0, 0.0, 12.0, 1.0, 0.002
aspirationWindow, int, 50.0, 10.0, 100.0, 5.0, 0.002
timeManMovesToGo, int, 20.0, 10.0, 40.0, 2.0, 0.002
timeManHardLimit, int, 3.0, 1.0, 5.0, 1.0, 0.002
seePawnValue, int, 100.0, 50.0, 1000.0, 10.0, 0.002
seeKnightValue, int, 422.0, 50.0, 1000.0, 10.0, 0.002
seeBishopValue, int, 422.0, 50.0, 1000.0, 10.0, 0.002
seeRookValue, int, 642.0, 50.0, 1000.0, 10.0, 0.002
seeQueenValue, int, 1015.0, 50.0, 1500.0, 10.0, 0.002
```

Mutable parameter list (inspecting the macros):
```cpp
// Expands to
static inline uint8_t fullDepthMove = 4;
static inline uint8_t lmrReductionLimit = 3;
static inline uint8_t rfpReductionLimit = 3;
static inline Score rfpMargin = 100;
static inline Score rfpEvaluationMargin = 120;
static inline uint8_t razorReductionLimit = 3;
static inline Score razorMarginShallow = 125;
static inline Score razorMarginDeep = 175;
static inline uint8_t razorDeepReductionLimit = 2;
static inline uint8_t nullMoveReduction = 2;
static inline uint8_t aspirationWindow = 50;
static inline uint8_t timeManMovesToGo = 20;
static inline uint8_t timeManHardLimit = 3;
static inline int32_t seePawnValue = 100;
static inline int32_t seeKnightValue = 422;
static inline int32_t seeBishopValue = 422;
static inline int32_t seeRookValue = 642;
static inline int32_t seeQueenValue = 1015;
```

Note how we now have UCI options that can set these mutable parameters. 
And the SPSA input to provide OpenBench.

### Example (SPSA disabled)

Using same list we can see the following:

UCI options no longer exposed:
```
uci 
id name Meltdown 0.0.0-dev
id author Hans Binderup
option name Ponder type check default false
option name Syzygy type string default <empty>
option name Hash type spin default 16 min 1 max 1024
option name Threads type spin default 1 min 1 max 128
uciok
```

SPSA inputs no longer generated:
```
spsa
Meltdown supports SPSA tuning
The feature is currently disabled
For more details, see: src/spsa/README.md
```

Parameters are now expanded as immutables:
```cpp
// Expands to
constexpr static inline uint8_t fullDepthMove = 4;
constexpr static inline uint8_t lmrReductionLimit = 3;
constexpr static inline uint8_t rfpReductionLimit = 3;
constexpr static inline Score rfpMargin = 100;
constexpr static inline Score rfpEvaluationMargin = 120;
constexpr static inline uint8_t razorReductionLimit = 3;
constexpr static inline Score razorMarginShallow = 125;
constexpr static inline Score razorMarginDeep = 175;
constexpr static inline uint8_t razorDeepReductionLimit = 2;
constexpr static inline uint8_t nullMoveReduction = 2;
constexpr static inline uint8_t aspirationWindow = 50;
constexpr static inline uint8_t timeManMovesToGo = 20;
constexpr static inline uint8_t timeManHardLimit = 3;
constexpr static inline int32_t seePawnValue = 100;
constexpr static inline int32_t seeKnightValue = 422;
constexpr static inline int32_t seeBishopValue = 422;
constexpr static inline int32_t seeRookValue = 642;
constexpr static inline int32_t seeQueenValue = 1015;
```


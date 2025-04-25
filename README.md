# <div align="center">Meltdown</div>

<div align="center">

[![GitHub Release](https://img.shields.io/github/v/release/hansbinderup/meltdown-chess-engine?display_name=tag&style=for-the-badge&label=latest%20release&color=red)](https://github.com/hansbinderup/meltdown-chess-engine/releases/latest)
[![GitHub commits since latest release](https://img.shields.io/github/commits-since/hansbinderup/meltdown-chess-engine/latest?style=for-the-badge&color=orange)](https://github.com/hansbinderup/meltdown-chess-engine/commits/main)
[![GitHub Downloads](https://img.shields.io/github/downloads/hansbinderup/meltdown-chess-engine/total?style=for-the-badge&color=yellow)](https://github.com/hansbinderup/meltdown-chess-engine/releases)

[![Linux](https://img.shields.io/badge/Linux-green?style=for-the-badge&logo=linux&logoColor=black)](#)
[![Windows](https://img.shields.io/badge/Windows-blue?style=for-the-badge&logo=gitforwindows&logoColor=black)](#)

[![Lichess Bot](https://img.shields.io/badge/Lichess-bot-purple?style=for-the-badge&logo=lichess)](https://lichess.org/@/Meltd0wn)

</div>

NOTE: this is a hobby project that is work in progress.

C++26 chess engine.

<details>
<summary>Design goals</summary>

```
#1 Fully stack allocated - engine should at no point allocate heap memory
#2 Fast - well, probably the goal of most chess engines out there
#3 Readable - the code and alogorithms should be easy to read and understand
#4 Modular - the algorithms should be easy to replace with faster, better etc. if needed
```
Comments:

#1: This goes for the engine itself. Can't bother with debug handles, debug logging etc. as these will only run locally, so no need to complicate things here. The engine's search functions, heuristics etc. **must** be stack allocated.

#2 Without compromising #3 and #4 it should be implemented in the most efficient way using the most common algorithms and techniques. 

#3 Starting this project I spend a fair amount of time getting some inspiration. Mostly for algorithms. And I often observed that the implementation of the algorithms were pretty hard to read. I believe that it's possible to implement these fancy and "magic" algorithms in a way that is readable but also don't compromise #2. When it comes to the "magic" algorithms etc. my goal is to separate the logic into method where it's easy(er) to follow what's going on based on the documentation that is currently available.

#4 Each component should, if possible, be decoupled so that it is easy to implement newer and potentially better algorithms if such were to be found.
</details>

## Getting started

The build scripts currently support Linux and Windows. The OS is automatically detected on build.

### Download Meltdown release

The easiest way to install Meltdown is by downloading one of the releases from the [release page](https://github.com/hansbinderup/meltdown-chess-engine/tags).
All binaries are statically compiled so it should be easy to run.

### Compiling through Docker

1. Setup [docker](https://docs.docker.com/get-started/get-docker/)
2. Run docker iteractively: `./docker/run.sh`
3. Run one of the 3 build scripts:
    * Release build: `./scripts/build.sh --release <-r | optional: run the executable>`
    * Development build: `./scripts/build.sh <-r | optional: run the executable>`
    * For debugging in gdb: `./scripts/debug.sh`
    * Compile and run unit-tests: `./scripts/unit_test.sh`

### Compiling without Docker

If you prefer to compile without Docker you must ensure that `meson` (min. v1.1) is installed and your compiler supports cpp23.
You can then run step #3 from above.

## Nightly builds

Every night at 23:30 a nightly build will be triggered by the github actions scheduler.
This build will be untagged (pre-release draft) to not polute actual releases.

A test server will scan for nightly builds and pick them up when they notice that a new one has been built.
The server will then run matchmaking on Lichess to test the engine's current state.

A manual nightly build can also be triggered using the [workflow's dispatch](https://github.com/hansbinderup/meltdown-chess-engine/actions/workflows/nightly.yml).

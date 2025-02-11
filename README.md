# Meltdown Chess Engine

NOTE: this is a hobby project that is work in progress.

C++26 chess engine.

Design goals:

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

## Getting started

1. Init dependencies: `git submodule update --init --recursive`
2. Setup meson by running: `meson setup build`
3. Compile using meson: `meson compile -C build`

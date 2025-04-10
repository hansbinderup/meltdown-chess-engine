# Meltdown Tuner

The tuner is based on the [paper](https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf) published by Andy Grant.

Inspiration has been drawn by [Ethereal tuner](https://github.com/AndyGrant/Ethereal/blob/master/src/tuner.h) (tuner in general) and [Weiss tuner](https://github.com/TerjeKir/weiss/blob/95b0951bcca19e64f6b4dc81e3270f3a67b64f8f/src/tuner/tuner.h) (AdamW specifics).

Special thanks to [Sam Roelants](https://github.com/sroelants) / [Simbelmyne](https://github.com/sroelants/simbelmyne) for giving me reality checks, being a beautiful rubber duck and helping me point at potential issues while implementing the tuner.

## How to run the tuner

The tuner operates on the popular training data set [lichess-big3-resolved](https://archive.org/details/lichess-big3-resolved.7z).

1. Download training data by running `tuner/scripts/download-training-data.sh`
2. Run `tuner/scripts/run.sh`

The tuner will run from a baseline of 0 meaning that an absolute clean tuning will be performed.
This is done to avoid potential pitfalls where the training might get overly fitted / stuck on a certain evaluation parameters that might turn out to be destructive.

The results are automatically written to the file `tuned_terms.h` in the `evaluation/generated/` sub folder.

## How the tuner works

First, please read the [paper](https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf) provided from Andy Grant.

The provided training data contains a fen position and an evaluation for that given position. The evaluation can be:

```
0.0: lost position
0.5: drawn position
1.0: won position
```

### Terms

First we need to understand the concept of `Terms`. A term is a value, or range of values, that will be evaluated based on certain conditions.

The simplest term is probably `pieceValues`. This is a representation of the value of a piece. Traditionally the pieces are evaluated as:

```
pawn: 100
knight: 300
bishop: 300
rook: 500
queen: 900
```
This means that for each of given piece we can evaluate a score with a multiplier from the table above.

Eg: both players have all pieces; except one side is missing a rook: we have a score difference of 500.

## Traces

In terms of tuning, score difference is the important word here. We actually don't care much about how many pawns, etc. each player have. We care about the `difference`.

For a "normal static evaluation" we have to consider each piece, but for our training data we have a snapshot of a position. The position will never change during our entire training. 

For each position (7.2m~ with the given data set) we have 7.2m positions with 100s of terms. Iterating all (480 as we speak) terms for each position would take forever. And it wouldn't give us much information as many of the terms would result in 0s (32 pieces on eg 64 * 6 psqt values) etc.

So to speed up our computations we store a trace of each term. The trace is from White's point of view. So a positive values => in favor of white; negative value => favor of black.

An example could be:

```
White: 4 pawns, 2 bishops, 1 rook, 1 queen, 1 king
Black: 3 pawns, 1 knight, 1 rook, 1 queen, 1 king
```
The resulting trace would be:

```
trace { 1, -1, 2, 0, 0, 0 ... } where { pawns, knights, bishops, rooks, queens, king ...}
```
This list goes on and on with all 100s of terms. Notice the "0" values. This means that the evaluaton of given term is the same for both sides; ie it's not actually present in our position's scoring as they're cancelling eachother out.

## Linear evaluation

Using the trace list from above with a list of coefficients (our tuning) we can actually replicate the static evaluation - but as a simplified linear evaluation.

This linear evaluation will be so much faster to calculate. And remember, the trace does not change for a given position.

Now we have a data representation that can be evaluated fast and in a linear way. Exactly what we needed for computing a gradient descent.

Examples from above: 

```
trace { 1, -1, 2, 0, 0, 0 ... } where { pawns, knights, bishops, rooks, queens, king ...}
pawn: 100, knight: 300, bishop: 300, rook: 500, queen: 900

linearEval = 1 * 100 + -1 * 300 + 2 * 300 = 400
```

The linear evaluation is therefore 400 (in favor of White) which is the same as if we did a static evaluation from the same position (with piece values as our only term (for simplicity)).

## Generating tuning values

When we start the tuning all terms will be set to 0. An error is calculated based on our modified evaluation.
This is done with the help of the `sigmoid` function which maps our score from `[0.0, 1.0]`; for compatibility with the training set.

An error is computed and the gradient is based on that.

Next epoch the values will have changed, based on the above, and we repeat that step millions of times. Each position will provide some useful information about our evaluation and those are stored in the coefficients.

This means that each epoch would improve our evaluation - that is if our terms are actually useful and provide information about the given position. This is the job for us, humans :) (at least when not using NNUE..)

## Tuning parameters

In the file `tuning_parameters.h` some constants have been defined.

The important ones for usage are:

```
k: a constant that is applied to all values - should be adjusted based on the final tuning (if values are too high/low)
lr: the learning rate. This is how aggressive the learning is.
lr step rate: reduction of the learning rate for each given step
lr drop rate: the amount of steps untill lr rate is reduced
```

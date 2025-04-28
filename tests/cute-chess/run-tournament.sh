#!/bin/bash

# Get relative path from caller
SOURCE_DIR=$(dirname $(realpath ${BASH_SOURCE[0]}))

# Load configuration from JSON
CONFIG_FILE="$SOURCE_DIR/config.json"

# Output files
OUTPUT_DIR="$SOURCE_DIR/outputs"
mkdir -p "$OUTPUT_DIR"
FILENAME_SUFFIX="$(date +"%d%m%Y-%H%M%S")"
RESULT_OUTPUT="$OUTPUT_DIR/results_$FILENAME_SUFFIX.txt"
PGN_OUTPUT="$OUTPUT_DIR/games_$FILENAME_SUFFIX.pgn"

# engine settings
ENGINE1_NAME=$(jq -r '.dev_engine.name' "$CONFIG_FILE")
ENGINE1_PATH=$(jq -r '.dev_engine.path' "$CONFIG_FILE")
for option in $(jq -r '.dev_engine.options[]' "$CONFIG_FILE"); do
    ENGINE1_OPTIONS="$ENGINE1_OPTIONS option.$option"
done

ENGINE2_NAME=$(jq -r '.base_engine.name' "$CONFIG_FILE")
ENGINE2_PATH=$(jq -r '.base_engine.path' "$CONFIG_FILE")
for option in $(jq -r '.base_engine.options[]' "$CONFIG_FILE"); do
    ENGINE2_OPTIONS="$ENGINE2_OPTIONS option.$option"
done

# tournament settings
ROUNDS=$(jq -r '.tournament.rounds' "$CONFIG_FILE")
OPENING_BOOK=$(jq -r '.tournament.opening_book' "$CONFIG_FILE")
OPENING_BOOK_MOVES=$(jq -r '.tournament.opening_book_moves' "$CONFIG_FILE")
CONCURRENCY=$(jq -r '.tournament.concurrency' "$CONFIG_FILE")
TIME_CONTROL=$(jq -r '.tournament.time_control' "$CONFIG_FILE")

# resign settings
RESIGN_MOVES=$(jq -r '.tournament.resign_move_count' "$CONFIG_FILE")
RESIGN_SCORE=$(jq -r '.tournament.resign_score' "$CONFIG_FILE")

# draw settings
DRAW_MOVE_NUMBER=$(jq -r '.tournament.draw_move_number' "$CONFIG_FILE")
DRAW_MOVES=$(jq -r '.tournament.draw_move_count' "$CONFIG_FILE")
DRAW_SCORE=$(jq -r '.tournament.draw_score' "$CONFIG_FILE")

# sprt settings
SPRT_STRING=""
SPRT_ENABLED=$(jq -r '.sprt.enabled' "$CONFIG_FILE")
if [[ "$SPRT_ENABLED" == "true" ]]; then
    SPRT_ELO1=$(jq -r '.sprt.elo_0_hypothesis' "$CONFIG_FILE")
    SPRT_ELO2=$(jq -r '.sprt.elo_1_hypothesis' "$CONFIG_FILE")
    SPRT_ALPHA=$(jq -r '.sprt.alpha' "$CONFIG_FILE")
    SPRT_BETA=$(jq -r '.sprt.beta' "$CONFIG_FILE")

    SPRT_STRING="-event sprt-test -sprt elo0=$SPRT_ELO1 elo1=$SPRT_ELO2 alpha=$SPRT_ALPHA beta=$SPRT_BETA"
fi

echo "Running tournament $ENGINE1_NAME VS $ENGINE2_NAME - SPRT enabled: $SPRT_ENABLED"
echo "Games: $ROUNDS, Opening book: $OPENING_BOOK, Time Control: $TIME_CONTROL"
echo "<ctrl-c> will stop tests early"

# Run the tournament
cutechess-cli \
  -engine cmd="$ENGINE1_PATH" name="$ENGINE1_NAME" $ENGINE1_OPTIONS \
  -engine cmd="$ENGINE2_PATH" name="$ENGINE2_NAME" $ENGINE2_OPTIONS \
  -each proto=uci $TIME_CONTROL \
  -concurrency $CONCURRENCY -rounds $ROUNDS -repeat 2 -games 2 \
  -draw movenumber="$DRAW_MOVE_NUMBER" movecount="$DRAW_MOVES" score="$DRAW_SCORE" \
  -resign movecount=$RESIGN_MOVES score=$RESIGN_SCORE \
  $SPRT_STRING \
  -openings file=$OPENING_BOOK plies=$OPENING_BOOK_MOVES order=random policy=round \
  -ratinginterval $CONCURRENCY -outcomeinterval $CONCURRENCY -pgnout "$PGN_OUTPUT" \
  -resultformat default > $RESULT_OUTPUT

echo "Tournament finished!"
echo "Results saved in $RESULT_OUTPUT"
echo "Games saved in $PGN_OUTPUT"


## Cutechess tournament tests

Copy the `config.json.example` to `config.json` and setup the engines you wanna test against each
other.
The results are printed to files in the `outputs` directory.

To run the tests call `run-tournament.sh`. NOTE: it can be called from any location.

The script will setup a tournament against the two specified engines. It will run random openings from the provided opening book.
Each opening will be played twice (once from each side).
It is possible to add SPRT; both enabling and configuring.

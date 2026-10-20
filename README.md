# RallyState

A tennis match engine in C++17 with a rating system on top.

- **Engine**: a point-by-point state machine for game, set and match scoring
  (deuce, advantage, tiebreaks, best-of-3 and best-of-5).
- **Replay**: reads point-by-point CSVs and checks the engine reaches the
  recorded result on the final point.
- **Ratings**: Glicko-2, checked against the worked example in Glickman's paper.
- **CLI**: replay a points file, rank players, show a rating history or a
  head-to-head record.

## Layout

    include/rallystate/   public headers
    src/                  implementation
    apps/                 command line tool
    tests/                Catch2 tests
    data/                 notes on getting real match data

## Build and test

    mkdir build && cd build
    cmake ..
    make -j4
    ./rallystate_tests

Run one group of tests with a tag, for example `./rallystate_tests "[tiebreak]"`.
Tags: `game`, `tiebreak`, `set`, `match`, `replay`, `csv`, `glicko`, `book`.

## CLI

    ./rallystate_cli replay <points.csv>
    ./rallystate_cli rate <results.csv> [player]
    ./rallystate_cli h2h <results.csv> <playerA> <playerB>

`results.csv` has one `winner,loser` row per match, oldest first.

## Notes

- Each match is its own Glicko-2 rating period, and ratings do not decay for
  inactive players. That keeps the model simple but means RD does not grow
  between matches.
- Serve order is not modelled. Only who won each point matters.

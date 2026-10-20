# RallyState

A tennis match engine in C++17 with a rating system on top.

- **Engine**: a point-by-point state machine for game, set and match scoring
  (deuce, advantage, tiebreaks, best-of-3 and best-of-5). The deciding set
  can use one of four rules: a tiebreak at 6-6 (default), no tiebreak
  (advantage set), a tiebreak at 12-12, or a 10-point tiebreak at 6-6.
- **Replay**: reads point-by-point CSVs and checks the engine reaches the
  recorded result on the final point.
- **Ratings**: Glicko-2, checked against the worked example in Glickman's paper.
- **CLI**: replay a points file, rank players, show a rating history or a
  head-to-head record.

## Validation against real matches

The replay was run on all 49 Grand Slam singles point-by-point files in
Jeff Sackmann's data (2011 to 2024, 10,508 matches). See `data/README.md`
for where the data came from and the full per-file table.

| Check | Matches |
|---|---|
| Matches in the files | 10,508 |
| Engine declares a winner exactly on the last recorded point | 10,099 (96.1%) |
| ... and every set score matches the official ATP/WTA result | 9,759 |
| Mismatches (engine does not finish on the last point) | 409 |
| Mismatches with no cause found in the data | 0 |

Before the final-set rules were added, 9,839 matches replayed correctly.
The other 260 needed the rule that was in force at that event: 193 had an
advantage final set, 2 had a tiebreak at 12-12 and 65 had a 10-point
tiebreak.

All 409 mismatches come from the data, not from the scoring logic:

| Cause | Matches |
|---|---|
| Retirement or walkover (official result says RET or W/O) | 223 |
| Point data stops before the match ended | 104 |
| Point winners contradict the data's own game scores | 80 |
| Extra points recorded after match point | 2 |

Limits of this check:

- 23 of the 10,099 "valid" matches are men's retirements after two or three
  sets. The replay counts them because it tries best-of-3 as well as
  best-of-5. The match file does not say which format was played.
- 31 valid matches end on the last point but have a different set score from
  the official result. The point data is missing or has extra games.
- 286 valid matches could not be matched to an official result by player
  name, so their set scores were not checked.

`tools/crosscheck.py` reruns all of this from the CLI output.

## Layout

    include/rallystate/   public headers
    src/                  implementation
    apps/                 command line tool
    tests/                Catch2 tests
    tools/                validation script for real match data
    data/                 notes on getting real match data

## Build and test

    mkdir build && cd build
    cmake ..
    make -j4
    ./rallystate_tests

Run one group of tests with a tag, for example `./rallystate_tests "[tiebreak]"`.
Tags: `game`, `tiebreak`, `set`, `match`, `replay`, `csv`, `glicko`, `book`,
`finalset`.

## CLI

    ./rallystate_cli replay <points.csv> [--sets]
    ./rallystate_cli rate <results.csv> [player]
    ./rallystate_cli h2h <results.csv> <playerA> <playerB>

`replay` tries each final-set rule and reports which one fitted. With
`--sets` it also prints the set scores the engine produced for every match.

`results.csv` has one `winner,loser` row per match, oldest first.

## Notes

- Each match is its own Glicko-2 rating period, and ratings do not decay for
  inactive players. That keeps the model simple but means RD does not grow
  between matches.
- Serve order is not modelled. Only who won each point matters.
- When more than one final-set rule gives the same result, replay reports the
  first one in this order: tiebreak at 6-6, advantage, tiebreak at 12-12,
  10-point tiebreak.

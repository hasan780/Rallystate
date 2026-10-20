# Data

Real match data is not committed. The replay validation uses Jeff Sackmann's
Grand Slam point-by-point data. The data is licensed CC BY-NC-SA 4.0 and all
credit for it goes to Jeff Sackmann.

## Getting the data

The original repository (github.com/JeffSackmann/tennis_slam_pointbypoint)
was no longer public in October 2026. The results below use this archive,
which says it is a copy of upstream commit `6febb77` (October 2024):

    cd ..
    git clone https://github.com/Aneeshers/tennis-sackmann-archive.git

As a check on the archive, its 34 files for 2011 to 2015 were compared with
an older copy of the original repository (github.com/halepmania/tennis_slam_pointbypoint,
commits by Jeff Sackmann in 2015). All 34 files are byte-for-byte identical.
Files from 2016 on could not be checked this way.

The archive has the Australian Open and French Open up to 2021, and
Wimbledon and the US Open up to 2024 (no Wimbledon in 2020).

## Running the validation

From the project folder, after building into `build/`:

    ./build/rallystate_cli replay ../tennis-sackmann-archive/slam_pointbypoint/2019-wimbledon-points.csv
    python3 tools/crosscheck.py ../tennis-sackmann-archive build/rallystate_cli build/validation

The parser needs the columns `match_id`, `PointNumber` and `PointWinner`
(1 or 2). The real files use exactly these names. Rows whose `PointWinner` is
not 1 or 2 are skipped. These include the two `0X` and `0Y` rows at the start
of each match in newer files.

A match counts as valid when the engine declares a winner exactly on the last
recorded point. `tools/crosscheck.py` then compares the engine's set scores
with the official result in the archive's ATP and WTA results files. It also
looks for the cause of each mismatch in the point data. It writes
`results.md` and `mismatches.csv` (match_id, cause, official score) to the
output folder.

## Final-set rules

The engine used to play a tiebreak at 6-6 in every set. The real events used
these rules in the deciding set:

| Event | Deciding set |
|---|---|
| US Open 2011 to 2021 | tiebreak at 6-6 |
| Australian Open and Wimbledon 2011 to 2018, French Open 2011 to 2021 | no tiebreak (advantage set) |
| Wimbledon 2019 to 2021 | tiebreak at 12-12 |
| Australian Open 2019 to 2021, all four events from 2022 | 10-point tiebreak at 6-6 |

`MatchStateMachine` now takes a `FinalSetRule`. Replay tries the rules in
the order above (tiebreak at 6-6 first) and keeps the first one that ends the
match on the last point. Adding the rules raised the valid count from 9,839
to 10,099 matches. In every recovered match, the rule actually in force at
that event also fits, except one match (2012-usopen-1108) whose point data
contradicts its own set scores.

## Results

Run on all 49 singles points files (October 2026).

- **Valid**: the engine declares a winner exactly on the last point.
- **Same score as official**: valid, and every set score and the winner match
  the ATP/WTA result.
- **Retired or W/O**: the official result ends in RET or W/O, so the match
  never reached a normal end.
- **Data stops early**: by the data's own set scores, nobody has won the match
  when the points end. 95 of these have an official result showing a full
  match. 9 have no official result that could be matched.
- **Data contradicts itself**: the point winners do not add up to the games
  that the data itself records (for example, a game score that goes
  backwards, or a game marked as won by the player who lost the point).
- **Extra points**: points are recorded after the match point.

| File | Matches | Valid | Same score as official | Mismatch | Retired or W/O | Data stops early | Data contradicts itself | Extra points |
|---|---|---|---|---|---|---|---|---|
| 2011-ausopen | 179 | 172 | 167 | 7 | 6 | 0 | 1 | 0 |
| 2011-frenchopen | 194 | 185 | 181 | 9 | 2 | 3 | 4 | 0 |
| 2011-usopen | 161 | 152 | 150 | 9 | 9 | 0 | 0 | 0 |
| 2011-wimbledon | 117 | 114 | 113 | 3 | 3 | 0 | 0 | 0 |
| 2012-ausopen | 167 | 160 | 159 | 7 | 7 | 0 | 0 | 0 |
| 2012-frenchopen | 172 | 164 | 160 | 8 | 2 | 0 | 6 | 0 |
| 2012-usopen | 171 | 166 | 162 | 5 | 3 | 0 | 2 | 0 |
| 2012-wimbledon | 140 | 134 | 133 | 6 | 4 | 2 | 0 | 0 |
| 2013-ausopen | 179 | 176 | 173 | 3 | 2 | 0 | 1 | 0 |
| 2013-frenchopen | 186 | 178 | 175 | 8 | 1 | 2 | 5 | 0 |
| 2013-usopen | 167 | 164 | 160 | 3 | 2 | 1 | 0 | 0 |
| 2013-wimbledon | 146 | 138 | 133 | 8 | 4 | 4 | 0 | 0 |
| 2014-ausopen | 158 | 154 | 148 | 4 | 3 | 0 | 1 | 0 |
| 2014-frenchopen | 254 | 230 | 222 | 24 | 9 | 0 | 15 | 0 |
| 2014-usopen | 187 | 179 | 172 | 8 | 7 | 0 | 1 | 0 |
| 2014-wimbledon | 253 | 250 | 245 | 3 | 1 | 1 | 1 | 0 |
| 2015-ausopen | 209 | 208 | 204 | 1 | 1 | 0 | 0 | 0 |
| 2015-frenchopen | 208 | 196 | 189 | 12 | 2 | 2 | 8 | 0 |
| 2015-usopen | 176 | 167 | 161 | 9 | 7 | 0 | 2 | 0 |
| 2015-wimbledon | 252 | 242 | 234 | 10 | 3 | 7 | 0 | 0 |
| 2016-ausopen | 231 | 221 | 210 | 10 | 5 | 3 | 2 | 0 |
| 2016-frenchopen | 176 | 167 | 160 | 9 | 3 | 5 | 1 | 0 |
| 2016-usopen | 172 | 167 | 156 | 5 | 5 | 0 | 0 | 0 |
| 2016-wimbledon | 126 | 124 | 120 | 2 | 2 | 0 | 0 | 0 |
| 2017-ausopen | 237 | 231 | 202 | 6 | 5 | 0 | 1 | 0 |
| 2017-frenchopen | 242 | 228 | 209 | 14 | 7 | 7 | 0 | 0 |
| 2017-usopen | 165 | 162 | 150 | 3 | 3 | 0 | 0 | 0 |
| 2017-wimbledon | 254 | 229 | 217 | 25 | 10 | 4 | 10 | 1 |
| 2018-ausopen | 253 | 249 | 238 | 4 | 4 | 0 | 0 | 0 |
| 2018-frenchopen | 253 | 248 | 237 | 5 | 3 | 2 | 0 | 0 |
| 2018-usopen | 178 | 169 | 166 | 9 | 7 | 0 | 2 | 0 |
| 2018-wimbledon | 254 | 251 | 243 | 3 | 1 | 0 | 2 | 0 |
| 2019-ausopen | 254 | 231 | 223 | 23 | 9 | 14 | 0 | 0 |
| 2019-frenchopen | 252 | 240 | 232 | 12 | 1 | 11 | 0 | 0 |
| 2019-usopen | 251 | 244 | 238 | 7 | 6 | 0 | 1 | 0 |
| 2019-wimbledon | 254 | 248 | 239 | 6 | 6 | 0 | 0 | 0 |
| 2020-ausopen | 253 | 251 | 241 | 2 | 1 | 1 | 0 | 0 |
| 2020-frenchopen | 253 | 250 | 242 | 3 | 3 | 0 | 0 | 0 |
| 2020-usopen | 248 | 240 | 230 | 8 | 4 | 3 | 1 | 0 |
| 2021-ausopen | 253 | 239 | 231 | 14 | 5 | 9 | 0 | 0 |
| 2021-frenchopen | 251 | 245 | 240 | 6 | 5 | 1 | 0 | 0 |
| 2021-usopen | 253 | 246 | 240 | 7 | 5 | 0 | 2 | 0 |
| 2021-wimbledon | 253 | 239 | 231 | 14 | 5 | 5 | 3 | 1 |
| 2022-usopen | 252 | 228 | 222 | 24 | 6 | 16 | 2 | 0 |
| 2022-wimbledon | 252 | 246 | 245 | 6 | 5 | 0 | 1 | 0 |
| 2023-usopen | 253 | 246 | 238 | 7 | 5 | 1 | 1 | 0 |
| 2023-wimbledon | 254 | 250 | 244 | 4 | 4 | 0 | 0 | 0 |
| 2024-usopen | 253 | 239 | 233 | 14 | 12 | 0 | 2 | 0 |
| 2024-wimbledon | 252 | 242 | 241 | 10 | 8 | 0 | 2 | 0 |
| **Total** | 10508 | 10099 | 9759 | 409 | 223 | 104 | 80 | 2 |

Unexplained mismatches: 0

No mismatch is left without a cause in the data. None of them points to a
scoring bug in the engine.

## Limits

- Format is not in the data, so replay tries best-of-3 and best-of-5. 23 men's
  matches that ended in a retirement after two or three sets are counted as
  valid best-of-3 matches. They are in the Valid column but not in the Same
  score as official column.
- 31 valid matches have a set score that differs from the official result.
  The match still ends on its last point, but the data is missing or has
  extra games.
- 286 valid matches had no official result that could be matched by player
  name, so only the last-point check was done for them.
- Tiebreak points are not part of the score comparison. Some official
  results leave them out (2016 Australian Open women) or list the winner's
  points instead of the loser's (2019 French Open women).

# Data

Real match data is not committed. To run the replay validation, clone
Jeff Sackmann's point-by-point repository next to this project:

    git clone https://github.com/JeffSackmann/tennis_slam_pointbypoint.git

Then run the replay on any `*-points.csv` file:

    ./build/rallystate_cli replay ../tennis_slam_pointbypoint/2019-wimbledon-points.csv

The parser only needs the columns `match_id`, `PointNumber` and `PointWinner`
(1 or 2). A match counts as valid when the engine declares a winner exactly on
the last recorded point.

Known limitation: the engine always plays a tiebreak at 6-6, including in the
final set. Older Wimbledon and Australian Open matches (before 2019) played
the final set to a 2-game margin, so those matches will show up as
mismatches. That is a rules difference, not a scoring bug.

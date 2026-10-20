#pragma once

#include <istream>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "rallystate/match_state_machine.hpp"
#include "rallystate/types.hpp"

namespace rallystate {

// All points of one match, in play order. Player::A is "player 1" in the
// source data and Player::B is "player 2".
struct RecordedMatch {
    std::string match_id;
    std::vector<PointEvent> points;
};

// Outcome of feeding a recorded match through the engine.
struct ReplayResult {
    MatchFormat format = MatchFormat::BestOf3;
    bool match_finished = false;   // the engine declared a winner
    bool finished_on_last_point = false; // ... exactly on the final recorded point
    std::optional<Player> winner;
    int points_played = 0;
    int sets_a = 0;
    int sets_b = 0;
};

// Parses a point-by-point CSV (Sackmann slam point-by-point layout).
// Needs the columns match_id, PointNumber and PointWinner (1 or 2). Other
// columns are ignored. Matches come back in order of first appearance, and
// points within a match are sorted by PointNumber.
std::vector<RecordedMatch> parsePointsCsv(std::istream& in);

// Replays a match under one format.
ReplayResult replayMatch(const RecordedMatch& match, MatchFormat format);

// A recorded match is valid when the engine declares a winner exactly on
// the last recorded point. Best-of-3 and best-of-5 are both tried, since
// the source data does not say which was used; best-of-3 wins ties.
ReplayResult replayMatchInferFormat(const RecordedMatch& match);

// Splits one CSV line, honouring double quotes.
std::vector<std::string> splitCsvLine(const std::string& line);

} // namespace rallystate

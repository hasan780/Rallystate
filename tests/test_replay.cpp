#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include "rallystate/replay.hpp"

using namespace rallystate;

// Builds a recorded match from a string of '1'/'2' point winners.
static RecordedMatch fromString(const std::string& s) {
    RecordedMatch m;
    m.match_id = "test";
    for (char c : s) {
        PointEvent e;
        e.winner = (c == '1') ? Player::A : Player::B;
        m.points.push_back(e);
    }
    return m;
}

static std::string repeat(const std::string& s, int n) {
    std::string out;
    for (int i = 0; i < n; i++) out += s;
    return out;
}

TEST_CASE("splitCsvLine: handles quotes and empty fields", "[replay][csv]") {
    auto f = splitCsvLine("a,\"b,c\",,d");
    REQUIRE(f.size() == 4);
    REQUIRE(f[1] == "b,c");
    REQUIRE(f[2] == "");
    REQUIRE(f[3] == "d");
}

TEST_CASE("parsePointsCsv: groups by match and sorts by point number", "[replay][csv]") {
    std::stringstream in(
        "match_id,SetNo,PointNumber,PointWinner\n"
        "m1,1,2,2\n"
        "m1,1,1,1\n"
        "m2,1,1,2\n"
        "m1,1,3,1\n");
    auto matches = parsePointsCsv(in);
    REQUIRE(matches.size() == 2);
    REQUIRE(matches[0].match_id == "m1");
    REQUIRE(matches[0].points.size() == 3);
    REQUIRE(matches[0].points[0].winner == Player::A); // point 1
    REQUIRE(matches[0].points[1].winner == Player::B); // point 2
    REQUIRE(matches[1].match_id == "m2");
}

TEST_CASE("parsePointsCsv: skips rows without a 1 or 2 winner", "[replay][csv]") {
    std::stringstream in(
        "match_id,PointNumber,PointWinner\n"
        "m1,1,1\n"
        "m1,2,0\n"
        "m1,3,2\n");
    auto matches = parsePointsCsv(in);
    REQUIRE(matches.size() == 1);
    REQUIRE(matches[0].points.size() == 2);
}

TEST_CASE("parsePointsCsv: missing required columns is an error", "[replay][csv]") {
    std::stringstream in("match_id,Other\nm1,1\n");
    REQUIRE_THROWS(parsePointsCsv(in));
}

TEST_CASE("replayMatch: straight-sets best-of-3 ends on the last point", "[replay]") {
    // Player 1 wins 12 straight games = 48 straight points.
    auto m = fromString(repeat("1", 48));
    auto r = replayMatch(m, MatchFormat::BestOf3);
    REQUIRE(r.match_finished);
    REQUIRE(r.finished_on_last_point);
    REQUIRE(r.winner == Player::A);
    REQUIRE(r.sets_a == 2);
    REQUIRE(r.points_played == 48);
}

TEST_CASE("replayMatch: stops feeding points once the match is decided", "[replay]") {
    auto m = fromString(repeat("2", 60)); // more points than needed
    auto r = replayMatch(m, MatchFormat::BestOf3);
    REQUIRE(r.match_finished);
    REQUIRE_FALSE(r.finished_on_last_point);
    REQUIRE(r.points_played == 48);
    REQUIRE(r.winner == Player::B);
}

TEST_CASE("replayMatch: an unfinished match is reported as such", "[replay]") {
    auto m = fromString(repeat("1", 30));
    auto r = replayMatch(m, MatchFormat::BestOf3);
    REQUIRE_FALSE(r.match_finished);
    REQUIRE_FALSE(r.winner.has_value());
}

TEST_CASE("replayMatchInferFormat: picks best-of-5 when best-of-3 ends early", "[replay]") {
    // Player 2 wins the first two sets, player 1 the next three (best-of-5).
    std::string set_p2 = repeat("2", 24);
    std::string set_p1 = repeat("1", 24);
    auto m = fromString(set_p2 + set_p2 + set_p1 + set_p1 + set_p1);
    auto r = replayMatchInferFormat(m);
    REQUIRE(r.format == MatchFormat::BestOf5);
    REQUIRE(r.finished_on_last_point);
    REQUIRE(r.winner == Player::A);
    REQUIRE(r.sets_a == 3);
    REQUIRE(r.sets_b == 2);
}

TEST_CASE("replayMatchInferFormat: keeps best-of-3 when it fits", "[replay]") {
    auto m = fromString(repeat("1", 48));
    auto r = replayMatchInferFormat(m);
    REQUIRE(r.format == MatchFormat::BestOf3);
    REQUIRE(r.finished_on_last_point);
}

TEST_CASE("replayMatch: a tiebreak set replays to the correct winner", "[replay]") {
    // 6-6 by alternating games, then player 2 wins the tiebreak 7-0.
    std::string alt;
    for (int i = 0; i < 6; i++) alt += repeat("1", 4) + repeat("2", 4);
    std::string tb = repeat("2", 7);
    // Player 2 then takes set 2 6-0 to win the match.
    auto m = fromString(alt + tb + repeat("2", 24));
    auto r = replayMatch(m, MatchFormat::BestOf3);
    REQUIRE(r.finished_on_last_point);
    REQUIRE(r.winner == Player::B);
    REQUIRE(r.sets_b == 2);
}

#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <string>
#include "rallystate/match_state_machine.hpp"
#include "rallystate/replay.hpp"

using namespace rallystate;

static void winGame(MatchStateMachine& m, Player p) {
    for (int pt = 0; pt < 4; pt++) m.awardPoint(p);
}

static void winGames(MatchStateMachine& m, Player p, int n) {
    for (int g = 0; g < n; g++) winGame(m, p);
}

// Brings the current set to n-n by alternating games.
static void toLevel(MatchStateMachine& m, int n) {
    for (int i = 0; i < n; i++) {
        winGame(m, Player::A);
        winGame(m, Player::B);
    }
}

// Plays the first sets so the next set is the deciding set (one set each
// in best-of-3, two each in best-of-5).
static void toDecidingSet(MatchStateMachine& m, MatchFormat format) {
    int each = (format == MatchFormat::BestOf3) ? 1 : 2;
    for (int i = 0; i < each; i++) {
        winGames(m, Player::A, 6);
        winGames(m, Player::B, 6);
    }
}

static void points(MatchStateMachine& m, Player p, int n) {
    for (int i = 0; i < n; i++) m.awardPoint(p);
}

static std::string repeat(const std::string& s, int n) {
    std::string out;
    for (int i = 0; i < n; i++) out += s;
    return out;
}

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

TEST_CASE("TiebreakTracker: 10-point target", "[finalset][tiebreak]") {
    TiebreakTracker tb(10);
    REQUIRE(tb.pointsToWin() == 10);
    for (int i = 0; i < 9; i++) REQUIRE_FALSE(tb.awardPoint(Player::A).has_value());
    REQUIRE(tb.awardPoint(Player::A) == Player::A);
    REQUIRE(TiebreakTracker{}.pointsToWin() == 7);
}

TEST_CASE("FinalSetRule: default is a 7-point tiebreak at 6-6 in the deciding set",
          "[finalset]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    REQUIRE(m.finalSetRule() == FinalSetRule::Tiebreak7At6);
    toDecidingSet(m, MatchFormat::BestOf3);
    REQUIRE(m.inDecidingSet());
    toLevel(m, 6);
    REQUIRE(m.currentSetScore().in_tiebreak);
    points(m, Player::B, 7);
    REQUIRE(m.isMatchOver());
    REQUIRE(m.matchWinner() == Player::B);
    REQUIRE(m.completedSets().back().games_b == 7);
}

TEST_CASE("FinalSetRule::Advantage: no tiebreak in the deciding set", "[finalset]") {
    MatchStateMachine m(MatchFormat::BestOf3, FinalSetRule::Advantage);
    toDecidingSet(m, MatchFormat::BestOf3);
    toLevel(m, 6);
    REQUIRE_FALSE(m.currentSetScore().in_tiebreak);
    winGame(m, Player::A);  // 7-6
    REQUIRE_FALSE(m.isMatchOver());
    winGame(m, Player::B);  // 7-7
    winGame(m, Player::A);  // 8-7
    winGame(m, Player::A);  // 9-7
    REQUIRE(m.isMatchOver());
    REQUIRE(m.matchWinner() == Player::A);
    REQUIRE(m.completedSets().back().games_a == 9);
    REQUIRE(m.completedSets().back().games_b == 7);
    REQUIRE_FALSE(m.completedSets().back().in_tiebreak);
}

TEST_CASE("FinalSetRule::Advantage: earlier sets still use a tiebreak", "[finalset]") {
    MatchStateMachine m(MatchFormat::BestOf3, FinalSetRule::Advantage);
    REQUIRE_FALSE(m.inDecidingSet());
    toLevel(m, 6);
    REQUIRE(m.currentSetScore().in_tiebreak);
}

TEST_CASE("FinalSetRule::Advantage: best-of-5 only changes the fifth set", "[finalset]") {
    MatchStateMachine m(MatchFormat::BestOf5, FinalSetRule::Advantage);
    winGames(m, Player::A, 6);
    winGames(m, Player::B, 6);
    winGames(m, Player::A, 6);
    REQUIRE_FALSE(m.inDecidingSet());
    toLevel(m, 6);  // fourth set
    REQUIRE(m.currentSetScore().in_tiebreak);
    points(m, Player::B, 7);  // 2 sets all
    REQUIRE(m.inDecidingSet());
    toLevel(m, 6);
    REQUIRE_FALSE(m.currentSetScore().in_tiebreak);
}

TEST_CASE("FinalSetRule::Tiebreak7At12: tiebreak only at 12-12", "[finalset]") {
    MatchStateMachine m(MatchFormat::BestOf3, FinalSetRule::Tiebreak7At12);
    toDecidingSet(m, MatchFormat::BestOf3);
    toLevel(m, 6);
    REQUIRE_FALSE(m.currentSetScore().in_tiebreak);
    toLevel(m, 6);  // 12-12
    REQUIRE(m.currentSetScore().in_tiebreak);
    points(m, Player::A, 7);
    REQUIRE(m.isMatchOver());
    REQUIRE(m.completedSets().back().games_a == 13);
    REQUIRE(m.completedSets().back().games_b == 12);
}

TEST_CASE("FinalSetRule::Tiebreak10At6: 10-point tiebreak in the deciding set", "[finalset]") {
    MatchStateMachine m(MatchFormat::BestOf3, FinalSetRule::Tiebreak10At6);
    toDecidingSet(m, MatchFormat::BestOf3);
    toLevel(m, 6);
    REQUIRE(m.currentSetScore().in_tiebreak);
    points(m, Player::A, 7);
    REQUIRE_FALSE(m.isMatchOver());  // 7-0 is not enough
    points(m, Player::B, 9);         // 7-9
    points(m, Player::A, 3);         // 10-9, still needs a 2-point lead
    REQUIRE_FALSE(m.isMatchOver());
    points(m, Player::A, 1);         // 11-9
    REQUIRE(m.isMatchOver());
    REQUIRE(m.matchWinner() == Player::A);
    REQUIRE(m.completedSets().back().tiebreak_points_a == 11);
    REQUIRE(m.completedSets().back().tiebreak_points_b == 9);
}

TEST_CASE("FinalSetRule::Tiebreak10At6: earlier tiebreaks are still to 7", "[finalset]") {
    MatchStateMachine m(MatchFormat::BestOf3, FinalSetRule::Tiebreak10At6);
    toLevel(m, 6);
    points(m, Player::A, 7);
    REQUIRE(m.completedSets().size() == 1);
    REQUIRE(m.completedSets().back().tiebreak_points_a == 7);
}

TEST_CASE("replayMatchInferFormat: finds an advantage final set", "[finalset][replay]") {
    std::string set1 = repeat("1", 24);
    std::string set2 = repeat("2", 24);
    std::string alt = repeat(repeat("1", 4) + repeat("2", 4), 6);  // 6-6
    std::string fin = repeat("1", 8);                              // 8-6
    auto m = fromString(set1 + set2 + alt + fin);
    REQUIRE_FALSE(replayMatch(m, MatchFormat::BestOf3).finished_on_last_point);
    auto r = replayMatchInferFormat(m);
    REQUIRE(r.finished_on_last_point);
    REQUIRE(r.final_set_rule == FinalSetRule::Advantage);
    REQUIRE(r.format == MatchFormat::BestOf3);
    REQUIRE(r.winner == Player::A);
    REQUIRE(r.sets.size() == 3);
    REQUIRE(r.sets[0].games_a == 6);
    REQUIRE(r.sets[1].games_b == 6);
    REQUIRE(r.sets[2].games_a == 8);
    REQUIRE(r.sets[2].games_b == 6);
}

TEST_CASE("replayMatchInferFormat: finds a 10-point final-set tiebreak", "[finalset][replay]") {
    std::string set1 = repeat("1", 24);
    std::string set2 = repeat("2", 24);
    std::string alt = repeat(repeat("1", 4) + repeat("2", 4), 6);  // 6-6
    auto m = fromString(set1 + set2 + alt + repeat("2", 10));
    auto r = replayMatchInferFormat(m);
    REQUIRE(r.finished_on_last_point);
    REQUIRE(r.final_set_rule == FinalSetRule::Tiebreak10At6);
    REQUIRE(r.winner == Player::B);
}

TEST_CASE("replayMatchInferFormat: keeps the default rule when it fits", "[finalset][replay]") {
    auto m = fromString(repeat("1", 48));
    auto r = replayMatchInferFormat(m);
    REQUIRE(r.finished_on_last_point);
    REQUIRE(r.final_set_rule == FinalSetRule::Tiebreak7At6);
}

TEST_CASE("parsePointsCsv: real Sackmann header and 0X/0Y rows", "[replay][csv]") {
    // Header and rows in the layout of the real *-points.csv files. Each match
    // starts with two 0X/0Y rows that have PointWinner 0.
    std::stringstream in(
        "match_id,ElapsedTime,SetNo,P1GamesWon,P2GamesWon,SetWinner,GameNo,GameWinner,"
        "PointNumber,PointWinner,PointServer\n"
        "2019-usopen-1101,0:00:00,1,0,0,0,1,0,0X,0,0\n"
        "2019-usopen-1101,0:00:00,1,0,0,0,1,0,0Y,0,0\n"
        "2019-usopen-1101,0:00:20,1,0,0,0,1,0,1,1,1\n"
        "2019-usopen-1101,0:00:45,1,0,0,0,1,0,2,2,1\n");
    auto matches = parsePointsCsv(in);
    REQUIRE(matches.size() == 1);
    REQUIRE(matches[0].match_id == "2019-usopen-1101");
    REQUIRE(matches[0].points.size() == 2);
    REQUIRE(matches[0].points[0].winner == Player::A);
    REQUIRE(matches[0].points[1].winner == Player::B);
}

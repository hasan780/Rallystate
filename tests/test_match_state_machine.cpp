#include <catch2/catch_test_macros.hpp>
#include "rallystate/match_state_machine.hpp"

using namespace rallystate;

// Plays one game to completion via 4 straight points for `p`.
static PointResult winGame(MatchStateMachine& m, Player p) {
    PointResult result;
    for (int pt = 0; pt < 4; pt++) result = m.awardPoint(p);
    return result;
}

static PointResult winGames(MatchStateMachine& m, Player p, int n) {
    PointResult result;
    for (int g = 0; g < n; g++) result = winGame(m, p);
    return result;
}

// Reaches 6-6 by alternating game wins. (Six straight games would end the
// set 6-0 and never reach a tiebreak.)
static void toSixAll(MatchStateMachine& m) {
    for (int i = 0; i < 6; i++) {
        winGame(m, Player::A);
        winGame(m, Player::B);
    }
}

// Plays a tiebreak where `p` wins 7-0. Returns the final point result.
static PointResult winTiebreak(MatchStateMachine& m, Player p) {
    PointResult result;
    for (int i = 0; i < 7; i++) result = m.awardPoint(p);
    return result;
}

TEST_CASE("MatchStateMachine: winning a game increments set game tally", "[match]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    auto result = m.awardPoint(Player::A);
    REQUIRE_FALSE(result.game_won);
    for (int i = 0; i < 3; i++) m.awardPoint(Player::A);
    REQUIRE(m.currentSetScore().games_a == 1);
    REQUIRE(m.currentSetScore().games_b == 0);
}

TEST_CASE("MatchStateMachine: PointResult reports the game winner", "[match]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    auto result = winGame(m, Player::B);
    REQUIRE(result.game_won);
    REQUIRE(result.game_winner == Player::B);
    REQUIRE_FALSE(result.set_won);
}

TEST_CASE("MatchStateMachine: set won 6-4, no tiebreak", "[match][set]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    for (int i = 0; i < 4; i++) {
        winGame(m, Player::A);
        winGame(m, Player::B);
    } // 4-4
    winGame(m, Player::A); // 5-4
    auto result = winGame(m, Player::A); // 6-4
    REQUIRE(result.set_won);
    REQUIRE(result.set_winner == Player::A);
    REQUIRE(m.completedSets().size() == 1);
    REQUIRE(m.completedSets()[0].games_a == 6);
    REQUIRE(m.completedSets()[0].games_b == 4);
    REQUIRE_FALSE(m.completedSets()[0].in_tiebreak);
    REQUIRE(m.currentSetScore().games_a == 0);
}

TEST_CASE("MatchStateMachine: 6-0 set ends immediately", "[match][set]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    auto result = winGames(m, Player::A, 6);
    REQUIRE(result.set_won);
    REQUIRE(m.completedSets()[0].games_a == 6);
    REQUIRE(m.completedSets()[0].games_b == 0);
}

TEST_CASE("MatchStateMachine: 6-5 does not end the set, 7-5 does", "[match][set]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    for (int i = 0; i < 5; i++) {
        winGame(m, Player::A);
        winGame(m, Player::B);
    } // 5-5
    auto r1 = winGame(m, Player::A); // 6-5
    REQUIRE_FALSE(r1.set_won);
    REQUIRE(m.completedSets().empty());
    auto r2 = winGame(m, Player::A); // 7-5
    REQUIRE(r2.set_won);
    REQUIRE(r2.set_winner == Player::A);
    REQUIRE(m.completedSets()[0].games_a == 7);
    REQUIRE(m.completedSets()[0].games_b == 5);
    REQUIRE_FALSE(m.completedSets()[0].in_tiebreak);
}

TEST_CASE("MatchStateMachine: 6-6 starts a tiebreak", "[match][tiebreak]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    toSixAll(m);
    REQUIRE(m.currentSetScore().in_tiebreak);
    REQUIRE(m.currentSetScore().games_a == 6);
    REQUIRE(m.currentSetScore().games_b == 6);
    REQUIRE(m.completedSets().empty());
}

TEST_CASE("MatchStateMachine: tiebreak decides the set 7-6", "[match][tiebreak]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    toSixAll(m);
    auto mid = m.awardPoint(Player::A);
    REQUIRE_FALSE(mid.game_won);
    REQUIRE_FALSE(mid.set_won);
    REQUIRE(m.currentSetScore().tiebreak_points_a == 1);

    PointResult result;
    for (int i = 0; i < 6; i++) result = m.awardPoint(Player::A); // 7-0
    REQUIRE(result.game_won);
    REQUIRE(result.set_won);
    REQUIRE(result.set_winner == Player::A);
    REQUIRE(m.completedSets().size() == 1);
    REQUIRE(m.completedSets()[0].games_a == 7);
    REQUIRE(m.completedSets()[0].games_b == 6);
    REQUIRE(m.completedSets()[0].in_tiebreak);
    REQUIRE(m.completedSets()[0].tiebreak_points_a == 7);
    REQUIRE(m.completedSets()[0].tiebreak_points_b == 0);
}

TEST_CASE("MatchStateMachine: tiebreak requires a 2-point margin", "[match][tiebreak]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    toSixAll(m);
    for (int i = 0; i < 6; i++) {
        m.awardPoint(Player::A);
        m.awardPoint(Player::B);
    } // 6-6 in the tiebreak
    REQUIRE_FALSE(m.awardPoint(Player::B).set_won); // 6-7
    REQUIRE_FALSE(m.awardPoint(Player::A).set_won); // 7-7
    REQUIRE_FALSE(m.awardPoint(Player::B).set_won); // 7-8
    auto result = m.awardPoint(Player::B);          // 7-9
    REQUIRE(result.set_won);
    REQUIRE(result.set_winner == Player::B);
    REQUIRE(m.completedSets()[0].games_b == 7);
    REQUIRE(m.completedSets()[0].games_a == 6);
}

TEST_CASE("MatchStateMachine: best-of-3 match won in straight sets", "[match]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    auto set1 = winGames(m, Player::A, 6);
    REQUIRE(set1.set_won);
    REQUIRE_FALSE(set1.match_won);
    REQUIRE_FALSE(m.isMatchOver());

    auto set2 = winGames(m, Player::A, 6);
    REQUIRE(set2.match_won);
    REQUIRE(set2.match_winner == Player::A);
    REQUIRE(m.isMatchOver());
    REQUIRE(m.matchWinner() == Player::A);
    REQUIRE(m.setsWonBy(Player::A) == 2);
    REQUIRE(m.setsWonBy(Player::B) == 0);
}

TEST_CASE("MatchStateMachine: best-of-3 match decided in 3rd set", "[match]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    winGames(m, Player::A, 6);
    winGames(m, Player::B, 6);
    REQUIRE_FALSE(m.isMatchOver());
    auto result = winGames(m, Player::A, 6);
    REQUIRE(result.match_won);
    REQUIRE(result.match_winner == Player::A);
    REQUIRE(m.setsWonBy(Player::A) == 2);
    REQUIRE(m.setsWonBy(Player::B) == 1);
    REQUIRE(m.completedSets().size() == 3);
}

TEST_CASE("MatchStateMachine: best-of-5 requires 3 sets to win", "[match]") {
    MatchStateMachine m(MatchFormat::BestOf5);
    winGames(m, Player::A, 6);
    winGames(m, Player::A, 6);
    REQUIRE_FALSE(m.isMatchOver());
    auto result = winGames(m, Player::A, 6);
    REQUIRE(result.match_won);
    REQUIRE(m.setsWonBy(Player::A) == 3);
}

TEST_CASE("MatchStateMachine: a match can be won in a tiebreak set", "[match][tiebreak]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    winGames(m, Player::A, 6); // set 1 to A
    toSixAll(m);
    auto result = winTiebreak(m, Player::A);
    REQUIRE(result.set_won);
    REQUIRE(result.match_won);
    REQUIRE(result.match_winner == Player::A);
    REQUIRE(m.isMatchOver());
}

TEST_CASE("MatchStateMachine: no-op after match already decided", "[match]") {
    MatchStateMachine m(MatchFormat::BestOf3);
    winGames(m, Player::A, 6);
    winGames(m, Player::A, 6);
    REQUIRE(m.isMatchOver());

    auto result = m.awardPoint(Player::B);
    REQUIRE_FALSE(result.game_won);
    REQUIRE_FALSE(result.set_won);
    REQUIRE_FALSE(result.match_won);
    REQUIRE(m.setsWonBy(Player::A) == 2);
    REQUIRE(m.setsWonBy(Player::B) == 0);
    REQUIRE(m.matchWinner() == Player::A);
}

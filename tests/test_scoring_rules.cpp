#include <catch2/catch_test_macros.hpp>
#include "rallystate/scoring_rules.hpp"

using namespace rallystate;

// Brings a fresh GameTracker to 3-3 (deuce).
static void toDeuce(GameTracker& g) {
    for (int i = 0; i < 3; i++) {
        g.awardPoint(Player::A);
        g.awardPoint(Player::B);
    }
}

TEST_CASE("GameTracker: standard 4-point win, no deuce", "[game]") {
    GameTracker g;
    REQUIRE_FALSE(g.awardPoint(Player::A).has_value()); // 15-0
    REQUIRE_FALSE(g.awardPoint(Player::A).has_value()); // 30-0
    REQUIRE_FALSE(g.awardPoint(Player::A).has_value()); // 40-0
    auto result = g.awardPoint(Player::A);              // game
    REQUIRE(result.has_value());
    REQUIRE(*result == Player::A);
    REQUIRE(g.isOver());
}

TEST_CASE("GameTracker: scoreFor maps points to scoreboard values", "[game]") {
    GameTracker g;
    REQUIRE(g.scoreFor(Player::A) == GameScore::Love);
    g.awardPoint(Player::A);
    REQUIRE(g.scoreFor(Player::A) == GameScore::Fifteen);
    g.awardPoint(Player::A);
    REQUIRE(g.scoreFor(Player::A) == GameScore::Thirty);
    g.awardPoint(Player::A);
    REQUIRE(g.scoreFor(Player::A) == GameScore::Forty);
    REQUIRE(g.scoreFor(Player::B) == GameScore::Love);
}

TEST_CASE("GameTracker: reaching 40-40 enters deuce, game not over", "[game]") {
    GameTracker g;
    toDeuce(g);
    REQUIRE(g.scoreFor(Player::A) == GameScore::Deuce);
    REQUIRE(g.scoreFor(Player::B) == GameScore::Deuce);
    REQUIRE_FALSE(g.isOver());
}

TEST_CASE("GameTracker: advantage -> win requires a 2-point margin", "[game]") {
    GameTracker g;
    toDeuce(g);
    REQUIRE_FALSE(g.awardPoint(Player::A).has_value()); // 4-3
    REQUIRE(g.scoreFor(Player::A) == GameScore::Advantage);
    REQUIRE_FALSE(g.isOver());
    auto result = g.awardPoint(Player::A); // 5-3, A wins
    REQUIRE(result.has_value());
    REQUIRE(*result == Player::A);
    REQUIRE(g.isOver());
}

TEST_CASE("GameTracker: advantage -> opponent scores -> back to deuce", "[game]") {
    GameTracker g;
    toDeuce(g);
    REQUIRE_FALSE(g.awardPoint(Player::A).has_value()); // 4-3
    REQUIRE(g.scoreFor(Player::A) == GameScore::Advantage);
    REQUIRE_FALSE(g.awardPoint(Player::B).has_value()); // 4-4
    REQUIRE(g.scoreFor(Player::A) == GameScore::Deuce);
    REQUIRE(g.scoreFor(Player::B) == GameScore::Deuce);
    REQUIRE_FALSE(g.isOver());
}

TEST_CASE("GameTracker: long deuce battle still ends with a 2-point margin", "[game]") {
    GameTracker g;
    toDeuce(g);
    for (int i = 0; i < 5; i++) {
        REQUIRE_FALSE(g.awardPoint(Player::A).has_value());
        REQUIRE_FALSE(g.awardPoint(Player::B).has_value());
    }
    REQUIRE_FALSE(g.awardPoint(Player::B).has_value());
    auto result = g.awardPoint(Player::B);
    REQUIRE(result.has_value());
    REQUIRE(*result == Player::B);
}

TEST_CASE("GameTracker: no-op after game already decided", "[game]") {
    GameTracker g;
    for (int i = 0; i < 3; i++) REQUIRE_FALSE(g.awardPoint(Player::A).has_value());
    auto win_result = g.awardPoint(Player::A);
    REQUIRE(win_result.has_value());
    REQUIRE(*win_result == Player::A);
    auto result = g.awardPoint(Player::B);
    REQUIRE(result.has_value());
    REQUIRE(*result == Player::A);
    REQUIRE(g.winner() == Player::A);
    REQUIRE(g.isOver());
}

TEST_CASE("TiebreakTracker: win at 7-0", "[tiebreak]") {
    TiebreakTracker tb;
    for (int i = 0; i < 6; i++) REQUIRE_FALSE(tb.awardPoint(Player::A).has_value());
    auto result = tb.awardPoint(Player::A); // 7-0
    REQUIRE(result.has_value());
    REQUIRE(*result == Player::A);
    REQUIRE(tb.isOver());
    REQUIRE(tb.pointsFor(Player::A) == 7);
    REQUIRE(tb.pointsFor(Player::B) == 0);
}

TEST_CASE("TiebreakTracker: must win by 2 past 6-6", "[tiebreak]") {
    TiebreakTracker tb;
    for (int i = 0; i < 6; i++) {
        tb.awardPoint(Player::A);
        tb.awardPoint(Player::B);
    } // 6-6
    REQUIRE_FALSE(tb.awardPoint(Player::A).has_value()); // 7-6
    REQUIRE_FALSE(tb.awardPoint(Player::B).has_value()); // 7-7
    REQUIRE_FALSE(tb.awardPoint(Player::A).has_value()); // 8-7
    auto result = tb.awardPoint(Player::A);              // 9-7
    REQUIRE(result.has_value());
    REQUIRE(*result == Player::A);
    REQUIRE(tb.isOver());
}

TEST_CASE("TiebreakTracker: 7-5 wins, 7-6 does not", "[tiebreak]") {
    TiebreakTracker tb;
    for (int i = 0; i < 5; i++) {
        tb.awardPoint(Player::A);
        tb.awardPoint(Player::B);
    } // 5-5
    REQUIRE_FALSE(tb.awardPoint(Player::A).has_value()); // 6-5
    REQUIRE_FALSE(tb.awardPoint(Player::B).has_value()); // 6-6
    REQUIRE_FALSE(tb.awardPoint(Player::A).has_value()); // 7-6
    REQUIRE_FALSE(tb.isOver());
}

TEST_CASE("TiebreakTracker: no-op after tiebreak decided", "[tiebreak]") {
    TiebreakTracker tb;
    for (int i = 0; i < 7; i++) tb.awardPoint(Player::A);
    auto result = tb.awardPoint(Player::B);
    REQUIRE(result.has_value());
    REQUIRE(*result == Player::A);
    REQUIRE(tb.pointsFor(Player::B) == 0);
}

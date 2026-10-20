#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include "rallystate/glicko2.hpp"

using namespace rallystate;
using Catch::Approx;

// Worked example from Glickman's Glicko-2 paper: a 1500 player (RD 200,
// vol 0.06) beats a 1400/30 opponent and loses to 1550/100 and 1700/300.
TEST_CASE("Glicko2: matches the published worked example", "[glicko]") {
    Glicko2Rating player{1500.0, 200.0, 0.06};
    std::vector<Glicko2Result> results = {
        {{1400.0, 30.0, 0.06}, 1.0},
        {{1550.0, 100.0, 0.06}, 0.0},
        {{1700.0, 300.0, 0.06}, 0.0},
    };
    auto out = glicko2Update(player, results, 0.5);
    REQUIRE(out.rating == Approx(1464.06).margin(0.05));
    REQUIRE(out.rd == Approx(151.52).margin(0.05));
    REQUIRE(out.vol == Approx(0.05999).margin(0.00005));
}

TEST_CASE("Glicko2: no games only grows the deviation", "[glicko]") {
    Glicko2Rating player{1500.0, 200.0, 0.06};
    auto out = glicko2Update(player, {}, 0.5);
    REQUIRE(out.rating == Approx(1500.0));
    REQUIRE(out.rd > 200.0);
    REQUIRE(out.vol == Approx(0.06));
}

TEST_CASE("Glicko2: a win raises the rating, a loss lowers it", "[glicko]") {
    Glicko2Rating a;
    Glicko2Rating b;
    REQUIRE(glicko2Update(a, {{b, 1.0}}).rating > 1500.0);
    REQUIRE(glicko2Update(a, {{b, 0.0}}).rating < 1500.0);
}

TEST_CASE("Glicko2: beating a stronger opponent gains more", "[glicko]") {
    Glicko2Rating a{1500.0, 100.0, 0.06};
    Glicko2Rating weak{1300.0, 100.0, 0.06};
    Glicko2Rating strong{1700.0, 100.0, 0.06};
    double gain_weak = glicko2Update(a, {{weak, 1.0}}).rating - 1500.0;
    double gain_strong = glicko2Update(a, {{strong, 1.0}}).rating - 1500.0;
    REQUIRE(gain_strong > gain_weak);
}

TEST_CASE("RatingBook: tracks ratings, history and head-to-head", "[glicko][book]") {
    RatingBook book;
    book.record("Alice", "Bob");
    book.record("Alice", "Bob");
    book.record("Bob", "Alice");

    REQUIRE(book.matchesRecorded() == 3);
    REQUIRE(book.has("Alice"));
    REQUIRE_FALSE(book.has("Carol"));
    REQUIRE(book.historyOf("Alice").size() == 3);
    REQUIRE(book.historyOf("Carol").empty());

    auto h = book.headToHead("Alice", "Bob");
    REQUIRE(h.first == 2);
    REQUIRE(h.second == 1);

    REQUIRE(book.ranked().size() == 2);
}

TEST_CASE("RatingBook: ranks an unbeaten player first", "[glicko][book]") {
    RatingBook book;
    book.record("Alice", "Bob");
    book.record("Alice", "Carol");
    book.record("Alice", "Bob");
    book.record("Carol", "Bob");

    auto ranked = book.ranked();
    REQUIRE(ranked.size() == 3);
    REQUIRE(ranked.front().first == "Alice");
    REQUIRE(ranked.back().first == "Bob");
}

TEST_CASE("RatingBook: ratings are zero-sum-ish for a first match", "[glicko][book]") {
    RatingBook book;
    book.record("Alice", "Bob");
    double up = book.ratingOf("Alice").rating - 1500.0;
    double down = 1500.0 - book.ratingOf("Bob").rating;
    REQUIRE(up == Approx(down).margin(0.001));
}

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace rallystate {

// Glicko-2 rating (Glickman, 2013). `rating` and `rd` are on the familiar
// Glicko scale (default 1500 / 350); `vol` is the volatility sigma.
struct Glicko2Rating {
    double rating = 1500.0;
    double rd = 350.0;
    double vol = 0.06;
};

// One game result against a single opponent: score is 1 (win), 0.5 or 0.
struct Glicko2Result {
    Glicko2Rating opponent;
    double score;
};

// Updates `r` over one rating period. With no results only the RD grows.
// `tau` constrains volatility change over time (0.3 to 1.2 is typical).
Glicko2Rating glicko2Update(const Glicko2Rating& r,
                            const std::vector<Glicko2Result>& results,
                            double tau = 0.5);

// A snapshot of one player's rating after a given match.
struct RatingSnapshot {
    int match_index;
    Glicko2Rating rating;
};

// Tracks ratings for many players across a chronological list of matches.
// Simplification: every match is its own rating period, and RD does not
// grow while a player is inactive.
class RatingBook {
public:
    explicit RatingBook(double tau = 0.5) : tau_(tau) {}

    // Records a match result. Both players are updated from their
    // pre-match ratings.
    void record(const std::string& winner, const std::string& loser);

    bool has(const std::string& player) const;
    Glicko2Rating ratingOf(const std::string& player) const;
    const std::vector<RatingSnapshot>& historyOf(const std::string& player) const;

    // {wins by first, wins by second} between two players.
    std::pair<int, int> headToHead(const std::string& a, const std::string& b) const;

    // Players ordered by rating, highest first.
    std::vector<std::pair<std::string, Glicko2Rating>> ranked() const;

    int matchesRecorded() const { return match_count_; }

private:
    double tau_;
    int match_count_ = 0;
    std::map<std::string, Glicko2Rating> ratings_;
    std::map<std::string, std::vector<RatingSnapshot>> history_;
    std::map<std::pair<std::string, std::string>, int> wins_;  // (winner, loser) -> count
};

} // namespace rallystate

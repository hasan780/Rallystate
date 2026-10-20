#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace rallystate {

enum class Player { A, B };

inline Player other(Player p) {
    return p == Player::A ? Player::B : Player::A;
}

// A single point won by a player. This is the atomic unit your engine
// will consume, one at a time, in awardPoint().
//
// point_type is a free-form string for now (e.g. "ace", "unforced_error",
// "winner", "double_fault") so you can extend later without touching
// engine control flow. You won't need it for Phase 1 scoring logic --
// it becomes useful later if you add stats beyond win/loss.
struct PointEvent {
    Player winner;
    std::string point_type;
    std::int64_t timestamp_ms = 0;
};

enum class MatchFormat { BestOf3, BestOf5 };

// How the deciding set (set 3 of best-of-3, set 5 of best-of-5) ends when
// the players are level late in the set. All other sets always use a
// 7-point tiebreak at 6-6.
enum class FinalSetRule {
    Tiebreak7At6,   // 7-point tiebreak at 6-6 (the default)
    Advantage,      // no tiebreak; play on until one player leads by 2 games
    Tiebreak7At12,  // 7-point tiebreak at 12-12
    Tiebreak10At6,  // 10-point tiebreak at 6-6
};

// The "0/15/30/40" style score a player would see on a scoreboard.
// Deuce and Advantage only apply once both players have reached 40 (3
// points) -- see GameTracker::scoreFor for where you'll implement this.
enum class GameScore { Love, Fifteen, Thirty, Forty, Deuce, Advantage, GameOver };

// A completed (or in-progress) set's game tally. `winner` is only set
// once the set is actually decided.
struct SetScore {
    int games_a = 0;
    int games_b = 0;
    bool in_tiebreak = false;
    int tiebreak_points_a = 0;
    int tiebreak_points_b = 0;
    std::optional<Player> winner;
};

} // namespace rallystate

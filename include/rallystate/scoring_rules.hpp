#pragma once

#include <optional>
#include "rallystate/types.hpp"

namespace rallystate {

// Tracks scoring within a SINGLE game: 0/15/30/40, deuce, advantage.
// Deliberately knows nothing about sets or matches -- keep this class
// pure single-game logic so it's trivially testable in isolation.
class GameTracker {
public:
    // Record a point for `winner`. Return the winner of the GAME if this
    // point ended it, otherwise return std::nullopt.
    //
    // Rules to implement:
    //   - Points go 0 -> 15 -> 30 -> 40 -> game, UNLESS both players
    //     reach 40 (i.e. 3 points each), in which case you enter deuce.
    //   - From deuce: winning a point gives you "advantage". If you have
    //     advantage and win the next point, you win the game. If you have
    //     advantage and LOSE the next point, you go back to deuce.
    //   - Equivalently (easier to implement): a player wins the game once
    //     they have at least 4 points AND at least a 2-point lead.
    //   - Once a game is decided, further calls should be a no-op (return
    //     the same winner, don't change state).
    std::optional<Player> awardPoint(Player winner);

    // Return the current scoreboard-style score for player `p`.
    GameScore scoreFor(Player p) const;

    bool isOver() const { return over_; }
    std::optional<Player> winner() const { return winner_; }

private:
    int points_a_ = 0;
    int points_b_ = 0;
    bool over_ = false;
    std::optional<Player> winner_;
};

// Tracks a TIEBREAK game: first to 7 points, win by 2. (Serve order
// alternates every 2 points in real tennis, but you don't need to model
// serve for this project -- just the point/win logic.)
class TiebreakTracker {
public:
    // Same shape as GameTracker::awardPoint, but the win condition is
    // different: first to at least 7 points with at least a 2-point lead.
    std::optional<Player> awardPoint(Player winner);

    int pointsFor(Player p) const;
    bool isOver() const { return over_; }
    std::optional<Player> winner() const { return winner_; }

private:
    int points_a_ = 0;
    int points_b_ = 0;
    bool over_ = false;
    std::optional<Player> winner_;
};

} // namespace rallystate

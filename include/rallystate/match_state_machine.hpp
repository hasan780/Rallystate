#pragma once

#include <optional>
#include <vector>
#include "rallystate/scoring_rules.hpp"
#include "rallystate/types.hpp"

namespace rallystate {

// Result of processing a single point: what changed at each level of the
// hierarchy (game/set/match). Fields are false/nullopt unless that level
// was JUST decided by this exact point -- useful for logging and testing
// "what happened as a result of this point".
struct PointResult {
    bool game_won = false;
    std::optional<Player> game_winner;
    bool set_won = false;
    std::optional<Player> set_winner;
    bool match_won = false;
    std::optional<Player> match_winner;
};

// Orchestrates the full Game -> Set -> Match hierarchy. This is the main
// entry point: feed it one point at a time via awardPoint().
class MatchStateMachine {
public:
    explicit MatchStateMachine(MatchFormat format);

    // Feed a single point into the engine. This is the core entry point;
    // later, MatchReplay will call this once per PointEvent from a
    // recorded match.
    //
    // High-level logic to implement:
    //   1. If the match is already over, no-op (return default PointResult).
    //   2. If a tiebreak is currently active, route the point to the
    //      TiebreakTracker instead of the normal GameTracker. If the
    //      tiebreak decides a winner, that ALSO decides the set (a
    //      tiebreak win is scored as a 7-6 set) -- increment that
    //      player's game count by 1 before finishing the set.
    //   3. Otherwise, route the point to the current GameTracker.
    //      - If the game isn't decided yet, return early.
    //      - If it IS decided, increment the winner's game count and
    //        reset for the next game.
    //   4. After a game is won (non-tiebreak path), check: are both
    //      players at 6 games? If so, start a tiebreak (don't check for
    //      a set winner yet -- the tiebreak itself decides the set).
    //   5. Otherwise check the normal set-win condition: a player wins
    //      the set once they have >= 6 games AND a lead of >= 2 games
    //      (this naturally covers 6-4, 6-3, ..., and 7-5).
    //   6. When a set is won, record it in completed_sets_, reset the
    //      in-progress game/set counters, and check whether the match
    //      is now over (a player has won enough sets: 2 for best-of-3,
    //      3 for best-of-5).
    PointResult awardPoint(Player winner);

    bool isMatchOver() const { return match_over_; }
    std::optional<Player> matchWinner() const { return match_winner_; }

    // Completed sets so far (does not include the in-progress set).
    const std::vector<SetScore>& completedSets() const { return completed_sets_; }

    // How many sets has `p` won so far?
    int setsWonBy(Player p) const;

    // Current in-progress set's game tally (for inspection/debugging/tests).
    SetScore currentSetScore() const;

private:
    MatchFormat format_;
    int sets_needed_to_win_;

    std::vector<SetScore> completed_sets_;

    // Current set state
    int cur_games_a_ = 0;
    int cur_games_b_ = 0;
    GameTracker cur_game_;
    std::optional<TiebreakTracker> cur_tiebreak_;

    bool match_over_ = false;
    std::optional<Player> match_winner_;

    // --- Helpers you'll probably want (implement as you see fit -- feel
    // free to change signatures if a different breakdown makes more sense
    // to you) ---

    // True once both players are at 6 games in the current set.
    bool shouldStartTiebreak() const;

    // Returns the set winner based on cur_games_a_/cur_games_b_, if the
    // normal (non-tiebreak) win condition is met. Returns nullopt if the
    // set isn't decided yet.
    std::optional<Player> checkSetWinner() const;

    // Records `set_winner` as having won the current set, resets
    // in-progress counters, and checks/updates match completion.
    // Fills in result.set_won / result.match_won as appropriate.
    void finishSet(Player set_winner, PointResult& result);
};

} // namespace rallystate

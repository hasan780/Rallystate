#include "rallystate/match_state_machine.hpp"

namespace rallystate {

MatchStateMachine::MatchStateMachine(MatchFormat format, FinalSetRule final_set_rule)
    : format_(format),
      final_set_rule_(final_set_rule),
      sets_needed_to_win_(format == MatchFormat::BestOf3 ? 2 : 3) {}

bool MatchStateMachine::inDecidingSet() const {
    return static_cast<int>(completed_sets_.size()) == 2 * sets_needed_to_win_ - 2;
}

bool MatchStateMachine::shouldStartTiebreak() const {
    int level = 6;
    if (inDecidingSet()) {
        if (final_set_rule_ == FinalSetRule::Advantage) return false;
        if (final_set_rule_ == FinalSetRule::Tiebreak7At12) level = 12;
    }
    return cur_games_a_ == level && cur_games_b_ == level;
}

int MatchStateMachine::tiebreakTarget() const {
    if (inDecidingSet() && final_set_rule_ == FinalSetRule::Tiebreak10At6) return 10;
    return 7;
}

std::optional<Player> MatchStateMachine::checkSetWinner() const {
    if (cur_games_a_ >= 6 && cur_games_a_ >= cur_games_b_ + 2) return Player::A;
    if (cur_games_b_ >= 6 && cur_games_b_ >= cur_games_a_ + 2) return Player::B;
    return std::nullopt;
}

void MatchStateMachine::finishSet(Player set_winner, PointResult& result) {
    result.set_won = true;
    result.set_winner = set_winner;

    SetScore finished;
    finished.games_a = cur_games_a_;
    finished.games_b = cur_games_b_;
    finished.winner = set_winner;
    if (cur_tiebreak_.has_value()) {
        finished.in_tiebreak = true;
        finished.tiebreak_points_a = cur_tiebreak_->pointsFor(Player::A);
        finished.tiebreak_points_b = cur_tiebreak_->pointsFor(Player::B);
    }
    completed_sets_.push_back(finished);

    cur_games_a_ = 0;
    cur_games_b_ = 0;
    cur_game_ = GameTracker{};
    cur_tiebreak_.reset();

    if (setsWonBy(set_winner) >= sets_needed_to_win_) {
        match_over_ = true;
        match_winner_ = set_winner;
        result.match_won = true;
        result.match_winner = set_winner;
    }
}

PointResult MatchStateMachine::awardPoint(Player winner) {
    PointResult result;
    if (match_over_) return result;

    // Tiebreak path: the tiebreak decides the game and the set together.
    if (cur_tiebreak_.has_value()) {
        auto tb_winner = cur_tiebreak_->awardPoint(winner);
        if (!tb_winner.has_value()) return result;

        result.game_won = true;
        result.game_winner = tb_winner;
        if (*tb_winner == Player::A) {
            cur_games_a_++;
        } else {
            cur_games_b_++;
        }
        finishSet(*tb_winner, result);
        return result;
    }

    // Normal game path.
    auto game_winner = cur_game_.awardPoint(winner);
    if (!game_winner.has_value()) return result;

    result.game_won = true;
    result.game_winner = game_winner;
    cur_game_ = GameTracker{};

    if (*game_winner == Player::A) {
        cur_games_a_++;
    } else {
        cur_games_b_++;
    }

    if (shouldStartTiebreak()) {
        cur_tiebreak_ = TiebreakTracker{tiebreakTarget()};
        return result;
    }

    auto set_winner = checkSetWinner();
    if (set_winner.has_value()) {
        finishSet(*set_winner, result);
    }
    return result;
}

int MatchStateMachine::setsWonBy(Player p) const {
    int count = 0;
    for (const auto& s : completed_sets_) {
        if (s.winner == p) count++;
    }
    return count;
}

SetScore MatchStateMachine::currentSetScore() const {
    SetScore s;
    s.games_a = cur_games_a_;
    s.games_b = cur_games_b_;
    s.in_tiebreak = cur_tiebreak_.has_value();
    if (cur_tiebreak_.has_value()) {
        s.tiebreak_points_a = cur_tiebreak_->pointsFor(Player::A);
        s.tiebreak_points_b = cur_tiebreak_->pointsFor(Player::B);
    }
    return s;
}

} // namespace rallystate

#include "rallystate/scoring_rules.hpp"

namespace rallystate {

std::optional<Player> GameTracker::awardPoint(Player winner) {
    if (over_) {
        return winner_;
    }

    if (winner == Player::A) {
        points_a_++;
    } else {
        points_b_++;
    }

    int winner_pts = (winner == Player::A) ? points_a_ : points_b_;
    int opponent_pts = (winner == Player::A) ? points_b_ : points_a_;

    if (winner_pts >= 4 and winner_pts >= (opponent_pts + 2)) {
        over_ = true;
        winner_ = winner;
        return winner_;
    }

    return std::nullopt;
}

GameScore GameTracker::scoreFor(Player p) const {
    int p_pts = (p == Player::A) ? points_a_ : points_b_;
    int opp_pts = (p == Player::A) ? points_b_ : points_a_;

    if (p_pts >= 3 and opp_pts >= 3) {
        if (p_pts >= (opp_pts + 1)) {
            return GameScore::Advantage;
        } else {
            return GameScore::Deuce;
        }
    } else {
        if (p_pts == 0) {
            return GameScore::Love;
        } else if (p_pts == 1) {
            return GameScore::Fifteen;
        } else if (p_pts == 2) {
            return GameScore::Thirty;
        } else {
            return GameScore::Forty;
        }
    }
}

std::optional<Player> TiebreakTracker::awardPoint(Player winner) {
    if (over_) {
        return winner_;
    }

    if (winner == Player::A) {
        points_a_++;
    } else {
        points_b_++;
    }

    int winner_pts = (winner == Player::A) ? points_a_ : points_b_;
    int opponent_pts = (winner == Player::A) ? points_b_ : points_a_;

    if (winner_pts >= 7 and winner_pts >= (opponent_pts + 2)) {
        over_ = true;
        winner_ = winner;
        return winner_;
    }

    return std::nullopt;
}

int TiebreakTracker::pointsFor(Player p) const {
    if (p == Player::A) {
        return points_a_;
    } else {
        return points_b_;
    }
}

} // namespace rallystate

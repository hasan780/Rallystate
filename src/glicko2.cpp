#include "rallystate/glicko2.hpp"

#include <algorithm>
#include <cmath>

namespace rallystate {

namespace {

constexpr double kScale = 173.7178;
constexpr double kPi = 3.14159265358979323846;
constexpr double kEps = 0.000001;

double g(double phi) {
    return 1.0 / std::sqrt(1.0 + 3.0 * phi * phi / (kPi * kPi));
}

double expectedScore(double mu, double mu_j, double phi_j) {
    return 1.0 / (1.0 + std::exp(-g(phi_j) * (mu - mu_j)));
}

} // namespace

Glicko2Rating glicko2Update(const Glicko2Rating& r,
                            const std::vector<Glicko2Result>& results,
                            double tau) {
    const double mu = (r.rating - 1500.0) / kScale;
    const double phi = r.rd / kScale;
    const double sigma = r.vol;

    // No games this period: only the deviation grows.
    if (results.empty()) {
        Glicko2Rating out = r;
        double phi_star = std::sqrt(phi * phi + sigma * sigma);
        out.rd = phi_star * kScale;
        return out;
    }

    // Step 3: estimated variance v.
    double v_inv = 0.0;
    double score_sum = 0.0;
    for (const auto& res : results) {
        double mu_j = (res.opponent.rating - 1500.0) / kScale;
        double phi_j = res.opponent.rd / kScale;
        double e = expectedScore(mu, mu_j, phi_j);
        double gj = g(phi_j);
        v_inv += gj * gj * e * (1.0 - e);
        score_sum += gj * (res.score - e);
    }
    const double v = 1.0 / v_inv;

    // Step 4: estimated improvement delta.
    const double delta = v * score_sum;

    // Step 5: new volatility via the Illinois algorithm.
    const double a = std::log(sigma * sigma);
    auto f = [&](double x) {
        double ex = std::exp(x);
        double num = ex * (delta * delta - phi * phi - v - ex);
        double den = 2.0 * (phi * phi + v + ex) * (phi * phi + v + ex);
        return num / den - (x - a) / (tau * tau);
    };

    double A = a;
    double B;
    if (delta * delta > phi * phi + v) {
        B = std::log(delta * delta - phi * phi - v);
    } else {
        int k = 1;
        while (f(a - k * tau) < 0.0) k++;
        B = a - k * tau;
    }
    double fA = f(A);
    double fB = f(B);
    while (std::fabs(B - A) > kEps) {
        double C = A + (A - B) * fA / (fB - fA);
        double fC = f(C);
        if (fC * fB <= 0.0) {
            A = B;
            fA = fB;
        } else {
            fA /= 2.0;
        }
        B = C;
        fB = fC;
    }
    const double new_sigma = std::exp(A / 2.0);

    // Steps 6 and 7: new deviation and rating.
    const double phi_star = std::sqrt(phi * phi + new_sigma * new_sigma);
    const double new_phi = 1.0 / std::sqrt(1.0 / (phi_star * phi_star) + 1.0 / v);
    const double new_mu = mu + new_phi * new_phi * score_sum;

    Glicko2Rating out;
    out.rating = new_mu * kScale + 1500.0;
    out.rd = new_phi * kScale;
    out.vol = new_sigma;
    return out;
}

void RatingBook::record(const std::string& winner, const std::string& loser) {
    Glicko2Rating w = has(winner) ? ratings_[winner] : Glicko2Rating{};
    Glicko2Rating l = has(loser) ? ratings_[loser] : Glicko2Rating{};

    Glicko2Rating new_w = glicko2Update(w, {{l, 1.0}}, tau_);
    Glicko2Rating new_l = glicko2Update(l, {{w, 0.0}}, tau_);

    ratings_[winner] = new_w;
    ratings_[loser] = new_l;
    history_[winner].push_back({match_count_, new_w});
    history_[loser].push_back({match_count_, new_l});
    wins_[{winner, loser}]++;
    match_count_++;
}

bool RatingBook::has(const std::string& player) const {
    return ratings_.find(player) != ratings_.end();
}

Glicko2Rating RatingBook::ratingOf(const std::string& player) const {
    auto it = ratings_.find(player);
    return it == ratings_.end() ? Glicko2Rating{} : it->second;
}

const std::vector<RatingSnapshot>& RatingBook::historyOf(const std::string& player) const {
    static const std::vector<RatingSnapshot> empty;
    auto it = history_.find(player);
    return it == history_.end() ? empty : it->second;
}

std::pair<int, int> RatingBook::headToHead(const std::string& a, const std::string& b) const {
    int a_wins = 0;
    int b_wins = 0;
    auto ia = wins_.find({a, b});
    if (ia != wins_.end()) a_wins = ia->second;
    auto ib = wins_.find({b, a});
    if (ib != wins_.end()) b_wins = ib->second;
    return {a_wins, b_wins};
}

std::vector<std::pair<std::string, Glicko2Rating>> RatingBook::ranked() const {
    std::vector<std::pair<std::string, Glicko2Rating>> out(ratings_.begin(), ratings_.end());
    std::sort(out.begin(), out.end(), [](const auto& x, const auto& y) {
        return x.second.rating > y.second.rating;
    });
    return out;
}

} // namespace rallystate

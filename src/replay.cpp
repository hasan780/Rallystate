#include "rallystate/replay.hpp"

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace rallystate {

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string cur;
    bool in_quotes = false;
    for (std::size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur += '"';
                    i++;
                } else {
                    in_quotes = false;
                }
            } else {
                cur += c;
            }
        } else if (c == '"') {
            in_quotes = true;
        } else if (c == ',') {
            fields.push_back(cur);
            cur.clear();
        } else if (c != '\r') {
            cur += c;
        }
    }
    fields.push_back(cur);
    return fields;
}

std::vector<RecordedMatch> parsePointsCsv(std::istream& in) {
    std::string line;
    if (!std::getline(in, line)) return {};

    auto header = splitCsvLine(line);
    int id_col = -1, num_col = -1, win_col = -1;
    for (std::size_t i = 0; i < header.size(); i++) {
        if (header[i] == "match_id") id_col = static_cast<int>(i);
        if (header[i] == "PointNumber") num_col = static_cast<int>(i);
        if (header[i] == "PointWinner") win_col = static_cast<int>(i);
    }
    if (id_col < 0 || num_col < 0 || win_col < 0) {
        throw std::runtime_error("points CSV needs match_id, PointNumber and PointWinner columns");
    }

    struct Row {
        long number;
        Player winner;
    };
    std::vector<std::string> order;
    std::map<std::string, std::vector<Row>> rows;
    const int max_col = std::max({id_col, num_col, win_col});

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = splitCsvLine(line);
        if (static_cast<int>(f.size()) <= max_col) continue;
        // Rows without a clear 1 or 2 winner (e.g. unfinished points) are skipped.
        if (f[win_col] != "1" && f[win_col] != "2") continue;

        const std::string& id = f[id_col];
        if (rows.find(id) == rows.end()) order.push_back(id);
        rows[id].push_back({std::strtol(f[num_col].c_str(), nullptr, 10),
                            f[win_col] == "1" ? Player::A : Player::B});
    }

    std::vector<RecordedMatch> out;
    for (const auto& id : order) {
        auto& r = rows[id];
        std::stable_sort(r.begin(), r.end(),
                         [](const Row& x, const Row& y) { return x.number < y.number; });
        RecordedMatch m;
        m.match_id = id;
        for (const auto& row : r) {
            PointEvent e;
            e.winner = row.winner;
            m.points.push_back(e);
        }
        out.push_back(std::move(m));
    }
    return out;
}

ReplayResult replayMatch(const RecordedMatch& match, MatchFormat format,
                         FinalSetRule final_set_rule) {
    ReplayResult res;
    res.format = format;
    res.final_set_rule = final_set_rule;
    MatchStateMachine m(format, final_set_rule);
    const std::size_t total = match.points.size();
    for (std::size_t i = 0; i < total; i++) {
        if (m.isMatchOver()) break;
        auto pr = m.awardPoint(match.points[i].winner);
        res.points_played++;
        if (pr.match_won) {
            res.match_finished = true;
            res.winner = pr.match_winner;
            res.finished_on_last_point = (i + 1 == total);
        }
    }
    res.sets_a = m.setsWonBy(Player::A);
    res.sets_b = m.setsWonBy(Player::B);
    res.sets = m.completedSets();
    return res;
}

ReplayResult replayMatchInferFormat(const RecordedMatch& match) {
    const FinalSetRule rules[] = {FinalSetRule::Tiebreak7At6, FinalSetRule::Advantage,
                                  FinalSetRule::Tiebreak7At12, FinalSetRule::Tiebreak10At6};
    ReplayResult fallback = replayMatch(match, MatchFormat::BestOf3);
    for (FinalSetRule rule : rules) {
        for (MatchFormat format : {MatchFormat::BestOf3, MatchFormat::BestOf5}) {
            ReplayResult r = replayMatch(match, format, rule);
            if (r.finished_on_last_point) return r;
        }
    }
    return fallback;
}

const char* finalSetRuleName(FinalSetRule rule) {
    switch (rule) {
        case FinalSetRule::Tiebreak7At6: return "tiebreak at 6-6";
        case FinalSetRule::Advantage: return "advantage final set";
        case FinalSetRule::Tiebreak7At12: return "tiebreak at 12-12";
        case FinalSetRule::Tiebreak10At6: return "10-point tiebreak at 6-6";
    }
    return "unknown";
}

} // namespace rallystate

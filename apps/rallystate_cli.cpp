#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>

#include "rallystate/glicko2.hpp"
#include "rallystate/replay.hpp"

using namespace rallystate;

static void usage() {
    std::cout <<
        "RallyState CLI\n"
        "  rallystate_cli replay <points.csv>\n"
        "      Replay every match in a point-by-point CSV through the engine\n"
        "      and report how many end exactly on their last recorded point.\n"
        "  rallystate_cli rate <results.csv> [player]\n"
        "      results.csv has one 'winner,loser' row per match, oldest first.\n"
        "      Prints the top 10 ratings, or one player's rating history.\n"
        "  rallystate_cli h2h <results.csv> <playerA> <playerB>\n"
        "      Prints head-to-head wins and both current ratings.\n";
}

static bool loadResults(const std::string& path, RatingBook& book) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "cannot open " << path << "\n";
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        auto f = splitCsvLine(line);
        if (f.size() < 2) continue;
        if (f[0] == "winner" && f[1] == "loser") continue; // header row
        book.record(f[0], f[1]);
    }
    return true;
}

static int cmdReplay(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "cannot open " << path << "\n";
        return 1;
    }
    std::vector<RecordedMatch> matches;
    try {
        matches = parsePointsCsv(in);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    int valid = 0;
    for (const auto& m : matches) {
        ReplayResult r = replayMatchInferFormat(m);
        if (r.finished_on_last_point) {
            valid++;
        } else {
            std::printf("MISMATCH %s: %d of %zu points, finished=%d\n", m.match_id.c_str(),
                        r.points_played, m.points.size(), r.match_finished ? 1 : 0);
        }
    }
    std::printf("%d of %zu matches replayed to a winner on the final point\n", valid,
                matches.size());
    return 0;
}

static int cmdRate(const std::string& path, const std::string& player) {
    RatingBook book;
    if (!loadResults(path, book)) return 1;

    if (player.empty()) {
        auto ranked = book.ranked();
        std::printf("%-4s %-28s %8s %7s\n", "#", "player", "rating", "rd");
        for (std::size_t i = 0; i < ranked.size() && i < 10; i++) {
            std::printf("%-4zu %-28s %8.1f %7.1f\n", i + 1, ranked[i].first.c_str(),
                        ranked[i].second.rating, ranked[i].second.rd);
        }
        return 0;
    }

    if (!book.has(player)) {
        std::cerr << "no matches for " << player << "\n";
        return 1;
    }
    std::printf("%-8s %8s %7s\n", "match", "rating", "rd");
    for (const auto& s : book.historyOf(player)) {
        std::printf("%-8d %8.1f %7.1f\n", s.match_index, s.rating.rating, s.rating.rd);
    }
    return 0;
}

static int cmdH2h(const std::string& path, const std::string& a, const std::string& b) {
    RatingBook book;
    if (!loadResults(path, book)) return 1;
    auto h = book.headToHead(a, b);
    std::printf("%s %d - %d %s\n", a.c_str(), h.first, h.second, b.c_str());
    std::printf("ratings: %s %.1f, %s %.1f\n", a.c_str(), book.ratingOf(a).rating, b.c_str(),
                book.ratingOf(b).rating);
    return 0;
}

int main(int argc, char** argv) {
    if (argc >= 3 && std::string(argv[1]) == "replay") return cmdReplay(argv[2]);
    if (argc >= 3 && std::string(argv[1]) == "rate") {
        return cmdRate(argv[2], argc >= 4 ? argv[3] : "");
    }
    if (argc >= 5 && std::string(argv[1]) == "h2h") return cmdH2h(argv[2], argv[3], argv[4]);
    usage();
    return argc == 1 ? 0 : 1;
}

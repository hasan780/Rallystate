"""Cross-checks rallystate_cli replay results against the real data.

Usage:
    python3 tools/crosscheck.py <archive_root> <rallystate_cli> [out_dir]

<archive_root> holds slam_pointbypoint/, atp/ and wta/ (Jeff Sackmann's data).
For every singles *-points.csv file it runs `rallystate_cli replay <file> --sets`,
then:
  - compares the engine's set scores for each valid match with the official
    score in the ATP/WTA results files, and
  - looks for the cause of each mismatch in the point data itself.
Writes results.md (per-file table) and mismatches.csv to out_dir.
"""
import collections, csv, glob, os, re, subprocess, sys, unicodedata

SLAM_NAME = {'ausopen': 'australian open', 'frenchopen': 'roland garros',
             'wimbledon': 'wimbledon', 'usopen': 'us open'}
CAUSES = ['retirement or walkover',
          'point data stops before the match ended',
          "point winners contradict the data's own game scores",
          'extra points recorded after match point',
          'unexplained']


def best_of(match_id):
    tail = match_id.split('-')[-1]
    if tail.startswith('MS'):
        return 5
    if tail.startswith('WS'):
        return 3
    return 5 if int(tail) < 2000 else 3


def norm(name):
    s = unicodedata.normalize('NFKD', name).encode('ascii', 'ignore').decode().lower()
    return re.sub(r'[^a-z ]', ' ', s.replace('-', ' ')).split()


def name_keys(name):
    t = norm(name)
    return [' '.join(t), t[0][0] + ' ' + t[-1], t[-1]] if t else []


def load_tour(root):
    tour = collections.defaultdict(list)
    files = glob.glob(root + '/atp/atp_matches_20*.csv') + glob.glob(root + '/wta/wta_matches_20*.csv')
    for path in files:
        with open(path, encoding='utf-8', errors='replace', newline='') as f:
            for row in csv.DictReader(f):
                if row.get('tourney_level') != 'G':
                    continue
                slam = next((k for k, v in SLAM_NAME.items() if v == row['tourney_name'].lower()), None)
                if slam:
                    year = int(row['tourney_date'][:4])
                    tour[(year, slam)].append((row['winner_name'], row['loser_name'], row['score'].strip()))
    return tour


def find_tour(tour, year, slam, p1, p2):
    """Returns (1 or 2 for the winner, score) or None."""
    k1, k2 = name_keys(p1), name_keys(p2)
    for level in range(3):
        if len(k1) <= level or len(k2) <= level:
            continue
        hits = []
        for wn, ln, sc in tour.get((year, slam), []):
            kw, kl = name_keys(wn), name_keys(ln)
            if len(kw) <= level or len(kl) <= level:
                continue
            if (kw[level], kl[level]) == (k1[level], k2[level]):
                hits.append((1, sc))
            elif (kw[level], kl[level]) == (k2[level], k1[level]):
                hits.append((2, sc))
        if len(hits) == 1:
            return hits[0]
        if hits:
            return None
    return None


def games_of(score):
    score = re.sub(r'\([^)]*\)', '', score)
    return [(int(a), int(b)) for a, b in re.findall(r'(\d+)-(\d+)', score)]


def tiebreak_losers(sets):
    return [min(int(a), int(b)) for a, b in re.findall(r'\((\d+)-(\d+)\)', sets)]


def load_points(path):
    rows = collections.OrderedDict()
    with open(path, encoding='utf-8', newline='') as f:
        for row in csv.DictReader(f):
            rows.setdefault(row['match_id'], []).append(row)
    return rows


def played_points(rows):
    pts = []
    for row in rows:
        if row['PointWinner'] in ('1', '2'):
            m = re.match(r'\s*(\d+)', row['PointNumber'])
            pts.append((int(m.group(1)) if m else 0, row))
    pts.sort(key=lambda x: x[0])
    return [r for _, r in pts]


def data_consistent(pts):
    """Every game marked by the data's GameWinner flag is a legal game for the
    point winners in it, and the data's game counters move up by one."""
    a = b = 0
    cur_set, games = '1', (0, 0)
    for row in pts:
        if row['PointWinner'] == '1':
            a += 1
        else:
            b += 1
        gw = row['GameWinner']
        if gw not in ('1', '2'):
            continue
        hi, lo = (a, b) if gw == '1' else (b, a)
        if gw != row['PointWinner']:
            return False
        if not any(hi >= t and hi >= lo + 2 and not (hi - 1 >= t and hi - 1 >= lo + 2) for t in (4, 7, 10)):
            return False
        a = b = 0
        if row['SetNo'] != cur_set:
            cur_set, games = row['SetNo'], (0, 0)
        expect = (games[0] + (gw == '1'), games[1] + (gw == '2'))
        if (int(row['P1GamesWon'] or 0), int(row['P2GamesWon'] or 0)) != expect:
            return False
        if row['SetWinner'] in ('1', '2'):
            cur_set, games = str(int(row['SetNo']) + 1), (0, 0)
        else:
            games = expect
    return True


def mismatch_cause(match_id, rows, status, tour_score, cli_line):
    if any(k in tour_score for k in ('RET', 'W/O', 'DEF')) or status == 'Retired':
        return CAUSES[0]
    pts = played_points(rows)
    if not data_consistent(pts):
        return CAUSES[2]
    sets_won = collections.Counter(r['SetWinner'] for r in pts if r['SetWinner'] in ('1', '2'))
    if max(sets_won.values(), default=0) < best_of(match_id) // 2 + 1:
        return CAUSES[1]
    if 'finished=1' in cli_line:
        return CAUSES[3]
    return CAUSES[4]


def main():
    root, cli = sys.argv[1], sys.argv[2]
    out_dir = sys.argv[3] if len(sys.argv) > 3 else '.'
    os.makedirs(out_dir, exist_ok=True)
    tour = load_tour(root)
    table, mismatches = [], []
    total = collections.Counter()
    for path in sorted(glob.glob(root + '/slam_pointbypoint/*-points.csv')):
        name = os.path.basename(path)
        year, slam = int(name[:4]), name.split('-')[1]
        meta = {}
        with open(path.replace('-points.csv', '-matches.csv'), encoding='utf-8', newline='') as f:
            for row in csv.DictReader(f):
                meta[row['match_id']] = row
        out = subprocess.run([cli, 'replay', path, '--sets'], capture_output=True, text=True, check=True).stdout
        miss_lines = {m.group(1): line for line in out.splitlines()
                      for m in [re.match(r'MISMATCH (\S+):', line)] if m}
        points = load_points(path)
        c = collections.Counter()
        for line in out.splitlines():
            if not line.startswith('SETS\t'):
                continue
            _, mid, result, fmt, rule, sets = (line.split('\t') + [''])[:6]
            m = meta.get(mid, {})
            t = find_tour(tour, year, slam, m.get('player1', ''), m.get('player2', ''))
            c['matches'] += 1
            if result == 'valid':
                c['valid'] += 1
                if t is None:
                    c['no official record'] += 1
                    continue
                winner, score = t
                if any(k in score for k in ('RET', 'W/O', 'DEF')):
                    c['official record is a retirement'] += 1
                    continue
                eng = games_of(sets)
                if winner == 2:
                    eng = [(y, x) for x, y in eng]
                eng_winner = 1 if sum(x > y for x, y in games_of(sets)) > sum(y > x for x, y in games_of(sets)) else 2
                if eng_winner == winner and eng == games_of(score):
                    c['same score as official'] += 1
                    if tiebreak_losers(sets) != [int(x) for x in re.findall(r'\((\d+)\)', score)]:
                        c['tiebreak points differ'] += 1
                else:
                    c['score differs from official'] += 1
            else:
                cause = mismatch_cause(mid, points[mid], m.get('status', ''), t[1] if t else '', miss_lines[mid])
                c['mismatch'] += 1
                c[cause] += 1
                mismatches.append((mid, cause, t[1] if t else ''))
        total.update(c)
        table.append((name, c))

    def row(label, c):
        return '| {} | {} | {} | {} | {} | {} | {} | {} | {} |'.format(
            label, c['matches'], c['valid'], c['same score as official'], c['mismatch'],
            c[CAUSES[0]], c[CAUSES[1]], c[CAUSES[2]], c[CAUSES[3]])
    lines = ['| File | Matches | Valid | Same score as official | Mismatch | Retired or W/O '
             '| Data stops early | Data contradicts itself | Extra points |',
             '|---|---|---|---|---|---|---|---|---|']
    lines += [row(n.replace('-points.csv', ''), c) for n, c in table]
    lines.append(row('**Total**', total))
    lines.append('')
    lines.append(f'Unexplained mismatches: {total[CAUSES[4]]}')
    with open(os.path.join(out_dir, 'results.md'), 'w') as f:
        f.write('\n'.join(lines) + '\n')
    with open(os.path.join(out_dir, 'mismatches.csv'), 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['match_id', 'cause', 'official_score'])
        w.writerows(mismatches)
    for k in sorted(total):
        print(f'{k}: {total[k]}')


if __name__ == '__main__':
    main()

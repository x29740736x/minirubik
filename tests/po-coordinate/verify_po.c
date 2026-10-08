/* Host-only verification. All cube operations come from the supplied source. */
#include "po_search.h"
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
static double wall_time(void)
{
    LARGE_INTEGER t, f;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t);
    return (double)t.QuadPart / (double)f.QuadPart;
}
#else
#include <sys/time.h>
static double wall_time(void)
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec + t.tv_usec / 1000000.0;
}
#endif

static uint8_t *load_exact(void)
{
    const unsigned expected[12] = {1,9,54,321,1847,9992,50136,227536,870072,1887748,623800,2644};
    unsigned histogram[12] = {0};
    FILE *f = fopen("exact_distances.bin", "rb");
    if (!f) { fprintf(stderr, "Run build_table first\n"); return NULL; }
    uint8_t *distance = malloc(STATES);
    if (!distance) { fclose(f); return NULL; }
    size_t n = fread(distance, 1, STATES, f);
    int extra = fgetc(f);
    int bad = ferror(f);
    fclose(f);
    if (n != STATES || extra != EOF || bad) { free(distance); return NULL; }
    for (uint32_t r = 0; r < STATES; ++r) {
        if (distance[r] > 11) { free(distance); return NULL; }
        ++histogram[distance[r]];
    }
    if (distance[0] != 0 || memcmp(histogram, expected, sizeof expected)) { free(distance); return NULL; }
    return distance;
}
static int read_number(const char *s, uint32_t *value)
{
    char *end;
    errno = 0;
    unsigned long n = strtoul(s, &end, 10);
    if (errno || !*s || *end || s[0] == '-' || n > STATES) return 0;
    *value = (uint32_t)n;
    return 1;
}

static int save_result(const char *file, const char *status,
                       uint32_t start, uint32_t next, uint32_t end,
                       uint32_t replayed, const char *mode, double seconds)
{
    FILE *f = fopen(file, "w");
    if (!f) return 0;
    int ok = fprintf(f,
        "{\n  \"implementation\": \"P/O coordinate IDA*\",\n"
        "  \"bfs_and_replay_source\": \"same vin_solver.c; not independent\",\n"
        "  \"status\": \"%s\",\n  \"start_rank\": %u,\n"
        "  \"next_rank\": %u,\n  \"end_rank_exclusive\": %u,\n"
        "  \"verified_this_run\": %u,\n"
        "  \"all_state_lengths_verified\": %s,\n"
        "  \"replay_mode\": \"%s\",\n  \"replayed_states\": %u,\n"
        "  \"all_state_paths_replayed\": %s,\n"
        "  \"verification_wall_seconds\": %.6f\n}\n",
        status, (unsigned)start, (unsigned)next, (unsigned)end,
        (unsigned)(next - start),
        start == 0 && next == STATES && !strcmp(status, "PASS") ? "true" : "false",
        mode, (unsigned)replayed,
        replayed == STATES && !strcmp(status, "PASS") ? "true" : "false", seconds) >= 0;
    return fclose(f) == 0 && ok;
}

int main(int argc, char **argv)
{
    uint32_t start = 0, count = STATES;
    const char *file = "po_result.json", *mode = "all";
    if (argc != 1 && argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: verify_po [START COUNT [RESULT_JSON]]\n");
        return 2;
    }
    if (argc >= 3 && (!read_number(argv[1], &start) || !read_number(argv[2], &count)
        || !count || start >= STATES || count > STATES - start)) return 2;
    if (argc >= 4) file = argv[3];

    if (strcmp(mode, "sample") && strcmp(mode, "none") && strcmp(mode, "all")) return 2;
    uint32_t end = start + count, replayed = 0;
    if (!save_result(file, "SETTING_UP", start, start, end, 0, mode, 0)) return 2;
    double setup = wall_time();
    if (!build_po_transitions()) { fprintf(stderr, "Transition-table check failed\n"); return 1; }
    uint8_t *exact = load_exact();
    if (!exact) { fprintf(stderr, "Missing or invalid exact_distances.bin\n"); return 1; }
    uint8_t pd[PERMUTATIONS], od[ORIENTATIONS];
    build_p_distance(pd); build_o_distance(od);
    printf("Loaded complete BFS table and built IDA* heuristic tables; setup %.3f s\n", wall_time() - setup);
    printf("Search range [%u,%u); replay mode=%s\n", (unsigned)start, (unsigned)end, mode);
    fflush(stdout);
    double begin = wall_time();

    for (uint32_t r = start; r < end; ++r) {
        state_t state;
        uint8_t path[11];
        unrank_state(r, &state);
        int length = po_solve(state, pd, od, path);
        const char *reason = NULL;
        if (length != exact[r]) reason = "length differs from own BFS distance";
        if (!reason) {
            if (!po_replay(state, path, length)) reason = "own cubie replay did not solve cube";
            else ++replayed;
        }
        if (reason) {
            fprintf(stderr, "FAIL rank=%u expected=%u found=%d: %s\n", (unsigned)r, exact[r], length, reason);
            save_result(file, "FAIL", start, r, end, replayed, mode, wall_time() - begin);
            free(exact); return 1;
        }
        if ((r - start + 1) % 1000 == 0 || r + 1 == end) {
            double elapsed = wall_time() - begin;
            printf("Verified %u/%u; replayed=%u; elapsed=%.3f s\n", (unsigned)(r - start + 1), (unsigned)count, (unsigned)replayed, elapsed);
            fflush(stdout);
            if (!save_result(file, "RUNNING", start, r + 1, end, replayed, mode, elapsed)) { free(exact); return 1; }
        }
    }
    double elapsed = wall_time() - begin;
    int saved = save_result(file, "PASS", start, end, end, replayed, mode, elapsed);
    printf("PASS: %u optimal lengths; %u own replays; %.3f wall seconds\n", (unsigned)count, (unsigned)replayed, elapsed);
    free(exact); return saved ? 0 : 1;
}



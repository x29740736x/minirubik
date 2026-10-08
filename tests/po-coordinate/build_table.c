/* Host-only verification. All cube operations come from the supplied source. */
#define main vin_original_main
#include "vin_solver.c"
#undef main
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

static uint8_t *build_exact(unsigned histogram[12])
{
    uint8_t *distance = malloc(STATES);
    uint32_t *queue = malloc((size_t)STATES * sizeof *queue);
    uint16_t pt[3][PERMUTATIONS], ot[3][ORIENTATIONS];
    state_t state;
    if (!distance || !queue) {
        free(distance); free(queue); return NULL;
    }
    memset(histogram, 0, 12 * sizeof *histogram);
    for (uint16_t p = 0; p < PERMUTATIONS; ++p) {
        unrank_state((uint32_t)p * ORIENTATIONS, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            pt[face][p] = rank_p(&next);
        }
    }
    for (uint16_t o = 0; o < ORIENTATIONS; ++o) {
        unrank_state(o, &state);
        for (uint8_t face = 0; face < 3; ++face) {
            state_t next = quarter_turn(state, face);
            ot[face][o] = rank_o(&next);
        }
    }
    memset(distance, UINT8_MAX, STATES);
    distance[0] = 0; queue[0] = 0;
    uint32_t head = 0, tail = 1;
    while (head < tail) {
        uint32_t here = queue[head++];
        if (distance[here] > 11) {
            free(distance); free(queue); return NULL;
        }
        ++histogram[distance[here]];
        uint16_t p = (uint16_t)(here / ORIENTATIONS);
        uint16_t o = (uint16_t)(here % ORIENTATIONS);
        for (uint8_t face = 0; face < 3; ++face) {
            uint16_t np = p, no = o;
            for (uint8_t turn = 0; turn < 3; ++turn) {
                np = pt[face][np]; no = ot[face][no];
                uint32_t next = (uint32_t)np * ORIENTATIONS + no;
                if (next >= STATES) {
                    free(distance); free(queue); return NULL;
                }
                if (distance[next] == UINT8_MAX) {
                    distance[next] = (uint8_t)(distance[here] + 1);
                    queue[tail++] = next;
                }
            }
        }
    }
    free(queue);
    if (tail != STATES || histogram[11] != 2644) {
        free(distance); return NULL;
    }
    return distance;
}

int main(void)
{
    double begin = wall_time();
    unsigned histogram[12];
    uint8_t *distance = build_exact(histogram);
    if (!distance) { fprintf(stderr, "BFS construction failed\n"); return 1; }
    FILE *f = fopen("exact_distances.bin", "wb");
    if (!f) { free(distance); return 1; }
    size_t written = fwrite(distance, 1, STATES, f);
    int closed = fclose(f);
    free(distance);
    if (written != STATES || closed != 0) {
        fprintf(stderr, "Could not save complete table\n"); return 1;
    }
    printf("BFS TABLE PASS: %u states; diameter 11; distance-11 count %u\n", STATES, histogram[11]);
    for (unsigned d = 0; d < 12; ++d) printf("distance %u: %u\n", d, histogram[d]);
    printf("Saved exact_distances.bin: %u bytes; %.3f wall seconds\n", STATES, wall_time() - begin);
    return 0;
}

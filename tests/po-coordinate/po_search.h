#ifndef PO_SEARCH_H
#define PO_SEARCH_H

/* Reuse the supplied parser, ranking and cubie moves without modifying them. */
#define main vin_original_main
#include "vin_solver.c"
#undef main

/* Full-state distances are NOT stored here. These are separate P/O moves. */
static uint16_t p_transition[PERMUTATIONS * MOVES];
static uint16_t o_transition[ORIENTATIONS * MOVES];

typedef struct {
    uint16_t p, o;
    uint8_t next_move, previous_face;
} po_frame_t;

/* Nine entries per coordinate; 9*r = (r << 3) + r. */
static uint32_t transition_index(uint16_t rank, uint8_t move)
{
    return ((uint32_t)rank << 3) + rank + move;
}

static int build_po_transitions(void)
{
    state_t state;
    for (uint16_t p = 0; p < PERMUTATIONS; ++p) {
        /* Construct a representative with permutation p and orientation 0. */
        uint32_t combined = ((uint32_t)p << 9) + ((uint32_t)p << 7)
                          + ((uint32_t)p << 6) + ((uint32_t)p << 4)
                          + ((uint32_t)p << 3) + p;
        unrank_state(combined, &state);
        for (uint8_t move = 0; move < MOVES; ++move) {
            state_t next = apply_move(state, move);
            uint16_t coordinate = rank_p(&next);
            if (coordinate >= PERMUTATIONS) return 0;
            p_transition[transition_index(p, move)] = coordinate;
        }
    }
    for (uint16_t o = 0; o < ORIENTATIONS; ++o) {
        /* Construct a representative with permutation 0 and orientation o. */
        unrank_state(o, &state);
        for (uint8_t move = 0; move < MOVES; ++move) {
            state_t next = apply_move(state, move);
            uint16_t coordinate = rank_o(&next);
            if (coordinate >= ORIENTATIONS) return 0;
            o_transition[transition_index(o, move)] = coordinate;
        }
    }
    /* Check every coordinate/move has the expected inverse. */
    for (uint16_t p = 0; p < PERMUTATIONS; ++p)
        for (uint8_t move = 0; move < MOVES; ++move) {
            uint16_t next = p_transition[transition_index(p, move)];
            if (p_transition[transition_index(next, inverse_move[move])] != p) return 0;
        }
    for (uint16_t o = 0; o < ORIENTATIONS; ++o)
        for (uint8_t move = 0; move < MOVES; ++move) {
            uint16_t next = o_transition[transition_index(o, move)];
            if (o_transition[transition_index(next, inverse_move[move])] != o) return 0;
        }
    return 1;
}

/* Same threshold search and move order as vin_solver.c; coordinate frames only. */
static int po_search(state_t state, uint8_t g, uint8_t limit,
                     const uint8_t pd[PERMUTATIONS],
                     const uint8_t od[ORIENTATIONS],
                     uint8_t path[11], uint8_t previous_face)
{
    if (g > limit || limit > 11) return 0;
    po_frame_t frames[12];
    uint8_t depth = g;
    frames[depth].p = rank_p(&state);
    frames[depth].o = rank_o(&state);
    frames[depth].next_move = 0;
    frames[depth].previous_face = previous_face;

    while (1) {
        po_frame_t *current = &frames[depth];
        if (current->next_move == 0) {
            uint8_t h = heuristic(pd[current->p], od[current->o]);
            if (depth + h > limit) {
                if (depth == g) return 0;
                --depth;
                continue;
            }
            if (current->p == 0 && current->o == 0) return 1;
            if (depth == limit) {
                if (depth == g) return 0;
                --depth;
                continue;
            }
        }
        if (current->next_move >= MOVES) {
            if (depth == g) return 0;
            --depth;
            continue;
        }
        uint8_t move = current->next_move++;
        uint8_t face = move < 3 ? 0 : (move < 6 ? 1 : 2);
        if (face == current->previous_face) continue;

        path[depth] = move;
        po_frame_t *child = &frames[depth + 1];
        child->p = p_transition[transition_index(current->p, move)];
        child->o = o_transition[transition_index(current->o, move)];
        child->next_move = 0;
        child->previous_face = face;
        ++depth;
    }
}

static int po_solve(state_t state, const uint8_t pd[PERMUTATIONS],
                    const uint8_t od[ORIENTATIONS], uint8_t path[11])
{
    uint8_t first = heuristic(pd[rank_p(&state)], od[rank_o(&state)]);
    for (uint8_t bound = first; bound <= 11; ++bound) {
        memset(path, UINT8_MAX, 11);
        if (po_search(state, 0, bound, pd, od, path, 3)) return bound;
    }
    return -1;
}

static int po_replay(state_t state, const uint8_t path[11], int length)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    if (length < 0 || length > 11) return 0;
    for (int i = 0; i < length; ++i) {
        if (path[i] >= MOVES) return 0;
        state = apply_move(state, path[i]);
    }
    return !memcmp(state.p, solved.p, CUBIES) && !memcmp(state.o, solved.o, CUBIES);
}

#endif

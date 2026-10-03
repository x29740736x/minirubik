#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    CUBIES = 7,
    PERMUTATIONS = 5040,
    ORIENTATIONS = 729,
    STATES = PERMUTATIONS * ORIENTATIONS,
    MOVES = 9
};

typedef struct {
    uint8_t p[CUBIES], o[CUBIES];
} state_t;

/*@ predicate valid_state(state_t *state) =
      (\forall integer i; 0 <= i < CUBIES ==>
         state->p[i] < CUBIES && state->o[i] < 3) &&
      (\forall integer i, j; 0 <= i < j < CUBIES ==>
         state->p[i] != state->p[j]) &&
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
 */

static const char *const move_names[MOVES] = {"R",  "R2", "R'", "B", "B2",
                                              "B'", "D",  "D2", "D'"};
static const uint8_t inverse_move[MOVES] = {2, 1, 0, 5, 4, 3, 8, 7, 6};
/* Each destination takes a cubie from source[face][destination]. */
static const uint8_t source[3][CUBIES] = {
    {1, 4, 2, 0, 3, 5, 6},
    {0, 1, 2, 4, 5, 6, 3},
    {0, 2, 5, 3, 1, 4, 6},
};
static const uint8_t twist[3][CUBIES] = {
    {1, 2, 0, 2, 1, 0, 0},
    {0, 0, 0, 1, 2, 1, 2},
    {0, 0, 0, 0, 0, 0, 0},
};

/* The three quarter-turns preserve the fixed front-upper-left corner. */
/*@ requires face < 3;
    assigns \nothing;
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.p[i] == state.p[source[face][i]];
    ensures \forall integer i; 0 <= i < CUBIES ==>
              \result.o[i] == (state.o[source[face][i]] + twist[face][i]) % 3;
 */
static state_t quarter_turn(state_t state, uint8_t face)
{
    state_t result;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant \forall integer j; 0 <= j < i ==>
          result.p[j] == state.p[source[face][j]];
        loop invariant \forall integer j; 0 <= j < i ==>
          result.o[j] == (state.o[source[face][j]] + twist[face][j]) % 3;
        loop assigns i, result.p[0..6], result.o[0..6];
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t from = source[face][i];
        result.p[i] = state.p[from];
        result.o[i] = (uint8_t) ((state.o[from] + twist[face][i]) % 3U);
    }
    return result;
}

static state_t apply_move(state_t state, uint8_t move)
{
    uint8_t turns = (uint8_t) (move % 3U + 1U);
    for (uint8_t i = 0; i < turns; ++i)
        state = quarter_turn(state, (uint8_t) (move / 3U));
    return state;
}
static void print_state(const state_t *state)
{
    for (uint8_t i = 0; i < CUBIES; ++i)
        printf("%u", state->p[i] + 1);

    for (uint8_t i = 0; i < CUBIES; ++i)
        printf("%u", state->o[i] + 1);

    printf("\n");
}

/*@ requires \valid_read(state);
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->p[i] < CUBIES;
    requires \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    requires \forall integer i; 0 <= i < CUBIES ==>
      0 <= state->o[i] < 3;
    assigns \nothing;
    ensures \result < STATES;
 */
static uint32_t rank_state(const state_t *state)
{
    uint32_t p = 0, o = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant (i == 0 ==> p == 0) && (i == 1 ==> p <= 6) &&
          (i == 2 ==> p <= 41) && (i == 3 ==> p <= 209) &&
          (i == 4 ==> p <= 839) && (i == 5 ==> p <= 2519) &&
          (i >= 6 ==> p <= 5039);
        loop assigns i, p;
        loop variant CUBIES - i;
     */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t smaller = 0;
        /*@ loop invariant i + 1 <= j <= CUBIES;
            loop invariant smaller <= j - i - 1;
            loop assigns j, smaller;
            loop variant CUBIES - j;
         */
        for (uint8_t j = (uint8_t) (i + 1U); j < CUBIES; ++j)
            if (state->p[j] < state->p[i])
                ++smaller;
        p = p * (CUBIES - i) + smaller;
    }
    /*@ loop invariant 0 <= i <= 6;
        loop invariant (i == 0 ==> o == 0) && (i == 1 ==> o < 3) &&
          (i == 2 ==> o < 9) && (i == 3 ==> o < 27) &&
          (i == 4 ==> o < 81) && (i == 5 ==> o < 243) &&
          (i == 6 ==> o < 729);
        loop assigns i, o;
        loop variant 6 - i;
     */
    for (uint8_t i = 0; i < 6; ++i)
        o = o * 3U + state->o[i];
    return p * ORIENTATIONS + o;
}

/*@ requires \valid(state); requires rank < STATES; assigns *state; */
static void unrank_state(uint32_t rank, state_t *state)
{
    uint8_t available[CUBIES] = {0, 1, 2, 3, 4, 5, 6};
    uint32_t p = rank / ORIENTATIONS, o = rank % ORIENTATIONS, f = 720;
    uint8_t sum = 0;
    for (uint8_t i = 0; i < CUBIES; ++i) {
        uint8_t q = (uint8_t) (p / f);
        p %= f;
        state->p[i] = available[q];
        for (uint8_t j = q; j + 1U < CUBIES - i; ++j)
            available[j] = available[j + 1U];
        if (i < 5)
            f /= 6U - i;
    }
    for (uint8_t i = 6; i-- > 0;) {
        state->o[i] = (uint8_t) (o % 3U);
        sum = (uint8_t) (sum + state->o[i]);
        o /= 3U;
    }
    state->o[6] = (uint8_t) ((3U - sum % 3U) % 3U);
}

/*@ requires \valid_read(state);
    requires \initialized(&state->p[0..6]) && \initialized(&state->o[0..6]);
    assigns \nothing;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures complete: valid_state(state) ==> \result != 0;
 */
static int valid(const state_t *state)
{
    uint8_t sum = 0;
    /*@ loop invariant 0 <= i <= CUBIES;
        loop invariant sum <= 2 * i;
        loop invariant sum == (i > 0 ? state->o[0] : 0) +
          (i > 1 ? state->o[1] : 0) + (i > 2 ? state->o[2] : 0) +
          (i > 3 ? state->o[3] : 0) + (i > 4 ? state->o[4] : 0) +
          (i > 5 ? state->o[5] : 0) + (i > 6 ? state->o[6] : 0);
        loop invariant \forall integer j; 0 <= j < i ==>
          state->p[j] < CUBIES && state->o[j] < 3;
        loop invariant \forall integer j, k; 0 <= j < k < i ==>
          state->p[j] != state->p[k];
        loop assigns i, sum;
        loop variant CUBIES - i;
    */
    for (uint8_t i = 0; i < CUBIES; ++i) {
        if (state->p[i] >= CUBIES || state->o[i] >= 3)
            return 0;
        /*@ loop invariant 0 <= j <= i;
            loop invariant \forall integer k; 0 <= k < j ==>
              state->p[k] != state->p[i];
            loop assigns j;
            loop variant i - j;
        */
        for (uint8_t j = 0; j < i; ++j)
            if (state->p[j] == state->p[i])
                return 0;
        sum = (uint8_t) (sum + state->o[i]);
    }
    return sum % 3U == 0;
}

static void build_p_distance(uint8_t p_distance[PERMUTATIONS])//from Vincent
{
    uint16_t permutation[3][PERMUTATIONS];
    uint16_t queue[PERMUTATIONS];

    state_t state;

    //建立 permutation table 
    for (uint16_t rank = 0; rank < PERMUTATIONS; ++rank) {

        unrank_state((uint32_t) rank * ORIENTATIONS, &state);

        for (uint8_t face = 0; face < 3; ++face) {

            state_t next = quarter_turn(state, face);

            permutation[face][rank] =
                (uint16_t)(rank_state(&next) / ORIENTATIONS);
        }
    }


    /* assign all p unvisited
    memset(p_distance, UINT8_MAX, PERMUTATIONS);


    /* BFS queue */
    uint16_t head = 0;
    uint16_t tail = 1;

    queue[0] = 0;

    // solved permutation rank = 0 
    p_distance[0] = 0;


    //BFS
    while (head < tail) {

        uint16_t here = queue[head++];

        for (uint8_t face = 0; face < 3; ++face) {

            uint16_t next = here;

            for (uint8_t turn = 0; turn < 3; ++turn) {

                next = permutation[face][next];

                if (p_distance[next] == UINT8_MAX) {

                    p_distance[next] =
                        (uint8_t)(p_distance[here] + 1);

                    queue[tail++] = next;
                }
            }
        }
    }
}


static void build_o_distance(uint8_t o_distance[ORIENTATIONS])//From Vincent
{
    uint16_t orientation[3][ORIENTATIONS];
    uint16_t queue[ORIENTATIONS];

    state_t state;

    //build o table
    for (uint16_t rank = 0; rank < ORIENTATIONS; ++rank) {

        unrank_state(rank, &state);

        for (uint8_t face = 0; face < 3; ++face) {

            state_t next = quarter_turn(state, face);

            orientation[face][rank] =
                (uint16_t)(rank_state(&next) % ORIENTATIONS);
        }
    }

    //let all o_state unvisited
    memset(o_distance, UINT8_MAX, ORIENTATIONS);

    // BFS queue
    uint16_t head = 0;
    uint16_t tail = 1;

    queue[0] = 0;

    // solved o rank = 0 
    o_distance[0] = 0;

    // BFS 
    while (head < tail) {

        uint16_t here = queue[head++];

        for (uint8_t face = 0; face < 3; ++face) {

            uint16_t next = here;

            for (uint8_t turn = 0; turn < 3; ++turn) {

                next = orientation[face][next];

                if (o_distance[next] == UINT8_MAX) {

                    o_distance[next] =
                        (uint8_t)(o_distance[here] + 1);

                    queue[tail++] = next;
                }
            }
        }
    }
}


static uint8_t heuristic(uint8_t h_p, uint8_t h_o)//from Vincent
{
    if (h_p >= h_o) {
        return h_p;
    }
    else {
        return h_o;
    }
}

static int ida_search(state_t state,
                      uint8_t g,
                      uint8_t limit,
                      const uint8_t p_distance[PERMUTATIONS],
                      const uint8_t o_distance[ORIENTATIONS],
                      uint8_t path[11],
                      uint8_t previous_face)
{
    uint32_t rank = rank_state(&state);

    uint16_t p_rank = (uint16_t)(rank / ORIENTATIONS);
    uint16_t o_rank = (uint16_t)(rank % ORIENTATIONS);

    uint8_t h_p = p_distance[p_rank];
    uint8_t h_o = o_distance[o_rank];
    uint8_t h_value = heuristic(h_p, h_o);

    //剪枝
    if ((uint8_t)(g + h_value) > limit)
        return 0;

    // solved rank = 0
    if (rank == 0)
        return 1;

    // limit
    if (g == limit)
        return 0;

    // try 9 moves
    for (uint8_t move = 0; move < MOVES; ++move) {

        uint8_t face = move / 3;

        if (face == previous_face)
            continue;

        state_t next = apply_move(state, move);

        path[g] = move;

        if (ida_search(next,
                    (uint8_t)(g + 1),
                    limit,
                    p_distance,
                    o_distance,
                    path,
                    face)) {
            return 1;
        }
    }

    return 0;
}

static int ida_solve(state_t state,
                     const uint8_t p_distance[PERMUTATIONS],
                     const uint8_t o_distance[ORIENTATIONS])
{
    uint8_t path[11];

    uint32_t rank = rank_state(&state);

    uint16_t p_rank = (uint16_t)(rank / ORIENTATIONS);
    uint16_t o_rank = (uint16_t)(rank % ORIENTATIONS);

    uint8_t h_p = p_distance[p_rank];
    uint8_t h_o = o_distance[o_rank];

    uint8_t limit = heuristic(h_p, h_o);

    while (limit <= 11) {

        if (ida_search(state,
                       0,
                       limit,
                       p_distance,
                       o_distance,
                       path,3)) {

            printf("solution (%u moves):", limit);

            for (uint8_t i = 0; i < limit; ++i)
                printf(" %s", move_names[path[i]]);

            printf("\n");

            return 1;
        }

        ++limit;
    }

    return 0;
}



/*@ requires valid_read_string(input);
    requires \valid(state);
    assigns state->p[0..6], state->o[0..6];
    ensures \result != 0 ==> input[14] == '\0';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] < CUBIES && state->o[i] < 3;
    ensures \result != 0 ==> \forall integer i, j; 0 <= i < j < CUBIES ==>
      state->p[i] != state->p[j];
    ensures \result != 0 ==>
      (state->o[0] + state->o[1] + state->o[2] + state->o[3] +
       state->o[4] + state->o[5] + state->o[6]) % 3 == 0;
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->p[i] == input[i] - '1';
    ensures \result != 0 ==> \forall integer i; 0 <= i < CUBIES ==>
      state->o[i] == input[i + CUBIES] - '1';
 */
static int parse_state(const char *input, state_t *state)
{
    /*@ loop invariant 0 <= i <= 14;
        loop invariant i <= strlen(input);
        loop invariant i <= 7 ==> \initialized(&state->p[0..i-1]);
        loop invariant i >= 7 ==> \initialized(&state->p[0..6]);
        loop invariant i >= 7 ==> \initialized(&state->o[0..i-8]);
        loop invariant \forall integer j; 0 <= j < i && j < CUBIES ==>
          state->p[j] == input[j] - '1';
        loop invariant \forall integer j; 0 <= j < i - CUBIES ==>
          state->o[j] == input[j + CUBIES] - '1';
        loop assigns i, state->p[0..6], state->o[0..6];
        loop variant 14 - i;
     */
    for (int i = 0; i < 14; ++i) {
        int limit = i < 7 ? 7 : 3;
        if (input[i] < '1' || input[i] > '0' + limit)
            return 0;
        (i < 7 ? state->p : state->o)[i % 7] = (uint8_t) (input[i] - '1');
    }
    return input[14] == '\0' && valid(state);
}

/* stdout is fully buffered off a terminal, so a write error surfaces at the
 * flush, not at the printf that queued the bytes. Every exit path that has
 * produced output goes through here.
 */
static int output_failed(void)
{
    return fflush(stdout) != 0 || ferror(stdout);
}

static int self_test(void)
{
    const state_t solved = {{0, 1, 2, 3, 4, 5, 6}, {0}};
    state_t state;
    for (uint8_t move = 0; move < MOVES; ++move) {
        state = solved;
        state = apply_move(state, move);
        state = apply_move(state, inverse_move[move]);
        if (memcmp(&solved, &state, sizeof solved))
            return 0;
    }
    for (uint32_t rank = 0; rank < STATES; ++rank) {
        unrank_state(rank, &state);
        if (!valid(&state) || rank_state(&state) != rank)
            return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    uint8_t p_distance[PERMUTATIONS];
    uint8_t o_distance[ORIENTATIONS];
    state_t state;

    if (argc != 2 || !parse_state(argv[1], &state)) {
        printf("invalid state\n");
        return 1;
    }

    build_p_distance(p_distance);
    build_o_distance(o_distance);

    if (!ida_solve(state, p_distance, o_distance)) {
        printf("solution not found\n");
        return 1;
    }

    return 0;
}
#include "po_search.h"

int main(int argc, char **argv)
{
    state_t state;
    if (argc != 2 || !parse_state(argv[1], &state)) {
        fprintf(stderr, "Usage: solver_po CUBE_STATE (14 characters)\n");
        return 1;
    }
    if (!build_po_transitions()) {
        fprintf(stderr, "Transition-table check failed\n");
        return 1;
    }
    uint8_t pd[PERMUTATIONS], od[ORIENTATIONS], path[11];
    build_p_distance(pd);
    build_o_distance(od);
    int length = po_solve(state, pd, od, path);
    if (length < 0 || !po_replay(state, path, length)) {
        fprintf(stderr, "Search/replay failed\n");
        return 1;
    }
    printf("solution (%u moves):", (unsigned)length);
    for (int i = 0; i < length; ++i) printf(" %s", move_names[path[i]]);
    printf("\nverification passed\n");
    return 0;
}

/* Baseline functions stay in a separate translation unit, unchanged. */
#define main baseline_original_main
#include "solver_baseline.c"
#undef main
#include "oracle.h"
void oracle_decode(uint32_t r,uint8_t b[14]) {
    state_t s;unrank_state(r,&s);memcpy(b,s.p,7);memcpy(b+7,s.o,7);
}
uint32_t oracle_rank(const uint8_t b[14]) {
    state_t s;memcpy(s.p,b,7);memcpy(s.o,b+7,7);return rank_state(&s);
}
void oracle_apply(uint8_t b[14],uint8_t m) {
    state_t s;memcpy(s.p,b,7);memcpy(s.o,b+7,7);s=apply_move(s,m);
    memcpy(b,s.p,7);memcpy(b+7,s.o,7);
}
uint8_t *oracle_build(unsigned hist[12]) {
    uint8_t *d=malloc(STATES);uint32_t *q=malloc((size_t)STATES*sizeof *q);
    uint16_t pt[3][PERMUTATIONS],ot[3][ORIENTATIONS];state_t s;
    if(!d || !q){free(d);free(q);return NULL;}
    memset(hist,0,12*sizeof *hist);
    for(uint16_t r=0;r<PERMUTATIONS;++r) {
        unrank_state((uint32_t)r*ORIENTATIONS,&s);
        for(uint8_t f=0;f<3;++f){state_t n=quarter_turn(s,f);pt[f][r]=(uint16_t)(rank_state(&n)/ORIENTATIONS);}
    }
    for(uint16_t r=0;r<ORIENTATIONS;++r) {
        unrank_state(r,&s);
        for(uint8_t f=0;f<3;++f){state_t n=quarter_turn(s,f);ot[f][r]=(uint16_t)(rank_state(&n)%ORIENTATIONS);}
    }
    memset(d,255,STATES);d[0]=0;q[0]=0;uint32_t head=0,tail=1;
    while(head<tail) {
        uint32_t r=q[head++];if(d[r]>11){free(q);free(d);return NULL;}++hist[d[r]];
        uint16_t p=(uint16_t)(r/ORIENTATIONS),o=(uint16_t)(r%ORIENTATIONS);
        for(uint8_t f=0;f<3;++f){uint16_t np=p,no=o;
            for(uint8_t t=0;t<3;++t){np=pt[f][np];no=ot[f][no];uint32_t n=(uint32_t)np*ORIENTATIONS+no;
                if(d[n]==255){d[n]=(uint8_t)(d[r]+1);q[tail++]=n;}
            }
        }
    }
    free(q);if(tail!=STATES || hist[11]!=2644){free(d);return NULL;}return d;
}

/* Actual supplied ida_search is included unchanged, not reimplemented. */
#define main vin_original_main
#include "vin_solver.c"
#undef main
#include "oracle.h"
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
static double now(void){LARGE_INTEGER t,f;QueryPerformanceFrequency(&f);QueryPerformanceCounter(&t);return (double)t.QuadPart/f.QuadPart;}
#else
#include <sys/time.h>
static double now(void){struct timeval t;gettimeofday(&t,NULL);return t.tv_sec+t.tv_usec/1000000.0;}
#endif
static int number(const char *s,uint32_t *v){char *end;errno=0;unsigned long n=strtoul(s,&end,10);
    if(errno || !*s || *end || s[0]=='-' || n>STATES)return 0;
    *v=(uint32_t)n;return 1;}
static int save(const char *file,uint32_t start,uint32_t next,uint32_t end,double sec,const char *status){
    FILE *f=fopen(file,"w");if(!f)return 0;
    fprintf(f,"{\n  \"implementation\": \"supplied vin_solver.c cubie-array IDA*\",\n  \"status\": \"%s\",\n"
      "  \"start_rank\": %u,\n  \"next_rank\": %u,\n  \"end_rank_exclusive\": %u,\n"
      "  \"verified_this_run\": %u,\n  \"complete_entire_domain\": %s,\n  \"h3_wall_seconds\": %.6f\n}\n",
      status,(unsigned)start,(unsigned)next,(unsigned)end,(unsigned)(next-start),
      start==0 && next==STATES && !strcmp(status,"PASS")?"true":"false",sec);
    return fclose(f)==0;
}
int main(int argc,char **argv){
    uint32_t start=0,count=STATES;const char *result="h3_result.json";
    if(argc!=1 && argc!=3 && argc!=4){fprintf(stderr,"Usage: verify_all [START COUNT [RESULT_JSON]]\n");return 2;}
    if(argc>=3 && (!number(argv[1],&start)||!number(argv[2],&count)||!count||start>=STATES||count>STATES-start)){
        fprintf(stderr,"Invalid range.\n");return 2;}
    if(argc==4)result=argv[3];
    uint32_t end=start+count;
    if(!save(result,start,start,end,0,"SETTING_UP"))return 2;
    double setup=now();unsigned hist[12];uint8_t *exact=oracle_build(hist);
    if(!exact){fprintf(stderr,"Oracle construction failed.\n");return 1;}
    printf("Oracle: %u states; diameter 11; distance-11 count %u\n",STATES,hist[11]);
    for(unsigned i=0;i<12;++i)printf("distance %u: %u\n",i,hist[i]);
    uint8_t pd[PERMUTATIONS],od[ORIENTATIONS];build_p_distance(pd);build_o_distance(od);
    unsigned maxp=0,maxo=0;
    for(unsigned i=0;i<PERMUTATIONS;++i){if(pd[i]==255){fprintf(stderr,"Unvisited P entry\n");free(exact);return 1;}if(pd[i]>maxp)maxp=pd[i];}
    for(unsigned i=0;i<ORIENTATIONS;++i){if(od[i]==255){fprintf(stderr,"Unvisited O entry\n");free(exact);return 1;}if(od[i]>maxo)maxo=od[i];}
    if(pd[0]||od[0]){fprintf(stderr,"Solved distance nonzero\n");free(exact);return 1;}
    printf("H2 distance tables PASS: P max=%u; O max=%u; solved entries=0\n",maxp,maxo);
    for(uint32_t r=0;r<STATES;++r){uint8_t b[14];state_t s;oracle_decode(r,b);unrank_state(r,&s);
        if(memcmp(s.p,b,7)||memcmp(s.o,b+7,7)||!valid(&s)||rank_state(&s)!=r
          ||rank_p(&s)!=r/ORIENTATIONS||rank_o(&s)!=r%ORIENTATIONS
          ||heuristic(pd[r/ORIENTATIONS],od[r%ORIENTATIONS])>exact[r]){
            fprintf(stderr,"H1/rank agreement failed at %u\n",(unsigned)r);free(exact);return 1;}}
    printf("H1 + rank agreement PASS: all %u states; setup %.3f s\n",STATES,now()-setup);
    printf("H3: unchanged vin_solver search, ranks [%u,%u)\n",(unsigned)start,(unsigned)end);fflush(stdout);
    double begin=now();
    for(uint32_t r=start;r<end;++r){uint8_t b[14],path[11];state_t s;oracle_decode(r,b);unrank_state(r,&s);int length=-1;
        for(uint8_t bound=heuristic(pd[rank_p(&s)],od[rank_o(&s)]);bound<=11;++bound){
            memset(path,255,sizeof path);if(ida_search(s,0,bound,pd,od,path,3)){length=bound;break;}}
        const char *reason=NULL;
        if(length!=exact[r])reason="length differs from BFS distance";
        else{for(int i=0;i<length;++i){if(path[i]>=MOVES){reason="invalid/unwritten path entry";break;}oracle_apply(b,path[i]);}
            if(!reason && oracle_rank(b)!=0)reason="baseline replay did not solve cube";}
        if(reason){oracle_decode(r,b);char input[15];for(int i=0;i<14;++i)input[i]=(char)('1'+b[i]);input[14]=0;
            fprintf(stderr,"FAIL rank=%u input=%s expected=%u found=%d: %s\n",(unsigned)r,input,exact[r],length,reason);
            save(result,start,r,end,now()-begin,"FAIL");free(exact);return 1;}
        if((r-start+1)%1000==0 || r+1==end){double elapsed=now()-begin;
            printf("H3 verified %u/%u; next_rank=%u; elapsed=%.3f s\n",(unsigned)(r-start+1),(unsigned)count,(unsigned)(r+1),elapsed);fflush(stdout);
            if(!save(result,start,r+1,end,elapsed,"RUNNING")){free(exact);return 1;}}
    }
    double elapsed=now()-begin;if(!save(result,start,end,end,elapsed,"PASS")){free(exact);return 1;}
    printf("PASS: %u states; optimal lengths and baseline replay; %.3f wall seconds\n",(unsigned)count,elapsed);free(exact);return 0;
}

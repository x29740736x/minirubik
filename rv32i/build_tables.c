#include "po_search.h"

/*本機建立表格，搜尋仍由組語程式執行*/
static void emit_bytes(FILE *f, const char *name, const uint8_t *table, unsigned count)
{
    fprintf(f,"%s:\n",name);
    for (unsigned i=0;i<count;++i) {
        if (i%16==0) fputs("    .byte ",f);
        fprintf(f,"%u",(unsigned)table[i]);
        fputs(i%16==15 || i+1==count ? "\n" : ",",f);
    }
    fputc('\n',f);
}

static void emit_halves(FILE *f, const char *name, const uint16_t *table, unsigned count)
{
    fprintf(f,".align 2\n%s:\n",name);
    for (unsigned i=0;i<count;++i) {
        if (i%12==0) fputs("    .half ",f);
        fprintf(f,"%u",(unsigned)table[i]);
        fputs(i%12==11 || i+1==count ? "\n" : ",",f);
    }
    fputc('\n',f);
}

int main(void)
{
    uint8_t pd[PERMUTATIONS],od[ORIENTATIONS];
    build_p_distance(pd);
    build_o_distance(od);
    if (pd[0] || od[0] || !build_po_transitions()) return 1;
    for (unsigned i=0;i<PERMUTATIONS;++i) if (pd[i]==UINT8_MAX) return 1;
    for (unsigned i=0;i<ORIENTATIONS;++i) if (od[i]==UINT8_MAX) return 1;

    FILE *f=fopen("tables.s","w");
    if (!f) return 1;
    fputs("#本機產生的唯讀表格，不要手動修改\n.section .rodata\n",f);
    emit_bytes(f,"p_distance",pd,PERMUTATIONS);
    emit_bytes(f,"o_distance",od,ORIENTATIONS);
    emit_halves(f,"p_transition",p_transition,PERMUTATIONS*MOVES);
    emit_halves(f,"o_transition",o_transition,ORIENTATIONS*MOVES);
    if (fclose(f)) return 1;
    puts("PASS: built P/O distances and transitions; checked coverage, bounds and inverses.");
    return 0;
}

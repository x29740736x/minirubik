#include <stdint.h>
/* Same coordinate IDA*, move order, pruning and replay as the handwritten solver. */
extern const uint8_t p_distance[5040],o_distance[729];
extern const uint16_t p_transition[5040*9],o_transition[729*9];
#ifndef INPUT_STATE
#define INPUT_STATE "25314672313211"
#endif
char input_state[15]=INPUT_STATE;
typedef struct {uint8_t p[7],o[7];} state_t;
static state_t state,original_state,next_state;
static uint16_t search_coordinates[12][2];
static uint8_t search_next_move[12],search_previous_face[12],solution_path[11];
static const uint8_t source[3][7]={{1,4,2,0,3,5,6},{0,1,2,4,5,6,3},{0,2,5,3,1,4,6}};
static const uint8_t twist[3][7]={{1,2,0,2,1,0,0},{0,0,0,1,2,1,2},{0,0,0,0,0,0,0}};
static const char move_names[9][4]={"R","R2","R'","B","B2","B'","D","D2","D'"};
static void print_string(const char *s){
 register const char *a0 __asm__("a0")=s; register int a7 __asm__("a7")=4;
 __asm__ volatile("ecall": "+r"(a0),"+r"(a7)::"memory");
}
static void print_number(int n){
 register int a0 __asm__("a0")=n; register int a7 __asm__("a7")=1;
 __asm__ volatile("ecall": "+r"(a0),"+r"(a7)::"memory");
}
static int parse_state(void){
 uint32_t seen=0;unsigned sum=0;
 for(unsigned i=0;i<14;i++){
  if(!input_state[i])return 0;
  uint32_t v=(uint32_t)((int)(unsigned char)input_state[i]-49);
  if(i<7){if(v>=7||(seen&(1u<<v)))return 0;seen|=1u<<v;state.p[i]=(uint8_t)v;}
  else{if(v>=3)return 0;state.o[i-7]=(uint8_t)v;sum+=v;}
 }
 if(input_state[14])return 0;
 while(sum>=3)sum-=3;
 return sum==0;
}
static uint16_t rank_p(void){
 uint32_t p=0;
 for(unsigned i=0;i<7;i++){
  unsigned smaller=0;
  for(unsigned j=i+1;j<7;j++)if(state.p[j]<state.p[i])smaller++;
  /* Explicit small constant products avoid GCC folding an add loop into __mulsi3. */
  switch(7-i){
   case 7:p=(p<<3)-p;break;
   case 6:p=(p<<2)+(p<<1);break;
   case 5:p=(p<<2)+p;break;
   case 4:p=p<<2;break;
   case 3:p=(p<<1)+p;break;
   case 2:p=p<<1;break;
   default:break;
  }
  p+=smaller;
 }
 return (uint16_t)p;
}
static uint16_t rank_o(void){
 uint32_t o=0;
 for(unsigned i=0;i<6;i++)o=(o<<1)+o+state.o[i];
 return (uint16_t)o;
}
static unsigned get_heuristic(uint16_t p,uint16_t o){
 unsigned hp=p_distance[p],ho=o_distance[o];return hp>=ho?hp:ho;
}
static unsigned transition_index(uint16_t r,unsigned move){return ((unsigned)r<<3)+r+move;}
static int ida_solve(void){
 unsigned g=0;uint16_t p=rank_p(),o=rank_o();
 search_coordinates[0][0]=p;search_coordinates[0][1]=o;
 unsigned limit=get_heuristic(p,o);
 search_next_move[0]=0;search_previous_face[0]=3;
check_node:
 if(g+get_heuristic(p,o)>limit)goto pruned;
 if((p|o)==0)return (int)g;
 if(g>=limit)goto pruned;
expand:
 {
  unsigned move=search_next_move[g];
  if(move>=9)goto pruned;
  search_next_move[g]=(uint8_t)(move+1);
  unsigned face=0,rest=move;
  while(rest>=3){rest-=3;face++;}
  if(face==search_previous_face[g])goto expand;
  p=p_transition[transition_index(search_coordinates[g][0],move)];
  o=o_transition[transition_index(search_coordinates[g][1],move)];
  solution_path[g]=(uint8_t)move;
  ++g;search_coordinates[g][0]=p;search_coordinates[g][1]=o;
  search_next_move[g]=0;search_previous_face[g]=(uint8_t)face;
  goto check_node;
 }
pruned:
 if(g){--g;goto expand;}
 if(++limit>11)return -1;
 g=0;search_next_move[0]=0;search_previous_face[0]=3;
 p=search_coordinates[0][0];o=search_coordinates[0][1];goto check_node;
}
static void apply_move(unsigned move){
 unsigned face=0;while(move>=3){move-=3;face++;}
 unsigned turns=move+1;
 while(turns--){
  for(unsigned i=0;i<7;i++){
   unsigned from=source[face][i];next_state.p[i]=state.p[from];
   unsigned o=state.o[from]+twist[face][i];if(o>=3)o-=3;next_state.o[i]=(uint8_t)o;
  }
  state=next_state;
 }
}
int main(void){
 if(!parse_state()){print_string("invalid state\n");return 1;}
 original_state=state;
 int length=ida_solve();
 if(length<0){print_string("solution not found\n");return 1;}
 state=original_state;
 for(int i=0;i<length;i++){
  if(solution_path[i]>=9){print_string("solution replay failed\n");return 1;}
  apply_move(solution_path[i]);
 }
 if(rank_p()||rank_o()){print_string("solution replay failed\n");return 1;}
 print_string("solution (");print_number(length);print_string(" moves):");
 for(int i=0;i<length;i++){print_string(" ");print_string(move_names[solution_path[i]]);}
 print_string("\n");return 0;
}

.data
input_state:
    .asciz "25314672313211"

state:
    .zero 14 #14 bytes p7|o7 '0'
    #uint8_t p[7];uint8_t o[7];
    #initiallize zero

next_state:
    .zero 14

source_r:
    .byte 1,4,2,0,3,5,6 #新位置從哪裡來的

twist_r:
    .byte 1,2,0,2,1,0,0 #新位置的方向要增加多少

source_b:
    .byte 0,1,2,4,5,6,3 #B的新位置從哪裡來的

twist_b:
    .byte 0,0,0,1,2,1,2 #B的新位置方向要增加多少

source_d:
    .byte 0,2,5,3,1,4,6 #D的新位置從哪裡來的

twist_d:
    .byte 0,0,0,0,0,0,0 #D只改位置，O不增加
#P/O距離表與transition table由建置工具產生並接在檔案後面

#IDA*資料

search_states:
    .zero 168 #保留原本空間；查表搜尋只用第0層備份輸入，供重播使用

search_next_move:
    .zero 12 #每層下一個要嘗試的move，0~8，9代表試完

search_previous_face:
    .zero 12 #每層是從哪個面轉過來，0=R、1=B、2=D
             #第0層沒有前一個面，之後會設定成3

solution_path:
    .zero 11 #最多11步，每個byte保存一個move編號
#查表搜尋使用的P/O編號
.align 2 #讓下面的16-bit資料從偶數地址開始
search_coordinates:
    .zero 48 #12層，每層4 bytes：P編號2 bytes、O編號2 bytes

#文字輸出需要的資料
msg_solution:
    .asciz "solution ("
msg_moves:
    .asciz " moves):"
msg_space:
    .asciz " "
msg_newline:
    .asciz "\n"
msg_invalid:
    .asciz "invalid state\n"
msg_replay_failed:
    .asciz "solution replay failed\n"
msg_no_solution:
    .asciz "solution not found\n"

#每個動作名稱固定4 bytes，包含字串結尾的0
move_name_bytes:
    .byte 82,0,0,0 #R
    .byte 82,50,0,0 #R2
    .byte 82,39,0,0 #R'
    .byte 66,0,0,0 #B
    .byte 66,50,0,0 #B2
    .byte 66,39,0,0 #B'
    .byte 68,0,0,0 #D
    .byte 68,50,0,0 #D2
    .byte 68,39,0,0 #D'

.text
.globl main
main:
    la t0, input_state
    la t1, state
    li t6, 0
    li t2, 14 #counter
    li a2, 0 #O的總和

parse_loop:
    lbu t3, 0(t0)
    beq t3, zero, invalid_input #長度不足
    addi t3, t3, -49 #in ACSII '1' is represented by 49

    li t4, 7
    li t5, 7
    bltu t5, t2, check_range #用t2>7判斷正在處理p
    li t4, 3
    add a2, a2, t3

check_range:
    bgeu t3, t4, invalid_input
    li t5, 7
    bgeu t5, t2, store_value #如果t2<7代表處理o,O可以重複不用做這個check
                            #更正：實際條件是t2<=7，包含第一個O

    li t4, 1
    sll t4, t4, t3 #小考的seen，紀錄seen的位置，判定重複
    and t5, t6, t4
    bne t5, zero, invalid_input
    or t6, t6, t4

store_value:
    sb t3, 0(t1) #合法才save byte

    addi t0, t0, 1
    addi t1, t1, 1
    addi t2, t2, -1 #counter--
    bne t2, zero, parse_loop #if t2!=0 跳回parseloop

    lbu t3, 0(t0) #把'\0'放進t3 check
    bne t3, zero, invalid_input

    #全部14個字元處理完後，再檢查O的總和
    li t4, 3

reduce_sum:
    bltu a2, t4, check_sum
    addi a2, a2, -3
    j reduce_sum
check_sum:
    bne a2, zero, invalid_input

    #保存輸入狀態，作為搜尋的第0層
    li a0, 0
    jal ra, save_search_state

    li s3, 0 #g=0，目前搜尋深度

    #第0層從move=0開始嘗試
    la t0, search_next_move
    sb zero, 0(t0)

    #第0層沒有前一個面，用3表示
    la t0, search_previous_face
    li t1, 3
    sb t1, 0(t0)

    #計算起始狀態的P編號
    la a0, state
    jal ra, rank_p
    mv s2, a0 #保存p_rank

       #計算起始狀態的O編號
    la a0, state
    jal ra, rank_o

    mv s5, a0 #保存起始o_rank

    #保存第0層的P/O編號
    la t0, search_coordinates
    sh s2, 0(t0) #s2保存的是p_rank
    sh a0, 2(t0) #a0目前是o_rank

    #查詢heuristic：a0=p_rank，a1=o_rank
    mv a1, a0
    mv a0, s2
    jal ra, get_heuristic

    mv s4, a0 #limit從起始heuristic開始

    la s8, search_coordinates #目前層座標地址
    la s9, search_next_move #目前層next_move地址
    la s0, p_distance #保留heuristic表格地址
    la s1, o_distance
    la s10, p_transition #搜尋時保留表格地址
    la s11, o_transition
    j ida_check_node #開始檢查目前搜尋節點

invalid_input:
    la a0, msg_invalid
    li a7, 4
    ecall

    li a0, 1
    li a7, 93
    ecall


#依照s1選擇R、B、D的table
#s1：0=R，1=B，2=D
#這邊直接跳到對應入口，不另外使用jal
#所以ra還是回到呼叫程式的位置
quarter_turn_face:
    beq s1, zero, quarter_turn_r
    li t0, 1
    beq s1, t0, quarter_turn_b
    j quarter_turn_d


#R quarter-turn：讀state，將結果寫入next_state
quarter_turn_r:
    la t1, source_r #corner的table
    la t2, twist_r #方向的table
    j quarter_turn


#B quarter-turn：讀state，將結果寫入next_state
quarter_turn_b:
    la t1, source_b #corner的table
    la t2, twist_b #方向的table
    j quarter_turn


#D quarter-turn：讀state，將結果寫入next_state
quarter_turn_d:
    la t1, source_d #corner的table
    la t2, twist_d #方向的table
    j quarter_turn


#R、B、D共用同一個轉動迴圈
#t1、t2已經指向目前面的source、twist
quarter_turn:
    la t0, state #舊位置的起始address
    la t3, next_state #new_state
    li t4, 7 #7corners
    li a3, 3 # upper bound

turn_loop:
    lbu t5, 0(t1) #舊位置編號
    add t5, t0, t5 #舊位置地址

    lbu t6, 0(t5)        # 舊位置的P
    sb t6, 0(t3)         # 新位置的P

    lbu t6, 7(t5)        # 舊位置O
    lbu a2, 0(t2)        # 方向變化
    add t6, t6, a2       # 舊方向+變化=新方向

    bltu t6, a3, store_orientation
    addi t6, t6, -3      # 若>=3，減3取得合法方向

store_orientation:
    sb t6, 7(t3)         # 新位置的O

    addi t1, t1, 1
    addi t2, t2, 1
    addi t3, t3, 1
    addi t4, t4, -1
    bne t4, zero, turn_loop

    ret


#複製14 bytes：a0是來源地址，a1是目的地址
copy_state:
    li t0, 14

copy_state_loop:
    lbu t1, 0(a0)
    sb t1, 0(a1)

    addi a0, a0, 1
    addi a1, a1, 1
    addi t0, t0, -1
    bne t0, zero, copy_state_loop

    ret
    
#0 R、1 R2、2 R'
#3 B、4 B2、5 B'
#6 D、7 D2、8 D'
apply_move:
    #保存返回地址(在stack裡面)，以及這個函式會使用的s0、s1
    addi sp, sp, -16
    sw ra, 12(sp)
    sw s0, 8(sp)
    sw s1, 4(sp)

    mv t0, a0 #先把move放到t0
    li s1, 0 #face：0=R，1=B，2=D
    li t1, 3

decode_move:
    bltu t0, t1, move_decoded #剩下的值<3，代表已找到面
    addi t0, t0, -3 #move-3
    addi s1, s1, 1 #換下一個面
    j decode_move

move_decoded:
    addi s0, t0, 1 #剩下的值+1，就是轉動次數

apply_move_loop:
    jal ra, quarter_turn_face #依照s1選擇面，轉一次

    #next_state → state，下一次使用轉動後的狀態
    la a0, next_state
    la a1, state
    jal ra, copy_state

    addi s0, s0, -1 #轉動次數--
    bne s0, zero, apply_move_loop

    #恢復呼叫前的s0、s1，返回地址
    lw s1, 4(sp)
    lw s0, 8(sp)
    lw ra, 12(sp)
    addi sp, sp, 16

    ret
    
#P編碼：a0 state的起始地址
#回傳a0：p_rank，範圍0~5039
#輸入的P check passed
rank_p:
    mv t0, a0 #保存state的起始地址
    li t1, 0 #i，目前的角塊位置
    li t2, 0 #p，目前累積排列編號

rank_p_loop:
    add t6, t0, t1
    lbu t4, 0(t6) #P[i]

    li t5, 0 #smaller，右邊比P[i]小的數量
    addi t3, t1, 1 #j從i+1開始

count_smaller_loop:
    li t6, 7
    bgeu t3, t6, count_smaller_done #j>=7，右邊已經看完

    add t6, t0, t3
    lbu t6, 0(t6) #讀取P[j]

    bgeu t6, t4, not_smaller #P[j]>=P[i]，不用增加
    addi t5, t5, 1 #找到一個比較小的值

not_smaller:
    addi t3, t3, 1 #j++
    j count_smaller_loop

count_smaller_done:
    #計算p = p * (7-i) + smaller
    #用重複加法取代乘法
    mv a1, t2 #保存舊的p
    li t2, 0 #從0開始累加
    li a2, 7
    sub a2, a2, t1 #要累加7-i次

rank_p_add_loop:
    add t2, t2, a1 #加上一次舊的p
    addi a2, a2, -1 #累加次數--
    bne a2, zero, rank_p_add_loop

    add t2, t2, t5 #加上smaller

    addi t1, t1, 1 #i++
    li t6, 7
    bltu t1, t6, rank_p_loop

    mv a0, t2 #將p_rank放到a0回傳
    ret
    
#O編碼：a0是state的起始地址
#回傳a0：o_rank，範圍0~728
#輸入的O必須已經通過合法性檢查
rank_o:
    addi t0, a0, 7 #跳過前7個P，指向O[0]
    li t1, 6 #只編碼前6個O
    li t2, 0 #o，目前累積的方向編號

rank_o_loop:
    lbu t3, 0(t0) #讀取目前的O

    #o = o * 3 + O[i]
    #o*3等於(o<<1)+o，不用乘法
    slli t4, t2, 1 #舊的o乘2
    add t2, t4, t2 #再加舊的o，得到乘3
    add t2, t2, t3 #加上目前的O

    addi t0, t0, 1 #指向下一個O
    addi t1, t1, -1 #counter--
    bne t1, zero, rank_o_loop

    mv a0, t2 #將o_rank放到a0回傳
    ret
#heuristic：a0是p_rank，a1是o_rank
#回傳a0：max(h_p, h_o)
#p_distance、o_distance必須已經放入真正的距離表資料
get_heuristic:
    la t0, p_distance #P距離表的起始地址
    add t0, t0, a0 #找到p_rank對應的byte
    lbu t1, 0(t0) #h_p

    la t0, o_distance #O距離表的起始地址
    add t0, t0, a1 #找到o_rank對應的byte
    lbu t2, 0(t0) #h_o

    mv a0, t1 #先假設h_p比較大
    bgeu t1, t2, heuristic_done

    mv a0, t2 #如果h_o比較大，就改成h_o

heuristic_done:
    ret
    
#保存目前的state到指定搜尋層
#a0是深度g，必須是0~11
save_search_state:
    #每層14 bytes，計算g*14
    #g*14 = g*16 - g*2，不用乘法
    slli t0, a0, 4 #g*16
    slli t1, a0, 1 #g*2
    sub t0, t0, t1 #g*14

    la a1, search_states
    add a1, a1, t0 #目的地址：search_states + g*14

    la a0, state #來源地址：目前的state

    #直接使用copy_state複製14 bytes
    #用j保留原本的ra，copy_state的ret會回到呼叫者
    j copy_state
#將指定搜尋層的狀態取回state
#a0是深度g，必須是0~11
load_search_state:
    #每層14 bytes，計算g*14
    #g*14 = g*16 - g*2，不用乘法
    slli t0, a0, 4 #g*16
    slli t1, a0, 1 #g*2
    sub t0, t0, t1 #g*14

    la a0, search_states
    add a0, a0, t0 #來源地址：search_states + g*14

    la a1, state #目的地址：目前的state

    #直接使用copy_state複製14 bytes
    #copy_state的ret會回到原本呼叫的位置
    j copy_state
#檢查目前搜尋層的狀態
#s3是g，s4是limit
ida_check_node:
    #新子層的P/O已放在s2、s5，直接查heuristic
    add t0, s0, s2
    lbu t0, 0(t0) #h_p
    add t1, s1, s5
    lbu t1, 0(t1) #h_o
    bgeu t0, t1, ida_h_ready
    mv t0, t1
ida_h_ready:
    add t0, s3, t0 #g+h
    bltu s4, t0, ida_pruned

    or t0, s2, s5 #P、O編號都為0才是解好
    beq t0, zero, ida_found

    bgeu s3, s4, ida_pruned #還沒解好，但已達深度上限
    j ida_expand


#找到解法，先重播確認真的可以還原
ida_found:
    mv s9, s3 #保存找到的解法步數
    li s8, 0 #目前重播到第幾步

    li a0, 0
    jal ra, load_search_state #取回第0層的原始輸入

replay_loop:
    bgeu s8, s9, replay_check #全部動作都重播完

    la t0, solution_path
    add t0, t0, s8
    lbu a0, 0(t0) #取出這一步的move

    li t1, 9
    bgeu a0, t1, replay_failed #move必須是0~8

    jal ra, apply_move #執行這一步，更新state

    addi s8, s8, 1 #換下一步
    j replay_loop

replay_check:
    la a0, state
    jal ra, rank_p
    mv s2, a0 #重播後的p_rank

    la a0, state
    jal ra, rank_o
    mv s5, a0 #重播後的o_rank

    or t0, s2, s5
    bne t0, zero, replay_failed #P、O編號必須都為0

    j print_solution #驗證通過才輸出解法

replay_failed:
    la a0, msg_replay_failed
    li a7, 4
    ecall

    li a4, 5 #5=解法重播驗證失敗
    mv a5, s9
    la a6, solution_path
    j ida_error_end

#這個節點不能繼續，退回上一層
ida_pruned:
    beq s3, zero, ida_limit_exhausted #第0層也停止，這輪結束
    addi s3, s3, -1 #g--，回到父層
    addi s8, s8, -4 #座標地址回到父層
    addi s9, s9, -1 #next_move地址回到父層
    j ida_expand #父層繼續試下一個move

#選擇目前層下一個要試的move
ida_expand:
    lbu s6, 0(s9) #直接從目前層地址取得move

    li t1, 9
    bgeu s6, t1, ida_pruned #0~8都試完，退回父層

    addi t1, s6, 1
    sb t1, 0(s9) #先記錄下次要試的move

    #解析這個move屬於哪個面
    mv t0, s6
    li s7, 0 #face：0=R、1=B、2=D
    li t1, 3

ida_decode_face:
    bltu t0, t1, ida_face_decoded
    addi t0, t0, -3
    addi s7, s7, 1
    j ida_decode_face

ida_face_decoded:
    la t0, search_previous_face
    add t0, t0, s3
    lbu t1, 0(t0) #進入目前層使用的面
    beq s7, t1, ida_expand #連續同面就跳過

    #取出父層的P/O編號
    lhu t1, 0(s8) #父層P
    lhu t2, 2(s8) #父層O

    #每個rank有9個move，每個結果2 bytes
    #位移=rank*18+move*2，用位移和加法計算
    slli t3, s6, 1 #move*2

    slli t4, t1, 4 #P*16
    slli t5, t1, 1 #P*2
    add t4, t4, t5 #P*18
    add t4, t4, t3
    add t4, t4, s10
    lhu t4, 0(t4) #子層P

    slli t5, t2, 4 #O*16
    slli t6, t2, 1 #O*2
    add t5, t5, t6 #O*18
    add t5, t5, t3
    add t5, t5, s11
    lhu t5, 0(t5) #子層O

    la t0, solution_path
    add t0, t0, s3
    sb s6, 0(t0) #保存走向子層的move

    addi s3, s3, 1 #g++
    addi s8, s8, 4 #指向子層座標
    sh t4, 0(s8) #保存子層P
    sh t5, 2(s8) #保存子層O

    addi s9, s9, 1 #指向子層next_move
    sb zero, 0(s9) #新子層從move=0開始試

    la t0, search_previous_face
    add t0, t0, s3
    sb s7, 0(t0) #記錄進入子層使用的面

    mv s2, t4 #子層P留在暫存器，避免重新讀取
    mv s5, t5 #子層O
    j ida_check_node #檢查子層是否剪枝或已解好

#這輪沒有找到解法，提高limit，重新從第0層搜尋
ida_limit_exhausted:
    addi s4, s4, 1 #limit++
    li t0, 11
    bltu t0, s4, ida_no_solution #limit>11，停止搜尋

    li s3, 0 #g回到第0層

    la t0, search_next_move
    sb zero, 0(t0) #第0層重新從move=0開始

    la t0, search_previous_face
    li t1, 3
    sb t1, 0(t0) #第0層沒有前一個面

    la s8, search_coordinates #恢復根節點地址
    la s9, search_next_move
    mv t0, s8
    lhu s2, 0(t0) #恢復根節點P
    lhu s5, 2(t0) #恢復根節點O
    #第0層的P/O座標保持不變，重新從根節點搜尋
    j ida_check_node

ida_no_solution:
    la a0, msg_no_solution
    li a7, 4
    ecall

    li a4, 4 #4=搜尋到limit 11仍沒有找到解法
    li a5, 0 #沒有解法長度
    j ida_error_end

ida_check_end:
    li a0, 0
    li a7, 93
    ecall

#輸出solution (步數 moves): 動作序列
print_solution:
    la a0, msg_solution
    li a7, 4 #輸出字串
    ecall

    mv a0, s9 #解法步數
    li a7, 1 #輸出整數
    ecall

    la a0, msg_moves
    li a7, 4
    ecall

    li s8, 0 #從解法的第0個動作開始

print_move_loop:
    bgeu s8, s9, print_solution_done

    la a0, msg_space
    li a7, 4
    ecall

    la t0, solution_path
    add t0, t0, s8
    lbu t1, 0(t0) #取得move編號

    slli t1, t1, 2 #每個名稱4 bytes，位移是move*4
    la a0, move_name_bytes
    add a0, a0, t1 #找到這個move的名稱
    li a7, 4
    ecall

    addi s8, s8, 1
    j print_move_loop

print_solution_done:
    la a0, msg_newline
    li a7, 4
    ecall

    li a4, 1 #搜尋與重播驗證通過
    mv a5, s9 #解法步數
    la a6, solution_path #保留Memory查看解法的地址
    j ida_check_end

ida_error_end:
    li a0, 1 #搜尋或重播失敗
    li a7, 93
    ecall
#查詢轉動後的P/O編號
#輸入：a0=p_rank，a1=o_rank，a2=move
#move必須是0~8
#回傳：a0=next_p，a1=next_o
lookup_transition:
    #每個rank有9個move，每個結果2 bytes
    #位移=rank*18 + move*2

    slli t0, a2, 1 #move*2

    #查詢P轉動表
    slli t1, a0, 4 #p_rank*16
    slli t2, a0, 1 #p_rank*2
    add t1, t1, t2 #p_rank*18
    add t1, t1, t0 #加上move*2

    la t2, p_transition
    add t1, t2, t1 #這個結果的地址
    lhu a0, 0(t1) #next_p

    #查詢O轉動表
    slli t1, a1, 4 #o_rank*16
    slli t2, a1, 1 #o_rank*2
    add t1, t1, t2 #o_rank*18
    add t1, t1, t0 #加上move*2

    la t2, o_transition
    add t1, t2, t1 #這個結果的地址
    lhu a1, 0(t1) #next_o

    ret
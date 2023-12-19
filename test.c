#include "unp.h"

#define RED     "\x1b[;31;1m"
#define GREEN   "\x1b[;32;1m"
#define YELLOW  "\x1b[;33;1m"
#define BLUE    "\x1b[;34;1m"
#define PURPLE  "\x1b[;35;1m"
#define CYAN    "\x1b[;36;1m"
#define WHITE   "\x1b[;37;1m"

int line_num = 1, slide_ptr = 0, curr_turn = -1;
char sys_msg[20][45], dice_value[5], roll_dices[5];
int score_data[4][19];

void od_set_cursor(int x, int y) { printf("\x1B[%d;%dH", x, y); }

void od_clr_scr() { printf("\x1B[2J"); }

void od_disp_str_red(const char *str)    { printf(RED "%s", str); }
void od_disp_str_green(const char *str)  { printf(GREEN "%s", str); }
void od_disp_str_yellow(const char *str) { printf(YELLOW "%s", str); }
void od_disp_str_blue(const char *str)   { printf(BLUE "%s", str); }
void od_disp_str_purple(const char *str) { printf(PURPLE "%s", str); }
void od_disp_str_cyan(const char *str)   { printf(CYAN "%s", str); }
void od_disp_str_white(const char *str)  { printf(WHITE "%s", str); }

void putxy(int x, int y, const char *str, const char *color) {
    od_set_cursor(x, y);
    printf("%s%s", color, str);
    od_set_cursor(47, 1);
}

void draw_title(int x, int y) {
    putxy(x    , y, "░░▒█░░▒█░█▀▀▄░▒█░▒█░▀▀█▀▀▒█▀▀▀█░▒█▀▀▀░▒█▀▀▀", CYAN); 
    putxy(x + 1, y, "░░▒▀▄▄▄▀▒█▄▄█░▒█▀▀█░░▒█░░░▄▄▄▀▀░▒█▀▀▀░▒█▀▀▀", BLUE); 
    putxy(x + 2, y, "░░░░▒█░░▒█░▒█░▒█░▒█░░▒█░░▒█▄▄▄█░▒█▄▄▄░▒█▄▄▄", PURPLE); 
}

void draw_table() {
    od_set_cursor(4, 1);
    od_disp_str_white("　┌─────────────┬─────────────┬──────────┬──────────┬──────────┬──────────┐\r\n");
    od_disp_str_white("　│UPPER SECTION│HOW TO SCORE?│ PLAYER 1 │ PLAYER 2 │ PLAYER 3 │ PLAYER 4 │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Aces         │    Count    │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Twos         │    Count    │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Threes       │    Count    │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Fours        │    Count    │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Fives        │    Count    │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Sixes        │    Count    │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│NUM_TOTAL(NT)│  ========>  │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Bonus        │ (NT>63) +35 │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│UPPER TOTAL  │  ========>  │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┴──────────┴──────────┴──────────┴──────────┤\r\n");
    od_disp_str_white("　│LOWER SECTION│                                                         │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┬──────────┬──────────┬──────────┬──────────┤\r\n");
    od_disp_str_white("　│3 of a kind  │     Sum     │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│4 of a kind  │     Sum     │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Full House   │     +25     │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Sm. Straight │     +30     │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Lg. Straight │     +40     │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│YAHTZEE      │     +50     │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│Chance       │     Sum     │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│YAHTZEE BONUS│ (each) +100 │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│LOWER TOTAL  │  ========>  │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│GRAND TOTAL  │  ========>  │          │          │          │          │\r\n");
    od_disp_str_white("　└─────────────┴─────────────┴──────────┴──────────┴──────────┴──────────┘\r\n");
}

void draw_sys_msg_board(int x, int y) {
    putxy(x     , y, "┌────────────────────────────────────────────┐", WHITE);
    putxy(x + 1 , y, "│               System Message               │", WHITE);
    putxy(x + 2 , y, "├────────────────────────────────────────────┤", WHITE);
    putxy(x + 3 , y, "│                                            │", WHITE);
    putxy(x + 4 , y, "│                                            │", WHITE);
    putxy(x + 5 , y, "│                                            │", WHITE);
    putxy(x + 6 , y, "│                                            │", WHITE);
    putxy(x + 7 , y, "│                                            │", WHITE);
    putxy(x + 8 , y, "│                                            │", WHITE);
    putxy(x + 9 , y, "│                                            │", WHITE);
    putxy(x + 10, y, "│                                            │", WHITE);
    putxy(x + 11, y, "│                                            │", WHITE);
    putxy(x + 12, y, "│                                            │", WHITE);
    putxy(x + 13, y, "│                                            │", WHITE);
    putxy(x + 14, y, "│                                            │", WHITE);
    putxy(x + 15, y, "│                                            │", WHITE);
    putxy(x + 16, y, "│                                            │", WHITE);
    putxy(x + 17, y, "│                                            │", WHITE);
    putxy(x + 18, y, "└────────────────────────────────────────────┘", WHITE);
}

void put_sys_msg(int x, int y, char *str) {
    slide_ptr = (line_num > 20) ? (line_num - 20) % 20 : 0;
    strcpy(sys_msg[(line_num - 1) % 20], str);

    if (line_num <= 20) {
        for (int i = 0; i < line_num; i++) 
            putxy(x + i, y, sys_msg[i], YELLOW);
    } else {
        for (int i = slide_ptr; i < 20; i++) 
            putxy(x + i, y, sys_msg[i], YELLOW);
        for (int i = 0; i < slide_ptr; i++) 
            putxy(x + i, y, sys_msg[i], YELLOW);
    }
    line_num++;
}

void draw_cmd_board(int x, int y) {
    putxy(x    , y, "┌──────────────────────┐", WHITE);
    putxy(x + 1, y, "│ [ 1-5 ] Choose dices │", WHITE);
    putxy(x + 2, y, "│ [  C  ] Change dices │", WHITE);
    putxy(x + 3, y, "│ [ ▲/▼ ] Move         │", WHITE);
    putxy(x + 4, y, "│ [Enter] Fill in      │", WHITE);
    putxy(x + 5, y, "│                      │", WHITE);
    putxy(x + 6, y, "│ [  Q  ] Quit Game    │", WHITE);
    putxy(x + 7, y, "└──────────────────────┘", WHITE);
}

void draw_dice(int x, int y) {
    putxy(x    , y, "┌───────────┐", WHITE);
    putxy(x + 1, y, "│           │", WHITE);
    putxy(x + 2, y, "│           │", WHITE);
    putxy(x + 3, y, "│           │", WHITE);
    putxy(x + 4, y, "│           │", WHITE);
    putxy(x + 5, y, "│           │", WHITE);
    putxy(x + 6, y, "└───────────┘", WHITE);
}

void start_game() {
    od_clr_scr();
    draw_title(1, 16);
    draw_table();
    draw_dice(5, 77);
    draw_dice(5, 93);
    draw_dice(5, 109);
    draw_dice(12, 85);
    draw_dice(12, 101);
    draw_sys_msg_board(19, 77);
    draw_cmd_board(38, 77);

    // initialize
    line_num = 1;
    slide_ptr = 0;
    curr_turn = -1;
    memset(sys_msg, 0, sizeof(sys_msg));
    memset(roll_dices, '1', sizeof(roll_dices));
    memset(score_data, -1, sizeof(score_data));
}

int main(int argc, char **argv) {
    start_game();
}

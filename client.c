#include "unp.h"
#include <termios.h>
#include <time.h>

#define RED       "\x1b[;31;1m"
#define GREEN     "\x1b[;32;1m"
#define YELLOW    "\x1b[;33;1m"
#define BLUE      "\x1b[;34;1m"
#define PURPLE    "\x1b[;35;1m"
#define CYAN      "\x1b[;36;1m"
#define WHITE     "\x1b[;37;1m"
#define SHINING   "\x1b[;32;1m\x1b[6m"
#define HIDECUR   "\x1b[?25l"
#define SHOWCUR   "\x1b[?25h"
#define CLEARLINE "\x1b[K"

int line_num = 0, curr_turn = -1, player_num, selector_pos = 0;
char sys_msg[15][44], dice_value[5], roll_dices[5], score_table[4][MAXLINE], choosing_table[MAXLINE];
struct timeval timeout = {0, 1000};

void od_set_cursor(int x, int y) { 
    printf("\x1B[%d;%dH", x, y); 
    fflush(stdout);
}

void od_clr_scr() { printf("\x1B[2J"); }

void od_disp_str_red(const char *str)    { printf(RED "%s" WHITE, str); }
void od_disp_str_green(const char *str)  { printf(GREEN "%s" WHITE, str); }
void od_disp_str_yellow(const char *str) { printf(YELLOW "%s" WHITE, str); }
void od_disp_str_blue(const char *str)   { printf(BLUE "%s" WHITE, str); }
void od_disp_str_purple(const char *str) { printf(PURPLE "%s" WHITE, str); }
void od_disp_str_cyan(const char *str)   { printf(CYAN "%s" WHITE, str); }
void od_disp_str_white(const char *str)  { printf(WHITE "%s", str); }

int getch() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    fd_set set;
    struct timeval timeout;

    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);

    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;

    if (select(STDIN_FILENO + 1, &set, NULL, NULL, &timeout) > 0) {
        ch = getchar();
    } else {
        ch = EOF;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    return ch;
}

void set_cursor_bottom() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    int rows = w.ws_row;
    od_set_cursor(rows, 1);
}

void putxy(int x, int y, char *str, char *color) {
    od_set_cursor(x, y);
    printf("%s%s" WHITE, color, str);
    fflush(stdout);
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
    od_disp_str_white("　│BONUS        │ (NT>63) +35 │          │          │          │          │\r\n");
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
    od_disp_str_white("　│YAHTZEE BONUS│  (per) +50  │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│LOWER TOTAL  │  ========>  │          │          │          │          │\r\n");
    od_disp_str_white("　├─────────────┼─────────────┼──────────┼──────────┼──────────┼──────────┤\r\n");
    od_disp_str_white("　│GRAND TOTAL  │  ========>  │          │          │          │          │\r\n");
    od_disp_str_white("　└─────────────┴─────────────┴──────────┴──────────┴──────────┴──────────┘\r\n");
}

void draw_dice_outline(int x, int y, char *color) {
    putxy(x    , y, "┌───────────┐", color);
    putxy(x + 1, y, "│           │", color);
    putxy(x + 2, y, "│           │", color);
    putxy(x + 3, y, "│           │", color);
    putxy(x + 4, y, "│           │", color);
    putxy(x + 5, y, "│           │", color);
    putxy(x + 6, y, "└───────────┘", color);
}

void draw_dice_content(int num, int val, char *color) {
    int x = 0, y = 0;

    switch (num) {
    case 1:
        x = 5;
        y = 77;
        break;
    case 2:
        x = 5;
        y = 93;
        break;
    case 3:
        x = 5;
        y = 109;
        break;
    case 4:
        x = 12;
        y = 85;
        break;
    case 5:
        x = 12;
        y = 101;
        break;        
    }

    switch (val) {
    case 1:
        putxy(x    , y, "┌───────────┐", color);
        putxy(x + 1, y, "│           │", color);
        putxy(x + 2, y, "│           │", color);
        putxy(x + 3, y, "│     ●     │", color);
        putxy(x + 4, y, "│           │", color);
        putxy(x + 5, y, "│           │", color);
        putxy(x + 6, y, "└───────────┘", color);
        break;
    case 2:
        putxy(x    , y, "┌───────────┐", color);
        putxy(x + 1, y, "│         ● │", color);
        putxy(x + 2, y, "│           │", color);
        putxy(x + 3, y, "│           │", color);
        putxy(x + 4, y, "│           │", color);
        putxy(x + 5, y, "│ ●         │", color);
        putxy(x + 6, y, "└───────────┘", color);
        break;
    case 3:
        putxy(x    , y, "┌───────────┐", color);
        putxy(x + 1, y, "│         ● │", color);
        putxy(x + 2, y, "│           │", color);
        putxy(x + 3, y, "│     ●     │", color);
        putxy(x + 4, y, "│           │", color);
        putxy(x + 5, y, "│ ●         │", color);
        putxy(x + 6, y, "└───────────┘", color);
        break;
    case 4:
        putxy(x    , y, "┌───────────┐", color);
        putxy(x + 1, y, "│ ●       ● │", color);
        putxy(x + 2, y, "│           │", color);
        putxy(x + 3, y, "│           │", color);
        putxy(x + 4, y, "│           │", color);
        putxy(x + 5, y, "│ ●       ● │", color);
        putxy(x + 6, y, "└───────────┘", color);
        break;        
    case 5:
        putxy(x    , y, "┌───────────┐", color);
        putxy(x + 1, y, "│ ●       ● │", color);
        putxy(x + 2, y, "│           │", color);
        putxy(x + 3, y, "│     ●     │", color);
        putxy(x + 4, y, "│           │", color);
        putxy(x + 5, y, "│ ●       ● │", color);
        putxy(x + 6, y, "└───────────┘", color);
        break;        
    case 6:
        putxy(x    , y, "┌───────────┐", color);
        putxy(x + 1, y, "│ ●       ● │", color);
        putxy(x + 2, y, "│           │", color);
        putxy(x + 3, y, "│ ●       ● │", color);
        putxy(x + 4, y, "│           │", color);
        putxy(x + 5, y, "│ ●       ● │", color);
        putxy(x + 6, y, "└───────────┘", color);
        break;        
    }

    set_cursor_bottom();
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

void put_sys_msg(char *str) {
    int index = line_num % 15;
    line_num++;
    strncpy(sys_msg[index], str, sizeof(sys_msg[index]) - 1);
    sys_msg[index][sizeof(sys_msg[index]) - 1] = '\0';
    int start = line_num > 15 ? line_num % 15 : 0;
    for (int i = 0; i < 15; i++) {
        int idx = (start + i) % 15;
        putxy(22 + i, 79, "                                           ", YELLOW);
        putxy(22 + i, 79, sys_msg[idx], YELLOW);
    }
    set_cursor_bottom();
}

void draw_cmd_board(int x, int y) {
    putxy(x    , y, "┌─────────────────────────────┐", WHITE);
    putxy(x + 1, y, "│ [ 1-5 ] Choose dices        │", WHITE);
    putxy(x + 2, y, "│ [  C  ] Change chosen dices │", WHITE);
    putxy(x + 3, y, "│ [ W/S ] Move up / Move down │", WHITE);
    putxy(x + 4, y, "│ [Enter] Fill in             │", WHITE);
    putxy(x + 5, y, "│                             │", WHITE);
    putxy(x + 6, y, "│ [  Q  ] Quit Game           │", WHITE);
    putxy(x + 7, y, "└─────────────────────────────┘", WHITE);
}

void draw_time_board(int x, int y, char *color) {
    putxy(x    , y, "┌────────────┐", color);
    putxy(x + 1, y, "│ COUNT DOWN │", color);
    putxy(x + 2, y, "├────────────┤", color);
    putxy(x + 3, y, "│            │", color);
    putxy(x + 4, y, "│            │", color);
    putxy(x + 5, y, "│   SECOND   │", color);
    putxy(x + 6, y, "│            │", color);
    putxy(x + 7, y, "└────────────┘", color);
}

void start_game() {
    od_clr_scr();
    draw_title(1, 16);
    draw_table();
    draw_dice_outline(5, 77, WHITE);
    draw_dice_outline(5, 93, WHITE);
    draw_dice_outline(5, 109, WHITE);
    draw_dice_outline(12, 85, WHITE);
    draw_dice_outline(12, 101, WHITE);
    draw_sys_msg_board(19, 77);
    draw_cmd_board(38, 77);
    draw_time_board(38, 109, WHITE);

    // initialize
    line_num = 0;
    curr_turn = -1;
    selector_pos = 0;
    memset(sys_msg, 0, sizeof(sys_msg));
    memset(roll_dices, '1', sizeof(roll_dices));
    memset(score_table, 0, sizeof(score_table));
    memset(choosing_table, 0, sizeof(choosing_table));
}

void print_score_data(int turn, char *score, char *color) {
    char *data = strtok(score, ",");
    int pos = 0;

    while (data != NULL) {
        char formattedData[10];

        if (pos == 9) pos = 10; // skip "LOWER SECTION" row

        if (strcmp(color, SHINING) == 0) {
            if (strstr(data, "-")) {
                pos++;
                data = strtok(NULL, ",");
                continue;
            }
            else sprintf(formattedData, "+%s ", data);
        }
        else if (strcmp(color, WHITE) == 0) {
            if (strstr(data, "-")) sprintf(formattedData, "   ");
            else sprintf(formattedData, "%s  ", data);
        }
        putxy(7 + 2 * pos++, 36 + 11 * turn, formattedData, color);
        data = strtok(NULL, ",");
    }
    set_cursor_bottom();
}

void move_selector(int dir) {
    char table_data[20][4];
    int pos = 0, init_pos = -1, last_pos = -1;
    char token[4];
    strcpy(table_data[9], "-1");

    const char* p = choosing_table;
    while (sscanf(p, "%3[^,],", token) == 1) {
        if (pos == 9) pos++;
        strncpy(table_data[pos], token, 4);
        if (strcmp(table_data[pos], "-1") != 0) {
            if (init_pos == -1) init_pos = pos;
            last_pos = pos;
        }
        pos++;
        p = strchr(p, ',');
        if (!p) break;
        p++;
    }

    if (selector_pos == -1) 
        selector_pos = init_pos; 
    else {
        while (dir != 0) {
            selector_pos += dir;
            if (selector_pos < init_pos) {
                selector_pos = init_pos;
                break;
            } else if (selector_pos > last_pos) {
                selector_pos = last_pos;
                break;
            } else if (strcmp(table_data[selector_pos], "-1") != 0) {
                break; 
            }
        }
    }

    if (curr_turn == player_num)
        od_set_cursor(7 + 2 * selector_pos, 36 + 11 * curr_turn);
    else
        set_cursor_bottom();
}



void xchg_data(FILE *fp, int sockfd) {
    int maxfdp1;
    char sendline[MAXLINE], recvline[MAXLINE], msg[MAXLINE];
    int key, dice_chosen[5] = {0}, change_dice_times = 0;
    int login_flag = 1, cmd_flag = 0, restart_flag = 0, print_turn_flag = 0, welcome_flag = 0, waiting_room_flag = 0;

    fd_set rset;
    FD_ZERO(&rset);

    for (;;) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd) + 1;
        Select(maxfdp1, &rset, NULL, NULL, &timeout);

        memset(sendline, 0 ,sizeof(sendline));
        memset(recvline, 0 ,sizeof(recvline));
        memset(msg, 0 ,sizeof(msg));

        if (FD_ISSET(sockfd, &rset)) {
            if (Read(sockfd, recvline, MAXLINE) == 0) {
                // set_cursor_bottom();
                // od_disp_str_red("Server has aborted.\n");
                return;
            }

            if (recvline[0] == 'm' && recvline[1] == ':') { // system msg
                if (strstr(recvline, "Game start!") != NULL) {
                    start_game();
                    sscanf(recvline + 2, "Game start!\nn:%d\n\n", &player_num);
                    char player[MAXLINE];
                    sprintf(player, "PLAYER %d", player_num + 1);
                    putxy(5, 33 + 11 * player_num, player, RED);
                    login_flag = 0;
                    waiting_room_flag = 0;
                    change_dice_times = 0;
                }

                strcpy(msg, recvline + 2);
                char *data = strtok(msg, "\n");
                while (data != NULL) {
                    if (!(data[0] == 'n' && data[1] == ':')) {
                        if (strstr(data, "Winner") != NULL) {
                            char tmp[MAXLINE];
                            sprintf(tmp, "%s%s", RED, data);
                            put_sys_msg(tmp);
                        }
                        else put_sys_msg(data);
                    }
                    data = strtok(NULL, "\n");
                }
                
                if (strstr(recvline, "winner") != NULL || strstr(recvline, "Winner") != NULL) {
                    cmd_flag = 0;
                    restart_flag = 1;
                }
                    
                if (strstr(recvline, "See you") != NULL)
                    break;
            }
            else if (recvline[0] == 't' && recvline[1] == ':') { // turn & dice value & score table for 1 person
                char tmp_table[MAXLINE];
                sscanf(recvline + 2, "%d\nv:%s\ns:%s\n", &curr_turn, dice_value, choosing_table);
                strcpy(tmp_table, choosing_table);
                if (curr_turn == player_num) {
                    if (!print_turn_flag) {
                        sprintf(msg, RED "It's your turn!\n");
                        put_sys_msg(msg);
                        print_turn_flag = 1;
                        Writen(sockfd, "start\n", 7);
                    }
                    sprintf(msg, RED "PLAYER %d (YOU) rolled: %c %c %c %c %c\n", curr_turn + 1, dice_value[0], dice_value[1], dice_value[2], dice_value[3], dice_value[4]);
                    put_sys_msg(msg);
                }
                else {
                    sprintf(msg, "PLAYER %d rolled: %c %c %c %c %c\n", curr_turn + 1, dice_value[0], dice_value[1], dice_value[2], dice_value[3], dice_value[4]);
                    put_sys_msg(msg);
                    print_turn_flag = 0;
                }
                for (int i = 0; i < 5; i++) draw_dice_content(i + 1, dice_value[i] - '0', WHITE);
                print_score_data(curr_turn, tmp_table, SHINING);
                cmd_flag = 1;
                
                if (curr_turn == player_num) {
                    selector_pos = -1;
                    move_selector(0);
                }
                else {
                    change_dice_times = 0;
                    memset(dice_chosen, 0, sizeof(dice_chosen));
                }
            }
            else if (recvline[0] == 'a' && recvline[1] == ':') { // all table
                int pos = 0, lineStart = 2;

                for (int i = lineStart; recvline[i] != '\0'; i++) {
                    if (recvline[i] == ' ' || recvline[i+1] == '\0') {
                        char lineData[MAXLINE];
                        int lineLength = (recvline[i+1] == '\0') ? (i+1 - lineStart) : (i - lineStart);
                        strncpy(lineData, recvline + lineStart, lineLength);
                        lineData[lineLength] = '\0';

                        char *scoreData = strchr(lineData, ':');
                        if (scoreData != NULL) {
                            scoreData++;
                            strcpy(score_table[pos], scoreData);
                            print_score_data(pos, score_table[pos], WHITE);
                        }

                        lineStart = i + 1; 
                        pos++;
                        if (recvline[i+1] == '\0') break;
                    }
                }
            }
            else if (recvline[0] == 'l' && recvline[1] == ':') { // time limit
                int time_left = strtol(recvline + 2, NULL, 10);
                if (time_left >= 20) {
                    draw_time_board(38, 109, GREEN);
                    putxy(42, 115, recvline + 2, GREEN);
                }
                else if (time_left < 20 && time_left >= 10) {
                    draw_time_board(38, 109, YELLOW);
                    putxy(42, 115, recvline + 2, YELLOW);
                }
                else if (time_left < 10) {
                    draw_time_board(38, 109, RED);
                    putxy(42, 115, "0", RED);
                    putxy(42, 116, recvline + 2, RED);
                }
                move_selector(0);
            }
            else if (recvline[0] == 'c' && recvline[1] == ':') { // dice chosen status
                for (int i = 0; i < 5; i++)
                    draw_dice_content(i + 1, dice_value[i] - '0', recvline[i + 2] - '0' ? RED : WHITE);
            }
            else if (strstr(recvline, "Welcome") != NULL) {
                for (int i = 1; i <= 17; i++) {
                    od_clr_scr();
                    draw_title(i, 16);
                    usleep(50000);
                }
                welcome_flag = 1;
            }
            else if (strstr(recvline, "Login account") != NULL) { // login page
                for (int i = 1; i < 26; i++) putxy(i, 1, CLEARLINE, WHITE);  
                draw_title(17, 16);
                putxy(22, 32, "[1] Sign up\n", WHITE);
                putxy(25, 32, "[2] Log in\n", WHITE);
                printf(HIDECUR);
                fflush(stdout);

                while (1){
                    key = getch();
                    if (key == '1'){
                        Writen(sockfd, "1\n", 3);
                        break;
                    }
                    else if (key == '2') {
                        Writen(sockfd, "2\n", 3);
                        break;
                    }
                }
            }
            else if (strstr(recvline, "In the waiting room, ") != NULL) { // waiting room
                for (int i = 1; i < 26; i++) putxy(i, 1, CLEARLINE, WHITE);    
                draw_title(1, 1);
                putxy(5, 1, "IN THE WAITING ROOM\n", YELLOW);
                putxy(7, 1, "Enter [1] to start a game\n", YELLOW);
                putxy(9, 1, "Enter [2] to show your game history\n", YELLOW);
                putxy(11, 1, "Enter [3] to show players in the room\n", YELLOW);
                putxy(13, 1, "Enter [4] to exit\n", YELLOW);
                set_cursor_bottom();
                printf(SHOWCUR);
                fflush(stdout);
                waiting_room_flag = 1;
                welcome_flag = 0;
            }
            else {
                set_cursor_bottom();
                printf(CLEARLINE);
                printf("%s", recvline);
                fflush(stdout);

                if (waiting_room_flag) {
                    for (int i = 1; i < 14; i++) putxy(i, 1, CLEARLINE, WHITE);  
                    draw_title(1, 1);
                    putxy(5, 1, "IN THE WAITING ROOM\n", YELLOW);
                    putxy(7, 1, "Enter [1] to start a game\n", YELLOW);
                    putxy(9, 1, "Enter [2] to show your game history\n", YELLOW);
                    putxy(11, 1, "Enter [3] to show players in the room\n", YELLOW);
                    putxy(13, 1, "Enter [4] to exit\n", YELLOW);
                    set_cursor_bottom();
                }
            }
        }

        if (FD_ISSET(fileno(fp), &rset) && login_flag) {
            if (Fgets(sendline, MAXLINE, fp) == NULL) return;
            Writen(sockfd, sendline, strlen(sendline));

            if (welcome_flag) {
                for (int i = 1; i < 26; i++) putxy(i, 1, CLEARLINE, WHITE);  
                draw_title(17, 16);
                putxy(22, 32, "[1] Sign up\n", WHITE);
                putxy(25, 32, "[2] Log in\n", WHITE);
            }

        }

        if (cmd_flag && curr_turn == player_num) {
            key = getch();
            switch (key) {
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
                if (change_dice_times < 2) {
                    dice_chosen[key - '0' - 1] = dice_chosen[key - '0' - 1] ? 0 : 1;
                    draw_dice_content(key - '0', dice_value[key - '0' - 1] - '0', dice_chosen[key - '0' - 1] ? RED : WHITE);

                    sprintf(sendline, "c:%d%d%d%d%d\n", dice_chosen[0], dice_chosen[1], dice_chosen[2], dice_chosen[3], dice_chosen[4]);
                    Writen(sockfd, sendline, strlen(sendline));
                }
                else if (change_dice_times == 2) {
                    sprintf(msg, RED "You have changed twice!\n");
                    put_sys_msg(msg);
                }
                move_selector(0);
                break;
            case 'c':
            case 'C':
                if ((dice_chosen[0] || dice_chosen[1] || dice_chosen[2] || dice_chosen[3] || dice_chosen[4]) && change_dice_times < 2) {
                    sprintf(sendline, "r:%d%d%d%d%d\n", dice_chosen[0], dice_chosen[1], dice_chosen[2], dice_chosen[3], dice_chosen[4]);
                    Writen(sockfd, sendline, strlen(sendline));
                    cmd_flag = 0;
                    change_dice_times++;
                    for (int i = 0; i < 5; i++) dice_chosen[i] = 0;
                }
                else if (change_dice_times == 2) {
                    sprintf(msg, RED "You have changed twice!\n");
                    put_sys_msg(msg);
                }
                else {
                    sprintf(msg, RED "You haven't chosen any dice!\n");
                    put_sys_msg(msg);
                }  
                move_selector(0);    
                break;
            case 'w':
            case 'W':                
                move_selector(-1);
                break;
            case 's':
            case 'S':
                move_selector(1);
                break;
            case '\n': // Enter
                if (selector_pos > 9) selector_pos--;
                sprintf(sendline, "d:%d\n", selector_pos);
                Writen(sockfd, sendline, strlen(sendline));
                cmd_flag = 0;
                set_cursor_bottom();
                break;
            case 'q':
            case 'Q':
                sprintf(msg, "Quit the game? [Y/N]\n");
                put_sys_msg(msg);

                int chose_flag = 0;
                while (!chose_flag) {
                    int ch = getch();
                    switch (ch) {
                        case 'y':
                        case 'Y':
                            Writen(sockfd, "Q\n", 3);
                            cmd_flag = 0;
                            chose_flag = 1;
                            break;
                        case 'n':
                        case 'N':
                            sprintf(msg, "Game continue.\n");
                            put_sys_msg(msg);
                            chose_flag = 1;
                            break;
                    }
                }
                set_cursor_bottom();
                break;
            }
        }

        else if (cmd_flag && curr_turn != player_num) {
            key = getch();
            switch (key) {
            case 'q':
            case 'Q':
                sprintf(msg, "Quit the game? [Y/N]\n");
                put_sys_msg(msg);

                int chose_flag = 0;
                while (!chose_flag) {
                    int ch = getch();
                    switch (ch) {
                        case 'y':
                        case 'Y':
                            Writen(sockfd, "Q\n", 3);
                            cmd_flag = 0;
                            chose_flag = 1;
                            break;
                        case 'n':
                        case 'N':
                            sprintf(msg, "Game continue.\n");
                            put_sys_msg(msg);
                            chose_flag = 1;
                            break;
                    }
                }
                set_cursor_bottom();
                break;
            }
        }

        if (restart_flag) {
            key = getch();
            switch (key) {
            case '1':
                Writen(sockfd, "1\n", 3);
                restart_flag = 0;
                login_flag = 1;
                od_clr_scr();
                break;
            case '2':
                Writen(sockfd, "2\n", 3);
                restart_flag = 0;
                break;
            }
        }
    } 
}

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;

    if (argc != 2) err_quit("usage: client <IPaddress>");

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT + 3);
    Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    Connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

    xchg_data(stdin, sockfd);

    exit(0);
}

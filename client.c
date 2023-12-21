#include "unp.h"
#include <termios.h>

#define RED     "\x1b[;31;1m"
#define GREEN   "\x1b[;32;1m"
#define YELLOW  "\x1b[;33;1m"
#define BLUE    "\x1b[;34;1m"
#define PURPLE  "\x1b[;35;1m"
#define CYAN    "\x1b[;36;1m"
#define WHITE   "\x1b[;37;1m"

int line_num = 0, slide_ptr = 0, curr_turn = -1, player_num;
char sys_msg[15][44], dice_value[5], roll_dices[5];
struct timeval timeout = {0, 100};

void od_set_cursor(int x, int y) { 
    printf("\x1B[%d;%dH", x, y); 
    fflush(stdout);
}

void od_clr_scr() { printf("\x1B[2J"); }

void od_disp_str_red(const char *str)    { printf(RED "%s", str); }
void od_disp_str_green(const char *str)  { printf(GREEN "%s", str); }
void od_disp_str_yellow(const char *str) { printf(YELLOW "%s", str); }
void od_disp_str_blue(const char *str)   { printf(BLUE "%s", str); }
void od_disp_str_purple(const char *str) { printf(PURPLE "%s", str); }
void od_disp_str_cyan(const char *str)   { printf(CYAN "%s", str); }
void od_disp_str_white(const char *str)  { printf(WHITE "%s", str); }

int getch() {
    int ch;
    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    memcpy(&newt, &oldt, sizeof(newt));
    newt.c_lflag &= ~(ECHO | ICANON | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return ch;
}

void putxy(int x, int y, char *str, char *color) {
    od_set_cursor(x, y);
    printf("%s%s\n", color, str);
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

    od_set_cursor(47, 1);
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
        putxy(22 + i, 79, "                                          ", YELLOW);
        putxy(22 + i, 79, sys_msg[idx], YELLOW);
    }
    od_set_cursor(47, 1);
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

    // initialize
    line_num = 0;
    slide_ptr = 0;
    curr_turn = -1;
    memset(sys_msg, 0, sizeof(sys_msg));
    memset(roll_dices, '1', sizeof(roll_dices));
}

void print_score_data(int turn, char *score) {
    char *data = strtok(score, ",");
    int pos = 0;

    while (data != NULL) {
        char formattedData[10];

        if (pos == 9) pos = 10; // skip "LOWER SECTION" row

        if (strlen(data) == 1) sprintf(formattedData, " %s", data);
        else if (data[0] == '-') sprintf(formattedData, "   ");
        else strcpy(formattedData, data);

        putxy(7 + 2 * pos++, 36 + 11 * turn, formattedData, GREEN);
        data = strtok(NULL, ",");
    }
    od_set_cursor(47, 1);
}

void xchg_data(FILE *fp, int sockfd) {
    int maxfdp1;
    char sendline[MAXLINE], recvline[MAXLINE], msg[MAXLINE];
    int key, dice_chosen[5] = {0}, wait_flag = 0;

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
            if (Read(sockfd, recvline, MAXLINE) == 0) return;

            if (recvline[0] == 'm' && recvline[1] == ':') { // system msg
                if (strstr(recvline, "Game start!") != NULL) {
                    start_game();
                    sscanf(recvline + 2, "Game start!\nn:%d\n\n", &player_num);
                    sprintf(msg, "Game start! You are PLAYER %d!\n", player_num + 1);
                    put_sys_msg(msg);
                }
                else {
                    strcpy(msg, recvline + 2);
                    char *data = strtok(msg, "\n");
                    while (data != NULL) {
                        if (!(data[0] == 'n' && data[1] == ':'))
                            put_sys_msg(data);
                        data = strtok(NULL, "\n");
                    }
                }
            }
            else if (recvline[0] == 't' && recvline[1] == ':') { // turn & dice value & score table for 1 person
                char scoreTable[MAXLINE];
                sscanf(recvline + 2, "%d\nv:%s\ns:%s\n", &curr_turn, dice_value, scoreTable);
                for (int i = 0; i < 5; i++) draw_dice_content(i + 1, dice_value[i] - '0', WHITE);
                sprintf(msg, "Player %d rolled: %c %c %c %c %c\n", curr_turn + 1, dice_value[0], dice_value[1], dice_value[2], dice_value[3], dice_value[4]);
                put_sys_msg(msg);
                print_score_data(curr_turn, scoreTable);
                wait_flag = 1;
            }
            else if (recvline[0] == 'a' && recvline[1] == ':') { // all table
                char scoreTable[4][MAXLINE];
                int pos = 0;

                strcpy(msg, recvline + 2);
                char *data = strtok(msg, "\n");
                while (data != NULL) {
                    strcpy(scoreTable[pos], data + 3);
                    print_score_data(pos, scoreTable[pos]);
                    data = strtok(NULL, "\n");
                    pos++;
                }
            }
            else {
                printf("recv: %s", recvline);
                fflush(stdout);
            }
        }

        if (FD_ISSET(fileno(fp), &rset)) {
            if (Fgets(sendline, MAXLINE, fp) == NULL) return;
            Writen(sockfd, sendline, strlen(sendline));
        }

        if (wait_flag) {
            while ((key = getch()) == 0)
                ;
                
            switch (key) {
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                    draw_dice_content(key - '0', dice_value[key - '0' - 1] - '0', dice_chosen[key - '0' - 1] ? WHITE : RED);
                    dice_chosen[key - '0' - 1] = dice_chosen[key - '0' - 1] ? 0 : 1;
                    break;
                case 'c':
                case 'C':
                    wait_flag = 0;
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

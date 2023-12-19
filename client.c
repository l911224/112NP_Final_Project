#include "unp.h"

#define RED     "\x1b[;31;1m"
#define GREEN   "\x1b[;32;1m"
#define YELLOW  "\x1b[;33;1m"
#define BLUE    "\x1b[;34;1m"
#define PURPLE  "\x1b[;35;1m"
#define CYAN    "\x1b[;36;1m"
#define WHITE   "\x1b[;37;1m"

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
    putxy(x     , y, "┌──────────────────────────────────────────────┐", WHITE);
    putxy(x + 1 , y, "│                System Message                │", WHITE);
    putxy(x + 2 , y, "├──────────────────────────────────────────────┤", WHITE);
    putxy(x + 3 , y, "│                                              │", WHITE);
    putxy(x + 4 , y, "│                                              │", WHITE);
    putxy(x + 5 , y, "│                                              │", WHITE);
    putxy(x + 6 , y, "│                                              │", WHITE);
    putxy(x + 7 , y, "│                                              │", WHITE);
    putxy(x + 8 , y, "│                                              │", WHITE);
    putxy(x + 9 , y, "│                                              │", WHITE);
    putxy(x + 10, y, "│                                              │", WHITE);
    putxy(x + 11, y, "│                                              │", WHITE);
    putxy(x + 12, y, "│                                              │", WHITE);
    putxy(x + 13, y, "│                                              │", WHITE);
    putxy(x + 14, y, "│                                              │", WHITE);
    putxy(x + 15, y, "│                                              │", WHITE);
    putxy(x + 16, y, "│                                              │", WHITE);
    putxy(x + 17, y, "│                                              │", WHITE);
    putxy(x + 18, y, "│                                              │", WHITE);
    putxy(x + 19, y, "│                                              │", WHITE);
    putxy(x + 19, y, "│                                              │", WHITE);
    putxy(x + 20, y, "│                                              │", WHITE);
    putxy(x + 21, y, "│                                              │", WHITE);
    putxy(x + 22, y, "│                                              │", WHITE);
    putxy(x + 23, y, "└──────────────────────────────────────────────┘", WHITE);
}

int line_num = 0, slide_ptr = 0;
char sys_msg[20][45];

void put_sys_msg(int x, int y, char *str) {
    slide_ptr = (line_num >= 20) ? (line_num - 20) % 20 : 0;
    strcpy(sys_msg[line_num % 20], str);

    if (line_num < 20) {
        for (int i = 0; i <= line_num; i++) 
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

void start_game() {
    od_clr_scr();
    draw_title(1, 16);
    draw_table();
    draw_sys_msg_board(14, 77);
    draw_cmd_board(38, 77);
}

void xchg_data(FILE *fp, int sockfd) {
    int maxfdp1;
    char sendline[MAXLINE], recvline[MAXLINE];

    fd_set rset;
    FD_ZERO(&rset);

    for (;;) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd) + 1;
        Select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset)) {
            if (Read(sockfd, recvline, MAXLINE) == 0) return;

            if (recvline[0] == 'm' && recvline[1] == ':') { // system msg
                if (strcmp(recvline + 2, "Game start!\n\n") == 0) 
                    start_game();

                char msg[MAXLINE];
                sprintf(msg, "[System] %s", recvline + 2);
                put_sys_msg(17, 79, msg);
            }
            else if (recvline[0] == 's' && recvline[1] == ':') { // system msg
                char msg[MAXLINE];
                sprintf(msg, "[System] %s", recvline + 2);
                put_sys_msg(17, 79, msg);
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



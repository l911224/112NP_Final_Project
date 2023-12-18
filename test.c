#include <stdio.h>

#include "unp.h"

void od_set_cursor(int row, int col) {
    char msg_buf[10];
    sprintf(msg_buf, "\x1B[%d;%dH", row, col);
    printf(msg_buf);
}

void od_clr_scr() { printf("\x1B[2J"); }

void od_disp_str(const char *str) { printf(str); }

void draw_title() {
    od_set_cursor(1, 1);
    od_disp_str("　███████╗\n\r");
    od_disp_str("　\\ \\ / // \\  | | | |_   _|__  / ____| ____|\n\r");
    od_disp_str("　 \\ V // _ \\ | |_| | | |   / /|  _| |  _|  \n\r");
    od_disp_str("　  | |/ ___ \\|  _  | | |  / /_| |___| |___ \n\r");
    od_disp_str("　  |_/_/   \\_\\_| |_| |_| /____|_____|_____|\n\r");
}

void draw_table() {
    od_set_cursor(6, 1);
    od_disp_str("　┌─────────────┬─────────┬────┬────┬────┬────┐\n\r");
    od_disp_str("　│UPPER SECTION│  SCORE  │NO.1│NO.2│NO.3│NO.4│\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Aces         │  Count  │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Twos         │  Count  │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Threes       │  Count  │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Fours        │  Count  │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Fives        │  Count  │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Sixes        │  Count  │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│NUM_TOTAL(N) │========>│    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Bonus        │(N>63)+35│    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│UPPER TOTAL  │========>│    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┴────┴────┴────┴────┤\n\r");
    od_disp_str("　│LOWER SECTION│                             │\n\r");
    od_disp_str("　├─────────────┼─────────┬────┬────┬────┬────┤\n\r");
    od_disp_str("　│3 of a kind  │   Sum   │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│4 of a kind  │   Sum   │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Full House   │   +25   │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Sm. Straight │   +30   │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Lg. Straight │   +40   │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│YAHTZEE      │   +50   │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│Chance       │   Sum   │    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│YAHTZEE BONUS│(per)+100│    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│LOWER TOTAL  │========>│    │    │    │    │\n\r");
    od_disp_str("　├─────────────┼─────────┼────┼────┼────┼────┤\n\r");
    od_disp_str("　│GRAND TOTAL  │========>│    │    │    │    │\n\r");
    od_disp_str("　└─────────────┴─────────┴────┴────┴────┴────┘\n\r");
}

int main(int argc, char **argv) {
    od_clr_scr();
    draw_title();
    draw_table();
}

#include <dirent.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include "unp.h"
#define _XOPEN_SOURCE 700

int numOfMember = 0;
int waitingRoomConnfd[4] = {0};
char waitingRoomUserID[4][MAXLINE];
int startGame = 0;
struct timeval timeout = {0, 100};

void sigchld_handler(int signo);
char *xchg_user_data(int sockfd);
void *waitingRoom(void *argv);
void gameRoom(int sockfd[4], char userID[4][MAXLINE], int *connfdFlag, int *addSockfd, char *IDBuffer, int *disconnect);
int logout(char ID[MAXLINE]);
void dice(char diceToRoll[6], char *diceValue);
int logoutAll();
void countScore(char diceValue[6], int *scoreTable);
int updateHistory(char ID[MAXLINE], int gameType, int win);

int main(int argc, char **argv) {
    // Logout all users
    int l = logoutAll();
    if (l == 1)
        printf("Logout all success\n");
    else if (l == -1)
        printf("Logout all failed\n");
    int listenfd, connfd[20] = {0}, maxfdp1 = -1;
    struct sockaddr_in cliaddr[20], servaddr;
    char userID[20][MAXLINE];
    socklen_t clilen[20];
    // Shared memory set
    int shmFlag = 0, shmUserID = 0, shmAdd = 0, shmDis = 0;
    if ((shmFlag = shmget(IPC_PRIVATE, sizeof(int) * 1, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    if ((shmAdd = shmget(IPC_PRIVATE, sizeof(int) * 1, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    if ((shmDis = shmget(IPC_PRIVATE, sizeof(int) * 1, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    if ((shmUserID = shmget(IPC_PRIVATE, sizeof(char) * MAXLINE, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    int *connfdFlag = NULL, *addSockfd = NULL, *disconnect = NULL;
    char *IDBuffer = NULL;
    // Attach the shm
    if ((connfdFlag = (int*)shmat(shmFlag, NULL, 0)) == -1) {
        perror("shmat");
        exit(1);
    }
    if ((addSockfd = (int*)shmat(shmAdd, NULL, 0)) == -1) {
        perror("shmat");
        exit(1);
    }
    if ((disconnect = (int*)shmat(shmDis, NULL, 0)) == -1) {
        perror("shmat");
        exit(1);
    }
    if ((IDBuffer = (char *)shmat(shmUserID, NULL, 0)) == -1) {
        perror("shmat");
        exit(1);
    }
    // Initialize shared matrix C
    *addSockfd = -1;
    *disconnect = -1;
    *connfdFlag = -10;
    memset(IDBuffer, 0, sizeof(char[MAXLINE]));
    // Create listening socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    //servaddr.sin_addr.s_addr = inet_addr("172.1.1.12");
    servaddr.sin_port = htons(SERV_PORT + 3);
    Bind(listenfd, (struct sockaddr_in*)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);


    // Create FD_set
    fd_set rset;
    // Signal hangler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags |= SA_RESTART;
    Sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    // Create wating room thread
    pthread_t wr;
    pthread_create(&wr, NULL, waitingRoom, NULL);
    for (;;) {
        if (startGame) {        // Start a game
            if (Fork() == 0) {  // Fork child process to start a game
                Close(listenfd);
                gameRoom(waitingRoomConnfd, waitingRoomUserID, connfdFlag, addSockfd, IDBuffer, disconnect);
                shmdt(connfdFlag);
                shmdt(IDBuffer);
                shmdt(addSockfd);
                shmdt(disconnect);
                exit(0);
            }
            startGame = 0;
            numOfMember = 0;
            memset(waitingRoomConnfd, 0, sizeof(waitingRoomConnfd));
            memset(waitingRoomUserID, 0, sizeof(waitingRoomUserID));
            memset(connfd, 0, sizeof(connfd));
            memset(userID, 0, sizeof(userID));
            memset(cliaddr, 0, sizeof(cliaddr));
            memset(clilen, 0, sizeof(clilen));
            printf("memset done\n");
            continue;
        }
        //Disconnect client who wants to exit
        if(*disconnect != -1){
            Close(*disconnect);
            *disconnect = -1;
            continue;
        }

        if (*connfdFlag != -10) {  // Find space and add user to waiting room
            if (*connfdFlag >= 1000) {
                connfd[*connfdFlag - 1000] = 0;
                memset(IDBuffer, 0, sizeof(IDBuffer));
                *connfdFlag = -10;
                printf("Connfd reset\n");
                continue;
            }
            int freeSpace = 0;
            for (int i = 0; i < 4; i++) {
                if (waitingRoomConnfd[i] == 0) {
                    freeSpace = i;
                    break;
                }
            }
            strcpy(waitingRoomUserID[freeSpace], IDBuffer);
            if (*addSockfd != -1) {
                waitingRoomConnfd[freeSpace] = *addSockfd;
                *addSockfd = -1;
            } else {
                waitingRoomConnfd[freeSpace] = connfd[*connfdFlag];
            }
            numOfMember++;
            char sendline[MAXLINE];
            // Broadcast to all players
            sprintf(sendline, "%s entered the waiting room. There are %d players waiting now.\n\nIn the waiting room, you can type to chat with other players.\n\nPress [1] to start a game.\nPress [2] to show your game history.\nPress [3] to show players in waiting room.\nPress [4]"
            " to exit the game.\n\n", IDBuffer, numOfMember);
            for (int i = 0; i < 4; i++) {
                if (waitingRoomConnfd[i] == 0 || i == freeSpace) continue;
                Writen(waitingRoomConnfd[i], sendline, MAXLINE);
            }
            *connfdFlag = -10;
            memset(IDBuffer, 0, sizeof(IDBuffer));
            continue;
        }
        char sendline[MAXLINE], recvline[MAXLINE];
        FD_ZERO(&rset);
        FD_SET(listenfd, &rset);
        maxfdp1 = -1;
        maxfdp1 = max(maxfdp1, listenfd) + 1;
        int sel = select(maxfdp1, &rset, 0, 0, &timeout);
        if (sel == 0)
            continue;
        else if (sel < 0) {
            if (errno == EINTR) {
                continue;
            }
            // Handle other select error
            perror("select error");
            break;
        }
        int freeSpace_Connfd = 0;
        for (int i = 0; i < 20; i++) {
            if (connfd[i] == 0) {
                freeSpace_Connfd = i;
                break;
            }
        }
        if (FD_ISSET(listenfd, &rset)) {  // New connect arrived
            if ((connfd[freeSpace_Connfd] = accept(listenfd, (SA *)&cliaddr[freeSpace_Connfd], &clilen[freeSpace_Connfd])) < 0) {
                if (errno == EINTR)
                    continue;
                else
                    perror("accept error");
            }
            int num = freeSpace_Connfd;
            if (Fork() == 0) {  // Fork child process to get user ID
                Close(listenfd);
                // Use shared memory to store data
                strcpy(IDBuffer, xchg_user_data(connfd[num]));
                if (!strcmp(IDBuffer, "error")) {
                    printf("Client disconnected\n");
                    *connfdFlag = 1000 + num;
                    exit(0);
                }
                *connfdFlag = num;
                printf("Successful get data Child Flag = %d and ID = %s and num = %d\n", *connfdFlag, IDBuffer, num);
                shmdt(connfdFlag);
                shmdt(IDBuffer);
                shmdt(disconnect);
                shmdt(addSockfd);
                exit(0);
            }
        }
    }

    pthread_join(wr, NULL);
    return 0;
}

/*
    gameRoom control message:
    t:1 (player 1)
    r:01101(roll dice)

*/

void gameRoom(int sockfd[4], char userID[4][MAXLINE], int *connfdFlag, int *addSockfd, char *IDBuffer, int *disconnect) {
    char sendline[MAXLINE], recvline[MAXLINE], diceValue[6] = {0}, diceToRoll[6] = {0};
    int maxfdp1 = -1, stepCount = -1, turn = -1, oneTurnDoneFlag = 1, scoreTable[19], totalScoreTable[4][19], first = 1, endGame = 0, gameType;
    memset(totalScoreTable, -1, sizeof(totalScoreTable));
    memset(scoreTable, -1, sizeof(scoreTable));
    // Initialize yahtzee bonus
    totalScoreTable[0][16] = 0;
    totalScoreTable[1][16] = 0;
    totalScoreTable[2][16] = 0;
    totalScoreTable[3][16] = 0;
    //Check gameType
    int numOfPlayer = 0;
    for (int i = 0; i < 4; i++) {
        if (sockfd[i] == 0) continue;
        numOfPlayer++;
    }
    gameType = numOfPlayer;
    fd_set rset;
    FD_ZERO(&rset);
    for (;;) {
        if (first) goto NEXTTURN;
        // Fd set
        memset(sendline, 0, sizeof(sendline));
        memset(recvline, 0, sizeof(recvline));
        numOfPlayer = 0;
        maxfdp1 = -1;
        for (int i = 0; i < 4; i++) {
            if (sockfd[i] == 0) continue;
            FD_SET(sockfd[i], &rset);
            numOfPlayer++;
            maxfdp1 = max(maxfdp1, sockfd[i]);
        }
        if(numOfPlayer == 1 && !endGame){
            for(int i = 0; i < 4; i++){
                if(sockfd[i] == 0) continue;
                Writen(sockfd[i], "m:You are the last player.\n\nSo you are the winner.\n\n[1] Play one more game. [2] Exit game.\n\n", MAXLINE);
                endGame = 1;
                updateHistory(userID[i], gameType, 1);
            }
        }
        if (numOfPlayer == 0) break;
        maxfdp1 += 1;
        int sel = select(maxfdp1, &rset, 0, 0, &timeout);
        if (sel == 0)
            continue;
        else if (sel < 0) {
            if (errno == EINTR) {
                continue;
            }
            // Handle other select error
            perror("select error");
            break;
        }
        // Recv messeages
        for (int i = 0; i < 4; i++) {
            if (FD_ISSET(sockfd[i], &rset)) {
                printf("got message\n");
                if (endGame) {                                                                  // Game is over
                    if (Read(sockfd[i], recvline, MAXLINE) == 0 || !strcmp(recvline, "2\n")) {  // Player do not want to play next game
                        printf("Player %d wants to exit game\n", i + 1);
                        Writen(sockfd[i], "m:See you next time!\n", MAXLINE);
                        numOfPlayer--;
                        Close(sockfd[i]);
                        *disconnect = sockfd[i];
                        for (int j = 0; j < 4; j++) {  // Broadcast to all users.
                            if (j == i || sockfd[j] == 0) continue;
                            sprintf(sendline, "m:Player %s has left the room.\n\n", userID[i]);
                            Writen(sockfd[j], sendline, MAXLINE);
                        }
                        logout(userID[i]);  // logout
                        sockfd[i] = 0;      // Reset sockfd and id
                        memset(userID[i], 0, sizeof(userID[i]));
                    } else if (!strcmp(recvline, "1\n")) {  // Add player to waiting room
                        printf("Player %d wants to play one more game\n", i + 1);
                        *addSockfd = sockfd[i];
                        strcpy(IDBuffer, userID[i]);
                        *connfdFlag = 10;
                        Writen(sockfd[i], "In the waiting room, you can type to chat with other players.\n\nPress [1] to start a game.\nPress [2] to show your game history.\nPress [3] to show players in waiting room.\nPress [4]"
                        " to exit the game.\n\n", MAXLINE);
                        Close(sockfd[i]);
                        sockfd[i] = 0;  // Reset sockfd and id
                        memset(userID[i], 0, sizeof(userID[i]));
                    } else {
                        Writen(sockfd[i], "m:Illegal operation. Please try again.\n\n", MAXLINE);
                    }
                } else {
                    if (Read(sockfd[i], recvline, MAXLINE) == 0 || recvline[0] == 'Q') {  // Deal with ctrl + c and 'Q'
                        Writen(sockfd[i], "m:See you next time!\n", MAXLINE);
                        numOfPlayer--;
                        Close(sockfd[i]);
                        *disconnect = sockfd[i];
                        for (int j = 0; j < 4; j++) {  // Broadcast to all users.
                            if (j == i || sockfd[j] == 0) continue;
                            sprintf(sendline, "m:Player %s has left the room.\n\n", userID[i]);
                            Writen(sockfd[j], sendline, MAXLINE);
                        }
                        logout(userID[i]);  // logout
                        sockfd[i] = 0;      // Reset sockfd and id
                        memset(userID[i], 0, sizeof(userID[i]));
                        continue;
                    } else if (recvline[0] == 'r' && recvline[1] == ':') {  // r:01101 means to roll NO.2 3 5 dices
                        // Check player legal or not
                        if (i != turn) {
                            /* sprintf(sendline, "w:Illegal operation.It is player %d turn.\n\n", turn + 1);
                            for (int j = 0; j < 4; j++) {
                                if (sockfd[j] == 0) continue;
                                Writen(sockfd[j], sendline, MAXLINE);
                            } */
                            continue;
                        }
                        sscanf(recvline, "%*[^:]:%s\n", diceToRoll);
                        diceToRoll[5] = '\0';
                        memset(scoreTable, -1, sizeof(scoreTable));
                        dice(diceToRoll, &diceValue);
                        printf("Rolled : %s\n", diceValue);
                        countScore(diceValue, scoreTable);
                        char sendScore[MAXLINE] = {0};
                        for (int j = 0; j < 19; j++) {  // sendScore like : 1,2,3,4,5,6,7,8,-1,-1,0,0,-1,-1,0,0,12,13,-1
                            char tmp[30];
                            scoreTable[j] = (totalScoreTable[turn][j] != -1) ? -1 : scoreTable[j];  //-1 means the table has been filled or do not need to print
                            sprintf(tmp, "%d,", scoreTable[j]);
                            strcat(sendScore, tmp);
                        }
                        sendScore[strlen(sendScore) - 1] = 0;
                        sprintf(sendline, "t:%d\nv:%s\ns:%s\n", turn, diceValue, sendScore);
                        printf("Rolled: %s\n", diceValue);
                        for (int j = 0; j < 4; j++) {
                            if (sockfd[j] == 0) continue;
                            Writen(sockfd[j], sendline, MAXLINE);
                        }
                    } else if (recvline[0] == 'd' && recvline[1] == ':') {  // d:10 fill the 10th table
                        // Check player legal or not
                        if (i != turn) {
                            /* sprintf(sendline, "w:Illegal operation. It is player %d's turn.\n\n", turn + 1);
                            for (int j = 0; j < 4; j++) {
                                if (sockfd[j] == 0) continue;
                                Writen(sockfd[j], sendline, MAXLINE);
                            } */
                            continue;
                        }
                        int toFill = 0;
                        sscanf(recvline, "%*[^:]:%d\n", &toFill);
                        if (totalScoreTable[turn][toFill] != -1 || toFill == 6 || toFill == 7 || toFill == 8 || toFill == 16 || toFill == 17 || toFill == 18) {  // Illegal operation
                            //Writen(sockfd[i], "w:Illegal operation.\n\n", MAXLINE);
                        } else {  // Fill the totalScoreTable
                            totalScoreTable[turn][toFill] = scoreTable[toFill];
                            oneTurnDoneFlag = 1;
                            // Check players totalScoreTable bonus
                            // Yahtzee bonus
                            if (scoreTable[14] == 50 && totalScoreTable[turn][14] != -1) {
                                totalScoreTable[turn][16] += 100;
                            }
                            // Upper section
                            int flag = 1, sum = 0;
                            for (int j = 0; j < 6; j++) {
                                if (totalScoreTable[turn][j] == -1) {
                                    flag = 0;
                                    break;
                                }
                                sum += totalScoreTable[turn][j];
                            }
                            if (flag) {
                                totalScoreTable[turn][6] = sum;
                                totalScoreTable[turn][7] = (sum > 63) ? 35 : 0;
                                totalScoreTable[turn][8] = totalScoreTable[turn][6] + totalScoreTable[turn][7];
                            }
                            // Lower section
                            flag = 1, sum = 0;
                            for (int j = 9; j < 17; j++) {
                                if (totalScoreTable[turn][j] == -1) {
                                    flag = 0;
                                    break;
                                }
                                sum += totalScoreTable[turn][j];
                            }
                            if (flag) {
                                totalScoreTable[turn][17] = sum;
                                totalScoreTable[turn][18] = (totalScoreTable[turn][8] != -1) ? totalScoreTable[turn][17] + totalScoreTable[turn][8] : -1;
                            }

                            // Send current score table to all players
                            char sendScore[MAXLINE] = {0};
                            strcat(sendScore, "a:"); // all table
                            for (int j = 0; j < 4; j++) {  // sendScore like a:1: 1,2,3,4,5,6,7,8,-1,-1,0,0,-1,-1,0,0,12,13,-1\n
                                if (sockfd[j] == 0) continue;
                                char n[30];
                                sprintf(n, "%d:", j);
                                strcat(sendScore, n);
                                for (int k = 0; k < 19; k++) {
                                    char tmp[30];
                                    sprintf(tmp, "%d,", totalScoreTable[j][k]);
                                    strcat(sendScore, tmp);
                                }
                                sendScore[strlen(sendScore) - 1] = ';';
                            }
                            for (int j = 0; j < 4; j++) {
                                if (sockfd[j] == 0) continue;
                                Writen(sockfd[j], sendScore, MAXLINE);
                            }
                            printf("total score table sent\n");
                        }
                    }
                }
            }
        }

    // Send control messages
    NEXTTURN:
        first = 0;
        if (oneTurnDoneFlag && !endGame) {  // Start a new turn
            memset(sendline, 0, sizeof(sendline));
            stepCount++;
            // Check game status
            if (stepCount == 4 * 1) {  // End game check, modify here to shorten the process normal size = 4 * 13
                // Find max score

                // For test only!!!
                totalScoreTable[0][18] = 10;
                endGame = 1;
                int maxScore = -100, winner[4] = {-1, -1, -1, -1}, count = 0;
                for (int i = 0; i < 4; i++) {
                    if (sockfd[i] == 0) continue;
                    maxScore = max(maxScore, totalScoreTable[i][18]);
                }
                // Find number of winner
                char sendWinner[MAXLINE];
                memset(sendWinner, 0, sizeof(sendWinner));
                for (int i = 0; i < 4; i++) {
                    if (sockfd[i] == 0) continue;

                    if (totalScoreTable[i][18] == maxScore) {
                        winner[i] = 1;
                    }
                }
                if (count)
                    strcat(sendWinner, "m:Winner are ");
                else
                    strcat(sendWinner, "m:Winner is ");
                for (int i = 0; i < 4; i++) {
                    if (sockfd[i] == 0) continue;

                    if(winner[i] == -1){
                        updateHistory(userID[i], gameType, 0);
                        continue;
                    }
                    strcat(sendWinner, userID[i]);
                    strcat(sendWinner, " ");
                    updateHistory(userID[i], gameType, 1);
                }
                strcat(sendWinner, "\n");
                sprintf(sendline, "%s\n[1] Play one more game. [2] Exit game.\n\n", sendWinner);
                // Broadcast winner
                for (int i = 0; i < 4; i++) {
                    if (sockfd[i] == 0) continue;
                    Writen(sockfd[i], sendline, MAXLINE);
                }
                printf("%s", sendline);
                continue;
            }

            turn++;
            if (turn == 4) turn = 0;

            if (sockfd[turn] == 0) goto NEXTTURN;  // No player, go to next turn

            oneTurnDoneFlag = 0;
            memset(diceValue, 0, sizeof(diceValue));
            memset(diceToRoll, 0, sizeof(diceToRoll));
            memset(scoreTable, -1, sizeof(scoreTable));
            dice("11111", &diceValue);
            printf("Rolled : %s\n", diceValue);
            countScore(diceValue, scoreTable);
            char sendScore[MAXLINE] = {0};
            for (int i = 0; i < 19; i++) {  // sendScore like : 1,2,3,4,5,6,7,8,-1,-1,0,0,-1,-1,0,0,12,13,-1
                char tmp[30];
                scoreTable[i] = (totalScoreTable[turn][i] != -1) ? -1 : scoreTable[i];  //-1 means the table has been filled or do not need to print
                sprintf(tmp, "%d,", scoreTable[i]);
                strcat(sendScore, tmp);
            }
            sendScore[strlen(sendScore) - 1] = 0;
            sprintf(sendline, "t:%d\nv:%s\ns:%s\n", turn, diceValue, sendScore);
            printf("sendScore success\n");
            for (int i = 0; i < 4; i++) {
                if (sockfd[i] == 0) continue;
                Writen(sockfd[i], sendline, MAXLINE);
            }
        }
    }

    for (int i = 0; i < 4; i++) {  // logout
        logout(userID[i]);
        if (sockfd[i] == 0) continue;
        Close(sockfd[i]);
    }
    return;
}

void *waitingRoom(void *argv) {
    int maxfdp1 = -1;
    char sendline[MAXLINE], recvline[MAXLINE];
    fd_set rset;
    FD_ZERO(&rset);
    for (;;) {
        if (numOfMember == 4) goto GAMESTART;
        maxfdp1 = -1;
        FD_ZERO(&rset);
        memset(sendline, 0, sizeof(sendline));
        memset(recvline, 0, sizeof(recvline));
        for (int i = 0; i < 4; i++) {
            if (waitingRoomConnfd[i] != 0) {
                FD_SET(waitingRoomConnfd[i], &rset);
                maxfdp1 = max(maxfdp1, waitingRoomConnfd[i]);
            }
        }

        maxfdp1 += 1;
        int sel = select(maxfdp1, &rset, 0, 0, &timeout);
        if (sel == 0) continue;

        for (int i = 0; i < 4; i++) {
            if (FD_ISSET(waitingRoomConnfd[i], &rset)) {
                printf("got message in waiting room\n");
                memset(recvline, 0, sizeof(recvline));
                if (Read(waitingRoomConnfd[i], recvline, MAXLINE) == 0 || !strcmp(recvline, "4\n")) {  // Fin recvd, broadcast to all players in the wating room.
                    Writen(waitingRoomConnfd[i], "See you next time!\n\n", MAXLINE);
                    numOfMember--;
                    Close(waitingRoomConnfd[i]);
                    for (int j = 0; j < 4; j++) {  // Broadcast to all users.
                        if (j == i || waitingRoomConnfd[j] == 0) continue;
                        sprintf(sendline, "Player %s has left the room.\n\nThere are %d players in the waiting room, waiting for somebody to start a game.\n\n", waitingRoomUserID[i], numOfMember);
                        Writen(waitingRoomConnfd[j], sendline, MAXLINE);
                    }
                    logout(waitingRoomUserID[i]);  // logout
                    waitingRoomConnfd[i] = 0;      // Reset sockfd and id
                    memset(waitingRoomUserID[i], 0, sizeof(waitingRoomUserID[i]));
                    continue;
                } else if (!strcmp(recvline, "1\n")) {  // Start a game.
                    if (numOfMember < 2) {              // Check player number is enough or not
                        Writen(waitingRoomConnfd[i], "Number of players are not enuogh. Wait for 1-3 more players to start!\n\n", MAXLINE);
                        continue;
                    }
                GAMESTART:
                    for (int j = 0; j < 4; j++) {
                        if (waitingRoomConnfd[j] == 0) continue;
                        sprintf(sendline, "m:Game start!\nn:%d\n\n", j);  //n:player number
                        Writen(waitingRoomConnfd[j], sendline, MAXLINE);
                    }
                    startGame = 1;
                    break;
                } else if (!strcmp(recvline, "2\n")) {  // Show game history
                    char dataPath[MAXLINE], password[MAXLINE];
                    int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, Using;
                    sprintf(dataPath, "userdata/%s.txt", waitingRoomUserID[i]);
                    FILE *f;
                    f = fopen(dataPath, "r");
                    if (f == NULL) perror("File open error");
                    fscanf(f, "%s%d %d\n%d %d\n%d %d\n%d", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed, &Using);
                    sprintf(sendline, "Your game history are showed below (win / played):\n\n2 players game : %d / %d\n3 players game : %d / %d\n4 players game : %d / %d\n\n", twoWin, twoPlayed,
                            threeWin, threePlayed, fourWin, fourPlayed);
                    fclose(f);
                    Writen(waitingRoomConnfd[i], sendline, MAXLINE);
                    continue;
                } else if (!strcmp(recvline, "3\n")) {
                    char tmp[MAXLINE];
                    for (int j = 0; j < 4; j++) {
                        if (waitingRoomConnfd[j] == 0) {
                            sprintf(tmp, "P%d : (Empty)    ", j + 1);
                            strcat(sendline, tmp);
                            continue;
                        }
                        sprintf(tmp, "P%d : %s    ", j, waitingRoomUserID[j]);
                        strcat(sendline, tmp);
                    }
                    strcat(sendline, "\n\n");
                    Writen(waitingRoomConnfd[i], sendline, MAXLINE);
                    continue;
                }

                sprintf(sendline, "(%s) %s", waitingRoomUserID[i], recvline);
                for (int j = 0; j < 4; j++) {
                    if (waitingRoomConnfd[j] == 0 || i == j) continue;
                    Writen(waitingRoomConnfd[j], sendline, MAXLINE);
                }
            }
        }
    }
}

/* ID.txt ={
    PW
    2W 2P
    3W 3P
    4W 4P
    Using bit
}
*/

char *xchg_user_data(int sockfd) {
    char sendline[MAXLINE], recvline[MAXLINE];
    static char ID[MAXLINE];
    // Ask for user data
    int n;
    Writen(sockfd, "Welcome to Yahtzee!\n\n", MAXLINE);
CHOOSE:
    memset(recvline, 0, sizeof(recvline));
    Writen(sockfd, "[1] Create a new account\t[2] Login account\n\n", MAXLINE);
    n = Read(sockfd, recvline, MAXLINE);
    if (n == 0) goto DISCONNECT;
    if (!strcmp(recvline, "1\n")) {  // Create new account and check the ID exists or not
    NEW:
        char password[MAXLINE], dataPath[MAXLINE], fileContent[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        n = Read(sockfd, ID, MAXLINE);
        if (n == 0) goto DISCONNECT;
        FILE *f, *check;
        ID[strlen(ID) - 1] = 0;
        sprintf(dataPath, "userdata/%s.txt", ID);
        printf("dataPath:%s\n", dataPath);
        check = fopen(dataPath, "r");
        if (check) {  // If ID already exist, goto login
            Writen(sockfd, "ID already exists. Please create a new one or login your account.\n\n", MAXLINE);
            goto CHOOSE;
        }
    PW:
        Writen(sockfd, "Enter your password (At least 1 character): ", MAXLINE);
        n = Read(sockfd, password, MAXLINE);
        if (n == 0) goto DISCONNECT;

        if (!strcmp(password, "\n")) {
            Writen(sockfd, "Password must be at least 1 character, please enter password again.\n\n", MAXLINE);
            goto PW;
        }
        printf("ID %s PW %s\n", ID, password);
        f = fopen(dataPath, "w");
        // printf("Seccessful open file\n");
        sprintf(fileContent, "%s%d %d\n%d %d\n%d %d\n%d", password, 0, 0, 0, 0, 0, 0, 1);
        fprintf(f, fileContent);
        Writen(sockfd, "\nCreate successfully!\n\n", MAXLINE);
        fclose(f);
    } else if (!strcmp(recvline, "2\n")) {  // Login account and show winning rate
    LOGIN:
        printf("Login\n");
        char password[MAXLINE], dataPath[MAXLINE], fileContent[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        n = Read(sockfd, ID, MAXLINE);
        if (n == 0) goto DISCONNECT;
        ID[strlen(ID) - 1] = 0;
        FILE *f;
        sprintf(dataPath, "userdata/%s.txt", ID);
        int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, Using;
        f = fopen(dataPath, "r");
        if (f == NULL) {
            Writen(sockfd, "ID does not exist. Please create a new one or login another ID\n\n", MAXLINE);
            goto CHOOSE;
        }
        fscanf(f, "%s%d %d\n%d %d\n%d %d\n%d", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed, &Using);
        if (Using) {
            Writen(sockfd, "This ID is login now, please create a new one or login another ID\n\n", MAXLINE);
            goto CHOOSE;
        }
    PW_AGAIN:
        memset(recvline, 0, sizeof(recvline));
        Writen(sockfd, "Enter your password : ", MAXLINE);
        n = Read(sockfd, recvline, MAXLINE);
        if (n == 0) goto DISCONNECT;
        recvline[strlen(recvline) - 1] = '\0';
        if (strcmp(password, recvline) != 0) {
            Writen(sockfd, "Password is wrong, please try again\n\n", MAXLINE);
            printf("pw:%s and revc:%s", password, recvline);
            goto PW_AGAIN;
        }
        float winRate2, winRate3, winRate4;
        winRate2 = (!twoPlayed) ? 0 : twoWin / twoPlayed;
        winRate3 = (!threePlayed) ? 0 : threeWin / threePlayed;
        winRate4 = (!fourPlayed) ? 0 : fourWin / fourPlayed;
        fclose(f);
        f = fopen(dataPath, "w");
        sprintf(fileContent, "%s\n%d %d\n%d %d\n%d %d\n%d", password, twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, 1);
        fprintf(f, fileContent);
        sprintf(sendline, "\nLogin successfully!\n\n");
        Writen(sockfd, sendline, MAXLINE);
        fclose(f);
    } else {
        Writen(sockfd, "Illegal operation. Please follow the instruction.\n\n", MAXLINE);
        goto CHOOSE;
    }
    memset(recvline, 0, MAXLINE);
    memset(sendline, 0, MAXLINE);
    if (numOfMember == 0) {
        sprintf(sendline, "You are the first player! Wait for 1 - 3 more players to start a game!\n\n");
    } else {
        sprintf(sendline, "There are %d players in waiting room. Wait for the room keeper to start a game!\n\n", numOfMember + 1);
    }
    Writen(sockfd, sendline, MAXLINE);
    memset(sendline, 0, MAXLINE);
    Writen(sockfd,
           "In the waiting room, you can type to chat with other players.\n\nPress [1] to start a game.\nPress [2] to show your game history.\nPress [3] to show players in waiting room.\nPress [4]"
           " to exit the game.\n\n",
           MAXLINE);
    Close(sockfd);
    return ID;
DISCONNECT:
    logout(ID);
    Close(sockfd);
    return "error";
}

int logout(char ID[MAXLINE]) {
    FILE *f;
    char dataPath[MAXLINE], password[MAXLINE], fileContent[MAXLINE];
    int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, Using;
    sprintf(dataPath, "userdata/%s.txt", ID);
    f = fopen(dataPath, "r");
    if (f == NULL) return 0;
    fscanf(f, "%s\n%d %d\n%d %d\n%d %d\n%d", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed, &Using);
    fclose(f);
    f = fopen(dataPath, "w");  // Clean file
    sprintf(fileContent, "%s\n%d %d\n%d %d\n%d %d\n%d", password, twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, 0);
    fprintf(f, fileContent);
    printf("Logout %s successfully.\n", ID);
    fclose(f);
    return 1;
}

int updateHistory(char ID[MAXLINE], int gameType, int win) {
    FILE *f;
    char dataPath[MAXLINE], password[MAXLINE], fileContent[MAXLINE];
    int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, Using;
    sprintf(dataPath, "userdata/%s.txt", ID);
    f = fopen(dataPath, "r");
    if (f == NULL) return 0;
    fscanf(f, "%s\n%d %d\n%d %d\n%d %d\n%d", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed, &Using);
    fclose(f);
    f = fopen(dataPath, "w");  // Clean file
    if(win == 1 && gameType == 2){
        twoWin++;
    } 
    else if(win == 1 && gameType == 3){
        threeWin++;
    }
    else if(win == 1 && gameType == 4){
        fourWin++;
    }

    if(gameType == 2){
        twoPlayed++;
    }
    else if(gameType == 3){
        threePlayed++;
    }
    else if(gameType == 4){
        fourPlayed++;
    }
    sprintf(fileContent, "%s\n%d %d\n%d %d\n%d %d\n%d", password, twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, 0);
    fprintf(f, fileContent);
    printf("Logout %s successfully.\n", ID);
    fclose(f);
    return 1;
}

void dice(char diceToRoll[6], char *diceValue) {
    srand(time(NULL));
    for (int i = 0; i < 5; i++) {
        if (diceToRoll[i] == '0') continue;
        int v = rand() % 6 + 1;
        *(diceValue + i) = v + '0';
    }
    *(diceValue + 5) = '\0';
    printf("rolled successfully\n");
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    return;
}

int logoutAll() {
    char dataPath[MAXLINE] = "userdata/";
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(dataPath)) != NULL) {
        printf("Open directory successfully\n");
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            char ID[MAXLINE] = {0};
            sscanf(ent->d_name, "%[^.]", ID);
            logout(ID);
        }
        closedir(dir);
    } else {
        printf("Cannot open directory\n");
        return -1;
    }
    return 1;
}

void countScore(char diceValue[6], int *scoreTable) {
    int tmp, flag;
    int count[7] = {0};
    for (int i = 0; i < 5; i++) {
        count[(int)diceValue[i] - '0']++;
    }
    for (int i = 1; i < 7; i++) {
        printf("%d ", count[i]);
    }

    // Aces
    tmp = 0;
    for (int i = 0; i < 5; i++) {
        if (diceValue[i] == '1') tmp += 1;
    }
    scoreTable[0] = tmp;

    // Twos
    tmp = 0;
    for (int i = 0; i < 5; i++) {
        if (diceValue[i] == '2') tmp += 2;
    }
    scoreTable[1] = tmp;

    // Threes
    tmp = 0;
    for (int i = 0; i < 5; i++) {
        if (diceValue[i] == '3') tmp += 3;
    }
    scoreTable[2] = tmp;

    // Fours
    tmp = 0;
    for (int i = 0; i < 5; i++) {
        if (diceValue[i] == '4') tmp += 4;
    }
    scoreTable[3] = tmp;

    // Fives
    tmp = 0;
    for (int i = 0; i < 5; i++) {
        if (diceValue[i] == '5') tmp += 5;
    }
    scoreTable[4] = tmp;

    // Sixes
    tmp = 0;
    for (int i = 0; i < 5; i++) {
        if (diceValue[i] == '6') tmp += 6;
    }
    scoreTable[5] = tmp;

    // 3 of a kind
    tmp = 0;
    flag = 0;
    for (int i = 1; i < 7; i++) {
        if (count[i] >= 3) flag = 1;
    }
    if (flag) {
        for (int i = 0; i < 5; i++) {
            tmp += diceValue[i] - '0';
        }
        scoreTable[9] = tmp;
    } else {
        scoreTable[9] = 0;
    }

    // 4 of a kind
    tmp = 0;
    flag = 0;
    for (int i = 1; i < 7; i++) {
        if (count[i] >= 4) flag = 1;
    }
    if (flag) {
        for (int i = 0; i < 5; i++) {
            tmp += diceValue[i] - '0';
        }
        scoreTable[10] = tmp;
    } else {
        scoreTable[10] = 0;
    }
    // Full house
    tmp = 0;
    int twoFlag = 0, threeFlag = 0;
    for (int i = 1; i < 7; i++) {
        if (count[i] == 2)
            twoFlag = 1;
        else if (count[i] == 3)
            threeFlag = 1;
    }
    if (twoFlag && threeFlag) {
        scoreTable[11] = 25;
    } else {
        scoreTable[11] = 0;
    }
    // Small straight
    flag = 0;
    if((count[1] >= 1 && count[2] >= 1 && count[3] >= 1 && count[4] >= 1) || (count[2] >= 1 && count[3] >= 1 && count[4] >= 1 && count[5] >= 1) || (count[3] >= 1 && count[4] >= 1 && count[5] >= 1 && count[6] >= 1)) flag = 1;
    if (flag)
        scoreTable[12] = 30;
    else
        scoreTable[12] = 0;
    // Large straight
    flag = 0;
    if((count[1] == 1 && count[2] == 1 && count[3] == 1 && count[4] == 1 && count[5] == 1) || (count[6] == 1 && count[2] == 1 && count[3] == 1 && count[4] == 1 && count[5] == 1)) flag = 1;

    if (flag)
        scoreTable[13] = 40;
    else
        scoreTable[13] = 0;
    // Yahtzee
    tmp = 0;
    flag = 0;
    for (int i = 1; i < 7; i++) {
        if (count[i] == 5) {
            flag = 1;
            printf("Yahtzee!\n");
            break;
        }
    }
    if (flag)
        scoreTable[14] = 50;
    else
        scoreTable[14] = 0;
    // Change
    tmp = 0;
    for (int i = 1; i < 7; i++) {
        tmp += count[i] * i;
    }
    scoreTable[15] = tmp;
    printf("countScore success\n");
}

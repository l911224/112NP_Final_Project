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
struct timeval timeout = {2, 0};

void sigchld_handler(int signo);
char *xchg_user_data(int sockfd);
void *waitingRoom(void *argv);
void gameRoom(int sockfd[4], char *userID[4]);

int main(int argc, char **argv) {
    int listenfd, connfd[4] = {0}, maxfdp1 = -1;
    struct sockaddr_in cliaddr[4], servaddr;
    char userID[4][MAXLINE];
    socklen_t clilen[4];
    // Shared memory set
    int shmFlag = 0, shmUserID = 0;
    if ((shmFlag = shmget(IPC_PRIVATE, sizeof(int) * 2, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    if ((shmUserID = shmget(IPC_PRIVATE, sizeof(char) * MAXLINE, IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }
    // Attach the shm
    int *connfdFlag = NULL;
    char *IDBuffer = NULL;
    if ((connfdFlag = (int *)shmat(shmFlag, NULL, 0)) == -1) {
        perror("shmat");
        exit(1);
    }
    if ((IDBuffer = (char *)shmat(shmUserID, NULL, 0)) == -1) {
        perror("shmat");
        exit(1);
    }
    // Initialize shared matrix C
    *connfdFlag = -10;
    memset(IDBuffer, 0, sizeof(char[MAXLINE]));
    // Create listening socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT + 3);
    Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    // Create FD_set
    fd_set rset;
    // Signal handler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags |= SA_RESTART;
    Sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    // Create waiting room thread
    pthread_t wr, xchg;
    pthread_create(&wr, NULL, waitingRoom, NULL);
    printf("Thread created\n");
    for (;;) {
        // printf("Parent Id = %s and Flag = %d\n", IDBuffer, connfdFlag);
        if (startGame) {        // Start a game
            if (Fork() == 0) {  // Fork child process to start a game
                close(listenfd);
                gameRoom(waitingRoomConnfd, waitingRoomUserID);
                exit(0);
            }
            startGame = 0;
            numOfMember = 0;
            memset(waitingRoomConnfd, 0, sizeof(waitingRoomConnfd));
            memset(waitingRoomUserID, 0, sizeof(waitingRoomUserID));
            memset(connfd, 0, sizeof(connfd));
            memset(userID, 0, sizeof(userID));
            continue;
        }

        if (*connfdFlag != -10) {  // Add user to waiting room
            strcpy(waitingRoomUserID[*connfdFlag], IDBuffer);
            waitingRoomConnfd[*connfdFlag] = connfd[*connfdFlag];
            char sendline[MAXLINE];
            // Broadcast to all players
            sprintf(sendline, "%s entered the room.\tThere are %d players waiting now.\n", IDBuffer, numOfMember);
            for (int i = 0; i < 4; i++) {
                if (waitingRoomConnfd[i] == 0) continue;
                Writen(waitingRoomConnfd[i], sendline, MAXLINE);
            }
            printf("waitingRoomConnfd = %d ", waitingRoomConnfd[*connfdFlag]);
            fflush(stdout);
            printf("waitingRoomUserId = %s\n", waitingRoomUserID[*connfdFlag]);
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
        if (FD_ISSET(listenfd, &rset)) {  // New connect arrived
            if ((connfd[numOfMember] = accept(listenfd, (SA *)&cliaddr[numOfMember], &clilen[numOfMember])) < 0) {
                if (errno == EINTR)
                    continue;
                else
                    perror("accept error");
            }
            int num = numOfMember;
            if (Fork() == 0) {  // Fork child process to get user ID
                close(listenfd);
                // Use shared memory to store data
                strcpy(IDBuffer, xchg_user_data(connfd[num]));
                *connfdFlag = num;
                printf("Successful get data\nChild Flag = %d and ID = %s and num = %d\n", *connfdFlag, IDBuffer, num);
                exit(0);
            }
            numOfMember++;
        }
    }

    pthread_join(wr, NULL);
}

void gameRoom(int sockfd[4], char *userID[4]) {
    char sendline[MAXLINE], recvline[MAXLINE];
    for (int i = 0; i < 4; i++) {
        Writen(sockfd[i], "Successfully entered a game!\n", MAXLINE);
        close(sockfd[i]);
    }
    return;
}

void *waitingRoom(void *argv) {
    int maxfdp1 = -1;
    char sendline[MAXLINE], recvline[MAXLINE];
    fd_set rset;
    FD_ZERO(&rset);
    for (;;) {
        maxfdp1 = -1;
        FD_ZERO(&rset);
        memset(sendline, 0, sizeof(sendline));
        memset(recvline, 0, sizeof(recvline));
        for (int i = 0; i < 4; i++) {
            // printf("waitingRoomSockfd[%d] = %d\n",i,waitingRoomSockfd[i]);
            if (waitingRoomConnfd[i] != 0) {
                FD_SET(waitingRoomConnfd[i], &rset);
                maxfdp1 = max(maxfdp1, waitingRoomConnfd[i]);
            }
        }

        maxfdp1 += 1;
        int sel = Select(maxfdp1, &rset, 0, 0, &timeout);
        if (sel == 0) continue;

        for (int i = 0; i < 4; i++) {
            if (FD_ISSET(waitingRoomConnfd[i], &rset)) {
                if (Read(waitingRoomConnfd[i], recvline, MAXLINE) == 0 || recvline[0] == '2') {  // Fin recvd, broadcast to all players in the wating room.
                    Writen(waitingRoomConnfd[i], "See you next time!\n", MAXLINE);
                    numOfMember--;
                    close(waitingRoomConnfd[i]);
                    waitingRoomConnfd[i] = 0;  // Reset sockfd and id
                    memset(waitingRoomUserID[i], 0, sizeof(waitingRoomUserID[i]));
                    for (int j = 0; j < 4; j++) {  // Broadcast to all users.
                        if (j == i || waitingRoomConnfd[j] == 0) continue;
                        sprintf(sendline, "Player: %s has left the room.\n\nThere are %d players in the room, waiting for somebody to start a game.", numOfMember);
                        Writen(waitingRoomConnfd[j], sendline, MAXLINE);
                    }
                    continue;
                } else if (recvline[0] == '1') {  // Start a game.
                    for (int j = 0; j < 4; j++) {
                        if (waitingRoomConnfd[j] == 0) continue;
                        Writen(waitingRoomConnfd[j], "Game start!\n", MAXLINE);
                        close(waitingRoomConnfd[j]);
                    }
                    startGame = 1;
                    break;
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

char *xchg_user_data(int sockfd) {  // client ctrl+c not implemented yet
    char sendline[MAXLINE], recvline[MAXLINE];
    static char ID[MAXLINE];
    // Ask for user data
    Writen(sockfd, "Welcome to Yahtzee!\n\n[1] Create a new account\t[2] Login account\n", MAXLINE);
    Read(sockfd, recvline, MAXLINE);
    if (recvline[0] == '1') {  // Create new account
    NEW:
        char password[MAXLINE], dataPath[MAXLINE], fileContent[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        Read(sockfd, ID, MAXLINE);
        Writen(sockfd, "Enter your password : ", MAXLINE);
        Read(sockfd, password, MAXLINE);
        printf("ID %sPW %s\n", ID, password);
        ID[strlen(ID) - 1] = 0;
        FILE *f;
        sprintf(dataPath, "userdata/%s.txt", ID);
        printf("%s\n", dataPath);
        f = fopen(dataPath, "w");
        printf("Seccessful open file\n");
        sprintf(fileContent, "%s%d %d\n%d %d\n%d %d\n", password, 0, 0, 0, 0, 0, 0);
        fprintf(f, fileContent);
        Writen(sockfd, "\nCreate successfully!\n\n", MAXLINE);
        fclose(f);
    } else if (recvline[0] == '2') {  // Login account and show winning rate
        printf("Login\n");
        char password[MAXLINE], dataPath[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        Read(sockfd, ID, MAXLINE);
        ID[strlen(ID) - 1] = 0;
        FILE *f;
        sprintf(dataPath, "userdata/%s.txt", ID);
        int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed;
        f = fopen(dataPath, "r");
        if (f == NULL) {
            Writen(sockfd, "Account does not exist. Please create a new one\n", MAXLINE);
            goto NEW;
        }
        fscanf(f, "%s\n%d %d\n%d %d\n%d %d\n", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed);
        password[strlen(password)] = '\n';
    PW:
        memset(recvline, 0, sizeof(recvline));
        Writen(sockfd, "Enter your password : ", MAXLINE);
        Read(sockfd, recvline, MAXLINE);
        if (strcmp(password, recvline) != 0) {
            Writen(sockfd, "Password is wrong, please try again\n", MAXLINE);
            goto PW;
        }
        float winRate2, winRate3, winRate4;
        winRate2 = (!twoPlayed) ? 0 : twoWin / twoPlayed;
        winRate3 = (!threePlayed) ? 0 : threeWin / threePlayed;
        winRate4 = (!fourPlayed) ? 0 : fourWin / fourPlayed;
        sprintf(sendline, "\nLogin successfuly!\n\nYour win rate is showed below :\n\nTwo players : %.2f  Three players : %.2f  Four players : %.2f\n", winRate2, winRate3, winRate4);
        Writen(sockfd, sendline, MAXLINE);
        fclose(f);
    }
    memset(recvline, 0, MAXLINE);
    memset(sendline, 0, MAXLINE);
    if (numOfMember == 0) {
        sprintf(sendline, "You are the first player! Wait for 1 - 3 more players to start a game!\n");
    } else {
        sprintf(sendline, "There are %d players in waiting room. Wait for the room keeper to start a game!\n", numOfMember + 1);
    }
    Writen(sockfd, sendline, MAXLINE);
    memset(sendline, 0, MAXLINE);
    Writen(sockfd, "In the waiting room, you can type to chat with other players.\n\nPress [1] to start a game.\tPress[2] to exit the game.\n", MAXLINE);
    Close(sockfd);
    ID[strlen(ID)] = '\0';
    return ID;
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    return;
}

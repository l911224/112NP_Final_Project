#include "unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

int numOfMember = 0;
int waitingRoomSockfd[4] = {0};
char waitingRoomUserID[4][MAXLINE];
int startGame = 0;
struct timeval timeout = {2,0};

void sig_chld(int signo);
void xchg_user_data(int sockfd, char * ID);
void *waitingRoom(void * argv);
void gameRoom(int sockfd[4], char *userID[4]);

int main(int argc, char **argv){
    int listenfd, connfd[4] = {0}, maxfdp1 = -1;
    struct sockaddr_in cliaddr[4], servaddr;
    char userID[4][MAXLINE];
    socklen_t clilen[4];
    // Create listening socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT + 3);
    Bind(listenfd, (SA*) &servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);

    // Create FD_set
    fd_set rset;
    Signal(SIGCHLD, sig_chld);

    // Create wating room thread
    pthread_t wr;
    pthread_create(&wr, NULL, waitingRoom, NULL);
    printf("Thread created\n");
    for(;;){
        if(startGame){
            if(Fork() == 0){    // Fork child process to start a game
                close(listenfd);
                gameRoom(waitingRoomSockfd, waitingRoomUserID);
                exit(0);
            }
            startGame = 0;
            numOfMember = 0;
            memset(waitingRoomSockfd, 0, sizeof(waitingRoomSockfd));
            memset(waitingRoomUserID, 0, sizeof(waitingRoomUserID));
            memset(connfd, 0, sizeof(connfd));
            memset(userID, 0, sizeof(userID));
            continue;
        }

        char sendline[MAXLINE], recvline[MAXLINE];
        FD_ZERO(&rset);
        FD_SET(listenfd, &rset);
        maxfdp1 = -1;
        maxfdp1 = max(maxfdp1, listenfd) + 1;
        int sel = Select(maxfdp1, &rset, 0, 0, &timeout);
        if(sel == 0) continue;

        if(FD_ISSET(listenfd, &rset)){  // New connect arrived
            if ((connfd[numOfMember] = accept(listenfd, (SA *) &cliaddr[numOfMember], &clilen[numOfMember])) < 0) {
                if (errno == EINTR) continue;
                else perror("accept error");
		    }

            if(Fork() == 0){    //Fork child process to get user ID
                close(listenfd);
                int num = numOfMember;
                xchg_user_data(connfd[num],userID[num]);
                strcpy(waitingRoomUserID[num], userID[num]);
                waitingRoomSockfd[num] = connfd[num];
                printf("Successful get data\n");
                for(int i = 0; i < 4; i++){
                    if(waitingRoomSockfd[i] == 0) continue;
                    sprintf(sendline, "%s entered the room.\tThere are %d players waiting now.\n", userID[num], num + 1);
                    Writen(waitingRoomSockfd[i], sendline, MAXLINE);
                }
                exit(0);
            }
            numOfMember++;
        }
        

    }

    pthread_join(wr, NULL);
}

void gameRoom(int sockfd[4], char *userID[4]){
    char sendline[MAXLINE], recvline[MAXLINE];
    for(int i = 0;i < 4; i++){
        Writen(sockfd[i], "Successfully entered a game!\n", MAXLINE);
        close(sockfd[i]);
    }
    return;
}

void *waitingRoom(void *argv){
    int maxfdp1 = -1;
    char sendline[MAXLINE], recvline[MAXLINE];
    fd_set rset;
    for(;;){
        FD_ZERO(&rset);
        memset(sendline, 0, sizeof(sendline));
        memset(recvline, 0, sizeof(recvline));
        for(int i = 0; i < 4; i++){
            if(waitingRoomSockfd[i] != 0){
                FD_SET(waitingRoomSockfd[i], &rset);
                maxfdp1 = max(maxfdp1, waitingRoomSockfd[i]);
            }
        }
        
        maxfdp1 += 1;
        int sel = Select(maxfdp1, &rset, 0, 0, &timeout);
        if(sel == 0) continue;

        for(int i = 0;i < 4; i++){
            if(FD_ISSET(waitingRoomSockfd[i], &rset)){
                if(Read(waitingRoomSockfd[i],recvline,MAXLINE) == 0 || recvline[0] == '2'){  //Fin recvd, broadcast to all players in the wating room.
                    Writen(waitingRoomSockfd[i], "See you next time!\n", MAXLINE);
                    numOfMember--;
                    close(waitingRoomSockfd[i]);
                    waitingRoomSockfd[i] = 0;    //Reset sockfd and id
                    memset(waitingRoomUserID[i], 0, sizeof(waitingRoomUserID[i]));
                    for(int j = 0; j < 4; j++){ // Broadcast to all users.
                        if(j == i || waitingRoomSockfd[j] == 0) continue;
                        sprintf(sendline, "Player: %s has left the room.\n\nThere are %d players in the room, waiting for somebody to start a game.", numOfMember);
                        Writen(waitingRoomSockfd[j], sendline, MAXLINE);
                    }
                    continue;
                }
                else if(recvline[0] == '1'){    // Start a game.
                    for(int j = 0; j < 4; j++){
                        if(waitingRoomSockfd[j] == 0) continue;
                        Writen(waitingRoomSockfd[j], "Game start!\n", MAXLINE);
                        close(waitingRoomSockfd[j]);
                    }
                    startGame = 1;
                    break;
                }

                sprintf(sendline, "(%s) %s", waitingRoomUserID[i], recvline);
                for(int j = 0;j < 4; j++){
                    if(waitingRoomSockfd[j] == 0 || i == j) continue;
                    Writen(waitingRoomSockfd[j], sendline, MAXLINE);
                }
            }
        }
    }
}

void xchg_user_data(int sockfd, char *ID){  //client ctrl+c not implemented yet
    char sendline[MAXLINE], recvline[MAXLINE];
    // Ask for user data
    Writen(sockfd, "Welcome to Yahtzee!\n\n[1] Create a new account\t[2] Login account\n", MAXLINE);
    Read(sockfd, recvline, MAXLINE);
    if(recvline[0] == '1'){   // Create new account
        NEW:
        char password[MAXLINE], dataPath[MAXLINE], fileContent[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        Read(sockfd, ID, MAXLINE);
        Writen(sockfd, "Enter your password : ", MAXLINE);
        Read(sockfd, password, MAXLINE);
        printf("ID %sPW %s\n",ID,password);
        ID[strlen(ID) - 1] = 0;
        FILE *f;
        sprintf(dataPath, "userdata/%s.txt", ID);
        printf("%s\n",dataPath);
        f = fopen(dataPath, "w");
        printf("Seccessful open file\n");
        sprintf(fileContent, "%s%d %d\n%d %d\n%d %d\n", password,0,0,0,0,0,0);
        fprintf(f, fileContent);
        Writen(sockfd, "Create successfully!\n\n", MAXLINE);
        fclose(f);
    }
    else if(recvline[0] == '2'){  // Login account and show winning rate
        printf("Login\n");
        char password[MAXLINE], dataPath[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        Read(sockfd, ID, MAXLINE);
        ID[strlen(ID) - 1] = 0;
        FILE *f;
        sprintf(dataPath, "userdata/%s.txt", ID);
        int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed;
        f = fopen(dataPath, "r");
        if(f == NULL){
            Writen(sockfd, "Account does not exist. Please create a new one\n", MAXLINE);
            goto NEW;
        }
        fscanf(f, "%s\n%d %d\n%d %d\n%d %d\n", &password,&twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed);
        password[strlen(password)] = '\n';
        PW:
        memset(recvline, 0, sizeof(recvline));
        Writen(sockfd, "Enter your password : ", MAXLINE);
        Read(sockfd, recvline, MAXLINE);
        if(strcmp(password, recvline) != 0) goto PW;
        sprintf(sendline, "Login successfuly!\n\nYour win rate is showed below :\n\nTwo players : %.2f\t\tThree players : %.2f\t\tFour players : %.2f", twoWin/twoPlayed, threeWin/threePlayed, fourWin/fourPlayed);
        Writen(sockfd, sendline, MAXLINE);
        fclose(f);
    }
    memset(recvline, 0, MAXLINE);
    memset(sendline, 0, MAXLINE);
    if(numOfMember == 0){
        sprintf(sendline, "You are the first player! Wait for 1 - 3 more players to start a game!\n");
    }
    else{
        sprintf(sendline, "There are %d players in waiting room. Wait for the room keeper to start a game!\n", numOfMember + 1);
    }
    Writen(sockfd, sendline, MAXLINE);
    memset(sendline, 0, MAXLINE);
    Writen(sockfd, "In the waiting room, you can type to chat with other players.\n\nPress [1] to start a game.\tPress[2] to exit the game.\n", MAXLINE);
}

void sig_chld(int signo)
{
    pid_t   pid;
    int  stat;

    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
    ;
    return;
}

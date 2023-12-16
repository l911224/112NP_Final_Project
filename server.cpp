#include "unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
using namespace std;

int numOfMember = 0;
int watingRoomSockfd[4] = {0};
char watingRoomUserID[4][MAXLINE];
bool startGame = false;
struct timeval timeout;
timeout.tv_sec = 2;
timeout.tv_usec = 0;

void sig_chld(int signo);
char* xchg_user_data(int sockfd,SA *cliaddr,socklen_t *clilen);
void watingRoom();
void gameRoom(int sockfd[4], char userID[4][MAXLINE]);

int main(int argc, char **argc){
    int listenfd, connfd[4] = {0}, maxfd1 = -1;
    SA cliaddr[4], servaddr;
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

    // Create wating room
    if(Fork() == 0){
        Close(listenfd);
        watingRoom();
        exit(0);
    }
    
    for(;;){
        
        if(startGame){
            if(Fork() == 0){    // Fork child process to start a game
                Close(listenfd);
                gameRoom(watingRoomSockfd, watingRoomUserID);
                exit(0);
            }
            startGame = false;
            numOfMember = 0;
            memset(watingRoomSockfd, 0, sizeof(watingRoomSockfd));
            memset(watingRoomUserID, 0, sizeof(watingRoomUserID));
            memset(connfd, 0, sizeof(connfd));
            memset(userID, 0, sizeof(userID));
            continue;
        }

        char sendline[MAXLINE], recvline[MAXLINE];
        FD_ZERO(&rset);
        FD_SET(listenfd, &rset);
        maxfd1 = max(maxfd1, listenfd) + 1;
        int sel = Select(maxfd1, &rset, 0, 0, &timeout);
        if(sel == 0) continue;

        if(FD_ISSET(listenfd, &rset)){  // New connect arrived
            if ((connfd[numOfMember] = accept(listenfd, (SA *) &cliaddr[numOfMember], &clilen[numOfMember])) < 0) {
                if (errno == EINTR) continue
                else err_sys("accept error");
		    }

            if(Fork() == 0){    //Fork child process to get user ID
                close(listenfd);
                userID[numOfMember] = xchg_user_data(connfd[numOfMember]);
                watingRoomUserID[numOfMember] = userID[numOfMember];
                watingRoomSockfd[numOfMember] = connfd[numOfMember];
                exit(0);
            }
            numOfMember++;
        }
        

    }

    
}

void gameRoom(int sockfd[4], char userID[4][MAXLINE]){
    char sendline[MAXLINE], recvline[MAXLINE];
    for(int i = 0;i < 4; i++){
        Writen(sockfd[i], "Successfully enter a game!\n", MAXLINE);
        Close(sockfd[i]);
    }
    return;
}

void watingRoom(){
    int maxfd1 = -1;
    char sendline[MAXLINE], recvline[MAXLINE];
    fd_set = rset;
    for(;;){
        FD_ZERO(&rset);
        memset(sendline, 0, sizeof(sendline));
        memset(recvline, 0, sizeof(recvline));
        for(int i = 0; i < 4; i++){
            if(watingRoomSockfd[i] != 0){
                FD_SET(watingRoomSockfd[i], &rset);
                maxfd1 = max(maxfd1, watingRoomSockfd[i]);
            }
        }
        
        int sel = Select(maxfd1, &rset, 0, 0, &timeout);
        if(sel == 0) continue;

        for(int i = 0;i < 4; i++){
            if(FD_ISSET(watingRoomSockfd[i], &rset)){
                if(Read(watingRoomSockfd[i],recvline,MAXLINE) == 0 || recvline[0] == '2'){  //Fin recvd, broadcast to all players in the wating room.
                    Writen(watingRoomSockfd[i], "See you next time!\n", MAXLINE);
                    numOfMember--;
                    Close(watingRoomSockfd[i]);
                    watingRoomSockfd[i] = 0;    //Reset sockfd and id
                    memset(watingRoomUserID[i], 0, sizeof(watingRoomUserID[i]));
                    for(int j = 0; j < 4; j++){ // Broadcast to all users.
                        if(j == i || watingRoomSockfd[j] == 0) continue;
                        sprintf(sendline, "User: %s has left the room.\n\nThere are %d people in the room, waiting for somebody to start a game.", numOfMember);
                        Writen(watingRoomSockfd[j], sendline, MAXLINE);
                    }
                    continue;
                }
                else if(recvline[0] == '1'){    // Start a game.
                    for(int j = 0; j < 4; j++){
                        if(watingRoomSockfd[j] == 0) continue;
                        Writen(watingRoomSockfd[j], "Game start!\n", MAXLINE);
                        Close(watingRoomSockfd[j]);
                    }
                    startGame = true;
                    break;
                }

                sprintf(sendline, "(%s) %s", watingRoomUserID[i], recvline);
                for(int j = 0;j < 4; j++){
                    if(watingRoomSockfd[j] == 0 || i == j) continue;
                    Writen(watingRoomSockfd[j], sendline, MAXLINE);
                }
            }
        }
    }
}

char* xchg_user_data(int sockfd){
    char sendline[MAXLINE], recvline[MAXLINE];
    // Ask for user data
    Writen(sockfd, "Welcome to Yahtzee !\n\n[1] Create a new account\t[2] Login account\n", MAXLINE);
    Read(sockfd, recvline, MAXLINE);
    memset(recvline, 0, MAXLINE);
    if(recvline[0] == 1){   // Create new account
        char ID[MAXLINE], password[MAXLINE], dataPath[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        Read(sockfd, ID, MAXLINE);
        Writen(sockfd, "Enter your password : ", MAXLINE);
        Read(sockfd, password, MAXLINE);
        ofstream f;
        sprintf(dataPath, "/userdata/%s.txt", ID);
        f.open(dataPath);
        f << password << "0 0\n0 0\n0 0\n";
        Writen(sockfd, "Create successfully !\n", MAXLINE);
        f.close();
    }
    else if(recvline[1] == 2){  // Login account and show winning rate
        char ID[MAXLINE], password[MAXLINE], dataPath[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        Read(sockfd, ID, MAXLINE);
        Writen(sockfd, "Enter your password : ", MAXLINE);
        Read(sockfd, password, MAXLINE);
        ifstream f;
        sprintf(dataPath, "/userdata/%s.txt", ID);
        int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed;
        f.open(dataPath);
        f >> ID >> twoWin >> twoPlayed >> threeWin >> threePlayed >> fourWin >> fourPlayed;
        sprintf(sendline, "Login successfuly !\nYour winning rate is showed below :\nTwo players : %.2f\t\tThree players : %.2f\t\tFour players : %.2f", twoWin/twoPlayed, threeWin/threePlayed, fourWin/fourPlayed);
        Writen(sockfd, sendline, MAXLINE);
        f.close();
    }
    memset(sendline, 0, MAXLINE);
    if(numOfMember == 0){
        sprintf(sendline, "You are the first player !\nWait for 1 - 3 more players to start a game !\n");
    }
    else{
        sprintf(sendline, "There are %d players in this room. Waiting for the room keeper to start a game !\n", numOfMember + 1);
    }
    Writen(sockfd, sendline, MAXLINE);
    memset(sendline, 0, MAXLINE);
    Writen(sockfd, "In the wating room, you can type to chat with other players.\n\nPress [1] to start a game.\tPress[2] to exit the game.\n", MAXLINE);
    close(sockfd);
    return ID;
}

void sig_chld(int signo)
{
        pid_t   pid;
        int             stat;

        while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
        ;
        return;
}
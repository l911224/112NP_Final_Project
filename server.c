#include "unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#define _XOPEN_SOURCE 700

int numOfMember = 0;
int waitingRoomConnfd[4] = {0};
char waitingRoomUserID[4][MAXLINE];
int startGame = 0;
struct timeval timeout = {2,0};

void sigchld_handler(int signo);
char* xchg_user_data(int sockfd);
void *waitingRoom(void * argv);
void gameRoom(int sockfd[4], char userID[4][MAXLINE]);
int logout(char ID[MAXLINE]);
void dice(char diceToRoll[6], char* diceValue);
int logoutAll();

int main(int argc, char **argv){
    // Logout all users
    int l = logoutAll();
    if(l == 1) printf("Logout all success\n");
    else if(l == -1) printf("Logout all failed\n");
    int listenfd, connfd[20] = {0}, maxfdp1 = -1;
    struct sockaddr_in cliaddr[20], servaddr;
    char userID[20][MAXLINE];
    socklen_t clilen[20];
    // Shared memory set
    int shmFlag = 0, shmUserID = 0;
    if((shmFlag = shmget(IPC_PRIVATE, sizeof(int) * 2, IPC_CREAT | 0666)) < 0){
        perror("shmget");
        exit(1);
    }
    if((shmUserID = shmget(IPC_PRIVATE, sizeof(char) * MAXLINE, IPC_CREAT | 0666)) < 0){
        perror("shmget");
        exit(1);
    }
    //Attach the shm
    int *connfdFlag = NULL;
    char *IDBuffer = NULL;
    if((connfdFlag = (int*)shmat(shmFlag, NULL, 0)) == -1){
        perror("shmat");
        exit(1);
    }
    if((IDBuffer = (char *)shmat(shmUserID, NULL, 0)) == -1){
        perror("shmat");
        exit(1);
    }
    //Initialize shared matrix C
    *connfdFlag = -10;
    memset(IDBuffer, 0, sizeof(char[MAXLINE]));
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
    // Signal hangler
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sa.sa_flags |= SA_RESTART;
    Sigemptyset(&sa.sa_mask);
    sigaction(SIGCHLD, &sa, NULL);

    // Create wating room thread
    pthread_t wr;
    pthread_create(&wr, NULL, waitingRoom, NULL);
    for(;;){
        if(startGame){  // Start a game
            if(Fork() == 0){    // Fork child process to start a game
                Close(listenfd);
                gameRoom(waitingRoomConnfd, waitingRoomUserID);
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

        if(*connfdFlag != -10){   // Find space and add user to waiting room
            if(*connfdFlag >=  1000){
                connfd[*connfdFlag - 1000] = 0;
                memset(IDBuffer, 0, sizeof(IDBuffer));
                *connfdFlag = -10;
                printf("Connfd reset\n");
                continue;
            }
            numOfMember++;
            int freeSpace = 0;
            for(int i = 0; i < 4; i++){
                if(waitingRoomConnfd[i] == 0){
                    freeSpace = i;
                    break;
                }
            }
            strcpy(waitingRoomUserID[freeSpace], IDBuffer);
            waitingRoomConnfd[freeSpace] = connfd[*connfdFlag];
            char sendline[MAXLINE];
            // Broadcast to all players
            sprintf(sendline, "%s entered the room.\tThere are %d players waiting now.\n\n", IDBuffer, numOfMember);
            for(int i = 0; i < 4; i++){
                if(waitingRoomConnfd[i] == 0 || i == *connfdFlag) continue;
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
        if (sel == 0) continue;
        else if (sel < 0) {
            if (errno == EINTR) {
                continue;
            }
            // Handle other select error
            perror("select error");
            break;
        }
        int freeSpace_Connfd = 0;
        for(int i = 0; i < 20; i++){
            if(connfd[i] == 0){ 
                freeSpace_Connfd = i;
                break;
            }
        }
        if(FD_ISSET(listenfd, &rset)){  // New connect arrived
            if ((connfd[freeSpace_Connfd] = accept(listenfd, (SA *) &cliaddr[freeSpace_Connfd], &clilen[freeSpace_Connfd])) < 0) {
                if (errno == EINTR) continue;
                else perror("accept error");
		    }
            int num = freeSpace_Connfd;
            if(Fork() == 0){    //Fork child process to get user ID
                Close(listenfd);
                // Use shared memory to store data
                strcpy(IDBuffer, xchg_user_data(connfd[num]));
                if(!strcmp(IDBuffer, "error")){
                    printf("Client disconnected\n");
                    *connfdFlag = 1000 + num;
                    exit(0);
                }
                *connfdFlag = num;
                printf("Successful get data Child Flag = %d and ID = %s and num = %d\n",*connfdFlag, IDBuffer, num);
                exit(0);
            }
        }
    }

    pthread_join(wr, NULL);
    return 0;
}


/*
    gameRoom control message:
    r:01101(roll dice)
    
*/



void gameRoom(int sockfd[4], char userID[4][MAXLINE]){
    printf("gameRoom and \n");
    char sendline[MAXLINE], recvline[MAXLINE], scoreTable[4][19] = {-1}, diceValue[6] = {0}, diceToRoll[6] = {0};
    int maxfdp1 = -1, stepCount = 0, turn = 0, doneFlag = 1;
    fd_set rset;
    FD_ZERO(&rset);
    int numOfPlayer = 0;
    for(;;){
        // Fd set
        memset(sendline, 0, sizeof(sendline));
        memset(recvline, 0, sizeof(recvline));
        numOfPlayer = 0;
        for(int i = 0;i < 4; i++){
            if(sockfd[i] == 0) continue;
            FD_SET(sockfd[i], &rset);
            numOfPlayer++;
            maxfdp1 = max(maxfdp1, sockfd[i]);
        }
        if(numOfPlayer == 0) break;
        maxfdp1 += 1;
        int sel = select(maxfdp1, &rset, 0, 0, &timeout);
        if (sel == 0) continue;
        else if (sel < 0) {
            if (errno == EINTR) {
                continue;
            }
            // Handle other select error
            perror("select error");
            break;
        }
        // Recv messeages
        for(int i = 0; i < 4; i++){
            if(FD_ISSET(sockfd[i], &rset)){
                printf("got message\n");
                if(Read(sockfd[i], recvline, MAXLINE) == 0 || recvline[0] == 'Q'){  // Deal with ctrl + c and 'Q'
                    Writen(sockfd[i], "m:See you next time!\n", MAXLINE);
                    numOfPlayer--;
                    Close(sockfd[i]);
                    for(int j = 0; j < 4; j++){ // Broadcast to all users.
                        if(j == i || sockfd[j] == 0) continue;
                        sprintf(sendline, "m:Player %s has left the room.\n\n", userID[i]);
                        Writen(sockfd[j], sendline, MAXLINE);
                    }
                    logout(userID[i]);   //logout
                    sockfd[i] = 0;    //Reset sockfd and id
                    memset(userID[i], 0, sizeof(userID[i]));
                }
                else if(recvline[0] == 'r' && recvline[1] == ':'){  //r:01101 means to roll NO.2 3 5 dices
                    sscanf(recvline, "%*[^:]:%s\n", diceToRoll);
                    diceToRoll[5] = '\0';
                    printf("ready to roll:%s\n", diceToRoll);
                    dice(diceToRoll, &diceValue);
                    sprintf(sendline, "t:%d\nv:%s\n", turn, diceValue);
                    printf("rolled:%s\n", diceValue);
                    for(int j = 0; j < 4; j++){
                        if(sockfd[j] == 0) continue;
                        Writen(sockfd[j], sendline, MAXLINE);
                    }
                }
            }
        }
        
        // Send control messages
        // if(doneFlag){
        //     doneFlag = 0;
        //     memset(diceValue, 0, sizeof(diceValue));
        //     dice(diceToRoll, &diceValue);
        //     printf("dice rolled : %s\n", diceValue);
        //     sprintf(sendline, "t:%d\nv:%s\n", turn, diceValue);
        //     for(int i = 0; i < 4; i++){
        //         if(sockfd[i] == 0) continue;
        //         Writen(sockfd[i], sendline, MAXLINE);
        //     }
        // }
        
    }
    
    
    for(int i = 0;i < 4; i++){  //logout
        logout(userID[i]);
        if(sockfd[i] == 0) continue;
        Close(sockfd[i]);
    }
    return;
}

void *waitingRoom(void *argv){
    int maxfdp1 = -1;
    char sendline[MAXLINE], recvline[MAXLINE];
    fd_set rset;
    FD_ZERO(&rset);
    for(;;){
        if(numOfMember == 3) goto GAMESTART;
        maxfdp1 = -1;
        FD_ZERO(&rset);
        memset(sendline, 0, sizeof(sendline));
        memset(recvline, 0, sizeof(recvline));
        for(int i = 0; i < 4; i++){
            //printf("waitingRoomSockfd[%d] = %d\n",i,waitingRoomSockfd[i]);
            if(waitingRoomConnfd[i] != 0){
                FD_SET(waitingRoomConnfd[i], &rset);
                maxfdp1 = max(maxfdp1, waitingRoomConnfd[i]);
            }
        }
        
        maxfdp1 += 1;
        int sel = select(maxfdp1, &rset, 0, 0, &timeout);
        if(sel == 0) continue;

        for(int i = 0;i < 4; i++){
            if(FD_ISSET(waitingRoomConnfd[i], &rset)){
                if(Read(waitingRoomConnfd[i], recvline, MAXLINE) == 0 || recvline[0] == '4'){  //Fin recvd, broadcast to all players in the wating room.
                    Writen(waitingRoomConnfd[i], "See you next time!\n\n", MAXLINE);
                    numOfMember--;
                    Close(waitingRoomConnfd[i]);
                    for(int j = 0; j < 4; j++){ // Broadcast to all users.
                        if(j == i || waitingRoomConnfd[j] == 0) continue;
                        sprintf(sendline, "Player %s has left the room.\n\nThere are %d players in the room, waiting for somebody to start a game.\n\n", waitingRoomUserID[i],numOfMember);
                        Writen(waitingRoomConnfd[j], sendline, MAXLINE);
                    }
                    logout(waitingRoomUserID[i]);   //logout
                    waitingRoomConnfd[i] = 0;    //Reset sockfd and id
                    memset(waitingRoomUserID[i], 0, sizeof(waitingRoomUserID[i]));
                    continue;
                }
                else if(recvline[0] == '1'){    // Start a game.
                    if(numOfMember < 2){    // Check player number is enough or not 
                        Writen(waitingRoomConnfd[i], "Number of players are not enuogh. Wait for 1-3 more players to start!\n\n", MAXLINE);
                        continue;
                    }
                    GAMESTART:
                    for(int j = 0; j < 4; j++){
                        if(waitingRoomConnfd[j] == 0) continue;
                        Writen(waitingRoomConnfd[j], "Game start!\n\n", MAXLINE);
                    }
                    startGame = 1;
                    break;
                }
                else if(recvline[0] == '2'){    // Show game history
                    char dataPath[MAXLINE], password[MAXLINE];
                    int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, Using;
                    sprintf(dataPath, "userdata/%s.txt", waitingRoomUserID[i]);
                    FILE *f;
                    f = fopen(dataPath, "r");
                    if(f == NULL) perror("File open error");
                    fscanf(f, "%s%d %d\n%d %d\n%d %d\n%d", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed, &Using);
                    sprintf(sendline, "Your game history are showed below (win / played):\n\n2 players game : %d / %d\n3 players game : %d / %d\n4 players game : %d / %d\n\n", twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed);
                    fclose(f);
                    Writen(waitingRoomConnfd[i], sendline, MAXLINE);
                    continue;
                }
                else if(recvline[0] == '3'){
                    char tmp[MAXLINE];
                    for(int j = 0; j < 4; j++){
                        if(waitingRoomConnfd[j] == 0){
                            sprintf(tmp, "P%d : (Empty)    ", j);
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
                for(int j = 0;j < 4; j++){
                    if(waitingRoomConnfd[j] == 0 || i == j) continue;
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

char* xchg_user_data(int sockfd){   
    char sendline[MAXLINE], recvline[MAXLINE];
    static char ID[MAXLINE];
    // Ask for user data
    int n;
    Writen(sockfd, "Welcome to Yahtzee!\n\n", MAXLINE);
    CHOOSE:
    Writen(sockfd, "[1] Create a new account\t[2] Login account\n\n", MAXLINE);
    n = Read(sockfd, recvline, MAXLINE);
    if(n == 0) goto DISCONNECT;
    if(recvline[0] == '1'){   // Create new account and check the ID exists or not
        NEW:
        char password[MAXLINE], dataPath[MAXLINE], fileContent[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        n = Read(sockfd, ID, MAXLINE);
        if(n == 0) goto DISCONNECT;
        FILE *f, *check;
        ID[strlen(ID) - 1] = 0;
        sprintf(dataPath, "userdata/%s.txt", ID);
        printf("dataPath:%s\n",dataPath);
        check = fopen(dataPath, "r");
        if(check){  // If ID already exist, goto login
            Writen(sockfd, "ID already exists. Please create a new one or login your account.\n\n", MAXLINE);
            goto CHOOSE;
        }
        PW:
        Writen(sockfd, "Enter your password (At least 1 character): ", MAXLINE);
        n = Read(sockfd, password, MAXLINE);
        if(n == 0) goto DISCONNECT;

        if(!strcmp(password, "\n")){
            Writen(sockfd, "Password must be at least 1 character, please enter password again.\n\n", MAXLINE);
            goto PW;
        }
        printf("ID %s PW %s\n",ID,password);
        f = fopen(dataPath, "w");
        //printf("Seccessful open file\n");
        sprintf(fileContent, "%s%d %d\n%d %d\n%d %d\n%d", password,0,0,0,0,0,0,1);
        fprintf(f, fileContent);
        Writen(sockfd, "\nCreate successfully!\n\n", MAXLINE);
        fclose(f);
    }
    else if(recvline[0] == '2'){  // Login account and show winning rate
        LOGIN:
        printf("Login\n");
        char password[MAXLINE], dataPath[MAXLINE], fileContent[MAXLINE];
        Writen(sockfd, "Enter your ID : ", MAXLINE);
        n = Read(sockfd, ID, MAXLINE);
        if(n == 0) goto DISCONNECT;
        ID[strlen(ID) - 1] = 0;
        FILE *f;
        sprintf(dataPath, "userdata/%s.txt", ID);
        int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, Using;
        f = fopen(dataPath, "r");
        if(f == NULL){
            Writen(sockfd, "ID does not exist. Please create a new one or login another ID\n\n", MAXLINE);
            goto CHOOSE;
        }
        fscanf(f, "%s%d %d\n%d %d\n%d %d\n%d", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed, &Using);
        if(Using){
            Writen(sockfd, "This ID is login now, please create a new one or login another ID\n\n", MAXLINE);
            goto CHOOSE;
        }
        PW_AGAIN:
        memset(recvline, 0, sizeof(recvline));
        Writen(sockfd, "Enter your password : ", MAXLINE);
        n = Read(sockfd, recvline, MAXLINE);
        if(n == 0) goto DISCONNECT;
        recvline[strlen(recvline) - 1] = '\0';
        if(strcmp(password, recvline) != 0){ 
            Writen(sockfd, "Password is wrong, please try again\n\n", MAXLINE);
            printf("pw:%s and revc:%s",password,recvline);
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
    }
    else{
        Writen(sockfd, "Illegal operation. Please follow the instruction.\n\n", MAXLINE);
        goto CHOOSE;
    }
    memset(recvline, 0, MAXLINE);
    memset(sendline, 0, MAXLINE);
    if(numOfMember == 0){
        sprintf(sendline, "You are the first player! Wait for 1 - 3 more players to start a game!\n\n");
    }
    else{
        sprintf(sendline, "There are %d players in waiting room. Wait for the room keeper to start a game!\n\n", numOfMember + 1);
    }
    Writen(sockfd, sendline, MAXLINE);
    memset(sendline, 0, MAXLINE);
    Writen(sockfd, "In the waiting room, you can type to chat with other players.\n\nPress [1] to start a game.\nPress [2] to show your game history.\nPress [3] to show players in waiting room.\nPress [4] to exit the game.\n\n", MAXLINE);
    Close(sockfd);
    return ID;
    DISCONNECT:
    logout(ID);
    Close(sockfd);
    return "error";
}

int logout(char ID[MAXLINE]){
    FILE *f;
    char dataPath[MAXLINE], password[MAXLINE], fileContent[MAXLINE];
    int twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, Using;
    sprintf(dataPath, "userdata/%s.txt", ID);
    f = fopen(dataPath, "r");
    if(f == NULL) return 0;
    fscanf(f, "%s\n%d %d\n%d %d\n%d %d\n%d", &password, &twoWin, &twoPlayed, &threeWin, &threePlayed, &fourWin, &fourPlayed, &Using);
    fclose(f);
    f = fopen(dataPath, "w");   // Clean file
    sprintf(fileContent, "%s\n%d %d\n%d %d\n%d %d\n%d", password, twoWin, twoPlayed, threeWin, threePlayed, fourWin, fourPlayed, 0);
    fprintf(f, fileContent);
    printf("Logout %s successfully.\n", ID);
    fclose(f);
    return 1;
}

void dice(char diceToRoll[6], char* diceValue){
    srand(time(NULL));
    for(int i = 0;i < 5; i++){
        if(diceToRoll[i] == '0') continue;
        int v = rand() % 6 + 1;
        *(diceValue + i) = v + '0';
    }
    *(diceValue+ 5) = '\0';
    printf("rolled successfully\n");
}

void sigchld_handler(int signo) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
    return;
}

int logoutAll(){
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

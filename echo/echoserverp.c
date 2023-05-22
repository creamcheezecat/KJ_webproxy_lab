#include "csapp.h"

/* 아직 테스트 x */

void echo(int connfd)

void sigchld_handler(int sig)
{
    while(waitpid(-1,0,WNOHANG) > 0){
        ;
    }
    return ;
}

int main(int argc, char **argv){

    int listenfd, connfd , port;               
    socklen_t clientlen = sizeof(struct sockaddr_in);      
    struct sockaddr_in clientaddr;                  
    
    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    
    port = atoi(argv[1]);
    Signal(SIGCHLD, sigchld_handler);

    listenfd = Open_listenfd(port);  // 주어진 포트로 소켓 생성 및 바인딩 후 리스닝
    
    while(1){
    
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        if(Fork() == 0){
            Close(listenfd);
            echo(connfd);
            Close(connfd);
            exit(0);
        } 

        Close(connfd);
    }
    exit(0);
}
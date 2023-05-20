#include "csapp.h"

void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, connfd); // RIO 구조체 초기화

    /*
    클라이언트로부터 데이터를 읽어오고, 서버가 받은 바이트 수를 출력한 후 해당 데이터를 
    다시 클라이언트에게 전송하는 역할을 합니다. 
    Rio_readlineb() 함수는 RIO 구조체를 사용하여 connfd 소켓에서 데이터를 읽어옵니다. 
    Rio_writen() 함수를 사용하여 읽어온 데이터를 다시 클라이언트에게 전송합니다.
    */
    while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
        printf("server received %d bytes\n", (int)n); // 서버가 받은 바이트 수 출력
        Rio_writen(connfd, buf , n);                  // 클라이언트에게 데이터 전송
    }
}
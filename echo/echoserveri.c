#include "csapp.h"

int main(int argc, char **argv){
    /* 
    서버의 리스닝 소켓과 클라이언트와의 연결을 위한 소켓 디스크립터 변수
    소켓 디스크립터 = 리눅스 시스템이 소켓에 접근할 수 있는 매개체
    */
    int listenfd, connfd;               
    socklen_t clientlen;                                 // 클라이언트 주소 길이 변수
    struct sockaddr_storage clientaddr;                  // 클라이언트 주소 저장을 위한 구조체
    char client_hostname[MAXLINE], client_port[MAXLINE]; // 클라이언트 ip 주소 문자열 | 클라이언트 포트 번호문자얼

    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    
    listenfd = Open_listenfd(argv[1]);  // 주어진 포트로 소켓 생성 및 바인딩 후 리스닝
    
    while(1){
        clientlen = sizeof(struct sockaddr_storage);
        
        // 클라이언트 연결
        /*
        Accept 함수는 클라이언트의 연결을 수락하고 연결된 클라이언트와 통신하기 위해
        새로운 소켓 디스크립터를 반환
        */
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);

        /*
        클라이언트의 IP 주소를 문자열로 변환
        clinentaddr 에 있는 클라이언트의 주소 정보를 Getnameinfo 함수를 이용해서
        client_hostname 에는 호스트명이 문자열로 저장되고
        client_post 에는 포트 번호가 문자열로 저장된다
        */
        Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0);
        
        // 클라이언트 IP 주소 출력
        printf("Connected to (%s, %s)\n" , client_hostname, client_port);
       
        echo(connfd);
        // 클라이언트와의 연결 종료
        Close(connfd);
    }
    exit(0);
}
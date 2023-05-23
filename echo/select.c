#include "csapp.h"

/* 테스트 o */

void echo(int connfd);
void command(void);

int main(int argc, char **argv){
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    fd_set read_set, ready_set;

    if (argc != 2){
        fprintf(stderr , "usage: %s <port>\n",argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    // 파일 디스크립터 집합 초기화
    FD_ZERO(&read_set);
    FD_SET(STDIN_FILENO, &read_set);
    FD_SET(listenfd, &read_set);

    while(1){
        ready_set = read_set;
        Select(listenfd+1, &ready_set, NULL,NULL,NULL);

        // 표준 입력 감지
        if(FD_ISSET(STDIN_FILENO, &ready_set)){
            command();
        }
        // 소켓 연결 감지
        if(FD_ISSET(listenfd, &ready_set)){
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            echo(connfd);
            Close(connfd);
        }
    }
}
// 표준 입력으로부터 명령을 읽고 출력합니다.
void command(void){
    char buf[MAXLINE];
    
    // 입력 받아서 출력
    if(!Fgets(buf, MAXLINE, stdin)){
        exit(0);
    }
    printf("%s", buf);
}

// 이 코드는 동시성 Echo 서버를 구현합니다.

// main 함수에서는 파일 디스크립터 집합을 초기화하고 무한 루프를 돌며 입출력 이벤트를 감지합니다.

// 표준 입력(STDIN_FILENO)과 listenfd(클라이언트 연결을 수락하는 소켓)을 집합에 추가하고 Select 함수를 사용하여 이벤트를 감지합니다.

// 표준 입력 감지 시 command 함수를 호출하여 입력 받고 출력합니다.

// 클라이언트 연결 감지 시 Accept 함수를 사용하여 연결을 수락하고 echo 함수를 호출하여 데이터를 읽어 에코한 뒤 연결을 종료합니다.

// 이를 통해 동시성을 유지하며 클라이언트의 요청을 처리할 수 있습니다.
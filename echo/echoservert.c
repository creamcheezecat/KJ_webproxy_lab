#include "csapp.h"

/* 테스트 o */

void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv)
{
    int listenfd, *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    // 지정된 포트로 소켓 생성 및 연결 대기
    listenfd = Open_listenfd(argv[1]);

    while(1){
        clientlen = sizeof(struct sockaddr_storage);

        // 동적으로 클라이언트와의 연결 소켓 디스크립터를 저장할 메모리 할당
        connfdp = Malloc(sizeof(int));

         // 클라이언트의 연결 수락
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);

        // 새로운 스레드를 생성하여 클라이언트와 통신을 수행
        Pthread_create(&tid,NULL,thread,connfdp);
        // 이곳에서 새로운 클라이언트의 연결이 수락되고 새로운 스레드가 생성됩니다.
        // 이렇게하면 여러 클라이언트와 동시에 통신할 수 있습니다.
    }
}

void *thread(void *vargp)
{
    // connfd로 전달된 값을 가져옴
    int connfd = *((int *)vargp);
    // 스레드 분리
    Pthread_detach(pthread_self());
     // 메모리 해제
    Free(vargp);
    // 클라이언트와의 통신 수행
    echo(connfd);
    // 연결 종료
    Close(connfd);
    return NULL;
}

// 이 코드는 다중 클라이언트 서버를 구현합니다.

// 주어진 포트로 연결을 수락하고, 
// 각 클라이언트와 독립적인 스레드를 생성하여 통신합니다.

// 새로운 클라이언트의 연결이 수락되면, 
// 연결 소켓 디스크립터를 동적으로 할당하고 스레드를 생성합니다.

// 생성된 스레드는 클라이언트와 통신을 수행한 후 연결을 종료합니다.

// 스레드는 자동으로 리소스를 회수하므로 메인 스레드는 다른 클라이언트의 연결을 수락하는 데 집중할 수 있습니다.
#include "csapp.h"

/* 테스트 o */

typedef struct {
    int maxfd;
    fd_set read_set;
    fd_set ready_set;
    int nready;
    int maxi;
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];

} pool;

int byte_cnt = 0;

void init_pool(int listenfd, pool *p);

/* I/O 다중화에 기초한 동시성 echo 서버 */
int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    static pool pool;

    if (argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);

    while(1){
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd + 1 , &pool.ready_set ,NULL, NULL,NULL);

        if(FD_ISSET(listenfd, &pool.ready_set)){
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);
        }

        check_client(&pool);
    }
}

/* 살아있는 클라이언트의 pool을 초기화*/
void init_pool(int listenfd, pool *p)
{
    int i;
    p->maxi = -1;
    for (i=0; i<FD_SETSIZE; i++){
        p->clientfd[i] = -1;
    }

    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);

}

/* add_client : 풀에 새 클라이언트 연결을 추가*/
void add_client(int connfd, pool *p)
{
    int i;
    p->nready--;
    for (i = 0; i < FD_SETSIZE; i++){
        if (p->clientfd[i] < 0){
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            FD_SET(connfd, &p->read_set);

            if (connfd > p->maxfd){
                p->maxfd = connfd;
            }
            if (i > p->maxi){
                p->maxi = i;
            }
            break;
        }
    }
    if(i == FD_SETSIZE){
        app_error("add_client error : Too many clients");
    }
}

/* 준비된 클라이언트 연결들을 서비스*/
void check_client(pool *p)
{
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for (i = 0; (i <= p->maxi) && (p->nready > 0); i++){
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if((connfd > 0 ) && (FD_ISSET(connfd, &p->ready_set))){
            p->nready--;
            if((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd %d\n" , n, byte_cnt, connfd);
                Rio_writen(connfd, buf, n);
            }
            
            else{
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1;
            }
        }
    }
}

// 이 코드는 I/O 다중화를 기반으로 동작하는 동시성 Echo 서버를 구현합니다.
// 클라이언트의 연결을 효율적으로 관리하기 위해 풀(pool)이라는 구조체를 사용합니다.
// 풀은 클라이언트 소켓 디스크립터를 저장하고, I/O 다중화를 위한 
// 파일 디스크립터 집합과 관련된 정보를 유지합니다.

// main 함수에서는 초기화 작업 후 무한 루프를 돌며 연결 수락과 클라이언트 서비스를 담당합니다.

// init_pool 함수는 풀을 초기화합니다. 
// 초기에는 클라이언트 연결이 없으므로 read_set에는 listenfd만 포함됩니다.

// add_client 함수는 풀에 새 클라이언트 연결을 추가합니다. 
// 비어있는 clientfd 슬롯에 연결 정보를 저장하고, read_set에 추가합니다.

// check_client 함수는 준비된 클라이언트 연결을 서비스합니다. 

// ready_set에 있는 클라이언트를 확인하고 데이터를 읽어 에코하며, 
// 연결이 종료된 경우 해당 클라이언트 정보를 제거합니다.

// 이를 통해 동시성을 유지하며 다중 클라이언트 요청을 처리할 수 있습니다.
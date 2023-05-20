#include "csapp.h"

int main(int argc, char **argv){
    int clientfd;                       // 클라이언트 소켓 디스크립터 변수
    /* host 와 port 포인터변수는 
    커맨드 라인 인자로 입력된 호스트와 포트 번호를 저장하는 변수*/
    char *host, *port, buf[MAXLINE];    
    rio_t rio;                          // RIO 입출력 구조체

    if (argc != 3){
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    /* 서버에 연결을 시도하는 클라이언트 소켓 생성 */
    clientfd = Open_clientfd(host,port);

    Rio_readinitb(&rio, clientfd); // RIO 구조체 초기화

    /*
    입력 스트림(stdin)으로부터 한 줄씩 문자열을 읽어오는 반복문입니다. 
    Fgets() 함수는 입력 스트림에서 문자열을 읽어와서 buf 버퍼에 저장합니다. 
    읽어온 문자열의 길이는 MAXLINE보다 작거나 같아야 합니다.
    */
    while(Fgets(buf, MAXLINE, stdin) != NULL){
        Rio_writen(clientfd, buf, strlen(buf)); // 서버로 입력한 데이터 전송
        Rio_readlineb(&rio, buf , MAXLINE);     // 서버로부터 응답을 읽어들임
        Fputs(buf, stdout);                     // 서버 응답을 표준 출력에 출력
        if(strcmp(buf, "quit\n") == 0){         // quit 입력시 종료
            break;
        }
    }

    Close(clientfd);
    exit(0);
}
#include <stdio.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
static const int is_local_test = 0;
/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3\r\n";

void doit(int fd);
void *thread(void *vargp);
void read_requesthdrs(rio_t *rp, void *buf, int serverfd, char *hostname, char *port);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void parse_uri(char *uri, char *hostname, char *port, char *path);

int main(int argc, char **argv) {
  int listenfd,  *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  pthread_t tid;

  if (argc != 2) {
    // f(파일)printf => 파일을 읽어주는 것, stderr(식별자 2) = standard error
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    
    connfdp = Malloc(sizeof(int));

    /* 클라이언트에게서 받은 연결 요청을 accept 한다. connfd = 서버 연결 식별자*/
    *connfdp = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept

    /* 연결이 성공했다는 메세지를 위해. Getnameinfo를 호출하면서 hostname과 port가 채워진다.*/
    /* 마지막 부분은 flag로 0: 도메인 네임, 1: IP address */
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    Pthread_create(&tid,NULL,thread,connfdp);

    printf("==============================================\n\n");
  }
  return 0;
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
    doit(connfd);
    // 연결 종료
    Close(connfd);
    return NULL;
}

void doit(int clientfd)
{
  char method[MAXLINE] , uri[MAXLINE] ,hostname[MAXLINE], port[MAXLINE], version[MAXLINE] , path[MAXLINE];
  char response_buf[MAXLINE],request_buf[MAXLINE];
  int serverfd, content_length , *response_ptr;
  rio_t request_rio, response_rio;

  /* Request line 읽기 (cilent -> proxy)*/
  Rio_readinitb(&request_rio, clientfd);
  Rio_readlineb(&request_rio,request_buf,MAXLINE);
  printf("Request headers:\n %s\n", request_buf);

  /* 요청 라인 파싱을 통해 method, uri,hostname,port,path*/
  sscanf(request_buf, "%s %s", method, uri);

  parse_uri(uri,hostname,port,path);

  sprintf(request_buf, "%s %s %s\r\n", method, path , "HTTP/1.0");

  if(strcasecmp(method, "GET") && strcasecmp(method, "HEAD")){
    clienterror(clientfd, method, "501", "Not implemented", "Tiny does not implement this method");
    return;
  }
  printf("소켓 생성전 hostname : %s port : %s\n", hostname, port);
  // Server 소켓 생성

  if (!strcasecmp(hostname,"")){ // ip 주소가 비어있다면 기본 ip 주소 잡아줌
    strcpy(hostname,"13.209.26.193");
  }
  printf("소켓 생성전 기본 호스트 넣은 후 hostname : %s port : %s\n", hostname, port);
  serverfd = Open_clientfd(hostname, port);
  printf("소켓 생성후\n");

  if (serverfd < 0)
  {
    clienterror(serverfd, method, "502", "Bad Gateway", "Failed to establish connection with the end server");
    return;
  }
  
  Rio_writen(serverfd, request_buf, strlen(request_buf));
  printf("%s \n", request_buf);
  /* Request Header 읽기 & 전송 (client ->  proxy -> server) */
  read_requesthdrs(&request_rio, request_buf, serverfd, hostname, port);
  
  /* Response Header 읽기 & 전송 (server -> proxy ->  client) */
  Rio_readinitb(&response_rio, serverfd);
  while (strcmp(response_buf, "\r\n"))
  {
    Rio_readlineb(&response_rio, response_buf, MAXLINE);
    if (strstr(response_buf, "Content-length")) // Response Body 수신에 사용하기 위해 Content-length 저장
      content_length = atoi(strchr(response_buf, ':') + 1);
    Rio_writen(clientfd, response_buf, strlen(response_buf));
  }

  /* Response Body 읽기 & 전송 (server -> proxy -> client) */
  response_ptr = malloc(content_length);
  Rio_readnb(&response_rio, response_ptr, content_length);
  Rio_writen(clientfd, response_ptr, content_length); // Client에 Response Body 전송
  free(response_ptr); // 캐싱하지 않은 경우만 메모리 반환

  Close(serverfd);
}

void parse_uri(char *uri, char *hostname, char *port, char *path)
{
  // host_name의 시작 위치 포인터: '//'가 있으면 //뒤(ptr+2)부터, 없으면 uri 처음부터
  char *hostname_ptr = strstr(uri, "//") ? strstr(uri, "//") + 2 : uri;
  char *port_ptr = strchr(hostname_ptr, ':'); // port 시작 위치 (없으면 NULL)
  char *path_ptr = strchr(hostname_ptr, '/'); // path 시작 위치 (없으면 NULL)

  strcpy(path, path_ptr);




  if (port_ptr) // port 있는 경우
  {
    strncpy(port, port_ptr + 1, path_ptr - port_ptr - 1); 
    strncpy(hostname, hostname_ptr, port_ptr - hostname_ptr);
  }
  else // port 없는 경우
  {
    if (is_local_test)
      strcpy(port, "80"); // port의 기본 값인 80으로 설정
    else
      strcpy(port, "8000");
    strncpy(hostname, hostname_ptr, path_ptr - hostname_ptr);
  }


}

// Request Header를 읽고 Server에 전송하는 함수
// 필수 헤더가 없는 경우에는 필수 헤더를 추가로 전송
void read_requesthdrs(rio_t *request_rio, void *request_buf, int serverfd, char *hostname, char *port)
{
  int is_host_exist;
  int is_connection_exist;
  int is_proxy_connection_exist;
  int is_user_agent_exist;

  Rio_readlineb(request_rio, request_buf, MAXLINE); // 첫번째 줄 읽기
  while (strcmp(request_buf, "\r\n"))
  {
    if (strstr(request_buf, "Proxy-Connection") != NULL)
    {
      sprintf(request_buf, "Proxy-Connection: close\r\n");
      is_proxy_connection_exist = 1;
    }
    else if (strstr(request_buf, "Connection") != NULL)
    {
      sprintf(request_buf, "Connection: close\r\n");
      is_connection_exist = 1;
    }
    else if (strstr(request_buf, "User-Agent") != NULL)
    {
      sprintf(request_buf, user_agent_hdr);
      is_user_agent_exist = 1;
    }
    else if (strstr(request_buf, "Host") != NULL)
    {
      is_host_exist = 1;
    }
    printf("%s \n", request_buf);
    Rio_writen(serverfd, request_buf, strlen(request_buf)); // Server에 전송
    Rio_readlineb(request_rio, request_buf, MAXLINE);       // 다음 줄 읽기
  }

  // 필수 헤더 미포함 시 추가로 전송
  if (!is_proxy_connection_exist)
  {
    sprintf(request_buf, "Proxy-Connection: close\r\n");
    Rio_writen(serverfd, request_buf, strlen(request_buf));
  }
  if (!is_connection_exist)
  {
    sprintf(request_buf, "Connection: close\r\n");
    Rio_writen(serverfd, request_buf, strlen(request_buf));
  }
  if (!is_host_exist)
  {
    sprintf(request_buf, "Host: %s:%s\r\n", hostname, port);
    Rio_writen(serverfd, request_buf, strlen(request_buf));
  }
  if (!is_user_agent_exist)
  {
    sprintf(request_buf, user_agent_hdr);
    Rio_writen(serverfd, request_buf, strlen(request_buf));
  }

  sprintf(request_buf, "\r\n"); // 종료문
  printf("%s \n", request_buf);
  Rio_writen(serverfd, request_buf, strlen(request_buf));
  return;
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  // 에러 Bdoy 생성
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor="
                "ffffff"
                ">\r\n",
          body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  // 에러 Header 생성 & 전송
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));

  // 에러 Body 전송
  Rio_writen(fd, body, strlen(body));
}
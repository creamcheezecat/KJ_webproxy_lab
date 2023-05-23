/* $begin tinymain */
/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 *
 * Updated 11/2019 droh
 *   - Fixed sprintf() aliasing issue in serve_static(), and clienterror().
 */

#include "csapp.h"


void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);
void echo(int connfd);
void *thread(void *vargp);
/* Tiny는 반복실행 서버로 명령줄에서 넘겨받은 포트로의 연결 요청을 듣는다*/
/* 
포트 번호를 인자로 받아 클라이언트의 요청이 올 때마다 
새로 연결 소켓을 만들어 doit 함수를 호출
ex) 입력 ./tiny 24230   =>  argc = 2, argv[0] = tiny, argv[1] = 8000
*/
int main(int argc, char **argv) // argc = 인자 개수, argv = 인자 배열
{ 
  int listenfd,  *connfdp;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  // 클라이언트에서 연결 요청을 보내면 알 수 있는 클라이언트 연결 소켓 주소

   pthread_t tid;

  /* 명령 줄 인수 확인 */
  if (argc != 2) {
    // f(파일)printf => 파일을 읽어주는 것, stderr(식별자 2) = standard error
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  /* 해당 포트 번호에 해당하는 듣기 소켓 식별자를 열어준다. */
  listenfd = Open_listenfd(argv[1]);

  /* 클라이언트의 요청이 올 때마다 새로 연결 소켓을 만들어 doit()호출 */
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

    
    /* doit 함수를 실행 */
    // doit(connfd);   // line:netp:tiny:doit

    /* 11.6.A */
    //echo(connfd);

    /* 서버 연결 식별자를 닫아준다. */
    
    // Close(connfd);  // line:netp:tiny:close

    printf("==============================================\n\n");
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
    doit(connfd);
    // 연결 종료
    Close(connfd);
    return NULL;
}

/* 한개의 HTTP 트랜잭션을 처리한다.*/
/* doit: 클라이언트의 요청 라인을 확인해 정적, 동적 컨텐츠인지 확인하고 각각의 서버에 보낸다. */
void doit(int fd)
{
  int is_static; // 정적(static) 콘텐츠 여부를 나타내는 변수입니다. 
  struct stat sbuf;  // 파일의 상태 정보를 저장하기 위한 구조체

  /* 
   * buf = 요청(request) 메시지를 저장하기 위한 문자열 버퍼
   * method , uri , version = 요청 메시지에서 추출한 메서드, URI 및 HTTP 버전 정보를 저장하기 위한 문자열 배열
   */
  char buf[MAXLINE], method[MAXLINE] , uri[MAXLINE] , version[MAXLINE]; 

  /*
   * filename = 요청된 파일의 경로를 저장하는 문자열 배열
   * cgiargs = CGI 프로그램에 전달할 인수들을 저장하기 위한 문자열 배열
   */
  char filename[MAXLINE], cgiargs[MAXLINE];

  rio_t rio; // Rio package를 사용하여 소켓으로부터 데이터를 읽어오기 위한 버퍼

  /* 요청 라인과 헤더를 읽음*/
  /* Rio_readlineb 함수를 사용하여 소켓으로부터 데이터를 읽어오고, 
  buf라는 문자열 버퍼에 저장*/
  Rio_readinitb(&rio, fd);          // rio 버퍼와 fd, 여기서는 서버의 connfd를 연결시켜준다.
  Rio_readlineb(&rio, buf, MAXLINE);
  
  printf("Request headers : \n");
  printf("%s" , buf);

  /*
   * sscanf 함수를 사용하여 buf에서 공백을 기준으로 분리된 문자열들을 추출하여 method, uri, version에 저장
   */
  sscanf(buf , "%s %s %s", method, uri,version);

  /* 
   * method가 GET인지 HEAD인지 파악
   * strcasecmp(method, "GET") => method가 GET이면 return 0 => if문에 해당 x
   * strcasecmp(method, "HEAD") => method가 HEAD이면 return 0 => if문에 해당 x
   * method가 GET이나 HEAD 아니라면 종료. main으로 가서 연결 닫고 다음 요청 기다림.
  */
  if (strcasecmp(method, "GET") != 0 && strcasecmp(method, "HEAD") != 0){
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return ;
  }
/*   if (strcasecmp(method, "GET")){
    // 하나라도 0이면 0
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method");
    return ;
  } */
  /* read_requesthdrs: Request line을 뺀 나머지 요청 헤더들을 무시(그냥 프린트)*/
  read_requesthdrs(&rio);

  /* 
   * GET 요청으로부터 URI를 파싱
   * parse_uri: 클라이언트 요청 라인에서 받아온 uri를 이용해 정적/동적 컨텐츠를 구분한다.
   * is_static이 1이면 정적 컨텐츠, 0이면 동적 컨텐츠 
   */
  is_static = parse_uri(uri,filename,cgiargs);
  printf("uri : %s, filename : %s, cgiargs : %s \n", uri, filename, cgiargs);
  /* 여기서 filename: 클라이언트가 요청한 서버의 컨텐츠 디렉토리 및 파일 이름 */
  if (stat(filename, &sbuf) < 0){
    // 파일이 없다면 404
    clienterror(fd, filename, "404", "Not found", "Tiny couldn`t find this file");
    return;
  }

  /*Serve static content*/
  if (is_static){
    // !(일반 파일이다) or !(읽기 권한이 있다)
    // 파일이 보통 파일인지 읽기 권한을 가지고 있는지 검증
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)){ // 버퍼가 사용가능 가능한 얘냐를 체크하는 것
      clienterror(fd, filename,"403", "Forbidden", "Tiny couldn`t read the file");
      return;
    }
    /* 정적 서버에 파일의 사이즈를 같이 보낸다. => Response header에 Content-length 위해 */
    serve_static(fd, filename, sbuf.st_size,method);
  }

  /*Serve dynamic content*/
  else{
    // !(일반 파일이다) or !(실행 권한이 있다)
    if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)){
      clienterror(fd, filename,"403", "Forbidden", "Tiny couldn`t run the CGI program");
      return;
    }
    // 동적 서버에 인자를 같이 보낸다.
    serve_dynamic(fd, filename, cgiargs,method);
  }

}

/* clientrerror 에러 메시지를 클라이언트에게 보낸다 */
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor = ""ffffff>\r\n",body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));

  // 에러 메시지와 응답 본체를 서버 소켓을 통해 클라이언트에 보낸다.
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}


/* read_requesthdrs: 클라이언트가 버퍼 rp에 보낸 나머지 요청 헤더들을 무시한다.(그냥 프린트)*/
void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];
  int i = 0;
  Rio_readlineb(rp, buf, MAXLINE);

  /* 
   * 버퍼 rp의 마지막 끝을 만날 때까지 
   * ("Content-length: %d\r\n\r\n에서 마지막 \r\n")
   * 계속 출력해줘서 없앤다. 
   */
  while(strcmp(buf, "\r\n")){
    Rio_readlineb(rp,buf,MAXLINE);
    printf("%d : %s",i,buf);
    i++;
  }

  return;
}

/* 
 * parse_uri HTTP URI를 분석한다.
 * parse_uri: uri를 받아 요청받은 파일의 이름(filename)과 요청 인자(cgiarg)를 채워준다.
 * cgiargs: 동적 컨텐츠의 실행 파일에 들어갈 인자 
 */
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  /* 정적 컨텐츠 */
  if(!strstr(uri, "cgi-bin")){
    // uri에 cgi-bin이 없으면 정적 컨텐츠
    strcpy(cgiargs, "");
    strcpy(filename, ".");
    strcat(filename, uri);

    if (uri[strlen(uri)-1] == '/'){
      strcat(filename, "home.html");  
    }
    return 1;                         // 정적 컨텐츠일 때는 1 리턴
  }
  /* 동적 컨텐츠 (adder)*/
  else{
    /* 
     * index: 문자를 찾았으면 문자가 있는 위치 반환 
     * ptr: '?'의 위치 
     */
    ptr = index(uri,'?');

    /* '?'가 있으면 cgiargs를 '?' 뒤 인자들과 값으로 채워주고 ?를 NULL로 만든다. */
    if (ptr) {
      strcpy(cgiargs, ptr + 1);
      *ptr = '\0';
    }
    /* '?'가 없으면 그냥 아무것도 안 넣어줌. */
    else{
      strcpy(cgiargs, "");
    }
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

/* 클라이언트가 원하는 정적 컨텐츠 디렉토리를 받아온다.
응답 라인과 헤더를 작성하고 서버에게 보낸다
그후 정적 컨텐츠 파일을 읽어 그 응답 본체를 클라이언트에 보낸다 */
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];
  rio_t rio;
  printf("응답 헤드 전송 전\n");
  /* Send response headers to client*/
  get_filetype(filename, filetype);                     // 파일 이름의 접미어 부분 검사 => 파일 타입 결정
  sprintf(buf, "HTTP/1.0 200 OK \r\n");
  sprintf(buf, "%sServer : Tiny Web Server\r\n", buf);  // 서버 이름
  sprintf(buf, "%sConnection : close\r\n", buf);        // 연결 방식
  sprintf(buf, "%sContent-length : %d\r\n", buf , filesize); // 컨텐츠 길이
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf , filetype);// 컨텐츠 타입

  /* 응답 라인과 헤더를 클라이언트에게 보냄 */
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  if (!strcasecmp(method, "HEAD")){
    return;
  }
  /* 11.6.B */
  /* FILE *file = fopen("Tinyserver_output.txt","w");
  if (file == NULL){
    printf("fileopen faild");
  }
  fprintf(file, buf);
  fclose(file); */
  printf("응답 바디 전송 전 filename : %s filesize : %d\n", filename , filesize);
  /* 응답 바디 전송 | 11.9 */
  srcfd = Open(filename, O_RDONLY, 0);                   // 파일 열기
  printf("1\n");
  // 파일을 가상 메모리에 맵핑
  // srcp = Mmap(0, filesize, PROT_READ , MAP_PRIVATE, srcfd, 0);
  srcp = (char *)Malloc(filesize);                       // 파일을 위한 메모리 할당
  printf("2\n");
  Rio_readinitb(&rio, srcfd);                            // 파일을 버퍼로 읽기위한 초기화
  printf("3\n");
  Rio_readn(srcfd, srcp, filesize);                      // 읽기
  printf("4\n");
  Close(srcfd);                                          // 파일 디스크립터 닫기 
  printf("5\n");
  /* 동영상을 재상 안하면 이 부분에서 막히면서 서버가 꺼짐 */
  Rio_writen(fd, srcp, filesize);                        // 파일 내용을 클라이언트에게 전송 (응답 바디 전송)
  // 맵핑된 가상메모리 해제
  // Munmap(srcp, filesize);
  printf("6\n");
  free(srcp);                                            // 할당된 메모리 공간을 해제한다.  
  printf("응답 바디 전송 후 정적 컨텐츠 파일 종료\n");

}



/*
 * get_filetype - Derive file type from filename
 */
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html")){
    strcpy(filetype, "text/html");
  }else if (strstr(filename, ".gif")){
    strcpy(filetype, "image/gif");
  }else if (strstr(filename, ".png")){
    strcpy(filetype, "image/png");
  }else if (strstr(filename, ".jpg")){
    strcpy(filetype, "image/jpeg");
  }else if (strstr(filename, ".mpg")){ // mpg 파일은 재생되지 않는다.
    strcpy(filetype, "video/mpeg");
  }else if (strstr(filename, ".mp4")){ // 11.7
    strcpy(filetype, "video/mp4");
  }else if (strstr(filename, ".ico")){
    strcpy(filetype, "image/x-icon");
  }else{
    strcpy(filetype, "text/plain");
  }
}

/* serve_dynamic 은 동적콘텐츠를 클라이언트에 제공 */
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  /*
  * fork(): 함수를 호출한 프로세스를 복사하는 기능
  * 부모 프로세스(원래 진행되던 프로세스), 자식 프로세스(복사된 프로세스)
  * Tiny는 자식 프로세스를 fork하고, CGI 프로그램을 자식에서 실행하여 동적 컨텐츠를 표준 출력으로 보냄
  */
  char buf[MAXLINE], *emptylist[] = {NULL};

  /* Return first part of HTTP response*/
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server : Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));
  printf("Fork() 하기전 동적 콘텐츠 안\n");
  /* Child */
  if (Fork() == 0){
    /* QUERY_STRING 환경 변수를 URI에서 추출한 CGI 인수로 설정 */
    setenv("QUERY_STRING", cgiargs , 1);
    /* method를 cgi-bin/adder.c에 넘겨주기 위해 환경변수 set */
    setenv("REQUEST_METHOD", method , 1);
    /* Redirect stdout to client */
    Dup2(fd, STDOUT_FILENO);
    /* Run CGI program */
    Execve(filename, emptylist,environ);
    /* 
    CGI 프로그램이 자식 컨텍스트에서 실행되기 때문에 execve함수를 호출하기 전에 존재하던 열린 파일들과
    환경 변수들에도 동일하게 접근할 수 있다. 그래서 CGI 프로그램이 표준 출력에 쓰는 모든 것은 
    직접 클라이언트 프로세스로 부모 프로세스의 어떤 간섭도 없이 전달된다.
    */
  }
  /* Parent waits for and reaps child */
  
  Wait(NULL);
  printf("wait 함수 후 동적 컨텐츠 파일 종료\n");
}

/* 
serve_dynamic 이해하기 
1. fork()를 실행하면 부모 프로세스와 자식 프로세스가 동시에 실행된다.
2. 만약 fork()의 반환값이 0이라면, 즉 자식 프로세스가 생성됐으면 if문을 수행한다. 
3. fork()의 반환값이 0이 아니라면, 즉 부모 프로세스라면 if문을 건너뛰고 Wait(NULL) 함수로 간다. 이 함수는 부모 프로세스가 먼저 도달해도 자식 프로세스가 종료될 때까지 기다리는 함수이다.
4. if문 안에서 setenv 시스템 콜을 수행해 "QUERY_STRING"의 값을 cgiargs로 바꿔준다. 우선순위가 1이므로 기존의 값과 상관없이 값이 변경된다.
5. Dup2 함수를 실행해서 CGI 프로세스의 표준 출력을 fd(서버 연결 소켓 식별자)로 복사한다. 이제 STDOUT_FILENO의 값은 fd이다. 다시 말해, CGI 프로세스에서 표준 출력을 하면 그게 서버 연결 식별자를 거쳐 클라이언트에 출력된다.
6. execuv 함수를 이용해 파일 이름이 filename인 파일을 실행한다.
*/

void echo(int connfd)
{
  size_t n;
  char buf[MAXLINE];
  rio_t rio;

  Rio_readinitb(&rio, connfd); // RIO 구조체 초기화

  while((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
      if(strcmp(buf,"\r\n")== 0){
        break;
      }
      Rio_writen(connfd, buf , n);                  // 클라이언트에게 데이터 전송
  }
}
/*
 * adder.c - a minimal CGI program that adds two numbers together
 */
/* $begin adder */
#include "csapp.h"

int main(void) {
  char *buf, *p;
  char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
  int n1 = 0 , n2 = 0;

  /* 두 개의 인자를 추출해주세요. */
  if ((buf = getenv("QUERY_STRING")) != NULL){// QUERY_STRING이라는 환경변수
    p = strchr(buf , '&');
    *p = '\0';
    strcpy(arg1, buf);
    strcpy(arg2, p+1);
    if (strchr(arg1, '=')) {
      //html형식에서도 숫자를 받아 처리 할 수 있게끔 변경
      p = strchr(arg1, '=');
      *p = '\0';
      strcpy(arg1, p + 1);

      p = strchr(arg2, '=');
      *p = '\0';
      strcpy(arg2, p + 1);
    }
    n1 = atoi(arg1); // atoi: ASCII to integer, string을 int로 바꿔줌, int를 return
    n2 = atoi(arg2);
  }

  if (strcasecmp(getenv("REQUEST_METHOD"), "HEAD") == 0){
    exit(0);
  }
  /* 응답 몸체를 만듬 */
  sprintf(content, "QUERY_STRING=%s", buf);
  sprintf(content, "Welcom to add.com : ");
  sprintf(content, "%s THE Internet addition portal.\r\n<p>", content);
  sprintf(content, "%s The answer is : %d + %d = %d \r\n<p>", content, n1 ,n2, n1+n2);
  sprintf(content, "%s Thanks for visiting!\r\n", content);

  /* HTTP 응답을 생성 */
  printf("Connection : close\r\n");
  printf("Content-length : %d\r\n", (int)strlen(content));
  printf("Content-type: text/html\r\n\r\n");

  /* if (strcasecmp(getenv("REQUEST_METHOD"), "GET") == 0){ // GET인 경우만
    printf("%s", content);                           // HTML 응답의 바디인 content 출력
  }else{
    printf("METHOD = HEAD\r\n");
  } */

  printf("%s", content);
  fflush(stdout); // 출력 버퍼 비움
  exit(0);
}
/* $end adder */

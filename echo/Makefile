# 빌드 규칙에서 $(CC)를 사용하면 gcc가 호출
CC = gcc

# 컴파일 옵션 = 최적화 레벨을 높이고(-O2), 
# 모든 경고 메시지를 표시하며(-Wall), 
# 현재 디렉토리를 헤더 파일 검색 경로에 추가(-I .)
# 하여 소스 코드를 컴파일하도록 지시합니다.
CFLAGS = -O2 -Wall -I .

# -lpthread는 POSIX 스레드 라이브러리를 나타냅니다. 
# 이는 프로그램에서 POSIX 스레드를 사용할 때 
# 필요한 라이브러리 파일을 링크하는 역할을 합니다.
LIB = -lpthread

# <Target> : <Dependency>
#   <Commend>


echo : echoserveri.c echo.o csapp.o
	$(CC) $(CFLAGS) -o echoserveri echoserveri.c echo.o csapp.o $(LIB) 

echo.o : echo.c
	$(CC) $(CFLAGS) -c echo.c

echoclient.o : echoclient.c csapp.o
	$(CC) $(CFLAGS) -o echoclient echoclient.c csapp.o $(LIB) 

csapp.o : csapp.c
	$(CC) $(CFLAGS) -c csapp.c

clean : 
	rm -f *.o echo *~
	rm -f echoserveri
	rm -f echoclient

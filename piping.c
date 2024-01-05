#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

//p[0] is read end
//p[1] is write end
int main(int argc, char** argv){
	int p[2][2], pid1, pid2, pid3, waitst, fd;
	char buff[256];
	pipe(p[0]);
	pipe(p[1]);

	pid1 = fork();
	if (pid1 == 0){
		close(1);
		dup(p[0][1]);
		close(p[0][0]);
		close(p[1][0]);
		close(p[1][1]);
		execlp("ls", "ls", NULL);
	}
	else{
		close(p[0][1]);
		//close(p[1][1]);
		wait(&waitst);
	}
	//read(p[0][0], buff, 256);
	//puts(buff);
	pid2 = fork();
	if (pid2 == 0){
		close(1);
		dup(p[1][1]);
		close(0);
		dup(p[0][0]);
		close(p[0][1]);
		close(p[1][0]);
		execlp("wc", "wc", NULL);
	}
	else{
		wait(&waitst);
		close(p[1][1]);
	}
//	read(p[1][0], buff, 256);
//	puts(buff);
	pid3 = fork();
	if (pid3 == 0){
		close(0);
		dup(p[1][0]);
		execlp("wc", "wc", NULL);
	}
	else{
		wait(&waitst);
	}
}

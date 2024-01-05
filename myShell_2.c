#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

enum{
	BUFFSIZE = 1024,
	TOKSSIZE = 32,
	};

void printPrompt();
void getInput(char*);
void tokenize1(char**, char*);//look for & and ;
int getStdinout(char**, char*); //look for stdin/out redirection if any
void trim(char**); //remove leading and/or trailing space
int tokenize2(char**, char*);//look for space
int tokenize3(char**, char*);//split |
void redirect(char**, int*, int*, int);
void execution(char**, char**, int*, int*, int*, int, int);
void redirectPipe(char**, int*, int*, int, int*, int, int);
void executionPipe(char**, char**, int*, int*, int*, int, int, int*, int, int);
void printToks(char**);

//procedure: tokenize ;& --> tokenize | --> tokenize <> --> tokenize sp

int main(){
	char buff[BUFFSIZE];
	char *toks[TOKSSIZE], *toks2[TOKSSIZE], *toks4[TOKSSIZE], *inout[2];//toks split & and ;      toks4 split space  
	int pids[TOKSSIZE], p[2];
	int pid, n, waitst, intargc, pipetoken, i;
	int fdinput, fdoutput, append;
	//n is the number of ;& tokens
	//intargc is the number of space token

	while (1){
		n = 0;
		*(inout) = NULL; //*(inout) is stdin redirect
		*(inout + 1) = NULL; //*(inout + 1) stdout redirect
		printPrompt();
		getInput(buff);
		tokenize1(toks, buff);
		pipe(p);

		while (*(toks + n) != NULL){
			if (strcmp(*(toks + n), "\0") == 0){
				n += 1;
				continue;
			}
//split |
			pipetoken = tokenize3(toks2, *(toks + n));
			//printToks(toks2);
			if (pipetoken > 1){
				for (i = 0 ; i < pipetoken ; i++){
					append = getStdinout(inout, *(toks2 + i));
					intargc = tokenize2(toks4, *(toks2 + i));
					//printf("%d\n", i);
					if (i == 0){
						executionPipe(toks4, inout, &fdinput, &fdoutput, &waitst, intargc, append, p, 1, 0);
					}
					else if (i + 1 == pipetoken){
						close(*(p + 1));
						executionPipe(toks4, inout, &fdinput, &fdoutput, &waitst, intargc, append, p, 0, 1);
					}
					else{
						close(*(p + 1));
						executionPipe(toks4, inout, &fdinput, &fdoutput, &waitst, intargc, append, p, 0, 0);
					}
				}
			}
			else{
				append = getStdinout(inout, *toks2);
				intargc = tokenize2(toks4, *toks2);
				execution(toks4, inout, &fdinput, &fdoutput, &waitst, intargc, append);
			}
			n += 1;
		}
	}
}

void printPrompt(){
	char buf[BUFFSIZE];
	getcwd(buf, BUFFSIZE);
	write(1, buf, strlen(buf) + 1);
	write(1, "> ", 3);
}

void getInput(char *buff){
	int n;
	n = read(0, buff, BUFFSIZE);
	*(buff + n - 1) = '\0';
}

void tokenize1(char** toks, char* buff){
	int n, token, tok;
	n = 0;
	token = 0;
	tok = 0;
	
	while(*(buff + n) != '\0' && n < BUFFSIZE){
		if (*(buff + n) == ';'){
			*(buff + n) = '\0';
			*(toks + tok) = (buff + token);
			token = n + 1;
			tok += 1;
		}
		else if(*(buff + n) == '&'){
			*(buff + n) = '\0';
			*(toks + tok) = (buff + token);
			token = n + 1;
			tok += 1;
		}
		n += 1;
	}
	*(toks + tok) = (buff + token);
	tok += 1;
	*(toks + tok) = NULL;
}

int tokenize2(char** toks, char* buff){
	char *tmp;
	int n;
	n = 0;
	tmp = strtok(buff, " ");
	*(toks + n) = tmp;
	n += 1;
	while (tmp != NULL){
		tmp = strtok(NULL, " ");
		*(toks + n) = tmp;
		n += 1;
	}
	*(toks + n) = NULL;
	return n - 1;
}

int tokenize3(char **toks, char* buff){
	char *tmp;
	int n;
	n = 0;
	tmp = strtok(buff, "|");
	*(toks + n) = tmp;
	n += 1;
	while (tmp != NULL){
		tmp = strtok(NULL, "|");
		*(toks + n) = tmp;
		n += 1;
	}
	*(toks + n) = NULL;
	return n - 1;
}

int getStdinout(char** inout, char* toks){
	int i, toReturn;
	toReturn, i = 0;
	while (*(toks + i) != '\0'){
		if (*(toks + i) == '<'){
			*(toks + i) = '\0';
			*(inout) = (toks + i + 1);
		}
		if (*(toks + i) == '>'){
			*(toks + i) = '\0';
			*(inout + 1) = (toks + i + 1);
			if (*(toks + i + 1) == '>'){
				*(toks + i + 1) = '\0';
				*(inout + 1) += 1;
				i += 1;
				toReturn = 1;
			}
		}
		i += 1;
	}
	if (*(inout) != NULL){
		trim(inout);
	}
	if (*(inout + 1) != NULL){
		trim(inout + 1);
	}
	return toReturn;
}

void trim(char** word){
	if (*(*(word)) == ' '){
		*word = *(word) + 1;
	}
	if (*(*(word) + strlen(*(word) - 1)) == ' '){
		*(*(word) + strlen(*word) - 1) = '\0';
	}
}

void redirect(char** inout, int *fdinput, int *fdoutput, int append){
	if (*(inout) != NULL){
		*fdinput = open(*(inout), O_RDONLY);
		if (fdinput > 0){
			close(0);
			dup(*fdinput);
		}
	}

	if (*(inout + 1) != NULL){
		if (append){
			*fdoutput = open(*(inout + 1), O_CREAT|O_WRONLY|O_APPEND);
		}
		else{
			*fdoutput = open(*(inout + 1), O_CREAT|O_WRONLY|O_TRUNC);
		}
		close(1);
		dup(*fdoutput);
	}
}

void printToks(char **toks){
	int n;
	n = 0;
	while (*(toks + n) != NULL){
		printf("%s\n", *(toks + n));
		n += 1;
	}
}

void execution(char** toks4, char** inout, int* fdinput, int* fdoutput, int* waitst, int intargc, int append){
	int pid;

	if (strcmp(*toks4, "cd") == 0){
		if (intargc != 2){
			fprintf(stderr, "Usage: cd path\n");
		}
		else{
			chdir(*(toks4 + 1));
		}
	}

	else{
		if ((pid = fork()) == 0){
			redirect(inout, fdinput, fdoutput, append);
			execvp(*toks4, toks4);
			perror("");

			if (*fdinput > 0){
				close(*fdinput);
				dup(0);
			}
			if (*fdoutput > 0){
				close(*fdoutput);
				dup(1);
			}

			exit(2);
		}

		else{
			wait(waitst);
		}
	}
}

void executionPipe(char** toks4, char** inout, int* fdinput, int* fdoutput, int* waitst, int intargc, int append, int*
pipe, int start, int end){
	int pid;

	if (strcmp(*toks4, "cd") == 0){
		if (intargc != 2){
			fprintf(stderr, "Usage: cd path\n");
		}
		else{
			chdir(*(toks4 + 1));
		}
	}

	else{
		if ((pid = fork()) == 0){
			redirectPipe(inout, fdinput, fdoutput, append, pipe, start, end);
			execvp(*toks4, toks4);
			perror("");

			if (*fdinput > 0){
				close(*fdinput);
				dup(0);
			}
			if (*fdoutput > 0){
				close(*fdoutput);
				dup(1);
			}
			exit(2);
		}

		else{
			wait(waitst);
		}
	}
}

void redirectPipe(char **inout, int* fdinput, int* fdoutput, int append, int* pipe, int start, int end){
	if (*(inout) != NULL && start){
		*fdinput = open(*(inout), O_RDONLY);
		if (fdinput > 0){
			close(0);
			dup(*fdinput);
		}
	}

	if (*(inout + 1) != NULL && end){
		if (append){
			*fdoutput = open(*(inout + 1), O_CREAT|O_WRONLY|O_APPEND);
		}
		else{
			*fdoutput = open(*(inout + 1), O_CREAT|O_WRONLY|O_TRUNC);
		}
		close(1);
		dup(*fdoutput);
	}

	if (start){
		close(1);
		dup(*(pipe + 1));
	}
	else if (end){
		close(0);
		dup(*pipe);
	}
	else{
		close(0);
		dup(*pipe);
		close(1);
		dup(*(pipe + 1));
	}
}



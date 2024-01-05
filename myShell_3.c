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
void tokenize1(char**, char*); //split & and ;
void tokenize2(char**, char*); //split space
int tokenize3(char**, char*); //split |
int getStdinout(char**, char*); //look for stdin/out redirection if any
void trim(char**); //remove leading and/or trailing space
void redirect(char**, int*, int*, int);
void execution(char**, char**, int);
void redirectPipe(char**, int*, int*, int, int**, int**, int, int);
void executionPipe(char**, char**, int, int**, int**, int, int);
void printToks(char**);
int** makePipe(int);
void freePipe(int**, int);

//procedure: tokenize ;& --> tokenize | --> tokenize <> --> tokenize sp

int main(){
	char buff[BUFFSIZE];
	char *toks[TOKSSIZE], *toks2[TOKSSIZE], *toks4[TOKSSIZE], *inout[2]; 
	int pids[TOKSSIZE];
	int pid, n, waitst, pipetoken, i;
	int fdinput, fdoutput, append;
	int **p;
	//n is the number of ;& tokens
	//toks split (& and ;), toks2 split |, toks4 split space
	//*(inout) is stdin redirect if any, *(inout + 1) is stdout redirect if any

	while (1){
		n = 0;
		*(inout) = NULL; 
		*(inout + 1) = NULL;
		printPrompt();
		getInput(buff);
		tokenize1(toks, buff);

		while (*(toks + n) != NULL){
			if (strcmp(*(toks + n), "\0") == 0){
				n += 1;
				continue;
			}
			pipetoken = tokenize3(toks2, *(toks + n));
			if (pipetoken > 1){
				p = makePipe(pipetoken - 1);
				for (i = 0 ; i < pipetoken - 1 ; i++){
					pipe(*(p + i));
				}
				for (i = 0 ; i < pipetoken ; i++){
					append = getStdinout(inout, *(toks2 + i));
					tokenize2(toks4, *(toks2 + i));
					if (i == 0){
						executionPipe(toks4, inout, append, NULL, (p + i), 1, 0);
					}
					else if ((i + 1) == pipetoken){
						executionPipe(toks4, inout, append, (p + i - 1), NULL, 0, 1);
					}
					else{
						close(*(*(p + i -1) + 1));
						executionPipe(toks4, inout, append, (p + i - 1), (p + i), 0, 0);
					}
				}
				freePipe(p, pipetoken - 1);
			}
			else{
				append = getStdinout(inout, *toks2);
				tokenize2(toks4, *toks2);
				execution(toks4, inout, append);
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

void tokenize2(char** toks, char* buff){
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

void execution(char** toks4, char** inout, int append){
	int pid, waitst, fdinput, fdoutput;

	if (strcmp(*toks4, "cd") == 0){
		chdir(*(toks4 + 1));
	}

	else{
		if ((pid = fork()) == 0){
			redirect(inout, &fdinput, &fdoutput, append);
			execvp(*toks4, toks4);
			perror("");

			if (fdinput > 0){
				close(fdinput);
				dup(0);
			}
			if (fdoutput > 0){
				close(fdoutput);
				dup(1);
			}

			exit(2);
		}

		else{
			wait(&waitst);
		}
	}
}

void executionPipe(char** toks4, char** inout, int append, int** pipeRead, int** pipeWrite, int start, int end){
	int pid, waitst, fdinput, fdoutput;

	if (strcmp(*toks4, "cd") == 0){
		chdir(*(toks4 + 1));
	}

	else{
		if ((pid = fork()) == 0){
			redirectPipe(inout, &fdinput, &fdoutput, append, pipeRead, pipeWrite, start, end);
			execvp(*toks4, toks4);
			perror("");

			if (fdinput > 0){
				close(fdinput);
				dup(0);
			}
			if (fdoutput > 0){
				close(fdoutput);
				dup(1);
			}
			exit(2);
		}
		else{
			wait(&waitst);
			if (pipeWrite != NULL){
				close(*((*pipeWrite) + 1));
			}
		}
	}
}

void redirectPipe(char **inout, int* fdinput, int* fdoutput, int append, int** pipeRead, int** pipeWrite, int start, int end){
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
		dup(*((*pipeWrite) + 1));
	}
	else if (end){
		close(0);
		dup(*((*pipeRead) + 0));
	}
	else{
		close(1);
		dup(*((*pipeWrite) + 1));
		close(0);
		dup(*((*pipeRead) + 0));
	}
}

int** makePipe(int pipeNum){
	int **temp;
	int i;
	temp = (int**) malloc(pipeNum * sizeof(int));
	for (i = 0 ; i < pipeNum ; i++){
		*(temp + i) = (int*) malloc(2 * sizeof(int));
	}
	return temp;
}

void freePipe(int** myPipe, int pipeNum){
	int i;
	for (i = 0 ; i < pipeNum ; i++){
		free(*(myPipe + i));
	}
	free(myPipe);
}


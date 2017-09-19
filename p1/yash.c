/* file yash.c: This is a simple shell program
   Author: Karl Solomon
*/

/*
INCLUDES
*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include "str.h"

/*
MACROS
*/
#define MAXCHARS 2000
#define MAXTOKENS 10
#define PIPE_WRITE 1
#define PIPE_READ 0
#define FALSE 0
#define TRUE 1

/*
CONSTANTS
*/
typedef int bool;
enum {false, true};

typedef int direction;
enum { in, out, err};

/* A job is a pipeline of processes.  */
typedef struct job
{
  struct job *previous;	//
  char *command;              /* command*/
  pid_t pgid;                 /* process group ID */
  bool running;
  bool foreground;
  int jobNumber;
} job;

/*
FUNCTION PROTOTYPES
*/
void handleProcess(char** args);
void handleParent(int numChildren);
void handleRedirect(char* fileName, direction dir);
void handleJob();
void handleChild(char* child);
bool readFromFile(char* fileName, char* output, int maxLength);
void writeToFile(char* fileName, char* message);
void handlePipe(char* child1, char* child2);

/*
GLOBALS
*/
job *tail = NULL;
int pipefd[2];
char termid[L_ctermid];
int status, pid_ch1, pid_ch2, pid, parentPID;
int jobNumber = 0;

static void sig_handler(int signo) {
  switch(signo){
  case SIGINT:
        printf("caught SIGINT\n");
        exit(0);
        break;
  case SIGTSTP:
        printf("caught SIGTSTP\n");
        break;
  // case SIGCHLD:
  // 		printf("caught SIGCHLD\n");
  // 		break;
  }
}

int main (void) {
	parentPID =  setsid();	// creates a new session
	char previous[MAXCHARS];
	char poll[MAXCHARS];
	char input[MAXCHARS];
	char** process;
	int i = 0;
	ctermid(termid);
	//input[0] = '\0';
	printf("# ");
	while(fgets(input, MAXCHARS, stdin) != NULL) {
		strTrim(input);
		job j;
		jobNumber++;
		j.running = true;
		j.foreground = !strContains(input, "&");
		j.jobNumber = jobNumber;
		strcpy(j.command, input);

		if(tail == NULL) {
			tail = &j;
			j.previous = NULL;
		} else {
			j.previous = tail;
			tail = &j;
		}
		handleJob(); // handles job @ Tail		
		i++;
		printf("# ");
	}
	exit(0);
	
	//fgets(input, MAXCHARS, stdin)
	
}

void handleJob() {
	bool hasPipe = strContains((*tail).command, "|");
	char** commands = strSplit((*tail).command, '|');
	int i = 0;
	while(commands[i] != '\0') {
		strTrim(commands[i]);
		i++;
	}

	if(pipe(pipefd) == -1) {
		perror("pipe");
		exit(-1);
	}

	pid_ch1 = fork();
	if(pid_ch1 > 0) {
		sleep(1); // make child 1 first
		if(hasPipe) {
			pid_ch2 = fork();
			if(pid_ch2 > 0) {
				// Parent w/ 2 children
				setpgid(pid_ch1, 0);
				setpgid(pid_ch2, pid_ch1);
				handleParent(2);
			} else {
				//Child 2
				//sleep(1);
				close(pipefd[PIPE_WRITE]);	//close write;
				dup2(pipefd[PIPE_READ], STDIN_FILENO);
				handleChild(commands[1]);
			}
		} else {
			//Parent w/ 1 child
			setpgid(pid_ch1, 0);
			handleParent(1);
		}
	} else {
		//Child 1
		(*tail).pgid = pid_ch1;
		if(hasPipe) {
			close(pipefd[PIPE_READ]);
			dup2(pipefd[PIPE_WRITE], STDOUT_FILENO);			
		}
		handleChild(commands[0]);
	}
	(*tail).pgid = pid_ch1;
}

void handleParent(int numChildren) {
	int count = 0;
	close(pipefd[PIPE_WRITE]);
	close(pipefd[PIPE_READ]);
	FILE* fp = fopen(termid, "r");
	bool test;
	// if((*tail).foreground == true) {
	// 	test = tcsetpgrp(fileno(fp), pid_ch1);
	// }

	if(signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("signal(SIGINT) error");
	} else if (signal(SIGTSTP, sig_handler) == SIG_ERR) {
		printf("signal(SIGTSTP) error");
	} else if (signal(SIGTTOU, sig_handler) == SIG_ERR) {
		printf("signal(SIGTTOU) error");
	}
	// else if (signal(SIGCHLD, sig_handler) == SIG_ERR) {
	//  	printf("signal(SIGCHLD) error");
	// }

	while(count < numChildren) {
		pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);
		//test = tcsetpgrp(fileno(fp), parentPID);
		if (pid == -1) {
		    perror("waitpid");
		    exit(EXIT_FAILURE);
		}


		if (WIFEXITED(status)) {
		    printf("child %d exited, status=%d\n", pid, WEXITSTATUS(status));
		    count++;
		} else if (WIFSIGNALED(status)) {
		    printf("child %d killed by signal %d\n", pid, WTERMSIG(status));
		    count++;
		} else if (WIFSTOPPED(status)) {
		    printf("%d stopped by signal %d\n", pid, WSTOPSIG(status));
		} else if (WIFCONTINUED(status)) {
		    printf("Continuing %d\n",pid);
		}
	}
	//exit(1);
}

/*
Brief:
	uses dup2 to change the input/output from a process from/to the specified file.
Params:
	fileName 	> name of the file (path incl. if not in the same directory) to write/read
	dir 		> direction specifies if this is using STDIN, STDOUT, STDERR (see )
	i.e. 
		child1 = "ls > B.txt"
		child2 = "grep .txt < A.txt > C.txt"
Return:
	Void
*/
void handleRedirect(char* fileName, direction dir) { 
	FILE *file;
	if(dir == in) {
		file = fopen(fileName, "r");
	} else if (dir == out) {
		file = fopen(fileName, "w+");
	} else if (dir = err) {
		file = fopen(fileName, "w+");
	} else {
		exit(1); //invalid direction
	}
	dup2(fileno(file), dir);
	fclose(file);
}

/*
Brief:
	Redirects any file tokens from left to right (so right-most will overwrite others if there are multiple of the same redirection for a single process)
	then calls handleProcess to execute the process to the left of the first file redirect
Params:
	child 	> child's string, includes all file redirections & arguments
		i.e. grep .txt < A.txt > C.txt
Return:
	Void
*/
void handleChild(char* child) {
	char** tokens = strSplit(child, ' ');
	int i = 0;
	while(tokens[i] != NULL) {
		strTrim(tokens[i]);
		i++;
	}
	i = 0;
	while(tokens[i] != NULL) {
		if(strContains(tokens[i], "<")) {	
			handleRedirect(tokens[i+1], in);
		} else if (strContains(tokens[i], ">")) {
			handleRedirect(tokens[i+1], out);
		} else if (strContains(tokens[i], "2>")) {
			handleRedirect(tokens[i+1], err);
		}
		i++;
	}
	i = 0;
	while(tokens[i] != NULL) {
		if(strContains(tokens[i], "<") || strContains(tokens[i], ">") || strContains(tokens[i], "2>")) {
			tokens[i] = '\0';
			break;
		}
		i++;
	}
	handleProcess(tokens);
}
/*
Brief:
	Runs a process given its parameters
	i.e. grep firefox
Params:
	args	> arguments that that a process (index 0) requires to run
Return:
	Void
*/
void handleProcess(char** args) {
	char* myargs[30];
	int i = 0;
	while(1) {
		if(args[i] == NULL) {			
			myargs[i] = NULL;
			break;
		} else {			
			myargs[i] = strdup(args[i]);
		}
		i++;
	}
	execvp(myargs[0], myargs);
	exit(1);
}
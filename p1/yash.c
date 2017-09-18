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
  struct job *next;
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
void handleChild(char* child);
bool readFromFile(char* fileName, char* output, int maxLength);
void writeToFile(char* fileName, char* message);
void handlePipe(char* child1, char* child2);

/*
GLOBALS
*/
job *head = NULL;
int pipefd[2];
int status, pid_ch1, pid_ch2, pid;
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
	char input[MAXCHARS];
	char output[MAXCHARS];
	char *process;
	char **token;
	printf("# ");
	fgets(input, MAXCHARS, stdin);
	strTrim(input);
	job j;
	jobNumber++;
	if(*head == NULL) {
		head = &j;				// make new job head b/c no other jobs exist
		j.previous = NULL;
	} else {
		job current = *head;
		while(current.next != NULL) {
			&current = current.next;
		}
		current.next = &j;	//add new job to list
		j.previous = &current;
	}
	j.next = NULL;
	j.running = true;
	j.foreground = true;
	j.jobNumber = jobNumber;
	strcpy(j.command, input);

	handleJob(&j);
	// if(strContains(input, "|")) {
	// 	token = strSplit(input, '|');
	// 	handlePipe(&j, token[0], token[1]);
	// } else {
	// 	pid_ch1 = fork();
	// 	if(pid_ch1 == 0) {
	// 		handleChild(&j input);
	// 	} else {
	// 		handleParent(1);
	// 	}
	// }
}

void handleParent(int numChildren) {
	int count = 0;
	close(pipefd[PIPE_WRITE]);
	close(pipefd[PIPE_READ]);

	if(signal(SIGINT, sig_handler) == SIG_ERR) {
		printf("signal(SIGINT) error");
	} else if (signal(SIGTSTP, sig_handler) == SIG_ERR) {
		printf("signal(SIGTSTP) error");
	}
	// else if (signal(SIGCHLD, sig_handler) == SIG_ERR) {
	//  	printf("signal(SIGCHLD) error");
	// }

	while(count < numChildren) {
		pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);
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
		    printf("Sending CONT to %d\n", pid);
		    sleep(5); //sleep for 4 seconds before sending CONT
		    kill(pid,SIGCONT);
		} else if (WIFCONTINUED(status)) {
		    printf("Continuing %d\n",pid);
		}
	}
	exit(1);
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

void handleJob(job* j) {
	bool hasPipe = strContains(j*.command, "|");
	char** commands = strSplit(j*.command, "|");
	char** tokens = strSplit(child, ' ');
	char** child1Args;
	char** child2Args;
	int i = 0;
	while(tokens[i] != NULL) {
		strTrim(tokens[i]);
		i++;
	}
	i = 0;
	pid_ch1 = fork();
	if(pid_ch1 > 0) {
		if(hasPipe) {
			pid_ch2 = fork();
			if(pid_ch2 > 0) {
				// Parent w/ 2 children
				handleParent(2);
			} else {
				//Child 2
				sleep(1);
				setpgid(0,pid_ch1);	//child 2 joins child 1's group
				close(pipefd[PIPE_WRITE]);	//close write;
				dup2(pipefd[PIPE_READ], STDIN_FILENO);
				strTrim(commands[1]);
				handleChild(commands[1]);
			}
		} else {
			//Parent w/ 1 child
			handleParent(1);
		}
	} else {
		//Child 1
		setsid();		// set pgid, establish child 1 as process leader
		j*.pgid = pid_ch1;
		if(hasPipe) {
			close(pipefd[PIPE_READ]);
			dup2(pipefd[PIPE_WRITE], STDOUT_FILENO);			
		}
		strTrim(commands[0]);
		handleChild(commands[0]);
	}
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
			handleProcess(tokens);
		}
		i++;
	}
}

/*
Brief:
	Executes two children processes after creating a pipe and linking the first processes' std out ls from a pipe
Params:
	child1 	> first child's string separate from pipe
	child2 	> second child's string sepatare from pipe
	i.e. 
		child1 = "ls > B.txt"
		child2 = "grep .txt < A.txt > C.txt"
Return:
	Void
*/
void handlePipe(job* j, char* child1, char* child2) {
	char **child1Args;
	char **child2Args;
	//int count = 0;

	if(pipe(pipefd) == -1) {
		perror("pipe");
		exit(-1);
	}

	pid_ch1 = fork();
	j*.pgid = pid_ch1;
	if(pid_ch1 > 0) {
		//PARENT
		printf("Child1 pid = %d\n", pid_ch1);
		pid_ch2 = fork();
		if(pid_ch2 > 0) {
			//PARENT
			printf("Child2 pid = %d\n", pid_ch2);
			handleParent(2);
		} else {
			//CHILD 2
			sleep(1);//let CHILD 1 run first
			setpgid(0,pid_ch1);	//child 2 joins child 1's group
			close(pipefd[PIPE_WRITE]);	//close write;
			dup2(pipefd[PIPE_READ], STDIN_FILENO);
			strTrim(child2);
			child2Args = strSplit(child2, ' ');
			//handleProcess(child2Args);
			handleChild(child2);
		}
	} else {
		//CHILD 1
		setsid();
		close(pipefd[PIPE_READ]);
		dup2(pipefd[PIPE_WRITE], STDOUT_FILENO);
		strTrim(child1);
		child1Args = strSplit(child1, ' ');
		//handleProcess(child1Args);
		handleChild(child1);
	}
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
void handleProcess(job* j, char** args) {
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
}

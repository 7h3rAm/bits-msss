#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
 
#define PFDREAD			0					/* pipe's read end file descriptor */
#define PFDWRITE		1					/* pipe's write end file descriptor */

#define FALSE			0					/* verbose boolean flags */
#define TRUE			!FALSE

#define MAXCMDBUFSIZE	512					/* buffer size for parsed user commands */
#define MAXINPUTBUFSIZE	1024				/* buffer size for raw user input */


pid_t pid;									/* stores pid from fork */
char *commandbuf[MAXCMDBUFSIZE];			/* buffer to contain parsed commands */
char infdbuf[MAXINPUTBUFSIZE];				/* buffer to read raw piped commands */
int commandcount = 0;						/* global commands counter */


void doWait(int);							/* wait for spwaned childs to complete execution */
char *trimWsp(char *);						/* trim leading whitespaces in a character buffer */
int doPipe(int, int, int);					/* tweak in/out fds and pipe/fork/dup2/execvp */
void tokenizeCommands(char *);				/* extract command and argument tokens from user input */
int doExecute(char *, int, int, int);		/* wrapper over tokenizeCommands and doPipe */


int main(int argc, char *argv[]) {
	char *prompt = "(commands) ", *banner = "ISZC462 - Network Programming | EC1 - P2\n"
											"Program to demo pipes based IPC mechanism\n"
											"Ankur Tyagi (2012HZ13084)\n";

	printf("\n%s\n", banner);
	while (TRUE) {
		printf("%s", prompt);
		fflush(NULL);						/* force printing of program banner + prompt */

		/* read user command into input buffer; exit if read fails */
		if (!fgets(infdbuf, MAXINPUTBUFSIZE, stdin)) { return EXIT_FAILURE; }
	
		int infd = 0;						/* input file descriptor */
		int startcommand = 1;				/* flag to indicate if this is the first command in pipeline */
 
		char *command = infdbuf;			/* pointer to commands pipeline */
		char *nextcommand = strchr(command, '|');	/* extract the  */
 
		while (nextcommand != NULL) {		/* extract all commands from the pipeline */
			*nextcommand = '\0';
			infd = doExecute(command, infd, startcommand, 0); /* execute each command while sharing its in out file descriptors */
			command = nextcommand + 1;		/* point to the next command in pipeline */
			nextcommand = strchr(command, '|');
			startcommand = 0;				/* mark completion of first command in pipeline */
		}

		infd = doExecute(command, infd, startcommand, 1); /* execute the last command in pipeline */
		doWait(commandcount);				/* wait for child processes to complete before exiting */
		commandcount = 0;					/* reset command counter */
	}

	return 0;
}

int doExecute(char* command, int infd, int startcommand, int last) {
	tokenizeCommands(command);				/* extract tokens from commands pipeline */

	if (commandbuf[0] != NULL) {			/* if we have recieved a valid command token */
		if (strcmp(commandbuf[0], "exit") == 0) { exit(EXIT_SUCCESS); }
		commandcount += 1;					/* update command count */
		return doPipe(infd, startcommand, last); /* try to execute it as a piped command */
	}

	return 0;
}

void tokenizeCommands(char *command) {
	command = trimWsp(command);				/* trim leading whitespaces from command buffer */
	char *nextcommand = strchr(command, ' '); /* extract offset of next token from command buffer */
	int temp = 0;
 
	while(nextcommand != NULL) {			/* while we have commands in pipeline */
		nextcommand[0] = '\0';
		commandbuf[temp] = command;			/* copy each command to commands buffer */
		++temp;

		command = trimWsp(nextcommand + 1);	/* trim leading whitespaces for subsequent commands */
		nextcommand = strchr(command, ' '); /* extract offset */
	}
 
	if (command[0] != '\0') {				/* check if command buffer is non-empty */
		commandbuf[temp] = command;			/* copy first command to command buffer */
		++temp;

		nextcommand = strchr(command, '\n'); /* extract offset of next '\n' delimited token */
		nextcommand[0] = '\0';				/* mark string termination */
	}
 
	commandbuf[temp] = NULL;
}

char *trimWsp(char *string) {
	while (isspace(*string)) { ++string; }	/* skip over any whitespace characters */
	return string;
}

int doPipe(int infd, int startcommand, int last) {
	int pipesfd[2];							/* pipe file descriptors array */
 
	pipe(pipesfd);							/* create a new unnamed pipe */
	pid = fork();							/* fork the current process */
 
	if (pid == 0) {							/* we're in child process */
		if (startcommand == 1 && infd == 0 && last == 0) { dup2(pipesfd[PFDWRITE], STDOUT_FILENO); } /* this is the first command, infd should be 0 */
		else if (startcommand == 0 && infd != 0 && last == 0) { /* this is the last command, infd should be nonzero */
			dup2(infd, STDIN_FILENO);
			dup2(pipesfd[PFDWRITE], STDOUT_FILENO);
		} else { dup2(infd, STDIN_FILENO); } /* this is some command from within the pipeline */

		if (execvp(commandbuf[0], commandbuf) == -1) { exit(EXIT_FAILURE); } /* pipe is ready, let's execvp the requested command */
	}

	if (infd != 0) { close(infd); }			/* except for first command, make sure you close the infd */
											/* this is required so that the next command in pipeline can open it for reading */
 
	close(pipesfd[PFDWRITE]);				/* ditto for the outfd */
 
	if (last == 1) { close(pipesfd[PFDREAD]); } /* for the last command close the infd of pipe (outfd is stdout and as such remains open) */

	return pipesfd[PFDREAD];
}

void doWait(int commandcount) {
	int temp;
	for (temp=0; temp<commandcount; ++temp) { wait(NULL); } /* cheap trick to keep parent waitig untill all child processes complete execution */
}

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAXHOSTSIZE		512																	/* buffer size to hold host name */
#define MAXCMDSIZE		512																	/* buffer size to hold command and its arguments */
#define MAXCWDSIZE		512																	/* buffer size to hold current workign directory */

#define TRUE			1
#define FALSE			!TRUE
#define DEBUG			!TRUE																/* flag to toggle debugging output */

extern char **environ;																		/* extern pointer to environment variables */

typedef struct {																			/* struct to hold command info */
	pid_t pid;																				/* pid of the command */
	char *cliargs[MAXCMDSIZE];																/* arguments list ([0] is the command itself) */
	unsigned int background;																/* flag to indicate if the command is to be executed in background */
	unsigned int argscount;																	/* count of arguments (analogous to the argc argument to main) */
} commandInfo;

typedef struct job {																		/* struct to hold background job info */
	pid_t pid;																				/* pid of the job */
	unsigned int id;																		/* id assigned to a running job (incremental; starts from 1) */
	char command[MAXCMDSIZE];																/* arguments list for this job */
	struct job *next;																		/* pointer to the next job in the list */
} jobInfo;

static jobInfo *jobslist = NULL;															/* global linked list of all running background jobs */
static unsigned int jobscount = 0;															/* global static counter for currently active number of jobs */

void do_exit(int);																			/* builtin "exit" command handler */
int do_info(void);																			/* builtin "info" command handler */
int do_jobs(void);																			/* builtin "jobs" command handler */
int do_cd(commandInfo *);																	/* builtin "cd" command handler */
int readCommand(char *);																	/* read a command from stdin and null terminate it */
int parseCommand(char *, commandInfo *);													/* parse command and populate command info struct */
int showCommand(commandInfo *);																/* show parsed command and its args list (only if DEBUG is on) */
int checkCommand(commandInfo *cinfo);														/* check if the parsed command is available or not */
pid_t do_execvp(commandInfo *);																/* fork and execvp parsed command (populate jobs struct if reqd.) */
void handleInterrupt(int);																	/* interrupt handler for registered signals */

int main(int cliargsc, char **cliargsv) {
	int rc = -1;																			/* signed int to hold recturn code from syscalls/functions */
	char *banner = "Yet Another Unix Shell - v0.1\n"										/* banner string */
					"ISZC426 - Network Programming | EC1 - P1\n"
					"Ankur Tyagi - 2012HZ13084 | 27/AUG/2013\n";
	char *user = NULL;																		/* poniter -> char that holds username */
	char hostname[MAXHOSTSIZE] = "\0";														/* buffer to hold hostname */
	char cwd[MAXCWDSIZE] = "\0";															/* buffer to hold current working directory */
	char *prompt = NULL;																	/* pointer -> char that hold the prompt string */

	printf("\n%s\n", banner);																/* start with a banner */
	while (TRUE) {																			/* command parsing/execution loop */
		char cmdline[MAXCMDSIZE];															/* buffer to hold commandline */
		commandInfo cinfo;																	/* struct to hold command info */
		memset(&cinfo, '\0', sizeof(cinfo));												/* zero out the buffer */

		if ((user = getenv("LOGNAME")) == NULL) { user = "user"; }							/* read username of currently logged in user (default: "user") */
		if (gethostname(hostname, sizeof(hostname))) { strcpy(hostname, "localhost"); }		/* read hostname (default: "localhost") */
		if (getcwd(cwd, sizeof(cwd)) == NULL) { strcpy(cwd, "\0"); }						/* read current working directory (default: null) */
		printf("[%s] at [%s] in [%s] ", user, hostname, cwd);								/* show an informative prompt made up of username, hostname and cwd */

		readCommand(cmdline);																/* read a command from stdin */
		if (!strcmp(cmdline, "") || !strcmp(cmdline, "\n") || !strcmp(cmdline, " ")) {		/* continue if only a newline or space was received */
			continue;
		}

		if (parseCommand(cmdline, &cinfo) == -1) {											/* parse commandline and populate the command info struct */
			return EXIT_FAILURE;
		} else {
			parseCommand(cmdline, &cinfo);													/* if no errors were reported, continue parsing command */
		}

		if (DEBUG) { showCommand(&cinfo); }													/* if DEBUG is on, show parsed command */

		signal(SIGABRT, handleInterrupt);													/* register signals with a custom handler */
		signal(SIGHUP, handleInterrupt);
		signal(SIGILL, handleInterrupt);
		signal(SIGINT, handleInterrupt);
		signal(SIGKILL, handleInterrupt);
		signal(SIGQUIT, handleInterrupt);
		signal(SIGSEGV, handleInterrupt);
		signal(SIGTERM, handleInterrupt);
		signal(SIGSTOP, handleInterrupt);
		signal(SIGTSTP, handleInterrupt);
		signal(SIGCHLD, handleInterrupt);

		if (!strcmp(cinfo.cliargs[0], "exit")) {											/* call builtin hanlder for exit */
			if (DEBUG) { printf("[D] main: calling exit handler: do_exit\n"); }
			do_exit(EXIT_SUCCESS);
		} else if (!strcmp(cinfo.cliargs[0], "info")) {										/* call builtin handler for info and show return status */
			if (DEBUG) { printf("[D] main: calling info handler: do_info\n"); }
			rc = do_info();
			if (DEBUG) { printf("[D] main: returned from info handler: do_info (RC: %d)\n", rc); }
		} else if (!strcmp(cinfo.cliargs[0], "jobs")) {										/* call builtin handler for jobs and show return status */
			if (DEBUG) { printf("[D] main: calling jobs handler: do_jobs\n"); }
			rc = do_jobs();
			if (DEBUG) { printf("[D] main: returned from jobs handler: do_jobs (RC: %d)\n", rc); }
		} else if (!strcmp(cinfo.cliargs[0], "cd")) {										/* call builtin handler for cd and show return status */
			if (DEBUG) { printf("[D] main: calling cd handler: do_cd\n"); }
			rc = do_cd(&cinfo);
			if (DEBUG) { printf("[D] main: returned from cd handler: do_cd (RC: %d)\n", rc); }
		} else {																			/* if its a non builtin command */
			if (!checkCommand(&cinfo)) {													/* check if its a valid command (not a shell builtin) */
				pid_t pid = do_execvp(&cinfo);												/* fork/execvp this command */

				if (cinfo.background == FALSE) {											/* if command is not to be executed in background */
					pid_t done_pid = waitpid(pid, NULL, 0);									/* wait for the child to complete execution */
					if (done_pid != -1 && done_pid != 0) {									/* when waitpid returns successfully */
						printf("\n");														/* evaluate and print exit status of child */
						printf("[+] main: process %d has changed state (execution complete)\n", done_pid);
					}
				} else if (cinfo.background == TRUE) {										/* if comamnd is to be executed in background */
					pid_t done_pid = waitpid(pid, NULL, WNOHANG);							/* nonblocking wait for the child to complete execution */
					if (done_pid != -1 && done_pid != 0) {									/* when waitpid return successfully */
						printf("\n");														/* evaluate and print exit status of child */
						printf("[+] main: background process %d has changed state (execution complete)\n", done_pid);
					}
				}
			}
		}
	}

	return 0;
}

void do_exit(int rc) {																		/* builtin "exit" command handler */
	printf("\n");

	if (jobscount == 0) {																	/* if there are no background jobs */
		exit(rc);																			/* exit cleanly */
	} else {																				/* else wait for background jobs to complete */
		printf("[+] do_exit: waiting for %d jobs to complete", jobscount);
		do_jobs();																			/* show a listing of currently active background jobs */
	}
}

int do_info(void) {																			/* builtin "info" command handler */
	pid_t pid = getpid();																	/* read pid of shell */
	pid_t ppid = getppid();																	/* read pid of parent */
	uid_t ruid = getuid();																	/* get user id of currently loggedin user */
	uid_t euid = geteuid();																	/* get effective user id of currently loggedin user */
	char *cwd = getcwd(NULL, 0);															/* get current working directory */

	printf("\n");
	printf("--------+--------+--------+--------+-----\n");
	printf(" PID    | PPID   | RUID   | EUID   | CWD \n");
	printf("--------+--------+--------+--------+-----\n");
	printf(" %-6d | %-6d | %-6d | %-6d | %-s\n", pid, ppid, ruid, euid, cwd);				/* show a tabular listing of above details */
	printf("\n");

	return 0;
}

int do_jobs(void) {																			/* builtin "jobs" command handler */
	printf("\n");

	jobInfo *job = jobslist;																/* pointer to the start of the global jobs linked list */

	printf("--------+--------+---------\n");
	printf(" ID#    | PID    | Command \n");
	printf("--------+--------+---------\n");
	while(job != NULL) {																	/* loop over the jobs linked list */
		printf(" %-6d | %-6d | %s\n", job->id, job->pid, job->command);						/* print job info */
		job = job->next;
	}

	printf("\n");
	printf("Total: %d\n", jobscount);														/* show count of currently active jobs */
	printf("\n");

	return 0;
}

int do_cd(commandInfo *cinfo) {																/* builtin "cd" handler */
	if (cinfo->argscount == 1) {															/* if no argument was passed to cd */
		chdir(getenv("HOME"));																/* change cwd to loggedin user's HOME */
		return EXIT_SUCCESS;
	} else if (chdir(cinfo->cliargs[1]) == -1) {											/* else change cwd to requested directory */
		perror("[-] do_cd");																/* if requested directory is not available show error message */
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int readCommand(char *str) {																/* reads user command from stdin */
	fgets(str, MAXCMDSIZE, stdin);
	if (strlen(str) >= 2) {																	/* if command has printable characters */
		str[strlen(str)-1] = '\0';															/* add null terminater after last character */
	} else {																				/* else null terminate the original string */
		str = '\0';
	}

	return EXIT_SUCCESS;
}

int parseCommand(char *cli, commandInfo *cinfo) {											/* parse command and populate command info struct */
	char *cliargs;
	char *cliargss[MAXCMDSIZE]; 

	int argscount = 0;
	cliargs = strtok(cli, " ");																/* read command tokens */
	while (cliargs != NULL) {																/* populate command info struct */
		cliargss[argscount] = cliargs;
		cliargs = strtok(NULL, " ");
		argscount++;																		/* update arguments count */
	}

	if (argscount == 0) { return EXIT_FAILURE; }											/* if argscount is 0, return */

	int i, icliargs = 0;
	for (i = 0; i < argscount; i++) {														/* populate command arguments */
		cinfo->cliargs[icliargs++] = cliargss[i];
	}

	if (!strcmp(cinfo->cliargs[argscount-1], "&")) {										/* if command has to be requested in background */
		cinfo->cliargs[argscount-1] = NULL;													/* remove the '&' character */
		cinfo->background = TRUE;															/* enable flag indicating this job is to be executed in background */
	}

	if (cinfo->argscount == 0) { cinfo->argscount = argscount; }							/* updae arguments count */

	return EXIT_SUCCESS;
}

int showCommand(commandInfo *cinfo) {														/* show parsed command and its arguments */
	int i;  

	for (i = 0; cinfo->cliargs[i] != NULL; i++) {											/* loop over command arguments */
		printf("[+] showCommands: cliargs[%d]=\"%s\"\n", i, cinfo->cliargs[i]);				/* show arguments */
	}

	printf("[+] showCommands: background: %d\n", cinfo->background);						/* show command's background status */
	printf("[+] showCommands: argscount: %d\n", cinfo->argscount);							/* show count of command arguments */

	return EXIT_SUCCESS;
}

int checkCommand(commandInfo *cinfo) {														/* check if the command is available or not */
	return EXIT_SUCCESS;
}

pid_t do_execvp(commandInfo *cinfo) {														/* fork and execvp the command */
	pid_t pid = fork();																		/* fork the parent */

	if (cinfo->background == TRUE) {														/* if this is a background command */
		jobInfo *job = malloc(sizeof(jobInfo));												/* create a new node to add to the global jobs list */
		job->pid = pid;																		/* update job pid */
		job->id = ++jobscount;																/* update job id */
		strcpy(job->command, cinfo->cliargs[0]);
		job->next = NULL;																	/* sanitize the next pointer */

		if (jobslist == NULL) {																/* if this is teh first background job */
			jobslist = job;																	/* point global job pointer to this node */
		} else {
			jobInfo *temp = jobslist;														/* else create a temporary node (to point to the current last node) */
			while(jobslist != NULL) {														/* loop over the jobs linked list */
				temp = temp->next;
			}
			temp->next = job;																/* update the next pointer of last node to the new node */
		}
	}

	if (pid < 0) {																			/* if fork returned a negative return id */
		perror("[!] do_execvp: fork failed!");												/* show error message and exit */
		return EXIT_FAILURE;
	} else if (pid == 0) {																	/* if fork passed and we're in parent process */
		execvp(cinfo->cliargs[0], cinfo->cliargs);											/* overwrite the child with requested command */
		perror("[-] do_execvp");															/* if execvp failed, show error and exit */
		exit(EXIT_FAILURE);
	} else {																				/* if fork passed and we're in child process */
		printf("[+] do_execvp: child process (pid: %d)\n", pid);							/* show child pid and continue executing the child */
		printf("\n");
	}

	return pid;																				/* return fork pid */
}

void handleInterrupt(int sig) {																/* interrup handlet for registered signals */
	int status;
	pid_t pid;

	switch(sig) {																			/* select the caught signal */
		case SIGCHLD:																		/* if it is SIGCHLD */
			pid = wait(&status);															/* wait for the child to complete execution */
			if (pid > 0) {																	/* when child returns */
				jobInfo *prev = NULL, *temp = jobslist;										/* prep to delete the returned child from jobs linked list */
				while(temp != NULL) {														/* loop over the linked list of background jobs */
					if (pid == temp->pid) {													/* if the returned child is found in the jobs linked list */
						if (prev == NULL) {													/* if the child node is the first node in the list */
							jobslist = temp->next;											/* point the global jobslist pointer to child's next */
						} else if (temp->next == NULL) {									/* if the child node is teh last node in the list */
							prev->next = NULL;												/* point child's prev node to NULL */
						} else {															/* if the child node is neither first nor last */
							prev->next = temp->next;										/* update child's prev to point to child's next */
						}
						--jobscount;														/* update global jobs count */
						free(temp);															/* free up the child node and return */

						printf("\n");														/* evaluate and print exit status of child */
						printf("[+] main: background process %d has changed state (execution complete)\n", pid);

						return;
					}
					prev = temp;															/* if child node is not yet found, keep traversing the list */
					temp = temp->next;
				}
			}
			break; 

		case SIGABRT:																		/* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGABRT! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGHUP:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGHUP! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGILL:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGILL! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGINT:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGINT! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGKILL:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGKILL! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGQUIT:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGQUIT! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGSEGV:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGSEGV! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGTERM:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGTERM! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGSTOP:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGSTOP! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		case SIGTSTP:                                                                       /* call builtin "exit" handler */
			printf("\n[+] handleInterrupt: recieved SIGTSTP! exiting.\n");
			do_exit(EXIT_SUCCESS);
			break;

		default:
			printf("[+] handleInterrupt: handling default ...");
			break;
	}
}

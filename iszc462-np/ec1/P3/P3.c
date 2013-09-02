#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
 
#define FALSE			0					/* verbose boolean flags */
#define TRUE			!FALSE
#define DEBUG			!TRUE

#define MAXINPUTBUFSIZE	1024				/* buffer size for raw user input */


pid_t pid1, pid2, pid3;
pid_t pid4, pid5, pid6, pid7;				/* stores pid from fork */
int buflen;									/* length of input string */
char infdbuf[MAXINPUTBUFSIZE];				/* buffer to read raw string input */

key_t key;
int mask, msgid;

struct msgbuf {
	long msgtype;
	char message[MAXINPUTBUFSIZE];
};
struct msgbuf mbuf;


int doP3(char []);


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

		buflen = strlen(infdbuf);

		if (!strcmp(infdbuf, "") || !strcmp(infdbuf, "\n") || !strcmp(infdbuf, " ") || buflen <= 0) {
			continue;
		} else {
			if (infdbuf[buflen-1] == '\n') {
				infdbuf[buflen-1] = '\0';
			}
			if (!strcmp(infdbuf, "exit")) {
				printf("c1: received exit!\n");
				printf("\n");
				exit(EXIT_SUCCESS);
			} else {
				doP3(infdbuf);

				int temp;
				for(temp=0; temp<6; ++temp) { wait(NULL); }

				return EXIT_SUCCESS;
			}
		}

	}

	return EXIT_SUCCESS;
}

int doP3(char infdbuf[]) {
	printf("\n");

	pid1 = getpid();
	printf("c1 (#%d): received message \'%s\'\n", pid1, infdbuf);
	char *str = infdbuf;
	for(; *str; ++str) { *str = tolower(*str); }
	printf("c1 (#%d): transform: message -> tolower -> \'%s\'\n", pid1, infdbuf);

	key = getuid();
	mask = 0666;

	mbuf.msgtype = 1;
	strcpy(mbuf.message, infdbuf);
	size_t msize = strlen(mbuf.message); 

	msgid = msgget(key, mask | IPC_CREAT);
	if (DEBUG) { printf("c1 (#%d): key: %d | mask: %o | msgid = %d | msgtype: %ld | msize: %d\n", pid1, key, mask, msgid, mbuf.msgtype, msize); }

	if (msgid != -1) {
		if (DEBUG) { printf("c1 (#%d): adding message \'%s\' of len %dB to message queue: %d\n", pid1, mbuf.message, msize, msgid); }
		int rv = msgsnd(msgid, &mbuf, sizeof(mbuf) - sizeof(long), 0);
		if (rv == -1) {
			if (DEBUG) { printf("c1 (#%d): msgsnd failed!\n", pid1); }
			perror("msgsnd");
		} else {
			if (DEBUG) { printf("c1 (#%d): added message to message queue: %d\n", pid1, msgid); }
		}
	}

	if (DEBUG) { printf("c1 (#%d): spawning child c2 ...\n", pid1); }

	if ((pid2 = fork()) == -1) {
		printf("c1 (#%d): forking c2 failed!\n", pid1);
		exit(EXIT_FAILURE);

	} else if (pid2 == 0) { /* first child of c1 -> c2 */
		pid2 = getpid();
		if (DEBUG) { printf("c2 (#%d): am alive!\n", pid2); }

		msgrcv(msgid, &mbuf, sizeof(mbuf) - sizeof(long), mbuf.msgtype, 0);
		if (DEBUG) { printf("c2 (#%d): read message \'%s\' of len %dB from message queue: %d\n", pid2, mbuf.message, strlen(mbuf.message), msgid); }
		strcat(mbuf.message, "C2");
		printf("c2 (#%d): transform: message -> append \'C2\' -> \'%s\'\n", pid2, mbuf.message);
		if (DEBUG) { printf("c2 (#%d): adding message \'%s\' of len %dB to message queue: %d\n", pid2, mbuf.message, msize, msgid); }
		int rv = msgsnd(msgid, &mbuf, sizeof(mbuf) - sizeof(long), 0);
		if (rv == -1) {
			printf("c2 (#%d): msgsnd failed!\n", pid2);
			perror("msgsnd");
		} else {
			if (DEBUG) { printf("c2 (#%d): added message to message queue: %d\n", pid2, msgid); }
		}

		if (DEBUG) { printf("c2 (#%d): spawning child c3 ...\n", pid2); }

		if ((pid3 = fork()) == -1) {
			printf("c2 (#%d): forking c3 failed!\n", pid2);
			exit(EXIT_FAILURE);

		} else if (pid3 == 0) { /* first child of c2 -> c3 */
			pid3 = getpid();
			if (DEBUG) { printf("c3 (#%d): am alive!\n", pid3); }

			msgrcv(msgid, &mbuf, sizeof(mbuf) - sizeof(long), mbuf.msgtype, 0);
			if (DEBUG) { printf("c3 (#%d): read message \'%s\' of len %dB from message queue: %d\n", pid3, mbuf.message, strlen(mbuf.message), msgid); }
			char temp[MAXINPUTBUFSIZE];
			strcpy(temp, mbuf.message);
			strcpy(mbuf.message, "C3");
			strcat(mbuf.message, temp);
			printf("c3 (#%d): transform: message -> prepend \'C3\' -> \'%s\'\n", pid3, mbuf.message);
			if (DEBUG) { printf("c3 (#%d): adding message \'%s\' of len %dB to message queue: %d\n", pid3, mbuf.message, msize, msgid); }
			int rv = msgsnd(msgid, &mbuf, sizeof(mbuf) - sizeof(long), 0);
			if (rv == -1) {
				printf("c3 (#%d): msgsnd failed!\n", pid3);
				perror("msgsnd");
			} else {
				if (DEBUG) { printf("c3 (#%d): added message to message queue: %d\n", pid3, msgid); }
			}

			if (DEBUG) { printf("c3 (#%d): spawning child c5 ...\n", pid3); }

			if ((pid5 = fork()) == -1) {
				printf("c3 (#%d): forking c5 failed!\n", pid3);
				exit(EXIT_FAILURE);

			} else if (pid5 == 0) { /* first child of c3 -> c5 */
				pid5 = getpid();
				if (DEBUG) { printf("c5 (#%d): am alive!\n", pid5); }

				msgrcv(msgid, &mbuf, sizeof(mbuf) - sizeof(long), mbuf.msgtype, 0);
				if (DEBUG) { printf("c5 (#%d): read message \'%s\' of len %dB from message queue: %d\n", pid5, mbuf.message, strlen(mbuf.message), msgid); }

				int count = 0;
				char *str = mbuf.message;
				for(; *str; ++str) {
					++count;
					if (count == 5) { *str = toupper(*str); }
				}

				printf("c5 (#%d): transform: message -> 5th char toupper -> \'%s\'\n", pid5, mbuf.message);
				if (DEBUG) { printf("c5 (#%d): adding message \'%s\' of len %dB to message queue: %d\n", pid5, mbuf.message, msize, msgid); }
				int rv = msgsnd(msgid, &mbuf, sizeof(mbuf) - sizeof(long), 0);
				if (rv == -1) {
					printf("c5 (#%d): msgsnd failed!\n", pid5);
					perror("msgsnd");
				} else {
					if (DEBUG) { printf("c5 (#%d): added message to message queue: %d\n", pid5, msgid); }
				}

				if (DEBUG) { printf("c5 (#%d): m done!\n", pid5); }
				return EXIT_SUCCESS;

			} else { /* in c3 */
				if (DEBUG) { printf("c3 (#%d): spawning child c6 ...\n", pid3); }

				if ((pid6 = fork()) == -1) {
					printf("c3 (#%d): forking c6 failed!\n", pid3);
					printf("\n");
					exit(EXIT_FAILURE);

				} else if (pid6 == 0) { /* second child of c3 -> c6 */
					pid6 = getpid();
					if (DEBUG) { printf("c6 (#%d): am alive!\n", pid6); }

					msgrcv(msgid, &mbuf, sizeof(mbuf) - sizeof(long), mbuf.msgtype, 0);
					if (DEBUG) { printf("c6 (#%d): read message \'%s\' of len %dB from message queue: %d\n", pid6, mbuf.message, strlen(mbuf.message), msgid); }

					int count = 0;
					char *str = mbuf.message;
					for(; *str; ++str) {
						++count;
						if (count == 6) { *str = toupper(*str); }
					}

					printf("c6 (#%d): transform: message -> 6th char toupper -> \'%s\'\n", pid6, mbuf.message);
					if (DEBUG) { printf("c6 (#%d): adding message \'%s\' of len %dB to message queue: %d\n", pid6, mbuf.message, msize, msgid); }
					int rv = msgsnd(msgid, &mbuf, sizeof(mbuf) - sizeof(long), 0);
					if (rv == -1) {
						printf("c6 (#%d): msgsnd failed!\n", pid6);
						perror("msgsnd");
					} else {
						if (DEBUG) { printf("c6 (#%d): added message to message queue: %d\n", pid6, msgid); }
					}

					if (DEBUG) { printf("c6 (#%d): m done!\n", pid6); }
					return EXIT_SUCCESS;

				} else { /* in c3 */
					if (DEBUG) { printf("c3 (#%d): spawning child c7 ...\n", pid3); }

					if ((pid7 = fork()) == -1) {
						printf("c3 (#%d): forking c7 failed!\n", pid3);
						printf("\n");
						exit(EXIT_FAILURE);

					} else if (pid7 == 0) { /* third child of c3 -> c7 */
						pid7 = getpid();
						if (DEBUG) { printf("c7 (#%d): am alive!\n", pid7); }

						msgrcv(msgid, &mbuf, sizeof(mbuf) - sizeof(long), mbuf.msgtype, 0);
						if (DEBUG) { printf("c7 (#%d): read message \'%s\' of len %dB from message queue: %d\n", pid7, mbuf.message, strlen(mbuf.message), msgid); }

						int count = 0;
						char *str = mbuf.message;
						for(; *str; ++str) {
							++count;
							if (count == 7) { *str = toupper(*str); }
						}

						printf("c7 (#%d): transform: message -> 7th char toupper -> \'%s\'\n", pid7, mbuf.message);
						if (DEBUG) { printf("c7 (#%d): adding message \'%s\' of len %dB to message queue: %d\n", pid7, mbuf.message, msize, msgid); }
						int rv = msgsnd(msgid, &mbuf, sizeof(mbuf) - sizeof(long), 0);
						if (rv == -1) {
							printf("c7 (#%d): msgsnd failed!\n", pid7);
							perror("msgsnd");
						} else {
							if (DEBUG) { printf("c7 (#%d): added message to message queue: %d\n", pid7, msgid); }
						}

						if (DEBUG) { printf("c7 (#%d): m done!\n", pid7); }
						return EXIT_SUCCESS;

					} else { /* in c3 */
						if (DEBUG) { printf("c3 (#%d): waiting for child c7 to complete ...\n", pid3); }
						//wait(NULL);
						waitpid(pid7, NULL, NULL);
						if (DEBUG) { printf("c3 (#%d): child c7 done!\n", pid3); }
						return EXIT_SUCCESS;
					}

					if (DEBUG) { printf("c3 (#%d): waiting for child c6 to complete ...\n", pid3); }
					//wait(NULL);
					waitpid(pid6, NULL, NULL);
					if (DEBUG) { printf("c3 (#%d): child c6 done!\n", pid3); }
					return EXIT_SUCCESS;
				}

				if (DEBUG) { printf("c3 (#%d): waiting for child c5 to complete ...\n", pid3); }
				//wait(NULL);
				waitpid(pid5, NULL, NULL);
				if (DEBUG) { printf("c3 (#%d): child c5 done!\n", pid3); }
				return EXIT_SUCCESS;
			}

			if (DEBUG) { printf("c3 (#%d): m done!\n", pid3); }
			return EXIT_SUCCESS;

		} else { /* in c2 */
			if (DEBUG) { printf("c2 (#%d): spawning child c4 ...\n", pid2); }

			if ((pid4 = fork()) == -1) {
				printf("c2 (#%d): forking c4 failed!\n", pid2);
				printf("\n");
				exit(EXIT_FAILURE);

			} else if (pid4 == 0) { /* second child of c2 -> c4 */
				pid4 = getpid();
				if (DEBUG) { printf("c4 (#%d): am alive!\n", pid4); }

				msgrcv(msgid, &mbuf, sizeof(mbuf) - sizeof(long), mbuf.msgtype, 0);
				if (DEBUG) { printf("c4 (#%d): read message \'%s\' of len %dB from message queue: %d\n", pid4, mbuf.message, strlen(mbuf.message), msgid); }

				int count = 0;
				char *str = mbuf.message;
				for(; *str; ++str) {
					++count;
					if (count == 4) { *str = toupper(*str); }
				}

				printf("c4 (#%d): transform: message -> 4th char toupper -> \'%s\'\n", pid4, mbuf.message);
				if (DEBUG) { printf("c4 (#%d): adding message \'%s\' of len %dB to message queue: %d\n", pid4, mbuf.message, msize, msgid); }
				int rv = msgsnd(msgid, &mbuf, sizeof(mbuf) - sizeof(long), 0);
				if (rv == -1) {
					printf("c4 (#%d): msgsnd failed!\n", pid4);
					perror("msgsnd");
				} else {
					if (DEBUG) { printf("c4 (#%d): added message to message queue: %d\n", pid4, msgid); }
				}

				if (DEBUG) { printf("c4 (#%d): m done!\n", pid4); }
				return EXIT_SUCCESS;

			} else { /* in c2 */
				if (DEBUG) { printf("c2 (#%d): waiting for child c4 to complete ...\n", pid2); }
				//wait(NULL);
				waitpid(pid4, NULL, NULL);
				if (DEBUG) { printf("c2 (#%d): child c4 done!\n", pid2); }
				return EXIT_SUCCESS;
			}

			if (DEBUG) { printf("c2 (#%d): waiting for child c3 to complete ...\n", pid2); }
			//wait(NULL);
			waitpid(pid3, NULL, NULL);
			if (DEBUG) { printf("c2 (#%d): child c3 done!\n", pid2); }
			return EXIT_SUCCESS;
		}

	} else { /* in c1 */
		if (DEBUG) { printf("c1 (#%d): waiting for child c2 to complete ...\n", pid1); }
		//wait(NULL);
		waitpid(pid2, NULL, NULL);
		if (DEBUG) { printf("c1 (#%d): child c2 done!\n", pid1); }
	}

	if (DEBUG) { printf("c1 (#%d): removing message queue: %d\n", pid1, msgid); }
	int rv = msgctl(msgid, IPC_RMID, NULL);
	if (rv == -1) {
		printf("c1 (#%d): msgctl - IPC_RMID failed!\n", pid1);
		perror("msgctl");
	}

	printf("\n");
	return EXIT_SUCCESS;
}


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <ftw.h>
#include <dirent.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>


struct chat_info {
	long mtype;
	int usernum;
	int lastid;
	long msgcount;
};
struct chat_msg {
	long mtype;
	char content[1000];
	int id;
};

void sender(int mqid, int id, int msgcount, int usernum) {
	struct chat_msg msg;
	struct chat_info info;
	int msgsize, infosize;
	int i;
	msgsize = sizeof(msg);
	infosize = sizeof(info);
	while (1) {
		scanf("%s", msg.content);
		msgrcv(mqid, &info, infosize, 1, NULL);
		if (info.usernum == 1) {
			if (strcmp(msg.content, "talk_quit") == 0) {
				msgctl(mqid, IPC_RMID, NULL);
				exit(1);
			}
			printf("talk_wait...\n");
			msgsnd(mqid, &info, infosize, NULL);
			continue;
		}
		msg.mtype = info.msgcount;
		msg.id = id;
		for (i = 0; i < info.usernum; i++) {
			msgsnd(mqid, &msg, msgsize, NULL);
		}
		if (strcmp(msg.content, "talk_quit") == 0) {
			info.usernum--;
			info.msgcount++;
			msgsnd(mqid, &info, infosize, NULL);
			exit(1);
		}
		else {
			info.msgcount++;
			msgsnd(mqid, &info, infosize, NULL);
		}

		//printf("snd %d : %d\n",id,info.msgcount);
	}
}
void receiver(int mqid, int id, int msgcount, int usernum, int pid) {
	int i;
	int status;
	struct chat_msg msg;
	struct chat_info info;
	int msgsize, infosize;
	msgsize = sizeof(msg);
	infosize = sizeof(info);
	printf("%d : %d\n", id, msgcount);
	while (1) {
		i = msgrcv(mqid, &msg, msgsize, msgcount, IPC_NOWAIT);
		if (i != -1) {
			//printf("rcv %d : %d\n",id,msgcount);
			if (msg.id != id && msg.content[0] != '\0') {
				printf("%d : %s\n", msg.id, msg.content);
			}
			else if (msg.id != id && msg.content[0] == '\0') {
				msgrcv(mqid, &info, infosize, 1, NULL);
				if (info.usernum == 2) {
					printf("talk_start\n");
				}
				msgsnd(mqid, &info, infosize, NULL);
			}
			msgcount++;
		}
		else {
			if (waitpid(pid, &status, WNOHANG) == pid) {
				return;
			}
		}
	}
}

int main(int argc, char *argv[]) {
	key_t key;
	int mqid;
	pid_t pid;
	struct chat_info info;
	struct chat_msg msg;
	int id;
	int msgcount;
	int usernum;
	size_t infosize, msgsize;
	int i;
	setvbuf(stdout, NULL, _IONBF, 0);
	infosize = sizeof(struct chat_info);
	msgsize = sizeof(struct chat_msg);
	info.mtype = 1;
	key = ftok("./p1-a.c", 3);
	mqid = msgget(key, 0666 | IPC_CREAT | IPC_EXCL);
	if (mqid == -1) {
		mqid = msgget(key, 0666);
		msgrcv(mqid, &info, infosize, 1, NULL);
		id = info.lastid + 1;
		info.lastid++;
		msgcount = info.msgcount;
		info.msgcount++;
		usernum = info.usernum + 1;
		info.usernum++;
		msg.mtype = msgcount;
		msg.id = id;
		printf("id=%d\n", id);
		printf("talk_start\n");
		msg.content[0] = '\0';
		for (i = 0; i < usernum - 1; i++) {
			msgsnd(mqid, &msg, msgsize, NULL);
		}
		msgsnd(mqid, &info, infosize, NULL);
	}
	else {
		info.usernum = 1;
		info.msgcount = 2;
		info.lastid = 1;
		info.mtype = 1;
		msgcount = 2;
		id = 1;
		msgsnd(mqid, &info, infosize, NULL);
		printf("id=%d\n", info.lastid);
		printf("talk_wait...\n");
		msgrcv(mqid, &msg, msgsize, info.msgcount, NULL);
		printf("talk_start\n");
	}
	msgcount++;
	pid = fork();
	if (pid == 0)
		sender(mqid, id, msgcount, usernum);
	else
		receiver(mqid, id, msgcount, usernum, pid);
}

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
#define BUFNUM 5
struct message {
	int id;
	char content[1000];
	int reader_num;
};
struct shared_memory {
	int usernum;
	int users[4];
	struct message msg[BUFNUM];
	int index;
};
union semun {
	int val;
	struct semid_ds *buf;
	ushort *array;
};

int writer(int id, struct shared_memory *sh, int smsemid, int index) {
	int writesem;
	union semun arg;
	struct sembuf sbuf;
	int i;
	char content[1000];
	int usernum;
	int readersem;
	ushort buf[5];
	writesem = semget(3333010, BUFNUM, 0666);
	readersem = semget(4444010, 4, 0666);
	while (1) {
		scanf("%s", content);
		sbuf.sem_op = -1;
		sbuf.sem_num = 0;
		semop(smsemid, &sbuf, 1);
		index = sh->index;
		sh->index = (sh->index + 1) % BUFNUM;
		usernum = sh->usernum;
		sbuf.sem_op = 1;
		semop(smsemid, &sbuf, 1);
		sbuf.sem_op = -1;
		sleep(3);
		sbuf.sem_num = index;
		semop(writesem, &sbuf, 1);
		strcpy(sh->msg[index].content, content);
		sh->msg[index].id = id;
		sh->msg[index].reader_num = usernum;
		for (i = 0; i < 4; i++) {
			sbuf.sem_op = 1;
			sbuf.sem_num = i;
			semop(readersem, &sbuf, 1);
		}
		if (strcmp(content, "talk_quit") == 0) {
			sbuf.sem_op = -1;
			sbuf.sem_num == 0;
			semop(smsemid, &sbuf, 1);
			sh->usernum--;
			sh->users[id - 1] = 0;
			sbuf.sem_op = 1;
			semop(smsemid, &sbuf, 1);
			exit(1);
		}
	}
}
int reader(int id, struct shared_memory *sh, int smsemid, int index, int pid) {
	int readersem, bufreadsem, writesem;
	int i;
	union semun arg;
	ushort buf[5];
	struct sembuf sbuf;
	int status;
	readersem = semget(4444010, 4, 0666);
	bufreadsem = semget(5555010, BUFNUM, 0666);
	writesem = semget(3333010, BUFNUM, 0666);
	sbuf.sem_flg = 0;
	while (1) {

		sbuf.sem_op = -1;
		sbuf.sem_num = id - 1;
		semop(readersem, &sbuf, 1);
		sbuf.sem_num = index;
		semop(bufreadsem, &sbuf, 1);
		if (sh->msg[index].id != id)
			printf("%d : %s\n", sh->msg[index].id, sh->msg[index].content);
		else if (sh->msg[index].id == id && strcmp(sh->msg[index].content, "talk_quit") == 0) {
			if (waitpid(pid, &status, NULL) == pid) {
				sbuf.sem_op = 1;
				semop(bufreadsem, &sbuf, 1);
				exit(1);
			}
		}
		sh->msg[index].reader_num--;
		if (sh->msg[index].reader_num == 0) {
			sbuf.sem_op = 1;
			sbuf.sem_num = index;
			semop(writesem, &sbuf, 1);
		}
		sbuf.sem_op = 1;
		semop(bufreadsem, &sbuf, 1);
		index = (index + 1) % BUFNUM;
	}
}
int main(int argc, char *argv[]) {
	int readersem, bufreadsem, writesem;
	int smsemid;
	int shmid;
	union semun arg;
	struct sembuf sbuf;
	int i;
	int id;
	struct shared_memory *sh;
	pid_t pid;
	int index;
	ushort buf[5];
	setvbuf(stdout, NULL, _IONBF, 0);
	sbuf.sem_flg = 0;
	smsemid = semget(2222010, 1, 0666 | IPC_CREAT | IPC_EXCL);
	shmid = shmget(1111010, sizeof(struct shared_memory), 0666 | IPC_CREAT | IPC_EXCL);
	if (smsemid == -1) {
		readersem = semget(4444010, 4, 0666);
		bufreadsem = semget(5555010, BUFNUM, 0666);
		writesem = semget(3333010, BUFNUM, 0666);
		smsemid = semget(2222010, 1, 0666);
		shmid = shmget(1111010, sizeof(struct shared_memory), 0666);
		sh = shmat(shmid, 0, 0);
		for (i = 0; i < 4; i++) {
			if (sh->users[i] == 0)
				break;
		}
		id = i + 1;
		sbuf.sem_op = -1;
		sbuf.sem_num = 0;
		semop(smsemid, &sbuf, 1);
		arg.val = 0;
		semctl(readersem, id - 1, SETVAL, arg);
		sh->usernum++;
		sh->users[i] = 1;
		index = sh->index;
		sbuf.sem_op = 1;
		semop(smsemid, &sbuf, 1);
		printf("id = %d\n", id);
	}
	else {
		readersem = semget(4444010, 4, 0666 | IPC_CREAT);
		bufreadsem = semget(5555010, BUFNUM, 0666 | IPC_CREAT);
		writesem = semget(3333010, BUFNUM, 0666 | IPC_CREAT);
		for (i = 0; i < BUFNUM; i++) {
			buf[i] = 1;
		}
		arg.array = buf;
		semctl(writesem, 0, SETALL, arg);
		for (i = 0; i < BUFNUM; i++) {
			buf[i] = 1;
		}
		arg.array = buf;
		semctl(bufreadsem, 0, SETALL, arg);
		for (i = 0; i < 4; i++) {
			buf[i] = 0;
		}
		arg.array = buf;
		semctl(readersem, 0, SETALL, arg);
		arg.val = 1;
		semctl(smsemid, 0, SETVAL, arg);
		sbuf.sem_op = -1;
		sbuf.sem_num = 0;
		semop(smsemid, &sbuf, 1);
		sh = shmat(shmid, 0, 0);
		sh->usernum = 1;
		id = 1;
		sh->users[0] = 1;
		for (i = 1; i < 4; i++)
			sh->users[i] = 0;
		sh->index = 0;
		index = 0;
		sbuf.sem_op = 1;
		semop(smsemid, &sbuf, 1);
		printf("id = %d\n", id);
	}
	pid = fork();
	if (pid == 0) {
		writer(id, sh, smsemid, index);
	}
	else {
		reader(id, sh, smsemid, index, pid);
	}
}

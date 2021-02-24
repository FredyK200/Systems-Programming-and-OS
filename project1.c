/*
	Project 1: Fork & Pipe Example
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#define READ_END	0
#define WRITE_END	1

int main(int argc, char *argv[])
{
		/* argument check */
	if (argc != 2) {
		printf("usage: ./project1 n; n > 0\n");
		return 1;
	}
	if ( atoll(argv[1]) < 1){
		printf("usage: ./project1 n; n > 0\n");
		return 1;
	}
		/* define variables */
	char *a = argv[1];
	long long parent_num = atoll(a);
	long long child_num;
	int status;	
	int stop_time = 0;
	long long peak_value = parent_num;
	pid_t pid;
	int fd1[2]; // P=>C
	int fd2[2]; // C=>P
	
		/* create pipes */
	if (pipe(fd1) == -1) {return 2;}
	if (pipe(fd2) == -1) {return 2;}

		/* fork a child process */
	pid = fork();
	if (pid < 0) {return 3;}

	if (pid > 0) {  /* parent process */
			/* close the unused ends of pipes */
		close(fd1[READ_END]);
		close(fd2[WRITE_END]);
			/* write to pipe1 */
		write(fd1[WRITE_END], &parent_num, sizeof(parent_num)); 
			/* read from pipe2 until a 1 is read or child terminates */
		while(parent_num != 1 || waitpid(-1, &status, WNOHANG) == -1)
		{
			printf("%lld, ", parent_num);
			read(fd2[READ_END], &parent_num, sizeof(parent_num));
			stop_time++;
			if (parent_num > peak_value) {peak_value = parent_num;}
		}
		printf("1 (%d,%lld)\n",stop_time,peak_value);
			/* close pipes */
		close(fd1[WRITE_END]);
		close(fd2[READ_END]);
	}
	else { /* child process */
			/* close the unused ends of pipes */
		close(fd1[WRITE_END]);
		close(fd2[READ_END]);
			/* read from pipe1 */
		read(fd1[READ_END], &child_num, sizeof(child_num));
			/* apply Collatz Conjecture, write each step to pipe2 */
		while(child_num != 1)
		{
			if (child_num % 2 == 0) { child_num = child_num / 2; }
			else { child_num = (3*child_num) + 1; };
			write(fd2[WRITE_END], &child_num, sizeof(child_num));
		}
			/* close the pipes */
		close(fd1[READ_END]);
		close(fd2[WRITE_END]);
	}
	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>

#define workers 4
int active_workers = workers;
void handle_sigint(int sig){
	char msg[128];
	int len = sprintf(msg, "ctrl + c active workers: %d\n", active_workers);
	write(1,msg,len);
}
int main(int argc, char *argv[]){
        int fdr; //read file
        int fdw;    //write file
        char c2c;   //copy char
        

        if (argc != 4) {
        write(2, "Wrong amount of arguments!\n", 27);
        exit(1);
        }

        fdr = open(argv[1], O_RDONLY);
        if (fdr ==-1){
                perror("error opening read-file");
                exit(1);
        }
        c2c = argv[3][0];
        fdw = open(argv[2], O_CREAT | O_WRONLY | O_TRUNC, 0644);
  	if(fdw==-1){perror("error write"); exit(1);
	}
struct sigaction sa;
sa.sa_handler = handle_sigint;
sa.sa_flags = SA_RESTART;
sigemptyset(&sa.sa_mask);
sigaction(SIGINT, &sa, NULL);


pid_t p, mypid, father_pid;
    mypid = -1;
int x = 5;
printf("x before fork is x=%d\n",x);
int pipefd[2];
if(pipe(pipefd) < 0 ){
	perror("pipe error");
	exit(1);
}

for (int j = 0; j < workers; j++){
   p = fork();
    if (p < 0) {
        perror("fork");
        exit(1);
    } 
    else if (p == 0) {
        mypid = getpid();
	close(pipefd[0]);
	printf("Child: %d with Pid (%d) : ready for work\n",j+1,mypid);
	int my_count = 0;
	char buff[128];
	ssize_t rcnt;
	for(;;){
		rcnt = read(fdr,buff,sizeof(buff));
		if ( rcnt == 0){ break;}
		if( rcnt == -1){ perror("error reading child"); exit(1);}
 		for(size_t i = 0 ; i<rcnt;i++){
	        	if(buff[i] == c2c) my_count++;
		}
		
		
	}
	write(pipefd[1], &my_count, sizeof(my_count));
	close(pipefd[1]);
	exit(0);
	
    }
 
    else {
      	
	 mypid = getpid();       
        printf("i am the father (my pid is %d) and my child's is %d\n", mypid, p);
    }
}
close(pipefd[1]);
int total_count = 0;
int partial_count = 0;
for(int j = 0 ; j < workers; j ++){
	read(pipefd[0], &partial_count, sizeof(partial_count));
	total_count += partial_count;
	active_workers--;	
}
//write parent
char out_buff[1024];
size_t len = sprintf(out_buff, "total prallel count, character '%c' appears %d times,\n", c2c,total_count);
size_t idx = 0;
ssize_t wcnt ; 
do{
	wcnt = write(fdw, out_buff + idx, len - idx);
	if( wcnt == -1) {
		perror("error writing to file"); exit (1);
	}
	idx += wcnt;

}while(idx < len);
for(int j = 0; j < workers;j++){
	wait(NULL);
}
printf("\nworkers finished\n");
close(pipefd[0]);
close(fdr);
close(fdw);

   




return 0;
}


#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
void show_pstree(pid_t p){
	int ret;
	char cmd[1024];
	snprintf(cmd,sizeof(cmd), "echo; echo; pstree -a -G -c -p %ld; echo; echo;",(long) p);
	cmd[sizeof(cmd) - 1] = '\0';
	ret = system(cmd);
	if(ret <0){
		perror("system");
		exit(104);
	
	}
}
int main(int argc, char* argv[]){
	if(argc != 3){
		write(2,"error try: ./frontend <file.txt> <character>\n",45);
		exit(1);
	}
	
	int pipe_req[2]; //front end request from dispatcher
	int pipe_res[2]; //front end reponse to dispatcher
	if(pipe(pipe_req) < 0 || pipe(pipe_res) < 0 ) {
		perror("error creating pipes!\n");
		exit(1);
	}
	pid_t p = fork();
	if(p < 0 ){
	 	perror("error fork!\n");
		exit(1);
	}
	else if( p == 0 ){
		close(pipe_req[1]);
		close(pipe_res[0]);
		char fe_req[10]; //front-end --> dispatcher
		char fe_res[10]; //front end <-- dispatcher 
		sprintf(fe_req,"%d",pipe_req[0]);
		sprintf(fe_res,"%d",pipe_res[1]);
		char *args[] = {"./dispatcher",fe_req, fe_res, argv[1], argv[2], NULL};
		execv("./dispatcher",args);
		perror("execv failed");
		exit(1);
			
	}
	else{   		
		close(pipe_req[0]);
		close(pipe_res[1]);
		char buff[1024];
		size_t len, idx;
		ssize_t wcnt,rcnt;
		
		for(;;){
			printf("\nENTER COMMAND: add, remove, info, progress, exit: ");
			if(fgets(buff,sizeof(buff),stdin) == NULL){
			 break;
			}
			if(strncmp(buff,"exit",4) == 0){
				break;
			}	
			if( strncmp(buff,"progress",8) == 0){
				kill(p,SIGUSR1);
			}
			if(strncmp(buff,"info",4)== 0){
                		show_pstree(p);
        	      		  continue;
			 }
			else{
				idx = 0 ;
				len = strlen(buff);
				do{
					wcnt = write(pipe_req[1], buff + idx, len - idx);
					if (wcnt == -1) break;
					idx+=wcnt;
					
				}while(idx<len);
			}
			rcnt = read(pipe_res[0],buff,sizeof(buff) - 1);
			if (rcnt <= 0) break;
			buff[rcnt] = '\0';
			write(1,"dispather response: ",21);
			write(1,buff, rcnt);
	
		}
		close(pipe_req[1]);
		close(pipe_res[0]);
		wait(NULL);

	}	
return 0;
}

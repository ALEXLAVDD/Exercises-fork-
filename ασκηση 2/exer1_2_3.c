
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>



int main(int argc, char *argv[]){
        int fdr; //read file
        int fdw;    //write file
        char c2c;   //copy char
        int count = 0;  //counter

        char buff[1024]; //buff reader
        char out_buff[1024]; //buff output
        ssize_t rcnt, wcnt; //return read-write
        size_t len, idx, i;

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
         if (fdw ==-1){
                perror("error opening write-file");
                exit(1);
        }
        for(;;){
            rcnt  = read(fdr, buff, sizeof(buff) - 1);
            if(rcnt==0){
                break;
            }
            if(rcnt==-1){
                perror("error reading");
                return 1;
            }
            buff[rcnt] = '\0';
            for( i = 0; i < rcnt; i++){
                if( buff[i] == c2c ) count++;
            }
        }
        
        idx = 0;
        len = sprintf(out_buff,"The character '%c' appears %d times in file %s.\n", c2c, count, argv[1]);
        do{
            wcnt = write(fdw,out_buff + idx, len - idx);
            if (wcnt == -1){ /* error */
                perror("error writing to file");
                return 1;
            }
            idx += wcnt;

        }while(idx<len);
        
    
lseek(fdr, 0, SEEK_SET); //can read file from the beggining again.    
pid_t p, mypid, father_pid;
    mypid = -1;
int x = 5;
printf("x before fork is x=%d\n",x);
    p = fork();

    if (p < 0) {
        perror("fork");
        exit(1);
    } 
    else if (p == 0) {
        mypid = getpid();
        father_pid = getppid();
        x  = 10;
        printf("hello world i am '%d' pid and my father is '%d' pid\n", mypid, father_pid);
   	printf("%d\n",x);
	count = 0;
	for (;;) {
            rcnt = read(fdr, buff, sizeof(buff) - 1);
            if (rcnt == 0) break;
            if (rcnt == -1) { perror("error reading child"); exit(1); }
            
            buff[rcnt] = '\0';
            for (i = 0; i < rcnt; i++) {
                if (buff[i] == c2c) count++;
            }
        }
	len = sprintf(out_buff, "CHILD  (PID %d): The character '%c' appears %d times.\n", getpid(), c2c, count);
        idx = 0;
        do {
            wcnt = write(fdw, out_buff + idx, len - idx);
            if (wcnt == -1) { perror("error writing child"); exit(1); }
            idx += wcnt;
        } while (idx < len);
	exit(0);


    }
 
    else {
      	wait(NULL);
	  mypid = getpid();       
        printf("i am the father (my pid is %d) and my child's is %d\n", mypid, p);
        x = 20;
	printf("%d\n",x);
        close(fdr);
	close(fdw);
        
    }

    return 0;




return 0;
}


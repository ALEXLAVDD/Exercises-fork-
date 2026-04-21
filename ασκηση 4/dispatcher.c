#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h> // Απαραίτητο για την waitpid

#define MAX_WORKERS 50

struct worker_info {
        pid_t pid;
        int pipe_to_worker;
        int active;
        int pipe_read_fd;
        long bytes_processed;
        int chars_found;
        long start_offset;
        long end_offset; //used to distribute work  (file)/(workers)
};

struct worker_info workers[MAX_WORKERS];

int got_sigusr1 = 0;
int got_sigchld = 0;
long total_file_size = 0;
int active_worker_count = 0;

void distribute_work() {
        if(active_worker_count == 0) return;
        long chunk = total_file_size / active_worker_count;
        int count = 0;
        for(int i = 0 ; i < MAX_WORKERS; i++){
                if(workers[i].active){
                        workers[i].start_offset = count * chunk;
                        if(count == active_worker_count - 1){ //count starts from 0 so 0 == 1
                                workers[i].end_offset = total_file_size - 1; 
                        }else{
                                workers[i].end_offset = (count + 1) * chunk - 1;
                        }
                        count++;
                }
        }
}

void sighandler (int signum){
        if(signum == SIGUSR1){
                got_sigusr1 = 1;
        }else if (signum == SIGCHLD) {
                got_sigchld = 1;
        }
}

int main(int argc,char* argv[]){
        if (argc < 5){
                write(2,"error dispatcher args\n", 22);
                exit(1);
        }
        
        int fe_read;
        int fe_write;
        sscanf(argv[1],"%d", &fe_read);
        sscanf(argv[2],"%d", &fe_write);
        
        for (int i = 0; i < MAX_WORKERS; i++) {
                 workers[i].active = 0;
                 workers[i].pipe_read_fd = -1;
        }
        
        struct stat st;
        int file_fd = open(argv[3],O_RDONLY);
        if (file_fd < 0) {
                perror("open target file");
                exit(1);
        }
        if (fstat(file_fd, &st) == 0) {
                total_file_size = st.st_size;
        } else { 
                perror("fstat failed");
        }
        close(file_fd);

        char *filename = argv[3];
        char *search_char = argv[4];
        
        struct sigaction sa;
        sigset_t sigset;
        sa.sa_handler = sighandler;
        sa.sa_flags = SA_RESTART;
        sigemptyset(&sigset);
        sa.sa_mask = sigset;
        
        if(sigaction(SIGCHLD, &sa, NULL) < 0) {
                perror("sigaction sigchld");
                exit(1);
        }
        if(sigaction(SIGUSR1, &sa, NULL) < 0 ){
                perror("sigaction sigusr1");
                exit(1);
        }

        fcntl(fe_read, F_SETFL, O_NONBLOCK); //doesnt block read --> 0_nonblock
        
        char buff[1024];
        char reply[1024];
        ssize_t rcnt;
        
        for(;;){
                if(got_sigusr1){
                         long total_p = 0;
                         int total_f = 0;
                         for (int i = 0; i < MAX_WORKERS; i++) {
                             if (workers[i].active) {
                                         total_p += workers[i].bytes_processed;
                                         total_f += workers[i].chars_found;
                              }
                          }
                          float perc = 0;
                          if (total_file_size > 0) perc = ((float)total_p / total_file_size) * 100;

                         sprintf(reply, "Progress: %.2f%%, Found: %d\n", perc, total_f);
                         write(fe_write, reply, strlen(reply));
                         got_sigusr1 = 0;
                }
                
                if (got_sigchld) {
                        pid_t p;
                        while ((p = waitpid(-1, NULL, WNOHANG)) > 0) {
                                for (int i = 0; i < MAX_WORKERS; i++) {
                                        if (workers[i].active && workers[i].pid == p) { //finds dead child and deletes
                                                close(workers[i].pipe_read_fd);
                                                workers[i].active = 0;
                                                active_worker_count--;
                                        }
                                }
                        }
                        got_sigchld = 0;
	
                }

                for (int i = 0; i < MAX_WORKERS; i++) {
                        if (workers[i].active) {
                              long stats[2];
				while (read(workers[i].pipe_read_fd, stats, sizeof(stats)) == sizeof(stats)) {
         			   workers[i].bytes_processed = stats[0];
         			   workers[i].chars_found = (int)stats[1];
       				 }
                        }
                }

                rcnt = read(fe_read, buff, sizeof(buff) - 1);
                if(rcnt > 0 ){
                        buff[rcnt] = '\0';
                        if(strncmp(buff,"add",3) == 0 ){
                                int n = 1;
                                sscanf(buff,"add %d", &n);
                                sprintf(reply,"Adding %d worker(s)...\n",n);
                                write(fe_write, reply,strlen(reply));

                                for(int i = 0 ; i < n ;i++){
                                        int slot = -1; //checking available workers
                                        for (int j = 0 ; j < MAX_WORKERS; j++){
                                                if(workers[j].active == 0){
                                                        slot = j;
                                                        break;
                                                }
                                        }
                                        if(slot != -1){
                                                int pipe_wk[2];
                                                pipe(pipe_wk);
                                                active_worker_count++;
                                                workers[slot].active = 1;
                                                distribute_work();
                                                
                                                pid_t p = fork();
                                                if(p < 0){
                                                        perror("fork failed");
                                                }
                                                else if( p == 0){
                                                        close(pipe_wk[0]);
                                                        
                                                        char pipe_write_end[10];
                                                        char start_s[20];
                                                        char end_s[20];
                                                        
                                                        sprintf(pipe_write_end, "%d", pipe_wk[1]);
                                                        sprintf(start_s, "%ld", workers[slot].start_offset);
                                                        sprintf(end_s, "%ld", workers[slot].end_offset);
                                                        
                                                        execl("./worker", "worker", filename, search_char, pipe_write_end, start_s, end_s, NULL);
                                                        perror("execl failed");
                                                        exit(1);
                                                }else{
                                                        close(pipe_wk[1]);
                                                        workers[slot].pid = p;
                                                        workers[slot].pipe_read_fd = pipe_wk[0];
                                                        workers[slot].bytes_processed = 0;
                                                        workers[slot].chars_found = 0;
                                                        fcntl(workers[slot].pipe_read_fd, F_SETFL, O_NONBLOCK);
                                                }
                                        }
                                }
                        }
                        else if(strncmp(buff, "remove", 6) == 0){
                                int n = 1;
                                sscanf(buff,"remove %d",&n);
                                int removed = 0;
                                
                                for(int i = 0 ; i < n ; i++){
                                         int slot = -1; //checking available workers
                                         for (int j = MAX_WORKERS-1 ; j >= 0; j--){
                                                if(workers[j].active == 1){
                                                        slot = j;
                                                        break;
                                                }
                                         }
                                         if(slot != -1){
                                                kill(workers[slot].pid, SIGTERM);
                                                removed++;
                                         }
                                }
                                sprintf(reply,"Dispatcher: Removed %d worker(s).\n", removed);
                                write(fe_write, reply, strlen(reply));
                        } else {
                                write(fe_write, "Command received.\n", 18);
                        }
                }
                else if(rcnt == 0){
                        break;
                }
                usleep(50000);
        }
        
        for(int i = 0; i < MAX_WORKERS; i++){
            if(workers[i].active){
                kill(workers[i].pid, SIGKILL);
            }
        }
        close(fe_read);
        close(fe_write);

        return 0;
}

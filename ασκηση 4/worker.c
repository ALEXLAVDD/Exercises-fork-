#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
        
        // argv[0] = "./worker"
        // argv[1] = filename 
        // argv[2] = search_char 
        // argv[3] = pipe_write_end
        // argv[4] = start_offset 
        // argv[5] = end_offset 
        
        if (argc < 6) {
                write(2, "Error: worker needs 6 arguments\n", 32);
                exit(1);
        }

        char *filename = argv[1];
        char target_char = argv[2][0];
        int pipe_fd;
	sscanf(argv[3], "%d", &pipe_fd);
	
        long start_offset;
        long end_offset;
	sscanf(argv[4], "%ld", &start_offset);
        sscanf(argv[5], "%ld", &end_offset);
        
        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
                perror("worker: open file");
                exit(1);
        }

        // moving "pointer" selidodeiktis
        if (lseek(fd, start_offset, SEEK_SET) < 0) {
                perror("worker: lseek");
                close(fd);
                exit(1);
        }

        // worker size
        long total_bytes_to_read = end_offset - start_offset + 1;
        long bytes_processed = 0;
        int chars_found = 0;
        
        char buff[1024];
        char pipe_msg[64];
	long stats[2];        
        size_t bytes_read;
        while(bytes_processed < total_bytes_to_read){
		bytes_read = read(fd,buff,sizeof(buff) - 1);
		if(bytes_read == 0 ) break;
		if(bytes_read == -1) {
			perror("read");
			exit(1);
		}
		for(size_t i = 0; i < bytes_read ; i++){
			if(buff[i] == target_char){
				chars_found++;
			}
		}
		bytes_processed += bytes_read;
		stats[0] = bytes_processed;
		stats[1] = (long) chars_found;
		
		write(pipe_fd, stats,sizeof(stats));
		usleep(10000);
	}
       		
		
		
         
        

       
        close(fd);
        close(pipe_fd);

        return 0;
}

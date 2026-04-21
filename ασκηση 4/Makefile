all: frontend dispatcher worker

frontend: frontend.c
	gcc -Wall -g -o frontend frontend.c

dispatcher: dispatcher.c
	gcc -Wall -g -o dispatcher dispatcher.c

worker: worker.c
	gcc -Wall -g -o worker worker.c

clean:
	rm -f frontend dispatcher worker

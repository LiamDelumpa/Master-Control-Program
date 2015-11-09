#include <stdio.h>
#include <wordexp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <proc/readproc.h>

volatile int *sig_flag;
int alarm_h;

void signal_handler(int s){};

void alarm_handler(int sig){
	alarm_h = 1;
}

//Parse information for processes
void print_proc(pid_t pid[]){
	PROCTAB* proc = openproc(PROC_FILLMEM | PROC_FILLSTAT | PROC_FILLSTATUS | PROC_PID, pid);
	proc_t proc_info;
	memset(&proc_info, 0, sizeof(proc_info));
	printf("\t\tCMD\tCUtime\tUtime\tSTART_TIME\n");
	while(readproc(proc, &proc_info) != NULL) {
		printf("%20s:\t%5lld\t%5lld\t%5lld\n",
         proc_info.cmd, proc_info.cutime,
         proc_info.utime, proc_info.start_time);
	}
	closeproc(proc);
}


int main(int argc, char const *argv[])
{
	//attributes for reading input file
	FILE *inputFile;
	char buff[255][255];
	char line[255];
	inputFile = fopen(argv[1],"r");
	int numPrograms = 0;
	int i, length, sig, r;
	wordexp_t expansion;
	wordexp_t progArray[255];
	pid_t pid[numPrograms];

	//attributes for signal
	sig_flag = malloc(sizeof(int));
	*sig_flag = 1;
	signal(SIGALRM, alarm_handler);
	sigset_t SIG_SET;
	sigemptyset(&SIG_SET);
	sigaddset(&SIG_SET, SIGUSR1);
	sigprocmask(SIG_BLOCK, &SIG_SET, NULL);
	PROCTAB* proc;

	


	if(inputFile != NULL){

		while(fgets(line, sizeof(line), inputFile) != NULL){
			length = strlen(line);
			if(line[length-1]=='\n') {
				line[length-1]='\0';
			}
			strcpy(buff[numPrograms], line); //copy line from input file into buffer
			wordexp(buff[numPrograms], &expansion, 0); //expand words in line 
			progArray[numPrograms] = expansion;
			numPrograms++;
		}
		for(i=0; i < numPrograms; ++i){		//forking processes into children
			pid_t temp = fork();		
			pid[i] = temp;

			if(temp < 0){
				printf("Failed to fork");
				exit(1);
			}
			if(temp == 0){ 					//child
				
				
				if(sigwait(&SIG_SET, &sig) < 0 ) printf("Failed to wait");
				printf("Executing %d", i);
				if(execvp(progArray[i].we_wordv[0], progArray[i].we_wordv) == -1){
					perror("Error: ");
					exit(1);
				}
				
			}
		}
		
		for(i=0; i<numPrograms; i++){	
			printf("child  %d is waiting\n", i);
			if(kill(pid[i], SIGUSR1) == -1) perror("Error: ");	//wake children up

		}
		for(i=0; i<numPrograms; i++){	
			printf("Stopping %d\n" , i );
			if(kill(pid[i], SIGSTOP) == -1) perror("Error: ");	//stop children
		}
		
		//Beginning of MCP 3.0 ---> control how long each process is running
		
		int process_running = 1;
		while(process_running == 1){
			process_running = 0;
			for(i = 0; i < numPrograms; i++){
				if(waitpid(pid[i], &r, WNOHANG) == 0){
					process_running = 1;
					printf("Continuing %d\n", i);
					if(kill(pid[i], SIGCONT) == -1) perror("Error: ");
					print_proc(pid);
					alarm(1);
					while(alarm_h == 0){
						;
					}
					alarm_h = 0;
					if(kill(pid[i], SIGSTOP) == -1) perror("Error: ");
				}
			}
		}
		printf("All processes complete\n");
	}
	
	fclose(inputFile);
	return 0;
}
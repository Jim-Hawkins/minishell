//  MSH main file
// Write your msh source code here

//#include "parser.h"
#include <stddef.h>			/* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>



#define MAX_COMMANDS 8

//creamos var global para la funcion mycalc y mycp con dimension 128
char res[128];


// ficheros por si hay redirección
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];
//environment variable to store the accumulated sums
int Acc = 0;

void siginthandler(int param)
{
	printf("****  Saliendo del MSH **** \n");
	//signal(SIGINT, siginthandler);
        exit(0);
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
    //reset first	
    printf("Estamos dentro de la funcion");
    for(int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for ( i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}

void mycalc(char *command[],char *res){

	if(3>5){
		sprintf(res, "La estructura del comando es <operando1><add/mod><operando2>");
	}else{
		int op1 = atoi(command[1]);
		int op2 = atoi(command[3]);
		int sum,mod,rest;
		if(strcmp(command[2], "add")==0){
			sum = op1 + op2;
			Acc = Acc + sum;
			sprintf(res, "[OK] %d + %d = %d; ACC = %d", op1, op2, sum,Acc);

			
		} else if(strcmp(command[2], "mod")==0){
			mod = op1 / op2;
			rest = op1-op2*mod;
			sprintf(res, "[OK] %d %% %d = %d * %d + %d", op1, op2, op2,mod,rest);
		}
	}
}


/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
    /**** Do not delete this code.****/
    int end = 0; 
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];
    int p[2];							//integer-type array to create pipes
    int last_fid;						//saves the edge of the current pipe so the next process can chain to it

    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF){
            if(strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush (stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;
    int num_commands;


	while (1) 
	{
		int status = 0;				//it will inform about the return of a child process
	        int command_counter = 0;			//indicates the number of commands introduced
		int in_background = 0;				//indicates whether the process has to run in background mode
		pid_t son_pid = 0;				//identifier of a child process
		signal(SIGINT, siginthandler);		//handler for Ctrl+C interruption

		// Prompt 
		write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

		// Get command
                //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
                executed_cmd_lines++;
                if( end != 0 && executed_cmd_lines < end) {
                    command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
                }else if( end != 0 && executed_cmd_lines == end)
                    return 0;
                else
                    command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
                //************************************************************************************************


              /************************ STUDENTS CODE ********************************/
	      if (command_counter > 0) {
                if (command_counter > MAX_COMMANDS){
                      printf("Error: Max number of commands is %d\n.", MAX_COMMANDS);
                } 
                //////////////////////////////////////////
                else if (command_counter == 1){
		   
                   if(strcmp(argvv[0][0], "mycalc")==0){

                   	mycalc(argvv[0],res);
			printf("%s\n",res);

                   }
                   /*if(strcmp(argvv[0][0], "mycp")){
                   	mycp(argvv[0]);
                   }*/
                
                   //printf("command_counter %d\n", command_counter);
            	   // Print command
		   //print_command(argvv, filev, in_background);
		   else{
		 	  son_pid = fork();
		  	 // Son's code
		 	  switch(son_pid){
		   		case -1: // error
		   			perror("Error creating the new process");
		   			exit(-1);
				case 0:  // son
			   		execvp(argvv[0][0],argvv[0]);
			   		perror("An error occured while executing the order");
				   	return -1;
				
				   // Father's code
				default:
				   if(!in_background){							//we will wait only if in_backgroun is false (-> foreground)
					while(son_pid != wait(&status));
					if(status < 0){
						perror("Child process terminated with errors:\t");	//display a message to inform the user if the son has had problems
					}
				   printf("%d\n", son_pid);
				  }
				}
			}
		}
                ///////////////////////////
                else {									//using a for loop allows to execute any number of commands from 1 to MAX_COMMANDS
                   //printf("command_counter %d\n", command_counter);
		   for(int i = 0; i < command_counter; i++){				//  by making the father process a 'monitor' in the task of chaining sons through pipes
			if (i < (command_counter-1)){					//if it isn't the last process, create a pipe (there must be 'command_counter - 1' pipes)
				if(pipe(p) < 0){
					perror("pipe Error:\t");
					exit(-1);			
				}
			}
			son_pid = fork();						//both father and son processes will preserve a copy of the pipe created in the previous step
			switch(son_pid){
				case -1:	//error creating a child process
					perror("FORK ERROR:\t");
					exit(-1);	
				case 0:	//SON'S CODE
					//printf("args: %s, %s\n", argvv[i][0], *argvv[i]);
					if(i==0 && filev[0]!=0){
						close(0);
						int file;
						if((file=open(filev[0],O_RDONLY))<0){
							perror("open Error:\t");
							exit(-1);
						}
											
					}
					else if(i==(command_counter-1) && filev[1]!=0){
						close(1);
						int file;
						
						if((file=open(filev[0],O_CREAT|O_WRONLY,0666))<0){
							perror("open Error:\t");
							exit(-1);
						}
											
					}
					else if(filev[2]!=0){
						close(2);
						int file;
						if((file=open(filev[2],O_CREAT|O_WRONLY,0666))<0){
							perror("open Error:\t");
							exit(-1);
						}
											
					}
					if(i != 0){					//every child (except the first one) must close standard input (the keyboard)
						close(STDIN_FILENO); dup(last_fid);	//  and duplicate the output descriptor of the previous pipe
						close(last_fid);			//once created, we can close the duplicate descriptor
					}
					if(i != (command_counter - 1)){		//every child (except the last one) must close standard output (the screen)
						close(STDOUT_FILENO); dup(p[1]);	//   and duplicate the input descriptor of the current pipe
						close(p[1]); close(p[0]);		//once created, we can close the duplicate descriptors of the pipe
					}

					execvp(argvv[i][0], argvv[i]);		//time to execute the #i command
			   		perror("An error occured while executing the order"); //this two lines will only be run if execvp fails
			   		exit(-1);
			   			
				default:	//FATHER'S CODE
					if(i != (command_counter - 1)){		//in every iteration, except the last one, the father process saves the output of
						last_fid = p[0]; 			//  the current pipe so it can be used as input by the next child (p[0] will be overwritten)
						close(p[1]);				//also, the father closes the descriptor p[1] (pipe's input), as it is no longer needed
											//  (even tough p[0] has a copy in last_fid, closing one would close both, as they are copies
											//  of the same descriptor)
					}
					else{						//in the last iteration, we can finally close the remaining descriptor
						close(last_fid);			//  (it would make no difference closing p[0] or last_fid, but this way is clearer)
					}
	
			}
		   }
		   if(!in_background){							//we will wait only if in_backgroun is false (-> foreground)
			while(son_pid != wait(&status));
			if(status < 0){
				perror("Child process terminated with errors:\t");	//display a message to inform the user if the son has had problems
			}
		   printf("%d\n", son_pid);
                  }
              }
        }
        }
	
	return 0;
}

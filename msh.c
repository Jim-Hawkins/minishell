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
#include <ctype.h>



#define MAX_COMMANDS 8
#define BUFFER 512

//global variable to store the result of the functions mycalc and mycp
char res[550];


//files in case there are redirections
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];


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

/*
 * Set global variable 'res' with the result of an addition or division,
 * or an error message if something went wrong.
 * @param command
 * @param res
 * @param num_args
 * @return
 */
void mycalc(char *command[],char *res,int num_args){
	//the 'command' parameter cannot have less or more than four strings (command = mycalc op1 add|mod op2)
	if(num_args != 4){
		sprintf(res, "[ERROR] La estructura del comando es mycalc <operando1><add/mod><operando2>");
	}else{
		//if the number of strings in 'command' is correct, we have to check the operator
		if(strcmp(command[2],"add") != 0 && strcmp(command[2],"mod") != 0){
			sprintf(res, "[ERROR] La estructura del comando es mycalc <operando1><add/mod><operando2>"); return;
		}
		//once checkings are done, convert to integer the operands and define variables for the results (sum, mod(ulus) and rem(ainder))
		int op1 = atoi(command[1]);				//atoi() converts the initial portion of the string argument to int
		int op2 = atoi(command[3]);				//  so, something like "-38.dsaf" is converted to -38; or "abc1" to 0, for example
		int sum, mod, rem;
		int Acc = atoi(getenv("Acc"));			//get the environment variable to add the sum
		char num[128];						//string to store the accumulated result and set the variable Acc
		
		if(strcmp(command[2], "add") == 0){
			sum = op1 + op2;
			Acc = Acc + sum;
			sprintf(res, "[OK] %d + %d = %d; Acc = %d", op1, op2, sum, Acc);
			sprintf(num, "%d", Acc);
			setenv("Acc", num, 1);
			
			
		} else if(strcmp(command[2], "mod") == 0){
			mod = op1 / op2;
			rem = op1 - op2*mod;
			sprintf(res, "[OK] %d %% %d = %d * %d + %d", op1, op2, op2, mod, rem);
		}
	}
}
/*
 * Set global variable 'res' with the result of the copy task,
 * a success or an error message.
 * @param command
 * @param res
 * @param num_args
 * @return
 */
void mycp(char *command[],char *res, int num_args){
	//the 'command' parameter cannot have less or more than three strings (command = mycp file file2)
	if(num_args != 3){
		sprintf(res,"[ERROR] La estructura del comando es mycp <fichero origen><fichero destino>");
	}else{
		//we create the necessary variables for the operation:
		int des1, des2, nread, nwrite;		//file descriptors, bytes read and bytes written
		char buffer[BUFFER];				//buffer for copying the content
		if( (des1 = open(command[1], O_RDONLY)) < 0){
			perror("[ERROR] Error al abrir el fichero origen\n"); return;
		}
		if( (des2 = open(command[2], O_WRONLY|O_CREAT, 0666)) < 0){
			perror("[ERROR] Error al abrir el fichero destino\n"); return;
		}
		//main part of the function
		while( (nread = read(des1, buffer, BUFFER)) > 0){	//read the file while there are characters left
			nwrite = write(des2, buffer, nread);		//write the content of the buffer in the new file
			if(nwrite < nread){				//if not all of the characters have been written, something went wrong
				sprintf(res, "[ERROR] Error al escribir en el fichero destino");		
			}
		}
		if(close(des1) < 0){							      //if there is an error closing the first file
			if(close(des2) < 0){						      //  try closing the second one
				perror("[ERROR] Error al cerrar los ficheros"); return;     //if both had problems, return the corresponding error message
			} else{
				perror("[ERROR] Error al cerrar el fichero origen"); return;//if only the first one had problems, warn the user
			}
		}
		if(close(des2) < 0){							      //if the first file was closed, but the second one has problems
			perror("[ERROR] Error al cerrar el fichero destino"); return;       //warn the user
		}
		
		sprintf(res,"[OK] Copiado con exito el fichero <origen> a <destino>");     //if everything went okay, inform the user with a success message
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
    int p[2];			//integer-type array to create pipes
    int last_fid;		//saves the edge of the current pipe so the next process can chain to it
    int infile, outfile, errfile;  //file descriptors in case there are redirections
    int num_arg;		//number of arguments sent to mycalc and mycp
    setenv("Acc", "0", 1);	//environment variable to store the accumulated sums (1 indicates overwriting)
    
	
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
                } else if (command_counter == 1){				//if there is only one command it can be a call to mycalc or mycp, o a simple command
                   if(strcmp(argvv[0][0], "mycalc") == 0){			//if it is mycalc, check how many words are sent (e.g. mycalc 1 add 2 -> num_arg = 4), this
			num_arg = 0;						//  parameter allows the function to return an error message if the number is not the expected
			for(int i = 0; argvv[0][i] != NULL; i++){
				num_arg++;			
			}
                   	mycalc(argvv[0], res, num_arg);			//the result will be copied in global 'res' and the printed out
			printf("%s\n",res);

                   } else if(strcmp(argvv[0][0], "mycp")==0){		//if it is mycp, check how many words are sent (e.g. mycp file1 file2 -> num_arg = 3)
			num_arg = 0;
			for(int i = 0;argvv[0][i]!=NULL;i++){
				num_arg++;			
			}			
                   	mycp(argvv[0], res, num_arg);				//the result will be copied in global 'res' and the printed out
			printf("%s\n",res);

                   } else{									//at this point, it must be a simple command, so we create a child process
		 	  son_pid = fork();
		 	  switch(son_pid){
		   		case -1: // error
		   			perror("Error creating the new process");
		   			exit(-1);
				case 0:  // son
					//Redirections block
					if( strcmp(filev[0], "0") != 0){			//first of all, check possible redirections,
						close(STDIN_FILENO);				//  close the corresponding entries of the BCP table
						if( (infile = open(filev[0], O_RDONLY)) < 0){ //  and open the specified file
							perror("open Error:\t"); exit(-1);	//in case of an error, display the problem
						}
											
					} else if(strcmp(filev[1],"0")!=0){
						close(STDOUT_FILENO);
						if( (outfile = open(filev[1], O_CREAT|O_WRONLY, 0666)) < 0){
							perror("open Error:\t"); exit(-1);
							}
														
						}
			
					else if(strcmp(filev[2],"0")!=0){
						close(STDERR_FILENO);
						if( (errfile = open(filev[2], O_CREAT|O_WRONLY, 0666)) < 0){
							perror("open Error:\t"); exit(-1);
							}								
						}
					//End of redirections block
			   		execvp(argvv[0][0], argvv[0]);			//finally, execute the command
			   		perror("An error occured while executing the order"); //this lines are only reached if execvp fails
				   	return -1;
				
				default:  // Father's code
				   if(!in_background){							//we will wait only if in_backgroun is false (i.e. foreground)
					while(son_pid != wait(&status));
					if(status < 0){
						perror("Child process terminated with errors:\t");	//display a message to inform the user if the son has had problems
					}
				   printf("%d\n", son_pid);
				  }
				}
			}
		} else {								//using a for loop allows to execute any number of commands from 2 to MAX_COMMANDS
		   for(int i = 0; i < command_counter; i++){				//  by making the father process a 'monitor' in the task of chaining sons through pipes
			if (i < (command_counter-1)){					//if it isn't the last process, create a pipe (there must be 'command_counter - 1' pipes)
				if(pipe(p) < 0){
					perror("pipe Error:\t"); exit(-1);			
				}
			}
			son_pid = fork();						//both father and son processes will preserve a copy of the pipe created in the previous step
			switch(son_pid){
				case -1:	//error creating a child process
					perror("FORK ERROR:\t"); exit(-1);	
				case 0:	//SON'S CODE
					//Redirections block
					if( i == 0 && strcmp(filev[0], "0") != 0){		//first of all, check possible redirections,
						close(STDIN_FILENO);				//  close the corresponding entries of the BCP table
						if( (infile = open(filev[0], O_RDONLY)) < 0){ //  and open the specified file
							perror("open Error:\t"); exit(-1);	//in case of an error, display the problem
						}					
					 } else if(strcmp(filev[1],"0")!=0){
						close(STDOUT_FILENO);
						if( (outfile = open(filev[1], O_CREAT|O_WRONLY, 0666)) < 0){
							perror("open Error:\t"); exit(-1);
							}
														
					} else if(strcmp(filev[2],"0")!=0){
						close(STDERR_FILENO);
						if( (errfile = open(filev[2], O_CREAT|O_WRONLY, 0666)) < 0){
							perror("open Error:\t"); exit(-1);
							}								
						}
					//End of redirections block
					//Pipes block
					if(i != 0){					//every child (except the first one) must close standard input (the keyboard)
						close(STDIN_FILENO); dup(last_fid);	//  and duplicate the output descriptor of the previous pipe
						close(last_fid);			//once created, we can close the duplicate descriptor
					}
					if(i != (command_counter - 1)){		//every child (except the last one) must close standard output (the screen)
						close(STDOUT_FILENO); dup(p[1]);	//   and duplicate the input descriptor of the current pipe
						close(p[1]); close(p[0]);		//once created, we can close the duplicate descriptors of the pipe
					}
					//End of pipes block

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
		    if(!in_background){						//we will wait only if in_backgroun is false (i.e. foreground)
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

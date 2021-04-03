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


// ficheros por si hay redirección
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
    int p[2];


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
		int status = 0;
	        int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

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
                      printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
                }
                /********************************************************************/
                else if (command_counter == 1){
            	   // Print command
		   print_command(argvv, filev, in_background);
		   pid_t son_id = fork();
		   // Son's code
		   switch(son_id){
		   	case -1: // error
		   		perror("Error creating the new process");
		   		exit(-1);
			case 0:  // son
			   	printf("hello, %s\n", **argvv);

			   	execvp(argvv[0][0],argvv[0]);
			   	perror("An error occured while executing the order");
			   	return -1;
			   	
			   	break;
			
			   // Father's code
			default:
			   	if (in_background == 0) {
			   		while(wait(&status) != son_id);
			   		
			   		
			   	}
			}
		}
		else {
		   for(int i = 0; i < command_counter; i++){
		   	printf("%d\n", command_counter);
			int ret;
			int p10;	
			pid_t pid;
			if (i < (command_counter-1)){
				ret = pipe(p);
				if(ret<0){
					perror("pipe Error:\t");
					exit(-1);			
				}
			}
			pid = fork();
			switch(pid){
				case -1://error creation
					perror("FORK ERROR:\t");
					exit(-1);	
				case 0:
					if(i!=0){//entrada
						close(0); dup(p10);
						close(p10);				
					}
					if(i!=(command_counter-1)){ //salida
						close(1);dup(p[1]);
						close(p[1]);close(p[0]);			
					}

					execvp(argvv[i][0],argvv[i]);
			   		perror("An error occured while executing the order");
			   		return -1;
			   			
				default:
					if(i!=(command_counter-1)){//no es salida
						p10=p[0]; close(p[1]);								
					}
					else{//final
						close(p[0]);		
					}
	
			}
			while(pid!=wait(&status));
		   }
                }
                /******************************************/
              }
        }
	
	return 0;
}

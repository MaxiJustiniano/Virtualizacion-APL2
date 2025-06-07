#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

/*############# INTEGRANTES ###############
###     Justiniano, Máximo              ###
###     Mallia, Leandro                 ###
###     Maudet, Alejandro               ###
###     Naspleda, Julián                ###
###     Rodriguez, Pablo                ###
#########################################*/

void ayuda(){
	printf("Uso del programa:\n\n");
	printf("Opcion: -help o --help para mostrar la ayuda");
	printf("\n\nEl programa crea una estructura jerárquica de procesos");
	printf("\nEl proceso Padre crea Hijo 1 e Hijo 2");
	printf("\nEl Hijo 1 crea Nieto 1, Zombie y Nieto 3");
	printf("\nEl hijo 2 crea Demonio\n");
}

int main(int argc, char *argv[]){
	int pidH1, pidH2, pidD, pidN1, pidN3, pidZ,i;
	char c;

	for(i = 1; i < argc; i++){
		if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			ayuda();
			return 0;
		}
		else{
			printf("Error: Opción no valida");
			printf("\nUse -help o --help para mas información\n");
			return 1;
		}
	}

	pidH1 = fork();

	if(pidH1 == 0){
		pidN1 = fork();

		if(pidN1 == 0){
			printf("Soy el proceso Nieto 1 con PID %d, mi padre es %d\n",getpid(),getppid());
			c = getchar();
			printf("Nieto 1 finalizado\n");
			exit(0);
		}
		else{
			pidZ = fork();

			if(pidZ == 0){
				printf("Soy el proceso Zombie con PID %d, mi padre es %d\n",getpid(),getppid());
				printf("Zombie finalizado\n");
				exit(0);
			}
			else{
				pidN3 = fork();
				if(pidN3 == 0){
					printf("Soy el proceso Nieto 3 con PID %d, mi padre es %d\n",getpid(),getppid());
					c = getchar();
					printf("Nieto 3 finalizado\n");
					exit(0);
				}
				else{
					printf("Soy el proceso Hijo 1 con PID %d, mi padre es %d\n",getpid(),getppid());
					waitpid(pidN1, NULL, 0);
					waitpid(pidN3, NULL, 0);
					c = getchar(); 
					printf("Hijo 1 finalizado\n");
					exit(0);
				}
			}
		}
		
	}
	else{
		pidH2 = fork();
		
		if(pidH2 == 0){
			pidD = fork();

			if(pidD == 0){
				printf("Soy el proceso Demonio con PID %d, mi padre es %d\n",getpid(),getppid());
				c = getchar();
				printf("Demonio finalizado\n");
				exit(0);
			}
			else{
				printf("Soy el proceso Hijo 2 con PID %d, mi padre es %d\n",getpid(),getppid());
				printf("Hijo 2 finalizado\n");
				exit(0); 
			}
		}
		else{
			printf("Soy el proceso Padre con PID %d\n", getpid());
			wait(NULL);
			wait(NULL);
			printf("Padre finalizado\n");
		}
	}

	return 0;
}

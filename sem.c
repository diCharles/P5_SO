

/**

When SIGSTOP is sent to a process, the usual behaviour is to pause that process in its current state. 
The process will only resume execution if it is sent the SIGCONT signal. 
SIGSTOP and SIGCONT are used for job control in the Unix shell, among other purposes. 
SIGSTOP cannot be caught or ignored.

good documentation from https://major.io/2009/06/15/two-great-signals-sigstop-and-sigcont/


*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <signal.h>/** library for SIGNALS SIGSTOP SIGCONT*/

#define CICLOS 10
char *pais[3]={"Peru","Bolvia","Colombia"};

/** implementation of semaphore*/
typedef struct{
	int cntr ;
}semaphore_struct_t;

typedef semaphore_struct_t * semaphore_t ;

int initsem(semaphore_t  s, int sem_value);
int waitsem(semaphore_t  s);
int signalsem(semaphore_t  s);

semaphore_t g_sem;

void proceso(int i)
{
	int k;
	for(k=0;k<CICLOS;k++)
	{
		// Llamada waitsem implementada en la parte 3
		waitsem(g_sem);
		printf("Entra %s ",pais[i]);
		fflush(stdout);
		sleep(rand()%3);
		printf("- %s Sale\n",pais[i]);
// Llamada waitsignal implementada en la parte 3
		signalsem(g_sem);
// Espera aleatoria fuera de la sección crítica
		sleep(rand()%3);
	}
	exit(0);
	// Termina el proceso
}
int main()
{
	int pid;
	int status;
	int shmid;
	int args[3];
	int i;
	void *thread_result;
	// Solicitar memoria compartida para el semaforo
	semaphore_t dummy_sem ;
	shmid=shmget(0x1234,sizeof(semaphore_t),0666|IPC_CREAT);
	if(shmid==-1)
	{
		perror("Error en la memoria compartida\n");
		exit(1);
	}
	// Conectar la variable a la memoria compartida
	g_sem =shmat(shmid,NULL,0);
	if(g_sem ==NULL)
	{
		perror("Error en el shmat\n");
		exit(2);
	}
	/** inicializar semaforo*/
	initsem(g_sem, 0);

	srand(getpid());
	for(i=0;i<3;i++)
	{
	// Crea un nuevo proceso hijo que ejecuta la función proceso()
		pid=fork();
		if(pid==0)
			proceso(i);
	}
	for(i=0;i<3;i++)
		pid = wait(&status);
	// Eliminar la memoria compartida
	shmdt(g_sem);
}

int initsem(semaphore_t s, int sem_value)
{

}
int waitsem(semaphore_t s)
{
	
	if( 0== s->cntr)
	/** check available space*/	
	{
		s->cntr--;
		//available space, you shall pass
	}
	else
	/** block task, avoiding busy waiting*/
	{

	}	
	return ( 0 ) ;
}
int signalsem(semaphore_t s)
{
	/** awake,or unblock process waiting*/

	return ( 0 ) ;
}
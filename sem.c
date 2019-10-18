
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#define CICLOS 10
char *pais[3]={"Peru","Bolvia","Colombia"};

/** implementation of semaphore*/
typedef struct{
	int cnt ;
}semaphore_t ; 

int waitsem(semaphore_t * shared_sem_ptr);
int signalsem(semaphore_t * shared_sem_ptr);

semaphore_t * g_sem;

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
	g_sem->cnt = 0 ;

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

int waitsem(semaphore_t * shared_sem_ptr)
{
	/**if*/

	/** block task, avoid busy waiting*/
	return ( 0 ) ;
}
int signalsem(semaphore_t * shared_sem_ptr)
{
	/** awake,or unblock process waiting*/
	return ( 0 ) ;
}
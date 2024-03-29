

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

#define atomic_xchg(A,B) __asm__ __volatile__(\
"lock xchg %1,%0 ;\n"\
: "=ir" (A)			 \
: "m" (B), "ir" (A) \
);

#define CICLOS 10
#define NPROCS 3
char *pais[3]={"Peru","Bolivia","Colombia"};

typedef enum{FALSE= 0, TRUE = 1}bool_t ;
/** implementation of semaphore*/
typedef struct
{
	int cntr ;/** semaphore counter*/
	unsigned int bloqueados ; 

	pid_t blocked_queue[NPROCS];/** will store the pid of blocked proccess*/
	bool_t f_atomic_wait ;
	bool_t f_atomic_init ;
	bool_t f_atomic_signal;

}semaphore_struct_t;

typedef semaphore_struct_t * semaphore_t ;
/** initsem must be atomic*/
int initsem(semaphore_t  s, int sem_value);
/** waitsem must be atomic*/
int waitsem(semaphore_t  s);
/** signalsem must be atomic*/
int signalsem(semaphore_t  s);
/** for enqueue blocked processses*/
void enqueue(semaphore_t s , pid_t process_pid);
/** for dequeue blocked processes*/
pid_t dequeue(semaphore_t s );


semaphore_t g_sem;

void proceso(int i)
{
	int k;
	for(k=0;k<CICLOS;k++)
	{
		// Llamada waitsem implementada en la parte 3
		waitsem(g_sem);
		//printf("Entra %s ",pais[i]);
		printf("Entra %s en ciclo %d   ",pais[i],k);
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
	semaphore_struct_t dummy_sem ;
	shmid=shmget(0x1236,sizeof(dummy_sem),0777|IPC_CREAT);
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
	initsem(g_sem, 1);

	srand(getpid());
	for(i=0; i< NPROCS ; i++ )
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
	
	s->cntr = sem_value;
	int iterator; 
	for (iterator = 0 ; iterator < NPROCS ; iterator ++)
		/** ensuring no child process running */
		s->blocked_queue[iterator] = 0 ;

	/** initial value for atomic flags of wait, init and signal sem*/
	s->f_atomic_wait = 0 ; 
	s->f_atomic_init = 0 ;
	s->f_atomic_signal = 0 ;

	s->bloqueados = 0 ;
}
int waitsem(semaphore_t s)
{
	
	bool_t my_turn = 1 ;
	/** 
	consider that the three processes can enter
	here at the same time
	*/
	int once = 1 ;
	do{
		atomic_xchg(my_turn,s->f_atomic_wait);
			/** super small busy wait in this region*/
			if(once==1)
				//printf("%d entra atomica de wait \n", getpid());
		
			once= 0; 
	}while ( my_turn != 0);
	
		/** the first process here will enter the critical section */
		/** consider to ask again if there is  free space again in cs*/
	s->cntr--;
	//printf("%d sale de atomica de wait , deja cntr en %d\n", getpid(),s->cntr);
	if( 0 > s->cntr   )
	{	
		// poner este proceso en la cola de bloqueados
		pid_t process_pid = getpid();      
		
		enqueue(s,process_pid);
		//bloquear este proceso
		s->bloqueados++;
		//printf("%d sleep , thus cntr =%d , bloqueados %d  [0]%d [1]%d  [2]%d \n", 
		//process_pid, s->cntr,s->bloqueados,s->blocked_queue[0],s->blocked_queue[1],s->blocked_queue[2]);
		
		if(s->bloqueados >= NPROCS)
		{
			pid_t new;
			new = dequeue(s );
			s->bloqueados--;
			kill(new, SIGCONT);
		}	
		s->f_atomic_wait = 0 ;
		kill( process_pid, SIGSTOP );
	}
	s->f_atomic_wait = 0 ;
	return ( 0 ) ;
}
int signalsem(semaphore_t s)
{
	bool_t my_turn = 1 ;
	/** awake,or unblock process waiting*/
	int once = 1 ;
	do{
		atomic_xchg(my_turn,s->f_atomic_wait);
		/** super small busy wait in this region*/
		if(once==1)
		{
			//printf("%d entra atomica de signal \n", getpid());
		}
		once = 0 ;
	}while ( my_turn != 0);
	//printf("%d sale atomica de signal \n", getpid());

	s->cntr++;
	
		// quitar un proceso de la cola de bloqueados
		pid_t new;
		new = dequeue(s );
		//activar proceso si lo hay
		if(new != 0)
		{
			s->bloqueados--;
			//printf(" signal to continue %d\n",new);
			kill(new, SIGCONT);
		}
		else
		{
			//printf("nobody in queue for signal [0]%i  [1]%i [2]%i \n", s->blocked_queue[0],s->blocked_queue[1],s->blocked_queue[2]);
		}
	
	
	
	s->f_atomic_wait = 0 ;
	return ( 0 ) ;
}
void enqueue(semaphore_t s , pid_t process_pid)
{
	int iterator;
	//printf("", s->blocked_queue[iterator]);
	for(iterator = 0; iterator < NPROCS ; iterator ++)
	{
		if (0 == s->blocked_queue[iterator])
		{
			/** stop here, a empty place is found*/
			s->blocked_queue[iterator]= process_pid ;
			//printf(" now in queue [0]%i  [1]%i [2]%i \n", s->blocked_queue[0],s->blocked_queue[1],s->blocked_queue[2]);
			return ;
		}
	}	
	//printf("error queue full\n");
}
pid_t dequeue(semaphore_t s )
{
	int iterator; 
	for (iterator = 0; iterator < NPROCS; iterator++)
	{
		if (0 != s->blocked_queue[iterator])
		{
			/** element found*/
			pid_t aux_dequeued = s->blocked_queue[iterator];
			/** now shift queue elements up*/
			/* TODO PARAMETRIC SHIFT**/
			s->blocked_queue[0] = s->blocked_queue[1];
			s->blocked_queue[1] = s->blocked_queue[2];
			s->blocked_queue[2] = 0 ;
			return aux_dequeued ;
		}	
	}	
	/** this return 0 NEED TO BE INTEPRETATED AS ERROR*/
	return 0 ;
}
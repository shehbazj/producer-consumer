/*
Creates 3 threads, 1 producer and 2 consumer on a 2 element array queue.
Thread 1 generates and places data on the array
Thread 2 and 3 read data from the array and delete the data
*/

#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/syscall.h>

#define QUEUE_SIZE 10 
#define NUM_PTHREADS 5 
#define NUM_CTHREADS 5 

typedef struct args{
	pthread_mutex_t *mutex;
	pthread_cond_t *cv;	
	int *count;
}args;

typedef struct con_args{
	pthread_mutex_t *mutex;
	pthread_cond_t *cv;
}con_args;

void copy_args(void *arg, struct args *p)
{
	p->mutex = ((args *)arg)->mutex;
	p->cv = ((args *)arg)->cv;
	p->count = ((args *)arg)->count;
	return;
}

void produce(){
	printf("%s\n", __func__);
}

void consume(){
	printf("%s\n", __func__);
}

static void * prod(void *arg)
{
	struct args *p;
	p = malloc(sizeof(struct args));
	copy_args(arg, p);

	/* try getting exclusive access to p.count variable */

	pthread_mutex_lock(p->mutex);

	/* no other thread can get here unless current thread unlocks mutex */
	while(*(p->count) < 1 ){	/*if count is 0, queue is overloaded with requests */
		pthread_cond_wait(p->cv, p->mutex);
				/* wait on cv and unlock mutex. Other threads may now access p.count*/
	}
	/* program reached here, that means atleast one of the queue elements is empty */
	/* TODO place data on queue here */

	//printf("%s():BEFORE decrementing count = %d tid = %ld\n",__func__, *(p->count), syscall(SYS_gettid));
	
	produce();
	*(p->count) = *(p->count)- 1; /* One more queue element filled */		
	//printf("%s():AFTER decrementing count = %d tid = %ld\n",__func__, *(p->count), syscall(SYS_gettid));

	pthread_mutex_unlock(p->mutex);
	free(p);
	return NULL;
}

static void * con(void *arg)
{
	struct args *c;
	c = malloc(sizeof(struct args));
	copy_args(arg, c);
	
	/* Try to get exclusive access to count */
	pthread_mutex_lock(c->mutex);

	/* wakeup other threads that may be waiting on the queue */
	pthread_cond_signal(c->cv);

	if(*(c->count) < QUEUE_SIZE){
		* (c->count)  = *(c->count) + 1;
	}

	/*TODO start processing data here*/
	//printf("%s(): count = %d tid = %ld\n",__func__, *(c->count), syscall(SYS_gettid));
	consume();
	pthread_mutex_unlock(c->mutex);
	free(c);
	return NULL;
}

int main()
{
	pthread_t thread_p[NUM_PTHREADS], thread_c[NUM_CTHREADS];
	pthread_mutex_t mutex;
	pthread_attr_t attr;
	pthread_cond_t cv;
	void *res;


	int i, count = QUEUE_SIZE;

	pthread_attr_init(&attr);
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cv, NULL);	
	
	// create producer args

	struct args *params;
	params = malloc(sizeof(args));
	params->mutex = &mutex;
	params->cv = &cv;
	params->count = malloc(sizeof(int));
	*(params->count) = QUEUE_SIZE;	
	
	// start the producer	
	for(i=0; i < NUM_PTHREADS ; i++)
	pthread_create(&thread_p[i], &attr, &prod,(void *)params);

	// start the consumer
	for(i=0; i < NUM_CTHREADS ; i++)
		pthread_create(&thread_c[i], &attr, &con, (void *)params);

	pthread_attr_destroy(&attr);
	for(i=0; i < NUM_PTHREADS ; i++)
		pthread_join(thread_p[i], &res);


	for(i=0; i < NUM_CTHREADS ; i++)
		pthread_join(thread_c[i], &res);

	free(params->count);
	free(params);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cv);

	return 0;
}

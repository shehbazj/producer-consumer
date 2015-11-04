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
#define CONCURRENT_OPS 4

typedef struct args{
	pthread_mutex_t *mutex_count;	// binary counter to ensure atomic operation on count variable
	pthread_cond_t *cv;		// Upto CONCURRENT_OPS 
	int *count;			// can operate on queue at the same time. 
}args;

void copy_args(void *arg, struct args *p)
{
	p->mutex_count = ((args *)arg)->mutex_count;
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

	pthread_mutex_lock(p->mutex_count);
	while(*(p->count) <= 0){
		pthread_cond_wait(p->cv, p->mutex_count);
	}

	*(p->count) = *(p->count)- 1; /* One more queue element filled */		
	pthread_mutex_unlock(p->mutex_count);

	produce();

	pthread_mutex_lock(p->mutex_count);
	if(*(p->count) < CONCURRENT_OPS){
		*(p->count) = *(p->count) + 1; /* One more queue element filled */		
	}
	pthread_mutex_unlock(p->mutex_count);
	free(p);
	return NULL;
}

static void * con(void *arg)
{
	struct args *c;
	c = malloc(sizeof(struct args));
	copy_args(arg, c);
	
	/* Try to get exclusive access to count */
	pthread_mutex_lock(c->mutex_count);
	if(*(c->count) < QUEUE_SIZE){
		* (c->count)  = *(c->count) + 1;
		consume();
	}
	pthread_mutex_unlock(c->mutex_count);
	/* wakeup other threads that may be waiting on the queue */
	pthread_cond_signal(c->cv);

	/*TODO start processing data here*/
	//printf("%s(): count = %d tid = %ld\n",__func__, *(c->count), syscall(SYS_gettid));
//	pthread_mutex_unlock(c->mutex);
	free(c);
	return NULL;
}

int main()
{
	pthread_t thread_p[NUM_PTHREADS], thread_c[NUM_CTHREADS];
	pthread_mutex_t mutex_count;
	pthread_attr_t attr;
	pthread_cond_t cv;
	void *res;


	int i, count = QUEUE_SIZE;

	pthread_attr_init(&attr);
	pthread_mutex_init(&mutex_count, NULL);
	pthread_cond_init(&cv, NULL);	
	
	// create producer args

	struct args *params;
	params = malloc(sizeof(args));
	params->mutex_count = &mutex_count;
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
	pthread_mutex_destroy(&mutex_count);
	pthread_cond_destroy(&cv);

	return 0;
}

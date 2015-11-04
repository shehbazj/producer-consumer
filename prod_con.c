/*
	The program runs for a single producer and multiple consumers.
	The consumers can sleep if there are no active requests queued by the producer.
	The producer can queue as many as QUEUE_SIZE elements on the queue, after which
	it blocks till NUM_CTHREADS consumers service requests in the queue. 
*/

#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<unistd.h>
#include<sys/syscall.h>
#include<stdbool.h>
#include<assert.h>

#define QUEUE_SIZE 10 
#define NUM_PTHREADS 1 
#define NUM_CTHREADS 5
#define CONCURRENT_OPS QUEUE_SIZE 


int req_no = 0;

typedef struct args{
	pthread_mutex_t *mutex_count;	// binary counter to ensure atomic operation on count variable
	int *queue;
	pthread_mutex_t *queue_lock;
	pthread_cond_t *cv_consumer;		// Upto CONCURRENT_OPS 
	pthread_cond_t *cv_producer;
	int *count;			// can operate on queue at the same time. 
}args;

void copy_args(void *arg, struct args *p)
{
	p->mutex_count = ((args *)arg)->mutex_count;
	p->cv_producer = ((args *)arg)->cv_producer;
	p->cv_consumer = ((args *)arg)->cv_consumer;
	p->queue  = ((args *)arg)->queue;
	p->queue_lock = ((args *)arg)->queue_lock;
	p->count = ((args *)arg)->count;
	return;
}

void produce(int count, int req_no, int *queue , pthread_mutex_t *queue_lock){
	int i;
	for(i = 0 ; i < QUEUE_SIZE ; i++){
		pthread_mutex_lock(&queue_lock[i]);
		if(queue[i] == -1){
			printf("%s sema_val = %d req_no = %d\n", __func__, count, req_no);
			queue[i] = req_no; 
			pthread_mutex_unlock(&queue_lock[i]);
			break;
		}
		pthread_mutex_unlock(&queue_lock[i]);
	}
	assert(i< QUEUE_SIZE);
}

void consume(int count, int *queue, pthread_mutex_t *queue_lock){
	int i;
	for(i = 0 ; i < QUEUE_SIZE ; i++){
		pthread_mutex_lock(&queue_lock[i]);
		if(queue[i] != -1){
			printf("%s %d %d\n", __func__, count, queue[i]);
			queue[i] = -1;
			pthread_mutex_unlock(&queue_lock[i]);
			return; 
		}
		pthread_mutex_unlock(&queue_lock[i]);
	}
	printf("consumer Count = %d\n", count);
	//assert(0);
}

static void * prod(void *arg)
{
	struct args *p;
	int i;
	int loop;

	for(loop = 0 ; loop < 50 ; loop++){
	p = malloc(sizeof(struct args));
	copy_args(arg, p);

	/* try getting exclusive access to p.count variable */

	pthread_mutex_lock(p->mutex_count);
	while(*(p->count) <= 0){
		pthread_cond_wait(p->cv_producer, p->mutex_count);
	}

	req_no++;
	*(p->count) = *(p->count)- 1; /* One more queue element filled */		
	produce(*(p->count), req_no, (p->queue) , (p->queue_lock));
	
	pthread_cond_signal(p->cv_consumer);
	pthread_mutex_unlock(p->mutex_count);

	free(p);
	}
	return NULL;
}

static void * con(void *arg)
{
	struct args *c;

	while(1){
		c = malloc(sizeof(struct args));
		copy_args(arg, c);
		
		/* Try to get exclusive access to count */
		pthread_mutex_lock(c->mutex_count);
		while(*(c->count) >= QUEUE_SIZE){	
			pthread_cond_wait(c->cv_consumer, c->mutex_count);
		}	
	
		pthread_mutex_unlock(c->mutex_count);
	
		consume(*(c->count), (c->queue) , (c->queue_lock));
	
		pthread_mutex_lock(c->mutex_count);
		*(c->count)  = *(c->count) + 1;
		pthread_cond_signal(c->cv_producer);
		pthread_mutex_unlock(c->mutex_count);
		/* wakeup other threads that may be waiting on the queue */
		free(c);
	}
	return NULL;
}

int main()
{
	pthread_t thread_p[NUM_PTHREADS], thread_c[NUM_CTHREADS];
	pthread_mutex_t mutex_count;
	int queue[QUEUE_SIZE];
	pthread_mutex_t queue_lock[QUEUE_SIZE];
	pthread_attr_t attr;
	pthread_cond_t cv_producer;
	pthread_cond_t cv_consumer;
	void *res;


	int i, count = QUEUE_SIZE;

	pthread_attr_init(&attr);
	pthread_mutex_init(&mutex_count, NULL);
	pthread_cond_init(&cv_producer, NULL);	
	pthread_cond_init(&cv_consumer, NULL);	

	// initialize queue to -1
	
	for(i=0; i < QUEUE_SIZE ; i++){
		queue[i] = -1;
		pthread_mutex_init(&queue_lock[i], NULL);
	}
	
	// create producer args

	struct args *params;
	params = malloc(sizeof(args));
	params -> queue =  queue;
	params -> queue_lock = queue_lock;
	params->mutex_count = &mutex_count;
	params->cv_producer = &cv_producer;
	params->cv_consumer = &cv_consumer;
	params->count = &count;
	
	// start the producer	
	for(i=0; i < NUM_PTHREADS ; i++)
	pthread_create(&thread_p[i], &attr, &prod,(void *)params);

	// start the consumer
	for(i=0; i < NUM_CTHREADS ; i++)
		pthread_create(&thread_c[i], &attr, &con, (void *)params);

	pthread_attr_destroy(&attr);
	for(i=0; i < NUM_PTHREADS ; i++)
		pthread_join(thread_p[i], &res);

	printf("Producer Thread Returned\n");

	for(i=0; i < NUM_CTHREADS ; i++)
		pthread_join(thread_c[i], &res);

	free(params);
	pthread_mutex_destroy(&mutex_count);
	pthread_cond_destroy(&cv_producer);
	pthread_cond_destroy(&cv_consumer);

	return 0;
}

/* Kaylan Mettus
 * CS 370
 * Project 4
 * 
 * This is an implementation of Peterson's distributed leader election 
 * algorithm. */
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <pthread.h>
 #include <semaphore.h>

typedef struct channel {
	sem_t race_sem;
	sem_t sync_sem;
	int *queue;
	int head;
	int tail;
	int qCnt;
} Channel;

typedef struct node {
	int uid;
	int tempid;
	int hop1;
	int hop2;
	int phase;
	int active;
	Channel *in, *out;
} Node;

//helper functions
void initNodes(Node *,Channel *, int);
void initChannels(Channel *, int);
void enqueue(Channel *, int);
int dequeue(Channel *);
int read(Channel *);
void write(Channel *, int);
void petersons(Node *);

int main () 
{
	int nodeCnt;
	scanf("%d", &nodeCnt);
	Node nodeArray[nodeCnt];
	Channel chanArray[nodeCnt];	
	
	initChannels(chanArray, nodeCnt);
	initNodes(nodeArray, chanArray, nodeCnt);
	
	pthread_t threads[nodeCnt];
	int i, j;
	
	for (i = 0; i < nodeCnt; i++)
		pthread_create(&threads[i], NULL, (void *)petersons, (void *)&nodeArray[i]);
	
	for (j = 0; j < nodeCnt; j++)
		pthread_join(threads[j], NULL);
	
	return 0;
}

//precondition: the number of nodes, count, has already been read from 
//standard input
//postcondition: the array of Nodes nArr has been initialized with
//node uids and phase 1
void initNodes(Node *nArr, Channel *cArr, int count)
{
	int i;
	
	for (i = 0; i < count; i++)
	{
		scanf("%d", &nArr[i].uid);
		nArr[i].tempid = nArr[i].uid;
		nArr[i].phase = 1;
		nArr[i].in = &cArr[i];
		nArr[i].out = &cArr[(i + 1) % count];
		nArr[i].active = 1;
	}
}

//precondition:the number of channels, count, has already been read
//from standard input
//postcondition: the semaphores in the channels have been initialized,
// 1 for the race condition semaphore and 0 for the synchronization
//semaphore.  The channel queues have been allocatted with a size of
//twice the number of channels and nodes.
void initChannels(Channel *cArr, int count)
{
	int i;
	int n = 2 * count;
	
	for (i = 0; i < count; i++)
	{
		sem_init(&cArr[i].race_sem, 0, 1);
		sem_init(&cArr[i].sync_sem, 0, 0);
		
		cArr[i].queue = (int *)calloc(n, sizeof(int));
		cArr[i].head = 0;
		cArr[i].tail = 0;
		cArr[i].qCnt = 0;
	}
}

//precondition: the channel has been initialized
//postcondition: val has been added to the front of the queue and the
//queue variables have been updated.
void enqueue(Channel *c, int val)
{
	c->queue[c->tail] = val;
	c->tail++;
	c->qCnt++;
}

//precondition: the channel has been initialized
//postcondition: the first item has been removed from the queue and
//the value has been returned.  Queue variables have been updated.
//If the queue was empty, 0 is returned.
int dequeue(Channel *c)
{
	int val = 0;
	
	if (c->qCnt != 0)
	{
		val = c->queue[c->head];
		c->head++;
		c->qCnt--;
	}
	
	return val;
}

//postcondition: A value has been removed from channel c's queue.  The
//channel will sleep if it is now empty.
int read(Channel *c)
{
	sem_wait(&(c->sync_sem));
	sem_wait(&(c->race_sem));
	int val = dequeue(c);
	sem_post(&(c->race_sem));
	
	return val;
}

//postcondition: Value val has been added to channel c's queue.  The 
//channel will wake up if it has been sleeping.
void write(Channel *c, int val)
{
	sem_wait(&(c->race_sem));
	enqueue(c, val);
	sem_post(&(c->race_sem));
	sem_post(&(c->sync_sem));
}

//precondition: node and channel arrays have been initialized
//postcondition: a leader has been selected from the nodes
void petersons(Node *temp)
{
	for (;;)
	{
		if (temp->active == 1)
		{
			printf("[%d] [%d] [%d]\n", temp->phase, temp->uid, temp->tempid);
			write(temp->out, temp->tempid);
			temp->hop1 = read(temp->in);
			write(temp->out, temp->hop1);
			temp->hop2 = read(temp->in);
			
			if (temp->hop1 == temp->tempid)
			{
				printf("leader: %d\n", temp->uid);
				write(temp->out, -1);
				pthread_exit(NULL);
			}
			else if (temp->hop1 > temp->hop2 && temp->hop1 > temp->tempid)
			{
				temp->phase++;
				temp->tempid = temp->hop1;
			}
			else
				temp->active = 0;
		}
		else
		{
			int n = read(temp->in);
			write(temp->out, n);
			if (n == -1)
				pthread_exit(NULL);
			n = read(temp->in);
			write(temp->out, n);
		}
	}
}

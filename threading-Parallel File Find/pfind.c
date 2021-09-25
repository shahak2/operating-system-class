#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ROOT "."
#define PARENT ".."
#define SUCCESS 0
#define FAILURE -1

// =============== Global Variables ===============

// Mutex
pthread_mutex_t qlock;
pthread_mutex_t counter_lock;
// Conditional Variables
pthread_cond_t start_searching;
pthread_cond_t all_threads_locked;
// Auxiliary variables
int threads_counter = 0;
int sleeping_threads_counter = 0;
int files_counter = 0;
int threads_encountered_error_counter = 0;
char *term;

// == Queue implementation (inspired by "Geeks for Geeks" website) ==
typedef struct Qnode
{
	char path[PATH_MAX];
	struct Qnode *next;
} qnode;

typedef struct queue
{
	qnode *first, *last;
} queue;

queue *dirs_queue; // ---- The global directories queue ----

qnode* newNode(char *path)
{
	qnode* new_node = (qnode*)malloc(sizeof(qnode));
	strcpy(new_node -> path, path);
	new_node -> next = NULL;
	return new_node;
}

queue* newQueue()
{
	queue* q = (queue*)malloc(sizeof(queue));
	q->first = q ->last = NULL;
	return q;
}

int enqueue(queue* q, char *path)
/* 
 Input: a queue and a path.
 Functionallity: adds the path to the queue. Returns 
 FAILURE if allocation failed or SUCCESS otherwise.
 */
{
	qnode* new_node = newNode(path);
	
	if( new_node == NULL)
		return FAILURE;
	
	pthread_mutex_lock(&qlock);
	
	if(q->last == NULL) // queue is empty.
	{
		q->first = q->last = new_node;
	}
	else
	{
		q->last->next = new_node;
		q->last = new_node;
	}
	pthread_mutex_unlock(&qlock);
	
	return SUCCESS;
}

qnode* dequeue(queue* q)
/* 
 Input: a queue
 Functionallity: removes the first dir in the queue. returns it, or null if the queue is empty.
 */
{
	if(q->first != NULL) // Queue isn't empty.
	{
		qnode* temp = q->first;
		q->first = q->first->next;
		if(q->first == NULL) // Last node was removed.
			q->last = NULL;
		return temp;
	}
	return NULL;
}

int not_empty(queue q)
//  return values: SUCCESS = queue isn't empty, FAILURE =queue is empty.
{
	if(q.first != NULL)
		return SUCCESS;
	
	return FAILURE;
}

int search_dir(char *path)
// Input: a file path.
/* Functionallity: According to the file given:
	- directory with permissions - adds to queue.
	- directory without permissions - prints path.
	- otherwise - if the file includes the term wanted, prints the path.
*/
{
	char new_path[PATH_MAX];
	DIR *folder;
	struct dirent *entry;
	struct stat stats;
	
	folder = opendir(path);
	if(folder == NULL)
	{
		return FAILURE;
	}
	
	while( (entry = readdir(folder)) )
	{
		stpcpy(new_path, path);
		strcat(strcat(new_path, "/"), entry->d_name);
		
		if( lstat(new_path, &stats) == FAILURE )
		{
			closedir(folder);
			return FAILURE;
		}
		
		if(S_ISDIR(stats.st_mode))
		{
			if(strcmp(entry->d_name, PARENT) == 0 || strcmp(entry->d_name, ROOT) == 0)
				continue;
			
			if(access(new_path, R_OK | X_OK) == SUCCESS)
			{
				if(enqueue(dirs_queue, new_path) == FAILURE)
				{
					closedir(folder);
					return FAILURE;
				}
				pthread_cond_signal(&start_searching); // Waking up a thread after adding a dir to the queue.
			}
			else
				printf("Directory %s: Permission denied. \n", new_path);
		}
		else // Not a directory.
		{
			if(strstr(entry->d_name, term) != NULL)
			{
				pthread_mutex_lock(&counter_lock);
				files_counter++;
				pthread_mutex_unlock(&counter_lock);
				printf("%s\n", new_path);
			}
		}
	}
	
	closedir(folder);
	return SUCCESS;
}

void wait_for_start_searching(long thread_num)
// Implements the mechanism for starting all threads at the same time.
{
	pthread_mutex_lock(&counter_lock);
	threads_counter++;
	if(thread_num == threads_counter)
		pthread_cond_signal(&all_threads_locked);
	pthread_cond_wait(&start_searching, &counter_lock);
    pthread_mutex_unlock(&counter_lock);
}

void *thread_search(void *thread_data) 
/*
	Input: the number of threads.
	Functionallity: manages the thread according to the queue.
*/
{
	long thread_num = (long)thread_data;
	qnode* dir;
	
	wait_for_start_searching(thread_num); // waiting for all threads to be created.

	while(1)
	{
		pthread_mutex_lock(&qlock);
		
		if( not_empty(*dirs_queue) == SUCCESS )
		{
			dir = dequeue(dirs_queue); //notice - it is locked for one thread.
			pthread_mutex_unlock(&qlock);
			if(search_dir(dir->path) == FAILURE)
			{
				pthread_mutex_lock(&qlock);
				fprintf(stderr, "An error occured in a searching thread!\n"); 
				threads_encountered_error_counter++;
				pthread_mutex_unlock(&qlock);
				pthread_exit(NULL);
			}
			free(dir);
		}
		else
		{
			if(thread_num == sleeping_threads_counter + 1)
			// If this is the last non-sleeping thread, and the queue is empty, we are done.
			{
				sleeping_threads_counter++;
				pthread_cond_broadcast(&start_searching);
				break;
			}
			else
			{
				sleeping_threads_counter++;
				pthread_cond_wait(&start_searching, &qlock);
				
				if(thread_num <= sleeping_threads_counter + 1)  // no working threads and queue empty.
					break;
				sleeping_threads_counter--;
				pthread_mutex_unlock(&qlock);
			}
		}
	}
	pthread_mutex_unlock(&qlock);
	pthread_exit(NULL);
}

void init()
// This function initializes the queue, locks and conditional variables.
{
	dirs_queue = newQueue();
	pthread_mutex_init(&qlock, NULL);
	pthread_mutex_init(&counter_lock, NULL);
	pthread_cond_init(&start_searching, NULL);
	pthread_cond_init(&all_threads_locked, NULL);
}

void cleanup(pthread_t* threads, char *root)
// This function ties up loose ends.
{
	free(threads);
	free(dirs_queue);
	pthread_mutex_destroy(&qlock);
	pthread_mutex_destroy(&counter_lock);
	pthread_cond_destroy(&start_searching);
	pthread_cond_destroy(&all_threads_locked);
}

// ============== main function==============

int main(int argc, char *argv[]) 
{
	long thread_num;
	char *root;
	struct stat stats;
	pthread_t* threads;

	if(argc != 4)
	{
		fprintf(stderr, "Invalid input arguments!\n");
		exit(EXIT_FAILURE);
	}
	
	init();
	root = argv[1];
	term = argv[2];
	thread_num = atoi(argv[3]);
	
	if( lstat(root, &stats) == FAILURE )
	{
		fprintf(stderr, "Invalid root folder\n");
		exit(EXIT_FAILURE);
	}
	if(!S_ISDIR(stats.st_mode))
	{
		fprintf(stderr, "Root folder is not a directory.\n");
		exit(EXIT_FAILURE);
	}
	if(access(root, R_OK | X_OK) == FAILURE)
	{
		fprintf(stderr, "Root has invalid permissions.\n");
		exit(EXIT_FAILURE);
	}	
	
	enqueue(dirs_queue, root);
	threads = (pthread_t*)malloc(sizeof(pthread_t)*thread_num );
	
	if(threads == NULL)
	{
		fprintf(stderr, "Threads memory allocation failed.\n");
		exit(EXIT_FAILURE);
	}	
	
	pthread_mutex_lock(&counter_lock);
	
	for (int i = 0; i < thread_num; i++) 
	{
        int rc = pthread_create(&threads[i], NULL, thread_search, (void *) thread_num);
        if (rc) 
		{
            fprintf(stderr, "Failed creating thread: %s\n", strerror(rc));
            exit(EXIT_FAILURE);
        }
    }
	
	pthread_cond_wait(&all_threads_locked, &counter_lock);
	pthread_cond_broadcast(&start_searching);
	pthread_mutex_unlock(&counter_lock);
	
	for (int i = 0; i < thread_num; i++) 
	{
        pthread_join(threads[i], NULL);
    }
	
	if(threads_encountered_error_counter == thread_num)
	{
		fprintf(stderr, "All threads encountered an error.\n");
        exit(EXIT_FAILURE);
	}
	
	
	printf("Done searching, found %d files\n", files_counter);
	cleanup(threads, root);
    exit(EXIT_SUCCESS);
}

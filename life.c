#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <wait.h>
#include <time.h>
#include "life_util.h"

/* Program return values */
enum {
	LIFE_SUCCESS      = 0,
	LIFE_BAD_PARAMS   = (1 << 0),
	LIFE_ALLOC_FAILED = (1 << 1),
	LIFE_READ_FAILED  = (1 << 2),
	LIFE_WRITE_FAILED = (1 << 3)
};

/* Struct for passing command line parameters */
typedef struct
{
	size_t size;          /* Grid size */
	size_t thread_count;  /* Number of threads  */
	size_t max_gen;       /* Maximum simulation generation  */
	char  *file_in;       /* Input file path  */
	char  *file_out;      /* Output file path  */
} parameters_t;

/* Aggregation of variables used to synchronize the worker threads */
typedef struct
{
	pthread_mutex_t mutex;        /* Mutex protecting generation changes  */
	pthread_mutex_t alloc_mutex;  /* Mutex protecting work allocation  */
	pthread_cond_t synch_cv;      /* CV used by main to wait for worker threads to finish a generation */
	pthread_cond_t worker_cv;     /* CV used by worker threads to wait for a generation change  */
	parameters_t params;          /* Command line parameters */
	grid_t *src;                  /* Soucre grid for the current generation  */
	grid_t *dst;                  /* Destination grid for the current generation */
	size_t work_index;            /* Used to allocate work */
	size_t work_mod;              /* Used to allocation work remainder */
	size_t current_gen;           /* Current generation */
	size_t gen_done;              /* Incremented by worker threads when they finish a generation  */
} synch_t;

/* A block of work */
typedef struct
{
	size_t first_index; /* Index of first cell in the work block  (inclusive)  */
	size_t last_index;  /* Index of last cell in the work block  (inclusive)  */
} work_t;

/* Thread safe work allocation function. */
work_t allocate_work(synch_t *synch);

/* Parses command line parameters into params. Returns -1 if an error occurs.  */
int init_parameters(parameters_t *params, int argc, char **argv);

/* Creates worker threads and points workers at an array of their TIDS.  */
void spawn_workers(synch_t *synch, pthread_t **workers);

/* Joins to the threads in the workers TID array. Then poitns workers to NULL.  */
void despawn_workers(synch_t *synch, pthread_t **workers);

/* Executes a work block on the calling thread for the current generation.  */
void worker_eval(synch_t *synch, work_t *work);

/* Start function for worker threads which primarily handles generation syncrhonization.  */
void *worker_start(void *args);

int main(int argc, char **argv) {
	pthread_t *workers; /* Pointer to an array of worker TIDS  */
	grid_t grids[2];    /* Grid structs on the stack  */
	grid_t *swap;       /* Used to swap the src and dst buffers */
	synch_t synch;      /* Synchronization primitives */
	work_t work;        /* Main thread's work block */

	synch.current_gen = 1;
	synch.gen_done = 0;
	synch.work_index = 0;
	synch.src = &grids[0];
	synch.dst = &grids[1];

	/* Parse our command line parameters, and exit if it faiils  */
	if(init_parameters(&synch.params, argc, argv) == -1)
		return LIFE_BAD_PARAMS; 


	/* Initialize boards, and exit if either fails */	
	if(grid_init(synch.src, synch.params.size) == -1)
	{
		return LIFE_ALLOC_FAILED;
	}
	if(grid_init(synch.dst, synch.params.size) == -1)
	{
		grid_deinit(synch.src);
		return LIFE_ALLOC_FAILED;
	}

	/* Read the grid in, and exit if it fails  */
	if(grid_read(synch.params.file_in, synch.src) == -1)
	{
		grid_deinit(synch.src);
		grid_deinit(synch.dst);
		return LIFE_READ_FAILED;
	}
	
	pthread_cond_init(&synch.synch_cv, NULL);
	pthread_cond_init(&synch.worker_cv, NULL);
	pthread_mutex_init(&synch.mutex, NULL);
	pthread_mutex_init(&synch.alloc_mutex, NULL);

	synch.work_mod = (synch.params.size * synch.params.size) % synch.params.thread_count;
	spawn_workers(&synch, &workers);
	work = allocate_work(&synch);

	/* Main loop executes work and synchronizes worker threads */
	while(synch.current_gen != synch.params.max_gen + 1)
	{
		worker_eval(&synch, &work);

		/* Synchronize the worker threads for a generation change  */
		if(synch.params.thread_count > 1)
		{
			pthread_mutex_lock(&synch.mutex);
			while(synch.gen_done != (synch.params.thread_count - 1))
				pthread_cond_wait(&synch.synch_cv, &synch.mutex);	
		}		
	
		/* Update synch for next generation */		
		swap = synch.src;
		synch.src = synch.dst;
		synch.dst = swap;
		synch.current_gen++;
		synch.gen_done = 0;
		
		/* Resume worker threads */
		if(synch.params.thread_count > 1)
		{	
			pthread_cond_broadcast(&synch.worker_cv);
			pthread_mutex_unlock(&synch.mutex);
		}
	}	

	/* Kill off worker threads and write the result */
	despawn_workers(&synch, &workers);
	if(grid_write(synch.params.file_out, synch.src) == -1)
	{
		grid_deinit(synch.src);
		grid_deinit(synch.dst);
		return LIFE_WRITE_FAILED;
	}

	grid_deinit(synch.src);
	grid_deinit(synch.dst);
	return LIFE_SUCCESS;
}

int init_parameters(parameters_t *params, int argc, char **argv)
{
	size_t i;

	params->size = 0;
	params->thread_count = 0;
	params->max_gen = 0;
	params->file_in = NULL;
	params->file_out = NULL;

	for(i = 1; i < argc; i += 2)
	{	
		/* Check if we have a matching argument  */
		if(i + 1 == argc)
		{
			fprintf(stderr, "Missing argument for: %s\n", argv[i]);
			return -1;
		}	
	
		/* Size of the grid */
		if(strcmp("-n", argv[i]) == 0)
		{
			params->size = atoi(argv[i + 1]);
		}

		/* Number of threads */
		else if(strcmp("-t", argv[i]) == 0)
		{
			params->thread_count = atoi(argv[i + 1]);
		}

		/* Number of generations */
		else if(strcmp("-r", argv[i]) == 0)
		{
			params->max_gen = atoi(argv[i + 1]);
		}

		/* Input file */
		else if(strcmp("-i", argv[i]) == 0)
		{
			params->file_in = argv[i + 1];
		}	

		/* Output file */
		else if(strcmp("-o", argv[i]) == 0)
		{
			params->file_out = argv[i + 1];
		}

		/* Unrecognized option */
		else
		{
			fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
			return -1;
		}
	}

	if(params->size < 1)
	{
		fprintf(stderr, "Grid size must be at least 1\n");
		return -1;
	}

	if(params->thread_count < 1)
	{
		fprintf(stderr, "Number of threads must be at least 1\n");
		return -1;
	}

	if(params->max_gen < 1)
	{
		fprintf(stderr, "Number of generations must be at least 1\n");
		return -1;
	}

	return 0;
}

void spawn_workers(synch_t *synch, pthread_t **workers)
{
	size_t i;
	
	if(synch->params.thread_count > 1)
	{
		/* # Threads = Main-thread + workers = 1 + workers */
		*workers = (pthread_t*) 
			malloc(sizeof(pthread_t) * (synch->params.thread_count - 1));
	
		/* Spawn off worker the threads */	
		for(i = 0; i < (synch->params.thread_count - 1); i++)
		{
			pthread_create(
				&((*workers)[i]),
				NULL,
				worker_start,
				(void*) synch);
		}
	}
	else
	{
		*workers = NULL;
	}
}

void despawn_workers(synch_t *synch, pthread_t **workers)
{
	size_t i;	

	if(synch->params.thread_count > 1)
	{
		for(i = 0; i < (synch->params.thread_count - 1); i++)
			pthread_join((*workers)[i], NULL);
	
		free(*workers);
		*workers = NULL;
	}
}

work_t allocate_work(synch_t *synch)
{
	work_t work;

	pthread_mutex_lock(&synch->alloc_mutex);

	/* Assign the base work */
	work.first_index = work.last_index = synch->work_index;
	work.last_index += (synch->params.size * synch->params.size) / synch->params.thread_count - 1;
	
	/* assign a unit of the remaining work (if there is any left) */
	if(synch->work_mod > 0)
	{
		work.last_index++;
		synch->work_mod--;
	}

	/* The work index is inclusive; set it up for the next thread */
	synch->work_index = work.last_index + 1;

	pthread_mutex_unlock(&synch->alloc_mutex);

	return work;
}

void worker_eval(synch_t *synch, work_t *work)
{
	size_t i;
	size_t x;
	size_t y;

	/* Exploit the data locality */
	for(i = work->first_index; i <= work->last_index; i++)
	{
		x = i % synch->params.size;
		y = i / synch->params.size;
		grid_eval(synch->dst, synch->src, x, y);
	}
}

void *worker_start(void *args)
{
	size_t last_gen = 0;
	synch_t *synch = (synch_t*) args;
	work_t work = allocate_work(synch);

	while(last_gen != synch->params.max_gen)
	{
		
		/* Wait for buffer swap */
		pthread_mutex_lock(&synch->mutex);
		while(last_gen == synch->current_gen)
			pthread_cond_wait(
				&synch->worker_cv,
				&synch->mutex);
		pthread_mutex_unlock(&synch->mutex);

		worker_eval(args, &work);
		
		/* Let the main thread know we're done with the gen */
		last_gen++;
		pthread_mutex_lock(&synch->mutex);
		synch->gen_done++;
		pthread_cond_signal(&synch->synch_cv);
		pthread_mutex_unlock(&synch->mutex);
	}

	return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "pthread.h"

#define WORKER_USE_COND 0

typedef int (*worker_exec_t)(void *);

/* Work item */
struct worker_info;
struct work_item {
	worker_exec_t func;
	void *arg;
};

/* Single worker */
struct worker_info {
        int slot;				/* My slot # */
	unsigned short flags;
        pthread_mutex_t mutex;			/* Mutex */
#if WORKER_USE_COND
	pthread_cond_t cond;			/* Conditional */
#endif
	struct work_item item;			/* Work item */
        pthread_t tid;				/* Thread ID */
};

#define FLAG_BUSY 1				/* Worker is busy */
#define FLAG_DONE 2				/* Worker can quit */

/* Worker pool */
struct worker_pool {
	int count;				/* Number of workers */
	struct worker_info *workers;		/* Worker info */
};

/* Create pool of workers */
extern int worker_init(struct worker_pool *pool, int threads, int timeout);

/* Get an idle worker */
//extern int worker_get_idle(struct worker_pool *, int *);

/* Crack the whip */
extern int worker_exec(struct worker_pool *, worker_exec_t, void *);

/* Wait for all workers to finish */
extern int worker_wait(struct worker_pool *);

/* Destroy pool of workers */
extern int worker_destroy(struct worker_pool *);

/* Arguments */
extern char *args[32];
extern int nargs;

#ifdef DEBUG
#define dprintf(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#else
#define dprintf(format, args...) /* noop */
#endif /* !DEBUG */

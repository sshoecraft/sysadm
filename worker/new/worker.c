
#include "worker.h"
#include <pthread.h>

/* The actual worker function */
static void *worker(void *arg) {
	struct worker_info *wp = arg;
	int stat;
#if !WORKER_USE_COND
	int busy, done;
#endif

	while(1) {
		/* Lock this slot */
		pthread_mutex_lock(&wp->mutex);

#if WORKER_USE_COND
		/* Wait for busy to be set */
		while((wp->flags & FLAG_BUSY) == 0) pthread_cond_wait(&wp->cond,&wp->mutex);

#else

		/* Get flags */
		busy = ((wp->flags & FLAG_BUSY) != 0);
		done = ((wp->flags & FLAG_DONE) != 0);
//		dprintf("worker[%d]: busy: %d, done: %d\n", wp->slot, busy, done);

		/* If not busy, check if done */
		if (!busy) {
			/* If done, unlock slot & exit */
			if (done) {
				pthread_mutex_unlock(&wp->mutex);
				break;
			}
			goto skip;
		}
#endif

		/* Exec the work item */
		dprintf("worker[%d]: arg: %s\n", wp->slot,(char *) wp->item.arg);
		stat = wp->item.func(wp->item.arg);
//		dprintf("worker[%d]: status: %d\n", wp->slot, stat);

		/* Un-set busy flag */
		wp->flags &= ~FLAG_BUSY;

#if !WORKER_USE_COND
skip:
#endif
		/* Unlock slot */
		pthread_mutex_unlock(&wp->mutex);

		/* Sleep */
//	usleep(100);
	}
	dprintf("worker[%d]: exiting...\n", wp->slot);
	pthread_exit(0);
//	return (void *)0;
}

/* Create worker pool */
int worker_init(struct worker_pool *pool, int count, int timeout) {
	pool->count = count;
	struct worker_info *wp;
	pthread_attr_t attr;
	int x;

	dprintf("count: %d, timeout: %d\n", count, timeout);

	pool->workers = malloc(count * sizeof(struct worker_info));
	if (!pool->workers) {
		perror("malloc workers");
		return 1;
	}

	/* Create joinable attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	/* Init worker info */
	dprintf("creating workers...\n");
	for(x=0; x < count; x++) {
		wp = (struct worker_info *) &pool->workers[x];
		memset(wp,0,sizeof(*wp));
		wp->slot = x;
		pthread_mutex_init(&wp->mutex, 0);
#if WORKER_USE_COND
		pthread_cond_init(&wp->cond, 0);
#endif
		if (pthread_create(&wp->tid, &attr, worker, wp)) {
			perror("pthread_create");
			return 1;
		}
	}

	return 0;
}

/* Put a worker to work */
int worker_exec(struct worker_pool *pool, worker_exec_t func, void *arg) {
	struct worker_info *wp;
	int x,found;

	dprintf("func: %p, arg: %p\n", func, arg);

	found = 0;
	do {
		for(x=0; x < pool->count; x++) {
			wp = &pool->workers[x];

			/* Lock this slot */
			pthread_mutex_lock(&wp->mutex);

			/* If busy, unlock and continue on */
			if (wp->flags & FLAG_BUSY) {
				pthread_mutex_unlock(&wp->mutex);
				continue;
			}

			/* Set work item */
			wp->item.func = func;
			wp->item.arg = arg;

			/* Set busy flag */
			wp->flags |= FLAG_BUSY;

#if WORKER_USE_COND
			/* Set condition */
			pthread_cond_signal(&wp->cond);
#endif

			/* Unlock */
			pthread_mutex_unlock(&wp->mutex);

			/* Done */
			return 0;
		} 
		dprintf("sleeping...\n");
		usleep(100);
	} while(!found);

	/* Should never return here */
	return -1;
}

void worker_finish(struct worker_pool *pool) {
	register int x;
	struct worker_info *wp;
	int busy;

	/* Mark all as done */
	for(x=0; x < pool->count; x++) {
		wp = &pool->workers[x];
		pthread_mutex_lock(&wp->mutex);
		wp->flags |= FLAG_DONE;
		pthread_mutex_unlock(&wp->mutex);
	}

#if 0
	dprintf("*** WAITING ON FINISH ****\n");
	for(x=0; x < pool->count; x++) {
		wp = &pool->workers[x];
		pthread_join(wp->tid, NULL);
	}
#else
	/* Wait for any remaining threads */
	dprintf("*** WAITING ON FINISH ****\n");
	do {
		busy = 0;
		for(x=0; x < pool->count; x++) {
			wp = &pool->workers[x];
			pthread_mutex_lock(&wp->mutex);
			if (wp->flags & FLAG_BUSY) {
				dprintf("wp[%d]: %s\n", wp->slot, (char *)wp->item.arg);
				busy++;
			}
			pthread_mutex_unlock(&wp->mutex);
		}
		dprintf("busy: %d\n", busy);
		if (busy) {
			usleep(100);
			continue;
		}
	} while(busy);
#endif

	/* Kill the workers */
	dprintf("killing workers...\n");
	for(x=0; x < pool->count; x++) pthread_cancel(pool->workers[x].tid);
}

int worker_destroy(struct worker_pool *pool) {
	register int x;

	/* Wait for workers to finish */
	worker_finish(pool);

	/* Destroy mutexes */
	for(x=0; x < pool->count; x++) pthread_mutex_destroy(&pool->workers[x].mutex);

	/* Free memory */
	free(pool->workers);

	return 0;
}

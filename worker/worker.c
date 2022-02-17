
#ifndef __MINGW32__
#include "worker.h"
#include <pthread.h>

worker_t workers;

static void *worker(void *);

int create_workers(int count) {
	register int x;
	worker_t wp;

	workers = malloc(count * sizeof(struct worker_info));
	if (!workers) {
		perror("malloc workers");
		exit(1);
	}

	/* Init worker info */
	dprintf("creating workers...\n");
	for(x=0; x < count; x++) {
		wp = (worker_t) &workers[x];
		memset(wp,0,sizeof(*wp));
		wp->slot = x;
		pthread_mutex_init(&wp->busy_mutex, 0);
		if (pthread_create(&wp->tid, NULL, worker, wp)) {
			perror("pthread_create");
			return 1;
		}
	}

	return 0;
}

int get_busy(worker_t wp) {
	int busy;

	pthread_mutex_lock(&wp->busy_mutex);
	busy = ((wp->flags & FLAG_BUSY) != 0);
	pthread_mutex_unlock(&wp->busy_mutex);

//	dprintf("get_busy: worker[%d]: busy: %d, line: %s\n", wp->slot, busy, wp->line);

	return busy;
}

void set_busy(worker_t wp, int val) {
//	printf("set_busy: val: %d, flags before: %x\n", val, wp->flags);
	pthread_mutex_lock(&wp->busy_mutex);
	if (val)
		wp->flags |= FLAG_BUSY;
	else
		wp->flags &= ~FLAG_BUSY;
	pthread_mutex_unlock(&wp->busy_mutex);
//	printf("flags after: %x\n", wp->flags);

//	dprintf("set_busy: worker[%d]: busy: %d\n", wp->slot, val);
}

/* The actual worker function */
static void *worker(void *arg) {
	worker_t wp;
	int busy, done, stat;

	wp = (worker_t)arg;
	while(1) {
		/* Get info */
		pthread_mutex_lock(&wp->busy_mutex);
		busy = ((wp->flags & FLAG_BUSY) != 0);
		done = ((wp->flags & FLAG_DONE) != 0);
		pthread_mutex_unlock(&wp->busy_mutex);
		dprintf("worker[%d]: busy: %d, done: %d\n", wp->slot, busy, done);

		if (!busy) {
			if (done) break;
//			usleep(100);
			sleep(1);
			continue;
		}

		dprintf("worker[%d]: line: %s\n", wp->slot, wp->line);

		stat = exec_prog(wp);
//		if (stat != 0 && error_fp) fprintf(error_fp,"%s,%d\n", wp->line, stat);
		if (stat != 0 && error_file) {
			FILE *fp;
			fp = fopen(error_file,"a+");
			if (fp) {
				fprintf(fp,"%s\n", wp->line);
				fclose(fp);
			}
		}

		/* Clear busy flag */
		set_busy(wp,0);
	}
//	pthread_exit(0);
	dprintf("worker[%d]: returning...\n", wp->slot);
	return (void *)0;
}

/* Get an idle worker slot */
worker_t get_idle_worker(int count) {
	register int x;

	while(1) {
		for(x=0; x < count; x++) {
			if (!get_busy(&workers[x])) {
				dprintf("returning worker: %d\n", x);
				return &workers[x];
			}
		}
		usleep(100);
	}
}

void worker_finish(int count) {
	register int x;
	worker_t wp;
	int busy;

	/* Mark done */
	for(x=0; x < count; x++) {
		wp = &workers[x];
		pthread_mutex_lock(&wp->busy_mutex);
		wp->flags |= FLAG_DONE;
		pthread_mutex_unlock(&wp->busy_mutex);
	}

	dprintf("*** WAITING ON FINISH ****\n");
	/* Wait for any remaining threads */
	do {
		busy = 0;
		for(x=0; x < count; x++) {
			wp = &workers[x];
			pthread_mutex_lock(&wp->busy_mutex);
			if (wp->flags & FLAG_BUSY) {
				dprintf("wp[%d]: %s\n", wp->slot, wp->line);
				busy++;
			}
			pthread_mutex_unlock(&wp->busy_mutex);
		}
		dprintf("busy: %d\n", busy);
		if (busy) {
			sleep(1);
			continue;
		}
	} while(busy);

	/* Kill the workers - like all good pharoahs */
	dprintf("killing workers...\n");
	for(x=0; x < count; x++) pthread_cancel(workers[x].tid);
}
#endif

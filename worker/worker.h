
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct worker_info {
        int slot;
	int serial;
        unsigned short flags;
        pthread_mutex_t busy_mutex;
        pthread_t tid;
        char line[1024];
};
typedef struct worker_info * worker_t;

#define FLAG_BUSY 1
#define FLAG_DONE 2

extern int create_workers(int);
extern worker_t get_idle_worker(int);
extern void set_busy(worker_t, int);
extern void worker_finish(int);

extern void add_arg(char *);
extern int exec_prog(worker_t);

//extern FILE *error_fp;
extern char *error_file;

extern int debug;
#define dprintf(format, args...) if (debug) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)

#ifndef PSEUDO_THREADS_H
#define PSEUDO_THREADS_H

#include <stdint.h>
#include <sys/types.h>

// PT_writ();

extern void PT_wait(unsigned int  msecs);
ssize_t PT_read(int fd, void *buff, size_t len, unsigned int timeout);
ssize_t PT_write(int fd, const void *buff, size_t len, unsigned int timeout);

struct EventState;

typedef void (*PT_entry_point_t)(void*);
extern void *PT_create(PT_entry_point_t tmain, void *arg);
extern void PT_destroy(void *thread);

extern void PT_init(struct EventState *evt);
extern void PT_run_all(void);


#endif

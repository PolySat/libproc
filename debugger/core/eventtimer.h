#ifndef _DEBUG_EVENT_TIMER_H_
#define _DEBUG_EVENT_TIMER_H_

#include <polysat/polysat.h>
#include <polysat/eventTimer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct EventTimer *ET_debug_init(struct ProcessData *proc);

#ifdef __cplusplus
}
#endif

#endif

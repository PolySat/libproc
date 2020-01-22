/*
 * Modified by Greg Eddington
 *   - Changed pqueue_pri_t to a struct timeval.
 *   - Changed file name from pqueue to priorityQueue
 *
 * Copyright 2010 Volkan Yazıcı <volkan.yazici@gmail.com>
 * Copyright 2006-2010 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

/**
 * @file  pqueue.h
 * @brief Priority Queue function declarations
 */

#ifndef PRIORITY_QUEUE_H
#define PRIORITY_QUEUE_H

#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** priority data type */
typedef struct timeval ps_pqueue_pri_t;

/** callback functions to get/set/compare the priority of an element */
typedef ps_pqueue_pri_t (*ps_pqueue_get_pri_f)(void *a);
typedef void (*ps_pqueue_set_pri_f)(void *a, ps_pqueue_pri_t pri);
typedef int (*ps_pqueue_cmp_pri_f)(ps_pqueue_pri_t next, ps_pqueue_pri_t curr);


/** callback functions to get/set the position of an element */
typedef size_t (*ps_pqueue_get_pos_f)(void *a);
typedef void (*ps_pqueue_set_pos_f)(void *a, size_t pos);


/** debug callback function to print a entry */
typedef void (*ps_pqueue_print_entry_f)(FILE *out, void *a);


/** the priority queue handle */
typedef struct ps_pqueue_t
{
    size_t size;
    size_t avail;
    size_t step;
    ps_pqueue_cmp_pri_f cmppri;
    ps_pqueue_get_pri_f getpri;
    ps_pqueue_set_pri_f setpri;
    ps_pqueue_get_pos_f getpos;
    ps_pqueue_set_pos_f setpos;
    void **d;
} ps_pqueue_t;


/**
 * initialize the queue
 *
 * @param n the initial estimate of the number of queue items for which memory
 *          should be preallocated
 * @param pri the callback function to run to assign a score to a element
 * @param get the callback function to get the current element's position
 * @param set the callback function to set the current element's position
 *
 * @Return the handle or NULL for insufficent memory
 */
ps_pqueue_t *
ps_pqueue_init(size_t n,
            ps_pqueue_cmp_pri_f cmppri,
            ps_pqueue_get_pri_f getpri,
            ps_pqueue_set_pri_f setpri,
            ps_pqueue_get_pos_f getpos,
            ps_pqueue_set_pos_f setpos);


/**
 * free all memory used by the queue
 * @param q the queue
 */
void ps_pqueue_free(ps_pqueue_t *q);


/**
 * return the size of the queue.
 * @param q the queue
 */
size_t ps_pqueue_size(ps_pqueue_t *q);


/**
 * insert an item into the queue.
 * @param q the queue
 * @param d the item
 * @return 0 on success
 */
int ps_pqueue_insert(ps_pqueue_t *q, void *d);


/**
 * move an existing entry to a different priority
 * @param q the queue
 * @param old the old priority
 * @param d the entry
 */
void
ps_pqueue_change_priority(ps_pqueue_t *q,
                       ps_pqueue_pri_t new_pri,
                       void *d);


/**
 * pop the highest-ranking item from the queue.
 * @param p the queue
 * @param d where to copy the entry to
 * @return NULL on error, otherwise the entry
 */
void *ps_pqueue_pop(ps_pqueue_t *q);


/**
 * remove an item from the queue.
 * @param p the queue
 * @param d the entry
 * @return 0 on success
 */
int ps_pqueue_remove(ps_pqueue_t *q, void *d);


/**
 * access highest-ranking item without removing it.
 * @param q the queue
 * @param d the entry
 * @return NULL on error, otherwise the entry
 */
void *ps_pqueue_peek(ps_pqueue_t *q);


/**
 * print the queue
 * @internal
 * DEBUG function only
 * @param q the queue
 * @param out the output handle
 * @param the callback function to print the entry
 */
void
ps_pqueue_print(ps_pqueue_t *q,
             FILE *out,
             ps_pqueue_print_entry_f print);


/**
 * dump the queue and it's internal structure
 * @internal
 * debug function only
 * @param q the queue
 * @param out the output handle
 * @param the callback function to print the entry
 */
void
ps_pqueue_dump(ps_pqueue_t *q,
             FILE *out,
             ps_pqueue_print_entry_f print);


/**
 * checks that the pq is in the right order, etc
 * @internal
 * debug function only
 * @param q the queue
 */
int ps_pqueue_is_valid(ps_pqueue_t *q);

#ifdef __cplusplus
}
#endif

#endif /* PQUEUE_H */
/** @} */

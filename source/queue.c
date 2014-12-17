/*
 * Copyright (c) 2014 Jim Tremblay
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string.h>

#define NOS_PRIVATE
#include "nOS.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if (NOS_CONFIG_QUEUE_ENABLE > 0)
nOS_Error nOS_QueueCreate (nOS_Queue *queue, void *buffer, uint16_t bsize, uint16_t bmax)
{
    nOS_Error   err;

#if (NOS_CONFIG_SAFE > 0)
    if (queue == NULL) {
        err = NOS_E_NULL;
    } else if (queue->e.type != NOS_EVENT_INVALID) {
        err = NOS_E_INV_OBJ;
    } else if (buffer == NULL) {
        err = NOS_E_NULL;
    } else if (bsize == 0) {
        err = NOS_E_INV_VAL;
    } else if (bmax == 0) {
        err = NOS_E_INV_VAL;
    } else
#endif
    {
        nOS_CriticalEnter();
#if (NOS_CONFIG_SAFE > 0)
        nOS_EventCreate((nOS_Event*)queue, NOS_EVENT_QUEUE);
#else
        nOS_EventCreate((nOS_Event*)queue);
#endif
        queue->buffer = (uint8_t*)buffer;
        queue->bsize = bsize;
        queue->bmax = bmax;
        queue->bcount = 0;
        queue->r = 0;
        queue->w = 0;
        nOS_CriticalLeave();
        err = NOS_OK;
    }

    return err;
}

#if (NOS_CONFIG_QUEUE_DELETE_ENABLE > 0)
nOS_Error nOS_QueueDelete (nOS_Queue *queue)
{
    nOS_Error   err;

#if (NOS_CONFIG_SAFE > 0)
    if (queue == NULL) {
        err = NOS_E_NULL;
    } else if (queue->e.type != NOS_EVENT_QUEUE) {
        err = NOS_E_INV_VAL;
    } else
#endif
    {
        nOS_CriticalEnter();
        queue->buffer = NULL;
        queue->bsize = 0;
        queue->bmax = 0;
        queue->bcount = 0;
        queue->r = 0;
        queue->w = 0;
        if (nOS_EventDelete((nOS_Event*)queue)) {
            nOS_Sched();
        }
        nOS_CriticalLeave();
        err = NOS_OK;
    }

    return err;
}
#endif

nOS_Error nOS_QueueRead (nOS_Queue *queue, void *buffer, uint16_t tout)
{
    nOS_Error   err;

#if (NOS_CONFIG_SAFE > 0)
    if (queue == NULL) {
        err = NOS_E_NULL;
    } else if (queue->e.type != NOS_EVENT_QUEUE) {
        err = NOS_E_INV_VAL;
    } else if (buffer == NULL) {
        err = NOS_E_NULL;
    } else
#endif
    {
        nOS_CriticalEnter();
        if (queue->bcount > 0) {
            memcpy(buffer, &queue->buffer[(size_t)queue->r * (size_t)queue->bsize], queue->bsize);
            queue->r = (queue->r + 1) % queue->bmax;
            queue->bcount--;
            err = NOS_OK;
        } else if (tout == NOS_NO_WAIT) {
            err = NOS_E_EMPTY;
        } else if (nOS_isrNestingCounter > 0) {
            err = NOS_E_ISR;
        }
#if (NOS_CONFIG_SCHED_LOCK_ENABLE > 0)
        else if (nOS_lockNestingCounter > 0) {
            err = NOS_E_LOCKED;
        }
#endif
        else if (nOS_runningThread == &nOS_mainThread) {
            err = NOS_E_IDLE;
        } else {
            nOS_runningThread->context = buffer;
            err = nOS_EventWait((nOS_Event*)queue, NOS_THREAD_READING_QUEUE, tout);
        }
        nOS_CriticalLeave();
    }

    return err;
}

nOS_Error nOS_QueueWrite (nOS_Queue *queue, void *buffer)
{
    nOS_Error   err;
    nOS_Thread  *thread;

#if (NOS_CONFIG_SAFE > 0)
    if (queue == NULL) {
        err = NOS_E_NULL;
    } else if (queue->e.type != NOS_EVENT_QUEUE) {
        err = NOS_E_INV_VAL;
    } else if (buffer == NULL) {
        err = NOS_E_NULL;
    } else
#endif
    {
        nOS_CriticalEnter();
        thread = nOS_EventSignal((nOS_Event*)queue, NOS_OK);
        if (thread != NULL) {
            memcpy(thread->context, buffer, queue->bsize);
            if ((thread->state == NOS_THREAD_READY) && (thread->prio > nOS_runningThread->prio)) {
                nOS_Sched();
            }
            err = NOS_OK;
        } else if (queue->bcount < queue->bmax) {
            memcpy(&queue->buffer[(size_t)queue->w * (size_t)queue->bsize], buffer, queue->bsize);
            queue->w = (queue->w + 1) % queue->bmax;
            queue->bcount++;
            err = NOS_OK;
        } else {
            err = NOS_E_FULL;
        }
        nOS_CriticalLeave();
    }

    return err;
}

bool nOS_QueueIsEmpty (nOS_Queue *queue)
{
    bool    empty;

#if (NOS_CONFIG_SAFE > 0)
    if (queue == NULL) {
        empty = false;
    } else if (queue->e.type != NOS_EVENT_QUEUE) {
        empty = false;
    } else
#endif
    {
        nOS_CriticalEnter();
        empty = (queue->bcount == 0);
        nOS_CriticalLeave();
    }

    return empty;
}

bool nOS_QueueIsFull (nOS_Queue *queue)
{
    bool    full;

#if (NOS_CONFIG_SAFE > 0)
    if (queue == NULL) {
        full = false;
    } else if (queue->e.type != NOS_EVENT_QUEUE) {
        full = false;
    } else
#endif
    {
        nOS_CriticalEnter();
        full = (queue->bcount == queue->bmax);
        nOS_CriticalLeave();
    }

    return full;
}
#endif  /* NOS_CONFIG_QUEUE_ENABLE */

#if defined(__cplusplus)
}
#endif

/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George on behalf of Pycom Ltd
 * Copyright (c) 2017 Pycom Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "stdio.h"

#include "py/mpconfig.h"
#include "py/mpstate.h"
#include "py/gc.h"
#include "py/mpthread.h"
#include "mpthreadport.h"

//#include "esp_task.h"

#if MICROPY_PY_THREAD

#define MP_THREAD_MIN_STACK_SIZE                        (5 * 1024)
#define MP_THREAD_DEFAULT_STACK_SIZE                    (MP_THREAD_MIN_STACK_SIZE + 1024)

#define MP_THREAD_STATUS_READY                   0
#define MP_THREAD_STATUS_RUNNING                 1
#define MP_THREAD_STATUS_FINISH                  2

char *mp_thread_name[] = {

#ifdef MP_THREAD_MAX_NUM_10
	"mp_th0",
	"mp_th1",
	"mp_th2",
	"mp_th3",
	"mp_th4",
	"mp_th5",
	"mp_th6",
	"mp_th7",
	"mp_th8",
	"mp_th9",
#endif
#ifdef MP_THREAD_MAX_NUM_20
	"mp_th10",
	"mp_th11",
	"mp_th12",
	"mp_th13",
	"mp_th14",
	"mp_th15",
	"mp_th16",
	"mp_th17",
	"mp_th18",
	"mp_th19",
#endif
#ifdef MP_THREAD_MAX_NUM_32
	"mp_th20",
	"mp_th21",
	"mp_th22",
	"mp_th23",
	"mp_th24",
	"mp_th25",
	"mp_th26",
	"mp_th27",
	"mp_th28",
	"mp_th29",
	"mp_th30",
	"mp_th31",
	"mp_th32",
#endif
};

// this structure forms a linked list, one node per active thread
typedef struct _thread_t {
    TaskHandle_t id;        // system id of thread
    int status;             // whether the thread is ready and running
    void *arg;              // thread Python args, a GC root pointer
    void *stack;            // pointer to the stack
    size_t stack_len;       // number of words in the stack
    struct _thread_t *next;
} thread_t;

// the mutex controls access to the linked list
STATIC mp_thread_mutex_t g_thread_mutex;
STATIC thread_t g_thread_root_node;
STATIC thread_t *g_thread_root; // root pointer, handled by mp_thread_gc_others

int g_thread_counter = 0;

void mp_thread_init(void *stack, uint32_t stack_len) {
    mp_thread_set_state(&mp_state_ctx.thread);
    // create the first entry in the linked list of all threads
    g_thread_root = &g_thread_root_node;
    g_thread_root->id = usr_GetCurrentThreadHandle();
    g_thread_root->status = MP_THREAD_STATUS_RUNNING;
    g_thread_root->arg = NULL;
    g_thread_root->stack = stack;
    g_thread_root->stack_len = stack_len;
	g_thread_root->next = NULL;
	
    mp_thread_mutex_init(&g_thread_mutex);
}

void mp_thread_gc_others(void) {
	
    mp_thread_mutex_lock(&g_thread_mutex, 1);
    for (thread_t *th = g_thread_root; th != NULL; th = th->next) {

        if (th->id != usr_GetCurrentThreadHandle()) {
            gc_collect_root((void**)&th, 1);
			gc_collect_root(&th->arg, 1); // probably not needed
        }

        if (th->status == MP_THREAD_STATUS_READY) {
            continue;
        }

		gc_collect_root((void**) &th->id, 1);
        gc_collect_root(th->stack, th->stack_len); // probably not needed
    }
    mp_thread_mutex_unlock(&g_thread_mutex);
}

mp_state_thread_t *mp_thread_get_state(void) {
	 return usr_GetThread_StoragePoint();
}

void mp_thread_set_state(void *state) {
	usr_SetThread_StoragePoint(state);
}

void mp_thread_start(void) {
    mp_thread_mutex_lock(&g_thread_mutex, 1);
	for (thread_t *th = g_thread_root; th != NULL; th = th->next) {
        if (th->id == usr_GetCurrentThreadHandle()) {
            th->status = MP_THREAD_STATUS_RUNNING;
            break;
        }
    }
    mp_thread_mutex_unlock(&g_thread_mutex);
}


void mp_thread_create_ex(void *(*entry)(void*), void *arg, size_t *stack_size, char *name) {
   
    if (*stack_size == 0) {
        *stack_size = MP_THREAD_DEFAULT_STACK_SIZE; // default stack size
    } else if (*stack_size < MP_THREAD_MIN_STACK_SIZE) {
        *stack_size = MP_THREAD_MIN_STACK_SIZE; // minimum stack size
    }

    // Allocate linked-list node (must be outside thread_mutex lock)
    thread_t *th = m_new_obj(thread_t);

    if (th == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "can't create thread obj"));
    }
	
	th->id = usr_CreateTaskID();
    if (th->id == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "can't create thread id"));
    }
	
	th->stack = m_new(uint8_t, *stack_size);
    if (th->stack == NULL) {
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "can't create thread stack"));
    }
	
	mp_log("(mpthread : 0x%p) stack addr is :0x%p, size is %d", th, th->stack, *stack_size);
    mp_thread_mutex_lock(&g_thread_mutex, 1);

    // adjust the stack_size to provide room to recover from hitting the limit
    *stack_size -= 1024;

    // add thread to linked list of all threads
    th->status = MP_THREAD_STATUS_READY;
    th->arg = arg;

    th->stack_len = *stack_size / sizeof(usr_StackType_t);
    th->next = g_thread_root;
    g_thread_root = th;
 
	
    // create thread
    usr_BaseType_t result = usr_xTaskCreate(entry, name, *stack_size, arg, th->id, th->stack);
    if (result != 1) {
        mp_thread_mutex_unlock(&g_thread_mutex);
        nlr_raise(mp_obj_new_exception_msg(&mp_type_OSError, "can't create thread"));
    }
	
	
    mp_thread_mutex_unlock(&g_thread_mutex);
}


void mp_thread_create(void *(*entry)(void*), void *arg, size_t *stack_size) {
 	mp_thread_create_ex(entry, arg, stack_size, mp_thread_name[g_thread_counter]);
	g_thread_counter = g_thread_counter + 1;

}

void mp_thread_finish(void) {
	thread_t *prev = NULL;
	
    mp_thread_mutex_lock(&g_thread_mutex, 1);
	for (thread_t *th = g_thread_root; th != NULL;prev = th, th = th->next) {
		// unlink the node from the list
        if (th->id == usr_GetCurrentThreadHandle()) {
			if (prev != NULL) {
                prev->next = th->next;
            } else {
                // move the start pointer
                g_thread_root = th->next;
            }
            th->status = MP_THREAD_STATUS_FINISH;
			// explicitly release all its memory
			usr_DeleteTaskID(th->id);
            m_del(uint8_t, th->stack, th->stack_len * sizeof(usr_StackType_t) + 1024);
            m_del_obj(thread_t, th);
            break;
        }
    }
    mp_thread_mutex_unlock(&g_thread_mutex);
}


void mp_thread_mutex_init(mp_thread_mutex_t *mutex) {
    usr_CreateMutexStatic(mutex);
}

int mp_thread_mutex_lock(mp_thread_mutex_t *mutex, int wait) {
    return (1 == usr_mutexTake(mutex, wait));
}

void mp_thread_mutex_unlock(mp_thread_mutex_t *mutex) {
	usr_mutexGive(mutex);
}

void mp_thread_deinit(void) {
	
	for (thread_t *th = g_thread_root; th != NULL; th = th->next) {
		if (th != &g_thread_root_node && th->status != MP_THREAD_STATUS_FINISH) {
			usr_ThreadDeinit(th->id);
		}
	}
}

mp_thread_evevt_t mp_Creat_event(void)
{
	return usr_Creat_event();
}

int mp_send_event(mp_thread_evevt_t event, int set)
{
	return usr_send_event(event, set);
}

int mp_recv_event(mp_thread_evevt_t event, int set, int timeout)
{
	return usr_recv_event(event, set, timeout);
}

void mp_delete_event(mp_thread_evevt_t event)
{
	return usr_delete_event(event);
}

void mp_set_tick(int tick)
{
	usr_set_tick(tick);
}

void mp_set_priority(unsigned char pri)
{
	usr_set_priority(pri);
}


#else

void vPortCleanUpTCB(void *tcb) {
}

/*
void vPortCleanUpTCB(void *tcb) {
    thread_t *prev = NULL;
    mp_thread_mutex_lock(&g_thread_mutex, 1);
    for (thread_t *th = thread; th != NULL; prev = th, th = th->next) {
        // unlink the node from the list
        if ((void*)th->id == tcb) {
            if (prev != NULL) {
                prev->next = th->next;
            } else {
                // move the start pointer
                thread = th->next;
            }
            // explicitly release all its memory
            m_del(thread_t, th, 1);
            break;
        }
    }
    mp_thread_mutex_unlock(&g_thread_mutex);
}
*/

#endif // MICROPY_PY_THREAD

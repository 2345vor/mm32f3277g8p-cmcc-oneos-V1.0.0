#ifndef _USR_THREAD_H
#define _USR_THREAD_H
#include <stdint.h>
#include <stdlib.h>
#include <os_task.h>

#include "usr_misc.h"

//#define mp_mutex struct os_mutex 


//usr def
typedef void * TaskHandle_t;


typedef void * mp_mutex;


typedef struct _mp_thread_mutex_t {
    mp_mutex mutex;
    int is_init;
} mp_thread_mutex_t;

//typedef os_event_t  * mp_thread_evevt_t;
typedef void  * mp_thread_evevt_t;

//usr mutex struct
/*
typedef struct usr_mutex{
	int is_init;
	mp_mutex sem;
}usr_mutex_t;
*/
void *usr_GetStack_addr(void);
void * usr_GetThread_StoragePoint(void);
void usr_SetThread_StoragePoint(void * state);

void usr_CreateMutexStatic(mp_thread_mutex_t * mutex);
int usr_mutexTake(mp_thread_mutex_t * mutex, unsigned int delay);
void usr_mutexGive(mp_thread_mutex_t * mutex);

TaskHandle_t usr_GetCurrentThreadHandle(void);
void usr_ThreadDelete(TaskHandle_t threadid );
int usr_xTaskCreate(	void *(*entry)(void*),
						const char * const pcName,
						const int usStackDepth,
						void * const pvParameters,
						TaskHandle_t pxCreatedTask,
							void * stackstart);

mp_thread_evevt_t usr_Creat_event(void);
int usr_send_event(mp_thread_evevt_t event, int set);
int usr_recv_event(mp_thread_evevt_t event, int set, int timeout);
void usr_delete_event(mp_thread_evevt_t event);
void usr_set_tick(int tick);
void usr_set_priority(unsigned char pri);
				
void * usr_CreateTaskID(void);	
void usr_DeleteTaskID(TaskHandle_t id);
void usr_ThreadDeinit(TaskHandle_t id);						
os_err_t usr_task_delete(os_task_t *task);

#endif

#include "usr_thread.h"
#include "py/mpstate.h"
#include <pin/pin.h>
#include "os_event.h"
#include "os_clock.h"
#include "mpthreadport.h"
#include "os_mutex.h"
#include "os_event.h"
#include "model_def.h"

#if (MICROPY_PY_THREAD)

unsigned int thread_tick = 0xff;
unsigned char thread_priority = 25;


mp_thread_evevt_t testevent;

void *usr_GetStack_addr(void)
{
	return os_task_self()->stack_begin;
}


mp_uint_t usr_GetStack_size(void)
{
	os_task_t * task = os_task_self();
	return (mp_uint_t)((u32_t *)(task->stack_end) - (u32_t *)task->stack_begin);
}

void * usr_GetThread_StoragePoint(void)
{
	return (mp_state_thread_t *)(os_task_self()->user_data);
}

void usr_SetThread_StoragePoint(void * state)
{
	os_task_self()->user_data = state;
}


void usr_CreateMutexStatic(mp_thread_mutex_t * mutex)
{
	static uint8_t count = 0;
    char name[OS_NAME_MAX];

    if (!mutex->is_init) {
        /* build name */
        snprintf(name, sizeof(name), "mpl%02d", count++);
		mutex->mutex = os_mutex_create(name, OS_FALSE);
		if (! mutex->mutex){
			mp_err("Created mutex falied!");
			while (1);
		}
        //os_mutex_init((struct os_mutex *)(mutex->mutex), name, OS_IPC_FLAG_FIFO, OS_FALSE);
        mutex->is_init = 1;
    }
}

int usr_mutexTake(mp_thread_mutex_t * mutex, unsigned int delay)
{
	return (OS_EOK == os_mutex_lock((struct os_mutex *)(mutex->mutex), delay ? OS_WAIT_FOREVER : OS_NO_WAIT));
}

void usr_mutexGive(mp_thread_mutex_t * mutex)
{
	os_mutex_unlock((struct os_mutex *)(mutex->mutex));
}



TaskHandle_t usr_GetCurrentThreadHandle(void)
{
	return os_task_self();
}

int usr_xTaskCreate(void *(*entry)(void*),
					const char * const pcName,
					const int usStackDepth,
					void * const pvParameters,
					TaskHandle_t  pxCreatedTask,
					void * stackstart)
{
	int result;

	os_task_init(pxCreatedTask, pcName, (void (*)(void *))entry, pvParameters, stackstart, usStackDepth, thread_priority);
	result = os_task_startup(pxCreatedTask);
	if(result == 0)
	{
		return 1;
	}
	
	return 0;
}


void * usr_CreateTaskID(void)
{
	return m_new_obj(struct os_task);
}

void usr_DeleteTaskID(TaskHandle_t id)
{
	m_del_obj(struct os_task, id);
}

void usr_ThreadDeinit(TaskHandle_t id)
{
	os_task_deinit(id);
}

mp_thread_evevt_t usr_Creat_event(void)
{
	mp_thread_evevt_t event = m_new_obj(os_event_t);
	
	if(event == NULL)
	{
		mp_err("malloc os_event_t failed!");
		return NULL;
	}
	
	os_event_init(event, "mpyevent");
	
	return event;
}

int usr_send_event(mp_thread_evevt_t event, int set)
{
	if(os_event_send(event, set) != OS_EOK)
	{
		return -1;
	}
	
	return 0;
}

int usr_recv_event(mp_thread_evevt_t event, int set, int timeout)
{
	if(os_event_recv(event, set, OS_EVENT_OPTION_AND | OS_EVENT_OPTION_CLEAR, timeout, NULL) != OS_EOK)
	{
		return -1;
	}
	
	return 0;
}

void usr_delete_event(mp_thread_evevt_t event)
{
	os_event_deinit(event);
	m_del(mp_thread_evevt_t, event, sizeof(os_event_t *));
}

//ms
void usr_set_tick(int tick)
{
	if((tick * OS_TICK_PER_SECOND) >= 1000)
	{
		thread_tick = (OS_TICK_PER_SECOND * tick) / 1000;
	}
	else
	{
		thread_tick = 1;
	}
}

void usr_set_priority(unsigned char pri)
{
	thread_priority = pri;
}

void call_sendevent(void *args)
{
	mp_log("send event:%d", os_tick_get());
	os_event_send(testevent, 0xff);
}

void usr_test_event(void)
{
	testevent = m_new_obj(struct os_event);
	os_event_init(testevent, "mpyevent");
	os_pin_mode(44, PIN_MODE_INPUT_PULLUP); //PA11
	os_pin_attach_irq(44,PIN_IRQ_MODE_FALLING ,call_sendevent ,OS_NULL);
	os_pin_irq_enable(44, PIN_IRQ_ENABLE);
	
	os_event_recv(testevent, 0xff, OS_EVENT_OPTION_AND | OS_EVENT_OPTION_CLEAR, 0xffffffff, NULL);
	mp_log("recv event:%d", os_tick_get());
}

#endif

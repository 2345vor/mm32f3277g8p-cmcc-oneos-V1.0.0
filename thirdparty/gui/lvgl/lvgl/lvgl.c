#include <board.h>
#include <os_memory.h>
#include <graphic/graphic.h>
#include <touch/touch.h>
#include <lvgl/lvgl.h>
#include <os_task.h>
#include <dlog.h>
#define DBG_TAG  "lvgl"

struct lvgl_device {
    /* disp */
    struct os_device *disp_device;
    struct os_device_graphic_info disp_info;

    /* touch */
#ifdef OS_USING_TOUCH
    struct os_device *indev_device;
#endif    
};

static void lvgl_disp_flush(struct _disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    struct lvgl_device *device = disp_drv->user_data;

    struct os_device_rect_info rect;

    rect.x = area->x1;
    rect.width = area->x2 - area->x1 + 1;
    rect.y = area->y1;
    rect.height = area->y2 - area->y1 + 1;
    rect.color = (char *)color_p;
                    
    os_device_control(device->disp_device, OS_GRAPHIC_CTRL_FILL, &rect);

    lv_disp_flush_ready(disp_drv);
}

#ifdef OS_USING_TOUCH
static bool lvgl_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    struct lvgl_device *device = indev_drv->user_data;

    struct os_touch_data touch_data;
    int count = os_device_read_nonblock(device->indev_device, 0, &touch_data, 1);
    if (count == 0)
        goto end;

    if (touch_data.event == OS_TOUCH_EVENT_DOWN)
        data->state = LV_INDEV_STATE_PR;
    else if (touch_data.event == OS_TOUCH_EVENT_UP)
        data->state = LV_INDEV_STATE_REL;
    else
        goto end;

    last_x = touch_data.x_coordinate;
    last_y = touch_data.y_coordinate;

    LOG_D(DBG_TAG, "%d, (%d, %d)", touch_data.event, last_x, last_y);

end:
    data->point.x = last_x;
    data->point.y = last_y;

    return false; /*Return `false` because we are not buffering and no more data to read*/
}
#endif

#ifndef OS_LV_BUFF_LINES
#define OS_LV_BUFF_LINES 30
#endif

static void disp_init(struct lvgl_device *lvgl_dev)
{
    /* hardware init */
    os_device_t *disp_device = os_device_find(OS_GUI_DISP_DEV_NAME);
    OS_ASSERT(disp_device);
    lvgl_dev->disp_device = disp_device;
    os_device_open(disp_device);
    
    os_device_control(disp_device, OS_GRAPHIC_CTRL_GET_INFO, (void *)&lvgl_dev->disp_info);
    //OS_ASSERT(lvgl_dev->disp_info.framebuffer);
    os_kprintf("LCD size: %dX%d\r\n", lvgl_dev->disp_info.width, lvgl_dev->disp_info.height);

    /* gui display buffer */
    /* Declare a buffer for 10 lines */
    lv_color_t *buf1 = OS_NULL;
    lv_color_t *buf2 = OS_NULL;

    buf1 = os_calloc(LV_HOR_RES_MAX * OS_LV_BUFF_LINES, sizeof(lv_color_t));
    OS_ASSERT(buf1);
   static lv_color_t disp_buf1[LV_HOR_RES_MAX * OS_LV_BUFF_LINES];
#if defined(OS_LV_BUFF_DOUBLE) && (OS_LV_BUFF_DOUBLE == 1)
    buf2 = os_calloc(LV_HOR_RES_MAX * OS_LV_BUFF_LINES, sizeof(lv_color_t));
    OS_ASSERT(buf2);
#endif
    
    lv_disp_buf_t *disp_buf = os_calloc(1, sizeof(lv_disp_buf_t));
    OS_ASSERT(disp_buf);
    lv_disp_buf_init(disp_buf, disp_buf1, buf2, LV_HOR_RES_MAX * OS_LV_BUFF_LINES);    /*Initialize the display buffer*/

    /* gui display */
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
    disp_drv.flush_cb = lvgl_disp_flush;    /*Set your driver function*/
    disp_drv.buffer = disp_buf;             /*Assign the buffer to the display*/
    disp_drv.user_data = lvgl_dev;
    lv_disp_drv_register(&disp_drv);        /*Finally register the driver*/
}

static void input_init(struct lvgl_device *lvgl_dev)
{
#ifdef OS_USING_TOUCH
    /* touch */
    os_device_t *indev_device = os_device_find(OS_GUI_INPUT_DEV_NAME);
    OS_ASSERT(indev_device);
    lvgl_dev->indev_device = indev_device;
    os_device_open(indev_device);

    uint8_t id[10];
    memset(id, 0, sizeof(id));
    os_device_control(indev_device, OS_TOUCH_CTRL_GET_ID, id);
    LOG_I(DBG_TAG, "touch id = %02x %02x %02x %02x", 
                id[0], id[1], id[2], id[3]);

    /* gui input */
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);               /*Descriptor of a input device driver*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;     /*Touch pad is a pointer-like device*/
    indev_drv.read_cb = lvgl_touchpad_read;     /*Set your driver function*/
    indev_drv.user_data = lvgl_dev;
    lv_indev_drv_register(&indev_drv);          /*Finally register the driver*/
#endif
}

uint64_t start=0, end=0;
static void gui_thread(void *parameter)
{
    const int tick = OS_TICK_PER_SECOND / 1000 + 1;

    while (1)
    {
        os_task_tsleep(tick);
        //start = DWT->CYCCNT;
        lv_task_handler();
        //end = DWT->CYCCNT;
        //os_kprintf("dwt: %d\n", (uint32_t)(end-start));
    }
}

static int os_gui_init(void)
{
    /* gui lib */
    lv_init();

    /* gui hardware */
    struct lvgl_device *lvgl_dev = os_calloc(1, sizeof(struct lvgl_device));
    OS_ASSERT(lvgl_dev);
    
    disp_init(lvgl_dev);
    input_init(lvgl_dev);

    /* gui thread */
    os_task_t *task = os_task_create("gui", gui_thread, NULL, 4096, OS_TASK_PRIORITY_MAX - 3);
    OS_ASSERT(task);
    os_task_startup(task);

    return 0;
}
OS_ENV_INIT(os_gui_init, OS_INIT_SUBLEVEL_HIGH);

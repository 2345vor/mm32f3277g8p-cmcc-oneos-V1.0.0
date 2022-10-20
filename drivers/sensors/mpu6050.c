

#include <drv_cfg.h>
#include <os_memory.h>
#include <sensors/sensor.h>
#include <math.h>

#define DBG_EXT_TAG "sensor.sensirion.mpu6050"
#include <dlog.h>

typedef struct
{
	struct os_sensor_device sensor;
	struct os_i2c_client    i2c;
	os_uint8_t              id;

	os_int16_t mpu_acce_x;
	os_int16_t mpu_acce_y;
	os_int16_t mpu_acce_z;

	os_int16_t mpu_gyro_x;
	os_int16_t mpu_gyro_y;
	os_int16_t mpu_gyro_z;

	os_mutex_t *lock;
} mpu6050_info_t;

#define	PWR_MGMT_1				0x6B										// 电源管理，典型值：0x00(正常启用)
#define	SMPLRT_DIV				0x19										// 陀螺仪采样率，典型值：0x07(125Hz)

#define	MPU6050_CONFIG			0x1A										// 低通滤波频率，典型值：0x06(5Hz)
#define	GYRO_CONFIG				0x1B										// 陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
#define	ACCEL_CONFIG			0x1C										// 加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz)
#define INT_PIN_CFG				0x37										// 设置6050辅助I2C为直通模式寄存器

#define INT_PIN_CFG				0x37										// 设置6050辅助I2C为直通模式寄存器
#define	ACCEL_XOUT_H			0x3B
#define	ACCEL_XOUT_L			0x3C
#define	ACCEL_YOUT_H			0x3D
#define	ACCEL_YOUT_L			0x3E
#define	ACCEL_ZOUT_H			0x3F
#define	ACCEL_ZOUT_L			0x40
#define	GYRO_XOUT_H				0x43
#define	GYRO_XOUT_L				0x44
#define	GYRO_YOUT_H				0x45
#define	GYRO_YOUT_L				0x46
#define	GYRO_ZOUT_H				0x47
#define	GYRO_ZOUT_L				0x48

#define User_Control			0x6A										// 关闭6050对辅助I2C设备的控制

static os_err_t mpu6050_self1_check(mpu6050_info_t *mpu6050)
{
	os_uint8_t val = 0x00;
	os_uint8_t reg = 0x07;

	os_i2c_client_write(&mpu6050->i2c, PWR_MGMT_1, 1, &val, 1);
	os_i2c_client_write(&mpu6050->i2c, SMPLRT_DIV, 1, &reg, 1);

	os_i2c_client_read(&mpu6050->i2c, SMPLRT_DIV, 1, &val, 1);

	if (val == 0x07)
		return OS_EOK;
	else
		return OS_ERROR;
}

static mpu6050_info_t *mpu6050_init(const char *bus_name, os_uint16_t addr)
{
	uint8_t count;
	// 接触休眠 125Hz采样 低通滤波频率 +/-2000deg/s量程 +/-8g量程 关闭辅助 I2C直通模式
	uint8_t tmp[7] = {0x00, 0x07, 0x04, 0x18, 0x10, 0x00, 0x02};
	static mpu6050_info_t *mpu6050   = OS_NULL;

	os_err_t result = 0;

	mpu6050 = os_calloc(1, sizeof(mpu6050_info_t));
	if (mpu6050 == OS_NULL)
	{
		LOG_E(DBG_EXT_TAG,"mpu6050 amlloc faile");
		return OS_NULL;
	}

	LOG_I(DBG_EXT_TAG,"mpu6050:[%s][0x%02x]", bus_name, addr);

	mpu6050->i2c.bus = os_i2c_bus_device_find(bus_name);
	if (mpu6050->i2c.bus == OS_NULL)
	{
		LOG_E(DBG_EXT_TAG,"mpu6050 i2c invalid.");
		os_free(mpu6050);
		return OS_NULL;
	}

	mpu6050->lock = os_mutex_create("mutex_mpu6050", OS_FALSE);
	if (mpu6050->lock == OS_NULL)
	{
		LOG_E(DBG_EXT_TAG,"Can't create mutex for mpu6050 device on '%s' ", bus_name);
		os_free(mpu6050);
		return OS_NULL;
	}

#ifdef OS_MPU6050_I2C_ADDR
	mpu6050->i2c.client_addr = OS_MPU6050_I2C_ADDR;
#else
	mpu6050->i2c.client_addr = 0x68;
#endif

	os_task_msleep(100);

	result = mpu6050_self1_check(mpu6050);
	while(result != OS_EOK)
	{
		os_task_msleep(20);
		result = mpu6050_self1_check(mpu6050);
		count++;
		if(count >= 10)
		{
			LOG_W(DBG_EXT_TAG,"mpu6050 init error.");
			return 0;
		}
	}

	os_i2c_client_write(&mpu6050->i2c, PWR_MGMT_1, 1, &tmp[0], 1);
	os_i2c_client_write(&mpu6050->i2c, SMPLRT_DIV, 1, &tmp[1], 1);
	os_i2c_client_write(&mpu6050->i2c, MPU6050_CONFIG, 1, &tmp[2], 1);
	os_i2c_client_write(&mpu6050->i2c, GYRO_CONFIG, 1, &tmp[3], 1);
	os_i2c_client_write(&mpu6050->i2c, ACCEL_CONFIG, 1, &tmp[4], 1);
	os_i2c_client_write(&mpu6050->i2c, User_Control, 1, &tmp[5], 1);
	os_i2c_client_write(&mpu6050->i2c, INT_PIN_CFG, 1, &tmp[6], 1);

	return mpu6050;
}

static os_size_t mpu6050_acce_fetch_data(struct os_sensor_device *sensor, void *buf, os_size_t len)
{
	mpu6050_info_t          *mpu6050 = OS_NULL;
	struct os_sensor_data	*data  = OS_NULL;
	os_uint8_t				temp[6];
	os_err_t				result;

	OS_ASSERT(sensor);
	OS_ASSERT(sensor->info.type == OS_SENSOR_CLASS_ACCE);
	OS_ASSERT(buf);

	mpu6050 = (mpu6050_info_t *)sensor;
	data  = (struct os_sensor_data *)buf;

	result = os_mutex_lock(mpu6050->lock, OS_WAIT_FOREVER);
	if (result == OS_EOK)
	{
		os_i2c_client_read(&mpu6050->i2c, ACCEL_XOUT_H, 1, temp, 6);

		mpu6050->mpu_acce_x = (os_int16_t)(((os_uint16_t)temp[0]<<8 | temp[1]));
		mpu6050->mpu_acce_y = (os_int16_t)(((os_uint16_t)temp[2]<<8 | temp[3]));
		mpu6050->mpu_acce_z = (os_int16_t)(((os_uint16_t)temp[4]<<8 | temp[5]));
	}
	else
	{
		LOG_E(DBG_EXT_TAG,"The mpu6050 could not respond acce measurement at this time. Please try again");
	}
	os_mutex_unlock(mpu6050->lock);

	data->type     		= sensor->info.type;
	data->data.acce.x	= mpu6050->mpu_acce_x;
	data->data.acce.y	= mpu6050->mpu_acce_y;
	data->data.acce.z	= mpu6050->mpu_acce_z;
	data->timestamp		= os_sensor_get_ts();
	return 0;
}

static os_err_t mpu6050_acce_control(struct os_sensor_device *sensor, int cmd, void *args)
{
	os_err_t		result		= OS_EOK;
	mpu6050_info_t	*mpu6050	= (mpu6050_info_t *)sensor;

	switch (cmd)
	{
		case OS_SENSOR_CTRL_GET_ID:
			*(os_uint8_t *)args = mpu6050->id;
			break;
		default:
			return OS_ERROR;
	}
	return result;
}

static struct os_sensor_ops mpu6050_acce_ops =
{
	mpu6050_acce_fetch_data,
	mpu6050_acce_control,
};

static int os_hw_mpu6050_acce_init(void)
{
	os_int8_t     result;
	mpu6050_info_t *mpu6050;

	mpu6050 = mpu6050_init(OS_MPU6050_I2C_BUS_NAME, OS_MPU6050_I2C_ADDR);
	if (mpu6050 == OS_NULL)
	{
		LOG_E(DBG_EXT_TAG,"mpu6050 acce init failed.");
		goto __exit;
	}

	/* Temp */
	mpu6050->sensor.info.type       = OS_SENSOR_CLASS_ACCE;
	mpu6050->sensor.info.vendor     = OS_SENSOR_VENDOR_SENSIRION;
	mpu6050->sensor.info.model      = "mpu6050";
	mpu6050->sensor.info.unit       = OS_SENSOR_UNIT_MG;
	mpu6050->sensor.info.intf_type  = OS_SENSOR_INTF_I2C;
	mpu6050->sensor.info.range_max  = 100000;
	mpu6050->sensor.info.range_min  = 0;
	mpu6050->sensor.info.period_min = 300;
	mpu6050->sensor.ops             = &mpu6050_acce_ops;

	result = os_hw_sensor_register(&mpu6050->sensor, "mpu6050", OS_NULL);
	if (result != OS_EOK)
	{
		LOG_E(DBG_EXT_TAG,"device acce register err code: %d", result);
		goto __exit;
	}

	struct os_sensor_data sensor_data;
	mpu6050_acce_fetch_data(&mpu6050->sensor, &sensor_data, sizeof(sensor_data));

	LOG_I(DBG_EXT_TAG,"mpu6050 acce init success");
	return OS_EOK;

__exit:
	if (mpu6050)
		os_free(mpu6050);
	return OS_ERROR;
}
OS_DEVICE_INIT(os_hw_mpu6050_acce_init, OS_INIT_SUBLEVEL_LOW);

static os_size_t mpu6050_gyro_fetch_data(struct os_sensor_device *sensor, void *buf, os_size_t len)
{
	mpu6050_info_t          *mpu6050 = OS_NULL;
	struct os_sensor_data	*data  = OS_NULL;
	os_uint8_t				temp[6];
	os_err_t				result;

	OS_ASSERT(sensor);
	OS_ASSERT(sensor->info.type == OS_SENSOR_CLASS_GYRO);
	OS_ASSERT(buf);

	mpu6050 = (mpu6050_info_t *)sensor;
	data  = (struct os_sensor_data *)buf;

	result = os_mutex_lock(mpu6050->lock, OS_WAIT_FOREVER);
	if (result == OS_EOK)
	{
		os_i2c_client_read(&mpu6050->i2c, GYRO_XOUT_H, 1, temp, 6);

		mpu6050->mpu_gyro_x = (os_int16_t)(((os_uint16_t)temp[0]<<8 | temp[1]));
		mpu6050->mpu_gyro_y = (os_int16_t)(((os_uint16_t)temp[2]<<8 | temp[3]));
		mpu6050->mpu_gyro_z = (os_int16_t)(((os_uint16_t)temp[4]<<8 | temp[5]));
	}
	else
	{
		LOG_E(DBG_EXT_TAG,"The mpu6050 could not respond gyro measurement at this time. Please try again");
	}
	os_mutex_unlock(mpu6050->lock);

	data->type     		= sensor->info.type;
	data->data.gyro.x	= mpu6050->mpu_gyro_x;
	data->data.gyro.y	= mpu6050->mpu_gyro_y;
	data->data.gyro.z	= mpu6050->mpu_gyro_z;
	data->timestamp		= os_sensor_get_ts();
	return 0;
}

static os_err_t mpu6050_gyro_control(struct os_sensor_device *sensor, int cmd, void *args)
{
	os_err_t		result		= OS_EOK;
	mpu6050_info_t	*mpu6050	= (mpu6050_info_t *)sensor;

	switch (cmd)
	{
		case OS_SENSOR_CTRL_GET_ID:
			*(os_uint8_t *)args = mpu6050->id;
			break;
		default:
			return OS_ERROR;
	}
	return result;
}

static struct os_sensor_ops mpu6050_gyro_ops =
{
	mpu6050_gyro_fetch_data,
	mpu6050_gyro_control,
};

static int os_hw_mpu6050_gyro_init(void)
{
	os_int8_t     result;
	mpu6050_info_t *mpu6050;

	mpu6050 = mpu6050_init(OS_MPU6050_I2C_BUS_NAME, OS_MPU6050_I2C_ADDR);
	if (mpu6050 == OS_NULL)
	{
		LOG_E(DBG_EXT_TAG,"mpu6050 gyro init failed.");
		goto __exit;
	}

	/* Temp */
	mpu6050->sensor.info.type       = OS_SENSOR_CLASS_GYRO;
	mpu6050->sensor.info.vendor     = OS_SENSOR_VENDOR_SENSIRION;
	mpu6050->sensor.info.model      = "mpu6050";
	mpu6050->sensor.info.unit       = OS_SENSOR_UNIT_MDPS;
	mpu6050->sensor.info.intf_type  = OS_SENSOR_INTF_I2C;
	mpu6050->sensor.info.range_max  = 100000;
	mpu6050->sensor.info.range_min  = 0;
	mpu6050->sensor.info.period_min = 300;
	mpu6050->sensor.ops             = &mpu6050_gyro_ops;

	result = os_hw_sensor_register(&mpu6050->sensor, "mpu6050", OS_NULL);
	if (result != OS_EOK)
	{
		LOG_E(DBG_EXT_TAG,"device gyro register err code: %d", result);
		goto __exit;
	}

	struct os_sensor_data sensor_data;
	mpu6050_gyro_fetch_data(&mpu6050->sensor, &sensor_data, sizeof(sensor_data));

	LOG_I(DBG_EXT_TAG,"mpu6050 gyro init success");
	return OS_EOK;

__exit:
	if (mpu6050)
		os_free(mpu6050);
	return OS_ERROR;
}
OS_DEVICE_INIT(os_hw_mpu6050_gyro_init, OS_INIT_SUBLEVEL_LOW);


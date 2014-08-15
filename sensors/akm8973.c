/*
 * Copyright (C) 2013 Paul Kocialkowski <contact@paulk.fr>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>

#include <hardware/sensors.h>
#include <hardware/hardware.h>

#define LOG_TAG "wave_sensors"
#include <utils/Log.h>

#include "wave_sensors.h"
#include "ak8973-reg.h"
#include "ak8973.h"

#define AKM8973_CONFIG_PATH			"/data/misc/akmd_set.txt"

struct akm8973_data {
	struct wave_sensors_handlers *orientation_sensor;

	sensors_vec_t magnetic;
	unsigned char magnetic_data[4][3];
	int magnetic_data_count;
	int magnetic_data_index;

	unsigned char magnetic_extrema[2][3];
	unsigned char gain_indexes[3];
	unsigned char hdac[3];
	int ho[3];

	long int delay;
	int device_fd;
	int uinput_fd;

	pthread_t thread;
	pthread_mutex_t mutex;
	int thread_continue;
};

// This AKM8973 implementation is based on intuitive understanding of how the
// AKM8973 data is translated to SI units.
//
// Different parameters are used to properly configure and interpret the data:
// * Hardware gain, which is stored in the chip's EEPROM and is a factory
//   setting that shouldn't be changed. We don't have to deal with this value at
//   all for our computation.
// * Software gain, that is a factory setting we use to determine the
//   coefficient to use at some point in the final equation.
// * Hardware DAC offset (HDAC) that we can tune in order to be able to see
//   variations of the raw values. This value is not a linear representation
//   of the offset! See page 22 of the AKM8973 datasheet for further details.
// * Software offset (HO) that we can tune in order to have the maximum final
//   value at ~45uT and the minimal final value at ~-45uT for each axis.
//
// The final equation used to determine usable values from the raw input data
// was found to be:
// out = ((in - 128 - HO / 16) * k / 16
//
// * out: Final value in uT units
// * in: Raw value as received from the sensor
// * HO: HO software offset
// * k: Coefficient that is determined from the software gain
//
// When no software calibration is used, we can translate HDAC to HO using the
// following formulas:
// if HDAC < 128: HO = HDAC * -1 * 16 * 16
// if HDAC >= 128: HO = (HDAC - 128) * 16 * 16

// Constant gain-specific coefficients used in the final formula
float akm8973_gain_coefficient[] = {
	16.33f,
	16.28f,
	16.24f,
	16.18f,
	16.14f,
	16.10f,
	16.04f,
	16.00f,
	15.96f,
	15.90f,
	15.86f,
	15.82f,
	15.76f,
	15.72f,
	15.69f,
	15.65f,
};

int akm8973_gain_coefficient_count = sizeof(akm8973_gain_coefficient) /
	sizeof(float);

int akm8973_hdac(struct akm8973_data *data)
{
	char i2c_data[RWBUF_SIZE] = { 0 };
	int device_fd;
	int rc;

	if (data == NULL)
		return -EINVAL;

	device_fd = data->device_fd;
	if (device_fd < 0)
		return -1;

	i2c_data[0] = 4;
	i2c_data[1] = AK8973_REG_HXDA;
	i2c_data[2] = data->hdac[0];
	i2c_data[3] = data->hdac[1];
	i2c_data[4] = data->hdac[2];

	rc = ioctl(device_fd, ECS_IOCTL_WRITE, &i2c_data);
	if (rc < 0) {
		ALOGE("%s: Unable to write akm8973 data", __func__);
		return -1;
	}

	return 0;
}

int akm8973_magnetic_extrema(struct akm8973_data *data, int index)
{
	int gain_index;

	if (data == NULL || index < 0 || index >= 3)
		return -EINVAL;

	gain_index = data->gain_indexes[index];
	if (gain_index < 0 || gain_index >= akm8973_gain_coefficient_count)
		return -1;

	// Calculate the extrema from HO (software offset)
	data->magnetic_extrema[0][index] = (unsigned char) ((int) ((16.0f * -45.0f) / akm8973_gain_coefficient[gain_index] + 128 + data->ho[index] / 16.0f + 10.0f) & 0xff);
	data->magnetic_extrema[1][index] = (unsigned char) ((int) ((16.0f * 45.0f) / akm8973_gain_coefficient[gain_index] + 128 + data->ho[index] / 16.0f - 10.0f) & 0xff);

	return 0;
}

int akm8973_config_read(struct akm8973_data *data)
{
	char buffer[256] = { 0 };
	int config_fd = -1;
	int offset = 0;
	int length;
	int count;
	int value;
	char *p;
	int rc;

	if (data == NULL)
		return -EINVAL;

	config_fd = open(AKM8973_CONFIG_PATH, O_RDONLY);
	if (config_fd < 0) {
		ALOGE("%s: Unable to open akm8973 config %d %s", __func__, errno, strerror(errno));
		goto error;
	}

	rc = 0;

	do {
		lseek(config_fd, offset, SEEK_SET);

		length = read(config_fd, buffer, sizeof(buffer));
		if (length <= 0)
			break;

		p = strchr((const char *) &buffer, '\n');
		if (p != NULL) {
			offset += (int) p - (int) buffer + 1;
			*p = '\0';
		} else if ((size_t) length < sizeof(buffer)) {
			buffer[length] = '\0';
		}

		count = sscanf((char const *) &buffer, "HSUC_HDAC_FORM0.x = %d", &value);
		if (count == 1) {
			data->hdac[0] = (unsigned char) (value & 0xff);
			rc |= 1;
		}

		count = sscanf((char const *) &buffer, "HSUC_HDAC_FORM0.y = %d", &value);
		if (count == 1) {
			data->hdac[1] = (unsigned char) (value & 0xff);
			rc |= 1;
		}

		count = sscanf((char const *) &buffer, "HSUC_HDAC_FORM0.z = %d", &value);
		if (count == 1) {
			data->hdac[2] = (unsigned char) (value & 0xff);
			rc |= 1;
		}

		count = sscanf((char const *) &buffer, "HSUC_HO_FORM0.x = %d", &value);
		if (count == 1) {
			data->ho[0] = value;
			rc |= akm8973_magnetic_extrema(data, 0);
		}

		count = sscanf((char const *) &buffer, "HSUC_HO_FORM0.y = %d", &value);
		if (count == 1) {
			data->ho[1] = value;
			rc |= akm8973_magnetic_extrema(data, 1);
		}

		count = sscanf((char const *) &buffer, "HSUC_HO_FORM0.z = %d", &value);
		if (count == 1) {
			data->ho[2] = value;
			rc |= akm8973_magnetic_extrema(data, 2);
		}
	} while (p != NULL && length > 0);

	goto complete;

error:
	rc = -1;

complete:
	if (config_fd >= 0)
		close(config_fd);

	return rc;
}

int akm8973_config_write(struct akm8973_data *data)
{
	char buffer[256] = { 0 };
	int config_fd = -1;
	int length;
	int value;
	int rc;

	if (data == NULL)
		return -EINVAL;

	config_fd = open(AKM8973_CONFIG_PATH, O_WRONLY | O_TRUNC | O_CREAT, 0664);
	if (config_fd < 0) {
		ALOGE("%s: Unable to open akm8973 config", __func__);
		goto error;
	}

	value = (int) data->hdac[0];
	length = snprintf((char *) &buffer, sizeof(buffer), "HSUC_HDAC_FORM0.x = %d\n", value);

	rc = write(config_fd, buffer, length);
	if (rc < length) {
		ALOGE("%s: Unable to write akm8973 config", __func__);
		goto error;
	}

	value = (int) data->hdac[1];
	length = snprintf((char *) &buffer, sizeof(buffer), "HSUC_HDAC_FORM0.y = %d\n", value);

	rc = write(config_fd, buffer, length);
	if (rc < length) {
		ALOGE("%s: Unable to write akm8973 config", __func__);
		goto error;
	}

	value = (int) data->hdac[2];
	length = snprintf((char *) &buffer, sizeof(buffer), "HSUC_HDAC_FORM0.z = %d\n", value);

	rc = write(config_fd, buffer, length);
	if (rc < length) {
		ALOGE("%s: Unable to write akm8973 config", __func__);
		goto error;
	}

	value = (int) data->ho[0];
	length = snprintf((char *) &buffer, sizeof(buffer), "HSUC_HO_FORM0.x = %d\n", value);

	rc = write(config_fd, buffer, length);
	if (rc < length) {
		ALOGE("%s: Unable to write akm8973 config", __func__);
		goto error;
	}

	value = (int) data->ho[1];
	length = snprintf((char *) &buffer, sizeof(buffer), "HSUC_HO_FORM0.y = %d\n", value);

	rc = write(config_fd, buffer, length);
	if (rc < length) {
		ALOGE("%s: Unable to write akm8973 config", __func__);
		goto error;
	}

	value = (int) data->ho[2];
	length = snprintf((char *) &buffer, sizeof(buffer), "HSUC_HO_FORM0.z = %d\n", value);

	rc = write(config_fd, buffer, length);
	if (rc < length) {
		ALOGE("%s: Unable to write akm8973 config", __func__);
		goto error;
	}

	rc = 0;
	goto complete;

error:
	rc = -1;

complete:
	if (config_fd >= 0)
		close(config_fd);

	return rc;
}

int akm8973_hdac_calibration(struct akm8973_data *data,
	unsigned char *magnetic_data, size_t magnetic_data_size)
{
	unsigned char value;
	int update;
	int rc;
	int i;

	if (data == NULL || magnetic_data == NULL || magnetic_data_size < 3)
		return -EINVAL;

	update = 0;

	for (i = 0; i < 3; i++) {
		// Transform non-linear HDAC to a linear value
		if (data->hdac[i] == 0)
			value = 0x80;
		else if (data->hdac[i] < 0x80)
			value = 0x80 - data->hdac[i];
		else
			value = data->hdac[i];

		// Adjust the (linear) HDAC offset if the value is out of range.
		// The correct range is [50;205] (in raw magnetic data).

		if (magnetic_data[i] < 50 ) {
			if (value > (0xff - 4))
				continue;

			if (magnetic_data[i] < 10)
				value += 4;
			else if (magnetic_data[i] < 20)
				value += 3;
			else if (magnetic_data[i] < 30)
				value += 2;
			else
				value += 1;

			update = 1;
		}

		if (magnetic_data[i] > 205) {
			if (value < (0x00 + 4))
				continue;

			if (magnetic_data[i] > 245)
				value -= 4;
			else if (magnetic_data[i] > 235)
				value -= 3;
			else if (magnetic_data[i] > 225)
				value -= 2;
			else
				value -= 1;

			update = 1;
		}

		if (update) {
			// When the HDAC (hardware offset) value is changed, the HO value and magnetic extrema become irrelevant.
			// We can calculate HO (software offset) from HDAC but it'll need to be finely tuned later on.

			data->magnetic_extrema[0][i] = 0;
			data->magnetic_extrema[1][i] = 0;

			// Transform linear value to non-linear HDAC
			if (value == 0x80) {
				data->hdac[i] = 0;
				data->ho[i] = 0;
			} else if (value < 0x80) {
				data->hdac[i] = 0x80 - value;
				data->ho[i] = data->hdac[i] * -1 * 16 * 16;
			} else {
				data->hdac[i] = value;
				data->ho[i] = (data->hdac[i] - 128) * 16 * 16;
			}
		}
	}

	if (update) {
		rc = akm8973_hdac(data);
		if (rc < 0) {
			ALOGE("%s: Unable to set akm8973 HDAC", __func__);
			return -1;
		}
	}

	return 0;
}

int akm8973_ho_calibration(struct akm8973_data *data,
	unsigned char *magnetic_data, size_t magnetic_data_size)
{
	float ho[2];
	int gain_index;
	int i;

	if (data == NULL || magnetic_data == NULL || magnetic_data_size < 3)
		return -EINVAL;

	// Update the extrema from the current raw magnetic data
	for (i = 0; i < 3; i++) {
		if (magnetic_data[i] < data->magnetic_extrema[0][i] || data->magnetic_extrema[0][i] == 0)
			data->magnetic_extrema[0][i] = magnetic_data[i];
		if (magnetic_data[i] > data->magnetic_extrema[1][i] || data->magnetic_extrema[1][i] == 0)
			data->magnetic_extrema[1][i] = magnetic_data[i];
	}

	// Calculate HO (software offset)
	if (data->magnetic_data_count % 10 == 0) {
		for (i = 0; i < 3; i++) {
			gain_index = data->gain_indexes[i];
			if (gain_index < 0 || gain_index >= akm8973_gain_coefficient_count)
				continue;

			// Calculate offset for minimum to be at -45uT
			ho[0] = ((float) (data->magnetic_extrema[0][i] - 0x80) + (16.0f * 45.0f) / akm8973_gain_coefficient[gain_index] ) * 16.0f;
			// Calculate offset for maximum to be at +45uT
			ho[1] = ((float) (data->magnetic_extrema[1][i] - 0x80) - (16.0f * 45.0f) / akm8973_gain_coefficient[gain_index] ) * 16.0f;
			// Average offset to make everyone (mostly) happy
			data->ho[i] = (int) (ho[0] + ho[1]) / 2.0f;
		}
	}

	return 0;
}

int akm8973_magnetic_axis(struct akm8973_data *data, int index, float *axis)
{
	float value;
	int count;
	int gain_index;
	int i;

	if (data == NULL || axis == NULL || index < 0 || index >= 3)
		return -EINVAL;

	count = data->magnetic_data_count >= 4 ? 4 : data->magnetic_data_count;
	value = 0;

	// Average the last 4 (or less) raw magnetic values
	for (i = 0; i < count; i++)
		value += (float) data->magnetic_data[i][index];
	value /= count;

	gain_index = data->gain_indexes[index];
	if (gain_index < 0 || gain_index >= akm8973_gain_coefficient_count)
		return -1;

	// Formula to get the magnetic field in uT from the raw magnetic value, HO and coefficient from gain
	*axis = ((value - 128 - ((float) data->ho[index] / 16.0f)) * akm8973_gain_coefficient[gain_index]) / 16.0f;

	return 0;
}

int akm8973_magnetic(struct akm8973_data *data)
{
	int rc;

	if (data == NULL)
		return -EINVAL;

	rc = 0;
	rc |= akm8973_magnetic_axis(data, 0, &data->magnetic.x);
	rc |= akm8973_magnetic_axis(data, 1, &data->magnetic.y);
	rc |= akm8973_magnetic_axis(data, 2, &data->magnetic.z);

	return rc;
}

void *akm8973_thread(void *thread_data)
{
	struct wave_sensors_handlers *handlers = NULL;
	struct akm8973_data *data = NULL;
	struct input_event event;
	struct timeval time;
	char i2c_data[SENSOR_DATA_SIZE] = { 0 };
	unsigned char magnetic_data[3] = { 0 };
	int index;
	int value;
	short mode;
	long int before, after;
	int diff;
	int device_fd;
	int uinput_fd;
	int rc;
	int i, j;

	if (thread_data == NULL)
		return NULL;

	handlers = (struct wave_sensors_handlers *) thread_data;
	if (handlers->data == NULL)
		return NULL;

	data = (struct akm8973_data *) handlers->data;

	device_fd = data->device_fd;
	if (device_fd < 0)
		return NULL;

	uinput_fd = data->uinput_fd;
	if (uinput_fd < 0)
		return NULL;

	while (data->thread_continue) {
		pthread_mutex_lock(&data->mutex);
		if (!data->thread_continue)
			break;

		while (handlers->activated) {
			gettimeofday(&time, NULL);
			before = timestamp(&time);

			mode = AK8973_MODE_MEASURE;
			rc = ioctl(device_fd, ECS_IOCTL_SET_MODE, &mode);
			if (rc < 0) {
				ALOGE("%s: Unable to set akm8973 mode", __func__);
				goto next;
			}

			memset(&i2c_data, 0, sizeof(i2c_data));
			rc = ioctl(device_fd, ECS_IOCTL_GETDATA, &i2c_data);
			if (rc < 0) {
				ALOGE("%s: Unable to get akm8973 data", __func__);
				goto next;
			}

			if (!(i2c_data[0] & 0x01)) {
				ALOGE("%s: akm8973 data is not ready", __func__);
				goto next;
			}

			magnetic_data[0] = (unsigned char) i2c_data[2];
			magnetic_data[1] = (unsigned char) i2c_data[3];
			magnetic_data[2] = (unsigned char) i2c_data[4];

			rc = akm8973_hdac_calibration(data, (unsigned char *) &magnetic_data, sizeof(magnetic_data));
			if (rc < 0) {
				ALOGE("%s: Unable to calibrate akm8973 HDAC", __func__);
				goto next;
			}

			index = data->magnetic_data_index;

			data->magnetic_data[index][0] = magnetic_data[0];
			data->magnetic_data[index][1] = magnetic_data[1];
			data->magnetic_data[index][2] = magnetic_data[2];

			data->magnetic_data_index = (index + 1) % 4;
			data->magnetic_data_count++;

			rc = akm8973_ho_calibration(data, (unsigned char *) &magnetic_data, sizeof(magnetic_data));
			if (rc < 0) {
				ALOGE("%s: Unable to calibrate akm8973 HO", __func__);
				goto next;
			}

			rc = akm8973_magnetic(data);
			if (rc < 0) {
				ALOGE("%s: Unable to get akm8973 magnetic", __func__);
				goto next;
			}

			input_event_set(&event, EV_REL, REL_X, (int) (data->magnetic.x * 1000));
			write(uinput_fd, &event, sizeof(event));
			input_event_set(&event, EV_REL, REL_Y, (int) (data->magnetic.y * 1000));
			write(uinput_fd, &event, sizeof(event));
			input_event_set(&event, EV_REL, REL_Z, (int) (data->magnetic.z * 1000));
			write(uinput_fd, &event, sizeof(event));
			input_event_set(&event, EV_SYN, 0, 0);
			write(uinput_fd, &event, sizeof(event));

next:
			gettimeofday(&time, NULL);
			after = timestamp(&time);

			diff = (int) (data->delay - (after - before)) / 1000;
			if (diff <= 0)
				continue;

			usleep(diff);
		}
	}
	return NULL;
}

int akm8973_init(struct wave_sensors_handlers *handlers,
	struct wave_sensors_device *device)
{
	struct akm8973_data *data = NULL;
	pthread_attr_t thread_attr;
	char i2c_data[RWBUF_SIZE] = { 0 };
	char mode;
	int device_fd = -1;
	int uinput_fd = -1;
	int input_fd = -1;
	int rc;
	int i;

	ALOGD("%s(%p, %p)", __func__, handlers, device);

	if (handlers == NULL || device == NULL)
		return -EINVAL;

	data = (struct akm8973_data *) calloc(1, sizeof(struct akm8973_data));

	for (i = 0; i < device->handlers_count; i++) {
		if (device->handlers[i] == NULL)
			continue;

		if (device->handlers[i]->handle == SENSOR_TYPE_ORIENTATION)
			data->orientation_sensor = device->handlers[i];
	}

	device_fd = open("/dev/akm8973", O_RDONLY);
	if (device_fd < 0) {
		ALOGE("%s: Unable to open device", __func__);
		goto error;
	}

	rc = ioctl(device_fd, ECS_IOCTL_RESET, NULL);
	if (rc < 0) {
		ALOGE("%s: Unable to reset akm8973", __func__);
		goto error;
	}

	mode = AK8973_MODE_E2P_READ;
	rc = ioctl(device_fd, ECS_IOCTL_SET_MODE, &mode);
	if (rc < 0) {
		ALOGE("%s: Unable to set akm8973 mode", __func__);
		goto error;
	}

	i2c_data[0] = 3;
	i2c_data[1] = AK8973_EEP_EHXGA;
	rc = ioctl(device_fd, ECS_IOCTL_READ, &i2c_data);
	if (rc < 0) {
		ALOGE("%s: Unable to read akm8973 EEPROM data", __func__);
		goto error;
	}

	data->gain_indexes[0] = (i2c_data[1] & 0xf0) >> 4;
	data->gain_indexes[1] = (i2c_data[2] & 0xf0) >> 4;
	data->gain_indexes[2] = (i2c_data[3] & 0xf0) >> 4;

	ALOGD("AKM8973 gain indexes are: (%d, %d, %d)", data->gain_indexes[0],
		data->gain_indexes[1], data->gain_indexes[2]);

	mode = AK8973_MODE_POWERDOWN;
	rc = ioctl(device_fd, ECS_IOCTL_SET_MODE, &mode);
	if (rc < 0) {
		ALOGE("%s: Unable to set akm8973 mode", __func__);
		goto error;
	}

	i2c_data[5] = i2c_data[1] & 0x0f;
	i2c_data[6] = i2c_data[2] & 0x0f;
	i2c_data[7] = i2c_data[3] & 0x0f;

	i2c_data[0] = 7;
	i2c_data[1] = AK8973_REG_HXDA;

	i2c_data[2] = 0;
	i2c_data[3] = 0;
	i2c_data[4] = 0;

	rc = ioctl(device_fd, ECS_IOCTL_WRITE, &i2c_data);
	if (rc < 0) {
		ALOGE("%s: Unable to write akm8973 data", __func__);
		goto error;
	}

	uinput_fd = uinput_rel_create("magnetic");
	if (uinput_fd < 0) {
		ALOGD("%s: Unable to create uinput", __func__);
		goto error;
	}

	input_fd = input_open("magnetic");
	if (input_fd < 0) {
		ALOGE("%s: Unable to open magnetic input", __func__);
		goto error;
	}

	data->thread_continue = 1;

	pthread_mutex_init(&data->mutex, NULL);
	pthread_mutex_lock(&data->mutex);

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

	rc = pthread_create(&data->thread, &thread_attr, akm8973_thread, (void *) handlers);
	if (rc < 0) {
		ALOGE("%s: Unable to create akm8973 thread", __func__);
		pthread_mutex_destroy(&data->mutex);
		goto error;
	}

	data->device_fd = device_fd;
	data->uinput_fd = uinput_fd;
	handlers->poll_fd = input_fd;
	handlers->data = (void *) data;

	return 0;

error:
	if (data != NULL)
		free(data);

	if (uinput_fd >= 0)
		close(uinput_fd);

	if (input_fd >= 0)
		close(input_fd);

	if (device_fd >= 0)
		close(device_fd);

	handlers->poll_fd = -1;
	handlers->data = NULL;

	return -1;
}

int akm8973_deinit(struct wave_sensors_handlers *handlers)
{
	struct akm8973_data *data = NULL;
	char mode;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct akm8973_data *) handlers->data;

	handlers->activated = 0;
	data->thread_continue = 0;
	pthread_mutex_unlock(&data->mutex);

	pthread_mutex_destroy(&data->mutex);

	if (data->uinput_fd >= 0) {
		uinput_destroy(data->uinput_fd);
		close(data->uinput_fd);
	}
	data->uinput_fd = -1;

	if (handlers->poll_fd >= 0)
		close(handlers->poll_fd);
	handlers->poll_fd = -1;

	mode = AK8973_MODE_POWERDOWN;
	rc = ioctl(data->device_fd, ECS_IOCTL_SET_MODE, &mode);
	if (rc < 0)
		ALOGE("%s: Unable to set akm8973 mode", __func__);

	if (data->device_fd >= 0)
		close(data->device_fd);
	data->device_fd = -1;

	free(handlers->data);
	handlers->data = NULL;

	return 0;
}

int akm8973_activate(struct wave_sensors_handlers *handlers)
{
	struct akm8973_data *data;
	int rc;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct akm8973_data *) handlers->data;

	rc = akm8973_config_read(data);
	if (rc < 0) {
		ALOGE("%s: Unable to read akm8973 config", __func__);
	} else if (rc > 0) {
		rc = akm8973_hdac(data);
		if (rc < 0) {
			ALOGE("%s: Unable to set akm8973 HDAC", __func__);
			return -1;
		}
	}

	handlers->activated = 1;
	pthread_mutex_unlock(&data->mutex);

	return 0;
}

int akm8973_deactivate(struct wave_sensors_handlers *handlers)
{
	struct akm8973_data *data;
	int device_fd;
	char mode;
	int empty;
	int rc;
	int i;

	ALOGD("%s(%p)", __func__, handlers);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct akm8973_data *) handlers->data;

	empty = 1;

	for (i = 0; i < (ssize_t) sizeof(data->magnetic_extrema) / 2; i++) {
		if (data->magnetic_extrema[0][i] != 0 || data->magnetic_extrema[1][i] != 0) {
			empty = 0;
			break;
		}
	}

	if (!empty) {
		rc = akm8973_config_write(data);
		if (rc < 0)
			ALOGE("%s: Unable to write akm8973 config", __func__);
	}

	device_fd = data->device_fd;
	if (device_fd < 0)
		return -1;

	mode = AK8973_MODE_POWERDOWN;
	rc = ioctl(data->device_fd, ECS_IOCTL_SET_MODE, &mode);
	if (rc < 0)
		ALOGE("%s: Unable to set akm8973 mode", __func__);

	handlers->activated = 0;

	return 0;
}

int akm8973_set_delay(struct wave_sensors_handlers *handlers, long int delay)
{
	struct akm8973_data *data;

	ALOGD("%s(%p, %ld)", __func__, handlers, delay);

	if (handlers == NULL || handlers->data == NULL)
		return -EINVAL;

	data = (struct akm8973_data *) handlers->data;

	data->delay = delay;

	return 0;
}

float akm8973_convert(int value)
{
	return (float) value / -1000.0f;
}

int akm8973_get_data(struct wave_sensors_handlers *handlers,
	struct sensors_event_t *event)
{
	struct akm8973_data *data;
	struct input_event input_event;
	int input_fd;
	int rc;

//	ALOGD("%s(%p, %p)", __func__, handlers, event);

	if (handlers == NULL || handlers->data == NULL || event == NULL)
		return -EINVAL;

	data = (struct akm8973_data *) handlers->data;

	input_fd = handlers->poll_fd;
	if (input_fd < 0)
		return -1;

	memset(event, 0, sizeof(struct sensors_event_t));
	event->version = sizeof(struct sensors_event_t);
	event->sensor = handlers->handle;
	event->type = handlers->handle;

	event->magnetic.status = SENSOR_STATUS_ACCURACY_MEDIUM;

	do {
		rc = read(input_fd, &input_event, sizeof(input_event));
		if (rc < (int) sizeof(input_event))
			break;

		if (input_event.type == EV_REL) {
			switch (input_event.code) {
				case REL_X:
					event->magnetic.y = akm8973_convert(input_event.value);
					break;
				case REL_Y:
					event->magnetic.x = akm8973_convert(input_event.value * -1);
					break;
				case REL_Z:
					event->magnetic.z = akm8973_convert(input_event.value);
					break;
				default:
					continue;
			}
		} else if (input_event.type == EV_SYN) {
			if (input_event.code == SYN_REPORT)
				event->timestamp = input_timestamp(&input_event);
		}
	} while (input_event.type != EV_SYN);

	if (data->orientation_sensor != NULL)
		orientation_fill(data->orientation_sensor, NULL, &event->magnetic);

	return 0;
}

struct wave_sensors_handlers akm8973 = {
	.name = "AKM8973",
	.handle = SENSOR_TYPE_MAGNETIC_FIELD,
	.init = akm8973_init,
	.deinit = akm8973_deinit,
	.activate = akm8973_activate,
	.deactivate = akm8973_deactivate,
	.set_delay = akm8973_set_delay,
	.get_data = akm8973_get_data,
	.activated = 0,
	.needed = 0,
	.poll_fd = -1,
	.data = NULL,
};

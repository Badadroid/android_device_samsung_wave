/*
 * Copyright (C) 2014 Nikolay Volkov <volk204@mail.ru>
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

#include <fcntl.h>
#include <hardware/gps.h>
#include <secril-client.h>
#include <samsung-ril-socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <pthread.h>

#define  LOG_TAG  "gps_wave"
#include <utils/Log.h>

#define  GPS_DEBUG  1

#if GPS_DEBUG
#  define  D(...)   ALOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

enum {
	STATE_QUIT = 0,
	STATE_INIT = 1,
	STATE_START = 2
};

typedef struct {
	int init;
	GpsCallbacks callbacks;
	GpsStatus status;
	GpsLocation location;
	GpsSvStatus svStatus;

	pthread_mutex_t GpsMutex;
} GpsState;

static GpsState _gps_state[1];

#define GPS_LOCK() pthread_mutex_lock(&_gps_state->GpsMutex)
#define GPS_UNLOCK() pthread_mutex_unlock(&_gps_state->GpsMutex)

void update_gps_location(void* arg) {
	GpsState* state = _gps_state;
	D("%s(): GpsLocation=%f, %f", __FUNCTION__, state->location.latitude, state->location.longitude);

	GPS_LOCK();
	if(state->callbacks.location_cb)
		state->callbacks.location_cb(&state->location);
	GPS_UNLOCK();
}

void update_gps_status(void* arg) {
	GpsState* state = _gps_state;
	D("%s(): GpsStatusValue=%d", __FUNCTION__, state->status.status);

	GPS_LOCK();
	if(state->callbacks.status_cb)
		state->callbacks.status_cb(&state->status);
	GPS_UNLOCK();
}

void update_gps_svstatus(void* arg) {
	GpsState* state = _gps_state;
	D("%s(): GpsSvStatus.num_svs=%d", __FUNCTION__, state->svStatus.num_svs);

	GPS_LOCK();
	if(state->callbacks.sv_status_cb)
		state->callbacks.sv_status_cb(&state->svStatus);
	GPS_UNLOCK();
}

/********************************* RIL interface *********************************/
HRilClient	mRilClient;

int _GpsHandler(int type, void *data)
{
	GpsState* state = _gps_state;
	if(type == SRS_GPS_SV_STATUS) {
		GPS_LOCK();
		memcpy(&state->svStatus, data, sizeof(GpsSvStatus));
		GPS_UNLOCK();
		if(state->callbacks.create_thread_cb)
			state->callbacks.create_thread_cb("update_gps_svstatus", update_gps_svstatus, NULL);
	} else if (type == SRS_GPS_LOCATION) {
		GPS_LOCK();
		memcpy(&state->location, data, sizeof(GpsLocation));
		GPS_UNLOCK();
		if(state->callbacks.create_thread_cb)
			state->callbacks.create_thread_cb("update_gps_location", update_gps_location, NULL);
	} else if (type == SRS_GPS_STATE) {
		GPS_LOCK();
		memcpy(&state->status.status, data, sizeof(GpsStatusValue));
		GPS_UNLOCK();
		if(state->callbacks.create_thread_cb)
			state->callbacks.create_thread_cb("update_gps_status", update_gps_status, NULL);
	}
	return 0;
}

int connectRILDIfRequired(void)
{
	if (isConnected_RILD(mRilClient)) {
		return 0;
	}

	if (Connect_RILD(mRilClient) != RIL_CLIENT_ERR_SUCCESS) {
		ALOGE("Connect_RILD() error");
		return -1;
	}

	/* Has to be registered after connecting to let SRS read ping in main thread*/
	RegisterGpsHandler(mRilClient, _GpsHandler);

	if (GpsHello(mRilClient) != RIL_CLIENT_ERR_SUCCESS) {
		ALOGE("GpsHello() error");
		return -1;
	}
    return 0;
}

/********************************* GPS interface *********************************/

static int
wave_gps_init(GpsCallbacks* callbacks)
{
	D("%s() is called", __FUNCTION__);
	/* not yet implemented */

	GpsState* s = _gps_state;

	if (!mRilClient) {
		mRilClient = OpenClient_RILD();
		if (!mRilClient) {
			ALOGE("OpenClient_RILD() error");
		}
	}

	if (!s->init)
	{
		s->callbacks = *callbacks;

		GPS_LOCK();
		s->status.status = GPS_STATUS_ENGINE_ON;
		GPS_UNLOCK();
		if(s->callbacks.create_thread_cb)
			s->callbacks.create_thread_cb("update_gps_status", update_gps_status, NULL);

		s->init = STATE_INIT;
	}
	return 0;
}

static void
wave_gps_cleanup(void)
{
	D("%s() is called", __FUNCTION__);

	GpsState* s = _gps_state;

	if (s->init) {
		GPS_LOCK();
		s->status.status = GPS_STATUS_ENGINE_OFF;
		GPS_UNLOCK();
		if(s->callbacks.create_thread_cb)
			s->callbacks.create_thread_cb("update_gps_status", update_gps_status, NULL);
		s->init = STATE_QUIT;
	}
}

static int
wave_gps_start()
{
	D("%s() is called", __FUNCTION__);

	GpsState* s = _gps_state;

	if (!s->init) {
		D("%s: called with uninitialized state !!", __FUNCTION__);
		return -1;
	}

	if (connectRILDIfRequired() == 0) {
		GpsSetNavigationMode(mRilClient, 1);
		s->init = STATE_START;
	}

	return 0;
}


static int
wave_gps_stop()
{
	D("%s() is called", __FUNCTION__);

	GpsState* s = _gps_state;

	if (!s->init) {
		D("%s: called with uninitialized state !!", __FUNCTION__);
		return -1;
	}

	if (connectRILDIfRequired() == 0) {
		GpsSetNavigationMode(mRilClient, 0);
		s->init = STATE_INIT;
	}

	return 0;
}


static int
wave_gps_inject_time(GpsUtcTime time, int64_t timeReference, int uncertainty)
{
	D("%s() is called", __FUNCTION__);
	D("time=%lld, timeReference=%lld, uncertainty=%d", time, timeReference, uncertainty);
	/* not yet implemented */
	return 0;
}

static int
wave_gps_inject_location(double latitude, double longitude, float accuracy)
{
	D("%s() is called", __FUNCTION__);
	D("latitude=%f, longitude=%f, accuracy=%f", latitude, longitude, accuracy);
	return 0;
	/* not yet implemented */
}

static void
wave_gps_delete_aiding_data(GpsAidingData flags)
{
	D("%s() is called", __FUNCTION__);
	D("flags=%d", flags);
	/* not yet implemented */
}

static int wave_gps_set_position_mode(GpsPositionMode mode, GpsPositionRecurrence recurrence,
            uint32_t min_interval, uint32_t preferred_accuracy, uint32_t preferred_time)
{
	D("%s() is called", __FUNCTION__);
	D("GpsPositionMode=%d, GpsPositionRecurrence=%d, min_interval=%d, preferred_accuracy=%d, preferred_time=%d,", mode, recurrence, min_interval, preferred_accuracy, preferred_time);
	/* not yet implemented */
	return 0;
}

static const void*
wave_gps_get_extension(const char* name)
{
	D("%s('%s') is called", __FUNCTION__, name);
	/* not yet implemented */
	return NULL;
}

static const GpsInterface  waveGpsInterface = {
    sizeof(GpsInterface),
    wave_gps_init,
    wave_gps_start,
    wave_gps_stop,
    wave_gps_cleanup,
    wave_gps_inject_time,
    wave_gps_inject_location,
    wave_gps_delete_aiding_data,
    wave_gps_set_position_mode,
    wave_gps_get_extension,
};

const GpsInterface* gps__get_gps_interface(struct gps_device_t* dev)
{
    return &waveGpsInterface;
}

static int open_gps(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    struct gps_device_t *dev = malloc(sizeof(struct gps_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->get_gps_interface = gps__get_gps_interface;

    *device = (struct hw_device_t*)dev;

	pthread_mutex_init(&_gps_state->GpsMutex, NULL);
    return 0;
}


static struct hw_module_methods_t gps_module_methods = {
    .open = open_gps
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = GPS_HARDWARE_MODULE_ID,
    .name = "Wave GPS Module",
    .author = "Nikolay Volkov <volk204@mail.ru>",
    .methods = &gps_module_methods,
};

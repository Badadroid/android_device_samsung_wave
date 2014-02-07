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
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#define  LOG_TAG  "gps_wave"
#include <utils/Log.h>

#define  GPS_DEBUG  1

#if GPS_DEBUG
#  define  D(...)   ALOGD(__VA_ARGS__)
#else
#  define  D(...)   ((void)0)
#endif

/********************************* RIL interface *********************************/

void*		mSecRilLibHandle;
HRilClient	mRilClient;
HRilClient	(*openClientRILD)	(void);
int		(*disconnectRILD)	(HRilClient);
int		(*closeClientRILD)	(HRilClient);
int		(*isConnectedRILD)	(HRilClient);
int		(*connectRILD)		(HRilClient);
int		(*gpsNavigation)	(HRilClient, int);
void		loadRILD(void);
int		connectRILDIfRequired(void);

void loadRILD(void)
{
	mSecRilLibHandle = dlopen("libril-client.so", RTLD_NOW);

	if (mSecRilLibHandle) {
		ALOGV("libril-client.so is loaded");

		openClientRILD   = (HRilClient (*)(void))
					dlsym(mSecRilLibHandle, "OpenClient_RILD");
		disconnectRILD   = (int (*)(HRilClient))
					dlsym(mSecRilLibHandle, "Disconnect_RILD");
		closeClientRILD  = (int (*)(HRilClient))
					dlsym(mSecRilLibHandle, "CloseClient_RILD");
		isConnectedRILD  = (int (*)(HRilClient))
					dlsym(mSecRilLibHandle, "isConnected_RILD");
		connectRILD      = (int (*)(HRilClient))
					dlsym(mSecRilLibHandle, "Connect_RILD");
		gpsNavigation    = (int (*)(HRilClient, int))
					dlsym(mSecRilLibHandle, "GPSNavigation");

		if (!openClientRILD  || !disconnectRILD   || !closeClientRILD ||
				!isConnectedRILD || !connectRILD || !gpsNavigation ) {
			ALOGE("Can't load all functions from libril-client.so");

			dlclose(mSecRilLibHandle);
			mSecRilLibHandle = NULL;
		} else {
			mRilClient = openClientRILD();
			if (!mRilClient) {
				ALOGE("OpenClient_RILD() error");

			dlclose(mSecRilLibHandle);
			mSecRilLibHandle = NULL;
			}
		}
	} else {
		ALOGE("Can't load libril-client.so");
	}
}

int connectRILDIfRequired(void)
{
	if (!mSecRilLibHandle) {
		ALOGE("connectIfRequired() lib is not loaded");
		return -1;
	}

	if (isConnectedRILD(mRilClient)) {
		return 0;
	}

	if (connectRILD(mRilClient) != RIL_CLIENT_ERR_SUCCESS) {
		ALOGE("Connect_RILD() error");
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

       if (!mSecRilLibHandle)
		loadRILD();

	return 0;
}

static void
wave_gps_cleanup(void)
{
	D("%s() is called", __FUNCTION__);
	/* not yet implemented */
}


static int
wave_gps_start()
{
	D("%s() is called", __FUNCTION__);
	/* not yet implemented */

	if ((mSecRilLibHandle) && (connectRILDIfRequired() == 0))
		gpsNavigation(mRilClient, 1);

	return 0;
}


static int
wave_gps_stop()
{
	D("%s() is called", __FUNCTION__);
	/* not yet implemented */

	if ((mSecRilLibHandle) && (connectRILDIfRequired() == 0))
		gpsNavigation(mRilClient, 0);

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

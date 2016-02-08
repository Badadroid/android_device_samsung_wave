/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __SECRIL_CLIENT_H__
#define __SECRIL_CLIENT_H__

#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

struct RilClient {
    int fd;
    int type;
};

typedef struct RilClient * HRilClient;


//---------------------------------------------------------------------------
// Defines
//---------------------------------------------------------------------------
#define RIL_CLIENT_ERR_SUCCESS      0
#define RIL_CLIENT_ERR_AGAIN        1
#define RIL_CLIENT_ERR_INIT         2 // Client is not initialized
#define RIL_CLIENT_ERR_INVAL        3 // Invalid value
#define RIL_CLIENT_ERR_CONNECT      4 // Connection error
#define RIL_CLIENT_ERR_IO           5 // IO error
#define RIL_CLIENT_ERR_RESOURCE     6 // Resource not available
#define RIL_CLIENT_ERR_UNKNOWN      7


//---------------------------------------------------------------------------
// Type definitions
//---------------------------------------------------------------------------

typedef int (*RilOnComplete)(HRilClient handle, const void *data, size_t datalen);

typedef int (*RilOnUnsolicited)(HRilClient handle, const void *data, size_t datalen);

typedef int (*RilOnError)(void *data, int error);

typedef int (*GpsHandler)(int type, void *data);


//---------------------------------------------------------------------------
// Client APIs
//---------------------------------------------------------------------------

/**
 * Open RILD multi-client.
 * Return is client handle, NULL on error.
 */
HRilClient OpenClient_RILD(void);

/**
 * Stop RILD multi-client. If client socket was connected,
 * it will be disconnected.
 */
int CloseClient_RILD(HRilClient client);

/**
 * Connect to RIL deamon. One client task starts.
 * Return is 0 or error code.
 */
int Connect_RILD(HRilClient client);

/**
 * check whether RILD is connected
 * Returns 0 or 1
 */
int isConnected_RILD(HRilClient client);

/**
 * Disconnect connection to RIL deamon(socket close).
 * Return is 0 or error code.
 */
int Disconnect_RILD(HRilClient client);

int RegisterGpsHandler(HRilClient client, GpsHandler handler);

/**
 * Register unsolicited response handler. If handler is NULL,
 * the handler for the request ID is unregistered.
 * The response handler is invoked in the client task context.
 * Return is 0 or error code.
 */
int RegisterUnsolicitedHandler(HRilClient client, uint32_t id, RilOnUnsolicited handler);

/**
 * Register solicited response handler. If handler is NULL,
 * the handler for the ID is unregistered.
 * The response handler is invoked in the client task context.
 * Return is 0 or error code.
 */
int RegisterRequestCompleteHandler(HRilClient client, uint32_t id, RilOnComplete handler);

/**
 * Register error callback. If handler is NULL,
 * the callback is unregistered.
 * The response handler is invoked in the client task context.
 * Return is 0 or error code.
 */
int RegisterErrorCallback(HRilClient client, RilOnError cb, void *data);

/**
 * Invoke OEM request. Request ID is RIL_REQUEST_OEM_HOOK_RAW.
 * Return is 0 or error code. For RIL_CLIENT_ERR_AGAIN caller should retry.
 */
int InvokeOemRequestHookRaw(HRilClient client, char *data, size_t len);


/**
 * Sound device types.
 */
typedef enum _SoundType {
    SOUND_TYPE_VOICE,
    SOUND_TYPE_SPEAKER,
    SOUND_TYPE_HEADSET,
    SOUND_TYPE_BTVOICE,
} SoundType;

/**
 * External sound device path.
 */
typedef enum _AudioPath {
    SOUND_AUDIO_PATH_HANDSET,
    SOUND_AUDIO_PATH_HEADSET,
    SOUND_AUDIO_PATH_SPEAKER,
    SOUND_AUDIO_PATH_BLUETOOTH,
    SOUND_AUDIO_PATH_BLUETOOTH_NO_NR,
    SOUND_AUDIO_PATH_HEADPHONE,	
} AudioPath;

/**
 * Set in-call volume.
 */
int SetVolume(HRilClient client, SoundType type, int vol_level);

/**
 * Set external sound device path for noise reduction.
 */
int SetAudioPath(HRilClient client, AudioPath path);

/**
 * PCM control packet
 */
int PcmIfCtrl(HRilClient client, int enabled);

/**
 * Introduce self as GPS SRS client
 */
int GpsHello(HRilClient data);

/**
 * GPS init
 */
int GpsInit(HRilClient data, int enabled);

/**
 * Set navigation mode
 */
int GpsSetNavigationMode(HRilClient data, int enabled);

/**
 * GPS XTRA Inject Data
 */
int GpsXtraInjectData(HRilClient data, int length);

/**
 * GPS XTRA Inject Time
 */
int GpsXtraInjectTime(HRilClient data, int64_t time, int64_t timeReference, int uncertainty);

/**
 * GPS Delete Data
 */
int GpsDeleteData(HRilClient data);

#ifdef __cplusplus
};
#endif

#endif // __SECRIL_CLIENT_H__

// end of file


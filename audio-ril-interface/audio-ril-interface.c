/*
 * Copyright (C) 2013 Paul Kocialkowski
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

#define LOG_TAG "Audio-RIL-Interface"
#include <cutils/log.h>

#include <secril-client.h>

#include <samsung-ril-socket.h>
#include <srs-client.h>

HRilClient OpenClient_RILD(void)
{
	struct srs_client *client = NULL;

	ALOGE("%s()", __func__);

	signal(SIGPIPE, SIG_IGN);

	srs_client_create(&client);

	return (void *) client;
}

int Connect_RILD(HRilClient data)
{
	struct srs_client *client;
	int rc;

	ALOGE("%s(%p)", __func__, data);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	rc = srs_client_open(client);
	if (rc < 0) {
		ALOGE("%s: Failed to open SRS client", __func__);
		return RIL_CLIENT_ERR_CONNECT;
	}

	rc = srs_client_ping(client);
	if (rc < 0) {
		ALOGE("%s: Failed to ping SRS client", __func__);
		return RIL_CLIENT_ERR_UNKNOWN;
	}

	return RIL_CLIENT_ERR_SUCCESS;
}

int Disconnect_RILD(HRilClient data)
{
	struct srs_client *client;
	int rc;

	ALOGE("%s(%p)", __func__, data);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	rc = srs_client_close(client);
	if (rc < 0) {
		ALOGE("%s: Failed to close SRS client", __func__);
		return RIL_CLIENT_ERR_INVAL;
	}

	return RIL_CLIENT_ERR_SUCCESS;
}

int CloseClient_RILD(HRilClient data)
{
	struct srs_client *client;
	int rc;

	ALOGE("%s(%p)", __func__, data);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	srs_client_destroy(client);

	return RIL_CLIENT_ERR_SUCCESS;
}

int isConnected_RILD(HRilClient data)
{
	struct srs_client *client;
	int rc;

	ALOGE("%s(%p)", __func__, data);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	rc = srs_client_ping(client);
	if (rc < 0) {
		ALOGE("%s: Failed to ping SRS client", __func__);
		return 0;
	}

	return 1;
}

int SetCallVolume(HRilClient data, SoundType type, int level)
{
	struct srs_client *client;
	struct srs_snd_call_volume call_volume;
	int rc;

	ALOGE("%s(%p, %d, %d)", __func__, data, type, level);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	call_volume.type = (enum srs_snd_type) type;
	call_volume.volume = level;

	rc = srs_client_send(client, SRS_SND_SET_CALL_VOLUME, &call_volume, sizeof(call_volume));
	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}


int SetCallAudioPath(HRilClient data, AudioPath path)
{
	struct srs_client *client;
	struct srs_snd_call_audio_path call_audio_path;
	int rc;

	ALOGE("%s(%p, %d)", __func__, data, path);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	call_audio_path.path = path;

	rc = srs_client_send(client, SRS_SND_SET_CALL_AUDIO_PATH, &call_audio_path, sizeof(call_audio_path));
	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}

int SetCallClockSync(HRilClient data, SoundClockCondition condition)
{
	struct srs_client *client;
	struct srs_snd_call_clock_sync call_clock_sync;
	int rc;

	ALOGE("%s(%p, %d)", __func__, data, condition);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	call_clock_sync.sync = condition;

	rc = srs_client_send(client, SRS_SND_SET_CALL_CLOCK_SYNC, &call_clock_sync, sizeof(call_clock_sync));
	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}

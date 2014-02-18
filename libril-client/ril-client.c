/*
 * Copyright (C) 2013 Paul Kocialkowski
 * Copyright (C) 2013 Dominik Marszk
 * Copyright (C) 2013-2014 Nikolay Volkov

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

#define LOG_TAG "LibRIL-Client"
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
GpsHandler _handler;
void srs_client_cb(struct srs_message *message)
{
	if(SRS_GROUP(message->command) == SRS_GPS) {
		if(_handler != NULL) {
			_handler(message->command, message->data);
		}
	}
}

int RegisterGpsHandler(HRilClient data, GpsHandler handler)
{
	struct srs_client *client;
	_handler = handler;
	client = (struct srs_client *) data;
	if(client->thread_run == 0) {
		srs_client_thread_start(client, srs_client_cb);
	}
	return 0;
}
/************************* Audio Interface *************************/

int SetVolume(HRilClient data, SoundType type, int level)
{
	struct srs_client *client;
	struct srs_snd_set_volume_packet volume;
	int rc;

	ALOGE("%s(%p, %d, %d)", __func__, data, type, level);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	switch(type)
	{
		case SOUND_TYPE_VOICE:
			volume.outDevice = SND_OUTPUT_EARPIECE;
			break;
		case SOUND_TYPE_SPEAKER:
			volume.outDevice = SND_OUTPUT_SPEAKER;
			break;
		case SOUND_TYPE_HEADSET:
			volume.outDevice = SND_OUTPUT_HEADSET;
			break;
		case SOUND_TYPE_BTVOICE:
			volume.outDevice = SND_OUTPUT_BLUETOOTH;
			break;
		default:
			ALOGE("%s: type %d not supported", __func__, type);
			return RIL_CLIENT_ERR_UNKNOWN;
			break;
	}

	volume.soundType = SND_TYPE_VOICE;

	volume.volume = level;

	rc = srs_client_send(client, SRS_SND_SET_VOLUME, &volume, sizeof(volume));
	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}


int SetAudioPath(HRilClient data, AudioPath path)
{
	struct srs_client *client;
	struct srs_snd_set_path_packet audio_path;
	struct srs_snd_enable_disable_packet en_pkt;
	int rc;

	ALOGE("%s(%p, %d)", __func__, data, path);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	switch(path)
	{
		case SOUND_AUDIO_PATH_HANDSET:
			audio_path.inDevice = SND_INPUT_MAIN_MIC;
			audio_path.outDevice = SND_OUTPUT_EARPIECE;
			audio_path.soundType = SND_TYPE_VOICE;
			break;
		case SOUND_AUDIO_PATH_SPEAKER:
			audio_path.inDevice = SND_INPUT_MAIN_MIC;
			audio_path.outDevice = SND_OUTPUT_SPEAKER;
			audio_path.soundType = SND_TYPE_VOICE;
			break;
		case SOUND_AUDIO_PATH_HEADSET:
			audio_path.inDevice = SND_INPUT_MAIN_MIC;
			audio_path.outDevice = SND_OUTPUT_HEADSET;
			audio_path.soundType = SND_TYPE_VOICE;
			break;
		case SOUND_AUDIO_PATH_HEADPHONE:
			audio_path.inDevice = SND_INPUT_EAR_MIC;
			audio_path.outDevice = SND_OUTPUT_HEADSET;
			audio_path.soundType = SND_TYPE_VOICE;
			break;
		case SOUND_AUDIO_PATH_BLUETOOTH:
		case SOUND_AUDIO_PATH_BLUETOOTH_NO_NR:
			audio_path.inDevice = SND_INPUT_BT_MIC;
			audio_path.outDevice = SND_OUTPUT_BLUETOOTH;
			audio_path.soundType = SND_TYPE_VOICE;
			break;
		default:
			ALOGE("%s: path %d not supported", __func__, path);
			return RIL_CLIENT_ERR_UNKNOWN;
			break;
	}

	client = (struct srs_client *) data;

	rc = srs_client_send(client, SRS_SND_SET_AUDIO_PATH, &audio_path, sizeof(audio_path));
	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}

int PcmIfCtrl(HRilClient data, int enabled)
{
	struct srs_client *client;
	struct srs_snd_enable_disable_packet en_pkt;
	int rc;

	ALOGE("%s(%p, %d)", __func__, data, enabled);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	en_pkt.enabled = enabled;

	rc = srs_client_send(client, SRS_SND_PCM_IF_CTRL, &en_pkt, sizeof(en_pkt));

	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}

/************************* GPS Interface *************************/

int GpsHello(HRilClient data)
{
	struct srs_client *client;
	int rc;

	ALOGE("%s(%p)", __func__, data);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	rc = srs_client_send(client, SRS_GPS_HELLO, NULL, 0);

	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}

int GpsSetNavigationMode(HRilClient data, int enabled)
{
	struct srs_client *client;
	struct srs_snd_enable_disable_packet en_pkt;
	int rc;

	ALOGE("%s(%p, %d)", __func__, data, enabled);

	if (data == NULL)
		return RIL_CLIENT_ERR_INVAL;

	client = (struct srs_client *) data;

	en_pkt.enabled = enabled;

	rc = srs_client_send(client, SRS_GPS_NAVIGATION_MODE, &en_pkt, sizeof(en_pkt));

	if (rc < 0)
		return RIL_CLIENT_ERR_UNKNOWN;

	return RIL_CLIENT_ERR_SUCCESS;
}

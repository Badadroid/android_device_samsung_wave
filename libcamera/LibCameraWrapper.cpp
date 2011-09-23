/*
 * Copyright (C) 2011 The Android Open Source Project
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

// Adapted from JordanCameraWrapper

// We can't use CameraParameters constants because we're linking against
// Samsung's libcamera_client.so

#define LOG_TAG "LibCameraWrapper"
//#define LOG_NDEBUG 0

#include <cmath>
#include <dlfcn.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <camera/Camera.h>
#include "LibCameraWrapper.h"

namespace android {

typedef sp<CameraHardwareInterface> (*OpenCamFunc)(int);
typedef void (*GetCamInfo)(int, struct CameraInfo*);

static void * g_libHandle = NULL;
static OpenCamFunc g_openCameraHardware = NULL;
static GetCamInfo g_getCameraInfo = NULL;

static const int CAMERA_CMD_SET_OBJECT_TRACKING_POSITION = 1103;
static const int CAMERA_CMD_SET_TOUCH_AF = 1105;

static void ensureLibOpened()
{
    if (g_libHandle == NULL) {
        g_libHandle = ::dlopen("libsamsungcamera.so", RTLD_NOW);
        if (g_libHandle == NULL) {
            assert(0);
            LOGE("dlopen() error: %s\n", dlerror());
        } else {
            g_openCameraHardware = (OpenCamFunc) ::dlsym(g_libHandle, "HAL_openCameraHardware");
            g_getCameraInfo = (GetCamInfo) ::dlsym(g_libHandle, "HAL_getCameraInfo");
            assert(g_openCameraHardware != NULL);
        }
    }
}

extern "C" int HAL_getNumberOfCameras()
{
#ifdef FFC_PRESENT
    return 2;
#else
    return 1;
#endif
}

extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo* cameraInfo)
{
    ensureLibOpened();
    g_getCameraInfo(cameraId, cameraInfo);
}

extern "C" sp<CameraHardwareInterface> HAL_openCameraHardware(int cameraId)
{
    LOGV("openCameraHardware: call createInstance");
    ensureLibOpened();
    return LibCameraWrapper::createInstance(cameraId);
}

wp<CameraHardwareInterface> LibCameraWrapper::singleton[2] = { 0 };

sp<CameraHardwareInterface> LibCameraWrapper::createInstance(int cameraId)
{
    LOGV("%s :", __func__);
    if (singleton[cameraId] != NULL) {
        sp<CameraHardwareInterface> hardware = singleton[cameraId].promote();
        if (hardware != NULL) {
            return hardware;
        }
    }

    ensureLibOpened();

    sp<CameraHardwareInterface> hardware(new LibCameraWrapper(cameraId));
    singleton[cameraId] = hardware;
    return hardware;
}

LibCameraWrapper::LibCameraWrapper(int cameraId) :
    mLibInterface(g_openCameraHardware(cameraId)),
    mCameraId(cameraId),
    mVideoMode(false),
    mContinuousAf(false),
    mFixFocus(false),
    mTouchFocus(false)
{
    LOGV("%s :", __func__);
}

LibCameraWrapper::~LibCameraWrapper()
{
    LOGV("%s :", __func__);
}

sp<IMemoryHeap>
LibCameraWrapper::getPreviewHeap() const
{
    LOGV("%s :", __func__);
    return mLibInterface->getPreviewHeap();
}

sp<IMemoryHeap>
LibCameraWrapper::getRawHeap() const
{
    LOGV("%s :", __func__);
    return mLibInterface->getRawHeap();
}

void
LibCameraWrapper::setCallbacks(notify_callback notify_cb,
                                  data_callback data_cb,
                                  data_callback_timestamp data_cb_timestamp,
                                  void* user)
{
    LOGV("%s :", __func__);
    mLibInterface->setCallbacks(notify_cb, data_cb, data_cb_timestamp, user);
}

void
LibCameraWrapper::enableMsgType(int32_t msgType)
{
    LOGV("%s :", __func__);
    mLibInterface->enableMsgType(msgType);
}

void
LibCameraWrapper::disableMsgType(int32_t msgType)
{
    LOGV("%s :", __func__);
    mLibInterface->disableMsgType(msgType);
}

bool
LibCameraWrapper::msgTypeEnabled(int32_t msgType)
{
    LOGV("%s :", __func__);
    return mLibInterface->msgTypeEnabled(msgType);
}

status_t
LibCameraWrapper::startPreview()
{
    LOGV("%s :", __func__);
    status_t ret = mLibInterface->startPreview();

    if (mFixFocus) {
        LOGV("Fix focus mode");
        // We need to switch the focus mode once after switching from video or the camera won't work.
        // Note: If the previous mode was macro, then it actually doesn't matter since the bug doesn't affect that case.
        CameraParameters pars = mLibInterface->getParameters();
        const char *prevFocusMode = pars.get("focus-mode");
        pars.set("focus-mode", "macro");
        mLibInterface->setParameters(pars);
        pars.set("focus-mode", prevFocusMode);
        mLibInterface->setParameters(pars);
        mFixFocus = false;
    }

    return ret;
}

bool
LibCameraWrapper::useOverlay()
{
    LOGV("%s :", __func__);
    return mLibInterface->useOverlay();
}

status_t
LibCameraWrapper::setOverlay(const sp<Overlay> &overlay)
{
    LOGV("%s :", __func__);
    return mLibInterface->setOverlay(overlay);
}

void
LibCameraWrapper::stopPreview()
{
    LOGV("%s :", __func__);
    mLibInterface->stopPreview();
}

bool
LibCameraWrapper::previewEnabled()
{
    LOGV("%s :", __func__);
    return mLibInterface->previewEnabled();
}

status_t
LibCameraWrapper::startRecording()
{
    LOGV("%s :", __func__);
    return mLibInterface->startRecording();
}

void
LibCameraWrapper::stopRecording()
{
    LOGV("%s :", __func__);
    mLibInterface->stopRecording();
}

bool
LibCameraWrapper::recordingEnabled()
{
    LOGV("%s :", __func__);
    return mLibInterface->recordingEnabled();
}

void
LibCameraWrapper::releaseRecordingFrame(const sp<IMemory>& mem)
{
    LOGV("%s :", __func__);
    return mLibInterface->releaseRecordingFrame(mem);
}

status_t
LibCameraWrapper::autoFocus()
{
    LOGV("%s :", __func__);
    return mLibInterface->autoFocus();
}

status_t
LibCameraWrapper::cancelAutoFocus()
{
    LOGV("%s :", __func__);
    return mLibInterface->cancelAutoFocus();
}

status_t
LibCameraWrapper::takePicture()
{
    LOGV("%s :", __func__);
    return mLibInterface->takePicture();
}

status_t
LibCameraWrapper::cancelPicture()
{
    LOGV("%s :", __func__);
    return mLibInterface->cancelPicture();
}

status_t
LibCameraWrapper::setParameters(const CameraParameters& params)
{
    LOGV("%s :", __func__);
    CameraParameters pars(params.flatten());

    if (mCameraId == 0) {
        const char *metering;
        const char *conAf;
        const char *touchCoordinate;
        bool prevContinuousAf;

        /*
         * getInt returns -1 if the value isn't present and 0 on parse failure,
         * so if it's larger than 0, we can be sure the value was parsed properly
         */
        mVideoMode = pars.getInt("cam-mode") > 0;
        pars.remove("cam-mode");

        if (mVideoMode) {
            // Special settings in video mode
            pars.set("video_recording_gamma", "on");
            pars.set("slow_ae", "on");
            pars.set("iso", "movie");
            pars.set("metering", "matrix");
        }
        else {
            pars.set("video_recording_gamma", "off");
            pars.set("slow_ae", "off");
        }

        // Parse continuous autofocus into a format the driver understands
        conAf = pars.get("enable-caf");
        prevContinuousAf = mContinuousAf;
        mContinuousAf = (conAf != 0 && strcmp(conAf, "on") == 0);
        pars.set("continuous_af", mContinuousAf ? 1 : 0);

        if (prevContinuousAf && !mContinuousAf) {
            mFixFocus = true;
        }

        // Always set antibanding to 50hz
        pars.set("antibanding", "50hz");

        // Parse metering into something the driver understands
        metering = pars.get("meter-mode");
        if (metering != 0) {
            if (strcmp(metering, "meter-center") == 0) {
                pars.set("metering", "center");
            }
            else if (strcmp(metering, "meter-spot") == 0) {
                pars.set("metering", "spot");
            }
            else if (strcmp(metering, "meter-matrix") == 0) {
                pars.set("metering", "matrix");
            }
            pars.remove("meter-mode");
        }

        // Read touch-to-focus
        touchCoordinate = pars.get("touch-focus");
        if (touchCoordinate != 0) {
            int width, height;
            int x, y;
            char *comma;
            x = mTouchFocusX = strtol(touchCoordinate, &comma, 10);
            y = mTouchFocusY = strtol(comma + 1, NULL, 10);

            pars.getPreviewSize(&width, &height);
            if (fabs((float)width/height - 1.66) > 0.1) {
                LOGV("Non-widescreen touch focus");
                x += 80; // Only aries' Camera needs this for non-widescreen
            }

            sendCommand(CAMERA_CMD_SET_TOUCH_AF, 0, 0);
            sendCommand(CAMERA_CMD_SET_OBJECT_TRACKING_POSITION, x, y);
            sendCommand(CAMERA_CMD_SET_TOUCH_AF, 1, 0);

            mTouchFocus = true;
            pars.remove("touch-focus");
        }
        else if (mTouchFocus) {
            LOGV("Disabling touch focus");
            sendCommand(CAMERA_CMD_SET_TOUCH_AF, 0, 0);
            mTouchFocus = false;
        }

    }

    return mLibInterface->setParameters(pars);
}

CameraParameters
LibCameraWrapper::getParameters() const
{
    LOGV("%s :", __func__);
    CameraParameters ret = mLibInterface->getParameters();

    if (mCameraId == 0) {
        // The only value of antibanding supported is 50hz. Let's not say we support anything
        ret.remove("antibanding-values");

        // We support facedetect as well
        ret.set("focus-mode-values", "auto,macro,facedetect");

        // Auto-exposure modes. NOTE: matrix isn't a value supported in stock android
        ret.set("meter-mode-values", "meter-center,meter-spot,meter-matrix");

        // ISO values. The driver fails to return any of this.
        ret.set("iso-values", "auto,50,100,200,400,800,1600,3200,sports,night,movie");

        // Scene modes. The driver fails to return its proper supported values.
        ret.set("scene-mode-values", "auto,portrait,landscape,sports,sunset,dusk-dawn,fireworks,beach,party,night,fall-color,text,candlelight,back-light");

        // This is for detecting if we're in camcorder mode or not
        ret.set("cam-mode", mVideoMode ? "1" : "0");

        // Continuous AF
        ret.set("enable-caf", mContinuousAf ? "on" : "off");

        // Touch-to-focus
        if (mTouchFocus) {
            if (mTouchFocusX > 9999 || mTouchFocusY > 9999) {
                LOGE("ERROR: Touch focus X, Y coordinate too large.");
                ret.set("touch-focus", "");
            }
            else {
                char touchfocus[10] = "";
                sprintf(touchfocus, "%d,%d", mTouchFocusX, mTouchFocusY);
                ret.set("touch-focus", touchfocus);
            }
        }
        else {
            ret.set("touch-focus", "");
        }
    }
    else if (mCameraId == 1) {
        // FFC: We need more preview and picture size to support GTalk
        ret.set("preview-size-values", "176x144,320x240,640x480");
        ret.set("picture-size-values", "176x144,320x240,640x480");
    }

    return ret;
}

status_t
LibCameraWrapper::sendCommand(int32_t cmd, int32_t arg1, int32_t arg2)
{
    LOGV("%s :", __func__);
    return mLibInterface->sendCommand(cmd, arg1, arg2);
}

void
LibCameraWrapper::release()
{
    LOGV("%s :", __func__);
    mLibInterface->release();
}

status_t
LibCameraWrapper::dump(int fd, const Vector<String16>& args) const
{
    LOGV("%s :", __func__);
    return mLibInterface->dump(fd, args);
}

}; //namespace android

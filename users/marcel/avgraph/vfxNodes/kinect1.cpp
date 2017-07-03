/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "kinect1.h"

#if ENABLE_KINECT1

#include "framework.h"
#include "freenect_internal.h" // for access to freenect_device.registration.zero_plane_info

Kinect1::~Kinect1()
{
	shut();
}

bool Kinect1::init()
{
	Assert(context == nullptr);
	if (freenect_init(&context, nullptr) < 0)
	{
		logError("freenect_init failed");
		shut();
		return false;
	}
    
	freenect_set_log_level(context, FREENECT_LOG_WARNING);
	freenect_select_subdevices(context, freenect_device_flags(FREENECT_DEVICE_MOTOR | FREENECT_DEVICE_CAMERA));
	
	//
	
	std::string serial;
	
	//
	
	freenect_device_attributes * devAttrib = nullptr;
	
	const int numDevices = freenect_list_device_attributes(context, &devAttrib);
	
	for (int i = 0; i < numDevices; ++i)
	{
		logDebug("camera serial: %s", devAttrib->camera_serial);
		
		serial = devAttrib->camera_serial;
		
		devAttrib = devAttrib->next;
	}
	
	freenect_free_device_attributes(devAttrib);
	devAttrib = nullptr;
	
	//
	
	bool deviceHasMotorControl = false;
	
	if (serial.empty())
	{
		logError("no camera found");
		shut();
		return false;
	}
	else
	{
		Assert(device == nullptr);
		if (freenect_open_device_by_camera_serial(context, &device, serial.c_str()) < 0)
		{
			logError("failed to open camera");
			shut();
			return false;
		}
		
		if (serial == "0000000000000000")
		{
			//if we do motor control via the audio device ( ie: 1473 or k4w ) and we have firmware uploaded
			//then we can do motor stuff! :)
			if (device->motor_control_with_audio_enabled)
			{
				deviceHasMotorControl = true;
			}
			else
			{
				logDebug("open device: device does not have motor control");
			}
		}
		else
		{
			deviceHasMotorControl = true;
		}
		
		mutex = SDL_CreateMutex();
		thread = SDL_CreateThread(threadMain, "Kinect1 Thread", this);
		
		//threadInit();
		//threadMain();
		//threadShut();
	}
	
	return true;
}

bool Kinect1::shut()
{
	if (thread != nullptr)
	{
		SDL_LockMutex(mutex);
		{
			stopThread = true;
		}
		SDL_UnlockMutex(mutex);
		
		SDL_WaitThread(thread, nullptr);
		thread = nullptr;
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		stopThread = false;
	}
	
	for (int i = 0; i < 2; ++i)
	{
		if (videoData[i] != nullptr)
		{
			free(videoData[i]);
			videoData[i] = nullptr;
		}
		
		if (depthData[i] != nullptr)
		{
			free(depthData[i]);
			depthData[i] = nullptr;
		}
	}
	
	if (device != nullptr)
	{
		freenect_close_device(device);
		device = nullptr;
	}
	
	if (context != nullptr)
	{
		freenect_shutdown(context);
		context = nullptr;
	}
	
	return true;
}

void Kinect1::lockBuffers()
{
	SDL_LockMutex(mutex);
}

void Kinect1::unlockBuffers()
{
	SDL_UnlockMutex(mutex);
}

void Kinect1::threadInit()
{
	//freenect_set_flag(device, FREENECT_AUTO_EXPOSURE, FREENECT_ON);
	//freenect_set_flag(device, FREENECT_AUTO_WHITE_BALANCE, FREENECT_ON);
	//freenect_set_flag(device, FREENECT_MIRROR_DEPTH, FREENECT_ON);
	//freenect_set_flag(device, FREENECT_MIRROR_VIDEO, FREENECT_ON);
	
	const freenect_frame_mode videoMode = freenect_find_video_mode(
		FREENECT_RESOLUTION_MEDIUM,
		bIsVideoInfrared
		? FREENECT_VIDEO_IR_8BIT
		: FREENECT_VIDEO_RGB);
	
	if (freenect_set_video_mode(device, videoMode) < 0)
	{
		logError("failed to set video mode");
	}
	
	const freenect_frame_mode depthMode = freenect_find_depth_mode(
		FREENECT_RESOLUTION_MEDIUM,
		bUseRegistration
		? FREENECT_DEPTH_REGISTERED
		: FREENECT_DEPTH_MM);
	
	if (freenect_set_depth_mode(device, depthMode) < 0)
	{
		logError("failed to set depth mode");
	}
	
	//We have to do this as freenect has 488 pixels for the IR image height.
	//Instead of having slightly different sizes depending on capture we will crop the last 8 rows of pixels which are empty.
	int videoHeight = height;
	if (bIsVideoInfrared)
		videoHeight = 488;
	
	const int depthDataSize = width * height * 2;
	const int videoDataSize = width * videoHeight * (bIsVideoInfrared ? 1 : 3);
	
	Assert(depthDataSize == depthMode.bytes);
	Assert(videoDataSize == videoMode.bytes);
	
	depthData[0] = malloc(depthDataSize);
	depthData[1] = malloc(depthDataSize);
	videoData[0] = malloc(videoDataSize);
	videoData[1] = malloc(videoDataSize);
	
	freenect_set_user(device, this);
	
	freenect_set_depth_buffer(device, depthData[0]);
	freenect_set_depth_callback(device, &grabDepthFrame);
	
	freenect_set_video_buffer(device, videoData[0]);
	freenect_set_video_callback(device, &grabVideoFrame);

	freenect_set_led(device, currentLed);

	if (freenect_start_depth(device) < 0)
	{
		logError("failed to start depth data stream");
	}
	
	if (freenect_start_video(device) < 0)
	{
		logError("failed to start video data stream");
	}
}

void Kinect1::threadShut()
{
	if (device != nullptr)
	{
		// finish up a tilt on exit
		
		if (tiltAngleIsDirty)
		{
			freenect_set_tilt_degs(device, newTiltAngle);
			tiltAngleIsDirty = false;
		}
		
		// stop capturing data
		
		freenect_stop_depth(device);
		freenect_stop_video(device);
		
		// restore the LED status
		
		freenect_set_led(device, LED_RED);
	}
}

bool Kinect1::threadProcess()
{
	if (context == nullptr)
	{
		return false;
	}
	
	if (freenect_process_events(context) < 0)
	{
		return false;
	}
	
	if (tiltAngleIsDirty)
	{
		freenect_set_tilt_degs(device, newTiltAngle);
		
		tiltAngleIsDirty = false;
	}
	
	if (ledIsDirty)
	{
		freenect_set_led(device, currentLed);
		
		ledIsDirty = false;
	}

	freenect_update_tilt_state(device);
	freenect_raw_tilt_state * tilt = freenect_get_tilt_state(device);
	oldTiltAngle = freenect_get_tilt_degs(tilt);

	rawAccel = Vec3(tilt->accelerometer_x, tilt->accelerometer_y, tilt->accelerometer_z);

	double dx,dy,dz;
	freenect_get_mks_accel(tilt, &dx, &dy, &dz);
	mksAccel = Vec3(dx, dy, dz);
	
	return true;
}

int Kinect1::threadMain(void * userData)
{
	Kinect1 * self = (Kinect1*)userData;
	
	self->threadInit();
	
	for (;;)
	{
		bool stop = false;
		
		SDL_LockMutex(self->mutex);
		{
			stop = self->stopThread;
		}
		SDL_UnlockMutex(self->mutex);
		
		if (stop)
		{
			break;
		}
		
		if (self->threadProcess() == false)
		{
			logDebug("thread process failed");
		}
	}
	
	self->threadShut();
	
	return 0;
}

void Kinect1::grabDepthFrame(freenect_device * dev, void * depth, uint32_t timestamp)
{
	//logDebug("got depth frame: %u", timestamp);
	
	Kinect1 * self = (Kinect1*)dev->user_data;
	
	SDL_LockMutex(self->mutex);
	{
		self->hasDepth = true;
		self->depth = depth;
		
		Assert(depth == self->depthData[0]);
		
		std::swap(self->depthData[0], self->depthData[1]);
		freenect_set_depth_buffer(self->device, self->depthData[0]);
	}
	SDL_UnlockMutex(self->mutex);
}

void Kinect1::grabVideoFrame(freenect_device * dev, void * video, uint32_t timestamp)
{
	//logDebug("got video frame: %u", timestamp);
	
	Kinect1 * self = (Kinect1*)dev->user_data;
	
	SDL_LockMutex(self->mutex);
	{
		self->hasVideo = true;
		self->video = video;
		
		Assert(video == self->videoData[0]);
		
		std::swap(self->videoData[0], self->videoData[1]);
		freenect_set_video_buffer(self->device, self->videoData[0]);
	}
	SDL_UnlockMutex(self->mutex);
}

#endif

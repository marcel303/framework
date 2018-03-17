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

#include "framework.h"
#include "Log.h"
#include "Timer.h"

#include "Calc.h"
#include "Mat4x4.h"
#include "Quat.h"
#include "StringEx.h"

#include "imu9250.h"

#include <atomic>
#include <cmath>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#define ENABLE_OSC 1

#if ENABLE_OSC
	#include "ip/UdpSocket.h"
	#include "osc/OscOutboundPacketStream.h"

	#define OSC_BUFFER_SIZE 1024
#endif

extern const int GFX_SX;
extern const int GFX_SY;

struct MyIMU9250 : IMU9250
{
	int port;
	
	SDL_mutex * mutex;
	
	SDL_Thread * receiveThread;
	
    uint32_t receiveEvent;
    
	std::atomic<bool> stop;
	
	IMU9250::ReturnMessageReader reader;
	
	short magnetMin[3] = { };
	short magnetMax[3] = { };
	bool hasMagnetMinMax = false;
	
	int receiveCount;
	
	MyIMU9250()
		: port(-1)
		, mutex(nullptr)
		, receiveThread(nullptr)
        , receiveEvent(-1)
		, stop(false)
		, reader()
		, receiveCount(0)
	{
        receiveEvent = SDL_RegisterEvents(1);
	}
	
	virtual void send(const uint8_t * bytes, const int numBytes) override
	{
        if (port >= 0)
        {
            for (int i = 0; i < 5; ++i)
            {
                write(port, bytes, numBytes);
            }
        }
	}
	
	void init(const int _port)
	{
		Assert(port == 0);
		Assert(mutex == nullptr);
		Assert(receiveThread == nullptr);
		Assert(stop == false);
		
		if (_port >= 0)
		{
			port = _port;
			
			mutex = SDL_CreateMutex();
			
			receiveThread = SDL_CreateThread(receiveThreadFunc, "IMU Receive", this);
		}
	}
	
	void shut()
	{
		if (receiveThread != nullptr)
		{
			stop = true;
			
			SDL_WaitThread(receiveThread, nullptr);
			receiveThread = nullptr;
			
			stop = false;
		}
		
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
		
		reader = IMU9250::ReturnMessageReader();
		
		port = -1;
	}
	
	void receive()
	{
		// read the next character coming from the tty
	
		int c;
	
		if (read(port, &c, 1) != 1)
		{
			LOG_WRN("read error", 0);
			SDL_Delay(1); // avoid getting into a 100% CPU situation if shit really hit the fan
			return;
		}
	
		SDL_LockMutex(mutex);
		{
			const ReturnType returnType = reader.handleByte(c);
			
			if (returnType != IMU9250::kReturnType_Invalid)
			{
				receiveCount++;
                
                SDL_Event e;
                e.type = receiveEvent;
                SDL_PushEvent(&e);
			}
			
			if (returnType == IMU9250::kReturnType_Magnet)
			{
				if (hasMagnetMinMax == false)
				{
					hasMagnetMinMax = true;
					
					for (int i = 0; i < 3; ++i)
					{
						magnetMin[i] = reader.magnet[i];
						magnetMax[i] = reader.magnet[i];
					}
				}
				else
				{
					for (int i = 0; i < 3; ++i)
					{
						magnetMin[i] = std::min(magnetMin[i], reader.magnet[i]);
						magnetMax[i] = std::max(magnetMax[i], reader.magnet[i]);
					}
				}
			}
		}
		SDL_UnlockMutex(mutex);
	}
	
	static int receiveThreadFunc(void * data)
	{
		MyIMU9250 * self = (MyIMU9250*)data;
		
		while (self->stop == false)
		{
			self->receive();
		}
		
		return 0;
	}
};

static MyIMU9250 imu;

static void mtu9250_init(MyIMU9250 & imu)
{
	SDL_LockMutex(imu.mutex);
	{
		imu.setLedStatus(imu.kLedState_On);
	
		// we want to operate at high speed
	
		//imu.setBaudRate(imu.kBaudRate_115200);
        imu.setBaudRate(imu.kBaudRate_921600);
		imu.setReturnRate(imu.kReturnRate_100);
	
		// tell the unit which values we're interested in
	
		imu.setReturnContent(
			imu.kReturnContent_Acceleration |
			imu.kReturnContent_AngularVelocity |
			imu.kReturnContent_Magnet |
			0);
	
	#if 1
		// reset the calibration offsets
	
		for (int i = 0; i < 4; ++i)
		{
			imu.setAccelOffsets(0, 0, 0);
			imu.setGyroOffsets(0, 0, 0);
			imu.setMagnetOffsets(0, 0, 0);
		}
	#endif
	}
	SDL_UnlockMutex(imu.mutex);
}

struct TTY
{
	int port;
	
	TTY()
		: port(-1)
	{
	}
	
	~TTY()
	{
		shut();
	}
	
	bool init()
	{
		// stty -f /dev/tty.HC-06-DevB 115200
		
		const char * ttyPath = "/dev/tty.HC-06-DevB";
		
		port = open(ttyPath, O_RDWR);
		
		if (port < 0)
		{
			shut();
			
			return false;
		}
		else
		{
		#if 1
			termios settings;
			
			if (tcgetattr(port, &settings) != 0)
			{
				LOG_ERR("failed to get terminal io settings", 0);
			}
			else
			{
				const int rate = 115200;
				
				if (cfsetispeed(&settings, rate) != 0)
					LOG_ERR("failed to set tty ispeed to %d", rate);
				if (cfsetospeed(&settings, rate) != 0)
					LOG_ERR("failed to set tty ospeed to %d", rate);
				
				//settings.c_cflag = (settings.c_cflag & (~CSIZE)) | CS8;
				//CDTR_IFLOW; // enable
				//CRTS_IFLOW; // disable
				
				if (tcsetattr(port, TCSANOW, &settings) != 0)
					LOG_ERR("failed to apply terminal io settings", 0);
				else
					LOG_INF("succesfully applied terminal io settings", 0);
				
				tcflush(port, TCIOFLUSH);
			}
		#endif
			
			return true;
		}
	}
	
	void shut()
	{
		if (port >= 0)
		{
			close(port);
		}
		
		port = -1;
	}
};

static void updateRotation(
	const float * acceleration,
	const float * gyro,
	const float * magnet,
	const double dt,
	Quat & r)
{
	const float degToRad = M_PI / 180.f;
	
	// integrate gyro

	Quat qX;
	Quat qY;
	Quat qZ;

	qX.fromAxisAngle(Vec3(1,0,0), gyro[0] * degToRad * dt);
	qY.fromAxisAngle(Vec3(0,1,0), gyro[1] * degToRad * dt);
	qZ.fromAxisAngle(Vec3(0,0,1), gyro[2] * degToRad * dt);

	r = r * qX;
	r = r * qY;
	r = r * qZ;

	// smoothly interpolate to magnet

	Mat4x4 mM;
	mM.MakeLookat(
		Vec3(0,0,0),
		Vec3(acceleration[0], acceleration[1], acceleration[2]).CalcNormalized(),
		Vec3(magnet[0], magnet[1], magnet[2]).CalcNormalized());
	Quat qM;
	qM.fromMatrix(mM);

	const float s = (float)std::pow(0.1, dt);
	//const float s = 0.f;
	//const float s = 1.f;

	r = qM.slerp(r, s);
}

static Quat r;

//static short magnetBias[3] = { -38, -116, -238 };
static short magnetBias[3] = { -96, -161, -355 };

static std::atomic<bool> quitRequested(false);

static int updateThreadProc(void * obj)
{
	MyIMU9250 & imu = *(MyIMU9250*)obj;
	
#if ENABLE_OSC
	UdpTransmitSocket * transmitSockets[1] = { };
	const int numTransmitSockets = sizeof(transmitSockets) / sizeof(transmitSockets[0]);

	const char * ipAddress = "127.0.0.1";
	//const int udpPort = 2000;
	//const char * ipAddress = "192.168.0.209";
	//const int udpPorts[2] = { 2002, 2018 };
	const int udpPorts[2] = { 2000, 2018 };
	
	LOG_DBG("setting up UDP transmit sockets for OSC messaging", 0);

	try
	{
		if (numTransmitSockets >= 1)
			transmitSockets[0] = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPorts[0]));
		if (numTransmitSockets >= 2)
			transmitSockets[1] = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPorts[1]));
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to create UDP transmit socket: %s", e.what());
	}
#endif

	uint64_t lastTime = g_TimerRT.TimeUS_get();

	while (quitRequested == false)
	{
		SDL_Delay(5);
		
		const uint64_t currentTime = g_TimerRT.TimeUS_get();
		const double dt = (currentTime - lastTime) / 1000000.0;
		lastTime = currentTime;
		
		//
		
		float acceleration[3] = { };
		float gyro[3] = { };
		float magnet[3] = { };
		
		if (imu.port >= 0)
		{
			SDL_LockMutex(imu.mutex);
			{
				for (int i = 0; i < 3; ++i)
				{
					acceleration[i] = imu.reader.accelerationf[i];
					gyro[i] = imu.reader.angularVelocityf[i];
					magnet[i] = imu.reader.magnet[i] + magnetBias[i];
				}
			}
			SDL_UnlockMutex(imu.mutex);
		}
		else
		{
			acceleration[2] = 1.f;
			magnet[1] = 1.f;
		}
		
	#if 1
		const int numSteps = 100;
		const float stepDt = dt / numSteps;
		
		for (int i = 0; i < numSteps; ++i)
		{
			updateRotation(
				acceleration,
				gyro,
				magnet,
				stepDt,
				r);
		}
	#endif
	
	#if ENABLE_OSC
		// send the result over OSC
		
		for (int i = 0; i < numTransmitSockets; ++i)
		{
			auto transmitSocket = transmitSockets[i];
			
			if (transmitSocket != nullptr)
			{
				try
				{
					char buffer[OSC_BUFFER_SIZE];
					osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);

					Vec3 axis;
					float angle;
					r.toAxisAngle(axis, angle);
					
					Vec3 axis2;
					axis2[0] = axis[1];
					axis2[1] = axis[2];
					axis2[2] = axis[0];
					
					p << osc::BeginMessage("/listener1/angleAxis");
					
					p << float(angle * 180.f / M_PI);
					p << axis2[0];
					p << axis2[1];
					p << axis2[2];
					
					p << osc::EndMessage;

					transmitSocket->Send(p.Data(), p.Size());
				}
				catch (std::exception & e)
				{
					LOG_ERR("failed to send OSC message: %s", e.what());
				}
			}
		}
	#endif
	}

	return 0;
}

void testImu9250()
{
	TTY tty;
	
	MyIMU9250 imu;
	
    if (tty.port >= 0)
    {
        imu.init(tty.port);
        
        mtu9250_init(imu);
    }

	Camera3d camera;
	camera.gamepadIndex = 0;
	
	r.makeIdentity();
	
	SDL_Thread * updateThread = SDL_CreateThread(updateThreadProc, "Update Rotation", &imu);
    
    bool isCalibrating = false;
	
    bool draw3d = false;
    
	do
	{
        framework.waitForEvents = !draw3d;
        
		framework.process();
		
        if (framework.waitForEvents)
        {
			framework.timeStep = 0.f;
		}
		
		if (framework.windowIsActive == false)
		{
			SDL_Delay(1000/10);
		}
		
		// input
		
		if (keyboard.wentDown(SDLK_RETURN))
		{
			if (tty.port >= 0)
			{
				imu.shut();
				
				tty.shut();
			}
			else
			{
				tty.init();
				
                if (tty.port >= 0)
                {
                    imu.init(tty.port);
                    
                    mtu9250_init(imu);
                }
			}
		}
		
        if (isCalibrating)
        {
            if (tty.port < 0)
            {
                isCalibrating = false;
            }
        }
        
        if (keyboard.wentDown(SDLK_c))
        {
            if (isCalibrating)
            {
                isCalibrating = false;
                
                SDL_LockMutex(imu.mutex);
                {
                    magnetBias[0] = - (imu.magnetMax[0] + imu.magnetMin[0]) / 2;
                    magnetBias[1] = - (imu.magnetMax[1] + imu.magnetMin[1]) / 2;
                    magnetBias[2] = - (imu.magnetMax[2] + imu.magnetMin[2]) / 2;
                }
                SDL_UnlockMutex(imu.mutex);
            }
            else
            {
                isCalibrating = true;
                
                SDL_LockMutex(imu.mutex);
                {
                    imu.hasMagnetMinMax = false;
                }
                SDL_UnlockMutex(imu.mutex);
            }
        }
		
        if (keyboard.wentDown(SDLK_v))
        {
        	draw3d = !draw3d;
		}
		
		//
		
		const float dt = framework.timeStep;
	
		//
		
        const bool enableCameraInput = draw3d;
        
		camera.tick(dt, enableCameraInput);
		
		Mat4x4 transform;
		
		transform = r.toMatrix();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (draw3d)
			{
				projectPerspective3d(90.f, .01f, 100.f);
				
				camera.pushViewMatrix();
				{
					gxTranslatef(0, 0, 2);
					//gxRotatef(+90, 0, 0, 1);
					
					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LESS);
			
					gxPushMatrix();
					{
						gxMultMatrixf(transform.m_v);
						
						setColor(colorRed);
						drawGrid3dLine(6, 6, 0, 1);
						
						setColor(colorWhite);
						
						gxPushMatrix();
						gxTranslatef(0, 0, -.1f);
						gxSetTexture(getTexture("imu9250-front.jpg"));
						gxBegin(GL_QUADS);
						{
							gxTexCoord2f(0.f, 0.f); gxVertex2f(-1, -1);
							gxTexCoord2f(1.f, 0.f); gxVertex2f(-1, +1);
							gxTexCoord2f(1.f, 1.f); gxVertex2f(+1, +1);
							gxTexCoord2f(0.f, 1.f); gxVertex2f(+1, -1);
						}
						gxEnd();
						gxSetTexture(0);
						gxPopMatrix();
						
						gxPushMatrix();
						gxTranslatef(0, 0, +.1f);
						gxSetTexture(getTexture("imu9250-back.jpg"));
						gxBegin(GL_QUADS);
						{
							gxTexCoord2f(0.f, 0.f); gxVertex2f(-1, +1);
							gxTexCoord2f(1.f, 0.f); gxVertex2f(-1, -1);
							gxTexCoord2f(1.f, 1.f); gxVertex2f(+1, -1);
							gxTexCoord2f(0.f, 1.f); gxVertex2f(+1, +1);
						}
						gxEnd();
						gxSetTexture(0);
						gxPopMatrix();
					}
					gxPopMatrix();
					
					gxPushMatrix();
					{
						gxScalef(4, 4, 4);
						
					#if 0
						gxBegin(GL_LINES);
						{
							setColor(colorWhite);
							gxVertex3f(0, 0, 0);
							gxVertex3f(magnet[0], magnet[1], magnet[2]);
							
							setColor(colorYellow);
							gxVertex3f(0, 0, 0);
							gxVertex3f(acceleration[0], acceleration[1], acceleration[2]);
						}
						gxEnd();
					#endif
						
						setColor(colorRed);
						drawLine3d(0);
						setColor(colorGreen);
						drawLine3d(1);
						setColor(colorBlue);
						drawLine3d(2);
					}
					gxPopMatrix();
					
					glDisable(GL_DEPTH_TEST);
				}
				camera.popViewMatrix();
				
				projectScreen2d();
			}
			
            setFont("calibri.ttf");
            
            // draw caption
            
            Color captionColor;
            std::string captionText;
            Color captionTextColor;
            
            if (isCalibrating)
            {
                captionColor = colorYellow;
                captionText = String::FormatC("CALIBRATING");
                captionTextColor = colorBlack;
            }
            else if (tty.port >= 0)
            {
                captionColor = colorGreen;
                captionText = String::FormatC("CONNECTED. receiveCount: %d", imu.receiveCount);
                captionTextColor = colorBlack;
            }
            else
            {
                captionColor = colorRed;
                captionText = String::FormatC("DISCONNECTED. press ENTER to connect");
                captionTextColor = colorWhite;
            }
            
            setColor(captionColor);
            hqBegin(HQ_FILLED_ROUNDED_RECTS);
            hqFillRoundedRect(GFX_SX/2-180, 6, GFX_SX/2+180, 5+26, 5);
            hqEnd();
            setColor(captionTextColor);
            drawText(GFX_SX/2, 5 + 15, 16, 0, 0, "%s", captionText.c_str());
			
            const int biasedMagnet[3] =
            {
				imu.reader.magnet[0] + magnetBias[0],
				imu.reader.magnet[1] + magnetBias[1],
				imu.reader.magnet[2] + magnetBias[2]
			};
			
            setColor(colorWhite);
			drawText(5, 38, 12, +1, +1, "magnet: (%d, %d, %d) -> (%.2f, %.2f, %.2f)",
				imu.reader.magnet[0],
				imu.reader.magnet[1],
				imu.reader.magnet[2],
				biasedMagnet[0],
				biasedMagnet[1],
				biasedMagnet[2]);
			drawText(5, 53, 12, +1, +1, "magnet min/max: (%d, %d, %d) - (%d, %d, %d)",
				imu.magnetMin[0],
				imu.magnetMin[1],
				imu.magnetMin[2],
				imu.magnetMax[0],
				imu.magnetMax[1],
				imu.magnetMax[2]);
            drawText(5, 68, 12, +1, +1, "magnet bias: (%d, %d, %d)",
                 magnetBias[0],
                 magnetBias[1],
                 magnetBias[2]);
            
            if (isCalibrating)
                drawText(5, GFX_SY - 100, 12, +1, +1, "PRESS [C] when done calibrating magnet");
            else if (tty.port >= 0)
                drawText(5, GFX_SY - 100, 12, +1, +1, "PRESS [C] to calibrate the magnet");
			
			drawText(5, GFX_SY - 20, 12, +1, +1, "PRESS [V] to toggle 3D preview (%s)", draw3d ? "enabled" : "disabled");
		}
		framework.endDraw();
	}
	while (!keyboard.wentDown(SDLK_ESCAPE) && !framework.quitRequested);
	
	quitRequested = true;
	
	SDL_WaitThread(updateThread, nullptr);
	updateThread = nullptr;
	
	imu.shut();
	
	tty.shut();
}

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

#include "imu9250.h"

#include <atomic>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#define ENABLE_OSC 0

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
		, stop(false)
		, reader()
		, receiveCount(0)
	{
	}
	
	virtual void send(const uint8_t * bytes, const int numBytes) override
	{
		write(port, bytes, numBytes);
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
	
		imu.setBaudRate(imu.kBaudRate_115200);
		imu.setReturnRate(imu.kReturnRate_200);
	
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
		#if 0
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

	const float s = powf(.1f, dt);
	//const float s = 0.f;
	//const float s = 1.f;

	r = qM.slerp(r, s);
}

void testImu9250()
{
	TTY tty;
	
	if (tty.init() == false)
	{
		LOG_ERR("failed to open TTY", 0);
	}
	else
	{
		LOG_INF("tty port: %d", tty.port);
	}
	
	MyIMU9250 imu;
	
	imu.init(tty.port);
	
	mtu9250_init(imu);
	
#if ENABLE_OSC
	UdpTransmitSocket * transmitSockets[2] = { };
	const int numTransmitSockets = sizeof(transmitSockets) / sizeof(transmitSockets[0]);

	//const char * ipAddress = "127.0.0.1";
	//const int udpPort = 8000;
	const char * ipAddress = "192.168.0.209";
	const int udpPorts[2] = { 2002, 2018 };
	
	LOG_DBG("setting up UDP transmit sockets for OSC messaging", 0);

	try
	{
		transmitSockets[0] = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPorts[0]));
		transmitSockets[1] = new UdpTransmitSocket(IpEndpointName(ipAddress, udpPorts[1]));
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to create UDP transmit socket: %s", e.what());
	}
#endif

	Camera3d camera;
	camera.gamepadIndex = 0;
	
	Quat r;
	r.makeIdentity();
	
	Quat c;
	c.makeIdentity();
	
	do
	{
		framework.process();
		
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
				
				imu.init(tty.port);
			}
		}
		
		if (keyboard.wentDown(SDLK_c))
		{
			c = r;
		}
		
		float acceleration[3] = { };
		float gyro[3] = { };
		float magnet[3] = { };
		
		if (imu.port >= 0)
		{
			SDL_LockMutex(imu.mutex);
			{
				if (keyboard.wentDown(SDLK_r))
				{
					imu.hasMagnetMinMax = false;
				}
				
				const short magnetBias[3] = { -38, -116, -238 };
				
				for (int i = 0; i < 3; ++i)
				{
					acceleration[i] = imu.reader.accelerationf[i];
					gyro[i] = imu.reader.angularVelocityf[i];
					magnet[i] = imu.reader.magnet[i] + magnetBias[i];
				}
			}
			SDL_UnlockMutex(imu.mutex);
		}
		
		if (keyboard.isDown(SDLK_r))
			gyro[0] = -mouse.x / 10.f;
		if (keyboard.isDown(SDLK_f))
			gyro[0] = +mouse.x / 10.f;
		if (keyboard.isDown(SDLK_t))
			gyro[1] = -mouse.x / 10.f;
		if (keyboard.isDown(SDLK_g))
			gyro[1] = +mouse.x / 10.f;
		if (keyboard.isDown(SDLK_y))
			gyro[2] = -mouse.x / 10.f;
		if (keyboard.isDown(SDLK_h))
			gyro[2] = +mouse.x / 10.f;
		
		//
		
		const float dt = framework.timeStep;
		
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
		
	#if ENABLE_OSC
		// send the result over OSC
		
		for (int i = 0; i < numTransmitSockets; ++i)
		{
			auto transmitSocket = transmitSockets[i];
			
			if (transmitSocket != nullptr)
			{
				try
				{
					auto & reader = imu.reader;
					
					char buffer[OSC_BUFFER_SIZE];
					
					osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);

					p << osc::BeginMessage("/gyro");
					
					p << reader.anglef[0];
					p << reader.anglef[1];
					p << reader.anglef[2];
					p << reader.accelerationf[0];
					p << reader.accelerationf[1];
					p << reader.accelerationf[2];
					p << reader.angularVelocityf[0];
					p << reader.angularVelocityf[1];
					p << reader.angularVelocityf[2];
					p << reader.temperature;
					
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
	
		//
		
		camera.tick(dt, true);
		
		Mat4x4 transform;
		
		const Quat r2 = r * c.calcInverse();
		
		transform = r2.toMatrix();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
				gxTranslatef(0, 0, 2);
				gxRotatef(+90, 0, 0, 1);
				
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
					gxBegin(GL_LINES);
					{
						gxVertex3f(0, 0, 0);
						gxVertex3f(magnet[0], magnet[1], magnet[2]);
						
						gxVertex3f(0, 0, 0);
						gxVertex3f(acceleration[0], acceleration[1], acceleration[2]);
					}
					gxEnd();
					
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
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(5, 5, 12, +1, +1, "TTY port: %d", tty.port);
			drawText(5, 20, 12, +1, +1, "magnet: (%d, %d, %d) -> (%.2f, %.2f, %.2f)",
				imu.reader.magnet[0],
				imu.reader.magnet[1],
				imu.reader.magnet[2],
				magnet[0], magnet[1], magnet[2]);
			drawText(5, 35, 12, +1, +1, "magnet min/max: (%d, %d, %d) / (%d, %d, %d)",
				imu.magnetMin[0],
				imu.magnetMin[1],
				imu.magnetMin[2],
				imu.magnetMax[0],
				imu.magnetMax[1],
				imu.magnetMax[2]);
			drawText(5, 50, 12, +1, +1, "magnet avg: (%d, %d, %d)",
				(imu.magnetMin[0] + imu.magnetMax[0]) / 2,
				(imu.magnetMin[1] + imu.magnetMax[1]) / 2,
				(imu.magnetMin[2] + imu.magnetMax[2]) / 2);
		}
		framework.endDraw();
	}
	while (!keyboard.wentDown(SDLK_ESCAPE));
	
	imu.shut();
	
	tty.shut();
	
	//exit(0);
}

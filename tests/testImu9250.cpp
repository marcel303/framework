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

enum State
{
	kState_None,
	kState_CalibrateGyroAndAccel,
	kState_CalibrateMagnet,
	kState_Reading
};

#if ENABLE_OSC
static UdpTransmitSocket * transmitSockets[2] = { };
static const int numTransmitSockets = sizeof(transmitSockets) / sizeof(transmitSockets[0]);
#endif

static MyIMU9250 imu;

static State state = kState_None;
static uint64_t stateTimer = 0;

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
			imu.kReturnContent_Quaternion |
			0);
	
		// reset the calibration offsets
	
		for (int i = 0; i < 4; ++i)
		{
			imu.setAccelOffsets(0, 0, 0);
			imu.setGyroOffsets(0, 0, 0);
			imu.setMagnetOffsets(0, 0, 0);
		}
	}
	SDL_UnlockMutex(imu.mutex);
	
	state = kState_Reading;
}

static double lerpAngles(const double a, const double b, const double t)
{
	Quat qA;
	Quat qB;
	
	qA.fromAxisAngle(Vec3(1, 0, 0), a);
	qB.fromAxisAngle(Vec3(1, 0, 0), b);

	const Quat q = qB.slerp(qA, t);

	Vec3 axis;
	float angle;
	
	q.toAxisAngle(axis, angle);
	
	if (axis[0] < -.5f)
		angle = - angle;
	
	return angle;
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
	
	Camera3d camera;
	camera.gamepadIndex = 0;
	
	Quat r;
	r.makeIdentity();

	do
	{
		framework.process();
		
		const float dt = framework.timeStep;
		
		const int numSteps = 100;
		
		const float stepDt = dt / numSteps;
		
		float magnet[3];
		float accel[3];
		
		for (int i = 0; i < numSteps; ++i)
		{
			// update fake gyro/accel/magnet
			
		#if 0
			float gyro[3] =
			{
				0.f,
				0.f,
				0.f
			};
		#else
			float gyro[3];
			
			SDL_LockMutex(imu.mutex);
			{
				for (int i = 0; i < 3; ++i)
				{
					gyro[i] = imu.reader.angularVelocityf[i] * M_PI / 180.f;
				}
			}
			SDL_UnlockMutex(imu.mutex);
		#endif
			
			if (keyboard.isDown(SDLK_r))
				gyro[0] = -mouse.x / 100.f;
			if (keyboard.isDown(SDLK_f))
				gyro[0] = +mouse.x / 100.f;
			if (keyboard.isDown(SDLK_t))
				gyro[1] = -mouse.x / 100.f;
			if (keyboard.isDown(SDLK_g))
				gyro[1] = +mouse.x / 100.f;
			if (keyboard.isDown(SDLK_y))
				gyro[2] = -mouse.x / 100.f;
			if (keyboard.isDown(SDLK_h))
				gyro[2] = +mouse.x / 100.f;
			
		#if 0
			magnet[0] = 1.f;
			magnet[1] = 0.f;
			magnet[2] = 0.f;
		#elif 0
			magnet[0] = std::cos(framework.time / 4.f);
			magnet[1] = std::sin(framework.time / 4.f);
			magnet[2] = 0.f;
		#else
			SDL_LockMutex(imu.mutex);
			{
				const float magnetBias[3] = { -35.08f, -8.69f, -47.37f };
				//const float magnetBias[3] = { };
				
				for (int i = 0; i < 3; ++i)
				{
					magnet[i] = imu.reader.magnetf[i] + magnetBias[i];
				}
			}
			SDL_UnlockMutex(imu.mutex);
		#endif
			
		#if 0
			accel[0] = 0.f;
			accel[1] = 0.f;
			accel[2] = 1.f;
		#else
			SDL_LockMutex(imu.mutex);
			{
				for (int i = 0; i < 3; ++i)
				{
					accel[i] = imu.reader.accelerationf[i];
				}
			}
			SDL_UnlockMutex(imu.mutex);
		#endif
		
			// integrate gyro
		
			Quat qX;
			Quat qY;
			Quat qZ;
			
			qX.fromAxisAngle(Vec3(1,0,0), gyro[0] * stepDt);
			qY.fromAxisAngle(Vec3(0,1,0), gyro[1] * stepDt);
			qZ.fromAxisAngle(Vec3(0,0,1), gyro[2] * stepDt);
			
			r = r * qX;
			r = r * qY;
			r = r * qZ;
			
			// smoothly interpolate to magnet
			
			Mat4x4 mM;
			mM.MakeLookat(Vec3(0,0,0), Vec3(accel[0], accel[1], accel[2]).CalcNormalized(), Vec3(magnet[0], magnet[1], magnet[2]).CalcNormalized());
			Quat qM;
			qM.fromMatrix(mM);
			
			const float s = std::pow(.1f, stepDt);
			//const float s = 0.f;
			//const float s = 1.f;
			
			r = qM.slerp(r, s);
		}
		
		//
		
		camera.tick(dt, true);
		
		Mat4x4 transform;
		
		transform = r.toMatrix();
		
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
						gxVertex3f(accel[0], accel[1], accel[2]);
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
			drawText(5, 20, 12, +1, +1, "magnet: %.2f, %.2f, %.2f", magnet[0], magnet[1], magnet[2]);
		}
		framework.endDraw();
	}
	while (!keyboard.wentDown(SDLK_ESCAPE));
	
	//exit(0);
}

void testImu9250_v1()
{
	// stty -f /dev/tty.HC-06-DevB 115200
	
	const char * ttyPath = "/dev/tty.HC-06-DevB";
	
#if ENABLE_OSC
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

	LOG_DBG("opening tty port to connect to gyro sensor", 0);
	
	int port = open(ttyPath, O_RDWR);
	
	if (port < 0)
	{
		LOG_ERR("failed to open tty port. path=%s", ttyPath);
	}
	else
	{
		LOG_INF("opened tty port. path=%s", ttyPath);
		
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
	
		imu.init(port);
		
		mtu9250_init(imu);
		
		float magnetMin[3] = { };
		float magnetMax[3] = { };
		bool hasMagnetMinMax = false;
		
		Camera3d camera;
		camera.gamepadIndex = 0;
		
		bool enableCameraInput = false;
		
		float summedAngles[3] = { };
		
		const int kMaxHistory = 10000;
		float magnetHistory[3][kMaxHistory];
		int magnetHistorySize = 0;
		int nextMagnetHistoryIndex = 0;
		
		//float magnetBias[3] = { };
		float magnetBias[3] = { -35.08f, -8.69f, -47.37f };
		float gyroBias[3] = { };
		
		float eulerBias[3] = { };
		
		do
		{
			framework.process();
			
			const float dt = framework.timeStep;
			
			SDL_LockMutex(imu.mutex);
			
			const float biasedGyro[3] =
			{
				imu.reader.angularVelocityf[0] + gyroBias[0],
				imu.reader.angularVelocityf[1] + gyroBias[1],
				imu.reader.angularVelocityf[2] + gyroBias[2]
			};
			
			const float biasedMagnet[3] =
			{
				imu.reader.magnetf[0] + magnetBias[0],
				imu.reader.magnetf[1] + magnetBias[1],
				imu.reader.magnetf[2] + magnetBias[2]
			};
			
			switch (state)
			{
			case kState_None:
				break;
			case kState_CalibrateGyroAndAccel:
				if (g_TimerRT.TimeUS_get() >= stateTimer + 7 * 1000000)
				{
					LOG_DBG("calibration done. entering reading mode", 0);
					imu.setCalibrationMode(imu.kCalibrationMode_Off);
					state = kState_Reading;
					break;
				}
				break;
				
			case kState_CalibrateMagnet:
				if (g_TimerRT.TimeUS_get() >= stateTimer + 7 * 1000000)
				{
					LOG_DBG("calibration done. entering reading mode", 0);
					imu.setCalibrationMode(imu.kCalibrationMode_Off);
					state = kState_Reading;
					break;
				}
				break;
				
			case kState_Reading:
				if (keyboard.wentDown(SDLK_f))
					enableCameraInput = !enableCameraInput;
				
				if (keyboard.wentDown(SDLK_g))
				{
					// enter calibration mode
					LOG_DBG("entering gyro calibration mode!", 0);
					state = kState_CalibrateGyroAndAccel;
					imu.setCalibrationMode(imu.kCalibrationMode_GyroAndAccel);
					stateTimer = g_TimerRT.TimeUS_get();
					break;
				}
				
				if (keyboard.wentDown(SDLK_m))
				{
					// enter calibration mode
					LOG_DBG("entering magnet calibration mode!", 0);
					state = kState_CalibrateMagnet;
					imu.setCalibrationMode(imu.kCalibrationMode_Magnet);
					stateTimer = g_TimerRT.TimeUS_get();
					break;
				}
				
				if (keyboard.wentDown(SDLK_r))
				{
					hasMagnetMinMax = false;
				}
				
				if (keyboard.wentDown(SDLK_t))
				{
				#if 0
					// note : imu.setMagnetOffsets doesn't seem to work as expected
					//imu.setMagnetOffsets(...)
				#else
					for (int i = 0; i < 3; ++i)
					{
						magnetBias[i] = - (magnetMin[i] + magnetMax[i]) / 2.f;
					}
				#endif
				
					magnetHistorySize = 0;
					nextMagnetHistoryIndex = 0;
				}
				
				if (keyboard.wentDown(SDLK_h))
				{
				#if 0
					imu.setGyroOffsets(
						imu.reader.angularVelocity[0],
						imu.reader.angularVelocity[1],
						imu.reader.angularVelocity[2]);
				#else
					gyroBias[0] = - imu.reader.angularVelocityf[0];
					gyroBias[1] = - imu.reader.angularVelocityf[1];
					gyroBias[2] = - imu.reader.angularVelocityf[2];
				#endif
				}
				
				if (hasMagnetMinMax == false)
				{
					hasMagnetMinMax = true;
					
					magnetMin[0] = imu.reader.magnetf[0];
					magnetMin[1] = imu.reader.magnetf[1];
					magnetMin[2] = imu.reader.magnetf[2];
					
					magnetMax[0] = imu.reader.magnetf[0];
					magnetMax[1] = imu.reader.magnetf[1];
					magnetMax[2] = imu.reader.magnetf[2];
				}
				else
				{
					magnetMin[0] = std::min(magnetMin[0], imu.reader.magnetf[0]);
					magnetMin[1] = std::min(magnetMin[1], imu.reader.magnetf[1]);
					magnetMin[2] = std::min(magnetMin[2], imu.reader.magnetf[2]);
					
					magnetMax[0] = std::max(magnetMax[0], imu.reader.magnetf[0]);
					magnetMax[1] = std::max(magnetMax[1], imu.reader.magnetf[1]);
					magnetMax[2] = std::max(magnetMax[2], imu.reader.magnetf[2]);
				}
				break;
			}
			
		// ----
		
			// calculate the magnetic angle
			const float magnetAngle = atan2f(-biasedMagnet[1], biasedMagnet[0]);
			
			// integrate gyro
			summedAngles[2] += Calc::DegToRad(imu.reader.angularVelocityf[2] * dt);
			
			// smoothe to magnetic angle
			const double retain = std::pow(0.05, double(dt));
			summedAngles[2] = lerpAngles(summedAngles[2], magnetAngle, retain);
			
		// ----
		
			for (int i = 0; i < 3; ++i)
				magnetHistory[i][nextMagnetHistoryIndex] = biasedMagnet[i];
			magnetHistorySize = std::min(kMaxHistory, magnetHistorySize + 1);
			nextMagnetHistoryIndex = (nextMagnetHistoryIndex + 1) % kMaxHistory;
			
			SDL_UnlockMutex(imu.mutex);
			
			camera.tick(dt, enableCameraInput);
			
			framework.beginDraw(0, 0, 0, 0);
			{
				setFont("calibri.ttf");
				
				IMU9250::ReturnMessageReader reader;
				SDL_LockMutex(imu.mutex);
				{
					reader = imu.reader;
				}
				SDL_UnlockMutex(imu.mutex);
				
				projectPerspective3d(90.f, .01f, 100.f);
				camera.pushViewMatrix();
				{
					gxTranslatef(0, 0, 2);
					
				#if 1
					Mat4x4 mY;
					
					mY.MakeRotationY(summedAngles[2]);
					
					const Mat4x4 magnetMatrix = mY;
				#elif 0
					Mat4x4 mY;
					Mat4x4 mP;
					Mat4x4 mR;
					
					mY.MakeRotationY(Calc::DegToRad(reader.anglef[2]));
					mP.MakeRotationX(-Calc::DegToRad(reader.anglef[1]));
					mR.MakeRotationZ(Calc::DegToRad(reader.anglef[0]));
					
					const Mat4x4 magnetMatrix = mY * mP * mR;
				#elif 1
					Quat q(
						reader.quaternionf[0],
						reader.quaternionf[1],
						reader.quaternionf[2],
						reader.quaternionf[3]);
					
					const Mat4x4 magnetMatrix = q.toMatrix();
					//magnetMatrix = magnetMatrix.CalcInv();
				#elif 0
					Mat4x4 magnetMatrix;
					magnetMatrix.MakeLookat(
						Vec3(0.f, 0.f, 0.f),
						Vec3(reader.accelerationf[0], reader.accelerationf[2], reader.accelerationf[1]),
						Vec3(0.f, 1.f, 0.f));
					magnetMatrix = magnetMatrix.CalcInv();
				#elif 1
					Mat4x4 magnetMatrix;
					magnetMatrix.MakeRotationY(Calc::DegToRad(reader.anglef[2]));
				#elif 1
					Mat4x4 magnetMatrix;
					magnetMatrix.MakeRotationY(std::atan2(reader.magnetf[0], reader.magnetf[1]));
				#else
					Mat4x4 magnetMatrix;
					magnetMatrix.MakeLookat(
						Vec3(0.f, 0.f, 0.f),
						Vec3(reader.magnetf[0], reader.magnetf[1], reader.magnetf[2]),
						Vec3(0.f, 1.f, 0.f));
				#endif
					
					gxPushMatrix();
					gxScalef(.1f, .1f, .1f);
					pushBlend(BLEND_ADD);
					gxBegin(GL_LINES);
					{
						gxColor4f(1, 1, 1, .5f);
						for (int i = 0; i < magnetHistorySize - 1; ++i)
						{
							gxVertex3f(magnetHistory[0][i + 0], magnetHistory[2][i + 0], -magnetHistory[1][i + 0]);
							gxVertex3f(magnetHistory[0][i + 1], magnetHistory[2][i + 1], -magnetHistory[1][i + 1]);
						}
					}
					gxEnd();
					popBlend();
					gxPopMatrix();
				
					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LESS);
					
					gxPushMatrix();
					{
						gxMultMatrixf(magnetMatrix.m_v);
						
						setColor(colorGreen);
						drawGrid3dLine(10, 10, 0, 1);
					}
					gxPopMatrix();
					
					glDisable(GL_DEPTH_TEST);
				}
				camera.popViewMatrix();
				projectScreen2d();
				
				Color color;
				const char * text = nullptr;
				if (state == kState_Reading)
				{
					color = colorBlue;
					text = "Reading";
				}
				else if (state == kState_CalibrateGyroAndAccel)
				{
					color = colorYellow;
					text = "Calibrating gyro and accel";
				}
				else if (state == kState_CalibrateMagnet)
				{
					color = colorRed;
					text = "Calibrating magnet";
				}
				
				if (text != nullptr)
				{
					hqBegin(HQ_FILLED_ROUNDED_RECTS);
					setColor(color);
					hqFillRoundedRect(GFX_SX/2-200, 100, GFX_SX/2+200, 160, 6.f);
					hqEnd();
					
					setColor(colorWhite);
					drawText(GFX_SX/2, 130, 30, 0, 0, "%s", text);
				}
				
				int x = GFX_SX/2;
				int y = 40;
				setLumi(255);
				
				drawText(x, y, 16, 0, 0, "State: %d. Receive count: %d", state, imu.receiveCount);
				y += 20;
				
				x = 40;
				y = GFX_SY * 1/3;
				setLumi(127);
				
				drawText(x, y, 16, +1, 0, "Angle: %.2f, %.2f, %.2f",
					reader.anglef[0],
					reader.anglef[1],
					reader.anglef[2]);
				y += 20;
				
				drawText(x, y, 16, +1, 0, "Acceleration: %.2f, %.2f, %.2f",
					reader.accelerationf[0],
					reader.accelerationf[1],
					reader.accelerationf[2]);
				y += 20;
				
				drawText(x, y, 16, +1, 0, "Angular velocity: %.2f, %.2f, %.2f",
					reader.angularVelocityf[0],
					reader.angularVelocityf[1],
					reader.angularVelocityf[2]);
				y += 20;
				drawText(x, y, 16, +1, 0, "Angular velocity + Bias: %.2f, %.2f, %.2f",
					biasedGyro[0],
					biasedGyro[1],
					biasedGyro[2]);
				y += 20;
				
				drawText(x, y, 16, +1, 0, "Magnet: %.2f, %.2f, %.2f",
					reader.magnetf[0],
					reader.magnetf[1],
					reader.magnetf[2]);
				y += 20;
				drawText(x, y, 16, +1, 0, "Magnet (Biased): %.2f, %.2f, %.2f",
					biasedMagnet[0],
					biasedMagnet[1],
					biasedMagnet[2]);
				y += 20;
				drawText(x, y, 16, +1, 0, "Magnet Bias: %.2f, %.2f, %.2f",
					magnetBias[0],
					magnetBias[1],
					magnetBias[2]);
				y += 20;
				drawText(x, y, 16, +1, 0, "Magnet Min/Max: %d, %d, %d , %d, %d, %d",
					magnetMin[0],
					magnetMin[1],
					magnetMin[2],
					magnetMax[0],
					magnetMax[1],
					magnetMax[2]);
				y += 20;
				
				// help text
				
				x = GFX_SX/2;
				y = GFX_SY - 100;
				setLumi(255);
				
				drawText(x, y, 16, 0, 0, "F = toggle camera");
				y += 20;
				drawText(x, y, 16, 0, 0, "R = reset magnet history, T = center magnet, H = center gyro");
				y += 20;
				drawText(x, y, 16, 0, 0, "G = GYRO CALIBRATION MODE, M = MAGNET CALIBRATION MODE");
				y += 20;
			}
			framework.endDraw();
		}
		while (!keyboard.wentDown(SDLK_ESCAPE));
		
		imu.shut();
		
	#if ENABLE_OSC && 0
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
		
		close(port);
	}
	
	//exit(0);
}

#include "Calc.h"
#include "framework.h"
#include "Noise.h"

#define GFX_SX 1600
#define GFX_SY 1200

struct Camera
{
	Vec3 position;
	Vec3 rotation;

	Camera()
		: position()
		, rotation()
	{
	}

	void tick(const float dt)
	{
		rotation[0] -= mouse.dy / 100.f;
		rotation[1] -= mouse.dx / 100.f;

		if (gamepad[0].isConnected)
		{
			rotation[0] -= gamepad[0].getAnalog(1, ANALOG_Y) * dt;
			rotation[1] -= gamepad[0].getAnalog(1, ANALOG_X) * dt;
		}

		Mat4x4 mat;

		calculateTransform(mat);

		const Vec3 xAxis(mat(0, 0), mat(0, 1), mat(0, 2));
		const Vec3 zAxis(mat(2, 0), mat(2, 1), mat(2, 2));

		Vec3 direction;

		if (keyboard.isDown(SDLK_UP))
			direction += zAxis;
		if (keyboard.isDown(SDLK_DOWN))
			direction -= zAxis;
		if (keyboard.isDown(SDLK_LEFT))
			direction -= xAxis;
		if (keyboard.isDown(SDLK_RIGHT))
			direction += xAxis;

		if (gamepad[0].isConnected)
		{
			direction -= zAxis * gamepad[0].getAnalog(0, ANALOG_Y);
			direction += xAxis * gamepad[0].getAnalog(0, ANALOG_X);
		}

		const float speed = 200.f;

		position += direction * speed * dt;
	}

	void calculateTransform(Mat4x4 & matrix) const
	{
		// todo : use the correct eye position when we're trying to do head mounted VR
		// right now the anatomy looks like this:
		//
		//      L----O----R
		//           |
		//           |
		//           |
		//
		// where L is the left eye, R is the right eye and O is where the head rotates around the axis
		// in real life, the head and eyes rotate a little more complicated..

		matrix = Mat4x4(true).Translate(position).RotateY(rotation[1]).RotateX(rotation[0]);
	}
};

struct QuirkyRotator
{
	float angle;
	float speed;
	float reliability;
	float horror;

	QuirkyRotator(const float _speed, const float _reliability, const float _horror)
		: angle(0.f)
		, speed(_speed)
		, reliability(_reliability)
		, horror(_horror)
	{
	}

	void tick(const float dt)
	{
		const float noise = scaled_octave_noise_1d(
				8.f,
				Calc::Lerp(.5f, .1f, horror),
				Calc::Lerp(.5f, 1.f, horror),
				0.f,
				1.f,
				angle * Calc::Lerp(1.f, 50.f, horror));

		const float relativeSpeed = Calc::Lerp(noise, 1.f, reliability);

		angle += relativeSpeed * speed * dt;
	}
};

static void drawGrid(const int numQuadsX, const int numQuadsY)
{
	const float stepX = 1.f / (numQuadsX + 1);
	const float stepY = 1.f / (numQuadsY + 1);

	gxBegin(GL_QUADS);
	{
		for (int x = 0; x < numQuadsX; ++x)
		{
			for (int y = 0; y < numQuadsY; ++y)
			{
				const float x1 = (x + 0) * stepX - .5f;
				const float y1 = (y + 0) * stepY - .5f;
				const float x2 = (x + 1) * stepX - .5f;
				const float y2 = (y + 1) * stepY - .5f;

				gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
				gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
				gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
				gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
			}
		}
	}
	gxEnd();
}

static void drawPicture(const Mat4x4 & pictureTransform, const Mat4x4 & lineTransform, const GLuint texture)
{
	gxPushMatrix();
	{
		gxMultMatrixf(pictureTransform.m_v);

		Shader shader("picture");
		shader.setTexture("texture", 0, texture, true, true);
		setShader(shader);
		{
			drawRect(0.f, 0.f, 1.f, 1.f);
		}
		clearShader();
	}
	gxPopMatrix();

	gxPushMatrix();
	{
		gxMultMatrixf(lineTransform.m_v);

		setColor(colorWhite);
		drawRectLine(0.f, 0.f, 1.f, 1.f);
	}
	gxPopMatrix();
}

int main(int argc, char * argv[])
{
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		mouse.setRelative(true);

		Surface surface(GFX_SX, GFX_SY, false, true);

		Surface surface1(400, 400, false, false);

		Camera camera;
		camera.position[1] = 100.f;
		camera.position[2] = -150.f;

		QuirkyRotator rotator(1.f, .1f, .6f);

		const int kMaxLinesSqrt = 80;
		const int kMaxLines = kMaxLinesSqrt * kMaxLinesSqrt;
		Vec3 lineCoords[kMaxLines * 2];

		int mode = -1;

		while (!framework.quitRequested)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			const int oldMode = mode;

			if (mode == -1)
				mode = 0;

			if (keyboard.wentDown(SDLK_1))
				mode = 0;
			if (keyboard.wentDown(SDLK_2))
				mode = 1;
			if (keyboard.wentDown(SDLK_3))
				mode = 2;
			if (keyboard.wentDown(SDLK_4))
				mode = 3;

			//

			const float dt = framework.timeStep;
			const float time = framework.time;

			camera.tick(dt);

			rotator.tick(dt);

			if (mode != oldMode)
			{
				if (mode == 0)
				{
					for (int i = 0; i < kMaxLines * 2; ++i)
						lineCoords[i] = Vec3(random(0.f, 1.f), random(0.f, 1.f), 0.f);
				}
				else if (mode == 1)
				{
					for (int i = 0; i < kMaxLines; ++i)
					{
						const float angle = random(0.f, Calc::m2PI);
						const float radius1 = 1.f;
						const float radius2 = random(0.f, radius1);
						const float x1 = std::cosf(angle) * radius1 + .5f;
						const float y1 = std::sinf(angle) * radius1 + .5f;
						const float x2 = std::cosf(angle) * radius2 + .5f;
						const float y2 = std::sinf(angle) * radius2 + .5f;
						lineCoords[i * 2 + 0] = Vec3(x1, y1, 0.f);
						lineCoords[i * 2 + 1] = Vec3(x2, y2, 0.f);
					}
				}
				else if (mode == 2)
				{
					const float step = 1.f / (kMaxLinesSqrt - 1.f);
					for (int x = 0; x < kMaxLinesSqrt; ++x)
					{
						for (int y = 0; y < kMaxLinesSqrt; ++y)
						{
							const int index = x + y * kMaxLinesSqrt;

							if (index < kMaxLines)
							{
								lineCoords[index * 2 + 0] = Vec3(x * step, y * step, 0.f);
								lineCoords[index * 2 + 1] = Vec3(x * step, y * step, 0.f);
							}
						}
					}
				}
				else if (mode == 3)
				{
					for (int i = 0; i < kMaxLines; ++i)
					{
						const float angle = random(0.f, Calc::m2PI);
						const float radius1 = 1.f;
						const float radius2 = random(0.f, radius1);
						float x1 = std::cosf(angle) * radius1;
						float y1 = std::sinf(angle) * radius1;
						const float x2 = std::cosf(angle) * radius2 + .5f;
						const float y2 = std::sinf(angle) * radius2 + .5f;
						const float sx = radius1 / x1;
						const float sy = radius1 / y1;
						const float ss = std::min(std::abs(sx), std::abs(sy)) / 2.f;
						x1 *= ss;
						y1 *= ss;
						x1 += .5f;
						y1 += .5f;
						lineCoords[i * 2 + 0] = Vec3(x1, y1, 0.f);
						lineCoords[i * 2 + 1] = Vec3(x2, y2, 0.f);
					}
				}
			}

			framework.beginDraw(0, 0, 0, 0);
			{
				pushSurface(&surface);
				{
					surface.clear(0, 0, 0);
					surface.clearDepth(1.f);

					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LESS);

					setBlend(BLEND_OPAQUE);

					Mat4x4 matP;
					matP.MakePerspectiveLH(Calc::DegToRad(60.f), surface.getHeight() / float(surface.getWidth()), 1.f, 10000.f);
					
					Mat4x4 matC(true);
					camera.calculateTransform(matC);
					matC = matC.Invert();

					gxMatrixMode(GL_PROJECTION);
					gxPushMatrix();
					{
						gxLoadMatrixf(matP.m_v);
						gxMultMatrixf(matC.m_v);

						gxMatrixMode(GL_MODELVIEW);
						gxPushMatrix();
						{
							gxLoadIdentity();

							//

							gxPushMatrix();
							{
								gxRotatef(90.f, -1.f, 0.f, 0.f);

								const float scale = 1000.f;
								gxScalef(scale, scale, scale);

								const bool wireMode = true;

								glPolygonMode(GL_FRONT_AND_BACK, wireMode ? GL_LINE : GL_FILL);
								checkErrorGL();

								setBlend(BLEND_OPAQUE);
								setColor(127, 127, 127);
								drawGrid(40, 40);

								glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
								checkErrorGL();
							}
							gxPopMatrix();

							//

							const float scale1 = 300.f;
							const float scale2 = 200.f;

							Mat4x4 matM1 = Mat4x4(true).Translate(-150.f, 0.f, 0.f).Scale(scale1, scale1, scale1).RotateZ(framework.time * .1765f).RotateX(framework.time * .1543f).Translate(-.5f, -.5f, 0.f);
							Mat4x4 matM2;
							if (mode == 0 || mode == 2)
								matM2 = Mat4x4(true).Translate(+150.f, 0.f, 0.f).Scale(scale2, scale2, scale2).RotateZ(framework.time * .1654f).RotateY(framework.time * .1432f).Translate(-.5f, -.5f, 0.f);
							else
								matM2 = matM1.Translate(0.f, 0.f, .02f);

							Mat4x4 matT1 = matM1.Translate(std::sinf(framework.time * .345f) * .5f, 0.f, 0.f);
							Mat4x4 matT2;
							if (mode == 0 || mode == 2)
								matT2 = matM2.Translate(std::sinf(framework.time * .456f) * .5f, 0.f, 0.f);
							else
								matT2 = matM2.Translate(std::sinf(framework.time * .345f) * .5f, 0.f, 0.f);

							const Mat4x4 matC1 = matT1.Invert() * matM1;
							const Mat4x4 matC2 = matT2.Invert() * matM2;

							pushSurface(&surface1);
							{
								surface1.clear(255, 255, 255);

								setBlend(BLEND_ALPHA);

								gxPushMatrix();
								{
									gxTranslatef(surface1.getWidth()/2.f, surface1.getHeight()/2.f, 0.f);

									for (int i = 0; i < 5; ++i)
									{
										setColor(colorBlack);
										const float f = i + 1;
										const float x = std::cosf(time * f) * 100.f;
										const float y = std::sinf(time * f) * 100.f;
										fillCircle(x, y, 50, 100);
										const std::string text = "Andra";
										if (i < text.length())
										{
											setColor(colorWhite);
											setFont("calibri.ttf");
											drawText(x, y, 32, 0.f, -1.f, "%c", text[i]);
										}
									}
								}
								gxPopMatrix();
							}
							popSurface();

							const GLuint texture1 = getTexture("cat.jpg");
							//const GLuint texture1 = surface1.getTexture();
							//const GLuint texture2 = getTexture("cat.jpg");
							const GLuint texture2 = surface1.getTexture();

							drawPicture(matT1, matM1, texture1);
							drawPicture(matT2, matM2, texture2);

							//

							Shader shader("lines");
							setShader(shader);
							shader.setTexture("texture1", 0, texture1, true, true);
							shader.setTexture("texture2", 1, texture2, true, true);
							shader.setImmediate("testOr", (mode == 1 || mode == 3) ? 1.f : 0.f, 0.f);
							{
								gxBegin(GL_LINES);
								{
									for (int i = 0; i < kMaxLines; ++i)
									{
										const Vec3 & c1 = lineCoords[i * 2 + 0];
										const Vec3 & c2 = lineCoords[i * 2 + 1];

										const Vec3 t1 = matC1.Mul4(c1);
										const Vec3 t2 = matC2.Mul4(c2);

										const Vec3 p1 = matM1.Mul4(c1);
										const Vec3 p2 = matM2.Mul4(c2);

										gxColor4f(t1[0], 1.f - t1[1], t2[0], 1.f - t2[1]);

										gxVertex3f(p1[0], p1[1], p1[2]);
										gxVertex3f(p2[0], p2[1], p2[2]);
									}
								}
								gxEnd();
							}
							clearShader();
						}
						gxMatrixMode(GL_MODELVIEW);
						gxPopMatrix();
					}
					gxMatrixMode(GL_PROJECTION);
					gxPopMatrix();

					glDisable(GL_DEPTH_TEST);

					//

					gxPushMatrix();
					{
						gxTranslatef(surface.getWidth()/2.f, surface.getHeight()/2.f, 0.f);
						gxScalef(100.f, 100.f, 100.f);
						gxRotatef(Calc::RadToDeg(rotator.angle), 0.f, 0.f, 1.f);

						setColor(colorWhite);
						drawRect(-1.f, -1.f, +1.f, +1.f);
						for (int i = 0; i < 10; ++i)
						{
							gxScalef(.9f, .9f, .9f);
							setColor(colorBlack);
							drawRectLine(-1.f, -1.f, +1.f, +1.f);
						}
					}
					gxPopMatrix();
				}
				popSurface();

				gxSetTexture(surface.getTexture());
				{
					setBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				gxSetTexture(0);
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}

#include <GL/gl.h>
#include <SDL/SDL.h>
#include "Event_SDL.h"
#include "EventManager.h"
#include "Gui.h"
#include "TextureRGBA.h"

class MySystem : public Gui::ISystem
{
public:
	size_t ImageWidth(ImageHandle image)
	{
		return 10;
	}

	size_t ImageHeight(ImageHandle image)
	{
		return 10;
	}
	
	size_t FontHeight(FontHandle font)
	{
		return 10;
	}

	size_t FontWidth(FontHandle font, const char* text)
	{
		return 10;
	}
};

class TextureCache
{
public:
	TextureCache()
	{
	}

	~TextureCache()
	{
	}

	GLuint GetTexture(const char* name)
	{
		std::map<std::string, GLuint>::iterator i = mTextureList.find(name);

		if (i != mTextureList.end())
			return i->second;

		int sx = 256;
		int sy = 256;
		uint8_t* bytes = new uint8_t[sx * sy * 4];
		for (int i = 0; i < sx * sy * 4; ++i)
			bytes[i] = rand();
		TextureRGBA* texture = new TextureRGBA(sx, sy, bytes, true);
		
		GLuint textureId = 0;

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->Sx_get(), texture->Sy_get(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->Bytes_get());

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		delete texture;
		texture = 0;

		mTextureList[name] = textureId;

		return textureId;
	}

private:
	std::map<std::string, GLuint> mTextureList;
};

TextureCache* gTextureCache;

class MyCanvas : public Gui::Graphics::ICanvas
{
public:
	virtual MatrixStack& GetMatrixStack()
	{
		return mMatrixStack;
	}

	virtual Gui::Graphics::RectStack& GetVisibleRectStack()
	{
		return mVisibleRectStack;
	}

	virtual void DrawImage(int x, int y, const Gui::Graphics::Image& image)
	{
		DrawImageStretch(x, y, image.GetWidth(), image.GetHeight(), image);
	}

	virtual void DrawImageScale(int x, int y, float scale, const Gui::Graphics::Image& image)
	{
		DrawImageStretch(x, y, (int)(image.GetWidth() * scale), (int)(image.GetHeight() * scale), image);
	}

	virtual void DrawImageStretch(int x, int y, int width, int height, const Gui::Graphics::Image& image)
	{
		UpdateMatrix();
		UpdateVisibleRect();
		UpdateBlend(image.IsTransparent());

		//throw ExceptionNA();

		GLuint textureId = gTextureCache->GetTexture(image.GetName().c_str());

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureId);

		float x1 = (float)x;
		float y1 = (float)y;
		float x2 = (float)x + width;
		float y2 = (float)y + height;

		glBegin(GL_QUADS);
		glColor3f(1.0f, 1.0f, 1.0f);
		glTexCoord2f(0.0f, 0.0f);
		glVertex3f(x1, y1, 0.0f);
		glTexCoord2f(1.0f, 0.0f);
		glVertex3f(x2, y1, 0.0f);
		glTexCoord2f(1.0f, 1.0f);
		glVertex3f(x2, y2, 0.0f);
		glTexCoord2f(0.0f, 1.0f);
		glVertex3f(x1, y2, 0.0f);
		glEnd();
	}

	virtual void Rect(int x1, int y1, int x2, int y2, Gui::Graphics::Color color)
	{
		UpdateMatrix();
		UpdateVisibleRect();
		UpdateBlend(color.a != 1.0f);

		glBegin(GL_LINE_LOOP);
		glColor3f(color.r, color.g, color.b);
		glVertex3f((float)x1, (float)y1, 0.0f);
		glVertex3f((float)x2, (float)y1, 0.0f);
		glVertex3f((float)x2, (float)y2, 0.0f);
		glVertex3f((float)x1, (float)y2, 0.0f);
		glEnd();
	}

	virtual void FilledRect(int x1, int y1, int x2, int y2, Gui::Graphics::Color color)
	{
		UpdateMatrix();
		UpdateVisibleRect();
		UpdateBlend(color.a != 1.0f);

		glBegin(GL_QUADS);
		glColor3f(color.r, color.g, color.b);
		glVertex3f((float)x1, (float)y1, 0.0f);
		glVertex3f((float)x2, (float)y1, 0.0f);
		glVertex3f((float)x2, (float)y2, 0.0f);
		glVertex3f((float)x1, (float)y2, 0.0f);
		glEnd();
	}

	virtual void BeveledRect(int x1, int y1, int x2, int y2, Gui::Graphics::Color colorLow, Gui::Graphics::Color colorHigh, int distance = 1.0f)
	{
		throw ExceptionNA();
	}

	virtual void Rect3D(int x1, int y1, int x2, int y2, Gui::Graphics::Color colorLow, Gui::Graphics::Color colorHigh, int size = 1)
	{
		throw ExceptionNA();
	}

	virtual void Arc(int x, int y, float radius, float angle1, float angle2, Gui::Graphics::Color color)
	{
		throw ExceptionNA();
	}

	virtual void Circle(int x, int y, float radius, Gui::Graphics::Color color)
	{
		throw ExceptionNA();
	}

	virtual void Text(const Gui::Graphics::Font& font, int x, int y, const std::string& text, Gui::Graphics::Color color, Gui::Alignment alignmentH = Gui::Alignment_Left, Gui::Alignment alignmentV = Gui::Alignment_Top, bool useFontColor = false)
	{
		throw ExceptionNA();
	}

	virtual void Outline(int x, int y, std::vector<Gui::Point>& points, Gui::Graphics::Color color, bool closed = true, Gui::Graphics::ICanvas::PATTERN pattern = Gui::Graphics::ICanvas::PATTERN_NONE, float animationSpeed = 0.0f)
	{
		throw ExceptionNA();
	}

	virtual void FilledOutline(int x, int y, std::vector<Gui::Point>& points, Gui::Graphics::Color color)
	{
		throw ExceptionNA();
	}

private:
	virtual void UpdateMatrix()
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(mMatrixStack.Top().m_v);
	}

	virtual void UpdateVisibleRect()
	{
#if 0
		GraphicsDevice* graphicsDevice = Renderer::I().GetGraphicsDevice();

		Gui::Rect visibleRect = m_visibleRectStack.Top();

		graphicsDevice->SetScissorRect(
			visibleRect.min.x,
			visibleRect.min.y,
			visibleRect.max.x,
			visibleRect.max.y);
#endif
	}

	void UpdateBlend(bool enabled)
	{
		if (enabled)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	MatrixStack mMatrixStack;
	Gui::Graphics::RectStack mVisibleRectStack;
};

class MyWidget : public Gui::Widget
{
public:
	MyWidget() : Widget()
	{
		SetClassName("MyWidget");
		SetCaptureFlag(CAPTURE_MOUSE | CAPTURE_KEYBOARD);

		ADD_EVENT(OnMouseDown,  mOnMouseDown,  (this, HandleMouseDown ));
		ADD_EVENT(OnMouseUp,    mOnMouseUp,    (this, HandleMouseUp   ));
		ADD_EVENT(OnMouseEnter, mOnMouseEnter, (this, HandleMouseEnter));
		ADD_EVENT(OnMouseLeave, mOnMouseLeave, (this, HandleMouseLeave));

		mColor1.r = (rand() & 4095) / 4095.0f;
		mColor1.g = (rand() & 4095) / 4095.0f;
		mColor1.b = (rand() & 4095) / 4095.0f;
		mColor1.a = 1.0f;

		mColor2.r = 1.0f;
		mColor2.g = 0.5f;
		mColor2.b = 0.25f;
		mColor2.a = 1.0f;

		mIsDown = false;
		mIsInside = false;
	}

	virtual Gui::GuiResult Render(Gui::Graphics::ICanvas* canvas)
	{
		Gui::Point size = GetSize();

		Gui::Graphics::Color color;

		if (mIsDown)
			color = mColor2;
		else
			color = mColor1;

		if (!mIsInside)
			for (int i = 0; i < 3; ++i)
				color[i] *= 0.5f;

		canvas->FilledRect(0, 0, size.x, size.y, color);
		canvas->Rect(0, 0, size.x, size.y, mColor2);

		return true;
	}

private:
	static void HandleMouseDown(Gui::Object* me, Gui::MouseButton button, Gui::ButtonState state, Gui::MouseState* mouseState)
	{
		MyWidget* self = (MyWidget*)me;

		self->mIsDown = true;
	}

	static void HandleMouseUp(Gui::Object* me, Gui::MouseButton button, Gui::ButtonState state, Gui::MouseState* mouseState)
	{
		MyWidget* self = (MyWidget*)me;

		self->mIsDown = false;
	}

	static void HandleMouseEnter(Gui::Object* me, Gui::Object* sender)
	{
		MyWidget* self = (MyWidget*)me;

		self->mIsInside = true;
	}

	static void HandleMouseLeave(Gui::Object* me, Gui::Object* sender)
	{
		MyWidget* self = (MyWidget*)me;

		self->mIsInside = false;
	}

	Gui::Graphics::Color mColor1;
	Gui::Graphics::Color mColor2;
	bool mIsDown;
	bool mIsInside;

	Gui::EHMouseButton mOnMouseDown;
	Gui::EHMouseButton mOnMouseUp;
	Gui::EHNotify mOnMouseEnter;
	Gui::EHNotify mOnMouseLeave;
};

class MyWidgetFactory : public Gui::IWidgetFactory
{
public:
	Gui::Widget* Create(const char* name)
	{
		return new MyWidget();
	}
};

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

	SDL_Surface* screen = SDL_SetVideoMode(800, 600, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_ANYFORMAT);

	printf("main\n");

	// todo: setup GUI system

	MySystem system;

	Gui::ISystem::SetCurrent(&system);

	// todo: setup GUI canvas

	gTextureCache = new TextureCache();

	MyCanvas canvas;

	//Gui::Graphics::ICanvas::SetCurrent(&canvas);

	MyWidgetFactory widgetFactory;

	Gui::Context context(&EventManager::I(), &widgetFactory);

	EventManager::I().Enable(EVENT_PRIO_INTERFACE);

	context.SetViewSize(400, 300);

	if (true)
	{
		Gui::Widget* widget = new MyWidget();
		widget->SetPosition(10, 10);
		widget->SetSize(30, 10);
		widget->SetAlignment(Gui::Alignment_Client);
		context.GetRootWidget()->AddChild(widget);
	}
	{
		Gui::Widget* widget = new MyWidget();
		widget->SetPosition(20, 20);
		widget->SetSize(100, 100);
		widget->SetAnchorMask(Gui::Anchor_All);
		context.GetRootWidget()->AddChild(widget);
	}
	{
		Gui::Widget* widget = new MyWidget();
		widget->SetPosition(150, 150);
		widget->SetSize(10, 10);
		widget->SetAnchorMask(Gui::Anchor_Left | Gui::Anchor_Right | Gui::Anchor_Bottom);
		context.GetRootWidget()->AddChild(widget);
	}
	{
		Gui::Widget* widget = new MyWidget();
		widget->SetPosition(10, 10);
		widget->SetSize(30, 10);
		widget->SetAlignment(Gui::Alignment_Left);
		context.GetRootWidget()->AddChild(widget);
	}
	{
		Gui::Widget* widget = new MyWidget();
		widget->SetPosition(10, 10);
		widget->SetSize(30, 10);
		widget->SetAlignment(Gui::Alignment_Top);
		context.GetRootWidget()->AddChild(widget);
	}

	for (int i = 0; i < 10; ++i)
	{
		Gui::Widget* widget = new MyWidget();
		widget->SetPosition(rand() % 300, rand() % 200);
		widget->SetSize(20, 10);
		context.GetRootWidget()->AddChild(widget);
	}

	{
		Gui::WidgetImage* widget = new Gui::WidgetImage();
		widget->SetPosition(10, 10);
		widget->SetSize(100, 100);
		widget->SetAnchorMask(Gui::Anchor_Left | Gui::Anchor_Right | Gui::Anchor_Bottom);
		widget->SetCaptureFlag(Gui::Widget::CAPTURE_ALL);
		Gui::Graphics::Image image;
		image.Setup("test.png", true);
		widget->SetImage(image);
		widget->SetScaleMode(Gui::WidgetImage::ScaleMode_Fit);
		context.GetRootWidget()->AddChild(widget);
	}

	//context.SetViewSize(300, 300);

	Event_SDL eventSDL;

	bool stop = false;

	bool grow = false;
	bool shrink = false;

	while (!stop)
	{
		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			Event events[4];
			int count = eventSDL.TranslateEvent(e, events, 4);
			for (int i = 0; i < count; ++i)
				EventManager::I().AddEvent(events[i]);

			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				stop = true;
			if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && e.key.keysym.sym == SDLK_SPACE)
				grow = e.key.state != 0;
			if ((e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) && e.key.keysym.sym == SDLK_a)
				shrink = e.key.state != 0;
		}

		EventManager::I().Purge();

		if (grow)
			context.SetViewSize(context.GetViewWidth() + 1, context.GetViewHeight() + 1);
		if (shrink)
			context.SetViewSize(context.GetViewWidth() - 1, context.GetViewHeight() - 1);

		/*Event e2(EVT_MOUSEBUTTON, 0, 1, 50, 50);
		EventManager::I().AddEvent(e2);
		EventManager::I().Purge();*/

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0f, screen->w, screen->h, 0.0f, -1000.0f, +1000.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		context.Render(&canvas);

		SDL_GL_SwapBuffers();

		//printf("size: %d, %d\n", widget->GetSize().x, widget->GetSize().y);
	}

	SDL_FreeSurface(screen);

	SDL_Quit();

	return 0;
}

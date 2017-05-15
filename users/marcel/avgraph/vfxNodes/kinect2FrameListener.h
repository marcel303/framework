#include <libfreenect2/frame_listener_impl.h> // FrameMap

struct SDL_cond;
struct SDL_mutex;

struct DoubleBufferedFrameListener : public libfreenect2::FrameListener
{
	SDL_mutex * mutex;

	int frameTypes;

	libfreenect2::Frame * video;
	libfreenect2::Frame * depth;

	DoubleBufferedFrameListener(int frameTypes);
	virtual ~DoubleBufferedFrameListener() override;

	virtual bool onNewFrame(libfreenect2::Frame::Type type, libfreenect2::Frame * frame) override;

	void lockBuffers();
	void unlockBuffers();
};

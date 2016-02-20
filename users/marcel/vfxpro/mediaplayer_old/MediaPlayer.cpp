#include "MediaPlayer.h"
#include "MPDebug.h"
#include "Renderer.h"

// FIXME: State.m_done gets sets.. At EOF, but theres still some AV data left in the queues..
//        Empty queues, empty audio.. Then set m_done to true.

// TODO: Resync audio. When packet received.. Update timestamp. Calculate difference timestamp
//       and time oldest update & current play cursor. Use this difference to correct clock.

// Sound stream provider for media player.
class SoundStreamProviderMP : public SoundStreamProvider
{
public:
	SoundStreamProviderMP(MP::Context* context) : SoundStreamProvider()
	{
		SetBufferSize(BUFFER_SIZE);
		SetBuffer(m_buffer);

		m_soundDevice = 0;

		m_context = context;
		m_channelCount = context->GetAudioChannelCount();
		m_frameCount = BUFFER_SIZE / m_channelCount / sizeof(int16_t);

		m_updated = false;
	}

	virtual bool CalculateSamples(size_t& out_size)
	{
		double timeAudio = m_context->m_audioContext->GetTime();
		UpdateTiming(timeAudio);

		bool gotAudio;
		m_context->RequestAudio((int16_t*)m_buffer, m_frameCount, gotAudio);
		Assert(gotAudio);

		out_size = BUFFER_SIZE;

		m_updated = true;

		return true;
	}

	void UpdateTiming(double timeAudio)
	{
		m_timing[2] = m_timing[1];
		m_timing[1] = m_timing[0];

		m_timing[0].timeAudio = timeAudio;
		m_timing[0].timeStream = m_soundDevice->GetPlaybackPosition(m_source);

		m_timeCorrection = m_timing[2].timeAudio - m_timing[0].timeStream;

		MP::Debug::Print("Sync = %03.3f. Phys = %03.3f. Correction = %+03.3f.\n",
			m_timing[0].timeAudio,
			m_timing[0].timeStream,
			m_timeCorrection);
	}

	// TODO: Make buffer size dependent on queue sizes, sample rates, etc.
	//       Currently, large buffers cause packets queues to fill up, stuff getting out of sync.
	const static int BUFFER_SIZE = 44000 * 2 * 2;
	//const static int BUFFER_SIZE = 20000;

	SoundDevice* m_soundDevice;
	SharedResourceSoundSource m_source;
	MP::Context* m_context;

	size_t m_channelCount;
	size_t m_frameCount;
	char m_buffer[BUFFER_SIZE];

	struct
	{
		double timeAudio;  ///< Time of decoder (audio PTS).
		double timeStream; ///< Time of stream sound.
	} m_timing[3];

	double m_timeCorrection;

	bool m_updated;
};

/**
 * Constructor.
 */
MediaPlayer::MediaPlayer()
{
	m_opened = false;
	m_playing = false;

	m_soundDevice = 0;
	m_graphicsDevice = 0;
	m_soundProvider = 0;

	// Use lowest input listener priority available.
	// This will let the media player always take precedence.
	InputListener::SetPriority(0);
}

/**
 * Destructor.
 */
MediaPlayer::~MediaPlayer()
{
	if (m_opened == true)
	{
		Close();
		Assert(0);
	}
}

/**
 * Open a media file. The provided graphics and sound devices are used to render the audio & video output.
 */
bool MediaPlayer::Open(const std::string& filename)
{
	Assert(m_opened == false);

	bool result = true;

	m_opened = true;

	if (m_mpContext.Begin(filename) != true)
		return false;

	if (m_mpContext.HasAudioStream())
		m_processAudio = true;
	else
		m_processAudio = false;

	if (m_mpContext.HasVideoStream())
		m_processVideo = true;
	else
		m_processVideo = false;

	// Prefill buffers.
	if (m_mpContext.FillBuffers() != true)
		result = false;

	if (m_processAudio)
	{
		// Setup stream, provider.
		m_soundProvider = new SoundStreamProviderMP(&m_mpContext);

		m_soundStream = SharedResourceSoundStream(new ResourceSoundStream);
		m_soundStream->SetSampleCount(m_soundProvider->GetBufferSize() / sizeof(int16_t));
		m_soundStream->SetChannelCount(m_mpContext.GetAudioChannelCount());
		m_soundStream->SetNumQueueBuffers(2);
		m_soundStream->SetSampleRate(m_mpContext.GetAudioFrameRate());
		m_soundStream->SetSoundStreamProvider(m_soundProvider);

		m_soundSource = SharedResourceSoundSource(new ResourceSoundSource);
		m_soundSource->SetSound(SharedResourceSound(m_soundStream));
	}

	if (m_processVideo)
	{
		// Allocate texture.
		size_t textureWidth;
		size_t textureHeight;

		CalculateNearestPowerOfTwo(
			m_mpContext.GetVideoWidth(),
			m_mpContext.GetVideoHeight(),
			textureWidth,
			textureHeight);

		m_texture = SharedTexture(new ResourceTexture);
		m_texture->SetSize(textureWidth, textureHeight);

		MP::Debug::Print("Created %dx%d texture.\n", static_cast<int>(textureWidth), static_cast<int>(textureHeight));

		// Create quad.
		m_mesh.Initialize(GraphicsDevice::PT_TRIANGLE_FAN, false, size_t(4), VertexBuffer::FVF_XYZ | VertexBuffer::FVF_TEX1);
		VertexBuffer& vb = m_mesh.GetVertexBuffer();
		float u1, v1, u2, v2;
		GetVideoRect(u1, v1, u2, v2);
		vb.SetPosition3(0, 0.0f, 0.0f, 0.0f);
		vb.SetPosition3(1, 1.0f, 0.0f, 0.0f);
		vb.SetPosition3(2, 1.0f, 1.0f, 0.0f);
		vb.SetPosition3(3, 0.0f, 1.0f, 0.0f);
		vb.SetTexcoord(0, 0, u1, v1);
		vb.SetTexcoord(1, 0, u2, v1);
		vb.SetTexcoord(2, 0, u2, v2);
		vb.SetTexcoord(3, 0, u1, v2);

		vb.SetOutdated();
	}

	return result;
}
/**
 * Close the media file.
 */
bool MediaPlayer::Close()
{
	Assert(m_opened == true);

	if (m_playing == true)
		Stop();

	m_opened = false;

	if (m_processAudio)
	{
		m_soundSource = SharedResourceSoundSource(static_cast<ResourceSoundSource*>(0));
		m_soundStream = SharedResourceSoundStream(static_cast<ResourceSoundStream*>(0));
		delete m_soundProvider;
	}

	if (m_processVideo)
	{
		m_texture = SharedTexture(static_cast<ResourceTexture*>(0));

		m_mesh.Initialize(GraphicsDevice::PT_TRIANGLE_FAN, false, 0, 0);
	}

	if (m_mpContext.End() != true)
		return false;

	return true;
}

/**
 * Update the playback of the opened media file. out_state captures whether new audio or video was generated and whether the file has finished playing.
 */
bool MediaPlayer::Update(State& out_state)
{
	bool result = true;

	m_state.m_audioUpdated = false;
	m_state.m_videoUpdated = false;

	if (m_mpContext.FillBuffers() != true)
		result = false;

	if (m_processAudio)
	{
		if (m_soundDevice->Update() != true)
			result = false;

		m_state.m_audioUpdated = m_soundProvider->m_updated;
		m_soundProvider->m_updated = false;
	}

	if (m_processVideo)
	{
		double time = GetTime();
		MP::VideoFrame* frame;

		bool gotVideo;
		if (m_mpContext.RequestVideo(time, &frame, gotVideo) != true)
			result = false;
		
		if (gotVideo)
		{
			MP::Debug::Print("VIDEO: Requested: %03.3f. Got: %03.3f.\n", float(time), float(frame->m_time));

			FrameToTexture(frame, m_texture.get());

			m_state.m_videoUpdated = true;
			m_state.m_videoPts = frame->m_time;
		}
	}

	m_state.m_done = m_mpContext.HasReachedEOF();

	out_state = m_state;

	return result;
}

bool MediaPlayer::Play(SoundDevice* soundDevice, GraphicsDevice* graphicsDevice)
{
	Assert(m_playing == false);

	bool result = true;

	m_playing = true;

	m_soundDevice = soundDevice;
	m_graphicsDevice = graphicsDevice;

	// Decide whether to process audio & video.
	if (m_mpContext.HasAudioStream() && m_soundDevice)
		m_processAudio = true;
	else
		m_processAudio = false;

	if (m_mpContext.HasVideoStream())
		m_processVideo = true;
	else
		m_processVideo = false;

	m_state.m_time = 0.0;
	m_state.m_done = false;
	m_timer.Restart();

	// Start audio.
	if (m_processAudio)
	{
		m_soundProvider->m_soundDevice = m_soundDevice;
		m_soundProvider->m_source = m_soundSource;

		if (m_soundDevice->PlaySoundStream(m_soundSource) != true)
			result = false;
	}

	return result;
}

bool MediaPlayer::Stop()
{
	Assert(m_playing == true);

	bool result = true;

	m_playing = false;

	// Stop audio.
	if (m_processAudio)
		if (m_soundDevice->StopSound(m_soundSource) != true)
			result = false;

	m_graphicsDevice = 0;
	m_soundDevice = 0;

	return result;
}

/**
 * Play media file until finish. The media player takes full control over the system. If specified, the user may cancel the playback.
 */
bool MediaPlayer::PlayUntillFinish(Display* display, SoundDevice* soundDevice, GraphicsDevice* graphicsDevice, bool userCanStopPlay)
{
	bool result = true;

	InputManager::I().RegisterInputListener(this);

	if (Play(soundDevice, graphicsDevice) != true)
		return false;

	m_stop = false;
	m_restart = false;

	bool stop = false;

	while (stop == false)
	{
		// FIXME: Need OS independent sleep or smt.. Or, consume 100% CPU, but some CPU coolers might start to blow heavily.
		#if defined(SYSTEM_WINDOWS)
		Sleep(1);
		#endif

		// FIXME: What if InputManager requires updates of other objects to generate input?
		InputManager::I().Update(0);

		if (display)
			display->Update();

		/*
		TODO: Restart not yet implemented.
		      Not of much importance really.

		if (m_restart)
		{
			Stop();

			if (m_mpContext.SeekToStart() != true)
				result = false;

			Play(soundDevice, graphicsDevice);

			m_restart = false;
		}
		*/

		if (m_stop)
			stop = true;

		if (Update(m_state) != true)
		{
			result = false;
			stop = true;
		}

		if (m_state.m_done)
			stop = true;

		if (m_state.m_videoUpdated)
		{
			if (m_graphicsDevice)
			{
				float sizeX = static_cast<float>(m_graphicsDevice->GetBackBufferWidth());
				float sizeY = static_cast<float>(m_graphicsDevice->GetBackBufferHeight());

				float videoSizeX = static_cast<float>(m_mpContext.GetVideoWidth());
				float videoSizeY = static_cast<float>(m_mpContext.GetVideoHeight());

				float scaleX = sizeX / videoSizeX;
				float scaleY = sizeY / videoSizeY;

				float scale = std::min<float>(scaleX, scaleY);

				float scaledSizeX = videoSizeX * scale;
				float scaledSizeY = videoSizeY * scale;

				float offsetX = (sizeX - scaledSizeX) / 2.0f;
				float offsetY = (sizeY - scaledSizeY) / 2.0f;

				m_graphicsDevice->Clear(GraphicsDevice::BT_ALL, 0.0f, 0.0f, 0.0f, 1.0f, 0x00);
				m_graphicsDevice->BeginRender();

				m_graphicsDevice->SetRenderState(GraphicsDevice::RS_DEPTHTEST, GraphicsDevice::RSV_FALSE);
				m_graphicsDevice->SetSamplerState(0, GraphicsDevice::SS_ADDRESSU, GraphicsDevice::SSV_CLAMP);
				m_graphicsDevice->SetSamplerState(0, GraphicsDevice::SS_ADDRESSV, GraphicsDevice::SSV_CLAMP);

				Matrix matW;
				Matrix matV;
				Matrix matP;

				Matrix matWS;
				Matrix matWT;

				matWS.MakeScaling(Vector(scale * videoSizeX, scale * videoSizeY, 1.0f));
				matWT.MakeTranslation(Vector(offsetX, offsetY, 0.0f));

				matW = matWT * matWS;
				matV.MakeTranslation(Vector(0.0f, 0.0f, 1.0f));
				matP.MakeOrthoOffCenterLH(0.0f, sizeX, 0.0f, sizeY, 0.1f, 10.0f);

				m_graphicsDevice->SetTransform(GraphicsDevice::MT_WORLD, matW);
				m_graphicsDevice->SetTransform(GraphicsDevice::MT_VIEW, matV);
				m_graphicsDevice->SetTransform(GraphicsDevice::MT_PROJECTION, matP);

				//const float t1 = m_timer.GetTime();
				m_graphicsDevice->SetTexture(0, m_texture.get());
				//const float t2 = m_timer.GetTime();

				//printf("SetTexture took %f MS.\n", (t2 - t1) * 1000.0f);

				Renderer::I().SetGraphicsDevice(m_graphicsDevice);
				Renderer::I().RenderMesh(m_mesh);

				m_graphicsDevice->SetSamplerState(0, GraphicsDevice::SS_ADDRESSU, GraphicsDevice::SSV_WRAP);
				m_graphicsDevice->SetSamplerState(0, GraphicsDevice::SS_ADDRESSV, GraphicsDevice::SSV_WRAP);
				m_graphicsDevice->SetRenderState(GraphicsDevice::RS_DEPTHTEST, GraphicsDevice::RSV_TRUE);

				m_graphicsDevice->EndRender();

				while (GetTime() < m_state.m_videoPts);
				m_graphicsDevice->Present();
				MP::Debug::Print("VIDEO: Present time: %03.3f.\n", float(GetTime()));
			}
		}
	}

	if (Stop() != true)
		return false;

	InputManager::I().RegisterInputListener(this);

	return result;
}

/**
 * Return true if the opened media file contains audio.
 */
bool MediaPlayer::HasAudio()
{
	return m_mpContext.HasAudioStream();
}

/**
 * Return true if the opened media file contains video.
 */
bool MediaPlayer::HasVideo()
{
	return m_mpContext.HasVideoStream();
}

double MediaPlayer::GetTime()
{
	double result = 0.0;

	if (m_processAudio)
	{
		result = m_soundDevice->GetPlaybackPosition(m_soundSource) + m_soundProvider->m_timeCorrection;
	}
	else if (m_processVideo)
		result = m_timer.GetTime();

	return result;
}

/**
 * Return the size of the video in pixels.
 */
bool MediaPlayer::GetVideoSize(
	size_t& out_width,
	size_t& out_height)
{
	bool result = true;

	if (m_processVideo == false)
		result = false;
	else
	{
		out_width = m_mpContext.GetVideoWidth();
		out_height = m_mpContext.GetVideoHeight();
	}

	return result;
}

/**
 * Return the video texture & the UV coordinates used to map the video image.
 */
bool MediaPlayer::GetVideoOutput(
	SharedTexture& out_texture)
{
	bool result = true;

	if (m_processVideo == false)
		result = false;
	else
	{
		out_texture = m_texture;
	}

	return result;
}

bool MediaPlayer::GetVideoRect(
	float& out_u1,
	float& out_v1,
	float& out_u2,
	float& out_v2)
{
	bool result = true;

	if (m_processVideo == false)
		result = false;
	else
	{
		size_t width;
		size_t height;

		GetVideoSize(width, height);

		out_u1 = 0.0f;
		out_v1 = 0.0f;
		out_u2 = 1.0f * width / m_texture->GetWidth();
		out_v2 = 1.0f * height / m_texture->GetHeight();
	}

	return result;
}

/**
 * Rescale the video output to the size of the provided texture.
 */
bool MediaPlayer::GetStretchedVideoOutput(RenderTarget* out_renderTarget)
{
	bool result = true;

	if (m_processVideo == false)
		result = false;
	else
	{
		// Set depth test & render target.
		m_graphicsDevice->SetRenderTarget(out_renderTarget);
		m_graphicsDevice->SetRenderState(GraphicsDevice::RS_DEPTHTEST, GraphicsDevice::RSV_FALSE);

		Matrix oldW;
		Matrix oldV;
		Matrix oldP;

		oldW = m_graphicsDevice->GetTransform(GraphicsDevice::MT_WORLD);
		oldV = m_graphicsDevice->GetTransform(GraphicsDevice::MT_VIEW);
		oldP = m_graphicsDevice->GetTransform(GraphicsDevice::MT_PROJECTION);

		// Set W/V/P matrices.
		Matrix matW;
		Matrix matV;
		Matrix matP;

		matW.MakeIdentity();
		matV.MakeTranslation(Vector(0.0f, 0.0f, 1.0f));
		matP.MakeOrthoOffCenterLH(0.0f, 1.0f, 0.0f, 1.0f, 0.1f, 10.0f);

		m_graphicsDevice->SetTransform(GraphicsDevice::MT_WORLD, matW);
		m_graphicsDevice->SetTransform(GraphicsDevice::MT_VIEW, matV);
		m_graphicsDevice->SetTransform(GraphicsDevice::MT_PROJECTION, matP);

		// Set texture.
		m_graphicsDevice->SetTexture(0, m_texture.get());

		// Render mesh, stretched over entire render target.
		Renderer::I().SetGraphicsDevice(m_graphicsDevice);
		Renderer::I().RenderMesh(m_mesh);

		// Unset texture.
		m_graphicsDevice->SetTexture(0, 0);

		// Restore W/V/P matrices
		m_graphicsDevice->SetTransform(GraphicsDevice::MT_WORLD, matW);
		m_graphicsDevice->SetTransform(GraphicsDevice::MT_VIEW, matV);
		m_graphicsDevice->SetTransform(GraphicsDevice::MT_PROJECTION, matP);

		// Restore depth test & render target.
		m_graphicsDevice->SetRenderState(GraphicsDevice::RS_DEPTHTEST, GraphicsDevice::RSV_TRUE);
		m_graphicsDevice->SetRenderTarget(0);
	}

	return result;
}

bool MediaPlayer::OnKeyEvent(InputEventKey* event)
{
	// FIXME: Input needs KEY_ENTER.
	if (event->keyCode == Input::KEY_ESCAPE || event->key == ' ' || event->keyCode == Input::KEY_SPACE)
	{
		m_stop = true;
	}

	if (event->keyCode == Input::KEY_F10)
	{
		m_restart = true;
	}

	return true;
}

/**
 * Calculate the nearest powers of two >= width & height. Required to create power-of-two texture for video frames.
 */
void MediaPlayer::CalculateNearestPowerOfTwo(size_t width, size_t height, size_t& out_width, size_t& out_height)
{
	out_width = 1;
	out_height = 1;

	while (out_width < width)
		out_width *= 2;
	while (out_height < height)
		out_height *= 2;
}

/**
 * Convert video frame to texture.
 */
void MediaPlayer::FrameToTexture(MP::VideoFrame* frame, ResourceTexture* out_texture)
{
	const int width = static_cast<int>(frame->m_width);
	const int height = static_cast<int>(frame->m_height);

	int y = 0;

	//for (int ty = height; ty != 0; --ty)
	for (int ty = 0; ty < height; ++ty)
	{
		ResourceTexture::Pixel* line = out_texture->GetLine(y);
		const unsigned char* data = frame->m_frame->data[0] + y * frame->m_frame->linesize[0];

		//for (int x = width; x != 0; --x)
		for (int x = width; x != 0; --x)
		{
			line->c = (data[0] << 0) | (data[1] << 8) | (data[2] << 16);

			++line;
			data += 3;
		}

		++y;
	}

	out_texture->SetOutdated();
}
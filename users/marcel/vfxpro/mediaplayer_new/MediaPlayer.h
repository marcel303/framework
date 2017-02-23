#pragma once

#include "GraphicsDevice.h"
#include "InputManager.h"
#include "Mesh.h"
#include "MPContext.h"
#include "ResourceSoundStream.h"
#include "SoundDevice.h"
#include "Types.h"

class SoundStreamProviderMP;

/**
 * MediaPlayer class.
 *
 * Uses MP::Context to read AV data.
 *
 * Usage:
 *7
 * Open:
 *     Open audio/video file.
 *
 * Play or PlayUntillFinish:
 *     Play: Begins playing the movie, outputting audio on the sound device passed along.
 *           Returns immidiately.
 *           The user must continually call Update to make sure the video keeps playing. Methods such as GetVideoOutput & GetVideoRect must be called to (manually) output video.
 *           Usage: video-as-a-texture, or video in a frame rendering.
 *     PlayUntillFinish: Requires a display, sound device & graphics device to be passed along.
 *           Plays the media file from ebgin till end. All updating, and displaying of video is handled by PlayUntillFinish.
 *           Returns when the media file has finished playing.
 *
 * Stop:
 *     Stops playing the media file.
 *
 * Close:
 *     Closes the media file.
 *
 * If omitted, Stop & Close are called automatically in the destructor of MediaPlayer.
 */
class MediaPlayer : public InputListener
{
public:
	class State
	{
	public:
		double m_time;
		double m_videoPts;
		bool m_audioUpdated;
		bool m_videoUpdated;
		bool m_done;
	};

	MediaPlayer();
	~MediaPlayer();

	bool Open(const std::string& filename);             ///< Open a media file. The provided graphics and sound devices are used to render the audio & video output.
	bool Close();                                       ///< Close the media file.
	bool Update(State& out_state);                      ///< Update the playback of the opened media file. out_state captures whether new audio or video was generated and whether the file has finished playing.

	bool Play(
		SoundDevice* soundDevice,
		GraphicsDevice* graphicsDevice);                ///< Start playing the media file. graphicsDevice is optional (required if you want to use GetStretchedVideoOutput). The user must call Update and GetVideoOutput manually to keep playing the media file and presents its video output.
	bool PlayUntillFinish(
		Display* display,
		SoundDevice* soundDevice,
		GraphicsDevice* graphicsDevice,
		bool userCanStopPlay);                          ///< Play media file until finish. The media player takes full control over the system. If specified, the user may cancel the playback.
	bool Stop();                                        ///< Stops the playback of the media file.

	bool HasAudio();                                    ///< Return true if the opened media file contains audio.
	bool HasVideo();                                    ///< Return true if the opened media file contains video.

	double GetTime();                                   ///< Return the time that has elapsed since Start was called.

	bool GetVideoSize(
		size_t& out_width,
		size_t& out_height);                            ///< Return the size of the video in pixels.
	bool GetVideoOutput(SharedTexture& out_texture);    ///< Return the video texture & the UV coordinates used to map the video image.
	bool GetVideoRect(
		float& out_u1,
		float& out_v1,
		float& out_u2,
		float& out_v2);                                 ///< Get the UV coordinates that map the region of the texture where the video is stored.
	bool GetStretchedVideoOutput(RenderTarget* out_renderTarget); ///< Rescale the video output to the size of the provided texture. NOTE: Leaves depth test enabled, texture disabled & render target set to the default back buffer.

	virtual bool OnKeyEvent(InputEventKey* event);

private:
	void CalculateNearestPowerOfTwo(
		size_t width,
		size_t height,
		size_t& out_width,
		size_t& out_height);
	void FrameToTexture(MP::VideoFrame* frame, ResourceTexture* out_texture);

	// Decoding related.
	MP::Context m_mpContext; ///< Media player context.
	State       m_state;     ///< Media player state.
	Timer       m_timer;     ///< Timer.
	bool        m_opened;    ///< True if a media file has been opened.
	bool        m_playing;   ///< True if the media player has started playback.

	// Output devices.
	SoundDevice*    m_soundDevice;    ///< Sound device used for audio output.
	GraphicsDevice* m_graphicsDevice; ///< Graphics device used to stretch video output or used by PlayUntillFinish when playing autonomously.

	// Output related.
	bool m_processAudio; ///< True if audio output ought to be processed.
	bool m_processVideo; ///< True if video output ought to be processed.

	// Audio related output.
	SharedResourceSoundSource m_soundSource;
	SharedResourceSoundStream m_soundStream;
	SoundStreamProviderMP*    m_soundProvider; ///< Custom sound provider. Used to send audio samples from the media player to the sound device.

	// Video related output.
	SharedTexture m_texture; ///< Texture used to store video output.

	// Autonomous player stuff.
	Mesh m_mesh;    ///< Used to render the video frame as a textured quad.
	bool m_stop;    ///< Flag that indicates the media player should stop playing when playing autonomously.
	bool m_restart; ///< Restart flag. Not yet implemented.
};

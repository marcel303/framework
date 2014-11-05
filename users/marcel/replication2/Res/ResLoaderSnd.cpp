#include "ResLoaderSnd.h"
#include "ResSnd.h"

#if WITH_SDL_MIXER

#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

#if 0

static int file_read(SDL_RWops* ops, void* dst, int size, int num)
{
	int result = 0;

	File* file = (File*)ops->hidden.unknown.data1;

	for (int i = 0; i < num; ++i)
		if (file->Read((char*)dst + result * size, size))
			result++;

	return result;
}

static int file_write(SDL_RWops* ops, const void* src, int size, int num)
{
	int result = num;

	File* file = (File*)ops->hidden.unknown.data1;

	if (!file->Write(src, size * num))
		result = -1;

	return result;
}

static int file_seek(SDL_RWops* ops, int position, int in_mode)
{
	File* file = (File*)ops->hidden.unknown.data1;

	FILE_SEEKMODE mode;

	if (in_mode == SEEK_SET)
		mode = SEEKMODE_ABS;
	else if (in_mode == SEEK_CUR)
		mode = SEEKMODE_REL;
	else if (in_mode == SEEK_END)
	{
		position += file->GetLength();
		mode = SEEKMODE_ABS;
	}
	else
		return file->GetPosition();

	file->Seek(position, mode);

	return file->GetPosition();
}

static int file_close(SDL_RWops* ops)
{
	File* file = (File*)ops->hidden.unknown.data1;

	return file->Close() ? 1 : 0;
}

static SDL_RWops file_ops(File& file)
{
	SDL_RWops ops;

	memset(&ops, 0, sizeof(ops));
	ops.read = file_read;
	ops.write = file_write;
	ops.seek = file_seek;
	ops.close = file_close;
	ops.hidden.unknown.data1 = &file;

	return ops;
}

#endif

Res* ResLoaderSnd::Load(const std::string& name)
{
	ResSnd* snd = new ResSnd();

	snd->m_data = Mix_LoadWAV(name.c_str());

	if (!snd->m_data)
	{
		const char * error = Mix_GetError();

		if (error)
			LOG_ERR(error);
	}

	return snd;
}

#else

Res* ResLoaderSnd::Load(const std::string& name)
{
	ResSnd* snd = new ResSnd();

	return snd;
}

#endif

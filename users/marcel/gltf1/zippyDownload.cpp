#include "zippyDownload.h"

#include "Path.h"

static void zippyDownload(const std::vector<std::string> & urls, const char * targetPath);
static void decompressZipFile(const char * filename, const char * targetPath);

void zippyDownload(const char * url, const char * targetPath)
{
	zippyDownload(std::vector<std::string>({ url }), targetPath);
}

void zippyDownload(const char * baseUrl, const std::vector<std::string> & filenames, const char * targetPath)
{
	std::vector<std::string> urls;
	
	for (auto & filename : filenames)
	{
		auto url = std::string(baseUrl) + "/" + filename;
		
		urls.emplace_back(url);
	}
	
	zippyDownload(urls, targetPath);
}

//

#include "framework.h"

#include "DownloadCache.h"

// todo : font calibri.ttf

static void doProgressBar(const int x, const int y, const int sx, const int sy, const double time, const double duration, const float opacity)
{
	if (duration <= 0.0)
		return;
		
	const double t = time / duration;
	
	setColor(63, 127, 255, 127 * opacity);
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	hqFillRoundedRect(x, y, x + sx * t, y + sy, 6);
	hqEnd();
	
	setColor(63, 127, 255, 255 * opacity);
	hqBegin(HQ_STROKED_ROUNDED_RECTS);
	hqStrokeRoundedRect(x, y, x + sx, y + sy, 6, 2);
	hqEnd();
}

static void zippyDownload(const std::vector<std::string> & urls, const char * targetPath)
{
	DownloadCache downloadCache;

	for (auto & url : urls)
	{
		auto targetFilename = std::string(targetPath) + "/" + Path::GetFileName(url);
		
		downloadCache.add(url.c_str(), targetFilename.c_str());
	}

	while (downloadCache.downloadQueue.isEmpty() == false)
	{
		framework.process();

		downloadCache.tick(4);

		framework.beginDraw(0, 0, 0, 0);
		{
			int viewSx;
			int viewSy;
			framework.getCurrentViewportSize(viewSx, viewSy);
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(viewSx/2, viewSy*1/3, 16, 0, 0, "Downloading media files..");

			int y = 200 + 20;
			for (auto & e : downloadCache.downloadQueue.activeElems)
			{
				drawText(viewSx/2, viewSy*1/3 + 20, 14, 0, 0, "Downloading %s/%dkb..", e.first.c_str(), e.second.getProgress() / 1024);
				y += 18;
				doProgressBar(viewSx/2, viewSy*1/3 + 20 + 18, 100, 10, e.second.getProgress(), e.second.getExpectedSize(), 1.f);
				y += 14;
			}
		}
		framework.endDraw();
	}
	
	for (int i = 0; i < 3; ++i)
	{
		framework.beginDraw(0, 0, 0, 0);
		{
			int viewSx;
			int viewSy;
			framework.getCurrentViewportSize(viewSx, viewSy);
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(viewSx/2, viewSy*1/3, 16, 0, 0, "Downloading media files.. [DONE]");
		}
		framework.endDraw();
	}
	
	for (auto & url : urls)
	{
		auto targetFilename = std::string(targetPath) + "/" + Path::GetFileName(url);
		
		decompressZipFile(targetFilename.c_str(), targetPath);
	}
}

#include "miniz.h"
#include "StringEx.h" // sprintf_s
#include <sys/stat.h> // mkdir
#include <sys/errno.h>

static void decompressZipFile(const char * filename, const char * targetPathBase)
{
	if (filename[0] == 0)
		filename = "."; // ensure the concatenated paths becomes at least ./<filename>
		
	mz_zip_archive zip;
	memset(&zip, 0, sizeof(zip));
	
	if (mz_zip_reader_init_file(&zip, filename, 0) == false)
	{
		logError("failed to open zip file for reading: %s",
			mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
	}
	else
	{
		const int numFiles = mz_zip_reader_get_num_files(&zip);
		
		for (int i = 0; i < numFiles; ++i)
		{
			if (mz_zip_reader_is_file_a_directory(&zip, i))
				continue;
			
			if (mz_zip_reader_is_file_supported(&zip, i) == false)
			{
				logError("found unsupported file within zip file");
				continue;
			}
			
			char filename[1024];
			mz_zip_reader_get_filename(&zip, i, filename, sizeof(filename));
			
			if (strstr(filename, "__MACOSX") != nullptr)
				continue;
			
			logDebug("filename: %s", filename);
			
			char targetFilename[1024];
			
			sprintf_s(targetFilename, sizeof(targetFilename), "%s/%s", targetPathBase, filename);
			
			{
				bool success = true;
				
				char * begin = targetFilename;
				
				for (;;)
				{
					char * end = strchr(begin, '/');
					
					if (end == nullptr)
						break;
					
					*end = 0;
					
					const int err = mkdir(targetFilename, S_IRWXU | S_IRWXG | S_IRWXO);
					
					if (err != 0 && errno != EEXIST)
					{
						success = false;
						break;
					}
					
					*end = '/';
					
					begin = end + 1;
				}
				
				if (success == false)
				{
					logError("failed to create path: %s", targetFilename);
					continue;
				}
			}
			
			if (mz_zip_reader_extract_to_file(&zip, i, targetFilename, 0) == false)
			{
				logError("failed to extract file from zip file: %s",
					mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
				continue;
			}
		}
		
		if (mz_zip_reader_end(&zip) == false)
		{
			logError("failed to end zip reader: %s",
				mz_zip_get_error_string(mz_zip_get_last_error(&zip)));
		}
	}
}

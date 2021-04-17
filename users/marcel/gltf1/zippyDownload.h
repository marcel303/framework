#pragma once

#include <string>
#include <vector>

void zippyDownload(const char * url, const char * targetPath);
void zippyDownload(const char * baseUrl, const std::vector<std::string> & filenames, const char * targetPath);

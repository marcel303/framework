#include "Debugging.h"
#include "fileBrowser.h"
#include "imgui.h"
#include "StringEx.h"
#include <algorithm>

#ifndef _MSC_VER
#include <dirent.h>
#endif

#ifdef _MSC_VER
#include <direct.h>
#include <Windows.h>
#endif

FileBrowser::~FileBrowser()
{
	Assert(elemsByPath.empty());
	
	shut();
}

void FileBrowser::init(const char * in_rootPath)
{
	shut();
	
	//
	
	rootPath = in_rootPath;
	
	//
	
	scanFiles(rootPath.c_str());
}

void FileBrowser::shut()
{
	clearFiles();
}

void FileBrowser::clearFiles()
{
	for (auto & elemByPath : elemsByPath)
	{
		auto *& elem = elemByPath.second;
		
		delete elem;
		elem = nullptr;
	}

	elemsByPath.clear();
}

void FileBrowser::scanFiles(const char * path)
{
	auto *& elems = elemsByPath[path];
	
	Assert(elems == nullptr);
	elems = new std::vector<FileElem>();
	
#ifdef WIN32
	WIN32_FIND_DATAA ffd;
	char wildcard[MAX_PATH];
	sprintf_s(wildcard, sizeof(wildcard), "%s\\*", path);
	HANDLE find = FindFirstFileA(wildcard, &ffd);
	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (strcmp(ffd.cFileName, ".") == 0 || strcmp(ffd.cFileName, "..") == 0)
				continue;

			char fullPath[MAX_PATH];
			sprintf_s(fullPath, sizeof(fullPath), "%s/%s", path, ffd.cFileName);

			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				FileElem elem;
				elem.name = ffd.cFileName;
				elem.path = fullPath;
				elem.isFile = false;

				elems->push_back(elem);
			}
			else
			{
				FileElem elem;
				elem.name = ffd.cFileName;
				elem.path = fullPath;
				elem.isFile = true;

				elems->push_back(elem);
			}
		} while (FindNextFileA(find, &ffd) != 0);

		FindClose(find);
	}
#else
	DIR * dir = opendir(path);
	
	if (dir != nullptr)
	{
		dirent * ent;
		
		while ((ent = readdir(dir)) != 0)
		{
			if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
				continue;
			
			char fullPath[PATH_MAX];
			sprintf_s(fullPath, sizeof(fullPath), "%s/%s", path, ent->d_name);
			
			if (ent->d_type == DT_DIR)
			{
				FileElem elem;
				elem.name = ent->d_name;
				elem.path = fullPath;
				elem.isFile = false;
				
				elems->push_back(elem);
			}
			else
			{
				FileElem elem;
				elem.name = ent->d_name;
				elem.path = fullPath;
				elem.isFile = true;
				
				elems->push_back(elem);
			}
		}
		
		closedir(dir);
	}
#endif

	// sort the elements by name
	
	std::sort(elems->begin(), elems->end(), [](const FileElem & e1, const FileElem & e2)
		{
			return strcasecmp(e1.name.c_str(), e2.name.c_str()) < 0;
		});
}

void FileBrowser::tickRecurse(const char * path)
{
	ImGui::PushID(path);
	{
		// draw foldable menu items for each file
		
		auto elems_itr = elemsByPath.find(path);
		
		if (elems_itr == elemsByPath.end())
		{
			scanFiles(path);
			
			elems_itr = elemsByPath.find(path);
		}
		
		auto *& elems = elems_itr->second;
		
		for (auto & elem : *elems)
		{
			if (elem.isFile == false)
			{
				if (ImGui::CollapsingHeader(elem.name.c_str()))
				{
					ImGui::Indent();
					tickRecurse(elem.path.c_str());
					ImGui::Unindent();
				}
			}
		}
		
		for (auto & elem : *elems)
			if (elem.isFile == true)
				if (ImGui::Button(elem.name.c_str()))
					onFileSelected(elem.path);
	}
	ImGui::PopID();
}

void FileBrowser::tick()
{
	ImGui::PushID("root");
	{
		tickRecurse(rootPath.c_str());
	}
	ImGui::PopID();
}

#pragma once

#include <assert.h>
#include <string>
#include "generator.h"

class Vc10Project
{
public:
	std::string guid;
	std::string file;
};

class Vc10ProjectReference
{
public:
	std::string guid;
	std::string file;
};

class Vc10Generator : public GeneratorBase<Vc10Project, Vc10ProjectReference>, public Generator
{
	std::string MakeGuid() const
	{
		// format : 8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942 (16 bytes long, 128 bits)

		char temp[64];
		char * tempptr = temp;
		const int lens[] = { 8, 4, 4, 4, 12 };
		for (int i = 0; i < sizeof(lens) / sizeof(lens[0]); ++i)
		{
			if (i != 0)
				*tempptr++ = '-';
			const int len = lens[i];
			for (int j = 0; j < len; ++j)
			{
				const int r = rand() & 0xf; // todo : include time
				*tempptr++ = r < 10 ? ('0' + r) : ('A' + r - 10);
			}
		}
		*tempptr = 0;

		return temp;
	}

	bool IsFileType(const std::string & path, const char * exts[], int numExts) const
	{
		size_t dot = path.find_last_of('.');

		if (dot == std::string::npos)
			return false;

		const char * ext = &path[0] + dot + 1;

		for (int i = 0; i < numExts; ++i)
			if (!strcmp(ext, exts[i]))
				return true;

		return false;
	}

	bool ShouldCompile(const Project::Source & source) const
	{
		const char * exts[] = { "c", "cpp", "c++" };
		return IsFileType(source.path, exts, sizeof(exts) / sizeof(exts[0]));
	}

	bool IsHeader(const Project::Source & source) const
	{
		const char * exts[] = { "h", "hpp", "c++" };
		return IsFileType(source.path, exts, sizeof(exts) / sizeof(exts[0]));
	}

public:
	virtual void Parse(XMLDocument * document, const char * basePath)
	{
		DoParse(document, basePath);

		// assign guids and filenames

		for (std::vector<Project>::iterator i = projects.begin(); i != projects.end(); ++i)
		{
			Project & project = *i;

			project.guid = MakeGuid();
			project.file = project.name + ".vcxproj";
		}

		// resolve project references

		for (std::vector<Project>::iterator i = projects.begin(); i != projects.end(); ++i)
		{
			Project & project = *i;

			for (std::vector<Project::Reference>::iterator j = project.references.begin(); j != project.references.end(); ++j)
			{
				Project::Reference & reference = *j;

				for (std::vector<Project>::iterator k = projects.begin(); k != projects.end(); ++k)
				{
					Project & otherProject = *k;

					if (otherProject.name == reference.name)
					{
						reference.guid = otherProject.guid;
						reference.file = otherProject.file;
					}
				}
			}
		}
	}

	virtual void WriteProjectFile(NodeWrapper & root)
	{
		// generate solution file

		std::string solutionFile = outputPath + solution.name + ".sln";

		FILE * file = fopen(solutionFile.c_str(), "wt");
		
		std::string solutionGuid = MakeGuid();

		fprintf(file, "Microsoft Visual Studio Solution File, Format Version 11.00\n");
		fprintf(file, "# Visual C++ Express 2010\n");

		for (std::vector<Project>::iterator i = projects.begin(); i != projects.end(); ++i)
		{
			Project & project = *i;

			fprintf(file, "Project(\"{%s}\") = \"%s\", \"%s\", \"{%s}\"\n",
				solutionGuid.c_str(),
				project.name.c_str(),
				project.file.c_str(),
				project.guid.c_str());
			for (std::vector<Project::Reference>::iterator j = project.references.begin(); j != project.references.end(); ++j)
			{
				Project::Reference & reference = *j;

				fprintf(file, "\tProjectSection(ProjectDependencies) = postProject\n");
				fprintf(file, "\t\t{%s} = {%s}\n", reference.guid.c_str(), reference.guid.c_str());
				fprintf(file, "\tEndProjectSection\n");
			}
			fprintf(file, "EndProject\n");
		}

		fprintf(file, "Global\n");

		fprintf(file, "\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n");
		fprintf(file, "\t\tDebug|Win32 = Debug|Win32\n");
		fprintf(file, "\t\tRelease|Win32 = Release|Win32\n");
		fprintf(file, "\tEndGlobalSection\n");

		fprintf(file, "\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n");
		for (std::vector<Project>::iterator i = projects.begin(); i != projects.end(); ++i)
		{
			Project & project = *i;

			fprintf(file, "\t\t{%s}.Debug|Win32.ActiveCfg = Debug|Win32\n", project.guid.c_str());
			fprintf(file, "\t\t{%s}.Debug|Win32.Build.0 = Debug|Win32\n", project.guid.c_str());
			fprintf(file, "\t\t{%s}.Release|Win32.ActiveCfg = Release|Win32\n", project.guid.c_str());
			fprintf(file, "\t\t{%s}.Release|Win32.Build.0 = Release|Win32\n", project.guid.c_str());
		}
		fprintf(file, "\tEndGlobalSection\n");

		fprintf(file, "\tGlobalSection(SolutionProperties) = preSolution\n");
		fprintf(file, "\t\tHideSolutionNode = FALSE\n");
		fprintf(file, "\tEndGlobalSection\n");

		fprintf(file, "EndGlobal\n");

		fclose(file);

		// generate project files

		#define PushAttributeFmt(name, fmt, ...) \
			do { \
				char temp[256]; \
				sprintf(temp, fmt, __VA_ARGS__); \
				printer.PushAttribute(name, temp); \
			} while (false)

		#define PushTextFmt(fmt, ...) \
			do { \
				char temp[256]; \
				sprintf(temp, fmt, __VA_ARGS__); \
				printer.PushText(temp); \
			} while (false)

		#define PushTextAttributeFmt(name, fmt, ...) \
			do { \
				printer.OpenElement(name); \
				char temp[256]; \
				sprintf(temp, fmt, __VA_ARGS__); \
				printer.PushText(temp); \
				printer.CloseElement(); \
			} while (false)

		for (std::vector<Project>::iterator i = projects.begin(); i != projects.end(); ++i)
		{
			Project & project = *i;

			std::string projectFile = outputPath + project.file;

			FILE * file = fopen(projectFile.c_str(), "wt");

			fprintf(file, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");

			XMLPrinter printer(file, false);

			printer.OpenElement("Project");
			{
				printer.PushAttribute("DefaultTargets", "Build");
				printer.PushAttribute("ToolsVersion", "4.0");
				printer.PushAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");

				printer.OpenElement("ItemGroup");
				{
					printer.PushAttribute("Label", "ProjectConfigurations");
					printer.OpenElement("ProjectConfiguration");
					{
						printer.PushAttribute("Include", "Debug|Win32");
						PushTextAttributeFmt("Configuration", "Debug");
						PushTextAttributeFmt("Platform", "Win32");
					}
					printer.CloseElement(); // ProjectConfiguration
				}
				printer.CloseElement(); // ItemGroup
			
				printer.OpenElement("PropertyGroup");
				{
					printer.PushAttribute("Label", "Globals");
					PushTextAttributeFmt("ProjectGuid", "{%s}", project.guid.c_str());
					PushTextAttributeFmt("Keyword", "Win32Proj");
					PushTextAttributeFmt("RootNamespace", project.name.c_str());
				}
				printer.CloseElement(); // PropertyGroup

				printer.OpenElement("Import");
				{
					printer.PushAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.Default.props");
				}
				printer.CloseElement();

				printer.OpenElement("PropertyGroup");
				{
					printer.PushAttribute("Label", "Configuration");

					if (project.type == "library")
						PushTextAttributeFmt("ConfigurationType", "StaticLibrary");
					else
						PushTextAttributeFmt("ConfigurationType", "Application");

					PushTextAttributeFmt("UseDebugLibraries", "true");
					PushTextAttributeFmt("CharacterSet", "Unicode");
					PushTextAttributeFmt("LinkIncremental", "false");
				}
				printer.CloseElement(); // PropertyGroup

				printer.OpenElement("Import");
				{
					printer.PushAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.props");
				}
				printer.CloseElement();

				printer.OpenElement("ImportGroup");
				{
					printer.PushAttribute("Label", "PropertySheets");
					printer.OpenElement("Import");
					{
						printer.PushAttribute("Project", "$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props");
						printer.PushAttribute("Condition", "exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')");
						printer.PushAttribute("Label", "LocalAppDataPlatform");
					}
					printer.CloseElement();
				}
				printer.CloseElement();

				printer.OpenElement("ItemDefinitionGroup");
				{
					printer.OpenElement("ClCompile");
					{
						PushTextAttributeFmt("WarningLevel", "Level3");
						PushTextAttributeFmt("Optimization", "Disabled");

						// preprocessor defines

						std::string definesString;
						for (size_t i = 0; i < project.preprocessor.defines.size(); ++i)
						{
							if (i != 0)
								definesString += ";";
							definesString += project.preprocessor.defines[i].value; // todo : platform / config filter
						}
						PushTextAttributeFmt("PreprocessorDefinitions", definesString.c_str());
					}
					printer.CloseElement();

					printer.OpenElement("Link");
					{
						PushTextAttributeFmt("SubSystem", "Console");
						PushTextAttributeFmt("GenerateDebugInformation", "true");

						// todo : additional link libraries
					}
					printer.CloseElement();
				}
				printer.CloseElement(); // ItemDefinitionGroup

				printer.OpenElement("ItemGroup");
				{
					for (std::vector<Project::Source>::iterator i = project.sources.begin(); i != project.sources.end(); ++i)
					{
						Project::Source & source = *i;

						// todo : ClInclude if header

						if (ShouldCompile(source))
						{
							printer.OpenElement("ClCompile");
							{
								printer.PushAttribute("Include", source.path.c_str());

								if (source.build == false)
									PushTextAttributeFmt("ExcludedFromBuild", "true");
							}
							printer.CloseElement();
						}
						else if (IsHeader(source))
						{
							printer.OpenElement("ClInclude");
							{
								printer.PushAttribute("Include", source.path.c_str());
							}
							printer.CloseElement();
						}
						else
						{
							printer.OpenElement("None");
							{
								printer.PushAttribute("Include", source.path.c_str());
							}
							printer.CloseElement();
						}
					}
				}
				printer.CloseElement();

				printer.OpenElement("ItemGroup");
				{
					for (std::vector<Project::Reference>::iterator i = project.references.begin(); i != project.references.end(); ++i)
					{
						printer.OpenElement("ProjectReference");
						{
							Project::Reference & reference = *i;

							printer.PushAttribute("Include", reference.file.c_str());
							PushTextAttributeFmt("Project", "{%s}", reference.guid.c_str());
						}
						printer.CloseElement();
					}
				}
				printer.CloseElement();

				printer.OpenElement("Import");
				{
					printer.PushAttribute("Project", "$(VCTargetsPath)\\Microsoft.Cpp.targets");
				}
				printer.CloseElement();
			}
			printer.CloseElement(); // Project

			fclose(file);
		}
	}
};

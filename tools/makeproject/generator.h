#pragma once

#include <string>
#include <vector>
#include "tinyxml2.h"

using namespace tinyxml2;

class NodeWrapper
{
public:
	XMLNode * m_node;

	XMLElement * elem()
	{
		return m_node ? m_node->ToElement() : nullptr;
	}

	NodeWrapper()
		: m_node(nullptr)
	{
	}

	NodeWrapper(XMLNode * node)
		: m_node(node)
	{
	}

	const char * GetString(const char * name, const char * _default)
	{
		const char * result = elem() ? elem()->Attribute(name) : nullptr;

		return result ? result : _default;
	}

	bool GetBool(const char * name, bool _default)
	{
		bool result;

		if (elem() && elem()->QueryBoolAttribute(name, &result) == XML_NO_ERROR)
			return result;
		else
			return _default;
	}

	NodeWrapper operator[](const char * name)
	{
		XMLNode * node = m_node ? m_node->FirstChildElement(name) : nullptr;

		return NodeWrapper(node);
	}
};

template <typename ProjectBase, typename ProjectReferenceBase>
class GeneratorBase
{
protected:
	class Solution
	{
	public:
		std::string name;
	};

	class Project : public ProjectBase
	{
	public:
		class Reference : public ProjectReferenceBase
		{
		public:
			std::string name;
		};

		class Preprocessor
		{
		public:
			struct Define
			{
				NodeWrapper node;
				std::string value;
				std::string platform;
				std::string config;
			};

			NodeWrapper node;
			std::vector<Define> defines;
		};

		class Source
		{
		public:
			Source()
				: build(true)
			{
			}

			NodeWrapper node;
			std::string path;
			bool build;
		};

		NodeWrapper node;
		std::string type;
		std::string name;
		std::vector<Reference> references;
		Preprocessor preprocessor;
		std::vector<Source> sources;
	};

	Solution solution;
	std::vector<Project> projects;
	std::string outputPath;

public:
	void DoParse(XMLDocument * document, const char * basePath)
	{
		outputPath = basePath;

		for (XMLNode * elem = document->FirstChildElement(); elem != nullptr; elem = elem->NextSiblingElement())
		{
			if (!strcmp(elem->Value(), "solution"))
			{
				NodeWrapper node(elem);

				solution.name = node.GetString("name", "");
			}

			if (!strcmp(elem->Value(), "project"))
			{
				NodeWrapper node(elem);

				projects.resize(projects.size() + 1);

				Project & project = projects.back();

				project.node = node;
				project.name = node.GetString("name", "");
				project.type = node.GetString("type", "");

				for (XMLNode * elem2 = elem->FirstChildElement(); elem2 != nullptr; elem2 = elem2->NextSiblingElement())
				{
					if (!strcmp(elem2->Value(), "reference"))
					{
						NodeWrapper node(elem2);

						project.references.resize(project.references.size() + 1);

						Project::Reference & reference = project.references.back();

						reference.name = node.GetString("name", "");
					}

					if (!strcmp(elem2->Value(), "preprocessor"))
					{
						Project::Preprocessor & preprocessor = project.preprocessor;

						preprocessor.node = NodeWrapper(elem2);

						for (XMLNode * elem3 = elem2->FirstChildElement(); elem3 != nullptr; elem3 = elem3->NextSiblingElement())
						{
							if (!strcmp(elem3->Value(), "define"))
							{
								NodeWrapper node(elem3);

								preprocessor.defines.resize(preprocessor.defines.size() + 1);

								Project::Preprocessor::Define & define = preprocessor.defines.back();

								define.node = node;
								define.value = node.GetString("value", "");
								define.platform = node.GetString("platform", "");
								define.config = node.GetString("config", "");
							}
						}
					}

					if (!strcmp(elem2->Value(), "files"))
					{
						for (XMLNode * elem3 = elem2->FirstChildElement(); elem3 != nullptr; elem3 = elem3->NextSiblingElement())
						{
							if (!strcmp(elem3->Value(), "file"))
							{
								NodeWrapper node(elem3);

								project.sources.resize(project.sources.size() + 1);

								Project::Source & source = project.sources.back();

								source.node = node;
								source.path = node.GetString("path", "");
								source.build = node.GetBool("build", true);
							}
						}
					}
				}
			}
		}
	}
};

class Generator
{
public:
	virtual void Parse(XMLDocument * document, const char * basePath) = 0;

	virtual void WriteProjectFile(NodeWrapper & root) = 0;
};

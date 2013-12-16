#include <string>
#include <vector>

class DirectoryScanner
{
public:
	static std::string GetDocumentPath();
	static std::string GetResourcePath();
	
	static std::vector<std::string> ListDirectories(std::string path);
	static std::vector<std::string> ListFiles(std::string path);
};

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using BuildTool.Cpp;

namespace BuildTool
{
    public class BuildSourcePath
    {
        public BuildSourcePath(string path, string extension, bool recursive)
        {
            Path = path;
            Extension = extension;
            Recursive = recursive;
        }

        public string Path;
        public string Extension;
        public bool Recursive;
    }

    public class BuildSettings
    {
        public string BuildDir = @"c:\gg-code\";
        public List<IBuildRule> BuildRuleList = new List<IBuildRule>();
        public Dictionary<string, IDepScanner> DepScannerList = new Dictionary<string, IDepScanner>();
        public List<string> IncludePathList = new List<string>();
        public List<BuildSourcePath> SourcePathList = new List<BuildSourcePath>();
        public List<string> ExclusionList = new List<string>();

        public void Load(XmlDocument document, string buildDir)
        {
            BuildDir = buildDir;

            foreach (XmlElement e in document["settings"])
            {
                switch (e.Name)
                {
                    case "rule_list":
                        {
                            foreach (XmlElement ruleElement in e.GetElementsByTagName("rule"))
                            {
                                string type = ruleElement.GetAttribute("type");
                                IBuildRule rule = null;
                                switch (type)
                                {
                                    case "gcc_cpp":
                                        rule = new BuildRule_Gcc_Cpp();
                                        break;
                                    case "msvc_cpp":
                                        rule = new BuildRule_Msvc_Cpp();
                                        break;
                                }
                                if (rule == null)
                                    throw new Exception("unknown rule type");
                                BuildRuleList.Add(rule);
                            }
                            break;
                        }

                    case "dep_scanner_list":
                        {
                            foreach (XmlElement depScannerElement in e.GetElementsByTagName("dep_scanner"))
                            {
                                string extension = depScannerElement.GetAttribute("extension");
                                string scannerName = depScannerElement.GetAttribute("scanner");
                                IDepScanner scanner = null;
                                switch (scannerName)
                                {
                                    case "cpp":
                                        scanner = new CppScanner();
                                        break;
                                    case "cpp_cached":
                                        scanner = new CppScanner_Cached();
                                        break;
                                }
                                if (scanner == null)
                                    throw new Exception("unknown scanner");
                                DepScannerList.Add(extension, scanner);
                            }
                            break;
                        }
                    case "include_list":
                        {
                            foreach (XmlElement includeDirElement in e.GetElementsByTagName("dir"))
                            {
                                string path = ApplyVars(includeDirElement.GetAttribute("path"));
                                IncludePathList.Add(path);
                            }
                            break;
                        }
                    case "source_list":
                        {
                            foreach (XmlElement sourceDirElement in e.GetElementsByTagName("dir"))
                            {
                                string path = ApplyVars(sourceDirElement.GetAttribute("path"));
                                string extension = sourceDirElement.GetAttribute("filter_extension");
                                bool recursive = sourceDirElement.GetAttribute("recursive") != "0";
                                SourcePathList.Add(new BuildSourcePath(path, extension, recursive));
                            }
                            break;
                        }
                    case "exclusion_list":
                        {
                            foreach (XmlElement exclusionFileNameAttribute in e.GetElementsByTagName("file_name"))
                            {
                                string name = ApplyVars(exclusionFileNameAttribute.GetAttribute("name"));
                                ExclusionList.Add(name);
                            }
                            break;
                        }
                }
            }
        }

        public string ApplyVars(string text)
        {
            return text.Replace("[build_dir]", BuildDir);
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml;
using System.IO;
using BuildTool;

namespace BuildCmd
{
	enum RequestType
	{
		Unknown,
		Build,
		DepList
	}

	class Settings
	{
		public void Parse(string[] args)
		{
			for (int i = 0; i < args.Length; )
			{
				switch (args[i])
				{
					case "-c":
						if (i + 1 >= args.Length)
							throw new Exception("expected argument");
						SourceList.Add(args[i + 1]);
						i += 2;
						break;
					case "-m":
						if (i + 1 >= args.Length)
							throw new Exception("expected argument");
						switch (args[i + 1])
						{
							case "build":
								RequestType = RequestType.Build;
								break;
							case "deps":
								RequestType = RequestType.DepList;
								break;
						}
						i += 2;
						break;
					default:
						throw new Exception("unknown option");
				}
			}
		}

		public void Validate()
		{
			switch (RequestType)
			{
				case RequestType.Build:
					if (SourceList.Count != 1)
						throw new Exception("expected one source file");
					break;
				case RequestType.DepList:
					if (SourceList.Count != 1)
						throw new Exception("expected one source file");
					break;
				case RequestType.Unknown:
					throw new Exception("unknown request type");
			}
		}

		public RequestType RequestType = RequestType.Build;
		public List<string> SourceList = new List<string>();
	}

	class Program
	{
		static int Main(string[] args)
		{
			try
			{
				Console.WriteLine("parsing command line");

				Settings appSettings = new Settings();

				appSettings.Parse(args);

				appSettings.Validate();

				Console.WriteLine("parsing command line [done]");

				Console.WriteLine("loading settings");

				//mTimeLoadSettings -= DateTime.Now.Ticks;

				//string fileName = Path.GetFullPath("settings.xml");
				//string buildDir = Path.GetDirectoryName(fileName) + "/";
				string buildDir = "C:/gg-code/";

				BuildSettings settings = new BuildSettings();

				XmlDocument document = new XmlDocument();

				document.Load(Path.GetFullPath(appSettings.SourceList[0]));

				settings.Load(document, buildDir);

				//mTimeLoadSettings += DateTime.Now.Ticks;

				Console.WriteLine("loading settings [done]");

				//

				BuildProcessor processor = new BuildProcessor();

				List<BuildObject> objList = new List<BuildObject>();

				//

				Console.WriteLine("scanning for source files");

				//mTimeSearch -= DateTime.Now.Ticks;

				FileFinder finder = new FileFinder();

				foreach (BuildSourcePath sourcePath in settings.SourcePathList)
					finder.Add(sourcePath.Path, sourcePath.Extension, sourcePath.Recursive);
				foreach (string exclusion in settings.ExclusionList)
					finder.Exclude(exclusion);

				//mTimeSearch += DateTime.Now.Ticks;

				Console.WriteLine("scanning for source files [done]");

				//

				Console.WriteLine("determining output files");

				//mTimeTranslate -= DateTime.Now.Ticks;

				IList<FileName> fileList = finder.Files;

				foreach (FileName fileName in fileList)
				{
					foreach (IBuildRule buildRule in settings.BuildRuleList)
					{
						FileName destination = null;

						if (!buildRule.GetOutputName(fileName, out destination))
							continue;

						FileName directDependency = buildRule.GetDirectDependency(fileName);

						BuildObject obj = new BuildObject(fileName, destination, directDependency, buildRule);

						objList.Add(obj);
					}
				}

				//mTimeTranslate += DateTime.Now.Ticks;

				Console.WriteLine("determining output files [done]");

				//Console.WriteLine("warming up build cache");

				//mTimeWarmup -= DateTime.Now.Ticks;

				//processor.Warmup(includePathList, true);

				//mTimeWarmup += DateTime.Now.Ticks;

				//Console.WriteLine("warming up build cache [done]");

				switch (appSettings.RequestType)
				{
					case RequestType.Build:
						{
							Console.WriteLine("building");

							//DateTime t1 = DateTime.Now;

							processor.Build(settings.DepScannerList, settings.IncludePathList, objList);

							/*DateTime t2 = DateTime.Now;

							decimal time = new decimal((t2 - t1).TotalSeconds);

							Console.WriteLine("done: {0} sec", time.ToString("N2"));*/

							Console.WriteLine("building [done]");
							break;
						}
					case RequestType.DepList:
						{
							foreach (BuildObject obj in objList)
							{
								Dictionary<FileName, bool> depList = new Dictionary<FileName, bool>();

								processor.DepList_GetFull(null, 0, obj.Source, settings.DepScannerList, settings.IncludePathList, depList);

								Console.WriteLine(obj.Source.FileNameString);

								List<FileName> fileList2 = depList.Keys.ToList();

								fileList2.Sort((v1, v2) => v1.DepLevel == v2.DepLevel ? v1.FileNameString.CompareTo(v2.FileNameString) : v1.DepLevel.CompareTo(v2.DepLevel));

								foreach (FileName file in fileList2)
								{
									Console.WriteLine("\t[{0:00}] {1}", file.DepLevel, file.FileNameString);
								}
							}

							break;
						}
				}

				return 0;
			}
			catch (Exception e)
			{
				Console.WriteLine(e.Message);
				return -1;
			}
		}
	}
}

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Xml;

namespace BuildTool
{
	public partial class Form1 : Form
	{
		private static long mTimeLoadSettings = 0;
		private static long mTimeSearch = 0;
		private static long mTimeTranslate = 0;
		private static long mTimeWarmup = 0;

		public Form1()
		{
			InitializeComponent();

			BuildAgent agent = new BuildAgent();
			agent.Show();

			WatchBegin("C:/gg-code/");
		}

		public static void DEBUG_ShowStats()
		{
			Console.WriteLine("TimeLoadSettings: {0} sec", mTimeLoadSettings / 10000000.0);
			Console.WriteLine("TimeSearch: {0} sec", mTimeSearch / 10000000.0);
			Console.WriteLine("TimeTranslate: {0} sec", mTimeTranslate / 10000000.0);
			Console.WriteLine("TimeWarmup: {0} sec", mTimeWarmup / 10000000.0);
		}

		private BuildSettings LoadBuildSettings()
		{
			mTimeLoadSettings -= DateTime.Now.Ticks;

			string fileName = Path.GetFullPath("settings.xml");
			//string buildDir = Path.GetDirectoryName(fileName) + "/";
			string buildDir = "C:/gg-code/";

			BuildSettings result = new BuildSettings();

			XmlDocument document = new XmlDocument();

			document.Load(fileName);

			result.Load(document, buildDir);

			mTimeLoadSettings += DateTime.Now.Ticks;

			return result;
		}

		private FileSystemWatcher mFileWatcher;
		private static Dictionary<string, DateTime> mFileDates = new Dictionary<string, DateTime>();

		private void WatchBegin(string path)
		{
			if (!string.IsNullOrEmpty(path))
			{
				mFileWatcher = new FileSystemWatcher(
					Path.GetDirectoryName(path),
					"*.*");
				mFileWatcher.NotifyFilter = NotifyFilters.LastWrite;
				mFileWatcher.IncludeSubdirectories = true;
				mFileWatcher.Changed += HandleFileChanged_DLG;
				mFileWatcher.EnableRaisingEvents = true;
			}
		}

		private void WatchEnd()
		{
			if (mFileWatcher != null)
			{
				mFileWatcher.EnableRaisingEvents = false;
				mFileWatcher = null;
			}
		}

		private delegate void FileChangedHandler();

		private void HandleFileChanged_DLG(object sender, FileSystemEventArgs e)
		{
			if (e.ChangeType != WatcherChangeTypes.Changed)
				return;

			string extension = Path.GetExtension(e.FullPath);

			if (extension != ".cpp" && extension != ".h")
				return;

			DateTime newDate = File.GetLastWriteTime(e.FullPath);
			DateTime oldDate;

			if (mFileDates.TryGetValue(e.FullPath, out oldDate))
			{
				if (newDate == oldDate)
					return;
			}

			mFileDates[e.FullPath] = newDate;

			Invoke(new FileChangedHandler(HandleFileChanged));
		}

		private void HandleFileChanged()
		{
			button1_Click(this, null);
		}

		private BuildSettings MakeBuildSettings()
		{
			BuildSettings result = new BuildSettings();

			if (radioButton1.Checked)
				result.BuildRuleList.Add(new BuildRule_Gcc_Cpp());
			if (radioButton2.Checked)
				result.BuildRuleList.Add(new BuildRule_Msvc_Cpp());

			//

			//IDepScanner scannerCpp = new CppScanner();
			IDepScanner scannerCpp = new Cpp.CppScanner_Cached();

			result.DepScannerList.Add("cpp", scannerCpp);
			result.DepScannerList.Add("h", scannerCpp);

			//

			result.IncludePathList.Add(@"C:\gg-code\libgrs\libgrs_cpp");
			result.IncludePathList.Add(@"C:\gg-code\libgg");
			result.IncludePathList.Add(@"C:\gg-code\libiphone");
			result.IncludePathList.Add(@"C:\gg-code\prototypes\vecrend");
			result.IncludePathList.Add(@"C:\gg-code\tools\atlcompiler");
			result.IncludePathList.Add(@"C:\gg-code\tools\fontcompiler");
			result.IncludePathList.Add(@"C:\gg-code\tools\rescompiler");
			result.IncludePathList.Add(@"C:\gg-code\tools\tgatool");
			result.IncludePathList.Add(@"C:\gg-code\tools\veccompiler");
			result.IncludePathList.Add(@"C:\gg-code\users\marcel\vincent");
			result.IncludePathList.Add(@"C:\gg-code\usg");
			result.IncludePathList.Add(@"C:\gg-code\usg\Classes");
			result.IncludePathList.Add(@"C:\gg-code\usg\Classes\Game");

			//

			string directory = @"C:\gg-code\";

			result.SourcePathList.Add(new BuildSourcePath(directory + @"libgrs\libgrs_cpp\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"libgg\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"prototypes\vecrend\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"tools\atlcompiler\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"tools\fontcompiler\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"tools\rescompiler\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"tools\tgatool\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"tools\veccompiler\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"users\marcel\vincent\", "cpp", true));
			result.SourcePathList.Add(new BuildSourcePath(directory + @"usg\Classes\", "cpp", true));
			result.ExclusionList.Add("DebrisMgr.cpp");
			result.ExclusionList.Add("test_tga.cpp");
			result.ExclusionList.Add("EntitySpawn.cpp");
			result.ExclusionList.Add("_Util_Color.cpp");
			result.ExclusionList.Add("GameView.cpp");
			result.ExclusionList.Add("OpenGLState.cpp");
			result.ExclusionList.Add("View_PauseTouch.cpp");
			result.ExclusionList.Add("GameView_Psp.cpp");
			result.ExclusionList.Add("Graphics_Psp.cpp");
			result.ExclusionList.Add("PspFileStream.cpp");

			return result;
		}

		private static void Build(BuildSettings settings)
		{
			Console.WriteLine("\n\n");
			Console.WriteLine("starting compilation");
			Console.WriteLine("--------------------");

			BuildProcessor processor = new BuildProcessor();

			List<BuildObject> objList = new List<BuildObject>();

			//

			mTimeSearch -= DateTime.Now.Ticks;

			FileFinder finder = new FileFinder();

			foreach (BuildSourcePath sourcePath in settings.SourcePathList)
				finder.Add(sourcePath.Path, sourcePath.Extension, sourcePath.Recursive);
			foreach (string exclusion in settings.ExclusionList)
				finder.Exclude(exclusion);

			mTimeSearch += DateTime.Now.Ticks;

			//

			mTimeTranslate -= DateTime.Now.Ticks;

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

			mTimeTranslate += DateTime.Now.Ticks;

			mTimeWarmup -= DateTime.Now.Ticks;

			//processor.Warmup(includePathList, true);

			mTimeWarmup += DateTime.Now.Ticks;

			Console.WriteLine("begin");

			DateTime t1 = DateTime.Now;

			processor.Build(settings.DepScannerList, settings.IncludePathList, objList);

			DateTime t2 = DateTime.Now;

			decimal time = new decimal((t2 - t1).TotalSeconds);

			Console.WriteLine("done: {0} sec", time.ToString("N2"));

			BuildTool.Cpp.CppScanner.DEBUG_ShowStats();
			BuildTool.BuildCache.DEBUG_ShowStats();
			DEBUG_ShowStats();
		}

		private void button1_Click(object sender, EventArgs e)
		{
			//BuildSettings settings = MakeBuildSettings();
			BuildSettings settings = LoadBuildSettings();

			Build(settings);
		}

		private void button2_Click(object sender, EventArgs e)
		{
			for (int i = 0; i < 10; ++i)
			{
				string fileName1 = string.Format("source{0:0000}.cpp", i);
				string fileName2 = string.Format("source{0:0000}.h", i);

				string template = File.ReadAllText("source.txt");

				File.WriteAllText(fileName1, string.Format("#include \"{0}\"\n\n{1}", fileName2, template));
				File.WriteAllText(fileName2, "#pragma once");
			}
		}
	}
}

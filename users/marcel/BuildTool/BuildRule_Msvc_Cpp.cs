using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace BuildTool
{
	class BuildRule_Msvc_Cpp : IBuildRule
	{
		public string GetToolName()
		{
			return "msvc";
		}

		public string GetRuleName()
		{
			return "cpp";
		}

		public bool GetOutputName(FileName fileName, out FileName destination)
		{
			destination = null;

			if (Path.GetExtension(fileName.FileNameString) == ".cpp")
			{
				destination = new FileName(Path.ChangeExtension(fileName.FileNameString, ".obj"));
				return true;
			}

			return false;
		}

		public FileName GetDirectDependency(FileName fileName)
		{
			if (Path.GetExtension(fileName.FileNameString) == ".cpp")
			{
				return new FileName(Path.ChangeExtension(fileName.FileNameString, ".h"));
			}

			return null;
		}

		class WorkInfo
		{
			public WorkInfo(ProcessStartInfo startInfo, string source, string cmd)
			{
				StartInfo = startInfo;
				Source = source;
				Command = cmd;
			}

			public ProcessStartInfo StartInfo;
			public string Source;
			public string Command;
		}

		public void Build(BuildCtx ctx, BuildObject obj)
		{
			//Console.WriteLine("compiling {0} to {1}", obj.Source, obj.Destination);

			//cl -c /nologo /Od /MDd /GX /FoDebug\gzio.obj gzio.c

			string app = @"cmd.exe";
			string arguments = @"/K ""C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat""";

			string cmd = @"""C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\cl.exe"" ";

			cmd += string.Format("-c /nologo /Od /MDd /EHsc /Fo\"{0}\" \"{1}\" ",
				obj.Destination.FileNameString,
				obj.Source.FileNameString);
			cmd += "/D_USE_MATH_DEFINES ";

			foreach (string includePath in ctx.IncludePathList)
				cmd += string.Format("/I\"{0}\" ", includePath);
			/*
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\libgrs\libgrs_cpp");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\prototypes\libgg");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\prototypes\vecrend");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\tools\atlcompiler");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\tools\fontcompiler");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\tools\rescompiler");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\tools\tgatool");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\tools\veccompiler");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\users\marcel\vincent");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\usg");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\usg\Classes");
			cmd += string.Format("/I\"{0}\" ", @"C:\gg-code\usg\Classes\Game");*/
			//cmd += "/DOS=win ";

			Console.WriteLine("\"{0}\" {1}", app, arguments);

			ProcessStartInfo psi = new ProcessStartInfo(app, arguments);

			psi.CreateNoWindow = true;
			psi.ErrorDialog = false;
			//psi.RedirectStandardError = true;
			psi.RedirectStandardInput = true;
			// psi.RedirectStandardOutput = true;
			psi.UseShellExecute = false;

			WorkInfo workInfo = new WorkInfo(psi, obj.Source.FileNameString, cmd);

			Task task = new Task(Execute, workInfo);

			ctx.TaskMgr.Add(task);
		}

		private bool Execute(object obj)
		{
			WorkInfo workInfo = (WorkInfo)obj;

			try
			{
				DateTime t1 = DateTime.Now;

				using (Process process = Process.Start(workInfo.StartInfo))
				{
					process.StandardInput.WriteLine(workInfo.Command);

					process.StandardInput.WriteLine("exit");

					//string error = process.StandardError.ReadToEnd();
					//string output = process.StandardOutput.ReadToEnd();
					string error = string.Empty;
					string output = string.Empty;

					process.WaitForExit();

					if (!process.HasExited)
					{
						Console.WriteLine("process still running..");

						process.Kill();

						return false;
					}

					DateTime t2 = DateTime.Now;

					TimeSpan td = t2 - t1;

					if (error.Length > 0 || process.ExitCode != 0)
						BuildOutput.WriteLine(BOT.Error, workInfo.Source, 0, "{0}: ExitCode: {1} ({2:0.00}ms): {3}", workInfo.Source, process.ExitCode, td.TotalMilliseconds, output);
					else
						BuildOutput.WriteLine(BOT.Info, workInfo.Source, 0, "{0}: OK ({1:0.00}ms)", workInfo.Source, td.TotalMilliseconds);

					return process.ExitCode == 0;
				}
			}
			catch (Exception e)
			{
				BuildOutput.WriteLine(BOT.Error, workInfo.Source, 0, e.Message);

				return false;
			}
		}
	}
}
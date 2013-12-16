using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Threading;

namespace BuildTool
{
	class BuildRule_Gcc_Cpp : IBuildRule
	{
		public string GetToolName()
		{
			return "gcc";
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
				destination = new FileName(Path.ChangeExtension(fileName.FileNameString, ".o"));
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
			public WorkInfo(ProcessStartInfo startInfo, string fileName)
			{
				StartInfo = startInfo;
				FileName = fileName;
			}

			public ProcessStartInfo StartInfo;
			public string FileName;
		}

		public void Build(BuildCtx ctx, BuildObject obj)
		{
			//Console.WriteLine("compiling {0} to {1}", obj.Source, obj.Destination);

			string gcc = @"C:\MingW\bin\gcc.exe";
			string arguments = string.Format("-c \"{0}\" -o \"{1}\" ",
				obj.Source.FileNameString,
				obj.Destination.FileNameString);

			foreach (string includePath in ctx.IncludePathList)
				arguments += string.Format("-I\"{0}\" ", includePath);

			//arguments += "-O7 -ffast-math ";
			arguments += "-DOS=win ";
			arguments += "-DWIN32 ";
			arguments += "-g ";

			ProcessStartInfo psi = new ProcessStartInfo(gcc, arguments);

			psi.CreateNoWindow = true;
			psi.ErrorDialog = false;
			psi.RedirectStandardError = true;
			psi.UseShellExecute = false;

			WorkInfo workInfo = new WorkInfo(psi, obj.Source.FileNameString);

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
					string error = process.StandardError.ReadToEnd();

					process.WaitForExit();

					DateTime t2 = DateTime.Now;

					TimeSpan td = t2 - t1;

					if (error.Length > 0)
					{
						BuildOutput.WriteLine(BOT.Warning, workInfo.FileName, 0, "{0}", error);
					}
					else
					{
						BuildOutput.WriteLine(BOT.Info, workInfo.FileName, 0, "Completed ({0:0.00}ms)", td.TotalMilliseconds);
					}

					return process.ExitCode == 0;
				}
			}
			catch (Exception e)
			{
				BuildOutput.WriteLine(BOT.Error, workInfo.FileName, 0, e.Message);

				return false;
			}
		}
	}
}

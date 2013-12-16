using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;
using System.IO;

namespace KlodderHQ
{
    public static class StaticMethods
    {
        private static string BaseDirectory
        {
            get
            {
#if DEBUG
                return "C:/gg-code/klodder/Distro";
#else
                return Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
#endif
            }
        }

        public static void ExecuteKlodder(bool synchronous, params string[] args)
        {
            for (int i = 0; i < args.Length; ++i)
                if (args[i].Contains(' '))
                    args[i] = string.Format("\"{0}\"", args[i]);

            StringBuilder sbArgs = new StringBuilder();
            foreach (string arg in args)
                sbArgs.AppendFormat("{0} ", arg);
            string argsStr = sbArgs.ToString().Trim();

            ProcessStartInfo psi = new ProcessStartInfo();

            psi.Arguments = argsStr;
            psi.ErrorDialog = false;
            psi.FileName = Path.Combine(BaseDirectory, "klodder.exe");
            psi.UseShellExecute = false;
            psi.WorkingDirectory = BaseDirectory;
            psi.CreateNoWindow = true;

            if (synchronous)
            {
                psi.RedirectStandardOutput = true;
            }

            Process process = new Process();
            process.StartInfo = psi;
            process.Start();

            if (synchronous)
            {
                process.WaitForExit();

                string output = process.StandardOutput.ReadToEnd();

                if (process.ExitCode != 0)
                    throw X.Exception("The process exited with error code {0}. FileName: {1}, Arguments: {2}, Output: {3}", process.ExitCode, psi.FileName, psi.Arguments, output);
            }
        }
    }
}

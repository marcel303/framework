using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Text.RegularExpressions;

namespace BuildTool.Cpp
{
    public class CppScanner : IDepScanner
    {
        private Regex mRegex = new Regex("(.*)\"(?<file>.*)\"(.*)", RegexOptions.Compiled);
		private static long mTimeScanLocal = 0;

        public IList<string> GetDependencies(BuildCache cache, FileName fileName)
        {
            //Console.WriteLine("getting dependencies for {0}", fileName);

            using (Stream stream = File.Open(fileName.FileNameString, FileMode.Open, FileAccess.Read, FileShare.Read))
            {
                return GetDependencies_Local(stream);
            }
        }

        public IList<string> GetDependencies_Local(Stream stream)
        {
			mTimeScanLocal -= DateTime.Now.Ticks;

            List<string> result = new List<string>();

            // todo: preprocess

            using (StreamReader reader = new StreamReader(stream))
            {
                string line;

                while ((line = reader.ReadLine()) != null)
                {
                    line = line.TrimStart();

                    if (!line.StartsWith("#include"))
                        continue;

                    // perform regex

                    Match match = mRegex.Match(line);
                    
                    if (!match.Success)
                        continue;

                    Group group = match.Groups["file"];

                    string dep = group.Value;

                    result.Add(dep);
                }
            }

			mTimeScanLocal += DateTime.Now.Ticks;

            return result;
        }

		public static void DEBUG_ShowStats()
		{
			Console.WriteLine("TimeScanLocal: {0} sec", mTimeScanLocal / 10000000.0);
		}
    }
}

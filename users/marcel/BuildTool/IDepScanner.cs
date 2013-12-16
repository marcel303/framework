using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace BuildTool
{
	public class FileName
	{
		public FileName(string fileName)
		{
			mFileName = Normalize(fileName);

		}
		public FileName(string fileName, int depLevel)
		{
			mFileName = Normalize(fileName);
			DepLevel = depLevel;
		}

		private static string Normalize(string fileName)
		{
			return Path.GetFullPath(fileName).Replace('\\', '/').ToLower();
		}

		public string FileNameString
		{
			get
			{
				return mFileName;
			}
		}

		public int DepLevel = 0;

		public override bool Equals(object obj)
		{
			FileName other = obj as FileName;

			if (other == null)
				return false;

			return mFileName == other.mFileName;
		}

		public override int GetHashCode()
		{
			return mFileName.GetHashCode();
		}

		private string mFileName;
	}

    public interface IDepScanner
    {
        IList<string> GetDependencies(BuildCache cache, FileName fileName);
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace BuildTool
{
	public class FileFinder
	{
		private List<FileName> mFiles = new List<FileName>();

		public IList<FileName> Files
		{
			get
			{
				return mFiles;
			}
		}

		public void Add(string path, string extension, bool recursive)
		{
			SearchOption option = recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;

			string[] fileList = Directory.GetFiles(path, "*." + extension, option);

			foreach (string file in fileList)
				mFiles.Add(new FileName(file));
		}

		public void Exclude(string fileName)
		{
			List<FileName> todo = new List<FileName>();

			foreach (FileName file in mFiles)
			{
				if (Path.GetFileName(file.FileNameString) == fileName.ToLower())
					todo.Add(file);
			}

			foreach (FileName file in todo)
				mFiles.Remove(file);
		}
	}
}

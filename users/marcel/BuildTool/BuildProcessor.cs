using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace BuildTool
{
	public class BuildCache
	{
		private Dictionary<string, DateTime> mFileModificationTime = new Dictionary<string, DateTime>();
		private Dictionary<string, bool> mFileExists = new Dictionary<string, bool>();
		private static long mTimeFileDate = 0;
		private static long mTimeFileExists = 0;

		public DateTime GetFileModificationTime(string fileName)
		{
			mTimeFileDate -= DateTime.Now.Ticks;

			DateTime result;

			if (mFileModificationTime.TryGetValue(fileName, out result))
			{
				mTimeFileDate += DateTime.Now.Ticks;

				return result;
			}

			//Console.WriteLine("getting last modification time for {0}", fileName);

			result = File.GetLastWriteTime(fileName);

			mFileModificationTime.Add(fileName, result);

			mTimeFileDate += DateTime.Now.Ticks;

			return result;
		}

		public bool FileExists(FileName fileName)
		{
			mTimeFileExists -= DateTime.Now.Ticks;

			bool result;

			if (mFileExists.TryGetValue(fileName.FileNameString, out result))
			{
				mTimeFileExists += DateTime.Now.Ticks;

				return result;
			}

			result = File.Exists(fileName.FileNameString);

			mFileExists.Add(fileName.FileNameString, result);

			mTimeFileExists += DateTime.Now.Ticks;

			return result;
		}

		public void AddFileExists(string[] fileList)
		{
			foreach (string file in fileList)
			{
				if (!mFileExists.ContainsKey(file))
					mFileExists.Add(file, true);
			}
		}

		class DepCacheEntry
		{
			public DepCacheEntry(DateTime date, IList<string> depList)
			{
				Date = date;
				DepList = depList;
			}

			public DateTime Date;
			public IList<string> DepList;
		}
		Dictionary<IDepScanner, Dictionary<string, DepCacheEntry>> mDepCache = new Dictionary<IDepScanner, Dictionary<string, DepCacheEntry>>();

		public IList<string> GetDependencies(FileName fileName, IDepScanner depScanner)
		{
			DateTime date = GetFileModificationTime(fileName.FileNameString);

			Dictionary<string, DepCacheEntry> depListEntryByFile = null;
			DepCacheEntry depListEntry = null;
			if (mDepCache.TryGetValue(depScanner, out depListEntryByFile))
				if (depListEntryByFile.TryGetValue(fileName.FileNameString, out depListEntry))
					if (depListEntry.Date == date)
						return depListEntry.DepList;

			IList<string> depList = depScanner.GetDependencies(this, fileName);

			if (depListEntryByFile == null)
			{
				depListEntryByFile = new Dictionary<string, DepCacheEntry>();
				mDepCache.Add(depScanner, depListEntryByFile);
			}

			if (depListEntry == null)
			{
				depListEntry = new DepCacheEntry(date, depList);
				depListEntryByFile.Add(fileName.FileNameString, depListEntry);
			}
			else
			{
				depListEntry.Date = date;
				depListEntry.DepList = depList;
			}

			return depList;
		}

		public static void DEBUG_ShowStats()
		{
			Console.WriteLine("TimeFileDate: {0} sec", mTimeFileDate / 10000000.0);
			Console.WriteLine("TimeFileExists: {0} sec", mTimeFileExists / 10000000.0);
		}
	}

	public class BuildCtx
	{
		public TaskMgr TaskMgr = new TaskMgr();
		public IList<string> IncludePathList;
	}

	public class BuildProcessor
	{
		private BuildCache mCache = new BuildCache();
		private BuildCtx mCtx = new BuildCtx();

		public void Build(Dictionary<string, IDepScanner> depScannerList, IList<string> includePathList, IList<BuildObject> objList)
		{
			bool syncEachLevel = true;
			bool bailOnErrors = true;
			bool forceBuild = false;

			//

			mCtx.IncludePathList = includePathList;

			List<BuildObject> level1 = new List<BuildObject>(); // out of date because direct src for dst is outdated
			List<BuildObject> level2 = new List<BuildObject>(); // out of date because one of its dependencies is outdated, where src name is similar to an outdated dependency
			List<BuildObject> levelX = new List<BuildObject>(); // out of date because one of its dependencies is outdated

			foreach (BuildObject obj in objList)
			{
				DateTime objSrcTime = mCache.GetFileModificationTime(obj.Source.FileNameString);
				DateTime objDstTime = mCache.GetFileModificationTime(obj.Destination.FileNameString);

				if (objSrcTime > objDstTime)
				{
					//Console.WriteLine("{0} outdated", obj.Destination);

					level1.Add(obj);
				}
				else
				{
					Dictionary<FileName, bool> depList = new Dictionary<FileName, bool>();

					DepList_GetFull(mCache, 0, obj.Source, depScannerList, includePathList, depList);

					bool build = false;
					bool hasDirectDep = false;

					foreach (KeyValuePair<FileName, bool> dep in depList)
					{
						FileName depFileName = dep.Key;

						//Console.WriteLine("\tdep: {0}", depFileName);

						DateTime depTime = mCache.GetFileModificationTime(depFileName.FileNameString);

						if (depTime > objDstTime || forceBuild)// ||objList.IndexOf(obj) <= 50)
						{
							//Console.WriteLine("{0} outdated due to dependency {1}", obj.Destination, depFileName);

							build = true;

							if (obj.DirectDependency == null)
								break;

							if (depFileName == obj.DirectDependency)
							{
								hasDirectDep = true;

								break;
							}
						}
					}

					if (build)
					{
						if (hasDirectDep)
							level2.Add(obj);
						else
							levelX.Add(obj);
					}
					else
					{
						//Console.WriteLine("{0} up to date", obj.Destination);
					}
				}
			}

			mCtx.TaskMgr.Start(bailOnErrors);

			//

			List<BuildObject>[] levelList = { level1, level2, levelX };

			foreach (List<BuildObject> level in levelList)
			{
				Console.WriteLine("building {0} files in current level", level.Count);

				level.Sort((v1, v2) => mCache.GetFileModificationTime(v2.Source.FileNameString).CompareTo(mCache.GetFileModificationTime(v1.Source.FileNameString)));

				foreach (BuildObject obj in level)
					obj.BuildRule.Build(mCtx, obj);

				if (syncEachLevel)
				{
					mCtx.TaskMgr.WaitForFinish();
					if (bailOnErrors && mCtx.TaskMgr.HadErrors())
						break;
					mCtx.TaskMgr.Start(bailOnErrors);
				}
			}

			//

			if (mCtx.TaskMgr.IsRunning)
			{
				mCtx.TaskMgr.WaitForFinish();
			}
		}

		public void Warmup(IList<string> includePathList, bool recursion)
		{
			SearchOption option = recursion ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;

			foreach (string includePath in includePathList)
			{
				string[] fileList = Directory.GetFiles(includePath, "*", option);

				mCache.AddFileExists(fileList);
			}
		}

		public void DepList_GetFull(BuildCache cache, int depLevel, FileName source, Dictionary<string, IDepScanner> depScannerList, IList<string> includePathList, Dictionary<FileName, bool> out_depList)
		{
			if (cache == null)
				cache = mCache;

			string extension = Path.GetExtension(source.FileNameString);

			if (extension != string.Empty)
				extension = extension.Substring(1);

			IDepScanner depScanner;

			if (!depScannerList.TryGetValue(extension, out depScanner))
				return;

			IList<string> depList = cache.GetDependencies(source, depScanner);

			for (int i = 0; i < depList.Count; ++i)
			{
				string dep = depList[i];
				FileName depFull2 = null;

				foreach (string includePath in includePathList)
				{
					FileName depFull = new FileName(Path.Combine(includePath, dep), depLevel);

					if (cache.FileExists(depFull))
					{
						depFull2 = depFull;

						break;
					}
				}

				if (depFull2 == null)
				{
					Console.WriteLine("warning: unable to locate dependency: {0}", dep);
					continue;
				}

				if (out_depList.ContainsKey(depFull2))
					continue;

				out_depList.Add(depFull2, true);

				DepList_GetFull(cache, depLevel + 1, depFull2, depScannerList, includePathList, out_depList);
			}
		}
	}
}

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace FontGen
{
	public class Mru
	{
		private string mFileName;
		private int mMaxItemCount;
		private List<string> mMruList = new List<string>();

		public string[] MruList
		{
			get
			{
				return mMruList.ToArray();
			}
		}

		public Mru(string fileName, int maxItemCount)
		{
			mFileName = fileName;
			mMaxItemCount = maxItemCount;

			Load();
		}

		public void AddOrUpdate(string text)
		{
			mMruList.Remove(text);

			mMruList.Insert(0, text);

			while (mMruList.Count > mMaxItemCount)
				mMruList.RemoveAt(mMruList.Count - 1);

			Save();
		}

		private void Load()
		{
			try
			{
				mMruList.Clear();

				if (!File.Exists(mFileName))
					return;

				mMruList = File.ReadAllLines(mFileName).ToList();
			}
			catch
			{
				// nop
			}
		}

		private void Save()
		{
			try
			{
				File.WriteAllLines(mFileName, mMruList.ToArray());
			}
			catch
			{
			}
		}
	}
}

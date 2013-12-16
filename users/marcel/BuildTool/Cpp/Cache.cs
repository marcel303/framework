using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BuildTool.Cpp
{
    class Cache
    {
        private Dictionary<string, CacheItem> mItems = new Dictionary<string, CacheItem>();
        private CppScanner mScanner = new CppScanner();

        public CacheItem GetItem(BuildCache cache, FileName fileName)
        {
            CacheItem item;

            if (mItems.TryGetValue(fileName.FileNameString, out item))
            {
                if (cache.GetFileModificationTime(fileName.FileNameString) == item.ModificationTime)
                {
                    return item;
                }
                else
                {
                    //mItems.Remove(fileName);
                }
            }

            item = new CacheItem();

            item.ModificationTime = cache.GetFileModificationTime(fileName.FileNameString);
            item.Dependencies = mScanner.GetDependencies(cache, fileName);

            mItems.Add(fileName.FileNameString, item);

            return item;
        }
    }
}

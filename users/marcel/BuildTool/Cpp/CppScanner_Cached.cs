using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BuildTool.Cpp
{
    public class CppScanner_Cached : IDepScanner
    {
        private Cache mCache = new Cache();

        public IList<string> GetDependencies(BuildCache cache, FileName fileName)
        {
            CacheItem item = mCache.GetItem(cache, fileName);

            return item.Dependencies;
        }
    }
}

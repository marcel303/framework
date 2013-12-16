using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BuildTool.Cpp
{
    class CacheItem
    {
        public DateTime ModificationTime;
        public IList<string> Dependencies;
    }
}

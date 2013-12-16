using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Globalization;

namespace KlodderHQ
{
    public static class X
    {
        public static Exception Exception(string text, params object[] args)
        {
            text = string.Format(text, args);

            throw new Exception(text);
        }

        public static CultureInfo CultureEN = new CultureInfo("en-US");
    }
}

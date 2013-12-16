using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using Microsoft.Win32;

namespace KlodderHQ
{
    class Settings
    {
        private static IRegistry Registry = new BasicRegistry(@"gg\KlodderHQ");

        /*private static string FileName(string name)
        {
            return string.Format("hq.cfg.{0}.txt", name);
        }*/

        private static string BoolToString(bool value)
        {
            if (value)
                return "1";
            else
                return "0";
        }

        private static bool StringToBool(string value)
        {
            if (value == "0")
                return false;
            else
                return true;
        }

        public static string Get(string name, string @default)
        {
            try
            {
                return Registry.Get(name, @default);

                //return File.ReadAllText(FileName(name));
            }
            catch
            {
                return @default;
            }
        }

        public static int Get(string name, int @default)
        {
            string value = Get(name, @default.ToString(X.CultureEN));

            try
            {
                return Int32.Parse(value, X.CultureEN);
            }
            catch
            {
                return @default;
            }
        }

        public static bool Get(string name, bool @default)
        {
            string value = Get(name, BoolToString(@default));

            try
            {
                return StringToBool(value);
            }
            catch
            {
                return @default;
            }
        }

        public static void Set(string name, string value)
        {
            try
            {
                //File.WriteAllText(FileName(name), value);

                Registry.Set(name, value);
            }
            catch
            {
            }
        }

        public static void Set(string name, int value)
        {
            try
            {
                Set(name, value.ToString(X.CultureEN));
            }
            catch
            {
            }
        }

        public static void Set(string name, bool value)
        {
            try
            {
                Set(name, BoolToString(value));
            }
            catch
            {
            }
        }
    }
}

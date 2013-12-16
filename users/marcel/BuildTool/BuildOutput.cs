using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;

namespace BuildTool
{
    class BuildOutput
    {
        private static Mutex mMutex = new Mutex();

        public static void WriteLine(BOT bot, string fileName, int line, string text, params object[] args)
        {
			mMutex.WaitOne();

			Console.WriteLine("{0}: {1} ({2}): {3}", bot, fileName, line, string.Format(text, args));

#if false
			BuildAgent.Deliver(new BuildMessage(bot, fileName, line, string.Format(text, args)));
#endif

			mMutex.ReleaseMutex();
        }
    }
}

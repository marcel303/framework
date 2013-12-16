using System;

class Logger
{
	public static void Log(String text, params object[] objs)
	{
#if DEBUG
		Console.WriteLine(text, objs);
#endif
	}
}
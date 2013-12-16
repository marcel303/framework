using System;

namespace FontShared
{
	public class MyException : Exception
	{
		public MyException(string text, params object[] args)
			: base(string.Format(text, args))
		{
		}
	}
}

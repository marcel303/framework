using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;

namespace vecdraw
{
	#region Angle

	public class VecAngle
	{
		public VecAngle()
		{
		}

		public int Degrees = 0;
	}

	#endregion

	#region Color

	public class VecColor
	{
		public VecColor()
		{
			R = 255;
			G = 255;
			B = 255;
		}

		public VecColor(int r, int g, int b)
		{
			R = r;
			G = g;
			B = b;
		}

		public int R;
		public int G;
		public int B;
	}

	#endregion

	#region FileName

	public class FileName
	{
		private static FileName m_Empty = new FileName(String.Empty);

		public static FileName Empty
		{
			get
			{
				return m_Empty;
			}
		}

		public FileName(String path)
		{
			Path = path;
		}

		public String Path;
	}

	#endregion

	public class Global
	{
		public static CultureInfo CultureEN = new CultureInfo("en-US");
	}

	public class X
	{
		public static void Assert(bool expression)
		{
			System.Diagnostics.Debug.Assert(expression);
		}
	}
}

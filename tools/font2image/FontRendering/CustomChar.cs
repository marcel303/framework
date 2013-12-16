using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace FontRendering
{
	public class CustomChar
	{
		public CustomChar(char code, string fileName, int scalePct, int offsetPct)
		{
			Code = code;
			FileName = fileName;
			ScalePct = scalePct;
			OffsetPct = offsetPct;
		}

		public char Code = (char)0;
		public string FileName
		{
			get
			{
				return mFileName;
			}
			set
			{
				//mFileName = Path.GetFileName(value);
				mFileName = value;
			}
		}
		public int ScalePct = 100;
		public int OffsetPct = 0;

		private string mFileName;
	}
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace vecdraw
{
	public class RangeProperyAttribute : PropertyAttribute
	{
		public RangeProperyAttribute(String name, int min, int max)
			: base(name)
		{
			Min = min;
			Max = max;
		}

		public int Min;
		public int Max;
	}
}

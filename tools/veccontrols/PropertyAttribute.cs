using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace vecdraw
{
	public class PropertyAttribute : Attribute
	{
		public PropertyAttribute(String name)
		{
			m_Name = name;
		}

		public String Name
		{
			get
			{
				return m_Name;
			}
		}

		private String m_Name;
	}
}

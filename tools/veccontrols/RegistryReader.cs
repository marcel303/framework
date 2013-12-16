using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace vecdraw
{
	public class RegistryIO
	{
		private IRegistry m_Registry;

		public RegistryIO(IRegistry registry)
		{
			m_Registry = registry;
		}

		public String Get(String key, String @default)
		{
			return m_Registry.Get(key, @default);
		}

		public int Get(String key, int @default)
		{
			return int.Parse(m_Registry.Get(key, @default.ToString(Global.CultureEN)));
		}

		public float Get(String key, float @default)
		{
			return float.Parse(m_Registry.Get(key, @default.ToString(Global.CultureEN)));
		}

		public void Set(String key, String value)
		{
			m_Registry.Set(key, value);
		}

		public void Set(String key, int value)
		{
			m_Registry.Set(key, value.ToString(Global.CultureEN));
		}

		public void Set(String key, float value)
		{
			m_Registry.Set(key, value.ToString(Global.CultureEN));
		}
	}
}

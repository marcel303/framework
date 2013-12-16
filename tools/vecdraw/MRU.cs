using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace vecdraw
{
	class MRU
	{
		private RegistryIO m_Registry;

		public MRU(String path)
		{
			m_Registry = new RegistryIO(new BasicRegistry(@"GG\vecdraw"));
		}

		private String MakeKey(int index)
		{
			return String.Format(@"MRU\Item{0}", index);
		}

		public void Load()
		{
			int size = m_Registry.Get("Size", 0);

			for (int i = size - 1; i >= 0; i--)
			{
				String path = m_Registry.Get(MakeKey(i), String.Empty);

				Add(path);
			}

			Sanitize();
		}

		public void Save()
		{
			m_Registry.Set("Size", m_MRU.Count);

			for (int i = 0; i < m_MRU.Count; ++i)
			{
				m_Registry.Set(MakeKey(i), m_MRU[i]);
			}
		}

		public void Sanitize()
		{
			String[] fileNames = m_MRU.ToArray();

			foreach (String fileName in fileNames)
				if (!File.Exists(fileName))
					m_MRU.Remove(fileName);
		}

		public void Add(String fileName)
		{
			if (m_MRU.Contains(fileName))
				m_MRU.Remove(fileName);

			m_MRU.Insert(0, fileName);
		}

		public String[] Files
		{
			get
			{
				return m_MRU.ToArray();
			}
		}

		private List<String> m_MRU = new List<string>();
	}
}

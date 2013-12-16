using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace vecdraw
{
	class PathEx
	{
		IList<String> m_Nodes = new List<String>();

		PathEx()
		{
		}

		PathEx(String path)
		{
			Parse(path);
		}

		void Parse(String path)
		{
			String temp = Sanitize(path);

			m_Nodes = temp.Split('/');
		}

		public static String GetRelativePath(String a_BaseDirectory, String a_Path)
		{
			PathEx path1 = new PathEx();
			PathEx path2 = new PathEx();

			path1.Parse(a_BaseDirectory);
			path2.Parse(a_Path);

			path1.MakeCompact();
			path2.MakeCompact();

			PathEx path3 = new PathEx();

			path3.MakeRelative(path1, path2);
			path3.MakeCompact();

			String result = path3.ToString();

			return result;
		}

		void MakeRelative (PathEx a_BaseDirectory, PathEx a_Path)
		{
			int n1 = (int)a_BaseDirectory.m_Nodes.Count;
			int n2 = (int)a_Path.m_Nodes.Count;

			int n = n1 < n2 ? n1 : n2;

			int i;

			for (i = 0; i < n && a_BaseDirectory.m_Nodes[i] == a_Path.m_Nodes[i]; ++i)
			{
				// nop
			}

			for (int j = i; j < n1; ++j)
				m_Nodes.Add("..");

			//m_Nodes = a_BaseDirectory.m_Nodes;

			for (; i < n2; ++i)
				m_Nodes.Add(a_Path.m_Nodes[i]);
		}

		void MakeCompact()
		{
			List<String> temp = new List<string>();

			for (int i = 0; i < m_Nodes.Count; ++i)
			{
				if (i != 0 && m_Nodes[i] == ".." && m_Nodes[i - 1] != "..")
				{
					temp.RemoveAt(temp.Count - 1);
				}
				else
				{
					temp.Add(m_Nodes[i]);
				}
			}

			m_Nodes = temp;
		}

		public override String ToString()
		{
			StringBuilder result = new StringBuilder();

			for (int i = 0; i < m_Nodes.Count; ++i)
			{
				if (i != 0)
					result.Append("/");

				result.Append(m_Nodes[i]);
			}

			return result.ToString();
		}

		static String Sanitize(String path)
		{
			StringBuilder result = new StringBuilder();

			for (int i = 0; i < path.Length; )
			{
				char c1 = SafeRead(path, i + 0);
				char c2 = SafeRead(path, i + 1);

				if (c1 == '\\')
					c1 = '/';
				if (c2 == '\\')
					c2 = '/';

				if (c1 == '/' && c2 == '/')
				{
					result.Append(c1);

					i += 2;
				}
				else
				{
					result.Append(c1);

					i += 1;
				}
			}

			String temp = result.ToString();

			if (temp.EndsWith("/"))
				temp = temp.Substring(0, temp.Length - 1);

			return temp;
		}

		static char SafeRead(String a_String, int a_Position)
		{
			if (a_Position < 0 || a_Position >= a_String.Length)
				return (char)0;

			return a_String[a_Position];
		}
	}
}

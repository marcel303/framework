using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Win32;
using System.Security;
using System.Security.Permissions;

namespace KlodderHQ
{
	public interface IRegistry
	{
		String Get(String path, String @default);
		void Set(String path, String value);
	}

	public class BasicRegistry : IRegistry
	{
		private String m_Application;

		public BasicRegistry(String application)
		{
			m_Application = application;
		}

		private String Path
		{
			get
			{
				String result = "Software" + @"\" + m_Application;

				return result;
			}
		}

		private RegistryKey Key
		{
			get
			{
				return Registry.CurrentUser;
			}
		}

		public String Get(String key, String @default)
		{
			PermissionSet permissionSet = new PermissionSet(PermissionState.Unrestricted);
			permissionSet.Assert();

			RegistryKey regKey = Key.OpenSubKey(Path, false);

			if (regKey == null)
			{
				Set(key, @default);

				regKey = Key.OpenSubKey(Path, false);
			}

			String result = regKey.GetValue(key, @default).ToString();

			if (Object.ReferenceEquals(result, @default))
			{
				Set(key, @default);
			}

			regKey.Close();

			PermissionSet.RevertAssert();

			return result;
		}

		public void Set(String key, String value)
		{
			PermissionSet permissionSet = new System.Security.PermissionSet(System.Security.Permissions.PermissionState.Unrestricted);
			permissionSet.Assert();

			RegistryKey regKey = Key.OpenSubKey(Path, true);

			if (regKey == null)
			{
				regKey = Key.CreateSubKey(Path);
			}

			regKey.SetValue(key, value);

			regKey.Close();

			PermissionSet.RevertAssert();
		}
	}
}

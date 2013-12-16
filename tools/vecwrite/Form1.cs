using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using vecdraw;

namespace vecwrite
{
	public partial class Form1 : Form
	{
		private RegistryIO m_Registry = new RegistryIO(new BasicRegistry(@"GG\vecdraw"));

		public Form1()
		{
			InitializeComponent();
		}

		public String PreviewExecutable
		{
			get
			{
				return m_Registry.Get("PreviewExecutable", String.Empty);
			}
			set
			{
				m_Registry.Set("PreviewExecutable", value);
			}
		}

		void Preview()
		{
			try
			{
				String previewExecutable = PreviewExecutable;

				if (previewExecutable == String.Empty)
				{
					throw new Exception("Preview executable not set");
				}

				String temp = Path.GetTempFileName();

				File.WriteAllText(temp, textBox1.Text);

				ProcessStartInfo psi = new ProcessStartInfo(previewExecutable, String.Format("\"{0}\"", temp));
				Process process = new Process();
				process.StartInfo = psi;
				process.Start();
			}
			catch (Exception e)
			{
				MessageBox.Show(String.Format("Failed to preview graphic: {0}", e.Message));
			}
		}

		private void toolStripButton1_Click(object sender, EventArgs args)
		{
			Preview();
		}

		private void Form1_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.F5)
				Preview();
		}
	}
}

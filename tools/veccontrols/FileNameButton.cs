using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
//using vecdraw.Elements;

namespace vecdraw
{
	public partial class FileNameButton : UserControl
	{
		public event EventHandler OnChange;

		public FileNameButton()
		{
			InitializeComponent();
		}

		public FileName FileName
		{
			get
			{
				return new FileName(openFileDialog1.FileName);
			}
			set
			{
				openFileDialog1.FileName = value.Path;
			}
		}

		private void button1_Click(object sender, EventArgs e)
		{
			if (openFileDialog1.ShowDialog() == DialogResult.OK)
			{
				if (OnChange != null)
					OnChange(this, null);
			}
		}
	}
}

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace veccompose
{
	public partial class Form_Help : Form
	{
		public Form_Help()
		{
			InitializeComponent();

			try
			{
				String text = Resource1.ReadMe;

				textBox1.Text = text;

				textBox1.Select(0, 0);
			}
			catch (Exception e)
			{
				String text = String.Format("Unable to load ReadMe.txt:\n{0}", e.Message);

				textBox1.Text = text;
			}
		}
	}
}

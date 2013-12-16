using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace veccompose
{
	public partial class Form_ItemAdd : Form
	{
		public Form_ItemAdd()
		{
			InitializeComponent();
		}

		public String ItemFileName
		{
			get
			{
				return textBox1.Text;
			}
			set
			{
				textBox1.Text = value;
			}
		}

		public String ItemName
		{
			get
			{
				return textBox2.Text;
			}
			set
			{
				textBox2.Text = value;
			}
		}

		public String ItemGroup
		{
			get
			{
				return textBox3.Text;
			}
			set
			{
				textBox3.Text = value;
			}
		}

		public String ItemPivot
		{
			get
			{
				return textBox4.Text;
			}
			set
			{
				textBox4.Text = value;
			}
		}

		private void button1_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.OK;

			Close();
		}

		private void button2_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.Cancel;

			Close();
		}

		private void button3_Click(object sender, EventArgs e)
		{
			if (openFileDialog1.ShowDialog() == DialogResult.OK)
			{
				// todo: calculate relative file name
				textBox1.Text = System.IO.Path.GetFileName(openFileDialog1.FileName);
			}
		}
	}
}

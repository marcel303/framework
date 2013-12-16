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
	public partial class Form_LinkAdd : Form
	{
		public Form_LinkAdd(ShapeLibrary library)
		{
			InitializeComponent();

			foreach (ShapeLibraryItem item in library.m_Items)
			{
				comboBox1.Items.Add(item.m_Name);
			}
		}

		private void button2_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.Cancel;

			Close();
		}

		private void button1_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.OK;

			Close();
		}

		public String LinkName
		{
			get
			{
				return textBox1.Text;
			}
		}

		public String LinkShape
		{
			get
			{
				return comboBox1.Text;
			}
		}

		public Point LinkLocation
		{
			get
			{
				int x = int.Parse(textBox4.Text);
				int y = int.Parse(textBox5.Text);

				return new Point(x, y);
			}
		}

		public int LinkMinAngle
		{
			get
			{
				return int.Parse(textBox6.Text);
			}
		}

		public int LinkMaxAngle
		{
			get
			{
				return int.Parse(textBox8.Text);
			}
		}

		public int LinkBaseAngle
		{
			get
			{
				return int.Parse(textBox7.Text);
			}
		}

		public int LinkMinLevel
		{
			get
			{
				return int.Parse(textBox3.Text);
			}
			set
			{
				textBox3.Text = value.ToString();
			}
		}

		public int LinkMaxLevel
		{
			get
			{
				return int.Parse(textBox2.Text);
			}
			set
			{
				textBox2.Text = value.ToString();
			}
		}
	}
}

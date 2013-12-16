using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace vecdraw
{
	public partial class Form_Attributes : Form
	{
		private Form1 m_Form;
		private GraphicAttributes m_Attributes;

		public Form_Attributes(Form1 form, GraphicAttributes attributes)
		{
			InitializeComponent();

			m_Form = form;
			m_Attributes = attributes;

			// graphics settings
			textBox1.Text = m_Attributes.Width.ToString();
			textBox2.Text = m_Attributes.Height.ToString();
			checkBox1.Checked = m_Form.vectorView1.m_Shape.m_Attributes.HasSilhouette;
			checkBox2.Checked = m_Form.vectorView1.m_Shape.m_Attributes.HasSprite;

			// application settings
			textBox3.Text = m_Form.PreviewExecutable;
		}

		private void button2_Click(object sender, EventArgs e)
		{
			textBox1.Text = "320";
			textBox2.Text = "480";
		}

		private void button3_Click(object sender, EventArgs e)
		{
			textBox1.Text = "480";
			textBox2.Text = "320";
		}

		private void button1_Click(object sender, EventArgs e)
		{
			Close();
		}

		private void Form_Attributes_FormClosed(object sender, FormClosedEventArgs args)
		{
			try
			{
				// graphics settings
				m_Attributes.Width = Convert.ToInt32(textBox1.Text);
				m_Attributes.Height = Convert.ToInt32(textBox2.Text);
				m_Form.vectorView1.m_Shape.m_Attributes.HasSilhouette = checkBox1.Checked;
				m_Form.vectorView1.m_Shape.m_Attributes.HasSprite = checkBox2.Checked;
				
				// application settings
				m_Form.PreviewExecutable = textBox3.Text;
			}
			catch (Exception e)
			{
				MessageBox.Show(e.Message);
			}
		}

		private void button4_Click(object sender, EventArgs e)
		{
			if (openFileDialog1.ShowDialog() != DialogResult.OK)
				return;

			m_Form.PreviewExecutable = openFileDialog1.FileName;
		}
	}
}

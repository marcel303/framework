using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ASCIIedit
{
	public partial class Form_SetSize : Form
	{
		public int SX
		{
			get
			{
				int result;
				if (int.TryParse(textBox1.Text, out result))
					return result;
				else
					return 0;
			}
			set
			{
				textBox1.Text = value.ToString();
			}
		}

		public int SY
		{
			get
			{
				int result;
				if (int.TryParse(textBox2.Text, out result))
					return result;
				else
					return 0;
			}
			set
			{
				textBox2.Text = value.ToString();
			}
		}

		public Form_SetSize()
		{
			InitializeComponent();
		}

		private void button1_Click(object sender, EventArgs e)
		{
			if (SX > 0 && SY > 0)
			{
				DialogResult = System.Windows.Forms.DialogResult.OK;
				Close();
			}
			else
			{
				MessageBox.Show("Invalid input");
			}
		}

		private void button2_Click(object sender, EventArgs e)
		{
			DialogResult = System.Windows.Forms.DialogResult.Cancel;
			Close();
		}
	}
}

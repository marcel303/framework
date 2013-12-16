using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace vecdraw
{
	public partial class AngleControl : UserControl
	{
		public event EventHandler ValueChanged;

		public AngleControl()
		{
			InitializeComponent();
		}

		public int Value
		{
			get
			{
				return m_Value;
			}
			set
			{
				if (value == m_Value)
					return;

				trackBar1.Value = value;
				textBox1.Text = value.ToString();

				m_Value = value;

				if (ValueChanged != null)
					ValueChanged(this, null);
			}
		}

		private int m_Value = 0;

		private void textBox1_TextChanged(object sender, EventArgs e)
		{
			try
			{
				int value = int.Parse(textBox1.Text);

				Value = value;
			}
			catch
			{
			}
		}

		private void trackBar1_ValueChanged(object sender, EventArgs e)
		{
			Value = trackBar1.Value;
		}
	}
}

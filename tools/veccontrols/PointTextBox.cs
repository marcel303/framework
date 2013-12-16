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
	public partial class PointTextBox : UserControl
	{
		public event EventHandler OnPointChanged;

		public PointTextBox()
		{
			InitializeComponent();
		}

		public String TextX
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

		public String TextY
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

		private void HandleTextChanged()
		{
			if (OnPointChanged != null)
				OnPointChanged(this, null);
		}

		private void textBox1_TextChanged(object sender, EventArgs e)
		{
			HandleTextChanged();
		}

		private void textBox2_TextChanged(object sender, EventArgs e)
		{
			HandleTextChanged();
		}
	}
}

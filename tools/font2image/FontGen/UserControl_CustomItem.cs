using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using FontRendering;

namespace FontGen
{
	public partial class UserControl_CustomItem : UserControl
	{
		public delegate void DeleteHandler(UserControl_CustomItem sender, CustomChar customChar);
		public event DeleteHandler OnDelete;

		public CustomChar CustomChar { get; set; }

		public UserControl_CustomItem(CustomChar customChar)
		{
			InitializeComponent();

			CustomChar = customChar;

			CharacterCode = CustomChar.Code;
			FileName = CustomChar.FileName;
			ScalePct = CustomChar.ScalePct;
			OffsetPct = CustomChar.OffsetPct;
		}

		private char CharacterCode
		{
			get
			{
				return CustomChar.Code;
			}
			set
			{
				if (value < 0)
					value = (char)0;
				else if (value > 255)
					value = (char)255;

				CustomChar.Code = value;

				textBox1.Text = ((int)value).ToString(FontShared.X.CultureEN);
			}
		}

		private string FileName
		{
			get
			{
				return CustomChar.FileName;
			}
			set
			{
				CustomChar.FileName = value;

				try
				{
					pictureBox1.ImageLocation = CustomChar.FileName;
				}
				catch
				{
				}
			}
		}

		private int ScalePct
		{
			get
			{
				return CustomChar.ScalePct;
			}
			set
			{
				CustomChar.ScalePct = value;

				textBox2.Text = value.ToString(FontShared.X.CultureEN);
			}
		}

		private int OffsetPct
		{
			get
			{
				return CustomChar.OffsetPct;
			}
			set
			{
				if (value < 0)
					value = 0;
				if (value > 100)
					value = 100;

				CustomChar.OffsetPct = value;

				textBox3.Text = value.ToString(FontShared.X.CultureEN);
			}
		}

		private void textBox1_TextChanged(object sender, EventArgs e)
		{
			int code;

			if (!int.TryParse(textBox1.Text, System.Globalization.NumberStyles.Number, FontShared.X.CultureEN, out code))
				return;

			CharacterCode = (char)code;
		}

		private void textBox1_Leave(object sender, EventArgs e)
		{
			textBox1.Text = ((int)CharacterCode).ToString(FontShared.X.CultureEN);
		}

		private void button1_Click(object sender, EventArgs e)
		{
			if (OnDelete != null)
				OnDelete(this, CustomChar);
		}

		private void button2_Click(object sender, EventArgs e)
		{
			openFileDialog1.FileName = FileName;

			if (openFileDialog1.ShowDialog() != DialogResult.OK)
				return;

			FileName = openFileDialog1.FileName;
		}

		private void pictureBox1_DoubleClick(object sender, EventArgs e)
		{
			button2_Click(sender, e);
		}

		private void textBox2_Leave(object sender, EventArgs e)
		{
			textBox2.Text = ScalePct.ToString(FontShared.X.CultureEN);
		}

		private void textBox3_Leave(object sender, EventArgs e)
		{
			textBox3.Text = OffsetPct.ToString(FontShared.X.CultureEN);
		}

		private void textBox2_TextChanged(object sender, EventArgs e)
		{
			int scale;

			if (!int.TryParse(textBox2.Text, System.Globalization.NumberStyles.Number, FontShared.X.CultureEN, out scale))
				return;

			ScalePct = scale;
		}

		private void textBox3_TextChanged(object sender, EventArgs e)
		{
			int offset;

			if (!int.TryParse(textBox3.Text, System.Globalization.NumberStyles.Number, FontShared.X.CultureEN, out offset))
				return;

			OffsetPct = offset;
		}
	}
}

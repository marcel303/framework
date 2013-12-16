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
	public partial class ColorButton : UserControl
	{
		public event EventHandler OnChange;

		public ColorButton()
		{
			InitializeComponent();
		}

		public VecColor Color
		{
			get
			{
				return m_Color;
			}
			set
			{
				m_Color = value;

				if (OnChange != null)
				{
					OnChange(this, null);
				}

				Invalidate();
			}
		}

		private VecColor m_Color = new VecColor();

		private void DoDialog()
		{
			colorDialog1.Color = System.Drawing.Color.FromArgb(Color.R, Color.G, Color.B);

			if (colorDialog1.ShowDialog() == DialogResult.OK)
			{
				Color color = colorDialog1.Color;

				Color = new VecColor(color.R, color.G, color.B);
			}
		}

		private void ColorButton_Click(object sender, EventArgs e)
		{
			DoDialog();
		}

		private void ColorButton_Paint(object sender, PaintEventArgs e)
		{
			Graphics g = e.Graphics;

			g.Clear(System.Drawing.Color.FromArgb(Color.R, Color.G, Color.B));

			if (Focused)
			{
				Pen pen = new Pen(new System.Drawing.SolidBrush(System.Drawing.Color.Black));
				pen.DashStyle = System.Drawing.Drawing2D.DashStyle.Dot;

				g.DrawRectangle(pen, 0.0f, 0.0f, ClientSize.Width - 1.0f, ClientSize.Height - 1.0f);
			}
		}

		private void ColorButton_Enter(object sender, EventArgs e)
		{
			Invalidate();
		}

		private void ColorButton_Leave(object sender, EventArgs e)
		{
			Invalidate();
		}

		private void ColorButton_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
			{
				DoDialog();
			}
		}
	}
}

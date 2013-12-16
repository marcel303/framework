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
	public partial class Form_Preview : Form
	{
		private Composition m_Composition;
		private int m_MaxLevel;
		private Dictionary<int, bool> m_HasLevel = new Dictionary<int, bool>();
		private int m_ViewWidth = 320 * 3;
		private int m_ViewHeight = 240 * 2;

		public Composition Composition
		{
			set
			{
				m_Composition = value;

				CreatePreview();

				Invalidate();
			}
		}

		public Form_Preview()
		{
			InitializeComponent();
		}

		private void CreatePreview()
		{
			AnalyzeLevels(m_Composition.m_Links);

			int width = m_ViewWidth;
			int height = m_ViewHeight;
			int padding = 5;

			int y = 0;

			for (int i = 0; i <= m_MaxLevel; ++i)
			{
				if (!m_HasLevel.ContainsKey(i))
					continue;

				CompositionView view = new CompositionView();

				view.m_Composition = m_Composition;
				view.Level = i;

				view.Size = new Size(width, height);
				view.Location = new Point(padding, (height + padding) * y);

				panel1.Controls.Add(view);
				
				y++;
			}
		}

		private void AnalyzeLevels(IList<Link> links)
		{
			int maxLevel = 0;

			foreach (Link link in links)
			{
				if (link.MinLevel > maxLevel)
					maxLevel = link.MinLevel;

				m_HasLevel[link.MinLevel] = true;
			}

			m_MaxLevel = maxLevel;
		}

		private void printDocument1_PrintPage(object sender, System.Drawing.Printing.PrintPageEventArgs e)
		{
			PaintEventArgs args = new PaintEventArgs(e.Graphics, panel1.ClientRectangle);

			int y = 0;

			float height = m_ViewHeight * panel1.Controls.Count;

			float scale = e.PageBounds.Height / height;

			foreach (Control control in panel1.Controls)
			{
				CompositionView view = (CompositionView)control;

				System.Drawing.Drawing2D.Matrix transform = new System.Drawing.Drawing2D.Matrix();
				transform.Scale(scale, scale);
				transform.Translate(0.0f, y);

				e.Graphics.Transform = transform;

				view.DoPaint(args);

				y += view.Height;
			}
		}

		private void button1_Click(object sender, EventArgs e)
		{
			printPreviewDialog1.ShowDialog();
		}

		private void Form_Preview_KeyPress(object sender, KeyPressEventArgs e)
		{
			int index = e.KeyChar - '0';

			if (index >= 0 && index <= 9)
			{
				if (index < panel1.Controls.Count)
				{
					Control control = panel1.Controls[index];

					panel1.ScrollControlIntoView(control);

					e.Handled = true;
				}
			}
		}
	}
}

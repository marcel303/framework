using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using FontRendering;

namespace FontGen
{
	public partial class Form_Custom : Form
	{
		private IList<CustomChar> mCustomCharList = new List<CustomChar>();

		public Form_Custom(IList<CustomChar> customCharList)
		{
			InitializeComponent();

			CustomCharList = customCharList;
		}

		public IList<CustomChar> CustomCharList
		{
			get
			{
				return mCustomCharList;
			}
			set
			{
				mCustomCharList = value;

				Rebuild();
			}
		}

		void Rebuild()
		{
			int y = 0;

			List<Control> controlList = new List<Control>();

			foreach (CustomChar customChar in CustomCharList)
			{
				UserControl_CustomItem item = new UserControl_CustomItem(customChar);

				item.Size = new Size(panel1.Width, item.Height);
				item.Location = new Point(0, y);
				item.OnDelete += HandleDelete;

				controlList.Add(item);

				y += item.Height;
			}

			panel1.Visible = false;
			panel1.Controls.Clear();
			panel1.Controls.AddRange(controlList.ToArray());
			panel1.Visible = true;
		}

		private void HandleDelete(UserControl_CustomItem sender, CustomChar customChar)
		{
			mCustomCharList.Remove(customChar);

			Rebuild();
		}

		private void toolStripButton1_Click(object sender, EventArgs e)
		{
			if (openFileDialog1.ShowDialog() != DialogResult.OK)
				return;

			CustomChar customChar = new CustomChar((char)0, openFileDialog1.FileName, 100, 0);

			mCustomCharList.Add(customChar);

			Rebuild();
		}
	}
}

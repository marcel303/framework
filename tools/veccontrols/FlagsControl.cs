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
	public partial class FlagsControl : UserControl
	{
		public event EventHandler ValueChanged;

		public FlagsControl()
		{
			InitializeComponent();
		}

		private void HandleCheckedChanged(object sender, EventArgs e)
		{
			ToolStripMenuItem item = (ToolStripMenuItem)sender;

			item.Checked = !item.Checked;

			RebuildValue();
		}

		private void RebuildValue()
		{
			int value = 0;

			foreach (ToolStripMenuItem item in contextMenuStrip1.Items)
			{
				if (!item.Checked)
					continue;

				int temp = (int)item.Tag;

				value |= temp;
			}

			Value = value;
		}

		private void RebuildEnum()
		{
			contextMenuStrip1.Items.Clear();

			if (Type != null)
			{
				String[] names = Enum.GetNames(Type);

				Array.Sort(names);

				foreach (String name in names)
				{
					ToolStripMenuItem item = (ToolStripMenuItem)contextMenuStrip1.Items.Add(name);

					item.Click += HandleCheckedChanged;
					int value = (int)Enum.Parse(Type, name);

					item.Checked = (Value & value) != 0;
					item.Tag = value;
				}
			}
		}

		public Type Type
		{
			get
			{
				return m_Type;
			}
			set
			{
				m_Type = value;

				RebuildEnum();
			}
		}
		public Int32 Value
		{
			get
			{
				return m_Value;
			}
			set
			{
				m_Value = value;

				RebuildEnum();

				if (ValueChanged != null)
					ValueChanged(this, null);
			}
		}

		private Type m_Type;
		private int m_Value = 0;

		private void button1_Click(object sender, EventArgs e)
		{
			contextMenuStrip1.Show(Cursor.Position);
		}
	}
}

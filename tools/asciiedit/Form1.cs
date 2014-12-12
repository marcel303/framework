using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace ASCIIedit
{
	public partial class Form1 : Form
	{
		private string m_filename = null;

		class FilenameItem
		{
			public string m_filename;

			public override string ToString()
			{
				return Path.GetFileName(m_filename);
			}
		}

		public Form1()
		{
			InitializeComponent();
		}

		private new void Load(string filename)
		{
			textGrid1.Load(filename);

			m_filename = filename;

			toolStripStatusLabel1.Text = string.Format("{0}: Loaded {1}", DateTime.Now, filename);
		}

		private void Save(string filename)
		{
			textGrid1.Save(filename);

			m_filename = filename;

			toolStripStatusLabel1.Text = string.Format("{0}: Saved {1}", DateTime.Now, filename);
		}

		#region Dialogs

		private void saveToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if (m_filename == null)
			{
				saveAsToolStripMenuItem_Click(sender, e);
			}
			else
			{
				Save(m_filename);
			}
		}

		private void saveAsToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if (m_filename != null)
			{
				saveFileDialog1.FileName = m_filename;
			}

			if (saveFileDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
			{
				Save(saveFileDialog1.FileName);
			}
		}

		private void loadToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if (m_filename != null)
			{
				openFileDialog1.FileName = m_filename;
			}

			if (openFileDialog1.ShowDialog() == System.Windows.Forms.DialogResult.OK)
			{
				listBox1.Items.Clear();

				foreach (string filename in openFileDialog1.FileNames)
				{
					FilenameItem item = new FilenameItem();
					item.m_filename = filename;

					listBox1.Items.Add(item);
				}

				Load(openFileDialog1.FileNames[0]);
			}
		}

		#endregion

		private void newToolStripMenuItem_Click(object sender, EventArgs e)
		{
			m_filename = null;

			textGrid1.Clear();

			toolStripStatusLabel1.Text = string.Format("{0}: New", DateTime.Now);
		}

		private void sizeToolStripMenuItem_Click(object sender, EventArgs e)
		{
			using (Form_SetSize setSize = new Form_SetSize())
			{
				setSize.SX = textGrid1.SX;
				setSize.SY = textGrid1.SY;

				if (setSize.ShowDialog() == System.Windows.Forms.DialogResult.OK)
				{
					textGrid1.SetSize(setSize.SX, setSize.SY);
				}
			}
		}

		private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
		{
			FilenameItem item = listBox1.SelectedItem as FilenameItem;

			if (item != null)
			{
				if (m_filename != null)
					Save(m_filename);

				Load(item.m_filename);
			}
		}
	}
}

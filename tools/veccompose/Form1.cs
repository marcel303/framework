using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using vecdraw;

namespace veccompose
{
	public partial class Form1 : Form
	{
		private ShapeLibrary m_Library = new ShapeLibrary();
		private FileSystemWatcher m_FileWatcher;
		private delegate void CurrentFileChangedHandler();

		public Form1(String[] args)
		{
			InitializeComponent();

			compositionView1.PropertyEditor = propertyEditor1;

			m_Library = compositionView1.m_Composition.m_ShapeLibrary;

			m_Library.OnItemAdded += HandleLibraryItemAdded;
			m_Library.OnItemRemoved += HandleLibraryItemRemoved;

			if (args.Length > 0)
			{
				try
				{
					Directory.SetCurrentDirectory(Path.GetDirectoryName(Path.GetFullPath(args[0])));

					LoadVCC(args[0]);
				}
				catch (Exception e)
				{
					HandleException(e);

					MessageBox.Show("The application will now terminate.");

					Application.Exit();
				}
			}
		}

		private void WatchBegin()
		{
			if (m_CurrentFile != String.Empty)
			{
				m_FileWatcher = new FileSystemWatcher(
					Path.GetDirectoryName(m_CurrentFile),
					Path.GetFileName(m_CurrentFile));
				m_FileWatcher.NotifyFilter = NotifyFilters.LastWrite;
				m_FileWatcher.Changed += HandleCurrentFileChanged_DLG;
				m_FileWatcher.EnableRaisingEvents = true;
			}
		}

		private void WatchEnd()
		{
			if (m_FileWatcher != null)
			{
				m_FileWatcher.EnableRaisingEvents = false;
				//m_FileWatcher.Changed -= HandleCurrentFileChanged_DLG;
				//m_FileWatcher.Dispose();
				m_FileWatcher = null;
			}
		}

		private String m_CurrentFile = String.Empty;

		private String CurrentFile
		{
			get
			{
				return m_CurrentFile;
			}
			set
			{
				if (value == m_CurrentFile)
					return;

				if (value == null)
					value = String.Empty;

				m_CurrentFile = value;

				toolStripButton4.Enabled = m_CurrentFile != String.Empty;

				String title = "VecCompose";

				if (m_CurrentFile != String.Empty)
					title += " - " + m_CurrentFile;

				Text = title;
			}
		}

		private void HandleCurrentFileChanged_DLG(object sender, FileSystemEventArgs e)
		{
			Invoke(new CurrentFileChangedHandler(HandleCurrentFileChanged));
		}

		private void HandleCurrentFileChanged()
		{
			if (MessageBox.Show("File has been changed outside of the editor. Reload?", "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
				return;

			LoadVCC(m_CurrentFile);
		}

		private void compositionView1_OnFocusChanged(Link link)
		{
			propertyEditor1.Editable = link;

			tabControl1.SelectedTab = tabPage1;
		}

		private void toolStripButton1_Click(object sender, EventArgs args)
		{
			try
			{
				using (Form_LinkAdd form = new Form_LinkAdd(m_Library))
				{
					if (compositionView1.FocusLink == null)
					{
						throw new Exception("Please select a root link for the link to be created newly.");
					}

					if (trackBar1.Value >= 0)
						form.LinkMinLevel = trackBar1.Value;

					if (form.ShowDialog() != DialogResult.OK)
						return;

					Link link = new Link(compositionView1.m_Composition, compositionView1.FocusLink.Name);

					link.Name = form.LinkName;
					link.Location = form.LinkLocation;
					link.ShapeName = form.LinkShape;
					link.BaseAngle.Degrees = form.LinkBaseAngle;
					link.MinAngle.Degrees = form.LinkMinAngle;
					link.MaxAngle.Degrees = form.LinkMaxAngle;
					link.MinLevel = form.LinkMinLevel;
					link.MaxLevel = form.LinkMaxLevel;

					compositionView1.FocusLink.AddChild(link);

					compositionView1.FocusLink = link;
				}
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void RebuildShapeLibraryList()
		{
			try
			{
				listView1.Items.Clear();

				foreach (ShapeLibraryItem item in m_Library.m_Items)
				{
					ListViewItem temp = new ListViewItem(item.m_Name);

					temp.Tag = item;

					temp.SubItems.Add(item.m_Group);
					temp.SubItems.Add(item.m_Pivot);

					listView1.Items.Add(temp);
				}
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void HandleLibraryItemAdded(ShapeLibraryItem item)
		{
			try
			{
				RebuildShapeLibraryList();
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void HandleLibraryItemRemoved(ShapeLibraryItem item)
		{
			try
			{
				RebuildShapeLibraryList();
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void LoadVCC(String fileName)
		{
			try
			{
				WatchEnd();

                string path = Path.GetDirectoryName(fileName);

                Directory.SetCurrentDirectory(path);

				compositionView1.m_Composition.Load(fileName);

				CurrentFile = fileName;

				WatchBegin();
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void SaveVCC(String fileName)
		{
			try
			{
				WatchEnd();

                string path = Path.GetDirectoryName(fileName);

                Directory.SetCurrentDirectory(path);

				compositionView1.m_Composition.Save(fileName);

				CurrentFile = fileName;

				WatchBegin();
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void toolStripButton3_Click(object sender, EventArgs args)
		{
			if (openFileDialog1.ShowDialog() != DialogResult.OK)
				return;

			String fileName = openFileDialog1.FileName;

			LoadVCC(fileName);
		}

		private void Menu_Save()
		{
			if (saveFileDialog1.ShowDialog() != DialogResult.OK)
				return;

			String fileName = saveFileDialog1.FileName;

			SaveVCC(fileName);
		}

		private void toolStripButton4_Click(object sender, EventArgs args)
		{
			try
			{
				if (CurrentFile == null)
					throw new Exception("CurrentFile is null");

				SaveVCC(CurrentFile);
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void addToolStripMenuItem_Click(object sender, EventArgs args)
		{
			try
			{
				using (Form_ItemAdd form = new Form_ItemAdd())
				{
					if (form.ShowDialog() != DialogResult.OK)
						return;

					if (form.ItemFileName == String.Empty)
						throw new Exception("Path must be set.");
					if (form.ItemName == String.Empty)
						throw new Exception("Name must be set.");

					ShapeLibraryItem item = new ShapeLibraryItem(form.ItemFileName, form.ItemName, form.ItemGroup, form.ItemPivot);

					item.Load();

					compositionView1.m_Composition.m_ShapeLibrary.AddItem(item);

					compositionView1.Invalidate();
				}
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void editToolStripMenuItem_Click(object sender, EventArgs args)
		{
			try
			{
				if (listView1.SelectedItems.Count == 0)
					return;

				ListViewItem temp = listView1.SelectedItems[0];

				ShapeLibraryItem item = temp.Tag as ShapeLibraryItem;

				using (Form_ItemAdd form = new Form_ItemAdd())
				{
					form.ItemName = item.m_Name;
					form.ItemFileName = item.m_FileName;
					form.ItemGroup = item.m_Group;
					form.ItemPivot = item.m_Pivot;

					if (form.ShowDialog() != DialogResult.OK)
						return;

					if (form.ItemFileName == String.Empty)
						throw new Exception("Path must be set.");
					if (form.ItemName == String.Empty)
						throw new Exception("Name must be set.");

					item.m_Name = form.ItemName;
					item.m_FileName = form.ItemFileName;
					item.m_Group = form.ItemGroup;
					item.m_Pivot = form.ItemPivot;

					item.Load();
				}
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void removeToolStripMenuItem_Click(object sender, EventArgs e)
		{
			if (listView1.SelectedItems.Count == 0)
				return;

			ListViewItem temp = listView1.SelectedItems[0];

			ShapeLibraryItem item = temp.Tag as ShapeLibraryItem;

			compositionView1.m_Composition.m_ShapeLibrary.RemoveItem(item);

			compositionView1.Invalidate();
		}

		private void toolStripButton5_Click(object sender, EventArgs args)
		{
			try
			{
				foreach (ShapeLibraryItem item in compositionView1.m_Composition.m_ShapeLibrary.m_Items)
				{
					item.Load();
				}

				// todo: let shape lib item generate visual changed evt

				compositionView1.Invalidate();
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void HandleException(Exception e)
		{
			MessageBox.Show("Operation failed:\n\n" + e.Message + "\n\n" + e.StackTrace, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}

		private void toolStripButton2_Click(object sender, EventArgs e)
		{
			DialogResult retval = MessageBox.Show("Save before creating new graphic?", "New", MessageBoxButtons.YesNoCancel);

			if (retval == DialogResult.Cancel)
				return;
			if (retval == DialogResult.Yes)
			{
				if (CurrentFile != null)
					SaveVCC(CurrentFile);
				else
					Menu_Save();
			}

			compositionView1.m_Composition.New();

			CurrentFile = null;
		}

		private void toolStripButton6_Click(object sender, EventArgs e)
		{
			Menu_Save();
		}

		private void toolStripButton7_Click(object sender, EventArgs e)
		{
			Application.Exit();
		}

		private void toolStripButton8_Click(object sender, EventArgs e)
		{
			using (Form_Help form = new Form_Help())
			{
				form.ShowDialog();
			}
		}

		private void compositionView1_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Delete)
			{
				// delete link

				if (compositionView1.FocusLink == null)
					return;

				List<Link> links = new List<Link>();

				Stack<Link> stack = new Stack<Link>();

				stack.Push(compositionView1.FocusLink);

				while (stack.Count > 0)
				{
					Link link = stack.Pop();

					links.Add(link);

					foreach (Link child in link.Children)
						stack.Push(child);
				}

				if (MessageBox.Show(String.Format("Warning: This action will remove 1 link and {0} child links.\n\nContinue?", links.Count - 1), "Delete", MessageBoxButtons.YesNo, MessageBoxIcon.Warning) != DialogResult.Yes)
					return;

				for (int i = links.Count - 1; i >= 0; --i)
				{
					compositionView1.m_Composition.RemoveLink(links[i]);
				}
			}
		}

		private void toolStripButton9_Click(object sender, EventArgs e)
		{
			using (Form_Preview form = new Form_Preview())
			{
				form.Composition = compositionView1.m_Composition;

				form.ShowDialog();
			}
		}

		private void trackBar1_ValueChanged(object sender, EventArgs e)
		{
			if (trackBar1.Value < 0)
			{
				label1.Text = "(all levels)";
				compositionView1.Level = null;
			}
			else
			{
				label1.Text = String.Format("level {0}", trackBar1.Value);
				compositionView1.Level = trackBar1.Value;
			}
		}

        private void editingToolStripMenuItem_Click(object sender, EventArgs e)
        {
            compositionView1.RenderMode = RenderMode.Editing;
        }

        private void preciseEditingToolStripMenuItem_Click(object sender, EventArgs e)
        {
            compositionView1.RenderMode = RenderMode.PreciseEditing;
        }

        private void previewToolStripMenuItem_Click(object sender, EventArgs e)
        {
            compositionView1.RenderMode = RenderMode.Preview;
        }

        private void zOrderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            compositionView1.RenderMode = RenderMode.ZOrder;
        }

        private void printToolStripMenuItem_Click(object sender, EventArgs e)
        {
            compositionView1.RenderMode = RenderMode.Clean;
        }

        private void toolStripButton10_Click(object sender, EventArgs e)
        {
            if (openFileDialog2.ShowDialog() != DialogResult.OK)
                return;

            string fileName = openFileDialog2.FileName;

            compositionView1.BackgroundImage = Image.FromFile(fileName);
        }

        private void toolStripComboBox2_TextChanged(object sender, EventArgs e)
        {
            string text = toolStripComboBox2.Text;

            float scale;

            if (!float.TryParse(text, out scale))
                return;

            compositionView1.Scale = scale / 100.0f;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (compositionView1.FocusLink == null)
                return;

            compositionView1.m_Composition.MoveUp(compositionView1.FocusLink);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (compositionView1.FocusLink == null)
                return;

            compositionView1.m_Composition.MoveDown(compositionView1.FocusLink);
        }
	}
}

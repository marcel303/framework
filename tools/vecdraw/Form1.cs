using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Globalization;
using System.Threading;
using vecdraw.Elements;

namespace vecdraw
{
	public partial class Form1 : Form
	{
		private RegistryIO m_Registry = new RegistryIO(new BasicRegistry(@"GG\vecdraw"));
		private MRU m_MRU = new MRU(@"GG\vecdraw\MRU");

		public Form1(String[] args)
		{
			InitializeComponent();

			Thread.CurrentThread.CurrentCulture = Global.CultureEN;

			vectorView1.OnFocusChange += HandleFocusChange;
			vectorView1.OnEditingFocusChange += HandleEditingFocusChange;
			vectorView1.m_Shape.OnAdd += HandleAdd;
			vectorView1.m_Shape.OnRemove += HandleRemove;
			vectorView1.m_Shape.OnChange += HandleChange;
			vectorView1.m_Shape.OnPropertyChange += HandlePropertyChange;
			vectorView1.m_Shape.OnDepthChange += HandleDepthChange;

			if (args.Length > 0)
			{
				try
				{
					LoadVG(args[0]);
				}
				catch (Exception e)
				{
					HandleException(e);

					MessageBox.Show("The application will now terminate.");

					Application.Exit();
				}
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
				if (value == null)
					value = String.Empty;

				m_CurrentFile = value;

				toolStripButton9.Enabled = m_CurrentFile != String.Empty;

				String title = "VecDraw";

				if (m_CurrentFile != String.Empty)
					title += " - " + m_CurrentFile;

				Text = title;
			}
		}

		private void RebuildElementList(VectorElement focus)
		{
			listView1.Items.Clear();

			ListViewItem focusItem = null;

			foreach (VectorElement temp in vectorView1.m_Shape.m_Elements)
			{
				X.Assert(temp != null);

				ListViewItem item = new ListViewItem();

				item.SubItems.Add(temp.Name);

				item.Tag = temp;

				listView1.Items.Add(item);

				if (temp == focus)
					focusItem = item;
			}

			if (focusItem != null)
			{
				focusItem.Selected = true;
			}
		}

		private void HandleAdd(VectorElement element)
		{
			X.Assert(element != null);

			RebuildElementList(vectorView1.FocusElement);
		}

		private void HandleRemove(VectorElement element)
		{
			X.Assert(element != null);

			RebuildElementList(vectorView1.FocusElement);
		}

		private void HandleChange(VectorElement element)
		{
			X.Assert(element != null);

			if (element == vectorView1.FocusElement)
			{
				propertyEditor1.UpdateValues();
			}
		}

		private void HandlePropertyChange(VectorElement element, DataInfo di)
		{
			X.Assert(element != null);
			X.Assert(di != null);

			if (element == vectorView1.FocusElement)
			{
				propertyEditor1.UpdateValues();
			}

			if (di.Name == "Name")
			{
				RebuildElementList(vectorView1.FocusElement);
			}
			if (di.Name == "Collision" || di.Name == "Visible")
			{
				listView1.Invalidate();
			}
		}

		private void HandleDepthChange(VectorElement element)
		{
			X.Assert(element != null);

			RebuildElementList(vectorView1.FocusElement);
		}

		private void HandleFocusChange(VectorElement element)
		{
			//X.Assert(element != null);

			//propertyEditor1.Editable = element;
		}

		private void HandleEditingFocusChange(IEditable property)
		{
			//X.Assert(property != null);

			propertyEditor1.Editable = property;
		}

		private void listView1_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (listView1.SelectedItems.Count == 0)
				return;

			ListViewItem item = listView1.SelectedItems[0];

			VectorElement element = item.Tag as VectorElement;

			X.Assert(element != null);

			vectorView1.FocusElement = element;
			vectorView1.EditingFocus = element;
		}

		private void LoadVG(String fileName)
		{
			vectorView1.m_Shape.Load(fileName);

			m_MRU.Load();
			m_MRU.Add(fileName);
			m_MRU.Save();

			CurrentFile = fileName;
		}

		// todo: move to Shape class

		private void SaveVG(String fileName, bool updateMRU)
		{
			vectorView1.m_Shape.Save(fileName);

			if (updateMRU)
			{
				m_MRU.Load();
				m_MRU.Add(fileName);
				m_MRU.Save();

				CurrentFile = fileName;
			}
		}

		private void toolStripButton1_Click(object sender, EventArgs args)
		{
			try
			{
				if (openFileDialog1.ShowDialog() == DialogResult.OK)
				{
					String fileName = openFileDialog1.FileName;

					LoadVG(fileName);
				}
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void Menu_SaveVG()
		{
			if (saveFileDialog1.ShowDialog() == DialogResult.OK)
			{
				String fileName = saveFileDialog1.FileName;

				SaveVG(fileName, true);
			}
		}

		private void toolStripButton2_Click(object sender, EventArgs args)
		{
			try
			{
				Menu_SaveVG();
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void toolStripButton8_Click(object sender, EventArgs e)
		{
			Application.Exit();
		}

		private void toolStripDropDownButton1_DropDownOpening(object sender, EventArgs e)
		{
			toolStripDropDownButton1.DropDown.Items.Clear();

			m_MRU.Load();

			m_MRU.Sanitize();

			for (int i = 0; i < m_MRU.Files.Length; ++i)
			{
				String fileName = m_MRU.Files[i];

				ToolStripItem item = toolStripDropDownButton1.DropDown.Items.Add(Path.GetFileName(fileName));

				item.Tag = fileName;

				item.Click += HandleMRUClick;
			}
		}

		private void HandleMRUClick(object sender, EventArgs e)
		{
			ToolStripItem item = (ToolStripItem)sender;

			String fileName = (String)item.Tag;

			LoadVG(fileName);
		}

		private void toolStripButton3_Click(object sender, EventArgs e)
		{
			DialogResult retval = MessageBox.Show("Save before creating new graphic?", "New", MessageBoxButtons.YesNoCancel);

			if (retval == DialogResult.Cancel)
				return;
			if (retval == DialogResult.Yes)
			{
				if (CurrentFile == null)
					Menu_SaveVG();
				else
					SaveVG(CurrentFile, true);
			}

			CurrentFile = null;

			vectorView1.m_Shape.ClearElements();
		}

		private void toolStripButton4_Click(object sender, EventArgs args)
		{
			try
			{
				VectorPoly element = new VectorPoly();

				Point center = vectorView1.MouseViewLocation();

				element.AddPoint(new PolyPoint(element, center.X + 0, center.Y + 0));
				element.AddPoint(new PolyPoint(element, center.X + 20, center.Y + 0));
				element.AddPoint(new PolyPoint(element, center.X + 0, center.Y + 20));

				vectorView1.m_Shape.AddElement(element);

				vectorView1.FocusElement = element;
				vectorView1.EditingFocus = element;
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void toolStripButton5_Click(object sender, EventArgs args)
		{
			try
			{
				VectorCircle element = new VectorCircle();

				Point center = vectorView1.MouseViewLocation();

				element.m_Location = center;
				element.m_Radius = 9;

				vectorView1.m_Shape.AddElement(element);

				vectorView1.FocusElement = element;
				vectorView1.EditingFocus = element;
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void toolStripButton9_Click(object sender, EventArgs args)
		{
			try
			{
				if (CurrentFile == null)
					throw new Exception("CurrentFile is null");

				SaveVG(CurrentFile, true);
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void button1_Click(object sender, EventArgs e)
		{
			if (vectorView1.FocusElement != null)
			{
				vectorView1.m_Shape.MoveUp(vectorView1.FocusElement);
			}
		}

		private void button2_Click(object sender, EventArgs e)
		{
			if (vectorView1.FocusElement != null)
			{
				vectorView1.m_Shape.MoveDown(vectorView1.FocusElement);
			}
		}

		private void trackBar1_ValueChanged(object sender, EventArgs e)
		{
			vectorView1.Zoom = trackBar1.Value;

			label1.Text = "Zoom x" + vectorView1.Zoom;
		}

		private void toolStripButton6_Click(object sender, EventArgs args)
		{
			try
			{
				VectorTag element = new VectorTag();

				Point center = vectorView1.MouseViewLocation();

				element.m_Location = center;

				vectorView1.m_Shape.AddElement(element);

				vectorView1.FocusElement = element;
				vectorView1.EditingFocus = element;
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

        private void toolStripButton15_Click(object sender, EventArgs args)
        {
            try
            {
                VectorRect element = new VectorRect();

                Point center = vectorView1.MouseViewLocation();

                element.m_Location = center;
                element.m_Size = new Point(10, 10);

                vectorView1.m_Shape.AddElement(element);

                vectorView1.FocusElement = element;
                vectorView1.EditingFocus = element;
            }
            catch (Exception e)
            {
                HandleException(e);
            }
        }

		public String PreviewExecutable
		{
			get
			{
				return m_Registry.Get("PreviewExecutable", String.Empty);
			}
			set
			{
				m_Registry.Set("PreviewExecutable", value);
			}
		}

		private void toolStripButton10_Click(object sender, EventArgs args)
		{
			String previewExecutable = PreviewExecutable;

			if (previewExecutable == String.Empty)
			{
				if (openFileDialog2.ShowDialog() != DialogResult.OK)
					return;

				previewExecutable = openFileDialog2.FileName;
			}

			PreviewExecutable = previewExecutable;

			try
			{
				String temp = Path.GetTempFileName();

				SaveVG(temp, false);

				ProcessStartInfo psi = new ProcessStartInfo(previewExecutable, String.Format("\"{0}\"", temp));
				Process process = new Process();
				process.StartInfo = psi;
				process.Start();
			}
			catch (Exception e)
			{
				MessageBox.Show(String.Format("Failed to preview graphic: {0}", e.Message));
			}
		}

		private void toolStripButton11_Click(object sender, EventArgs e)
		{
			using (Form_Help form = new Form_Help())
			{
				form.ShowDialog();
			}
		}

		private void toolStripButton12_Click(object sender, EventArgs e)
		{
			using (Form_Attributes form = new Form_Attributes(this, vectorView1.m_Shape.m_Attributes))
			{
				form.ShowDialog();

				vectorView1.Invalidate();
			}
		}

		private void HandleException(Exception e)
		{
			MessageBox.Show("Operation failed:\n\n" + e.Message + "\n\n" + e.StackTrace, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
		}

		private void toolStripButton13_Click(object sender, EventArgs e)
		{
			using (Form_Transform form = new Form_Transform())
			{
				if (form.ShowDialog() != DialogResult.OK)
					return;

				TransformType type = form.TransformType;

				vectorView1.Transform(type, form.SelectionOnly);
			}
		}

		private void toolStripButton14_Click(object sender, EventArgs args)
		{
			try
			{
				VectorPicture element = new VectorPicture();

				Point center = vectorView1.MouseViewLocation();

				element.m_Location = center;

				vectorView1.m_Shape.AddElement(element);

				vectorView1.FocusElement = element;
				vectorView1.EditingFocus = element;
			}
			catch (Exception e)
			{
				HandleException(e);
			}
		}

		private void listView1_DrawItem(object sender, DrawListViewItemEventArgs e)
		{
			VectorElement element = e.Item.Tag as VectorElement;

			Point p = e.Bounds.Location;

			if (element.Collision)
			{
				e.Graphics.DrawImage(Resources.icon_collision, new Rectangle(p, Resources.icon_collision.Size));
				p.X += 18;
			}

			if (element.Visible)
			{
				e.Graphics.DrawImage(Resources.icon_visible, new Rectangle(p, Resources.icon_visible.Size));
				p.X += 18;
			}
		}

		private void listView1_DrawSubItem(object sender, DrawListViewSubItemEventArgs e)
		{
			if (e.ColumnIndex != 0)
				e.DrawDefault = true;
		}

		private void listView1_DrawColumnHeader(object sender, DrawListViewColumnHeaderEventArgs e)
		{
			e.DrawDefault = true;
		}

		private void contextMenuStrip1_Opening(object sender, CancelEventArgs e)
		{
			ListViewItem item = null;

			if (listView1.SelectedItems.Count > 0)
				item = listView1.SelectedItems[0];

			if (item == null)
			{
				contextMenuStrip1.Enabled = false;
			}
			else
			{
				contextMenuStrip1.Enabled = true;
				contextMenuStrip1.Tag = item.Tag;

				VectorElement element = item.Tag as VectorElement;

				visibleToolStripMenuItem.Checked = element.Visible;
				filledToolStripMenuItem.Checked = element.Filled;
				collisionToolStripMenuItem.Checked = element.Collision;
			}
		}

		private void visibleToolStripMenuItem_CheckStateChanged(object sender, EventArgs e)
		{
			propertyEditor1.Set("Visible", visibleToolStripMenuItem.Checked);
		}

		private void filledToolStripMenuItem_CheckStateChanged(object sender, EventArgs e)
		{
			propertyEditor1.Set("Filled", filledToolStripMenuItem.Checked);
		}

		private void collisionToolStripMenuItem_CheckStateChanged(object sender, EventArgs e)
		{
			propertyEditor1.Set("Collision", collisionToolStripMenuItem.Checked);
		}

		private void vectorView1_MouseMove(object sender, MouseEventArgs e)
		{
			Point point = vectorView1.MouseViewLocation();

			label3.Text = String.Format("({0}, {1})", point.X, point.Y);
		}

        private void editingToolStripMenuItem_Click(object sender, EventArgs e)
        {
            vectorView1.RenderMode = RenderMode.Editing;
        }

        private void preciseEditingToolStripMenuItem_Click(object sender, EventArgs e)
        {
            vectorView1.RenderMode = RenderMode.PreciseEditing;
        }

        private void previewToolStripMenuItem_Click(object sender, EventArgs e)
        {
            vectorView1.RenderMode = RenderMode.Preview;
        }

        private void zOrderToolStripMenuItem_Click(object sender, EventArgs e)
        {
            vectorView1.RenderMode = RenderMode.ZOrder;
        }

        private void printToolStripMenuItem_Click(object sender, EventArgs e)
        {
            vectorView1.RenderMode = RenderMode.Clean;
        }
	}
}

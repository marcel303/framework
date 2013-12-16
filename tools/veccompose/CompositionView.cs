using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using vecdraw;
using vecdraw.Elements;

namespace veccompose
{
	// shape libray -> attach to link
	// link: root link
	// link: add link
	// pivot: set
	// link: min/max angle preview
	// render: hierachic transform
	// edit: select using transform locations
	// link properties: shape, weapon type (vulcan, missile, beam, ..), behaviour (follow target, random, code controlled..), truster effect yes/no (show blue truster particle effect when boss is moving)

	public partial class CompositionView : UserControl
	{
		public delegate void FocusChangedHandler(Link link);
		public event FocusChangedHandler OnFocusChanged;

        private RenderMode mRenderMode = RenderMode.PreciseEditing;
        private float mScale = 1.0f;

        public RenderMode RenderMode
        {
            get
            {
                return mRenderMode;
            }
            set
            {
                mRenderMode = value;

                Invalidate();
            }
        }
        public float Scale
        {
            get
            {
                return mScale;
            }
            set
            {
                mScale = value;

                Invalidate();
            }
        }
        public bool RenderEditing
        {
            get
            {
                return mRenderMode == RenderMode.Editing || mRenderMode == RenderMode.PreciseEditing;
            }
        }

		public bool AdvancedView = false;
		public bool ClearView = false;

		public PropertyEditor PropertyEditor;

		private int? m_Level = null;

		public int? Level
		{
			get
			{
				return m_Level;
			}
			set
			{
				m_Level = value;

				Invalidate();
			}
		}

		public CompositionView()
		{
			InitializeComponent();

			// create composition

			m_Composition.OnLinkAdded += HandleLinkAdded;
			m_Composition.OnLinkRemoved += HandleLinkRemoved;
            m_Composition.OnVisualChanged += HandleVisualChanged;

			HandleLinkAdded(m_Composition.RootLink);
		}

		private void HandleLinkAdded(Link link)
		{
			link.OnVisualChanged += HandleLinkVisualChange;

			Invalidate();
		}

		private void HandleLinkRemoved(Link link)
		{
			link.OnVisualChanged -= HandleLinkVisualChange;

			Invalidate();
		}

        private void HandleVisualChanged(Composition composition)
        {
            Invalidate();
        }

        public void RenderLink_Self(Link link, Graphics g, bool isFocused)
        {
            if (ClearView)
                return;

            Brush brush = new SolidBrush(Color.Black);
            Brush brush2 = new SolidBrush(!isFocused ? Color.White : Color.Blue);
            Pen pen = new Pen(brush, 2.0f);

            Brush linkBrush = new SolidBrush(Color.Gray);
            Pen linkPen = new Pen(linkBrush, 2.0f);

            Brush angleBrush = new SolidBrush(Color.Blue);
            Pen anglePen = new Pen(angleBrush, 2.0f);

            Brush angleBrush2 = new SolidBrush(Color.Lime);
            Pen anglePen2 = new Pen(angleBrush2, 2.0f);

            if (RenderEditing)
            {
                g.FillEllipse(brush2, link.m_EditingLocation.X - link.HitRadius, link.m_EditingLocation.Y - link.HitRadius, link.HitRadius * 2, link.HitRadius * 2);
                g.DrawEllipse(pen, link.m_EditingLocation.X - link.HitRadius, link.m_EditingLocation.Y - link.HitRadius, link.HitRadius * 2, link.HitRadius * 2);

                if (link.Parent != null)
                {
                    g.DrawLine(linkPen, new PointF(link.m_EditingLocation.X, link.m_EditingLocation.Y), new PointF(link.Parent.m_EditingLocation.X, link.Parent.m_EditingLocation.Y));
                }
            }

            float dx = 10.0f * (float)Math.Cos((link.m_GlobalAngle + link.Angle.Degrees * 0) / 180.0 * Math.PI);
            float dy = 10.0f * (float)Math.Sin((link.m_GlobalAngle + link.Angle.Degrees * 0) / 180.0 * Math.PI);

            if (RenderEditing)
            {
                Brush angleBrush3 = new SolidBrush(Color.Black);
                Pen anglePen3 = new Pen(angleBrush3, 4.0f);

                g.DrawLine(anglePen3, new PointF(link.m_EditingLocation.X, link.m_EditingLocation.Y), new PointF(link.m_EditingLocation.X + dx, link.m_EditingLocation.Y + dy));
                g.DrawLine(anglePen2, new PointF(link.m_EditingLocation.X, link.m_EditingLocation.Y), new PointF(link.m_EditingLocation.X + dx, link.m_EditingLocation.Y + dy));
            }

            if (RenderEditing)
            {
                int size = 10;

                g.DrawArc(anglePen, link.m_EditingLocation.X - size, link.m_EditingLocation.Y - size, size * 2, size * 2, link.m_GlobalAngle + link.MinAngle.Degrees, link.MaxAngle.Degrees - link.MinAngle.Degrees);
            }
        }

		public void RenderLink_Advanced(Link link, Graphics g)
		{
			if (ClearView)
				return;

			int flagY = 7;
			Brush flagBrush = new SolidBrush(Color.Black);
			Font flagFont = new Font(FontFamily.GenericSansSerif, 7.0f);

			foreach (LinkFlags flag in Enum.GetValues(typeof(LinkFlags)))
			{
				if ((link.Flags & flag) != 0)
				{
					String name = Enum.GetName(typeof(LinkFlags), flag);

					float x = link.m_EditingLocation.X;
					float y = link.m_EditingLocation.Y + flagY;

					g.DrawString(name, flagFont, flagBrush, new PointF(x, y));

					flagY += 10;
				}
			
			}
			// todo: draw angle ex
		}

        public void RenderLink_Shape(Link link, Graphics g)
        {
            RenderLink_Shape(link, g, link.ShapeName);

            float dx = 10.0f * (float)Math.Cos((link.m_GlobalAngle + link.Angle.Degrees * 0) / 180.0 * Math.PI);
            float dy = 10.0f * (float)Math.Sin((link.m_GlobalAngle + link.Angle.Degrees * 0) / 180.0 * Math.PI);

            if ((link.Flags & LinkFlags.Weapon_Beam) != 0)
            {
                Brush angleBrush_Beam = new SolidBrush(Color.FromArgb(63, 255, 0, 0));
                Pen anglePen_Beam = new Pen(angleBrush_Beam, 6.0f);

                g.DrawLine(
                    anglePen_Beam,
                    new PointF(link.m_EditingLocation.X, link.m_EditingLocation.Y),
                    new PointF(
                        link.m_EditingLocation.X + dx * 100.0f,
                        link.m_EditingLocation.Y + dy * 100.0f));
            }

            string podName = null;

            if ((link.Flags & LinkFlags.Weapon_Beam) != 0)
                podName = "pod_beam";
            if ((link.Flags & LinkFlags.Weapon_BlueSpray) != 0)
                podName = "pod_blue";
            if ((link.Flags & LinkFlags.Weapon_Missile) != 0)
                podName = "pod_missile";
            if ((link.Flags & LinkFlags.Weapon_PurpleSpray) != 0)
                podName = "pod_purple";
            if ((link.Flags & LinkFlags.Weapon_Vulcan) != 0)
                podName = "pod_vulcan";

            if (podName != null)
            {
                RenderLink_Shape(link, g, podName);
            }
        }

		public void RenderLink_Shape(Link link, Graphics g, string shapeName)
		{
			ShapeLibraryItem item = m_Composition.m_ShapeLibrary.FindByName(shapeName);

			if (item != null)
			{
				GraphicsState state = g.Save();

				Matrix temp = new Matrix();
				temp.Reset();
				temp.Multiply(link.m_Matrix);
				if ((link.Flags & LinkFlags.Draw_FlipHorizontally) != 0)
					temp.Scale(-1.0f, 1.0f);
				if ((link.Flags & LinkFlags.Draw_FlipVertically) != 0)
					temp.Scale(1.0f, -1.0f);
				VectorTag tag = null;
				if (item.m_Pivot != String.Empty)
					tag = item.m_Shape.FindElement(item.m_Pivot) as VectorTag;
				else
					tag = item.m_Shape.FindElement("Pivot") as VectorTag;
				if (tag != null)
					temp.Translate(-tag.m_Location.X, -tag.m_Location.Y);

				Matrix temp2 = new Matrix();
				temp2.Reset();
				temp2.Multiply(g.Transform);
				temp2.Multiply(temp);
				g.Transform = temp2;

				item.m_Shape.Render(g, mRenderMode, null);

				g.Restore(state);
			}
		}

		private void HandleLinkVisualChange(Link link)
		{
			Invalidate();
		}

		public PointF ToComposition(PointF location)
		{
			Matrix matrix = CompositionTF;
			matrix.Invert();
			PointF[] temp = new PointF[] { location };
			matrix.TransformPoints(temp);
			return temp[0];
		}

		private Matrix CompositionTF
		{
			get
			{
				Matrix matrix = new Matrix();
				matrix.Reset();
				matrix.Translate(Width / 2, Height / 2);
                matrix.Scale(mScale, mScale);
				return matrix;
			}
		}

		public void DoPaint(PaintEventArgs e)
		{
			OnPaint(e);
		}

		protected override void OnPaint(PaintEventArgs e)
		{
			Graphics g = e.Graphics;

			g.CompositingQuality = CompositingQuality.HighQuality;
			g.InterpolationMode = InterpolationMode.HighQualityBicubic;
			g.SmoothingMode = SmoothingMode.HighQuality;
			g.PixelOffsetMode = PixelOffsetMode.HighQuality;

			m_Composition.RootLink.TransformChildren();

			Matrix transform = g.Transform;

			transform.Multiply(CompositionTF);

			g.Transform = transform;

			if (m_Level.HasValue)
				g.DrawString(String.Format("Level {0}", m_Level), new Font(FontFamily.GenericSansSerif, 7.0f), new SolidBrush(Color.Black), new PointF(-Width / 2.0f, -Height / 2.0f));

			g.DrawRectangle(new Pen(new SolidBrush(Color.FromArgb(63, Color.Black))), new Rectangle(new Point(-160, -120), new Size(320, 240)));

			foreach (Link link in m_Composition.m_Links)
			{
				if (!link.IsVisible(m_Level))
					continue;

				RenderLink_Shape(link, g);
			}

                foreach (Link link in m_Composition.m_Links)
                {
                    if (!link.IsVisible(m_Level))
                        continue;

                    RenderLink_Self(link, g, link == m_FocusLink);
                }

			if (AdvancedView)
			{
				foreach (Link link in m_Composition.m_Links)
				{
					if (!link.IsVisible(m_Level))
						continue;

					RenderLink_Advanced(link, g);
				}
			}
		}

		public Composition m_Composition = new Composition();
		private Link m_FocusLink = null;
		public Link FocusLink
		{
			get
			{
				return m_FocusLink;
			}
			set
			{
				if (value == m_FocusLink)
					return;

				m_FocusLink = value;

				if (OnFocusChanged != null)
					OnFocusChanged(m_FocusLink);

				Invalidate();
			}
		}

		private void CompositionView_MouseClick(object sender, MouseEventArgs e)
		{
			PointF location = new PointF(e.X, e.Y);

			location = ToComposition(location);

			FocusLink = m_Composition.HitTest(location, m_Level, m_FocusLink);
		}

		private void CompositionView_Resize(object sender, EventArgs e)
		{
			//Point center = new Point(ClientSize.Width / 2, ClientSize.Height / 2);

			//m_Composition.RootLink.Location = center;

			Invalidate();
		}

		protected override bool IsInputKey(Keys keyData)
		{
			switch (keyData & Keys.KeyCode)
			{
				case Keys.Up:
					return true;
				case Keys.Down:
					return true;
				case Keys.Right:
					return true;
				case Keys.Left:
					return true;
				default:
					return base.IsInputKey(keyData);
			}
		}

		private void MoveLink(int dx, int dy)
		{
			if (FocusLink == null)
				return;

			Link link = FocusLink;

			link.Location.X += dx;
			link.Location.Y += dy;

			if (PropertyEditor != null)
			{
				PropertyEditor.UpdateValues();
			}

			Invalidate();
		}

		private void CompositionView_KeyDown(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Space)
			{
				e.Handled = true;

				AdvancedView = true;

				Invalidate();
			}
			if (e.KeyCode == Keys.C)
			{
				e.Handled = true;

				ClearView = true;

				Invalidate();
			}

			if (e.KeyCode == Keys.Left)
			{
				e.Handled = true;
				MoveLink(-1, 0);
			}
			if (e.KeyCode == Keys.Right)
			{
				e.Handled = true;
				MoveLink(+1, 0);
			}
			if (e.KeyCode == Keys.Up)
			{
				e.Handled = true;
				MoveLink(0, -1);
			}
			if (e.KeyCode == Keys.Down)
			{
				e.Handled = true;
				MoveLink(0, +1);
			}
		}

		private void CompositionView_KeyUp(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Space)
			{
				e.Handled = true;

				AdvancedView = false;

				Invalidate();
			}
			if (e.KeyCode == Keys.C)
			{
				e.Handled = true;

				ClearView = false;

				Invalidate();
			}
		}

		private void HandleLinkSelect(object sender, EventArgs e)
		{
			ToolStripMenuItem item = (ToolStripMenuItem)sender;

			Link link = (Link)item.Tag;

			FocusLink = link;
		}

		private void CompositionView_MouseDown(object sender, MouseEventArgs e)
		{
			Focus();

			if (e.Button == MouseButtons.Right)
			{
				contextMenuStrip1.Items.Clear();

				m_Composition.m_Links.Sort(delegate(Link link1, Link link2)
				{
					return link1.Name.CompareTo(link2.Name);
				});
				foreach (Link link in m_Composition.m_Links)
				{
					ToolStripMenuItem item = new ToolStripMenuItem(link.Name);

					item.Tag = link;
					item.Click += HandleLinkSelect;

					contextMenuStrip1.Items.Add(item);
				}

				contextMenuStrip1.Show(Cursor.Position);
			}
		}
	}
}

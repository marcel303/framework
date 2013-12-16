using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Reflection;
using vecdraw.Elements;
using System.Drawing.Drawing2D;

// todo: fix vector element transform:
//       create separate methods for moving shapes about and editing sub elements such as vertices

namespace vecdraw
{
	public partial class VectorView : UserControl
	{
        private RenderMode mRenderMode = RenderMode.PreciseEditing;
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

		public delegate void FocusChangeHandler(VectorElement element);
		public event FocusChangeHandler OnFocusChange;
		public delegate void EditingFocusChangeHandler(IEditable propery);
		public event EditingFocusChangeHandler OnEditingFocusChange;
		public delegate void DepthChangedHandler(VectorElement element);
		public event DepthChangedHandler OnDepthChange;

		public Shape m_Shape = new Shape();
		private VectorElement m_FocusElement = null;
		private IEditable m_EditingFocus = null;
		private Point m_MouseLocation = new Point();
		private Point m_MouseLocationControl = new Point();

		private bool m_MouseDown = false;
		private bool m_ShiftDown = false;

		private int m_Zoom = 1;
		private Point m_Pan = new Point(100, 100);

		private List<Ruler> m_Rulers = new List<Ruler>();

		public int Zoom
		{
			get
			{
				return m_Zoom;
			}
			set
			{
				X.Assert(value >= 1);

				m_Zoom = value;

				Invalidate();
			}
		}

		public VectorView()
		{
			InitializeComponent();

			m_Shape.OnAdd += HandleAdd;
			m_Shape.OnRemove += HandleRemove;
			m_Shape.OnChange += HandleChange;
			m_Shape.OnPropertyChange += HandlePropertyChange;
			m_Shape.OnDepthChange += HandleDepthChange;
		}

		public void AddRuler(Ruler ruler)
		{
			m_Rulers.Add(ruler);

			Invalidate();
		}

		public void RemoveRuler(Ruler ruler)
		{
			m_Rulers.Remove(ruler);

			Invalidate();
		}

		public void ClearRulers()
		{
			m_Rulers.Clear();

			Invalidate();
		}

		public Point ClientToView(Point location)
		{
			//return new Point(location.X / Zoom - m_Pan.X, location.Y / Zoom - m_Pan.Y);
			return new Point((int)(location.X / (float)Zoom - m_Pan.X - 0.5f), (int)(location.Y / (float)Zoom - m_Pan.Y - 0.5f));
		}

		public Point ScreenToView(Point location)
		{
			location = PointToClient(location);

			return ClientToView(location);
		}

		public Point MouseViewLocation()
		{
			Point result = ScreenToView(MousePosition);

			if (result.X < 0 || result.Y < 0 || result.X >= Width || result.Y >= Height)
			{
				result = ClientToView(new Point(Width / 2, Height / 2));
			}

			return result;
		}

		private void HandleAdd(VectorElement element)
		{
			X.Assert(element != null);

			Invalidate();
		}

		private void HandleRemove(VectorElement element)
		{
			X.Assert(element != null);

			if (element == FocusElement)
				FocusElement = null;
			if (element == EditingFocus)
				EditingFocus = null;

			Invalidate();
		}

		private void HandleChange(VectorElement element)
		{
			X.Assert(element != null);

			Invalidate();
		}

		private void HandlePropertyChange(VectorElement element, DataInfo di)
		{
			X.Assert(element != null);
			X.Assert(di != null);

			Invalidate();
		}

		private void HandleDepthChange(VectorElement element)
		{
			X.Assert(element != null);

			if (OnDepthChange != null)
				OnDepthChange(element);

			Invalidate();
		}

		public void Transform(TransformType type, bool selectionOnly)
		{
			VectorTag pivot = null;

			if (type == TransformType.CenterPivot ||
				type == TransformType.InvertX ||
				type == TransformType.InvertY ||
				type == TransformType.Rotate180 ||
				type == TransformType.RotateLeft ||
				type == TransformType.RotateRight)
			{
				VectorElement element = m_Shape.FindElement("Pivot");

				if (element == null || element.GetType() != typeof(VectorTag))
				{
					MessageBox.Show("The request operations requires a pivot point. To add a pivot point, create a new tag, with the name 'Pivot'.", "Info", MessageBoxButtons.OK);

					return;
				}

				pivot = (VectorTag)element;
			}

			for (int i = 0; i < m_Shape.m_Elements.Count; ++i)
			{
				VectorElement element = m_Shape.m_Elements[i];

				if (selectionOnly)
					if (element != m_FocusElement)
						continue;

				/*
				VectorElement oldFocus = m_FocusElement;
				IEditable oldEditFocus = m_EditingFocus;

				m_FocusElement = null;
				m_EditingFocus = null;
				*/

				int pivotX = m_Shape.m_Attributes.Width / 2;
				int pivotY = m_Shape.m_Attributes.Height / 2;

				if (pivot != null)
				{
					pivotX = pivot.m_Location.X;
					pivotY = pivot.m_Location.Y;
				}

				// Center element around pivot by offsetting it and back.

				element.HandleMove(new Point(0, 0), new Point(-pivotX, -pivotY));
				
				switch (type)
				{
					case TransformType.FlipXY:
						element.TF_FlipXY();
						break;

					case TransformType.InvertX:
						element.TF_InvertX();
						break;

					case TransformType.InvertY:
						element.TF_InvertY();
						break;

					case TransformType.RotateLeft:
						element.TF_FlipXY();
						element.TF_InvertY();
						break;

					case TransformType.RotateRight:
						element.TF_FlipXY();
						element.TF_InvertX();
						break;

					case TransformType.Rotate180:
						element.TF_InvertX();
						element.TF_InvertY();
						break;

					case TransformType.CenterPivot:
						element.HandleMove(new Point(0, 0), new Point(m_Shape.m_Attributes.Width / 2 - pivotX, m_Shape.m_Attributes.Height / 2 - pivotY));
						break;
				}

				element.HandleMove(new Point(0, 0), new Point(+pivotX, +pivotY));

				/*
				m_FocusElement = oldFocus;
				m_EditingFocus = oldEditFocus;
				*/
			}
		}

		private void VectorView_Paint(object sender, PaintEventArgs e)
		{
			Console.WriteLine("Validate");

			Graphics g = e.Graphics;

			// Set transform.

			Matrix matrix = new Matrix();
			matrix.Scale(Zoom, Zoom);
			matrix.Translate(m_Pan.X, m_Pan.Y);
			matrix.Translate(0.5f, 0.5f);
			g.Transform = matrix;

			// Set rendering options.

			{
				//g.Clip = new Region(new Rectangle(new Point(0, 0), new Size(Width, Height)));

#if true
				g.CompositingQuality = CompositingQuality.HighSpeed;
				g.InterpolationMode = InterpolationMode.Default;
				g.SmoothingMode = SmoothingMode.AntiAlias;
				//g.SmoothingMode = SmoothingMode.HighSpeed;
				g.PixelOffsetMode = PixelOffsetMode.None;

#endif

#if false
				g.CompositingQuality = CompositingQuality.HighQuality;
				g.InterpolationMode = InterpolationMode.HighQualityBicubic;
				g.SmoothingMode = SmoothingMode.HighQuality;
#endif

#if false
				g.CompositingQuality = CompositingQuality.HighQuality;
				g.InterpolationMode = InterpolationMode.NearestNeighbor;
				g.SmoothingMode = SmoothingMode.None;
				g.PixelOffsetMode = PixelOffsetMode.HighQuality;

				g.CompositingQuality = CompositingQuality.HighQuality;
				g.InterpolationMode = InterpolationMode.NearestNeighbor;
				//g.SmoothingMode = SmoothingMode.None;
				g.SmoothingMode = SmoothingMode.HighQuality;
				g.PixelOffsetMode = PixelOffsetMode.HighQuality;
#endif

				// Draw background.

				g.Clear(Color.Wheat);

				// Draw grid

				if ((Zoom % 2) == 1 && Zoom > 1)
				{
					//float offset = (Zoom - 0.0f) / 2.0f;
					float offset = 0.5f;
					//float offset = 0.0f;
					Color color = Color.FromArgb(31, 0, 0, 0);
					Pen pen = new Pen(color);
					int skip = 3;

					for (int i = 0; i < Width; ++i)
					{
						if ((i % skip) != 0)
							continue;

						PointF p1 = new PointF(i + offset, 0.0f);
						PointF p2 = new PointF(i + offset, Height - 1);

						g.DrawLine(pen, p1, p2);
					}

					for (int i = 0; i < Height; ++i)
					{
						if ((i % skip) != 0)
							continue;

						PointF p1 = new PointF(0.0f, i + offset);
						PointF p2 = new PointF(Width, i + offset);

						g.DrawLine(pen, p1, p2);
					}
				}

				// Draw canvas bounds

				{
					Pen pen = new Pen(new SolidBrush(Color.Black));

					float sx = m_Shape.m_Attributes.Width;
					float sy = m_Shape.m_Attributes.Height;
					float offset = 0.5f;

					//g.DrawRectangle(pen, -0.5f, -0.5f, sx + 1.0f, sy + 1.0f);
					g.DrawRectangle(pen, offset, offset, sx, sy);
				}

				// Draw rulers

				foreach (Ruler ruler in m_Rulers)
				{
					PointF p1;
					PointF p2;

					Pen pen = new Pen(new SolidBrush(Color.FromArgb(31, 127, 0, 63)));

					float offset = 0.5f;

					p1 = new PointF(ruler.Location.X + offset, 0.0f);
					p2 = new PointF(ruler.Location.X + offset, Height - 1);

					g.DrawLine(pen, p1, p2);

					p1 = new PointF(0.0f, ruler.Location.Y + offset);
					p2 = new PointF(Width - 1, ruler.Location.Y + offset);

					g.DrawLine(pen, p1, p2);
				}

				// Draw shape

				m_Shape.Render(g, RenderMode, m_FocusElement);
			}
		}

		private void VectorView_MouseDown(object sender, MouseEventArgs e)
		{
			Point location = ClientToView(e.Location);
			Point locationControl = e.Location;

			if (e.Button == MouseButtons.Left)
			{
				VectorElement element;
				IEditable editingFocus;

				if (m_Shape.HitTest(location, m_FocusElement, out element, out editingFocus))
				{
					X.Assert(element != null);

					if (editingFocus == null)
						editingFocus = element;

					FocusElement = element;
					EditingFocus = editingFocus;

					element.HandleHitBegin(location);
				}
				else
				{
					FocusElement = null;
					EditingFocus = null;
				}
			}

			if (e.Button == MouseButtons.Right)
			{
				if (m_MouseDown == false)
				{
					m_MouseDown = true;

					m_MouseLocation = location;
					m_MouseLocationControl = locationControl;
				}
			}

			if (e.Button == MouseButtons.Middle)
			{
				Perform_Insert(location);
			}
		}

		private void Perform_Insert(Point location)
		{
			if (FocusElement != null)
			{
				FocusElement.HandleInsert(location);
			}
		}

		private void Perform_Copy()
		{
			if (FocusElement != null)
			{
				VectorElement element = VectorElement.Copy(FocusElement);

				element.HandleMove(MouseViewLocation(), new Point(5, 5));

				m_Shape.AddElement(element);

				FocusElement = element;
				EditingFocus = element;
			}
		}

		private void VectorView_MouseMove(object sender, MouseEventArgs e)
		{
			Point location = ClientToView(e.Location);
			Point locationControl = e.Location;

			if (m_ShiftDown)
			{
				if (m_MouseDown)
				{
					Point delta = new Point();

					delta.X = locationControl.X - m_MouseLocationControl.X;
					delta.Y = locationControl.Y - m_MouseLocationControl.Y;

					delta.X /= Zoom;
					delta.Y /= Zoom;

					if (delta.X != 0 || delta.Y != 0)
					{
						m_MouseLocationControl.X += delta.X * Zoom;
						m_MouseLocationControl.Y += delta.Y * Zoom;

						m_Pan.X += delta.X;
						m_Pan.Y += delta.Y;

						Invalidate();
					}
				}
			}
			else
			{
				if (m_MouseDown)
				{
					Point delta = new Point();

					delta.X = location.X - m_MouseLocation.X;
					delta.Y = location.Y - m_MouseLocation.Y;

					if (delta.X != 0 || delta.Y != 0)
					{
						m_MouseLocation = location;

						if (FocusElement != null)
						{
							FocusElement.HandleMove(location, delta);
						}
					}
				}
			}

			if (FocusElement != null)
			{
				FocusElement.HandleHover(location);
			}
		}

		private void VectorView_MouseUp(object sender, MouseEventArgs e)
		{
			Point location = ClientToView(e.Location);

			if (e.Button == MouseButtons.Right)
			{
				m_MouseDown = false;
			}
		}

		public VectorElement FocusElement
		{
			get
			{
				return m_FocusElement;
			}
			set
			{
				if (m_FocusElement == value)
					return;

				m_FocusElement = value;

				if (OnFocusChange != null)
					OnFocusChange(m_FocusElement);

				EditingFocus = value;

				Invalidate();
			}
		}

		public IEditable EditingFocus
		{
			get
			{
				return m_EditingFocus;
			}
			set
			{
				if (m_EditingFocus == value)
					return;

				m_EditingFocus = value;

				if (OnEditingFocusChange != null)
					OnEditingFocusChange(m_EditingFocus);

				Invalidate();
			}
		}

		private void VectorView_KeyDown(object sender, KeyEventArgs e)
		{
			Point location = MouseViewLocation();

			if (e.KeyCode == Keys.Delete)
			{
				if (m_FocusElement != null)
				{
					m_Shape.RemoveElement(m_FocusElement);
				}
			}
			if (e.KeyCode == Keys.ShiftKey)
			{
				m_ShiftDown = true;
			}
			if (e.KeyCode == Keys.R)
			{
				Ruler daRuler = null;

				foreach (Ruler ruler in m_Rulers)
				{
					if (ruler.Location == location)
						daRuler = ruler;
				}

				if (daRuler != null)
				{
					RemoveRuler(daRuler);
				}
				else
				{
					Ruler ruler = new Ruler();
					ruler.Location = location;

					AddRuler(ruler);
				}
			}
			if (e.KeyCode == Keys.I)
			{
				Perform_Insert(location);
			}
			if (e.KeyCode == Keys.E)
			{
				if (m_FocusElement != null)
					m_FocusElement.HandleElementRemove();
			}
			if (e.KeyCode == Keys.D)
			{
				Perform_Copy();
			}
			if (e.KeyCode == Keys.PageUp)
			{
				if (m_FocusElement != null)
					m_Shape.MoveUp(m_FocusElement);
			}
			if (e.KeyCode == Keys.PageDown)
			{
				if (m_FocusElement != null)
					m_Shape.MoveDown(m_FocusElement);
			}
		}

		private void VectorView_KeyUp(object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.ShiftKey)
			{
				m_ShiftDown = false;
			}
		}
	}

	public class Ruler
	{
		public Point Location = new Point();
	}
}

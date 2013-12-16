using System;
using System.Xml.Serialization;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Drawing.Drawing2D;

namespace vecdraw
{
	public enum RenderMode
	{
		Editing,
        PreciseEditing,
		Clean,
		Groups,
        Preview,
        ZOrder
	}

	public class GraphicAttributes
	{
		public int Width = 480;
		public int Height = 320;
		public bool HasSilhouette = true;
		public bool HasSprite = false;
	}

    static class StaticMethods
    {
        private static Random mRandom = new Random();

        public static Brush GetBrush(RenderMode mode, bool active, bool outline, Elements.VectorElement elem)
        {
            Color color = GetColor(mode, active, outline, elem);

            if (elem.Visible)
                return new SolidBrush(color);
            else
                return new HatchBrush(HatchStyle.Percent50, color, Color.Transparent);
        }

        public static Pen GetPen(RenderMode mode, bool active, bool outline, Elements.VectorElement elem, float stroke)
        {
            Brush brush = GetBrush(mode, active, outline, elem);

            switch (mode)
            {
                case RenderMode.Clean:
                    return new Pen(brush, stroke * 2.0f);
                case RenderMode.Editing:
                    return new Pen(brush, stroke * 2.0f);
                case RenderMode.Groups:
                    return new Pen(brush, stroke * 2.0f);
                case RenderMode.PreciseEditing:
                    return new Pen(brush, 1.0f);
            }

            return new Pen(brush);
        }

        public static Color GetColor(RenderMode mode, bool active, bool outline, Elements.VectorElement elem)
        {
            if (active)
            {
                switch (mode)
                {
                    case RenderMode.Clean:
                        return Color.DarkGray;
                    case RenderMode.Editing:
                        return Color.Blue;
                    case RenderMode.Groups:
                        return Color.Black;
                    case RenderMode.PreciseEditing:
                        return Color.Blue;
                    case RenderMode.Preview:
                        {
                            int a = outline ? (int)(elem.LineOpacity * 255.0f) : (int)(elem.FillOpacity * 255.0f);
                            int r = outline ? elem.VecColor.R * elem.Line / 255 : elem.VecColor.R * elem.Fill / 255;
                            int g = outline ? elem.VecColor.G * elem.Line / 255 : elem.VecColor.G * elem.Fill / 255;
                            int b = outline ? elem.VecColor.B * elem.Line / 255 : elem.VecColor.B * elem.Fill / 255;
                            return Color.FromArgb(a, r, g, b);
                        }
                    case RenderMode.ZOrder:
                        return Color.FromArgb(255, mRandom.Next(256), 0, mRandom.Next(256));
                }
            }
            else
            {
                switch (mode)
                {
                    case RenderMode.Clean:
                        return Color.DarkGray;
                    case RenderMode.Editing:
                        return Color.Red;
                    case RenderMode.Groups:
                        return Color.Black;
                    case RenderMode.PreciseEditing:
                        return Color.Red;
                    case RenderMode.Preview:
                        {
                            int a = outline ? (int)(elem.LineOpacity * 255.0f) : (int)(elem.FillOpacity * 255.0f);
                            int r = outline ? elem.VecColor.R * elem.Line / 255 : elem.VecColor.R * elem.Fill / 255;
                            int g = outline ? elem.VecColor.G * elem.Line / 255 : elem.VecColor.G * elem.Fill / 255;
                            int b = outline ? elem.VecColor.B * elem.Line / 255 : elem.VecColor.B * elem.Fill / 255;
                            return Color.FromArgb(a, r, g, b);
                        }
                    case RenderMode.ZOrder:
                        return Color.FromArgb(255, mRandom.Next(256), mRandom.Next(256), mRandom.Next(256));
                }
            }

            return Color.White;
        }

        public static bool DrawSolid(RenderMode mode, Elements.VectorElement elem)
        {
            switch (mode)
            {
                case RenderMode.Clean:
                case RenderMode.Editing:
                case RenderMode.Groups:
                case RenderMode.PreciseEditing:
                    return false;

                case RenderMode.Preview:
                case RenderMode.ZOrder:
                    if (elem.Visible)
                        return true;
                    return false;

                default:
                    throw new NotImplementedException();
            }
        }

        public static bool DrawWire(RenderMode mode, Elements.VectorElement elem)
        {
            switch (mode)
            {
                case RenderMode.Clean:
                case RenderMode.Editing:
                case RenderMode.Groups:
                case RenderMode.PreciseEditing:
                case RenderMode.Preview:
                    return true;

                case RenderMode.ZOrder:
                    return false;

                default:
                    throw new NotImplementedException();
            }
        }

        public static bool DrawEdit(RenderMode mode)
        {
            switch (mode)
            {
                case RenderMode.Editing:
                case RenderMode.PreciseEditing:
                    return true;

                default:
                    return false;
            }
        }

        public static bool DrawInvisible(RenderMode mode)
        {
            switch (mode)
            {
                case RenderMode.Clean:
                case RenderMode.Editing:
                case RenderMode.Groups:
                case RenderMode.PreciseEditing:
                    return true;

                case RenderMode.Preview:
                case RenderMode.ZOrder:
                    return false;

                default:
                    throw new NotImplementedException();
            }
        }
    }

	namespace Elements
	{
		#region Element

		[Serializable]
		public class VectorElement : IEditable
		{
			public delegate void ChangeHandler(VectorElement element);
			public event ChangeHandler OnChange;
			public delegate void PropertyChangeHandler(VectorElement element, DataInfo di);
			public event PropertyChangeHandler OnPropertyChange;

			//protected static Color ControlColor = Color.Black;
			protected static Color ControlColor = Color.FromArgb(63, 0, 0, 0);
			protected static Color ActiveColor = Color.White;

			public VectorElement()
			{
			}

			public virtual bool HitTest(Point location, out IEditable editingFocus)
			{
				editingFocus = null;

				return false;
			}

			public virtual void HandleHitBegin(Point location)
			{
			}

			public virtual void HandleHover(Point location)
			{
			}

			public virtual void HandleMove(Point location, Point delta)
			{
			}

			public virtual void HandleInsert(Point location)
			{
			}

			public virtual void HandleElementRemove()
			{
			}

			public virtual void TF_FlipXY()
			{
			}

			public virtual void TF_InvertX()
			{
			}

			public virtual void TF_InvertY()
			{
			}

			public virtual void Render(Graphics g, RenderMode mode, bool hasFocus)
			{
			}

			public void HandlePropertyChanged(DataInfo fi)
			{
				//Invalidate();
				if (OnPropertyChange != null)
					OnPropertyChange(this, fi);
			}

			public void Invalidate()
			{
				if (OnChange != null)
					OnChange(this);
			}

			public static VectorElement Copy(VectorElement element)
			{
				XmlSerializerFactory f = new XmlSerializerFactory();
				System.Xml.Serialization.XmlSerializer serializer = f.CreateSerializer(element.GetType());
				//System.Xml.Serialization.XmlSerializer serializer = ;
				//System.Runtime.Serialization.Formatters.Binary.BinaryFormatter formatter = new System.Runtime.Serialization.Formatters.Binary.BinaryFormatter();
				System.IO.MemoryStream stream = new System.IO.MemoryStream();
				serializer.Serialize(stream, element);
				stream.Flush();
				stream.Seek(0, System.IO.SeekOrigin.Begin);

				VectorElement result = serializer.Deserialize(stream) as VectorElement;

				return result;
			}

			[Property("Name")]
			public String Name
			{
				get
				{
					if (m_Name == String.Empty)
						return "NoName";

					return m_Name;
				}
				set
				{
					m_Name = value;
				}
			}
			[Property("Group")]
			public String Group = String.Empty;

			[Property("Stroke")]
			public float LineWidth = 1.5f;
			[Property("Stroke Hardness")]
			public float LineHardness = 1.0f;
			[Property("Fill")]
			public int Fill = 31;
			[Property("Fill Opacity")]
			public float FillOpacity = 1.0f;
			[Property("Line")]
			public int Line = 255;
			[Property("Line Opacity")]
			public float LineOpacity = 1.0f;

			[Property("Color")]
			public VecColor VecColor = new VecColor();

			[Property("Visible")]
			public bool Visible = true;
			[Property("Collision")]
			public bool Collision = false;

			[Property("Filled")]
			public bool Filled = true;

			[NonSerialized]
			private String m_Name = String.Empty;
		}

		#endregion

		#region PolyPoint

		[Serializable]
		public class PolyPoint : IEditable
		{
			private IEditable m_Parent;

			public PolyPoint(IEditable parent)
			{
				m_Parent = parent;

				X = 0;
				Y = 0;
			}

			public PolyPoint(IEditable parent, int x, int y)
			{
				m_Parent = parent;

				X = x;
				Y = y;
			}

			public void HandlePropertyChanged(DataInfo fi)
			{
				m_Parent.HandlePropertyChanged(fi);
			}

			public Point ToPoint()
			{
				return new Point(X, Y);
			}

			public override int GetHashCode()
			{
				return 0;
			}

			public override bool Equals(object obj)
			{
				PolyPoint point = obj as PolyPoint;

				if (obj == null)
					return false;

				return point.X == X && point.Y == Y;
			}

			[Property("X")]
			public int X;
			[Property("Y")]
			public int Y;
		}

		#endregion

		#region Poly

		[Serializable]
		public class VectorPoly : VectorElement
		{
			const int INDEX_NONE = -1;
			const int HIT_RADIUS = 3;
			//const int HIT_RADIUS = 2;
			const int INSERT_RADIUS = HIT_RADIUS * 4;

			private int m_FocusPoint = INDEX_NONE;

			private Point m_HoverLocation;

			public VectorPoly()
				: base()
			{
			}

			public override bool HitTest(Point location, out IEditable editingFocus)
			{
				editingFocus = null;

				foreach (PolyPoint point in m_Points)
				{
					Point delta = point.ToPoint().Subtract(location);

					double distance = delta.Magnitude();

					if (distance <= HIT_RADIUS)
					{
						editingFocus = point;

						return true;
					}
				}

				List<PointF> outline = new List<PointF>();

				foreach (PolyPoint point in m_Points)
				{
					PointF temp = new PointF(point.X, point.Y);

					outline.Add(temp);
				}

				PointF p = new PointF(location.X, location.Y);

				if (CollisionDetect.HitTest_Outline(outline, p))
					return true;

				return false;
			}

			public override void HandleHitBegin(Point location)
			{
				m_FocusPoint = INDEX_NONE;

				double bestDistance = 1000000.0f;

				for (int i = 0; i < m_Points.Count; ++i)
				{
					PolyPoint point = m_Points[i];

					Point delta = point.ToPoint().Subtract(location);

					double distance = delta.Magnitude();

					if (distance <= HIT_RADIUS && distance < bestDistance)
						m_FocusPoint = i;
				}
			}

			public override void HandleHover(Point location)
			{
				m_HoverLocation = location;

				if (m_FocusPoint == INDEX_NONE)
				{
					Invalidate();
				}
			}

			public override void HandleMove(Point location, Point delta)
			{
				if (m_FocusPoint == INDEX_NONE)
				{
#if true
					for (int i = 0; i < m_Points.Count; ++i)
					{
						m_Points[i].X += delta.X;
						m_Points[i].Y += delta.Y;
					}
#endif
				}
				else
				{
					PolyPoint p = m_Points[m_FocusPoint];

					p.X += delta.X;
					p.Y += delta.Y;
				}

				RebuildLines();

				Invalidate();
			}

			struct Plane
			{
				public Plane FromPoints(PointF p1, PointF p2)
				{
					Plane result = new Plane();

					result.Normal = p2.Subtract(p1);
					result.Normal = result.Normal.Normalized();
					result.Distance = p1.Dot(result.Normal);

					return result;
				}

				public float Dot(PointF p)
				{
					return Normal.Dot(p) - Distance;
				}

				public PointF Normal;
				public float Distance;
			}

			private bool TryGetIntersectPoint(Point location, out Point out_Point, out int index)
			{
				out_Point = new Point(0, 0);

				float bestDistance = -1.0f;
				PointF bestPoint = new PointF();
				index = -1;

				for (int i = 0; i < m_Lines.Count; ++i)
				{
					PolyLine line = m_Lines[i];

					PointF point1 = line.m_Point1.ToPoint().ToPointF();
					PointF point2 = line.m_Point2.ToPoint().ToPointF();

					Plane plane1 = new Plane();
					Plane plane2 = new Plane();

					plane1 = plane1.FromPoints(line.m_Point1.ToPoint(), line.m_Point2.ToPoint());
					plane2 = plane2.FromPoints(line.m_Point2.ToPoint(), line.m_Point1.ToPoint());

					PointF locationF = location.ToPointF();

					float d1 = plane1.Dot(locationF);
					float d2 = plane2.Dot(locationF);

					if (d1 < 0.0f || d2 < 0.0f)
						continue;

					PointF direction = point2.Subtract(point1);
					direction = direction.Normalized();
					PointF normal = new PointF(-direction.Y, +direction.X);
					Plane plane = new Plane();
					plane.Normal = normal;
					plane.Distance = normal.Dot(point1);

					float distance = Math.Abs(plane.Dot(locationF));

					if (distance < INSERT_RADIUS && (distance < bestDistance || bestDistance < 0.0f))
					{
						bestDistance = distance;
						index = i;

						bestPoint = new PointF();
						bestPoint.X = point1.X + direction.X * d1;
						bestPoint.Y = point1.Y + direction.Y * d1;
					}
				}

				if (bestDistance < 0.0f)
				{
					// todo: hit corner points

#if false
					foreach (PolyPoint point in m_Points)
					{
						Point temp = point.ToPoint();

						float distance = temp.Subtract(location).Magnitude();
					}
#endif

					if (bestDistance < 0.0f)
						return false;
				}
				else
				{
					index = (index + 1) % m_Points.Count;

					out_Point = new Point((int)bestPoint.X, (int)bestPoint.Y);
				}

				return true;
			}

			public override void HandleInsert(Point location)
			{
				Point point;
				int index;

				if (TryGetIntersectPoint(location, out point, out index))
				{
					m_Points.Insert(index, new PolyPoint(this, point.X, point.Y));

					RebuildLines();

					Invalidate();

					Console.WriteLine("P: {0} {1}", point.X, point.Y);
				}
			}

			public override void HandleElementRemove()
			{
				if (m_FocusPoint == INDEX_NONE)
					return;
				if (m_Points.Count <= 3)
					return;

				m_Points.RemoveAt(m_FocusPoint);
				
				m_FocusPoint = INDEX_NONE;
				
				RebuildLines();

				Invalidate();
			}

			public void AddPoint(PolyPoint point)
			{
				m_Points.Add(point);

				RebuildLines();

				//m_FocusPoint = -1;

				Invalidate();
			}

			public void RemovePoint(int index)
			{
				if (m_Points.Count <= 3)
				{
					// todo: signal view we wish to remove polygon as a whole.

					throw new Exception("blaat");
				}

				m_Points.RemoveAt(index);

				m_FocusPoint = 0;

				Invalidate();
			}

			public override void TF_FlipXY()
			{
				List<Point> points = new List<Point>();

				for (int i = 0; i < m_Points.Count; ++i)
				{
					Point point = new Point(m_Points[i].Y, m_Points[i].X);

					points.Add(point);
				}

				Points = points.ToArray();
			}

			public override void TF_InvertX()
			{
				List<Point> points = new List<Point>();

				for (int i = 0; i < m_Points.Count; ++i)
				{
					Point point = new Point(-m_Points[i].X, m_Points[i].Y);

					points.Add(point);
				}

				Points = points.ToArray();
			}

			public override void TF_InvertY()
			{
				List<Point> points = new List<Point>();

				for (int i = 0; i < m_Points.Count; ++i)
				{
					Point point = new Point(m_Points[i].X, -m_Points[i].Y);

					points.Add(point);
				}

				Points = points.ToArray();
			}

			public override void Render(Graphics g, RenderMode mode, bool hasFocus)
			{
                if (m_Points.Count > 2)
                {
                    Point[] points = Points.ToArray();

                    PointF[] pointsF = new PointF[points.Length];

                    for (int i = 0; i < points.Length; ++i)
                    {
                        //pointsF[i] = new PointF(points[i].X, points[i].Y);
                        pointsF[i] = new PointF(points[i].X + 0.5f, points[i].Y + 0.5f);
                    }

                    if (StaticMethods.DrawWire(mode, this))
                    {
                        Pen pen = StaticMethods.GetPen(mode, hasFocus, true, this, LineWidth);

                        g.DrawPolygon(
                            pen,
                            pointsF);
                    }

                    if (StaticMethods.DrawSolid(mode, this))
                    {
                        Pen pen = StaticMethods.GetPen(mode, hasFocus, false, this, LineWidth);
                        Brush brush = StaticMethods.GetBrush(mode, hasFocus, false, this);

                        g.FillPolygon(
                            brush,
                            pointsF);
                    }
                }

				if (StaticMethods.DrawEdit(mode))
				{
					foreach (PolyPoint point in m_Points)
					{
						g.DrawEllipse(
							new Pen(new SolidBrush(ControlColor), 1.0f),
							//point.X + 0.5f - HIT_RADIUS,
							//point.Y + 0.5f - HIT_RADIUS,
							point.X - HIT_RADIUS,
							point.Y - HIT_RADIUS,
							//HIT_RADIUS * 2.0f,
							//HIT_RADIUS * 2.0f);
							HIT_RADIUS * 2.0f + 1.0f,
							HIT_RADIUS * 2.0f + 1.0f);
					}

					if (hasFocus && m_FocusPoint == INDEX_NONE)
					{
						Point point;
						int index;

						if (TryGetIntersectPoint(m_HoverLocation, out point, out index))
						{
							int radius = 2;

							g.DrawEllipse(
								new Pen(new SolidBrush(Color.White), 1.0f),
								//point.X + 0.5f - radius,
								//point.Y + 0.5f - radius,
								point.X - radius,
								point.Y - radius,
								//radius * 2.0f,
								//radius * 2.0f);
								radius * 2.0f + 1.0f,
								radius * 2.0f + 1.0f);
						}
					}
				}
			}

			private void RebuildLines()
			{
				m_Lines.Clear();

				for (int i = 0; i < m_Points.Count; ++i)
				{
					int index1 = (i + 0) % m_Points.Count;
					int index2 = (i + 1) % m_Points.Count;

					PolyPoint point1 = m_Points[index1];
					PolyPoint point2 = m_Points[index2];

					PolyLine line = new PolyLine(
						point1,
						point2);

					m_Lines.Add(line);
				}
			}

			[Serializable]
			class PolyLine
			{
				public PolyLine(PolyPoint point1, PolyPoint point2)
				{
					m_Point1 = point1;
					m_Point2 = point2;
				}

				public bool HitTest(Point location)
				{
					Point delta1 = m_Point1.ToPoint().Subtract(location);
					Point delta2 = m_Point2.ToPoint().Subtract(location);

					double radius = 4.0f;

					return delta1.Magnitude() < radius || delta2.Magnitude() < radius;
				}

				public PolyPoint m_Point1;
				public PolyPoint m_Point2;
			}

			public Point[] Points
			{
				get
				{
					Point[] points = new Point[m_Points.Count];

					for (int i = 0; i < m_Points.Count; ++i)
						points[i] = new Point(m_Points[i].X, m_Points[i].Y);

					return points;
				}
				set
				{
					m_Points.Clear();

					foreach (Point point in value)
					{
						PolyPoint temp = new PolyPoint(this, point.X, point.Y);

						AddPoint(temp);
					}
				}
			}

			private List<PolyPoint> m_Points = new List<PolyPoint>();
			private List<PolyLine> m_Lines = new List<PolyLine>();
		}

		#endregion

		#region Circle

		[Serializable]
		public class VectorCircle : VectorElement
		{
			public override bool HitTest(Point location, out IEditable editingFocus)
			{
				editingFocus = null;

				Point delta = m_Location.Subtract(location);

				double distance = delta.Magnitude();

				return distance <= m_Radius;
			}

			public override void HandleMove(Point location, Point delta)
			{
				m_Location.X += delta.X;
				m_Location.Y += delta.Y;

				Invalidate();
			}

			public override void TF_FlipXY()
			{
				int temp = m_Location.X;

				m_Location.X = m_Location.Y;
				m_Location.Y = temp;

				Invalidate();
			}

			public override void TF_InvertX()
			{
				m_Location.X = -m_Location.X;

				Invalidate();
			}

			public override void TF_InvertY()
			{
				m_Location.Y = -m_Location.Y;

				Invalidate();
			}

			public override void Render(Graphics g, RenderMode mode, bool hasFocus)
			{
                if (StaticMethods.DrawWire(mode, this))
                {
                    Pen pen = StaticMethods.GetPen(mode, hasFocus, true, this, LineWidth);
                    Brush brush = StaticMethods.GetBrush(mode, hasFocus, true, this);

                    g.DrawEllipse(
                        pen,
                        m_Location.X - m_Radius,
                        m_Location.Y - m_Radius,
                        m_Radius * 2 + 1.0f,
                        m_Radius * 2 + 1.0f);

                    if (StaticMethods.DrawEdit(mode))
                    {
                        g.DrawEllipse(
                            new Pen(new SolidBrush(Color.FromArgb(127, 0, 0, 0))),
                            m_Location.X,
                            m_Location.Y,
                            1.0f,
                            1.0f);
                    }
                }

                if (StaticMethods.DrawSolid(mode, this))
                {
                    Pen pen = StaticMethods.GetPen(mode, hasFocus, false, this, LineWidth);
                    Brush brush = StaticMethods.GetBrush(mode, hasFocus, false, this);

                    g.FillEllipse(
                        brush,
                        m_Location.X - m_Radius,
                        m_Location.Y - m_Radius,
                        m_Radius * 2 + 1.0f,
                        m_Radius * 2 + 1.0f);
                }
			}

			[Property("Location")]
			public Point m_Location = new Point(0, 0);
			[RangePropery("Radius", 1, 400)]
			public int m_Radius = 0;
		}

		#endregion

        #region Rect

        [Serializable]
        public class VectorRect : VectorElement
        {
            public override bool HitTest(Point location, out IEditable editingFocus)
            {
                editingFocus = null;

                Point delta = location.Subtract(m_Location);

                if (delta.X < 0)
                    return false;
                if (delta.Y < 0)
                    return false;
                if (delta.X > m_Size.X)
                    return false;
                if (delta.Y > m_Size.Y)
                    return false;

                return true;
            }

            public override void HandleMove(Point location, Point delta)
            {
                m_Location.Offset(delta);

                Invalidate();
            }

            public override void TF_FlipXY()
            {
                /*int temp;

                temp = m_Min.X;
                m_Min.X = m_Min.Y;
                m_Min.Y = temp;

                temp = m_Max.X;
                m_Max.X = m_Max.Y;
                m_Max.Y = temp;

                Invalidate();*/
            }

            public override void TF_InvertX()
            {
                /*int temp;

                temp = m_Min.X;
                m_Min.X = m_Max.Y;
                m_Max.X = temp;

                Invalidate();*/
            }

            public override void TF_InvertY()
            {
                /*int temp;

                temp = m_Min.Y;
                m_Min.Y = m_Max.Y;
                m_Max.Y = temp;

                Invalidate();*/
            }

            public override void Render(Graphics g, RenderMode mode, bool hasFocus)
            {
                Rectangle rect = new Rectangle(m_Location, new Size(m_Size.X, m_Size.Y));

                if (StaticMethods.DrawSolid(mode, this))
                {
                    Brush brush = StaticMethods.GetBrush(mode, hasFocus, false, this);
                    Pen pen = StaticMethods.GetPen(mode, hasFocus, false, this, LineWidth);

                    g.FillRectangle(brush, rect);
                }

                if (StaticMethods.DrawWire(mode, this))
                {
                    Brush brush = StaticMethods.GetBrush(mode, hasFocus, true, this);
                    Pen pen = StaticMethods.GetPen(mode, hasFocus, true, this, LineWidth);

                    g.DrawRectangle(pen, rect);
                }
            }

            [Property("Location")]
            public Point m_Location = new Point(0, 0);
            [Property("Size")]
            public Point m_Size = new Point(0, 0);
        }

        #endregion

		#region Tag

		[Serializable]
		public class VectorTag : VectorElement
		{
			private const int HIT_SIZE = 3;

			public override bool HitTest(Point location, out IEditable editingFocus)
			{
				editingFocus = null;

				if (location.X >= m_Location.X && location.Y >= m_Location.Y &&
					location.X < m_Location.X + HIT_SIZE && location.Y < m_Location.Y + HIT_SIZE)
					return true;

				return false;
			}

			public override void HandleMove(Point location, Point delta)
			{
				m_Location.X += delta.X;
				m_Location.Y += delta.Y;

				Invalidate();
			}

			public override void TF_FlipXY()
			{
				int temp = m_Location.X;

				m_Location.X = m_Location.Y;
				m_Location.Y = temp;

				Invalidate();
			}

			public override void TF_InvertX()
			{
				m_Location.X = -m_Location.X;

				Invalidate();
			}

			public override void TF_InvertY()
			{
				m_Location.Y = -m_Location.Y;

				Invalidate();
			}

			public override void Render(Graphics g, RenderMode mode, bool hasFocus)
			{
				//PointF p = new PointF(m_Location.X + 0.5f, m_Location.Y + 0.5f);
				PointF p = new PointF(m_Location.X - (HIT_SIZE - 1) / 2, m_Location.Y - (HIT_SIZE - 1) / 2);
				PointF p2 = new PointF(m_Location.X, m_Location.Y);
				PointF size = new PointF(20.0f, 8.0f);

				String text = Name.ToUpper();

				if (mode == RenderMode.Editing)
					text = Name.ToUpper();
				if (mode == RenderMode.Groups)
					text = String.Empty;

				Font font = new Font(FontFamily.GenericSansSerif, 7.0f, FontStyle.Regular);

				SizeF stringSize = g.MeasureString(text, font);

                if (StaticMethods.DrawEdit(mode))
                {
                    g.FillRectangle(
                        //new SolidBrush(Color.Lime),
                        new SolidBrush(Color.FromArgb(127, 127, 255, 0)),
                        new RectangleF(p, stringSize));

                    g.DrawString(
                        text,
                        font,
                        new SolidBrush(Color.Black),
                        p);

                    g.FillRectangle(
                        new SolidBrush(Color.FromArgb(127, 0, 0, 0)),
                        new RectangleF(p, new SizeF(HIT_SIZE, HIT_SIZE)));

                    g.FillRectangle(
                        new SolidBrush(Color.FromArgb(127, 0, 0, 0)),
                        new RectangleF(p2, new SizeF(1, 1)));
                }
			}

			[Property("Location")]
			public Point m_Location = new Point(0, 0);
		}

		#endregion

		#region Picture

		class VectorPicture : VectorElement
		{
			public override bool HitTest(Point location, out IEditable editingFocus)
			{
				editingFocus = null;

				if (location.X >= m_Location.X && location.Y >= m_Location.Y &&
					location.X < m_Location.X + m_Size.X && location.Y < m_Location.Y + m_Size.Y)
					return true;

				return false;
			}

			public override void HandleMove(Point location, Point delta)
			{
				m_Location.X += delta.X;
				m_Location.Y += delta.Y;

				Invalidate();
			}

			public override void TF_FlipXY()
			{
				int temp = m_Location.X;

				m_Location.X = m_Location.Y;
				m_Location.Y = temp;

				Invalidate();
			}

			public override void TF_InvertX()
			{
				m_Location.X = -m_Location.X;

				Invalidate();
			}

			public override void TF_InvertY()
			{
				m_Location.Y = -m_Location.Y;

				Invalidate();
			}

            public override void Render(Graphics g, RenderMode mode, bool hasFocus)
            {
                PointF p = new PointF(m_Location.X, m_Location.Y);
                SizeF size;
                if (m_Image != null)
                    size = new SizeF(m_Image.Width, m_Image.Height);
                else
                    size = new SizeF(32.0f, 32.0f);

                size = new SizeF(size.Width / m_ContentScale, size.Height / m_ContentScale);

                RectangleF rect = new RectangleF(p, size);

                if (StaticMethods.DrawEdit(mode))
                {
                    g.FillRectangle(
                        new SolidBrush(Color.FromArgb(127, 127, 255, 0)),
                        rect);
                }

                if (StaticMethods.DrawEdit(mode) || StaticMethods.DrawSolid(mode, this))
                {
                    if (m_Image != null)
                    {
                        g.DrawImage(m_Image, rect);
                    }
                }
            }

			[Property("Location")]
			public Point m_Location = new Point(0, 0);

			[Property("FileName")]
			public FileName FileName
			{
				get
				{
					return m_FileName;
				}
				set
				{
					m_Image = null;

					m_FileName = value;

					if (value != FileName.Empty)
					{
						try
						{
							m_Image = Image.FromFile(value.Path);
						}
						catch (Exception e)
						{
							// todo ?
						}
					}

					Invalidate();
				}
			}

            private FileName m_FileName = FileName.Empty;

            [Property("Content Scale")]
            public int ContentScale
            {
                get
                {
                    return m_ContentScale;
                }
                set
                {
                    m_ContentScale = value;

                    Invalidate();
                }
            }

            private int m_ContentScale = 1;

			private Image m_Image;
			private Point m_Size
			{
				get
				{
					if (m_Image == null)
						return new Point(32, 32);
					else
						return new Point(m_Image.Size.Width, m_Image.Size.Height);
				}
			}
		}

		#endregion

		#region Shape

		public class Shape
		{
			public delegate void AddHandler(VectorElement element);
			public event AddHandler OnAdd;
			public delegate void RemoveHandler(VectorElement element);
			public event RemoveHandler OnRemove;
			public delegate void ChangeHandler(VectorElement element);
			public event ChangeHandler OnChange;
			public delegate void PropertyChangeHandler(VectorElement element, DataInfo di);
			public event PropertyChangeHandler OnPropertyChange;
			public delegate void DepthChangedHandler(VectorElement element);
			public event DepthChangedHandler OnDepthChange;

			public void Render(Graphics g, RenderMode mode, VectorElement focusElement)
			{
				foreach (VectorElement element in m_Elements)
				{
					bool hasFocus = element == focusElement;

                    if (element.Visible == false && !StaticMethods.DrawInvisible(mode))
                        continue;

					element.Render(g, mode, hasFocus);
				}
			}

			private void HandleChange(VectorElement element)
			{
				if (OnChange != null)
					OnChange(element);
			}

			private void HandlePropertyChange(VectorElement element, DataInfo di)
			{
				if (OnPropertyChange != null)
					OnPropertyChange(element, di);
			}

			public void AddElement(VectorElement element)
			{
				element.Name = MakeName(element);

				m_Elements.Add(element);

				element.OnChange += HandleChange;
				element.OnPropertyChange += HandlePropertyChange;

				if (OnAdd != null)
					OnAdd(element);
			}

			public void RemoveElement(VectorElement element)
			{
				m_Elements.Remove(element);

				if (OnRemove != null)
					OnRemove(element);
			}

			public void ClearElements()
			{
				VectorElement[] elements = m_Elements.ToArray();

				foreach (VectorElement element in elements)
					RemoveElement(element);
			}

			private String MakeName(VectorElement element)
			{
				String className = element.GetType().Name;

				if (className.StartsWith("Vector"))
					className = className.Substring("Vector".Length);

				String result;

				int i = 1;

				do
				{
					result = className + i;

					i++;
				} while (FindElement(result) != null);

				return result;
			}

			public VectorElement FindElement(String name)
			{
				foreach (VectorElement element in m_Elements)
				{
					if (element.Name == name)
						return element;
				}

				return null;
			}

			public bool HitTest(Point location, VectorElement ignore, out VectorElement element, out IEditable editingFocus)
			{
				element = null;
				editingFocus = null;

				int index = m_Elements.IndexOf(ignore);

				for (int i = 0; i < m_Elements.Count; ++i)
				{
					VectorElement temp = m_Elements[(index + i + 1) % m_Elements.Count];

					if (temp.HitTest(location, out editingFocus))
					{
						element = temp;

						return true;
					}
				}

				return false;
			}

			public void MoveUp(VectorElement element)
			{
				int index = m_Elements.IndexOf(element);

				if (index > 0)
				{
					int index1 = index;
					int index2 = index - 1;

					m_Elements[index1] = m_Elements[index2];
					m_Elements[index2] = element;

					if (OnDepthChange != null)
						OnDepthChange(element);
				}
			}

			public void MoveDown(VectorElement element)
			{
				int index = m_Elements.IndexOf(element);

				if (index + 1 < m_Elements.Count)
				{
					int index1 = index;
					int index2 = index + 1;

					m_Elements[index1] = m_Elements[index2];
					m_Elements[index2] = element;

					if (OnDepthChange != null)
						OnDepthChange(element);
				}
			}

			enum SectionType
			{
				Undefined,
				Graphic,
				Element,
				Editor
			}

			public void Load(String fileName)
			{
				String dirName = Path.GetDirectoryName(Path.GetFullPath(fileName));

				Directory.SetCurrentDirectory(dirName);

				ClearElements();

				SectionType sectionType = SectionType.Undefined;

				using (TextReader reader = new StreamReader(new FileStream(fileName, FileMode.Open)))
				{
					String line;

					VectorElement element = null;

					while ((line = reader.ReadLine()) != null)
					{
						//Console.WriteLine(line);
						if (line.Trim() == String.Empty)
							continue;
						if (line.StartsWith("#"))
							continue;

						if (!line.StartsWith("\t"))
						{
							element = null;

							switch (line)
							{
								case "graphic":
									sectionType = SectionType.Graphic;
									break;

								case "circle":
									sectionType = SectionType.Element;
									element = new VectorCircle();
									break;

								case "poly":
									sectionType = SectionType.Element;
									element = new VectorPoly();
									break;

								case "curve":
									sectionType = SectionType.Element;
									// todo
									break;

                                case "rect":
                                    sectionType = SectionType.Element;
                                    element = new VectorRect();
                                    break;

								case "picture":
									sectionType = SectionType.Element;
									element = new VectorPicture();
									break;

								case "tag":
									sectionType = SectionType.Element;
									element = new VectorTag();
									break;

								case "editor":
									sectionType = SectionType.Editor;
									break;

								default:
									throw new Exception(String.Format("unknown element type: {0}", line));
							}

							if (element != null)
							{
								AddElement(element);
							}
						}
						else
						{
							line = line.Trim();

							String[] items = line.Split(' ');

							if (sectionType == SectionType.Graphic)
							{
								if (items[0] == "has_silhouette")
								{
									bool value = items[1] == "0" ? false : true;

									m_Attributes.HasSilhouette = value;
								}
								else if (items[0] == "has_sprite")
								{
									bool value = items[1] == "0" ? false : true;

									m_Attributes.HasSprite = value;
								}
								else
								{
									throw new Exception(String.Format("unknown property: {0}", items[0]));
								}
							}
							if (sectionType == SectionType.Editor)
							{
								if (items[0] == "canvas_width")
								{
									m_Attributes.Width = Int32.Parse(items[1]);
								}
								else if (items[0] == "canvas_height")
								{
									m_Attributes.Height = Int32.Parse(items[1]);
								}
								else
								{
									throw new Exception(String.Format("unknown property: {0}", items[0]));
								}
							}
							if (sectionType == SectionType.Element)
							{
								if (element == null)
									throw new Exception(String.Format("syntax error: {0}", line));

								// Shared properties

								if (items[0] == "name")
								{
									String name = items[1];

									element.Name = name;
								}
								else if (items[0] == "group")
								{
									String group = items[1];

									element.Group = group;
								}
								else if (items[0] == "color")
								{
									int r = Int32.Parse(items[1], Global.CultureEN);
									int g = Int32.Parse(items[2], Global.CultureEN);
									int b = Int32.Parse(items[3], Global.CultureEN);

									element.VecColor = new VecColor(r, g, b);
								}
								else if (items[0] == "fill")
								{
									int v = Int32.Parse(items[1], Global.CultureEN);

									element.Fill = v;

									if (items.Length >= 3)
										element.FillOpacity = float.Parse(items[2], Global.CultureEN);
								}
								else if (items[0] == "line")
								{
									int v = Int32.Parse(items[1], Global.CultureEN);

									element.Line = v;

									if (items.Length >= 3)
										element.LineOpacity = float.Parse(items[2], Global.CultureEN);
								}
								else if (items[0] == "stroke")
								{
									float v = float.Parse(items[1], Global.CultureEN);

									element.LineWidth = v;
								}
								else if (items[0] == "hardness")
								{
									float v = float.Parse(items[1], Global.CultureEN);

									element.LineHardness = v;
								}
								else if (items[0] == "visible")
								{
									bool v = items[1] == "1";

									element.Visible = v;
								}
								else if (items[0] == "filled")
								{
									bool v = items[1] == "1";

									element.Filled = v;
								}
								else if (items[0] == "collision")
								{
									bool v = items[1] == "1";

									element.Collision = v;
								}
								else
								{
									if (element.GetType() == typeof(VectorCircle))
									{
										VectorCircle circle = element as VectorCircle;

										if (items[0] == "position")
										{
											circle.m_Location.X = Int32.Parse(items[1], Global.CultureEN);
											circle.m_Location.Y = Int32.Parse(items[2], Global.CultureEN);
										}
										else if (items[0] == "radius")
										{
											circle.m_Radius = Int32.Parse(items[1], Global.CultureEN);
										}
										else
										{
											throw new Exception(String.Format("unknown property: {0}", items[0]));
										}
									}

									if (element.GetType() == typeof(VectorPoly))
									{
										VectorPoly poly = element as VectorPoly;

										if (items[0] == "point")
										{
											PolyPoint point = new PolyPoint(poly);

											point.X = Int32.Parse(items[1], Global.CultureEN);
											point.Y = Int32.Parse(items[2], Global.CultureEN);

											poly.AddPoint(point);
										}
										else
										{
											throw new Exception(String.Format("unknown property: {0}", items[0]));
										}
									}

                                    if (element.GetType() == typeof(VectorRect))
                                    {
                                        VectorRect rect = element as VectorRect;

                                        if (items[0] == "position")
                                        {
                                            rect.m_Location.X = Int32.Parse(items[1], Global.CultureEN);
                                            rect.m_Location.Y = Int32.Parse(items[2], Global.CultureEN);
                                        }
                                        else if (items[0] == "size")
                                        {
                                            rect.m_Size.X = Int32.Parse(items[1], Global.CultureEN);
                                            rect.m_Size.Y = Int32.Parse(items[2], Global.CultureEN);
                                        }
                                        else
                                        {
                                            throw new Exception(String.Format("unknown property: {0}", items[0]));
                                        }
                                    }

									if (element.GetType() == typeof(VectorPicture))
									{
										VectorPicture picture = element as VectorPicture;

										if (items[0] == "position")
										{
											picture.m_Location.X = Int32.Parse(items[1], Global.CultureEN);
											picture.m_Location.Y = Int32.Parse(items[2], Global.CultureEN);
										}
										else if (items[0] == "path")
										{
											picture.FileName = new FileName(items[1]);
										}
                                        else if (items[0] == "content_scale")
                                        {
                                            picture.ContentScale = Int32.Parse(items[1], Global.CultureEN);
                                        }
                                        else
                                        {
                                            throw new Exception(String.Format("unknown property: {0}", items[0]));
                                        }
									}

									if (element.GetType() == typeof(VectorTag))
									{
										VectorTag tag = element as VectorTag;

										if (items[0] == "position")
										{
											Point point = new Point();

											point.X = Int32.Parse(items[1], Global.CultureEN);
											point.Y = Int32.Parse(items[2], Global.CultureEN);

											tag.m_Location = point;
										}
									}
								}
							}
						}
					}
				}
			}

			public void Save(String fileName)
			{
				using (TextWriter writer = new StreamWriter(new FileStream(fileName, FileMode.Create)))
				{
					writer.WriteLine("graphic");
					writer.WriteLine("\thas_silhouette {0}", m_Attributes.HasSilhouette ? "1" : "0");
					writer.WriteLine("\thas_sprite {0}", m_Attributes.HasSprite ? "1" : "0");

					writer.WriteLine("editor");
					writer.WriteLine("\tcanvas_width {0}", m_Attributes.Width);
					writer.WriteLine("\tcanvas_height {0}", m_Attributes.Height);
					foreach (VectorElement element in m_Elements)
					{
						String typeName = String.Empty;

						// Write type name.

                        if (element.GetType() == typeof(VectorCircle))
                            typeName = "circle";
                        else if (element.GetType() == typeof(VectorPoly))
                            typeName = "poly";
                        else if (element.GetType() == typeof(VectorRect))
                            typeName = "rect";
                        else if (element.GetType() == typeof(VectorTag))
                            typeName = "tag";
                        else if (element.GetType() == typeof(VectorPicture))
                            typeName = "picture";
                        else
                            throw new Exception("unknown type");

						writer.WriteLine(typeName);

						// Write shared stuff.

						writer.WriteLine("\tname {0}", element.Name);
						if (element.Group != String.Empty)
							writer.WriteLine("\tgroup {0}", element.Group);
						writer.WriteLine("\tcolor {0} {1} {2}", element.VecColor.R, element.VecColor.G, element.VecColor.B);
						writer.WriteLine("\tline {0} {1}", element.Line, element.LineOpacity);
						writer.WriteLine("\tfill {0} {1}", element.Fill, element.FillOpacity);
						writer.WriteLine("\tstroke {0}", element.LineWidth);
						writer.WriteLine("\thardness {0}", element.LineHardness);
						writer.WriteLine("\tvisible {0}", element.Visible ? "1" : "0");
						writer.WriteLine("\tfilled {0}", element.Filled ? "1" : "0");
						writer.WriteLine("\tcollision {0}", element.Collision ? "1" : "0");

						// Write type specific stuff.

						if (element.GetType() == typeof(VectorCircle))
						{
							VectorCircle circle = (VectorCircle)element;

							writer.WriteLine("\tposition {0} {1}", circle.m_Location.X, circle.m_Location.Y);
							writer.WriteLine("\tradius {0}", circle.m_Radius);
						}
						else if (element.GetType() == typeof(VectorPoly))
						{
							VectorPoly poly = (VectorPoly)element;

							foreach (Point point in poly.Points)
								writer.WriteLine("\tpoint {0} {1}", point.X, point.Y);
						}
						else if (element.GetType() == typeof(VectorTag))
						{
							VectorTag tag = (VectorTag)element;

							writer.WriteLine("\tposition {0} {1}", tag.m_Location.X, tag.m_Location.Y);
						}
                        else if (element.GetType() == typeof(VectorRect))
                        {
                            VectorRect rect = (VectorRect)element;

                            writer.WriteLine("\tposition {0} {1}", rect.m_Location.X, rect.m_Location.Y);
                            writer.WriteLine("\tsize {0} {1}", rect.m_Size.X, rect.m_Size.Y);
                        }
                        else if (element.GetType() == typeof(VectorPicture))
                        {
                            VectorPicture picture = (VectorPicture)element;

                            String path = PathEx.GetRelativePath(Path.GetDirectoryName(fileName), Path.GetFullPath(picture.FileName.Path));

                            writer.WriteLine("\tposition {0} {1}", picture.m_Location.X, picture.m_Location.Y);
                            writer.WriteLine("\tpath {0}", path);
                            writer.WriteLine("\tcontent_scale {0}", picture.ContentScale);
                        }
                        else
                            throw new Exception("unknown type");
					}
				}
			}

			public GraphicAttributes m_Attributes = new GraphicAttributes();
			public List<VectorElement> m_Elements = new List<VectorElement>();
		}

		#endregion
	}
}

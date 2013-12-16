using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Linq;
using System.Text;
using vecdraw;
using vecdraw.Elements;

namespace veccompose
{
	[Flags]
	public enum LinkFlags
	{
		Draw_FlipHorizontally = 1 << 0,
		Draw_FlipVertically = 1 << 1,
		Behaviour_AngleSpin = 1 << 2,
		Animation_Thruster = 1 << 6,
		Defense_Shield = 1 << 8,
		Defense_Armour = 1 << 9,
		Weapon_Vulcan = 1 << 10,
		Weapon_Missile = 1 << 11,
		Weapon_Beam = 1 << 12,
        Weapon_BlueSpray = 1 << 13,
        Weapon_PurpleSpray = 1 << 14,
		Behaviour_AngleFollow = 2 << 16
	}

	public class Link : IEditable
	{
		public delegate void NameChangeBeginHandler(Link link, String newName);
		public delegate void NameChangeEndHandler(Link link);
		public delegate void ParentNameChangeBeginHandler(Link link, String newName);
		public delegate void ParentNameChangeEndHandler(Link link);
		public event NameChangeBeginHandler OnNameChangeBegin;
		public event NameChangeEndHandler OnNameChangeEnd;
		public event ParentNameChangeBeginHandler OnParentNameChangeBegin;
		public event ParentNameChangeEndHandler OnParentNameChangeEnd;

		public float HitRadius = 5.0f;

		public static float RadToDeg(float angle)
		{
			return angle * 180.0f / (float)Math.PI;
		}

		public delegate void VisualChangedHandler(Link link);
		public event VisualChangedHandler OnVisualChanged;

		public Link(Composition composition, String parentName)
		{
			m_Composition = composition;
			ParentName = parentName;
		}

		public bool InsideEditing(PointF point)
		{
			PointF delta = PointF.Subtract(m_EditingLocation, new SizeF(point.X, point.Y));

			float distance = (float)Math.Sqrt(delta.X * delta.X + delta.Y * delta.Y);

			return distance < HitRadius;
		}

		public bool IsVisible(int? level)
		{
			if (Parent != null)
				if (!Parent.IsVisible(level))
					return false;

			if (!level.HasValue)
				return true;

			if (level.Value >= MinLevel && level.Value <= MaxLevel)
				return true;

			return false;
		}

		private void ClampAngle()
		{
			if (Angle.Degrees < MinAngle.Degrees)
				Angle.Degrees = MinAngle.Degrees;
			if (Angle.Degrees > MaxAngle.Degrees)
				Angle.Degrees = MaxAngle.Degrees;
		}

		public void HandlePropertyChanged(DataInfo fi)
		{
			ClampAngle();

			if (fi.Name == "Name" || fi.Name == "ParentName" || fi.Name == "Location" || fi.Name == "BaseAngle" || fi.Name == "Angle" || fi.Name == "MinAngle" || fi.Name == "MaxAngle" || fi.Name == "ShapeName" || fi.Name == "Flags")
				if (OnVisualChanged != null)
					OnVisualChanged(this);
		}

		private String m_Name = String.Empty;
		[Property("Name")]
		public String Name
		{
			get
			{
				return m_Name;
			}
			set
			{
				if (OnNameChangeBegin != null)
					OnNameChangeBegin(this, value);

				m_Name = value;

				if (OnNameChangeEnd != null)
					OnNameChangeEnd(this);
			}
		}
		private String m_ParentName = String.Empty;
		[Property("Parent")]
		public String ParentName
		{
			get
			{
				return m_ParentName;
			}
			set
			{
				if (OnParentNameChangeBegin != null)
					OnParentNameChangeBegin(this, value);

				m_ParentName = value;

				if (OnParentNameChangeEnd != null)
					OnParentNameChangeEnd(this);
			}
		}
		[Property("Location")]
		public Point Location = new Point(0, 0);
		[Property("Base Angle")]
		public VecAngle BaseAngle = new VecAngle();
		[Property("Angle")]
		public VecAngle Angle = new VecAngle();
		[Property("Minimum Angle")]
		public VecAngle MinAngle = new VecAngle();
		[Property("Maximum Angle")]
		public VecAngle MaxAngle = new VecAngle();
		[Property("Angle Speed")]
		public VecAngle AngleSpeed = new VecAngle();
		[Property("Shape Name")]
		public String ShapeName = String.Empty;
		[Property("Minimum Level")]
		public int MinLevel = 0;
		[Property("Maximum Level")]
		public int MaxLevel = 1000;
		[Property("Flags")]
		public LinkFlags Flags = 0;

		public Link Parent
		{
			get
			{
				Link result;

				if (!m_Composition.m_LinksByName.TryGetValue(ParentName, out result))
					return null;

				return result;
			}
		}
		public List<Link> Children
		{
			get
			{
				List<Link> result;

				if (Name == String.Empty)
					return new List<Link>();

				if (!m_Composition.m_LinksByParentName.TryGetValue(Name, out result))
					return new List<Link>();

				return result;
			}
		}
		/*public Link m_Parent;
		public String ParentName
		{
			get
			{
				if (m_Parent == null)
					return String.Empty;
				else
					return m_Parent.Name;
			}
		}*/
		//public List<Link> m_Links = new List<Link>();
		public Composition m_Composition;

		public Matrix m_Matrix = new Matrix();
		public PointF m_EditingLocation = new PointF();
		public float m_GlobalAngle = 0.0f;

		public void AddChild(Link link)
		{
			link.ParentName = Name;

			m_Composition.AddLink(link);
		}

		public void TransformChildren()
		{
			m_GlobalAngle = BaseAngle.Degrees + Angle.Degrees;
			float angle = BaseAngle.Degrees + Angle.Degrees;

			Matrix matrix = new Matrix();
			matrix.Reset();
			if (Parent != null)
			{
				matrix.Multiply(Parent.m_Matrix);
				m_GlobalAngle += Parent.m_GlobalAngle;
			}
			matrix.Translate(Location.X, Location.Y);
			matrix.Rotate(angle);

			m_Matrix = matrix;

			foreach (Link link in Children)
				link.TransformChildren();

			//

			matrix = new Matrix();
			matrix.Reset();

			if (Parent != null)
				matrix = Parent.m_Matrix;

			PointF[] temp = new PointF[1];
			temp[0] = new PointF(Location.X, Location.Y);

			matrix.TransformPoints(temp);

			m_EditingLocation = temp[0];
		}
	}

	public class Pivot
	{
		public Point Location = new Point(0, 0);
	}

	public class ShapeLibraryItem
	{
		public ShapeLibraryItem()
		{
		}

		public ShapeLibraryItem(String fileName, String name, String group, String pivot)
		{
			m_FileName = fileName;
			m_Name = name;
			m_Group = group;
			m_Pivot = pivot;
		}

		public void Load()
		{
			m_Shape.Load(m_FileName);

			// filter elements by group

			List<VectorElement> todo = new List<VectorElement>();
			foreach (VectorElement element in m_Shape.m_Elements)
				if (element.Group != m_Group)
					todo.Add(element);
			foreach (VectorElement element in todo)
				m_Shape.RemoveElement(element);
		}

		public String m_FileName = String.Empty;
		public String m_Name = String.Empty;
		public String m_Group = String.Empty;
		public String m_Pivot = String.Empty;
		public Shape m_Shape = new Shape();
	}

	public class ShapeLibrary
	{
		public delegate void ItemAddedHandler(ShapeLibraryItem item);
		public delegate void ItemRemovedHandler(ShapeLibraryItem item);

		public event ItemAddedHandler OnItemAdded;
		public event ItemRemovedHandler OnItemRemoved;

		public void Load(String fileName)
		{
			Clear();

			using (Stream stream = File.Open(fileName, FileMode.Open))
			{
				using (StreamReader reader = new StreamReader(stream))
				{
					List<ShapeLibraryItem> items = new List<ShapeLibraryItem>();

					ShapeLibraryItem item = null;

					String line;

					while ((line = reader.ReadLine()) != null)
					{
                        if (line.Trim() == string.Empty)
                            continue;

						String[] lines = line.Trim().Split(new char[] { '\t', ' ' });

						if (line.StartsWith("\t"))
						{
							if (item == null)
								throw new Exception();

							if (lines[0] == "path")
							{
								item.m_FileName = lines[1];
							}
							else if (lines[0] == "name")
							{
								item.m_Name = lines[1];
							}
							else if (lines[0] == "group")
							{
								item.m_Group = lines[1];
							}
							else if (lines[0] == "pivot")
							{
								item.m_Pivot = lines[1];
							}
							else
							{
								throw new Exception();
							}
						}
						else
						{
							if (lines[0] == "item")
							{
								item = new ShapeLibraryItem();

								items.Add(item);
							}
							else
							{
								throw new Exception();
							}
						}
					}

					foreach (ShapeLibraryItem temp in items)
					{
						temp.Load();

						AddItem(temp);
					}
				}
			}
		}

		public void Save(String fileName)
		{
			using (Stream stream = File.Open(fileName, FileMode.Create))
			{
				using (StreamWriter writer = new StreamWriter(stream))
				{
					foreach (ShapeLibraryItem item in m_Items)
					{
						writer.WriteLine("item");
						writer.WriteLine("\tname {0}", item.m_Name);
						writer.WriteLine("\tpath {0}", item.m_FileName);
						if (item.m_Group != String.Empty)
							writer.WriteLine("\tgroup {0}", item.m_Group);
						if (item.m_Pivot != String.Empty)
							writer.WriteLine("\tpivot {0}", item.m_Pivot);
					}
				}
			}
		}

		public void AddItem(ShapeLibraryItem item)
		{
			m_Items.Add(item);
			m_ItemsByName[item.m_Name] = item;

			if (OnItemAdded != null)
				OnItemAdded(item);
		}

		public void RemoveItem(ShapeLibraryItem item)
		{
			m_Items.Remove(item);
			m_ItemsByName.Remove(item.m_Name);

			if (OnItemRemoved != null)
				OnItemRemoved(item);
		}

		public void Clear()
		{
			while (m_Items.Count > 0)
				RemoveItem(m_Items[0]);
		}

		public ShapeLibraryItem FindByName(String name)
		{
			ShapeLibraryItem result;

			if (!m_ItemsByName.TryGetValue(name, out result))
				return null;

			return result;
		}

		public List<ShapeLibraryItem> m_Items = new List<ShapeLibraryItem>();
		public Dictionary<String, ShapeLibraryItem> m_ItemsByName = new Dictionary<string, ShapeLibraryItem>();
	}

	public class Composition
	{
		public delegate void LinkAddedHandler(Link link);
		public delegate void LinkRemovedHandler(Link link);
        public delegate void VisualChangedHandler(Composition composition);
		public event LinkAddedHandler OnLinkAdded;
		public event LinkRemovedHandler OnLinkRemoved;
        public event VisualChangedHandler OnVisualChanged;

		public delegate bool ForEachHandler(Link link, object arg);

		public Composition()
		{
			New();
		}

		public void New()
		{
			Clear();

			Link link = new Link(this, String.Empty);

			link.Name = "root";

			AddLink(link);
		}

		public Link HitTest(PointF point, int? level, Link selectedLink)
		{
			int baseIndex;
			
			if (selectedLink != null)
				baseIndex = m_Links.IndexOf(selectedLink) + 1;
			else
				baseIndex = 0;

			for (int i = 0; i < m_Links.Count; ++i)
			{
				int index = (baseIndex + i) % m_Links.Count;

				Link link = m_Links[index];

				if (link.IsVisible(level))
					if (link.InsideEditing(point))
						return link;
			}

			return null;
		}

		public void Save(String fileName)
		{
			/*
			 library
				item <name> <path> <group>
			 link
				name
				parent
				item
			    position
			    angle
			    min_angle
			    max_angle
			    flags
			 */

			if (RootLink.Name != "root")
				throw new Exception(String.Format("Root link must be named 'root'. It's currently set to {0}.", RootLink.Name));

			// save shape library

			String libraryFileName = Path.ChangeExtension(fileName, "slib");

			m_ShapeLibrary.Save(libraryFileName);

			// save boss file

			using (FileStream stream = File.Open(fileName, FileMode.Create))
			{
				using (StreamWriter writer = new StreamWriter(stream))
				{
					// write links

					foreach (Link link in m_Links)
					{
						writer.WriteLine("link");
						if (link.Name != String.Empty)
							writer.WriteLine("\tname {0}", link.Name);
						if (link.ParentName != String.Empty)
							writer.WriteLine("\tparent {0}", link.ParentName);
						if (link.ShapeName != String.Empty)
							writer.WriteLine("\tshape {0}", link.ShapeName);
						writer.WriteLine("\tposition {0} {1}", link.Location.X, link.Location.Y);
						writer.WriteLine("\tbase_angle {0}", link.BaseAngle.Degrees);
						writer.WriteLine("\tangle {0}", link.Angle.Degrees);
						writer.WriteLine("\tmin_angle {0}", link.MinAngle.Degrees);
						writer.WriteLine("\tmax_angle {0}", link.MaxAngle.Degrees);
						writer.WriteLine("\tangle_speed {0}", link.AngleSpeed.Degrees);
						writer.WriteLine("\tmin_level {0}", link.MinLevel);
						writer.WriteLine("\tmax_level {0}", link.MaxLevel);
						writer.WriteLine("\tflags {0}", (int)link.Flags);
					}
				}
			}
		}

		enum Section
		{
			None,
			Link
		}

		public void Load(String fileName)
		{
			Clear();

			// load shape library

			String libraryFileName = Path.ChangeExtension(fileName, "slib");

			m_ShapeLibrary.Load(libraryFileName);

			// load boss file

			using (FileStream stream = File.Open(fileName, FileMode.Open))
			{
				using (StreamReader reader = new StreamReader(stream))
				{
					Section section = Section.None;

					String line;
					Link link = null;
					List<Link> links = new List<Link>();

					while ((line = reader.ReadLine()) != null)
					{
						if (line.Trim() == String.Empty)
							continue;

						String[] lines = line.Trim().Split(new char[] { '\t', ' ' });

						if (line.StartsWith("\t"))
						{
							if (section == Section.Link)
							{
								switch (lines[0])
								{
									case "name":
										link.Name = lines[1];
										break;
									case "parent":
										link.ParentName = lines[1];
										break;
									case "shape":
										link.ShapeName = lines[1];
										break;
									case "position":
										link.Location.X = Int32.Parse(lines[1]);
										link.Location.Y = Int32.Parse(lines[2]);
										break;
									case "base_angle":
										link.BaseAngle.Degrees = Int32.Parse(lines[1]);
										break;
									case "angle":
										link.Angle.Degrees = Int32.Parse(lines[1]);
										break;
									case "min_angle":
										link.MinAngle.Degrees = Int32.Parse(lines[1]);
										break;
									case "max_angle":
										link.MaxAngle.Degrees = Int32.Parse(lines[1]);
										break;
									case "angle_speed":
										link.AngleSpeed.Degrees = Int32.Parse(lines[1]);
										break;
									case "min_level":
									case "level":
										link.MinLevel = Int32.Parse(lines[1]);
										break;
									case "max_level":
										link.MaxLevel = Int32.Parse(lines[1]);
										break;
									case "flags":
										link.Flags = (LinkFlags)Int32.Parse(lines[1]);
										break;

									default:
										// todo
										throw new Exception("unknown paramter");
								}
							}
							else
							{
								// todo, message
								throw new Exception("Section: none");
							}
						}
						else
						{
							if (line == "link")
							{
								section = Section.Link;

								link = new Link(this, String.Empty);

								links.Add(link);
							}
							else
							{
								throw new Exception("unknown section");
							}
						}
					}

					foreach (Link temp in links)
						AddLink(temp);
				}
			}
		}

		private String MakeName()
		{
			for (int i = 1; true; ++i)
			{
				String name = String.Format("Link{0}", i);

				if (m_LinksByName.ContainsKey(name))
					continue;

				return name;
			}
		}

		private void CacheAdd(Link link)
		{
			m_LinksByName[link.Name] = link;
			if (!m_LinksByParentName.ContainsKey(link.ParentName))
				m_LinksByParentName[link.ParentName] = new List<Link>();
			m_LinksByParentName[link.ParentName].Add(link);
		}

		private void CacheRemove(Link link)
		{
			m_LinksByName.Remove(link.Name);
			m_LinksByParentName[link.ParentName].Remove(link);
		}

		private void HandleNameChangeBegin(Link link, String newName)
		{
			if (m_LinksByName.ContainsKey(newName))
				throw new Exception("A link with the same name already exists");

			CacheRemove(link);
		}

		private void HandleNameChangeEnd(Link link)
		{
			CacheAdd(link);
		}

		private void HandleParentNameChangeBegin(Link link, String newName)
		{
			//if (!m_LinksByName.ContainsKey(newName))
			//throw new Exception("Parent does not exist");

			CacheRemove(link);
		}

		private void HandleParentNameChangeEnd(Link link)
		{
			CacheAdd(link);
		}

		public void AddLink(Link link)
		{
			if (link.Name == String.Empty)
				link.Name = MakeName();

			m_Links.Add(link);

			CacheAdd(link);

			link.OnNameChangeBegin += HandleNameChangeBegin;
			link.OnNameChangeEnd += HandleNameChangeEnd;
			link.OnParentNameChangeBegin += HandleParentNameChangeBegin;
			link.OnParentNameChangeEnd += HandleParentNameChangeEnd;

			if (OnLinkAdded != null)
				OnLinkAdded(link);
		}

		public void RemoveLink(Link link)
		{
			//if (link.Parent == null)
			//	throw new Exception("Cannot remove root link.");

			if (OnLinkRemoved != null)
				OnLinkRemoved(link);

			link.OnNameChangeBegin -= HandleNameChangeBegin;
			link.OnNameChangeEnd -= HandleNameChangeEnd;
			link.OnParentNameChangeBegin -= HandleParentNameChangeBegin;
			link.OnParentNameChangeEnd -= HandleParentNameChangeEnd;

			m_Links.Remove(link);
			m_LinksByName.Remove(link.Name);
			m_LinksByParentName[link.ParentName].Remove(link);
		}

		public void Clear()
		{
			m_ShapeLibrary.Clear();

			while (m_Links.Count > 0)
				RemoveLink(m_Links[0]);
		}

        public void MoveUp(Link link)
        {
            m_Links.Remove(link);

            m_Links.Insert(m_Links.Count, link);

            if (OnVisualChanged != null)
                OnVisualChanged(this);
        }

        public void MoveDown(Link link)
        {
            m_Links.Remove(link);

            m_Links.Insert(0, link);

            if (OnVisualChanged != null)
                OnVisualChanged(this);
        }

		public Link RootLink
		{
			get
			{
				return m_LinksByName["root"];
			}
		}

		public ShapeLibrary m_ShapeLibrary = new ShapeLibrary();
		public List<Link> m_Links = new List<Link>();
		public Dictionary<String, Link> m_LinksByName = new Dictionary<string, Link>();
		public Dictionary<String, List<Link>> m_LinksByParentName = new Dictionary<string, List<Link>>();
		public Pivot m_Pivot = new Pivot();
	}
}

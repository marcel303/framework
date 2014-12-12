using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace ASCIIedit
{
	public partial class TextGrid : UserControl
	{
		class Coord : Tuple<int, int>
		{
			public Coord(int x, int y)
				: base(x, y)
			{
			}
		}

		private const int CSX = 32;
		private const int CSY = 32;

		private int m_sx = 15;
		private int m_sy = 10;

		Dictionary<Coord, char> m_values = new Dictionary<Coord, char>();

		private int m_selectionX = 0;
		private int m_selectionY = 0;

		//

		private int SelectionX
		{
			get
			{
				return m_selectionX;
			}
			set
			{
				m_selectionX = value;

				if (m_selectionX < 0)
					m_selectionX = 0;
				if (m_selectionX > m_sx - 1)
					m_selectionX = m_sx - 1;

				Invalidate();
			}
		}

		private int SelectionY
		{
			get
			{
				return m_selectionY;
			}
			set
			{
				m_selectionY = value;

				if (m_selectionY < 0)
					m_selectionY = 0;
				if (m_selectionY > m_sy - 1)
					m_selectionY = m_sy - 1;

				Invalidate();
			}
		}

		public int SX
		{
			get
			{
				return m_sx;
			}
		}

		public int SY
		{
			get
			{
				return m_sy;
			}
		}

		//

		public TextGrid()
		{
			InitializeComponent();
		}

		private void Form1_Paint(object sender, PaintEventArgs e)
		{
			Graphics g = e.Graphics;

			g.Clear(Color.Black);

			Brush selectionBrush = new SolidBrush(Color.FromArgb(40, 40, 40));

			Pen rectPen = new Pen(Color.FromArgb(100, 100, 100), 1.0f);

			Font textFont = new Font(FontFamily.GenericSansSerif, 16.0f, FontStyle.Regular, GraphicsUnit.Pixel);
			Brush textBrush = new SolidBrush(Color.White);
			StringFormat textFormat = new StringFormat();
			textFormat.Alignment = StringAlignment.Center;
			textFormat.LineAlignment = StringAlignment.Center;

			g.FillRectangle(selectionBrush, new Rectangle(SelectionX * CSX, SelectionY * CSY, CSX, CSY));

			for (int x = 0; x <= m_sx; ++x)
				g.DrawLine(rectPen, x * CSX, 0, x * CSX, m_sy * CSY);
			for (int y = 0; y <= m_sy; ++y)
				g.DrawLine(rectPen, 0, y * CSY, m_sx * CSX, y * CSY);

			for (int x = 0; x < m_sx; ++x)
			{
				for (int y = 0; y < m_sy; ++y)
				{
					char c;

					if (m_values.TryGetValue(new Coord(x, y), out c))
					{
						g.DrawString(c.ToString(), textFont, textBrush, new RectangleF(x * CSX, y * CSY, CSX, CSY), textFormat);
					}
				}
			}
		}

		private void Form1_Click(object sender, EventArgs e)
		{
			MouseEventArgs m = e as MouseEventArgs;

			if (m != null)
			{
				SelectionX = (m.X) / CSX;
				SelectionY = (m.Y) / CSY;
			}
		}

		protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
		{
			Keys key = keyData;

			if (key == Keys.Left)
				SelectionX -= 1;
			else if (key == Keys.Right)
				SelectionX += 1;
			else if (key == Keys.Up)
				SelectionY -= 1;
			else if (key == Keys.Down)
				SelectionY += 1;
			else if (key == Keys.Back)
			{
				SelectionX--;

				m_values.Remove(new Coord(SelectionX, SelectionY));
				Invalidate();
			}
			else if (key == Keys.Delete)
			{
				m_values.Remove(new Coord(SelectionX, SelectionY));
				Invalidate();

				SelectionX++;
			}

			return base.ProcessCmdKey(ref msg, keyData);
		}

		private void Form1_KeyPress(object sender, KeyPressEventArgs e)
		{
			char c = e.KeyChar;

			if (!char.IsWhiteSpace(c) && !char.IsControl(c))
			{
				m_values[new Coord(SelectionX, SelectionY)] = c;
				Invalidate();

				SelectionX++;
			}
		}

		public void Clear()
		{
			m_values.Clear();
			Invalidate();
		}

		public void SetSize(int sx, int sy)
		{
			m_sx = sx;
			m_sy = sy;

			SelectionX = SelectionX;
			SelectionY = SelectionY;

			Invalidate();
		}

		public new void Load(string filename)
		{
			try
			{
				using (StreamReader reader = new StreamReader(filename))
				{
					m_values.Clear();

					string line;

					List<string> lines = new List<string>();

					while ((line = reader.ReadLine()) != null)
					{
						lines.Add(line);
					}

					int sx = 0;
					int sy = 0;

					for (int i = 0; i < lines.Count; ++i)
					{
						if (lines[i].Length > sx)
							sx = lines[i].Length;
						if (lines[i].Length > 0)
							sy = i + 1;
					}

					SetSize(sx, sy);

					for (int y = 0; y < sy; ++y)
					{
						for (int x = 0; x < lines[y].Length; ++x)
						{
							m_values[new Coord(x, y)] = lines[y][x];
						}
					}

					Invalidate();
				}
			}
			catch (Exception e)
			{
				MessageBox.Show(e.Message);
			}
		}

		public void Save(string filename)
		{
			try
			{
				using (StreamWriter writer = new StreamWriter(filename))
				{
					for (int y = 0; y < m_sy; ++y)
					{
						StringBuilder sb = new StringBuilder();

						for (int x = 0; x < m_sx; ++x)
						{
							char c;

							if (m_values.TryGetValue(new Coord(x, y), out c))
							{
								sb.Append(c);
							}
							else
							{
								sb.Append(' ');
							}
						}

						writer.WriteLine(sb.ToString());
					}
				}
			}
			catch (Exception e)
			{
				MessageBox.Show(e.Message);
			}
		}
	}
}

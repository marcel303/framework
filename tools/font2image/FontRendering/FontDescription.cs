using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;
using FontShared;

namespace FontRendering
{
	public class FontDescription
	{
		public event EventHandler OnChange;

		public int TextureSize
		{
			get
			{
				return mTextureQuality;
			}
			set
			{
				mTextureQuality = value;

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
		}

		public int Quality
		{
			get
			{
				return mQuality;
			}
			set
			{
				mQuality = value;

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
		}

		public Font Font
		{
			get
			{
				return mFont;
			}
			set
			{
				mFont = value;

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
		}

		public Color ForeColor
		{
			get
			{
				return mForeColor;
			}
			set
			{
				mForeColor = value;

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
		}

		public Color BackColor
		{
			get
			{
				return mBackColor;
			}
			set
			{
				mBackColor = value;

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
		}

		public bool BackColorIsTransparent
		{
			get
			{
				return mBackColorIsTransparent;
			}
			set
			{
				mBackColorIsTransparent = value;

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
		}

		public string ExportFileName
		{
			get
			{
				return mExportFileName;
			}
			set
			{
				mExportFileName = value;

				//if (OnChange != null)
				//OnChange(this, new EventArgs());
			}
		}

		public int GlyphSpacing
		{
			get
			{
				return mGlyphSpacing;
			}
			set
			{
				mGlyphSpacing = value;

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
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

				if (OnChange != null)
					OnChange(this, new EventArgs());
			}
		}

		#region Private Member Variables

		private int mTextureQuality = 256;
		private int mQuality = 1;
		private Font mFont = new Font("Arial", 12.0f);
		private Color mForeColor = Color.White;
		private Color mBackColor = Color.Transparent;
		private bool mBackColorIsTransparent = true;
		private string mExportFileName = null;
		private int mGlyphSpacing = 8; // fixme, 1
		//private int mGlyphSpacing = 1;
		private IList<CustomChar> mCustomCharList = new List<CustomChar>();

		#endregion

		public FontDescription()
		{
			SetDefaults();
		}

		private void SetDefaults()
		{
			TextureSize = 512;
			Quality = 1;
			Font = new Font(FontFamily.GenericSansSerif, 12.0f, FontStyle.Regular);
			ForeColor = Color.White;
			BackColor = Color.Black;
			BackColorIsTransparent = true;
			ExportFileName = null;
			CustomCharList = new List<CustomChar>();
		}

		private static Color ReadColor(string text)
		{
			string[] rgba = text.Split(new char[] {'.'}, StringSplitOptions.RemoveEmptyEntries);

			if (rgba.Length != 4)
				throw new MyException("parse error: {0}", text);

			int r = int.Parse(rgba[0], X.CultureEN);
			int g = int.Parse(rgba[1], X.CultureEN);
			int b = int.Parse(rgba[2], X.CultureEN);
			int a = int.Parse(rgba[3], X.CultureEN);

			return Color.FromArgb(a, r, g, b);
		}

		private static CustomChar ReadCustomChar(string text)
		{
			string[] elems = text.Split(new char[] {'='}, StringSplitOptions.RemoveEmptyEntries);

			if (elems.Length == 2)
			{
				char code = (char)int.Parse(elems[0], X.CultureEN);
				int scalePct = 100;
				int offsetPct = 0;
				string fileName = elems[1];

				return new CustomChar(code, fileName, scalePct, offsetPct);
			}
			else if (elems.Length == 4)
			{
				char code = (char)int.Parse(elems[0], X.CultureEN);
				int scalePct = int.Parse(elems[1]);
				int offsetPct = int.Parse(elems[2]);
				string fileName = elems[3];

				return new CustomChar(code, fileName, scalePct, offsetPct);
			}
			else
			{
				throw new MyException("parse error: {0}", text);
			}
		}

		public void Load(string fileName)
		{
			SetDefaults();

			string[] lineList = File.ReadAllLines(fileName);

			string fontName = "Arial";
			float fontSize = 12.0f;
			bool fontIsBold = false;
			bool fontIsItalic = false;
			bool fontUseStrikeout = false;
			bool fontUseUnderline = false;
			List<CustomChar> customCharList = new List<CustomChar>();

			for (int i = 0; i < lineList.Length; ++i)
			{
				string line = lineList[i].Trim();

				if (line.StartsWith("#"))
					continue;

				int index = line.IndexOf(':');

				if (index < 0)
					throw new MyException("parse error: {0}", line);

				string[] kvp = new string[] { line.Substring(0, index), line.Substring(index + 1) };

				string key = kvp[0];
				string value = kvp[1];

				switch (key)
				{
					case "textureSize":
						TextureSize = int.Parse(value, X.CultureEN);
						break;
					case "quality":
						Quality = int.Parse(value, X.CultureEN);
						break;
					case "fontName":
						fontName = value;
						break;
					case "fontSize":
						fontSize = float.Parse(value, X.CultureEN);
						break;
					case "fontIsBold":
						fontIsBold = bool.Parse(value);
						break;
					case "fontIsItalic":
						fontIsItalic = bool.Parse(value);
						break;
					case "fontUseStrikeout":
						fontUseStrikeout = bool.Parse(value);
						break;
					case "fontUseUnderline":
						fontUseUnderline = bool.Parse(value);
						break;
					case "foreColor":
						{
							ForeColor = ReadColor(value);
							break;
						}
					case "backColor":
						{
							BackColor = ReadColor(value);
							break;
						}
					case "backColorIsTransparent":
						{
							BackColorIsTransparent = bool.Parse(value);
							break;
						}
					case "exportFileName":
						ExportFileName = value;
						break;
					case "customChar":
						{
							CustomChar customChar = ReadCustomChar(value);
							customCharList.Add(customChar);
							break;
						}
					default:
						throw new MyException("unknown key: {0}", key);
				}
			}

			FontStyle fontStyle = FontStyle.Regular;

			if (fontIsBold)
				fontStyle |= FontStyle.Bold;
			if (fontIsItalic)
				fontStyle |= FontStyle.Italic;
			if (fontUseStrikeout)
				fontStyle |= FontStyle.Strikeout;
			if (fontUseUnderline)
				fontStyle |= FontStyle.Underline;

			Font = new Font(fontName, fontSize, fontStyle);

			CustomCharList = customCharList;
		}

		private static void WriteKVP(List<string> lineList, string key, string value)
		{
			lineList.Add(string.Format("{0}:{1}", key, value));
		}

		public void Save(string fileName)
		{
			List<string> lineList = new List<string>();

			WriteKVP(lineList, "textureSize", TextureSize.ToString(X.CultureEN));
			WriteKVP(lineList, "quality", Quality.ToString(X.CultureEN));
			WriteKVP(lineList, "fontName", Font.Name);
			WriteKVP(lineList, "fontSize", Font.Size.ToString(X.CultureEN));
			WriteKVP(lineList, "fontIsBold", Font.Bold.ToString(X.CultureEN));
			WriteKVP(lineList, "fontIsItalic", Font.Italic.ToString(X.CultureEN));
			WriteKVP(lineList, "fontUseStrikeout", Font.Strikeout.ToString(X.CultureEN));
			WriteKVP(lineList, "fontUseUnderline", Font.Underline.ToString(X.CultureEN));
			WriteKVP(lineList, "foreColor", string.Format("{0}.{1}.{2}.{3}", ForeColor.R, ForeColor.G, ForeColor.B, ForeColor.A));
			WriteKVP(lineList, "backColor", string.Format("{0}.{1}.{2}.{3}", BackColor.R, BackColor.G, BackColor.B, BackColor.A));
			WriteKVP(lineList, "backColorIsTransparent", BackColorIsTransparent.ToString(X.CultureEN));
			if (!string.IsNullOrEmpty(ExportFileName))
				WriteKVP(lineList, "exportFileName", ExportFileName);
			foreach (CustomChar customChar in CustomCharList)
				WriteKVP(lineList, "customChar", string.Format("{0}={1}={2}={3}", (int)customChar.Code, customChar.ScalePct, customChar.OffsetPct, customChar.FileName));

			File.WriteAllLines(fileName, lineList.ToArray());

		}
	}
}

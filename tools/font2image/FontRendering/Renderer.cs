using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Text;
using System.Collections.Generic;

namespace FontRendering
{
	public static class Renderer
	{
		private static void HighQuality(Graphics g)
		{
			g.CompositingMode = CompositingMode.SourceOver;
			g.CompositingQuality = CompositingQuality.HighQuality;
			g.InterpolationMode = InterpolationMode.HighQualityBilinear;
			g.PixelOffsetMode = PixelOffsetMode.HighQuality;
			g.SmoothingMode = SmoothingMode.HighQuality;
			g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
		}

		public static Bitmap RenderFontToBitmap(FontDescription fontDescription, int cellSx, int cellSy)
		{
			return RenderFontToBitmap(
				fontDescription.Font.Name,
				fontDescription.Font.Size,
				fontDescription.Font.Style,
				cellSx,
				cellSy,
				fontDescription.Quality,
				fontDescription.BackColor,
				fontDescription.ForeColor,
				fontDescription.BackColorIsTransparent,
				fontDescription.GlyphSpacing,
				fontDescription.CustomCharList);
		}

		public static Bitmap RenderFontToBitmap(
			string fontName,
			float fontSize,
			FontStyle fontStyle,
			int cellSx,
			int cellSy,
			int quality,
			Color backColor,
			Color foreColor,
			bool backColorIsTransparent,
			int glyphSpacing,
			IList<CustomChar> customCharList)
		{
			Dictionary<char, CustomChar> customCharByCode = new Dictionary<char, CustomChar>();

			foreach (CustomChar customChar in customCharList)
				customCharByCode[customChar.Code] = customChar;

			//

			Bitmap bitmapHq = new Bitmap(
				16 * cellSx * quality,
				16 * cellSy * quality);

			Graphics gHq = Graphics.FromImage(bitmapHq);

			HighQuality(gHq);

			if (backColorIsTransparent)
				gHq.Clear(Color.Transparent);
			else
				gHq.Clear(backColor);

			Font font = new Font(fontName, fontSize * quality, fontStyle);
			Brush brush = new SolidBrush(foreColor);

			for (int x = 0; x < 16; ++x)
			{
				for (int y = 0; y < 16; ++y)
				{
					char c = (char)(x + y * 16);

					CustomChar customChar;

					if (customCharByCode.TryGetValue(c, out customChar))
					{
						try
						{
							using (Image image = Image.FromFile(customChar.FileName))
							{
								float offsetX = 0.0f;
								float offsetY = customChar.OffsetPct / 100.0f;
								float scale = customChar.ScalePct / 100.0f;

								PointF location = new PointF(
									((x + offsetX) * cellSx + glyphSpacing) * quality,
									((y + offsetY) * cellSy + glyphSpacing) * quality);

								SizeF size = new SizeF(
									cellSx * scale * quality,
									cellSy * scale * quality);

								RectangleF rect = new RectangleF(location, size);

								gHq.DrawImage(image, rect);
							}
						}
						catch
						{
						}
					}
					else
					{
						PointF location = new PointF(
							(x * cellSx + glyphSpacing) * quality,
							(y * cellSy + glyphSpacing) * quality);

						gHq.DrawString(c.ToString(), font, brush, location);
					}
				}
			}

			Bitmap bitmapLq = new Bitmap(
				16 * cellSx,
				16 * cellSy);

			Graphics gLq = Graphics.FromImage(bitmapLq);

			HighQuality(gLq);

			gLq.DrawImage(bitmapHq, new Rectangle(0, 0, 16 * cellSx, 16 * cellSy));

			bitmapHq.Dispose();

			return bitmapLq;
		}
	}
}

using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using FontRendering;
using FontShared;

namespace FontGen
{
	public partial class Form_Main : Form
	{
		#region Properties

		public string FileName
		{
			get
			{
				return mFileName;
			}
			set
			{
				mFileName = value;

				Text = string.Format("font2image: {0}", mFileName == null ? "(new document)" : mFileName);
			}
		}

		#endregion

		#region Private Member Variables

		private string mDataDirectory;
		private Mru mMru;
		private FontDescription mFontDescription;
		private bool mDisableUpdates = false;
		private string mFileName;

		#endregion

		#region Constructor

		public Form_Main()
		{
			InitializeComponent();

			SetupDataDirectory();

			mMru = new Mru(Path.Combine(mDataDirectory, "mru.txt"), 10);

			mDisableUpdates = true;

			CreateFontDescription();

			mDisableUpdates = false;

			string[] argumentList = Environment.GetCommandLineArgs();

			if (argumentList.Length >= 2)
			{
				FileLoad(argumentList.Last());
			}
			else
			{
				mDisableUpdates = true;

				CreateFontDescription();

				mDisableUpdates = false;

				HandleChange();
			}
		}

		#endregion

		#region Helper Methods

		private void SetupDataDirectory()
		{
			mDataDirectory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), "FontGen");

			if (!Directory.Exists(mDataDirectory))
				Directory.CreateDirectory(mDataDirectory);
		}

		private void CreateFontDescription()
		{
			mFontDescription = new FontDescription();

			mFontDescription.OnChange += HandleFontChanged;
		}

		private void HandleError(Exception e)
		{
			MessageBox.Show(e.Message);
		}

		private void HandleStatus(string text, params object[] args)
		{
			toolStripStatusLabel1.Text = string.Format("{0}: {1}", DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"), string.Format(text, args));
		}

		private void HandleFontChanged(object sender, EventArgs args)
		{
			mMenuSelectTextureSize.Text = mFontDescription.TextureSize.ToString(X.CultureEN);
			mMenuSelectQuality.SelectedItem = mFontDescription.Quality.ToString(X.CultureEN);
			mMenuSelectBackColorIsTransparent.SelectedIndex = mFontDescription.BackColorIsTransparent ? 1 : 0;

			if (!mDisableUpdates)
				HandleChange();
		}

		#endregion

		#region Rendering

		private Bitmap RenderFontToBitmap(bool drawGrid)
		{
			int cellSx = Math.Max(1, mFontDescription.TextureSize / 16);
			int cellSy = Math.Max(1, mFontDescription.TextureSize / 16);

			Bitmap bitmap = Renderer.RenderFontToBitmap(
				mFontDescription.Font.Name,
				mFontDescription.Font.Size,
				mFontDescription.Font.Style,
				cellSx,
				cellSy,
				mFontDescription.Quality,
				mFontDescription.BackColor,
				mFontDescription.ForeColor,
				mFontDescription.BackColorIsTransparent,
				mFontDescription.GlyphSpacing,
				mFontDescription.CustomCharList);

			if (drawGrid)
			{
				Graphics g = Graphics.FromImage(bitmap);

				Pen pen = new Pen(Color.Red);

				for (int i = 1; i < 16; ++i)
					g.DrawLine(pen, new Point(0, bitmap.Height / 16 * i), new Point(bitmap.Width, bitmap.Height / 16 * i));
				for (int i = 1; i < 16; ++i)
					g.DrawLine(pen, new Point(bitmap.Width / 16 * i, 0), new Point(bitmap.Width / 16 * i, bitmap.Width));
			}

			return bitmap;
		}

		private void HandleChange()
		{
			if (mImagePreview.Image != null)
				mImagePreview.Image.Dispose();

			Bitmap bitmap = RenderFontToBitmap(true);

			mImagePreview.Image = bitmap;

			mImagePreview.Size = bitmap.Size;

			GC.Collect();
		}

		#endregion

		#region Menu Actions

		private void SelectFont(object sender, EventArgs args)
		{
			try
			{
				fontDialog.Font = mFontDescription.Font;

				if (fontDialog.ShowDialog() != DialogResult.OK)
					return;

				mFontDescription.Font = fontDialog.Font;

				HandleStatus("Changed font: {0} @ {1} em",
					mFontDescription.Font.Name,
					mFontDescription.Font.Size.ToString("N2"));
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void SelectForeColor(object sender, EventArgs args)
		{
			try
			{
				colorDialog_ForeColor.Color = mFontDescription.ForeColor;

				if (colorDialog_ForeColor.ShowDialog() != DialogResult.OK)
					return;

				mFontDescription.ForeColor = colorDialog_ForeColor.Color;

				HandleStatus("Change foreground color: {0}", mFontDescription.ForeColor);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void SelectBackColor(object sender, EventArgs args)
		{
			try
			{
				colorDialog_BackColor.Color = mFontDescription.BackColor;

				if (colorDialog_BackColor.ShowDialog() != DialogResult.OK)
					return;

				mFontDescription.BackColor = colorDialog_BackColor.Color;

				HandleStatus("Changed background color: {0}", mFontDescription.BackColor);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void SelectBackColorIsTransparent(object sender, EventArgs args)
		{
			try
			{
				mFontDescription.BackColorIsTransparent = mMenuSelectBackColorIsTransparent.SelectedIndex == 1;

				HandleStatus("Changed back color transparency: {0}", mFontDescription.BackColorIsTransparent);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void SelectRenderingQuality(object sender, EventArgs args)
		{
			try
			{
				mFontDescription.Quality = Convert.ToInt32(mMenuSelectQuality.SelectedItem);

				HandleStatus("Changed rendering quality: {0}", mFontDescription.Quality);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void Exit(object sender, EventArgs e)
		{
			Application.Exit();
		}

		private void FileLoad(object sender, EventArgs args)
		{
			try
			{
				if (!string.IsNullOrEmpty(FileName))
					openFileDialog_Load.FileName = FileName;

				if (openFileDialog_Load.ShowDialog() != DialogResult.OK)
					return;

				FileLoad(openFileDialog_Load.FileName);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void FileSave(object sender, EventArgs args)
		{
			try
			{
				if (FileName == null)
				{
					SaveAs(sender, args);

					return;
				}

				Save();
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void SaveAs(object sender, EventArgs args)
		{
			try
			{
				if (!string.IsNullOrEmpty(FileName))
					saveFileDialog_SaveAs.FileName = FileName;

				saveFileDialog_SaveAs.OverwritePrompt = true;

				if (saveFileDialog_SaveAs.ShowDialog() != DialogResult.OK)
					return;

				FileName = saveFileDialog_SaveAs.FileName;

				Save();
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void About(object sender, EventArgs e)
		{
			using (Form_About form = new Form_About())
			{
				form.ShowDialog();
			}
		}

		private void SelectTextureSize(object sender, EventArgs args)
		{
			try
			{
				int textureSize;

				if (!int.TryParse(mMenuSelectTextureSize.Text, System.Globalization.NumberStyles.Integer, X.CultureEN, out textureSize))
					return;

				mFontDescription.TextureSize = textureSize;

				HandleStatus("Changed texture size: {0}", mFontDescription.TextureSize);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void Export(object sender, EventArgs args)
		{
			try
			{
				if (!string.IsNullOrEmpty(mFontDescription.ExportFileName))
					saveFileDialog_Export.FileName = mFontDescription.ExportFileName;

				if (saveFileDialog_Export.ShowDialog() != DialogResult.OK)
					return;

				mFontDescription.ExportFileName = saveFileDialog_Export.FileName;

				ImageFormat imageFormat = ImageFormat.Bmp;

				string extension = Path.GetExtension(mFontDescription.ExportFileName).ToLower();

				switch (extension)
				{
					case ".bmp":
						imageFormat = ImageFormat.Bmp;
						break;
					case ".jpg":
						imageFormat = ImageFormat.Jpeg;
						break;
					case ".png":
						imageFormat = ImageFormat.Png;
						break;

					default:
						throw new MyException("unknown image format: {0}", extension);
				}

				using (Bitmap bitmap = RenderFontToBitmap(false))
				{
					bitmap.Save(saveFileDialog_Export.FileName, imageFormat);
				}

				HandleStatus("Exported: {0}", mFontDescription.ExportFileName);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		#endregion

		#region Load

		private void FileLoad(string fileName)
		{
			try
			{
				FileName = null;

				mDisableUpdates = true;

				mFontDescription.Load(fileName);
			}
			finally
			{
				HandleFontChanged(fontDialog, new EventArgs());

				mDisableUpdates = false;

				HandleChange();

				FileName = fileName;

				mMru.AddOrUpdate(fileName);

				HandleStatus("Loaded: {0}", fileName);
			}
		}

		#endregion

		#region Save

		private void Save()
		{
			mMru.AddOrUpdate(FileName);

			mFontDescription.Save(FileName);

			HandleStatus("Saved: {0}", FileName);
		}

		#endregion

		#region MRU

		private void FileMenuClick(object sender, EventArgs args)
		{
			try
			{
				// update MRU list

				string[] mruList = mMru.MruList;

				mMenuOpenRecent.DropDownItems.Clear();

				foreach (string mru in mruList)
				{
					Console.WriteLine(mru);

					ToolStripItem item = mMenuOpenRecent.DropDownItems.Add(mru, null, HandleMruClick);

					item.Tag = mru;
				}
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		private void HandleMruClick(object sender, EventArgs args)
		{
			try
			{
				ToolStripItem item = (ToolStripItem)sender;

				string fileName = (string)item.Tag;

				FileLoad(fileName);
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		#endregion

		#region Paint (Checker Board)

		private void Form_Main_Paint(object sender, PaintEventArgs args)
		{
			try
			{
				Graphics g = args.Graphics;

				int size = 64;

				Brush[] brushList = new Brush[] { new SolidBrush(Color.FromArgb(20, 20, 20)), new SolidBrush(Color.FromArgb(30, 30, 30)) };

				int countX = Width / size + 1;
				int countY = Height / size + 1;

				for (int x = 0; x < countX; ++x)
				{
					for (int y = 0; y < countY; ++y)
					{
						int brushIndex = (x + y) % 2;

						g.FillRectangle(brushList[brushIndex], new Rectangle(x * size, y * size, size, size));
					}
				}
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}

		#endregion

		private void customGlyphsToolStripMenuItem_Click(object sender, EventArgs args)
		{
			try
			{
				using (Form_Custom form = new Form_Custom(mFontDescription.CustomCharList))
				{
					form.ShowDialog();
				}

				HandleChange();
			}
			catch (Exception e)
			{
				HandleError(e);
			}
		}
	}
}

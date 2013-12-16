namespace FontGen
{
    partial class Form_Main
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
			this.menuStrip = new System.Windows.Forms.MenuStrip();
			this.mMenuFile = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuLoad = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuSave = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuSaveAs = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuOpenRecent = new System.Windows.Forms.ToolStripMenuItem();
			this.mRUToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripSeparator();
			this.mMenuExport = new System.Windows.Forms.ToolStripMenuItem();
			this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator();
			this.mMenuExit = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuFont = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuBackColor = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuForeColor = new System.Windows.Forms.ToolStripMenuItem();
			this.mMenuSelectBackColorIsTransparent = new System.Windows.Forms.ToolStripComboBox();
			this.mMenuSelectTextureSize = new System.Windows.Forms.ToolStripComboBox();
			this.mMenuSelectQuality = new System.Windows.Forms.ToolStripComboBox();
			this.mMenuAbout = new System.Windows.Forms.ToolStripMenuItem();
			this.fontDialog = new System.Windows.Forms.FontDialog();
			this.openFileDialog_Load = new System.Windows.Forms.OpenFileDialog();
			this.saveFileDialog_Export = new System.Windows.Forms.SaveFileDialog();
			this.colorDialog_ForeColor = new System.Windows.Forms.ColorDialog();
			this.colorDialog_BackColor = new System.Windows.Forms.ColorDialog();
			this.mImagePreview = new System.Windows.Forms.PictureBox();
			this.saveFileDialog_SaveAs = new System.Windows.Forms.SaveFileDialog();
			this.mStatusStrip = new System.Windows.Forms.StatusStrip();
			this.toolStripStatusLabel1 = new System.Windows.Forms.ToolStripStatusLabel();
			this.customGlyphsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			this.menuStrip.SuspendLayout();
			((System.ComponentModel.ISupportInitialize)(this.mImagePreview)).BeginInit();
			this.mStatusStrip.SuspendLayout();
			this.SuspendLayout();
			// 
			// menuStrip
			// 
			this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mMenuFile,
            this.mMenuFont,
            this.mMenuBackColor,
            this.mMenuForeColor,
            this.mMenuSelectBackColorIsTransparent,
            this.mMenuSelectTextureSize,
            this.mMenuSelectQuality,
            this.customGlyphsToolStripMenuItem,
            this.mMenuAbout});
			this.menuStrip.Location = new System.Drawing.Point(0, 0);
			this.menuStrip.Name = "menuStrip";
			this.menuStrip.Size = new System.Drawing.Size(890, 27);
			this.menuStrip.TabIndex = 0;
			this.menuStrip.Text = "menuStrip1";
			// 
			// mMenuFile
			// 
			this.mMenuFile.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mMenuLoad,
            this.mMenuSave,
            this.mMenuSaveAs,
            this.mMenuOpenRecent,
            this.toolStripMenuItem1,
            this.mMenuExport,
            this.toolStripMenuItem2,
            this.mMenuExit});
			this.mMenuFile.Name = "mMenuFile";
			this.mMenuFile.Size = new System.Drawing.Size(37, 23);
			this.mMenuFile.Text = "&File";
			this.mMenuFile.Click += new System.EventHandler(this.FileMenuClick);
			// 
			// mMenuLoad
			// 
			this.mMenuLoad.Name = "mMenuLoad";
			this.mMenuLoad.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.L)));
			this.mMenuLoad.Size = new System.Drawing.Size(190, 22);
			this.mMenuLoad.Text = "&Load";
			this.mMenuLoad.Click += new System.EventHandler(this.FileLoad);
			// 
			// mMenuSave
			// 
			this.mMenuSave.Name = "mMenuSave";
			this.mMenuSave.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
			this.mMenuSave.Size = new System.Drawing.Size(190, 22);
			this.mMenuSave.Text = "&Save";
			this.mMenuSave.Click += new System.EventHandler(this.FileSave);
			// 
			// mMenuSaveAs
			// 
			this.mMenuSaveAs.Name = "mMenuSaveAs";
			this.mMenuSaveAs.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift)
						| System.Windows.Forms.Keys.S)));
			this.mMenuSaveAs.Size = new System.Drawing.Size(190, 22);
			this.mMenuSaveAs.Text = "Save &as..";
			this.mMenuSaveAs.Click += new System.EventHandler(this.SaveAs);
			// 
			// mMenuOpenRecent
			// 
			this.mMenuOpenRecent.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mRUToolStripMenuItem});
			this.mMenuOpenRecent.Name = "mMenuOpenRecent";
			this.mMenuOpenRecent.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.R)));
			this.mMenuOpenRecent.Size = new System.Drawing.Size(190, 22);
			this.mMenuOpenRecent.Text = "Open &recent";
			// 
			// mRUToolStripMenuItem
			// 
			this.mRUToolStripMenuItem.Name = "mRUToolStripMenuItem";
			this.mRUToolStripMenuItem.Size = new System.Drawing.Size(100, 22);
			this.mRUToolStripMenuItem.Text = "MRU";
			// 
			// toolStripMenuItem1
			// 
			this.toolStripMenuItem1.Name = "toolStripMenuItem1";
			this.toolStripMenuItem1.Size = new System.Drawing.Size(187, 6);
			// 
			// mMenuExport
			// 
			this.mMenuExport.Name = "mMenuExport";
			this.mMenuExport.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.E)));
			this.mMenuExport.Size = new System.Drawing.Size(190, 22);
			this.mMenuExport.Text = "&Export..";
			this.mMenuExport.Click += new System.EventHandler(this.Export);
			// 
			// toolStripMenuItem2
			// 
			this.toolStripMenuItem2.Name = "toolStripMenuItem2";
			this.toolStripMenuItem2.Size = new System.Drawing.Size(187, 6);
			// 
			// mMenuExit
			// 
			this.mMenuExit.Name = "mMenuExit";
			this.mMenuExit.Size = new System.Drawing.Size(190, 22);
			this.mMenuExit.Text = "E&xit";
			this.mMenuExit.Click += new System.EventHandler(this.Exit);
			// 
			// mMenuFont
			// 
			this.mMenuFont.Name = "mMenuFont";
			this.mMenuFont.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.F)));
			this.mMenuFont.Size = new System.Drawing.Size(49, 23);
			this.mMenuFont.Text = "F&ont..";
			this.mMenuFont.Click += new System.EventHandler(this.SelectFont);
			// 
			// mMenuBackColor
			// 
			this.mMenuBackColor.Name = "mMenuBackColor";
			this.mMenuBackColor.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.B)));
			this.mMenuBackColor.Size = new System.Drawing.Size(79, 23);
			this.mMenuBackColor.Text = "&BackColor..";
			this.mMenuBackColor.Click += new System.EventHandler(this.SelectBackColor);
			// 
			// mMenuForeColor
			// 
			this.mMenuForeColor.Name = "mMenuForeColor";
			this.mMenuForeColor.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.F)));
			this.mMenuForeColor.Size = new System.Drawing.Size(77, 23);
			this.mMenuForeColor.Text = "&ForeColor..";
			this.mMenuForeColor.Click += new System.EventHandler(this.SelectForeColor);
			// 
			// mMenuSelectBackColorIsTransparent
			// 
			this.mMenuSelectBackColorIsTransparent.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.mMenuSelectBackColorIsTransparent.Items.AddRange(new object[] {
            "Solid",
            "Transparent"});
			this.mMenuSelectBackColorIsTransparent.Name = "mMenuSelectBackColorIsTransparent";
			this.mMenuSelectBackColorIsTransparent.Size = new System.Drawing.Size(121, 23);
			this.mMenuSelectBackColorIsTransparent.SelectedIndexChanged += new System.EventHandler(this.SelectBackColorIsTransparent);
			// 
			// mMenuSelectTextureSize
			// 
			this.mMenuSelectTextureSize.Items.AddRange(new object[] {
            "128",
            "256",
            "512",
            "1024",
            "2048",
            "4096"});
			this.mMenuSelectTextureSize.Name = "mMenuSelectTextureSize";
			this.mMenuSelectTextureSize.Size = new System.Drawing.Size(121, 23);
			this.mMenuSelectTextureSize.TextChanged += new System.EventHandler(this.SelectTextureSize);
			// 
			// mMenuSelectQuality
			// 
			this.mMenuSelectQuality.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.mMenuSelectQuality.Items.AddRange(new object[] {
            "1",
            "2",
            "4",
            "8"});
			this.mMenuSelectQuality.Name = "mMenuSelectQuality";
			this.mMenuSelectQuality.Size = new System.Drawing.Size(121, 23);
			this.mMenuSelectQuality.SelectedIndexChanged += new System.EventHandler(this.SelectRenderingQuality);
			// 
			// mMenuAbout
			// 
			this.mMenuAbout.Name = "mMenuAbout";
			this.mMenuAbout.Size = new System.Drawing.Size(52, 23);
			this.mMenuAbout.Text = "&About";
			this.mMenuAbout.Click += new System.EventHandler(this.About);
			// 
			// openFileDialog_Load
			// 
			this.openFileDialog_Load.FileName = "openFileDialog1";
			this.openFileDialog_Load.Filter = "Font files|*.txt.f2i";
			// 
			// saveFileDialog_Export
			// 
			this.saveFileDialog_Export.Filter = "BMP|*.bmp|JPG|*.jpg|PNG|*.png";
			// 
			// mImagePreview
			// 
			this.mImagePreview.BackColor = System.Drawing.Color.Transparent;
			this.mImagePreview.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
			this.mImagePreview.Location = new System.Drawing.Point(12, 30);
			this.mImagePreview.Name = "mImagePreview";
			this.mImagePreview.Size = new System.Drawing.Size(625, 413);
			this.mImagePreview.TabIndex = 1;
			this.mImagePreview.TabStop = false;
			// 
			// saveFileDialog_SaveAs
			// 
			this.saveFileDialog_SaveAs.Filter = "Font files|*.txt.f2i";
			// 
			// mStatusStrip
			// 
			this.mStatusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripStatusLabel1});
			this.mStatusStrip.Location = new System.Drawing.Point(0, 457);
			this.mStatusStrip.Name = "mStatusStrip";
			this.mStatusStrip.Size = new System.Drawing.Size(890, 22);
			this.mStatusStrip.TabIndex = 2;
			this.mStatusStrip.Text = "statusStrip1";
			// 
			// toolStripStatusLabel1
			// 
			this.toolStripStatusLabel1.Name = "toolStripStatusLabel1";
			this.toolStripStatusLabel1.Size = new System.Drawing.Size(118, 17);
			this.toolStripStatusLabel1.Text = "toolStripStatusLabel1";
			// 
			// customGlyphsToolStripMenuItem
			// 
			this.customGlyphsToolStripMenuItem.Name = "customGlyphsToolStripMenuItem";
			this.customGlyphsToolStripMenuItem.Size = new System.Drawing.Size(106, 23);
			this.customGlyphsToolStripMenuItem.Text = "Custom Glyphs..";
			this.customGlyphsToolStripMenuItem.Click += new System.EventHandler(this.customGlyphsToolStripMenuItem_Click);
			// 
			// Form_Main
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(890, 479);
			this.Controls.Add(this.mStatusStrip);
			this.Controls.Add(this.mImagePreview);
			this.Controls.Add(this.menuStrip);
			this.MainMenuStrip = this.menuStrip;
			this.Name = "Form_Main";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Form_Main";
			this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
			this.Paint += new System.Windows.Forms.PaintEventHandler(this.Form_Main_Paint);
			this.menuStrip.ResumeLayout(false);
			this.menuStrip.PerformLayout();
			((System.ComponentModel.ISupportInitialize)(this.mImagePreview)).EndInit();
			this.mStatusStrip.ResumeLayout(false);
			this.mStatusStrip.PerformLayout();
			this.ResumeLayout(false);
			this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip;
        private System.Windows.Forms.ToolStripMenuItem mMenuFile;
        private System.Windows.Forms.ToolStripMenuItem mMenuLoad;
        private System.Windows.Forms.ToolStripMenuItem mMenuSave;
        private System.Windows.Forms.ToolStripMenuItem mMenuSaveAs;
        private System.Windows.Forms.ToolStripMenuItem mMenuExport;
        private System.Windows.Forms.ToolStripMenuItem mMenuExit;
        private System.Windows.Forms.FontDialog fontDialog;
        private System.Windows.Forms.OpenFileDialog openFileDialog_Load;
        private System.Windows.Forms.SaveFileDialog saveFileDialog_Export;
        private System.Windows.Forms.ColorDialog colorDialog_ForeColor;
        private System.Windows.Forms.ColorDialog colorDialog_BackColor;
        private System.Windows.Forms.ToolStripMenuItem mMenuFont;
        private System.Windows.Forms.ToolStripMenuItem mMenuBackColor;
        private System.Windows.Forms.ToolStripMenuItem mMenuForeColor;
        private System.Windows.Forms.PictureBox mImagePreview;
        private System.Windows.Forms.ToolStripComboBox mMenuSelectQuality;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem1;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
        private System.Windows.Forms.SaveFileDialog saveFileDialog_SaveAs;
        private System.Windows.Forms.ToolStripMenuItem mMenuAbout;
        private System.Windows.Forms.ToolStripComboBox mMenuSelectTextureSize;
        private System.Windows.Forms.ToolStripMenuItem mMenuOpenRecent;
        private System.Windows.Forms.ToolStripMenuItem mRUToolStripMenuItem;
        private System.Windows.Forms.StatusStrip mStatusStrip;
        private System.Windows.Forms.ToolStripStatusLabel toolStripStatusLabel1;
        private System.Windows.Forms.ToolStripComboBox mMenuSelectBackColorIsTransparent;
		private System.Windows.Forms.ToolStripMenuItem customGlyphsToolStripMenuItem;
    }
}


namespace vecdraw
{
	partial class ColorButton
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

		#region Component Designer generated code

		/// <summary> 
		/// Required method for Designer support - do not modify 
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.colorDialog1 = new System.Windows.Forms.ColorDialog();
			this.SuspendLayout();
			// 
			// colorDialog1
			// 
			this.colorDialog1.AnyColor = true;
			this.colorDialog1.FullOpen = true;
			this.colorDialog1.SolidColorOnly = true;
			// 
			// ColorButton
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.Name = "ColorButton";
			this.Size = new System.Drawing.Size(43, 19);
			this.Paint += new System.Windows.Forms.PaintEventHandler(this.ColorButton_Paint);
			this.Click += new System.EventHandler(this.ColorButton_Click);
			this.Leave += new System.EventHandler(this.ColorButton_Leave);
			this.Enter += new System.EventHandler(this.ColorButton_Enter);
			this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.ColorButton_KeyDown);
			this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.ColorDialog colorDialog1;
	}
}

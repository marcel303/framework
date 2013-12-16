namespace vecdraw
{
	partial class VectorView
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
			this.SuspendLayout();
			// 
			// VectorView
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.BackColor = System.Drawing.SystemColors.ControlDark;
			this.BackgroundImageLayout = System.Windows.Forms.ImageLayout.None;
			this.DoubleBuffered = true;
			this.Name = "VectorView";
			this.Size = new System.Drawing.Size(434, 300);
			this.Paint += new System.Windows.Forms.PaintEventHandler(this.VectorView_Paint);
			this.MouseMove += new System.Windows.Forms.MouseEventHandler(this.VectorView_MouseMove);
			this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.VectorView_KeyUp);
			this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.VectorView_MouseDown);
			this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.VectorView_MouseUp);
			this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.VectorView_KeyDown);
			this.ResumeLayout(false);

		}

		#endregion
	}
}

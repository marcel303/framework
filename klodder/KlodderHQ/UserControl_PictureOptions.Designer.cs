namespace KlodderHQ
{
    partial class UserControl_PictureOptions
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
            this.label1 = new System.Windows.Forms.Label();
            this.userControl_FileSelect2 = new KlodderHQ.UserControl_FileSelect();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 3);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(79, 17);
            this.label1.TabIndex = 13;
            this.label1.Text = "Destination";
            // 
            // userControl_FileSelect2
            // 
            this.userControl_FileSelect2.AutoSize = true;
            this.userControl_FileSelect2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.userControl_FileSelect2.FileName = "";
            this.userControl_FileSelect2.Location = new System.Drawing.Point(128, 3);
            this.userControl_FileSelect2.Name = "userControl_FileSelect2";
            this.userControl_FileSelect2.Size = new System.Drawing.Size(374, 28);
            this.userControl_FileSelect2.TabIndex = 12;
            // 
            // UserControl_PictureOptions
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.userControl_FileSelect2);
            this.Controls.Add(this.label1);
            this.Name = "UserControl_PictureOptions";
            this.Size = new System.Drawing.Size(505, 34);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private UserControl_FileSelect userControl_FileSelect2;
        private System.Windows.Forms.Label label1;
    }
}

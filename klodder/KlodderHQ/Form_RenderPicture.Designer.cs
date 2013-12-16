namespace KlodderHQ
{
    partial class Form_RenderPicture
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
            this.button2 = new System.Windows.Forms.Button();
            this.button1 = new System.Windows.Forms.Button();
            this.userControl_PictureOptions1 = new KlodderHQ.UserControl_PictureOptions();
            this.userControl_RenderOptions1 = new KlodderHQ.UserControl_RenderOptions();
            this.SuspendLayout();
            // 
            // button2
            // 
            this.button2.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.button2.Location = new System.Drawing.Point(93, 214);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(75, 31);
            this.button2.TabIndex = 1;
            this.button2.Text = "Cancel";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(12, 214);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 31);
            this.button1.TabIndex = 0;
            this.button1.Text = "Render";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // userControl_PictureOptions1
            // 
            this.userControl_PictureOptions1.AutoSize = true;
            this.userControl_PictureOptions1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.userControl_PictureOptions1.Destination = "";
            this.userControl_PictureOptions1.Location = new System.Drawing.Point(12, 174);
            this.userControl_PictureOptions1.Name = "userControl_PictureOptions1";
            this.userControl_PictureOptions1.Size = new System.Drawing.Size(505, 34);
            this.userControl_PictureOptions1.TabIndex = 3;
            // 
            // userControl_RenderOptions1
            // 
            this.userControl_RenderOptions1.AutoSize = true;
            this.userControl_RenderOptions1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.userControl_RenderOptions1.BackColor1 = System.Drawing.Color.FromArgb(((int)(((byte)(230)))), ((int)(((byte)(230)))), ((int)(((byte)(230)))));
            this.userControl_RenderOptions1.BackColor2 = System.Drawing.Color.FromArgb(((int)(((byte)(204)))), ((int)(((byte)(205)))), ((int)(((byte)(205)))));
            this.userControl_RenderOptions1.Location = new System.Drawing.Point(12, 12);
            this.userControl_RenderOptions1.Name = "userControl_RenderOptions1";
            this.userControl_RenderOptions1.Preview = true;
            this.userControl_RenderOptions1.RenderScale = 1;
            this.userControl_RenderOptions1.Size = new System.Drawing.Size(505, 156);
            this.userControl_RenderOptions1.Source = "";
            this.userControl_RenderOptions1.TabIndex = 2;
            // 
            // Form_RenderPicture
            // 
            this.AcceptButton = this.button1;
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.button2;
            this.ClientSize = new System.Drawing.Size(619, 261);
            this.Controls.Add(this.userControl_PictureOptions1);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.userControl_RenderOptions1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "Form_RenderPicture";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Render picture";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button button2;
        private System.Windows.Forms.Button button1;
        private UserControl_RenderOptions userControl_RenderOptions1;
        private UserControl_PictureOptions userControl_PictureOptions1;
    }
}
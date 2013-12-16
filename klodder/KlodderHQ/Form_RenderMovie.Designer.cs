namespace KlodderHQ
{
    partial class Form_RenderMovie
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
            this.button1 = new System.Windows.Forms.Button();
            this.button2 = new System.Windows.Forms.Button();
            this.userControl_MovieOptions1 = new KlodderHQ.UserControl_MovieOptions();
            this.userControl_RenderOptions1 = new KlodderHQ.UserControl_RenderOptions();
            this.SuspendLayout();
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(12, 338);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(75, 31);
            this.button1.TabIndex = 0;
            this.button1.Text = "Render";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // button2
            // 
            this.button2.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.button2.Location = new System.Drawing.Point(93, 338);
            this.button2.Name = "button2";
            this.button2.Size = new System.Drawing.Size(75, 31);
            this.button2.TabIndex = 1;
            this.button2.Text = "Cancel";
            this.button2.UseVisualStyleBackColor = true;
            this.button2.Click += new System.EventHandler(this.button2_Click);
            // 
            // userControl_MovieOptions1
            // 
            this.userControl_MovieOptions1.AutoSize = true;
            this.userControl_MovieOptions1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.userControl_MovieOptions1.Destination = "";
            this.userControl_MovieOptions1.FrameRate = 25;
            this.userControl_MovieOptions1.Location = new System.Drawing.Point(12, 174);
            this.userControl_MovieOptions1.Name = "userControl_MovieOptions1";
            this.userControl_MovieOptions1.PauseFrames = 25;
            this.userControl_MovieOptions1.Size = new System.Drawing.Size(505, 158);
            this.userControl_MovieOptions1.TabIndex = 3;
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
            // Form_RenderMovie
            // 
            this.AcceptButton = this.button1;
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Control;
            this.CancelButton = this.button2;
            this.ClientSize = new System.Drawing.Size(619, 384);
            this.Controls.Add(this.button2);
            this.Controls.Add(this.userControl_MovieOptions1);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.userControl_RenderOptions1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.MaximizeBox = false;
            this.Name = "Form_RenderMovie";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Render Movie";
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private UserControl_RenderOptions userControl_RenderOptions1;
        private System.Windows.Forms.Button button1;
        private UserControl_MovieOptions userControl_MovieOptions1;
        private System.Windows.Forms.Button button2;
    }
}
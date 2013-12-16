namespace KlodderHQ
{
    partial class UserControl_RenderOptions
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
            this.trackBar1 = new System.Windows.Forms.TrackBar();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.checkBox1 = new System.Windows.Forms.CheckBox();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.userControl_FileSelect1 = new KlodderHQ.UserControl_FileSelect();
            this.userControl_ColorButton2 = new KlodderHQ.UserControl_ColorButton();
            this.userControl_ColorButton1 = new KlodderHQ.UserControl_ColorButton();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).BeginInit();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(3, 11);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(119, 17);
            this.label1.TabIndex = 2;
            this.label1.Text = "Background color";
            // 
            // trackBar1
            // 
            this.trackBar1.Location = new System.Drawing.Point(128, 40);
            this.trackBar1.Maximum = 4;
            this.trackBar1.Minimum = 1;
            this.trackBar1.Name = "trackBar1";
            this.trackBar1.Size = new System.Drawing.Size(104, 56);
            this.trackBar1.TabIndex = 2;
            this.trackBar1.TickStyle = System.Windows.Forms.TickStyle.TopLeft;
            this.trackBar1.Value = 1;
            this.trackBar1.ValueChanged += new System.EventHandler(this.trackBar1_ValueChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(3, 40);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(43, 17);
            this.label2.TabIndex = 4;
            this.label2.Text = "Scale";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(3, 125);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(53, 17);
            this.label3.TabIndex = 5;
            this.label3.Text = "Source";
            // 
            // checkBox1
            // 
            this.checkBox1.AutoSize = true;
            this.checkBox1.Checked = true;
            this.checkBox1.CheckState = System.Windows.Forms.CheckState.Checked;
            this.checkBox1.Location = new System.Drawing.Point(128, 102);
            this.checkBox1.Name = "checkBox1";
            this.checkBox1.Size = new System.Drawing.Size(18, 17);
            this.checkBox1.TabIndex = 5;
            this.checkBox1.UseVisualStyleBackColor = true;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(3, 101);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(57, 17);
            this.label5.TabIndex = 10;
            this.label5.Text = "Preview";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(238, 40);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(43, 17);
            this.label6.TabIndex = 12;
            this.label6.Text = "Scale";
            // 
            // userControl_FileSelect1
            // 
            this.userControl_FileSelect1.AutoSize = true;
            this.userControl_FileSelect1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.userControl_FileSelect1.Enabled = false;
            this.userControl_FileSelect1.FileName = "";
            this.userControl_FileSelect1.Location = new System.Drawing.Point(128, 125);
            this.userControl_FileSelect1.Name = "userControl_FileSelect1";
            this.userControl_FileSelect1.Size = new System.Drawing.Size(374, 28);
            this.userControl_FileSelect1.TabIndex = 3;
            // 
            // userControl_ColorButton2
            // 
            this.userControl_ColorButton2.AutoSize = true;
            this.userControl_ColorButton2.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.userControl_ColorButton2.Location = new System.Drawing.Point(165, 3);
            this.userControl_ColorButton2.Name = "userControl_ColorButton2";
            this.userControl_ColorButton2.Size = new System.Drawing.Size(31, 31);
            this.userControl_ColorButton2.TabIndex = 1;
            this.userControl_ColorButton2.Value = System.Drawing.Color.White;
            // 
            // userControl_ColorButton1
            // 
            this.userControl_ColorButton1.AutoSize = true;
            this.userControl_ColorButton1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.userControl_ColorButton1.Location = new System.Drawing.Point(128, 3);
            this.userControl_ColorButton1.Name = "userControl_ColorButton1";
            this.userControl_ColorButton1.Size = new System.Drawing.Size(31, 31);
            this.userControl_ColorButton1.TabIndex = 0;
            this.userControl_ColorButton1.Value = System.Drawing.Color.White;
            // 
            // UserControl_RenderOptions
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 16F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.Controls.Add(this.label6);
            this.Controls.Add(this.checkBox1);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.userControl_FileSelect1);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.trackBar1);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.userControl_ColorButton2);
            this.Controls.Add(this.userControl_ColorButton1);
            this.Name = "UserControl_RenderOptions";
            this.Size = new System.Drawing.Size(505, 156);
            ((System.ComponentModel.ISupportInitialize)(this.trackBar1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private UserControl_ColorButton userControl_ColorButton1;
        private UserControl_ColorButton userControl_ColorButton2;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TrackBar trackBar1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private UserControl_FileSelect userControl_FileSelect1;
        private System.Windows.Forms.CheckBox checkBox1;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label6;

    }
}

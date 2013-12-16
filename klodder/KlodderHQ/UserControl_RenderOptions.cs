using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace KlodderHQ
{
    public partial class UserControl_RenderOptions : UserControl
    {
        [Browsable(false)]
        public string Source
        {
            get
            {
                return userControl_FileSelect1.FileName;
            }
            set
            {
                userControl_FileSelect1.FileName = value;
            }
        }

        [Browsable(false)]
        public Color BackColor1
        {
            get
            {
                return userControl_ColorButton1.Value;
            }
            set
            {
                userControl_ColorButton1.Value = value;
            }
        }

        [Browsable(false)]
        public Color BackColor2
        {
            get
            {
                return userControl_ColorButton2.Value;
            }
            set
            {
                userControl_ColorButton2.Value = value;
            }
        }

        [Browsable(false)]
        public int RenderScale
        {
            get
            {
                return trackBar1.Value;
            }
            set
            {
                if (value < trackBar1.Minimum)
                    value = trackBar1.Minimum;
                if (value > trackBar1.Maximum)
                    value = trackBar1.Maximum;

                trackBar1.Value = value;

                label6.Text = string.Format("{0} X", value);
            }
        }

        [Browsable(false)]
        public bool Preview
        {
            get
            {
                return checkBox1.Checked;
            }
            set
            {
                checkBox1.Checked = value;
            }
        }

        public UserControl_RenderOptions()
        {
            InitializeComponent();

            BackColor1 = Color.FromArgb(255, 230, 230, 230);
            BackColor2 = Color.FromArgb(255, 204, 205, 205);
            RenderScale = 1;
            Preview = true;
        }

        private void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            RenderScale = trackBar1.Value;
        }
    }
}

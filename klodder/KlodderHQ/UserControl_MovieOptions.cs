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
    public partial class UserControl_MovieOptions : UserControl
    {
        [Browsable(false)]
        public string Destination
        {
            get
            {
                return userControl_FileSelect2.FileName;
            }
            set
            {
                userControl_FileSelect2.FileName = value;
            }
        }

        [Browsable(false)]
        public int FrameRate
        {
            get
            {
                return trackBar1.Value;
            }
            set
            {
                trackBar1.Value = value;

                label5.Text = string.Format("{0} FPS", value);
            }
        }

        [Browsable(false)]
        public int PauseFrames
        {
            get
            {
                return trackBar2.Value;
            }
            set
            {
                trackBar2.Value = value;

                label6.Text = string.Format("{0} frames", value);
            }
        }

        public UserControl_MovieOptions()
        {
            InitializeComponent();

            FrameRate = 25;
            PauseFrames = 25;
        }

        private void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            FrameRate = trackBar1.Value;
        }

        private void trackBar2_ValueChanged(object sender, EventArgs e)
        {
            PauseFrames = trackBar2.Value;
        }
    }
}

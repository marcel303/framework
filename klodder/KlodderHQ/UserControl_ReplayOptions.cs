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
    public partial class UserControl_ReplayOptions : UserControl
    {
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

        public UserControl_ReplayOptions()
        {
            InitializeComponent();
        }

        private void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            FrameRate = trackBar1.Value;
        }
    }
}

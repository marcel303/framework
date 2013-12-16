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
    public partial class UserControl_PictureOptions : UserControl
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

        public UserControl_PictureOptions()
        {
            InitializeComponent();
        }
    }
}

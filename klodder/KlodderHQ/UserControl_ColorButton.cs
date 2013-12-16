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
    public partial class UserControl_ColorButton : UserControl
    {
        private Color mValue;

        public Color Value
        {
            get
            {
                return mValue;
            }
            set
            {
                mValue = value;

                button1.BackColor = value;
            }
        }

        public UserControl_ColorButton()
        {
            InitializeComponent();

            Value = Color.White;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            colorDialog1.Color = Value;

            if (colorDialog1.ShowDialog() != DialogResult.OK)
                return;

            Value = colorDialog1.Color;
        }
    }
}

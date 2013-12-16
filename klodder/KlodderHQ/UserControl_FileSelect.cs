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
    public partial class UserControl_FileSelect : UserControl
    {
        public string FileName
        {
            get
            {
                return textBox1.Text;
            }
            set
            {
                textBox1.Text = value;
            }
        }

        public UserControl_FileSelect()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (openFileDialog1.ShowDialog() != DialogResult.OK)
                return;

            FileName = openFileDialog1.FileName;
        }

        private void UserControl_FileSelect_EnabledChanged(object sender, EventArgs e)
        {
            textBox1.Enabled = Enabled;
            button1.Enabled = Enabled;
        }
    }
}

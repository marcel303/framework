using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace KlodderHQ
{
    public partial class Form_About : Form
    {
        public Form_About()
        {
            InitializeComponent();

            BackgroundImage = Resources.about;

            Size = BackgroundImage.Size;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            Close();
        }
    }
}

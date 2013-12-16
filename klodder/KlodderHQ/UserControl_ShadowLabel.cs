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
    public partial class UserControl_ShadowLabel : UserControl
    {
        private string mText = string.Empty;
        private Font mFont = new Font(FontFamily.GenericSansSerif, 12.0f, FontStyle.Regular);

        public string ShadowText
        {
            get
            {
                return mText;
            }
            set
            {
                mText = value;

                System.Drawing.Size size = TextRenderer.MeasureText(mText, mFont);
                size.Width += 2;
                size.Height += 2;
                Size = size;

                Invalidate();
            }
        }

        public UserControl_ShadowLabel()
        {
            InitializeComponent();

            BackColor = Color.Transparent;
            Text = "Label";
        }

        private void UserControl_ShadowLabel_Paint(object sender, PaintEventArgs e)
        {
            Graphics g = e.Graphics;

            for (int x = -1; x <= +1; x++)
                for (int y = -1; y <= +1; y++)
                    g.DrawString(mText, mFont, new SolidBrush(Color.Black), new PointF(1 + x, 1 + y));

            g.DrawString(mText, mFont, new SolidBrush(Color.White), new PointF(1, 1));
        }
    }
}

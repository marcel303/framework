using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace KlodderHQ
{
    public partial class Form1 : Form
    {
        public static Form1 DefaultForm;

        private string mFileName;

        public string FileName
        {
            get
            {
                return mFileName;
            }
            set
            {
                mFileName = value;

                if (mFileName != string.Empty)
                    userControl_ShadowLabel1.ShadowText = Path.GetFileName(mFileName);
                else
                    userControl_ShadowLabel1.ShadowText = "(drop a file onto this window first)";

                // load preview (if we opened a file)

                if (mFileName != string.Empty)
                {
                    try
                    {
                        LoadPreview();
                    }
                    catch (Exception e)
                    {
                        MessageBox.Show(e.Message);
                    }
                }
            }
        }

        public static bool Debug = false;

        public Form1()
        {
            InitializeComponent();

            pictureBox1.BackColor = Color.Transparent;
            BackgroundImage = Resources.preview2;

            DefaultForm = this;

            // parse arguments

            string[] args = Environment.GetCommandLineArgs();

            if (args.Length >= 2)
            {
                FileName = args[1];
            }
            else
            {
                FileName = string.Empty;
            }
        }

        private void LoadPreview()
        {
            try
            {
                FileArchive archive = new FileArchive();

                archive.Load(mFileName);

                Stream previewStream = archive.GetStream("preview");

                if (previewStream == null)
                    throw X.Exception("missing preview stream");

                BinaryReader reader = new BinaryReader(previewStream);

                byte version = reader.ReadByte();

                if (version != 1)
                    throw X.Exception("unknown image format: {0}", version);

                int sx = reader.ReadInt32();
                int sy = reader.ReadInt32();

                int byteCount = sx * sy * 4;

                byte[] bytes = new byte[byteCount];

                previewStream.Read(bytes, 0, byteCount);

                Bitmap bitmap = new Bitmap(sx, sy);

                BitmapData bitmapData = new BitmapData();

                bitmap.LockBits(new Rectangle(new Point(0, 0), new Size(sx, sy)), System.Drawing.Imaging.ImageLockMode.WriteOnly, System.Drawing.Imaging.PixelFormat.Format24bppRgb, bitmapData);

                unsafe
                {
                    int srcIndex = 0;

                    for (int y = 0; y < sy; ++y)
                    {
                        int y2 = sy - 1 - y;

                        byte* row = (byte*)bitmapData.Scan0 + (y2 * bitmapData.Stride);

                        for (int x = 0; x < sx; ++x)
                        {
                            row[0] = bytes[srcIndex + 2];
                            row[1] = bytes[srcIndex + 1];
                            row[2] = bytes[srcIndex + 0];
                            //row[0] = bytes[srcIndex + 3];

                            srcIndex += 4;
                            row += 3;
                        }
                    }
                }

                bitmap.UnlockBits(bitmapData);

                Graphics g = Graphics.FromImage(bitmap);

                //ControlPaint.DrawBorder3D(g, new Rectangle(new Point(0, 0), new Size(sx, sy)), Border3DStyle.Sunken);
                for (int i = 0; i < 3; ++i)
                ControlPaint.DrawBorder(g, new Rectangle(new Point(i, i), new Size(sx - i * 2, sy - i * 2)), Color.Black, ButtonBorderStyle.Solid);

                pictureBox1.Image = bitmap;
            }
            catch (Exception e)
            {
                //MessageBox.Show(e.Message);

                pictureBox1.Image = null;
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            using (Form_RenderMovie form = new Form_RenderMovie())
            {
                form.ShowDialog();
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }

        class Circle
        {
            public Circle(int x, int y, int r)
            {
                X = x;
                Y = y;
                R = r;
            }

            public int X, Y, R;
        };

        private void Fractal(IList<Circle> list, double x, double y, double r, int d, int _i)
        {
            if (d == 0)
                return;

            list.Add(new Circle((int)x, (int)y, (int)r));

            int[] angles = { -1, 0, 0, -1, +1, 0, 0, +1 };

            for (int i = 0; i < 4; ++i)
            {
                if (_i >= 0 && i == (_i + 2) % 4)
                    continue;

                double dx;
                double dy;

                dx = angles[i * 2 + 0];
                dy = angles[i * 2 + 1];

                Fractal(list,
                    x + dx * r,
                    y + dy * r,
                    r / 2,
                    d - 1,
                    i);
            }
        }

        private static int Cycle(float hue)
        {
            return (int)((Math.Sin(hue) + 1.0f) * 0.5f * 255.0f);
        }

        private void button4_Click(object sender, EventArgs e)
        {
            using (Form_About form = new Form_About())
            {
                form.ShowDialog();
            }

#if false
            List<Circle> list = new List<Circle>();

            Fractal(list, 0, 0, 128, 5, -1);

            list.Reverse();

            using (FileStream stream = new FileStream("test.vg", FileMode.Create, FileAccess.Write))
            {
                using (StreamWriter writer = new StreamWriter(stream))
                {
                    Random random = new Random();

                    float hue = 0.0f;

                    foreach (Circle circle in list)
                    {
                        hue += (float)Math.PI / 60.0f;

                        int r = Cycle(hue * 1.0f);
                        int g = Cycle(hue * 1.1f);
                        int b = Cycle(hue * 1.2f);

                        writer.WriteLine("circle");
                        writer.WriteLine("\tposition {0} {1}", circle.X, circle.Y);
                        writer.WriteLine("\tradius {0}", circle.R);
                        writer.WriteLine("\tfill 255");// {0}", (random.NextDouble() * 0.5).ToString(X.CultureEN));
                        writer.WriteLine("\tfilled 0");
                        writer.WriteLine("\tline 255");
                        double stroke = Math.Max(1.5, circle.R / 10.0);
                        //writer.WriteLine("\tstroke {0}", stroke.ToString(X.CultureEN));
                        //writer.WriteLine("\thardness {0}", (stroke - 0.5).ToString(X.CultureEN));
                        writer.WriteLine("\tcolor {0} {1} {2}", r, g, b);
                    }
                }
            }

            MessageBox.Show("created test.vg");
#endif
        }

        private void Form1_DragDrop(object sender, DragEventArgs args)
        {
            try
            {
                string[] fileNameList = (string[])args.Data.GetData(DataFormats.FileDrop);

                if (fileNameList.Length >= 1)
                    FileName = fileNameList[0];
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
            }
        }

        private void Form1_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop, false))
                e.Effect = DragDropEffects.All;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            using (Form_RenderReplay form = new Form_RenderReplay())
            {
                form.ShowDialog();
            }
        }

        private void button6_Click(object sender, EventArgs e)
        {
            using (Form_RenderPicture form = new Form_RenderPicture("png"))
            {
                form.ShowDialog();
            }
        }

        private void button5_Click(object sender, EventArgs e)
        {
            using (Form_RenderPicture form = new Form_RenderPicture("psd"))
            {
                form.ShowDialog();
            }
        }

        private void Form1_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F11)
            {
                Form1.Debug = true;

                MessageBox.Show("Diagnostics mode enabled");
            }
        }
    }
}

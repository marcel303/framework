using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;

namespace start
{
    public partial class Form1 : Form
    {
        private int DisplaySize
        {
            get
            {
                return Settings.Get("DisplaySize", -1);
            }
            set
            {
                Settings.Set("DisplaySize", value);
            }
        }

        private bool FullScreen
        {
            get
            {
                return Settings.Get("DisplayFullScreen", true);
            }
            set
            {
                Settings.Set("DisplayFullScreen", value);
            }
        }

        private ControllerType ControllerType
        {
            get
            {
                int type = Settings.Get("ControllerType", (int)ControllerType.Keyboard);

                foreach (ControllerType type2 in Enum.GetValues(typeof(ControllerType)))
                {
                    if (type == (int)type2)
                        return type2;
                }

                return ControllerType.Keyboard;
            }
            set
            {
                Settings.Set("ControllerType", (int)value);
            }
        }

        private static string BaseDirectory
        {
        	get
            {
                return Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]);
            }
		}

        public Form1()
        {
            InitializeComponent();

            pictureBox1.Image = Resources.img;

            comboBox1.Items.Add(new ResolutionItem(480, 320, "iPhone Native", 1));
            comboBox1.Items.Add(new ResolutionItem(960, 640, "x2", 2));
            comboBox1.Items.Add(new ResolutionItem(1440, 960, "x3", 3));

            int displaySize = DisplaySize;

            ResolutionItem selected = null;

            if (selected == null)
            {
                foreach (ResolutionItem item in comboBox1.Items)
                {
                    if (item.Scale == displaySize)
                        selected = item;
                }
            }
            
            if (selected == null)
            {
                int maxSx = Screen.PrimaryScreen.Bounds.Width;
                int maxSy = Screen.PrimaryScreen.Bounds.Height;

                foreach (ResolutionItem item in comboBox1.Items)
                {
                    if ((selected == null || item.Sx > selected.Sx) && item.Sx <= maxSx && item.Sy <= maxSy)
                    {
                        selected = item;
                    }
                }
            }

            comboBox1.SelectedItem = selected;

            checkBox1.Checked = FullScreen;

            ControllerType type = ControllerType;

            radioButton1.Checked = type == ControllerType.Gamepad;
            radioButton2.Checked = type == ControllerType.Keyboard;
        }

        private static string ControllerTypeToString(ControllerType type)
        {
            switch (type)
            {
                case ControllerType.Gamepad:
                    return "gamepad";
                case ControllerType.Keyboard:
                    return "keyboard";
            }

            throw X.Exception("unknown controller type: {0}", type);
        }

        private void ControllerTypeChanged(object sender, EventArgs e)
        {
            if (radioButton1.Checked)
                ControllerType = ControllerType.Gamepad;
            if (radioButton2.Checked)
                ControllerType = ControllerType.Keyboard;
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            FullScreen = checkBox1.Checked;
        }

        private void comboBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            ResolutionItem selected = comboBox1.SelectedItem as ResolutionItem;

            if (selected == null)
                return;

            DisplaySize = selected.Scale;
        }

        public static void ExecuteCritwave(bool synchronous, params string[] args)
        {
            for (int i = 0; i < args.Length; ++i)
                if (args[i].Contains(' '))
                    args[i] = string.Format("\"{0}\"", args[i]);

            StringBuilder sbArgs = new StringBuilder();
            foreach (string arg in args)
                sbArgs.AppendFormat("{0} ", arg);
            string argsStr = sbArgs.ToString().Trim();

            ProcessStartInfo psi = new ProcessStartInfo();

            psi.Arguments = argsStr;
            psi.ErrorDialog = false;
            psi.FileName = Path.Combine(BaseDirectory, "critwave.exe");
            psi.UseShellExecute = false;
            psi.WorkingDirectory = BaseDirectory;
            psi.CreateNoWindow = true;

            if (synchronous)
            {
                psi.RedirectStandardOutput = true;
            }

            Process process = new Process();
            process.StartInfo = psi;
            process.Start();

            if (synchronous)
            {
                process.WaitForExit();

                string output = process.StandardOutput.ReadToEnd();

                if (process.ExitCode != 0)
                    throw X.Exception("The process exited with error code {0}. FileName: {1}, Arguments: {2}, Output: {3}", process.ExitCode, psi.FileName, psi.Arguments, output);
            }
        }

        private void button1_Click(object sender, EventArgs _e)
        {
            try
            {
                List<string> args = new List<string>();

                args.Add("-s");
                args.Add(DisplaySize.ToString(X.CultureEN));
                args.Add("-f");
                args.Add(FullScreen ? "1" : "0");
                args.Add("-p");
                args.Add(ControllerTypeToString(ControllerType));

                ExecuteCritwave(true, args.ToArray());
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
            }
        }
    }

    class ResolutionItem
    {
        public ResolutionItem(int sx, int sy, string name, int scale)
        {
            Sx = sx;
            Sy = sy;
            Name = name;
            Scale = scale;
        }

        public override string ToString()
        {
            return string.Format("{0}x{1} ({2})", Sx, Sy, Name);
        }

        public int Sx;
        public int Sy;
        public string Name;
        public int Scale;
    }

    enum ControllerType
    {
        Gamepad = 0,
        Keyboard = 1
    }
}

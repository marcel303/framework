using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace BuildTool
{
	public partial class BuildAgent : Form
	{
		public static BuildAgent mInstance;

		public BuildAgent()
		{
			InitializeComponent();

			mInstance = this;

			//Hide();
		}

		public static void Deliver(BuildMessage message)
		{
			mInstance.DeliverInternal(message);
		}

		public void DeliverInternal(BuildMessage message)
		{
			switch (message.Type)
			{
				case BOT.Error:
					notifyIcon1.BalloonTipIcon = ToolTipIcon.Error;
					break;
				case BOT.Info:
					notifyIcon1.BalloonTipIcon = ToolTipIcon.Info;
					break;
				case BOT.Warning:
					notifyIcon1.BalloonTipIcon = ToolTipIcon.Warning;
					break;
			}

			//notifyIcon1.Visible = false;
			notifyIcon1.BalloonTipText = message.Text;
			notifyIcon1.BalloonTipTitle = message.FileName;
			//notifyIcon1.Visible = true;
			notifyIcon1.ShowBalloonTip(5000);
		}
	}
}

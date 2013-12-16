using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace vecdraw
{
	public partial class PropertyControl : UserControl
	{
		private Control m_Control;

		public String Label
		{
			get
			{
				return label1.Text;
			}
			set
			{
				label1.Text = value;
			}
		}

		public Control Control
		{
			get
			{
				return m_Control;
			}
			set
			{
				if (m_Control != null)
					Controls.Remove(m_Control);

				m_Control = value;

				if (m_Control != null)
				{
					m_Control.Left = 100;
					m_Control.Top = label1.Top;
					m_Control.Width = 130;

					Controls.Add(m_Control);
				}
			}
		}

		public PropertyControl()
		{
			InitializeComponent();
		}
	}
}

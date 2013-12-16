using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace vecdraw
{
	public partial class Form_Transform : Form
	{
		private TransformType m_TransformType = TransformType.Rotate180;

		public Form_Transform()
		{
			InitializeComponent();
		}

		public bool SelectionOnly
		{
			get
			{
				return checkBox1.Checked;
			}
		}

		public TransformType TransformType
		{
			get
			{
				return m_TransformType;
			}
		}

		private void button1_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.OK;

			Close();
		}

		private void button2_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.Cancel;

			Close();
		}

		private void button4_Click(object sender, EventArgs e)
		{
			m_TransformType = TransformType.InvertX;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void button5_Click(object sender, EventArgs e)
		{
			m_TransformType = TransformType.InvertY;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void button3_Click(object sender, EventArgs e)
		{
			m_TransformType = TransformType.FlipXY;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void button6_Click(object sender, EventArgs e)
		{
			m_TransformType = TransformType.RotateLeft;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void button7_Click(object sender, EventArgs e)
		{
			m_TransformType = TransformType.RotateRight;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void button8_Click(object sender, EventArgs e)
		{
			m_TransformType = TransformType.Rotate180;
			DialogResult = DialogResult.OK;
			Close();
		}

		private void button1_Click_1(object sender, EventArgs e)
		{
			m_TransformType = TransformType.CenterPivot;
			DialogResult = DialogResult.OK;
			Close();
		}
	}

	public enum TransformType
	{
		FlipXY,
		InvertX,
		InvertY,
		RotateLeft, // flip xy, inv x, inv y
		RotateRight, // flip xy
		Rotate180, // inv x, inv y
		CenterPivot
	}
}

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Reflection;
//using vecdraw.Elements;

namespace vecdraw
{
	public partial class PropertyEditor : UserControl
	{
		bool m_DisableEvents = false;
		private List<DataInfo> m_Datas = new List<DataInfo>();
		private Dictionary<String, DataInfo> m_DatasByName = new Dictionary<string, DataInfo>();

		public FlowDirection FlowDirection
		{
			get
			{
				return flowLayoutPanel1.FlowDirection;
			}
			set
			{
				flowLayoutPanel1.FlowDirection = value;
			}
		}

		public PropertyEditor()
		{
			InitializeComponent();
		}

		public void Set(String name, object value)
		{
			DataInfo di = m_DatasByName[name];

			di.SetValue(m_Editable, value);

			m_Editable.HandlePropertyChanged(di);
		}

		private void Rebuild()
		{
			Logger.Log("Rebuilding property editor");

			flowLayoutPanel1.Controls.Clear();

			if (m_Editable == null)
				return;

			Logger.Log("Creating attribute list");

			Type type = m_Editable.GetType();

			m_Datas.Clear();
			m_DatasByName.Clear();

			foreach (PropertyInfo member in type.GetProperties())
				m_Datas.Add(new DataInfo(null, member));
			foreach (FieldInfo member in type.GetFields())
				m_Datas.Add(new DataInfo(member, null));

			foreach (DataInfo di in m_Datas)
				m_DatasByName[di.Name] = di;

			Logger.Log("Creating data editing fields");

			foreach (DataInfo member in m_Datas)
			{
				object[] attributes = member.GetCustomAttributes();

				foreach (object attribute in attributes)
				{
					PropertyAttribute property = attribute as PropertyAttribute;

					if (property == null)
						continue;

					PropertyControl control = new PropertyControl();

					control.Label = property.Name;

					if (member.FieldType == typeof(Boolean))
					{
						CheckBox input = new CheckBox();
						input.AutoSize = true;
						input.Tag = new InputInfo(InputType.Boolean, member);
						input.CheckedChanged += HandleBooleanChanged;
						control.Control = input;
					}
					else if (member.FieldType == typeof(Point))
					{
						PointTextBox input = new PointTextBox();
						input.Tag = new InputInfo(InputType.Point, member);
						input.OnPointChanged += HandlePointChanged;
						control.Control = input;
					}
					else if (member.FieldType.IsSubclassOf(typeof(Enum)))
					{
						FlagsControl input = new FlagsControl();
						input.Tag = new InputInfo(InputType.Enum, member);
						input.Type = member.FieldType;
						input.ValueChanged += HandleEnumChanged;
						control.Control = input;
					}
					else if (member.FieldType == typeof(VecAngle))
					{
						AngleControl input = new AngleControl();
						input.Tag = new InputInfo(InputType.Angle, member);
						input.ValueChanged += HandleAngleChanged;
						control.Control = input;
					}
					else if (member.FieldType == typeof(VecColor))
					{
						ColorButton input = new ColorButton();
						input.Color = (VecColor)member.GetValue(m_Editable);
						input.Tag = new InputInfo(InputType.Undefined, member);
						input.OnChange += HandleColorChanged;
						control.Control = input;
					}
					else if (member.FieldType == typeof(FileName))
					{
						FileNameButton input = new FileNameButton();
						input.Tag = new InputInfo(InputType.FileName, member);
						input.OnChange += HandleFileNameChanged;
						control.Control = input;
					}
					else if (member.FieldType == typeof(int))
					{
						TextBox input = new TextBox();
						input.Tag = new InputInfo(InputType.Integer, member);
						input.TextChanged += HandleTextChanged;
						control.Control = input;
					}
					else if (member.FieldType == typeof(float))
					{
						TextBox input = new TextBox();
						input.Tag = new InputInfo(InputType.Float, member);
						input.TextChanged += HandleTextChanged;
						control.Control = input;
					}
					else
					{
						TextBox input = new TextBox();
						input.Tag = new InputInfo(InputType.Undefined, member);
						input.TextChanged += HandleTextChanged;
						control.Control = input;
					}

					flowLayoutPanel1.Controls.Add(control);
				}
			}

			UpdateValues();
		}

		public void UpdateValues()
		{
			Logger.Log("Updating values");

			m_DisableEvents = true;

			foreach (Control _control in flowLayoutPanel1.Controls)
			{
				PropertyControl propertyControl = _control as PropertyControl;

				if (propertyControl == null)
					continue;

				Control control = propertyControl.Control;

				if (control == null)
					continue;

				if (control.Tag == null)
					continue;

				InputInfo info = (InputInfo)control.Tag;

				DataInfo fi = (DataInfo)info.Tag;

				switch (info.Type)
				{
					case InputType.Boolean:
						{
							CheckBox checkBox = control as CheckBox;

							checkBox.Checked = (bool)fi.GetValue(m_Editable);
						}
						break;

					case InputType.Integer:
					case InputType.String:
					case InputType.Float:
					case InputType.Undefined:
						control.Text = fi.GetValue(m_Editable).ToString();
						break;

					case InputType.Point:
						{
							PointTextBox textBox = control as PointTextBox;
							Point p = (Point)fi.GetValue(m_Editable);
							textBox.TextX = p.X.ToString(Global.CultureEN);
							textBox.TextY = p.Y.ToString(Global.CultureEN);
						}
						break;

					case InputType.Enum:
						{
							FlagsControl flagsControl = control as FlagsControl;
							int value = (int)fi.GetValue(m_Editable);
							flagsControl.Value = value;
						}
						break;

					case InputType.Angle:
						{
							AngleControl angleControl = control as AngleControl;
							VecAngle angle = (VecAngle)fi.GetValue(m_Editable);
							angleControl.Value = angle.Degrees;
						}
						break;

					case InputType.FileName:
						{
							FileNameButton button = (FileNameButton)control;

							button.FileName = (FileName)fi.GetValue(m_Editable);
						}
						break;
				}
			}

			m_DisableEvents = false;
		}

		private void HandleBooleanChanged(object sender, EventArgs e)
		{
			if (m_DisableEvents)
				return;

			CheckBox control = (CheckBox)sender;

			InputInfo info = (InputInfo)control.Tag;

			DataInfo fi = (DataInfo)info.Tag;

			fi.SetValue(m_Editable, control.Checked);

			m_Editable.HandlePropertyChanged(fi);
		}

		private void HandleEnumChanged(object sender, EventArgs e)
		{
			if (m_DisableEvents)
				return;

			FlagsControl control = (FlagsControl)sender;

			InputInfo info = (InputInfo)control.Tag;

			DataInfo fi = (DataInfo)info.Tag;

			try
			{
				int value = control.Value;

				fi.SetValue(m_Editable, value);

				m_Editable.HandlePropertyChanged(fi);
			}
			catch
			{
			}
		}

		private void HandleAngleChanged(object sender, EventArgs e)
		{
			if (m_DisableEvents)
				return;

			AngleControl control = (AngleControl)sender;

			InputInfo info = (InputInfo)control.Tag;

			DataInfo fi = (DataInfo)info.Tag;

			try
			{
				VecAngle angle = new VecAngle();
				angle.Degrees = control.Value;

				fi.SetValue(m_Editable, angle);

				m_Editable.HandlePropertyChanged(fi);
			}
			catch
			{
			}
		}

		private void HandleColorChanged(object sender, EventArgs e)
		{
			if (m_DisableEvents)
				return;

			ColorButton control = (ColorButton)sender;

			InputInfo info = (InputInfo)control.Tag;

			DataInfo fi = (DataInfo)info.Tag;

			fi.SetValue(m_Editable, control.Color);

			m_Editable.HandlePropertyChanged(fi);
		}

		private void HandleFileNameChanged(object sender, EventArgs e)
		{
			if (m_DisableEvents)
				return;

			FileNameButton control = (FileNameButton)sender;

			InputInfo info = (InputInfo)control.Tag;

			DataInfo fi = (DataInfo)info.Tag;

			fi.SetValue(m_Editable, control.FileName);

			m_Editable.HandlePropertyChanged(fi);
		}

		private void HandlePointChanged(object sender, EventArgs e)
		{
			if (m_DisableEvents)
				return;

			PointTextBox control = (PointTextBox)sender;

			InputInfo info = (InputInfo)control.Tag;

			DataInfo fi = (DataInfo)info.Tag;

			try
			{
				if (control.TextX == String.Empty || control.TextY == String.Empty)
					throw new Exception("Input pending");

				Point point = new Point();

				point.X = Int32.Parse(control.TextX, Global.CultureEN);
				point.Y = Int32.Parse(control.TextY, Global.CultureEN);

				fi.SetValue(m_Editable, point);

				m_Editable.HandlePropertyChanged(fi);
			}
			catch
			{
			}
		}

		private void HandleTextChanged(object sender, EventArgs e)
		{
			if (m_DisableEvents)
				return;

			Control control = (Control)sender;

			InputInfo info = (InputInfo)control.Tag;

			DataInfo fi = (DataInfo)info.Tag;

			try
			{
				switch (info.Type)
				{
					case InputType.Integer:
					case InputType.String:
					case InputType.Undefined:
						fi.SetValue(m_Editable, Convert.ChangeType(control.Text, fi.FieldType, Global.CultureEN));
						break;

					case InputType.Float:
						if (control.Text.EndsWith("."))
							throw new Exception("Input Pending");
						fi.SetValue(m_Editable, Convert.ChangeType(control.Text, fi.FieldType, Global.CultureEN));
						break;

					default:
						throw new Exception(String.Format("type not implemented (application error): {0}", info.Type));
				}

				m_Editable.HandlePropertyChanged(fi);
			}
			catch
			{
			}
		}

		public IEditable Editable
		{
			get
			{
				return m_Editable;
			}
			set
			{
				m_Editable = value;

				Rebuild();
			}
		}

		private IEditable m_Editable;
	}

	public class DataInfo
	{
		public DataInfo(FieldInfo fi, PropertyInfo pi)
		{
			m_FieldInfo = fi;
			m_PropertyInfo = pi;
		}

		public object GetValue(object obj)
		{
			if (m_FieldInfo != null)
				return m_FieldInfo.GetValue(obj);
			if (m_PropertyInfo != null)
				return m_PropertyInfo.GetValue(obj, null);

			return null;
		}

		public void SetValue(object obj, object value)
		{
			if (m_FieldInfo != null)
				m_FieldInfo.SetValue(obj, value);
			if (m_PropertyInfo != null)
				m_PropertyInfo.SetValue(obj, value, null);
		}

		public object[] GetCustomAttributes()
		{
			if (m_FieldInfo != null)
				return m_FieldInfo.GetCustomAttributes(true);
			if (m_PropertyInfo != null)
				return m_PropertyInfo.GetCustomAttributes(true);

			return null;
		}

		public Type FieldType
		{
			get
			{
				if (m_FieldInfo != null)
					return m_FieldInfo.FieldType;
				if (m_PropertyInfo != null)
					return m_PropertyInfo.PropertyType;

				return null;
			}
		}

		public String Name
		{
			get
			{
				if (m_FieldInfo != null)
					return m_FieldInfo.Name;
				if (m_PropertyInfo != null)
					return m_PropertyInfo.Name;

				return null;
			}
		}

		public FieldInfo m_FieldInfo;
		public PropertyInfo m_PropertyInfo;
	}

	enum InputType
	{
		Undefined,
		Point,
		Point_X,
		Point_Y,
		Enum,
		Angle,
		Color_R,
		Color_G,
		Color_B,
		String,
		Integer,
		Float,
		Boolean,
		FileName
	}

	class InputInfo
	{
		public InputInfo(InputType type, object tag)
		{
			Type = type;
			Tag = tag;
		}

		public InputType Type = InputType.Undefined;
		public object Tag;
	}
}

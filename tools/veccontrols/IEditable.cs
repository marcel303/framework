using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace vecdraw
{
	public interface IEditable
	{
		void HandlePropertyChanged(DataInfo fi);
	}
}

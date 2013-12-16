using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BuildTool
{
	public class BuildObject
	{
		public BuildObject(
			FileName source,
			FileName destination,
			FileName directDependency,
			IBuildRule rule)
		{
			Source = source;
			Destination = destination;
			DirectDependency = directDependency;
			BuildRule = rule;
		}

		public FileName Source;
		public FileName Destination;
		public FileName DirectDependency; // for a .cpp file this would its corresponding .h file
		public IBuildRule BuildRule;

		/* public void Build()
		 {
			 BuildRule.Build(mCtx, this);
		 }*/
	}
}

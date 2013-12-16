using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace BuildTool
{
    public interface IBuildRule
    {
        string GetToolName();
        string GetRuleName();
        bool GetOutputName(FileName fileName, out FileName destination);
		FileName GetDirectDependency(FileName fileName);
        void Build(BuildCtx ctx, BuildObject obj);
    }
}

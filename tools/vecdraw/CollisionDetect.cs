using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

namespace vecdraw
{
	class CollisionDetect
	{
		// Returns true if a horizontal line fired in the +X direction intersects the line segment (p1, p2).

		public static bool HitTest_HLineO(PointF p1, PointF p2)
		{
			PointF d = p2.Subtract(p1);

			if (d.Y == 0.0f)
				return false;

			float t = -p1.Y / d.Y;

			float x = p1.X + d.X * t;

			return x >= 0.0f;
		}

		public static bool HitTest_Outline(IList<PointF> outline, PointF p)
		{
			int count = 0;

			for (int i = 0; i < outline.Count; ++i)
			{
				int index1 = (i + 0) % outline.Count;
				int index2 = (i + 1) % outline.Count;

				PointF v1 = outline[index1].Subtract(p);
				PointF v2 = outline[index2].Subtract(p);

				if (v1.Y > v2.Y)
				{
					PointF temp = v1;
					v1 = v2;
					v2 = temp;
				}

				if (v1.Y <= 0.0f && v2.Y >= 0.0f)
				{
					if (v1.X >= 0.0f && v2.X >= 0.0f)
					{
						count++;
					}
					else if (v1.X >= 0.0f || v2.X >= 0.0f)
					{
						if (HitTest_HLineO(v1, v2))
						{
							count++;
						}
					}
				}
			}

			return (count & 1) != 0;
		}
	}
}

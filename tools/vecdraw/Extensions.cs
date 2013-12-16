using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;

namespace vecdraw
{
	static class Extensions
	{
		// Point

		public static Point Subtract(this Point self, Point other)
		{
			Point result = new Point();

			result.X = self.X - other.X;
			result.Y = self.Y - other.Y;

			return result;
		}

		public static double Magnitude(this Point self)
		{
			int magSq = self.X * self.X + self.Y * self.Y;

			return Math.Sqrt(magSq);
		}

		public static PointF ToPointF(this Point self)
		{
			return new PointF(self.X, self.Y);
		}

		// PointF

		public static double Magnitude(this PointF self)
		{
			float magSq = self.X * self.X + self.Y * self.Y;

			return Math.Sqrt(magSq);
		}

		public static PointF Subtract(this PointF self, PointF other)
		{
			PointF result = new PointF();

			result.X = self.X - other.X;
			result.Y = self.Y - other.Y;

			return result;
		}

		public static float Dot(this PointF self, PointF other)
		{
			return self.X * other.X + self.Y * other.Y;
		}

		public static PointF Normalized(this PointF self)
		{
			float mag = (float)self.Magnitude();

			return new PointF(self.X / mag, self.Y / mag);
		}
	}
}

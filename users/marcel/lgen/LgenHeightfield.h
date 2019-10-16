#pragma once

namespace lgen
{
	struct Heighfield
	{
		int ** height = nullptr;
		int w = 0;
		int h = 0;

		virtual ~Heighfield();

		virtual bool setSize(int w, int h);
		virtual void clear();

		void clamp(int min, int max);
		void rerange(int min, int max);
		void copy(Heighfield * dst) const;
		bool getSizePowers(int & pw, int & ph) const;
	};
}

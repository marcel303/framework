#include "Paint.h"

void PaintBrush(Validatable* val, Canvas* canvas, coord_t* coord, Brush* brush, float* color, float opacity)
{
	int min[2];
	int max[2];

	min[0] = FL(coord->p[0] - brush->desc.size / 2.0f);
	min[1] = FL(coord->p[1] - brush->desc.size / 2.0f);
	max[0] = CL(coord->p[0] + brush->desc.size / 2.0f);
	max[1] = CL(coord->p[1] + brush->desc.size / 2.0f);

	float minf[2];

	minf[0] = coord->p[0] - brush->desc.size / 2.0f;
	minf[1] = coord->p[1] - brush->desc.size / 2.0f;

	for (int y = 0; y < brush->desc.size; ++y)
	{
		for (int x = 0; x < brush->desc.size; ++x)
		{
			canvas->Paint((int)(minf[0] + x), (int)(minf[1] + y), *brush->GetPix(x, y), color, opacity);
		}
	}

	Rect rect;

	rect.min[0] = min[0];
	rect.min[1] = min[1];
	rect.max[0] = max[0];
	rect.max[1] = max[1];

	val->Invalidate(rect);
}

void PaintBlur(Validatable* val, Canvas* canvas, coord_t* coord, Brush* brush, int size)
{
	int min[2];
	int max[2];

	min[0] = FL(coord->p[0] - brush->desc.size / 2.0f);
	min[1] = FL(coord->p[1] - brush->desc.size / 2.0f);
	max[0] = CL(coord->p[0] + brush->desc.size / 2.0f);
	max[1] = CL(coord->p[1] + brush->desc.size / 2.0f);

	float minf[2];

	minf[0] = coord->p[0] - brush->desc.size / 2.0f;
	minf[1] = coord->p[1] - brush->desc.size / 2.0f;

	//Canvas temp;

	//temp.Create(max[0] - min[0] + 1, max[1] - min[1] + 1, canvas->channels);
	
	for (int y = 0; y < brush->desc.size; ++y)
	{
		for (int x = 0; x < brush->desc.size; ++x)
		{
			const sample_t* bpix = brush->GetPix(x, y);
			const float c = bpix[0];
			//sample_t* tpix = temp.GetPix(x, y);
			sample_t* cpix1 = canvas->GetPix(min[0] + x, min[1] + y);

			if (!cpix1 || c <= 0.01f)
				continue;

			//const float c_1 = 1.0f / c;
			const float c_1 = 1.0f - c;

			sample_t* tpix = cpix1;

			for (int i = 0; i < canvas->channels; ++i)
				tpix[i] = cpix1[i] * c_1;

			float total = 1.0f * c_1;

			for (int oy = -size; oy <= +size; ++oy)
			{
				for (int ox = -size; ox <= +size; ++ox)
				{
					if (ox == 0 && oy == 0)
						continue;

					const sample_t* cpix2 = canvas->GetPix(min[0] + x + ox, min[1] + y + oy);

					if (!cpix2)
						continue;

					for (int i = 0; i < canvas->channels; ++i)
						tpix[i] += cpix2[i];

					total += 1.0f;
				}
			}

			const float total_1 = 1.0f / total;

			for (int i = 0; i < canvas->channels; ++i)
				tpix[i] *= total_1;
		}
	}

#if 0
	for (int y = 0; y < brush->desc.size; ++y)
	{
		for (int x = 0; x < brush->desc.size; ++x)
		{
			sample_t* cpix = canvas->GetPix(min[0] + x, min[1] + y);
			sample_t* tpix = temp.GetPix(x, y);

			if (!cpix)
				continue;

			for (int i = 0; i < canvas->channels; ++i)
				cpix[i] = tpix[i];
		}
	}
#endif

	Rect rect;

	rect.min[0] = min[0];
	rect.min[1] = min[1];
	rect.max[0] = max[0];
	rect.max[1] = max[1];

	val->Invalidate(rect);
}

void PaintSmudge(Validatable* val, Canvas* canvas, coord_t* coord, Brush* brush, coord_t* direction, float strength)
{
	int min[2];
	int max[2];

	min[0] = FL(coord->p[0] - brush->desc.size / 2.0f);
	min[1] = FL(coord->p[1] - brush->desc.size / 2.0f);
	max[0] = CL(coord->p[0] + brush->desc.size / 2.0f);
	max[1] = CL(coord->p[1] + brush->desc.size / 2.0f);

	float minf[2];

	minf[0] = coord->p[0] - brush->desc.size / 2.0f;
	minf[1] = coord->p[1] - brush->desc.size / 2.0f;

	Canvas temp;

	temp.Create(max[0] - min[0] + 1, max[1] - min[1] + 1, canvas->channels);
	
	for (int y = 0; y < brush->desc.size; ++y)
	{
		const sample_t* bpix = brush->GetPix(0, y);
		sample_t* cpix1 = temp.GetPix(0, y);

		for (int x = 0; x < brush->desc.size; ++x, bpix+=brush->channels, cpix1+=canvas->channels)
		{
			const float s = bpix[0] * strength;
			//sample_t* cpix1 = canvas->GetPix(min[0] + x, min[1] + y);

			//if (!cpix1)
				//continue;

			const float sx = min[0] + x - s * direction->p[0];
			const float sy = min[1] + y - s * direction->p[1];

			sample_t cpix2[MAX_CHANNELS];

			canvas->GetPixAA(sx, sy, cpix2);

			for (int i = 0; i < canvas->channels; ++i)
					cpix1[i] = cpix2[i];
		}
	}

#if 1
	for (int y = 0; y < brush->desc.size; ++y)
	{
		for (int x = 0; x < brush->desc.size; ++x)
		{
			sample_t* cpix = canvas->GetPix(min[0] + x, min[1] + y);
			sample_t* tpix = temp.GetPix(x, y);

			if (!cpix)
				continue;

			for (int i = 0; i < canvas->channels; ++i)
				cpix[i] = tpix[i];
		}
	}
#endif

	Rect rect;

	rect.min[0] = min[0];
	rect.min[1] = min[1];
	rect.max[0] = max[0];
	rect.max[1] = max[1];

	val->Invalidate(rect);
}

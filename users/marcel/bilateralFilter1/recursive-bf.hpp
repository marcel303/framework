#ifndef INCLUDE_RBF
#define INCLUDE_RBF
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================

RecursiveBF: A lightweight library for recursive bilateral filtering.

-------------------------------------------------------------------------

Intro:      Recursive bilateral filtering (developed by Qingxiong Yang) 
            is pretty fast compared with most edge-preserving filtering 
            methods.

            -   computational complexity is linear in both input size and 
                dimensionality
            -   takes about 43 ms to process a one mega-pixel color image
                (i7 1.8GHz & 4GB memory)
            -   about 18x faster than Fast high-dimensional filtering 
                using the permutohedral lattice
            -   about 86x faster than Gaussian kd-trees for fast high-
                dimensional filtering


Usage:      // ----------------------------------------------------------
            // Basic Usage
            // ----------------------------------------------------------

            uint8_t * img = ...;                    // input image
            uint8_t * img_out = 0;            // output image
            int width = ..., height = ..., channel = ...; // image size
            recursive_bf(img, img_out, 
                         sigma_spatial, sigma_range, 
                         width, height, channel);

            // ----------------------------------------------------------
            // Advanced: using external buffer for better performance
            // ----------------------------------------------------------

            uint8_t * img = ...;                    // input image
            uint8_t * img_out = 0;            // output image
            int width = ..., height = ..., channel = ...; // image size
            float * buffer = new float[                   // external buf
                                 ( width * height* channel 
                                 + width * height
                                 + width * channel 
                                 + width) * 2];
            recursive_bf(img, img_out, 
                         sigma_spatial, sigma_range, 
                         width, height, channel, 
                         buffer);
            delete[] buffer;


Notice:     Large sigma_spatial/sigma_range parameter may results in 
            visible artifact which can be removed by an additional 
            filter with small sigma_spatial/sigma_range parameter.

-------------------------------------------------------------------------

Reference:  Qingxiong Yang, Recursive Bilateral Filtering,
            European Conference on Computer Vision (ECCV) 2012, 399-413.

====================================================================== */

template <int channel>
inline void recursive_bf(
    const uint8_t * img_in,
    const uint8_t * lum_in,
    uint8_t * img_out,
    const float sigma_spatial,
    const float sigma_range,
    const int width,
    const int height,
    float * buffer = 0);

// ----------------------------------------------------------------------

inline float * recursive_bf_buffer_alloc(
	const int width,
    const int height,
    const int channel)
{
    const int width_height = width * height;
    const int width_channel = width * channel;
    const int width_height_channel = width * height * channel;

	float * buffer = new float[
		(
			width_height_channel +
			width_height +
			width_channel +
			width
		) * 2];
	
	return buffer;
}

inline void recursive_bf_buffer_free(float * buffer)
{
	delete [] buffer;
}

template <int channel>
inline void _recursive_bf(
    uint8_t * img,
    const uint8_t * lum,
    const float sigma_spatial,
    const float sigma_range,
    const int width,
    const int height,
    float * buffer = 0)
{
    const int width_height = width * height;
    const int width_channel = width * channel;
    const int width_height_channel = width * height * channel;

    const bool is_buffer_internal = (buffer == nullptr);
	
    if (is_buffer_internal)
    {
        buffer = new float[
        	(
        		width_height_channel +
        		width_height +
        		width_channel +
        		width
			) * 2];
	}
	
    float * __restrict img_out_f      = buffer;
    float * __restrict img_temp       = &img_out_f[width_height_channel];
    float * __restrict map_factor_a   = &img_temp[width_height_channel];
    float * __restrict map_factor_b   = &map_factor_a[width_height];
    float * __restrict slice_factor_a = &map_factor_b[width_height];
    float * __restrict slice_factor_b = &slice_factor_a[width_channel];
    float * __restrict line_factor_a  = &slice_factor_b[width_channel];
    float * __restrict line_factor_b  = &line_factor_a[width];
    
    // compute a lookup table
    float range_table[256];
    float inv_sigma_range = 1.0f / sigma_range;
    for (int i = 0; i < 256; i++) 
        range_table[i] = static_cast<float>(expf(-i * inv_sigma_range));

    const float alpha = static_cast<float>(expf(-sqrtf(2.f) / sigma_spatial));
	const float inv_alpha_ = 1 - alpha;
	
    for (int y = 0; y < height; y++)
    {
        const uint8_t * __restrict in_x = &img[y * width_channel];
        const uint8_t * __restrict luminance_x = &lum[y * width];
		
        float * __restrict temp_x = &img_temp[y * width_channel];
        float * __restrict temp_factor_x = &map_factor_a[y * width];
		
		float yp[channel];
        for (int i = 0; i < channel; ++i)
        	*temp_x++ = yp[i] = *in_x++;
		
		uint8_t lp = *luminance_x++;
		
		float fp;
        *temp_factor_x++ = fp = 1;

        // from left to right
        for (int x = 1; x < width; x++)
        {
			const uint8_t lc = *luminance_x++;
            const uint8_t range_dist = abs(lc - lp);
            const float weight = range_table[range_dist];
            const float alpha_ = weight * alpha;
			
            for (int i = 0; i < channel; ++i)
            {
            	const float yc =
            		inv_alpha_ * (*in_x) +
            		alpha_ * yp[i];
				
            	*temp_x = yc;
            	yp[i] = yc;
				
            	temp_x++;
            	in_x++;
			}
			
			lp = lc;

			const float fc =
				inv_alpha_ +
				alpha_ * fp;
			
            *temp_factor_x++ = fc;
            fp = fc;
        }
		
		for (int i = 0; i < channel; ++i)
		{
			--in_x;
			--temp_x;
			
			*temp_x = 0.5f * ((*temp_x) + (*in_x));
        	yp[i] = *in_x;
		}
		
		--luminance_x;
		lp = *luminance_x;

        --temp_factor_x;
        *temp_factor_x = 0.5f * ((*temp_factor_x) + 1);
		
        fp = 1;

        // from right to left
        for (int x = width - 2; x >= 0; x--) 
        {
        	--luminance_x;
			
			const uint8_t lc = *luminance_x;
            const uint8_t range_dist = abs(lc - lp);
            const float weight = range_table[range_dist];
            const float alpha_ = weight * alpha;

			for (int i = 0; i < channel; ++i)
			{
				--in_x;
				--temp_x;
				
				const float yc =
					inv_alpha_ * (*in_x) +
					alpha_ * yp[i];
				
            	*temp_x = 0.5f * ((*temp_x) + yc);
            	yp[i] = yc;
			}
			
			lp = lc;

            const float fc =
            	inv_alpha_ +
            	alpha_ * fp;
			
            --temp_factor_x;
            *temp_factor_x = 0.5f * ((*temp_factor_x) + fc);
            fp = fc;
        }
    }
	
    memcpy(img_out_f, img_temp, sizeof(float) * width_channel);

    float * __restrict in_factor = map_factor_a;
    memcpy(map_factor_b, in_factor, sizeof(float) * width);
	
    for (int y = 1; y < height; y++)
    {
        const uint8_t * __restrict tpy = &lum[(y - 1) * width];
        const uint8_t * __restrict tcy = &lum[y * width];
		
        float * __restrict xcy = &img_temp[y * width_channel];
        float * __restrict ypy = &img_out_f[(y - 1) * width_channel];
        float * __restrict ycy = &img_out_f[y * width_channel];

        float * __restrict xcf = &in_factor[y * width];
        float * __restrict ypf = &map_factor_b[(y - 1) * width];
        float * __restrict ycf = &map_factor_b[y * width];
		
        for (int x = 0; x < width; x++)
        {
            const uint8_t range_dist = abs((*tcy++) - (*tpy++));
            const float weight = range_table[range_dist];
            const float alpha_ = weight * alpha;
			
            for (int c = 0; c < channel; c++)
            {
                *ycy++ =
                	inv_alpha_ * (*xcy++) +
                	alpha_ * (*ypy++);
			}
			
            *ycf++ =
            	inv_alpha_ * (*xcf++) +
            	alpha_ * (*ypf++);
        }
    }
	
    const int h1 = height - 1;
    float * __restrict ycf = line_factor_a;
    float * __restrict ypf = line_factor_b;
    memcpy(ypf, &in_factor[h1 * width], sizeof(float) * width);
	
    for (int x = 0; x < width; x++)
        map_factor_b[h1 * width + x] = 0.5f * (map_factor_b[h1 * width + x] + ypf[x]);

    float * __restrict ycy = slice_factor_a;
    float * __restrict ypy = slice_factor_b;
    memcpy(ypy, &img_temp[h1 * width_channel], sizeof(float) * width_channel);
	
    int k = 0;
    for (int x = 0; x < width; x++)
    {
        for (int c = 0; c < channel; c++)
        {
            const int idx = (h1 * width + x) * channel + c;
            img_out_f[idx] = 0.5f * (img_out_f[idx] + ypy[k++]) / map_factor_b[h1 * width + x];
        }
    }

    for (int y = h1 - 1; y >= 0; y--)
    {
        const uint8_t * __restrict tpy = &lum[(y + 1) * width];
        const uint8_t * __restrict tcy = &lum[y * width];
        float * __restrict xcy = &img_temp[y * width_channel];
		
        float * __restrict ycy_ = ycy;
        float * __restrict ypy_ = ypy;
        float * __restrict out_ = &img_out_f[y * width_channel];

        float * __restrict xcf = &in_factor[y * width];
		
        float * __restrict ycf_ = ycf;
        float * __restrict ypf_ = ypf;
        float * __restrict factor_ = &map_factor_b[y * width];
		
        for (int x = 0; x < width; x++)
        {
            const uint8_t range_dist = abs((*tcy++) - (*tpy++));
            const float weight = range_table[range_dist];
            const float alpha_ = weight * alpha;

            const float fcc =
            	inv_alpha_ * (*xcf++) +
            	alpha_ * (*ypf_++);
			
            *ycf_++ = fcc;
            *factor_ = 0.5f * (*factor_ + fcc);

            for (int c = 0; c < channel; c++)
            {
                const float ycc =
                	inv_alpha_ * (*xcy++) +
                	alpha_ * (*ypy_++);
				
                *ycy_++ = ycc;
				
                *out_ = 0.5f * (*out_ + ycc) / (*factor_);
                out_++;
            }
			
            factor_++;
        }
		
        memcpy(ypy, ycy, sizeof(float) * width_channel);
        memcpy(ypf, ycf, sizeof(float) * width);
    }

    for (int i = 0; i < width_height_channel; ++i)
        img[i] = static_cast<uint8_t>(img_out_f[i]);

    if (is_buffer_internal)
        delete [] buffer;
}

template <int channel>
inline void recursive_bf(
    const uint8_t * img_in,
    const uint8_t * lum_in,
    uint8_t * img_out,
    const float sigma_spatial,
    const float sigma_range,
    const int width,
    const int height,
    float * buffer)
{
    for (int i = 0; i < width * height * channel; ++i)
        img_out[i] = img_in[i];
	
    _recursive_bf<channel>(
    	img_out,
    	lum_in,
    	sigma_spatial,
    	sigma_range,
    	width,
    	height,
    	buffer);
}

#endif // INCLUDE_RBF

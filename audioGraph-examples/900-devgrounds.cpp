	if (false)
	{
		// try loading an IRCAM sample set

		HRIRSampleSet sampleSet;

		loadHRIRSampleSet_Ircam("binaural/IRC_1057", sampleSet);
	}
	
	if (false)
	{
		// try loading an MIT sample set

		HRIRSampleSet sampleSet;

		loadHRIRSampleSet_Mit("binaural/MIT-HRTF-DIFFUSE", sampleSet);
		
		sampleSet.finalize();
	}
	
	if (false)
	{
		// try loading a CIPIC sample set
		
		HRIRSampleSet sampleSet;
		
		loadHRIRSampleSet_Cipic("binaural/CIPIC/subject147", sampleSet);
		
		sampleSet.finalize();
		
		if (true)
		{
			// save sample grid
			
			FILE * file = fopen("binaural/cipic_147.grid", "wb");
			
			if (file != nullptr)
			{
				debugTimerBegin("sampleGrid_save");
				
				if (sampleSet.sampleGrid.save(file) == false)
				{
					logError("failed to save sample grid");
				}
				
				debugTimerEnd("sampleGrid_save");
				
				fclose(file);
				file = nullptr;
			}
		}
		
		if (true)
		{
			// load sample grid
			
			HRIRSampleGrid sampleGrid;
			
			FILE * file = fopen("binaural/cipic_147.grid", "rb");
			
			if (file != nullptr)
			{
				debugTimerBegin("sampleGrid_load");
				
				if (sampleSet.sampleGrid.load(file) == false)
				{
					logError("failed to load sample grid");
				}
				
				debugTimerEnd("sampleGrid_load");
				
				fclose(file);
				file = nullptr;
			}
		}
		
		if (true)
		{
			// save sample set
			
			FILE * file = fopen("binaural/cipic_147", "wb");
			
			if (file != nullptr)
			{
				debugTimerBegin("sampleSet_save");
				
				if (sampleSet.save(file) == false)
				{
					logError("failed to save sample set");
				}
				
				debugTimerEnd("sampleSet_save");
				
				fclose(file);
				file = nullptr;
			}
		}
		
		if (true)
		{
			// load sample set
			
			HRIRSampleSet sampleSet;
			
			FILE * file = fopen("binaural/cipic_147", "rb");
			
			if (file != nullptr)
			{
				debugTimerBegin("sampleSet_load");
				
				if (sampleSet.load(file) == false)
				{
					logError("failed to load sample set");
				}
				
				debugTimerEnd("sampleSet_load");
				
				fclose(file);
				file = nullptr;
			}
		}
	}
	
	if (false)
	{
		// try loading a sample set and performing lookups

		HRIRSampleSet sampleSet;

		loadHRIRSampleSet_Mit("binaural/MIT-HRTF-DIFFUSE", sampleSet);
		
		sampleSet.finalize();
		
		for (int elevation = 0; elevation <= 180; elevation += 10)
		{
			for (int azimuth = 0; azimuth <= 360; azimuth += 10)
			{
				HRIRSampleData const * samples[3];
				float sampleWeights[3];
				
				if (sampleSet.lookup_3(elevation, azimuth, samples, sampleWeights) == false)
				{
					debugLog("sample set lookup failed! elevation=%d, azimuth=%d", elevation, azimuth);
				}
				else
				{
					debugTimerBegin("blend_hrir");
					
					HRIRSampleData weightedSample;
					
					blendHrirSamples_3(
						*samples[0], sampleWeights[0],
						*samples[1], sampleWeights[1],
						*samples[2], sampleWeights[2],
						weightedSample);
					
					debugTimerEnd("blend_hrir");
				}
			}
		}
	}
	
	{
		for (int i = 0; i < 100; ++i)
		{
			float elevation = random(-90.f, +90.f);
			float azimuth = random(-180.f, +180.f);
			float x, y, z;
			
			elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			cartesianToElevationAndAzimuth(x, y, z, elevation, azimuth);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			cartesianToElevationAndAzimuth(x, y, z, elevation, azimuth);
			debugLog("(%.2f, %.2f) -> (%.2f, %.2f, %.2f)", elevation, azimuth, x, y, z);
			
			debugLog("----");
		}
	}
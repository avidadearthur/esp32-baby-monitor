#include "zcr.h"

bool zcr(float* data, int N, int fs, int nBit)
{
	int i;
	int zcrcount = 0;
	float zcr;

	// iterate through the data and calculate the zero crossed count
	for (i = 0; i < N-1; i++)
	{
		num = data[i]*data[i+1];
		if (num < 0)
		{
			zcrcount++;
		}
	}
	// calculate the zero crossed rate by dividing the zero crossed count by the total number of samples
	zcr = (float)zcrcount/(float)N;
	// if zcr is greater than 0.07, then cry sound is detected
	if (zcr > 0.07)
	{
		return true;
	}

	return false;
}
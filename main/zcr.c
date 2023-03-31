#include "zcr.h"

// tag
static const char* TAG = "ZCR";

bool zcr(float* data, int N){
	int i;
	int zcrcount = 0;
	float zcr;

	// iterate through the data and calculate the zero crossed count
	for (i = 0; i < N-1; i++){
		float num = data[i]*data[i+1];
		if (num < 0)
		{
			zcrcount++;
			// no negative data. no use
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
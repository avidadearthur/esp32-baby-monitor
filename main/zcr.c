#include "zcr.h"

static const char* TAG = "ZCR";

bool zcr(float* data, int N, int fs){
	int i;
	int zcrcount = 0;
	float zcr;
	float acc_thr = 97965520.000000;

	// iterate through the data and calculate the zero crossed count
	for (i = 0; i < N-2; i++){
		float dx1 = data[i+1]-data[i];
		float dt = 1.0/(float)(fs);
		float v1 = dx1/dt;
		float dx2 = data[i+2]-data[i+1];
		float v2 = dx2/dt;
		float a = (v1-v2)/dt;
		if (a > acc_thr)
		{
			zcrcount++;
		}
	}
	// calculate the zero crossed rate by dividing the zero crossed count by the total number of samples
	zcr = (float)zcrcount/(float)N;
	ESP_LOGI(TAG, "zero crossing rate: %f \n", zcr);
	// if zcr is greater than 0.07, then cry sound is detected
	if (zcr > 0.003)
	{
		return true;
	}

	return false;
}
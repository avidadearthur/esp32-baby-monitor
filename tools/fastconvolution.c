#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h> 
#include <fftw3.h>

#include "fastconvolution.h"

int main(){ 

double duration_conv, duration_fastconv;
double *signal, *filter, *result_conv, *result_fastconv;
long N,M,P,length_signal;
double *w,*Wr,*Wi,*x,*y,*Xr,*Xi,*Yr,*Yi,sum; 
long k,n;
int algo;
clock_t start,finish;
int rank, howmany_rank;
long L1;
fftw_iodim dims[1], howmany_dims[1];
fftw_plan pFFT,pIFFT,pFFTw;
FILE   *fspeed,*fparameters,*fresult_conv,*fresult_fastconv,*fsignal,*ffilter;

/*Read parameters*/
fparameters=fopen("parameters.txt","r");
fscanf(fparameters,"%ld\n",&length_signal); //signal length
fscanf(fparameters,"%ld\n",&N); //FIR filter order = filter length - 1
fscanf(fparameters,"%ld\n",&M); //FFT size
fscanf(fparameters,"%ld\n",&P); //frame shift
fscanf(fparameters,"%d\n",&algo); //which algorithm to run ?
fclose(fparameters);

/*Memory allocation*/
signal=(double *) malloc(length_signal*sizeof(double));
filter=(double *) malloc((N+1)*sizeof(double));
result_conv=(double *) calloc(length_signal,sizeof(double));
result_fastconv=(double *) calloc(length_signal,sizeof(double));
w=(double *) fftw_malloc(M*sizeof(double)); 
x=(double *) fftw_malloc(M*sizeof(double)); 
y=(double *) fftw_malloc(M*sizeof(double)); 

/*initialize FFT*/
L1=(M/2)+1;
Wr=(double *) fftw_malloc(L1*sizeof(double));  /* Data vector (size L1) containing the real part of the zero-padded filter weights */
Wi=(double *) fftw_malloc(L1*sizeof(double));  /* Data vector (size L1) containing the imaginary part of the zero-padded filter weights */
Xr=(double *) fftw_malloc(L1*sizeof(double));  /* Data vector (size L1) containing the real part of the input signal buffer x*/
Xi=(double *) fftw_malloc(L1*sizeof(double));  /* Data vector (size L1) containing the imaginary part of the input signal buffer x */
Yr=(double *) fftw_malloc(L1*sizeof(double));  /* Data vector (size L1) containing the real part of the output signal buffer y */
Yi=(double *) fftw_malloc(L1*sizeof(double));  /* Data vector (size L1) containing the imaginary part of the output signal buffer y */
rank = 1;               /* Dimension of FFT */
dims[0].n = M;    /* Size of FFT */
dims[0].is = 1;         /* Input stride */
dims[0].os = 1;         /* Output stride */
howmany_rank = 1;  
howmany_dims[0].n = 1;  /* Number of transforms */
howmany_dims[0].is = 1; /* Input stride */
howmany_dims[0].os = 1; /* Output stride */
pFFTw=fftw_plan_guru_split_dft_r2c(rank,dims,howmany_rank,howmany_dims,w,Wr,Wi,FFTW_ESTIMATE);
pFFT=fftw_plan_guru_split_dft_r2c(rank,dims,howmany_rank,howmany_dims,x,Xr,Xi,FFTW_ESTIMATE);
pIFFT=fftw_plan_guru_split_dft_c2r(rank,dims,howmany_rank,howmany_dims,Yr,Yi,y,FFTW_ESTIMATE);

/*Read files*/
fsignal=fopen("signal.bin","rb");
fread(signal,sizeof(double),length_signal,fsignal);
fclose(fsignal);
ffilter=fopen("filter.bin","rb");
fread(filter,sizeof(double),N+1,ffilter);
fclose(ffilter);

/*Standard FIR filtering*/
start=clock();
if((algo==0)|(algo==2)){
	/*Add your standard FIR filtering code here*/
	for(k=N;k<length_signal;k++){
		result_conv[k] = 0;
		for (n = 0; n <= N; n++){
			result_conv[k] += filter[n]*signal[k-n];
		}
	}
}

/*Compute execution time standard FIR filtering code*/
finish=clock();
duration_conv=((double) finish-start)/CLOCKS_PER_SEC;  
fspeed=fopen("speed.txt","w");
fprintf(fspeed,"C code for standard FIR filtering lasted for %f seconds\n",duration_conv);

/*Write results standard FIR filtering to file*/
fresult_conv=fopen("result_conv.bin","wb");
fwrite(result_conv,sizeof(double),length_signal,fresult_conv);
fclose(fresult_conv);

/*Fast convolution*/
start=clock();
if((algo==1)|(algo==2)|(algo==-1)){
	/*Add your fast convolution based filtering code here */
	/*Zero pad filter[n] and store the zero-padded filter in array w */
	for (int i = 0; i < M ; i++){
		if (i < N){
			w[i] = filter[i];
		}
		else{
			w[i] = 0;
		}
	}

	/*Convert the zero-padded filter weights w[n] into the frequency domain*/
	fftw_execute_split_dft_r2c(pFFTw,w,Wr,Wi); /* Computes the real-input FFT of array w of length M. The results are stored in arrays Wr (real part) and Wi (imaginary part), which both have length M/2+1 */
	
	int frame = 0;

	for (k = 0; (k + M) <= length_signal ; k += P) {
		/*Select a frame of M samples from array signal and store in array x. Shift subsequent frames by P samples with respect to each other */
		for (n = 0; n < M; n++){
			x[n] = signal[k + n];	
		}
		/*Convert array x into the frequency domain*/
		fftw_execute_split_dft_r2c(pFFT, x, Xr, Xi); //Computes the real-input FFT of array x of length M. The results are stored in arrays Xr (real part) and Xi (imaginary part), which both have length M/2+1.

		/*Perform an element-wise multiplication in the frequency domain : Yr+jYi=(Wr+jWi)*(Xr+jXi). Mind the size of Xr, Xi, Yr, Yi, Wr and Wi (see above)*/
		for (n = 0; n < (M/2 + 1); n++){
			Yr[n] = Wr[n] * Xr[n] - Wi[n] * Xi[n];
			Yi[n] = Wi[n] * Xr[n] + Wr[n] * Xi[n];
		}

		/*Convert Yr+jYi into the time domain*/
		fftw_execute_split_dft_c2r(pIFFT, Yr, Yi, y);	//Computes the real-output IFFT of arrays Yr (real part) and Yi (imaginary part), which both have length M/2+1. The result is array y of length M.
														//The inverse FFT implementation fftw_execute_split_dft_c2r does not include the normalization by the FFT size.
														//Hence, you have to divide all the data in the output array by M yourself !!!!

		/*Select the last P samples of array y and store in result_fastconv at the right position.*/
		for (n = 0; n < P; n++){
			result_fastconv[(frame-1)*P +M +n] = y[M - P + n] / M;
		}

		frame++;
	}
}

/*Compute execution time fast convolution based filtering code*/
finish=clock();
duration_fastconv=((double) finish-start)/CLOCKS_PER_SEC; 
if (M>0){
	fprintf(fspeed,"C code for fast convolution based filtering lasted for %f seconds\n",duration_fastconv);
	fprintf(fspeed,"Complexity gain (standard FIR/fast conv) for filter order N=%ld is %f\n",N,duration_conv/duration_fastconv);
}
fclose(fspeed);

/*Write results fast convolution based filtering to file*/
fresult_fastconv=fopen("result_fastconv.bin","wb");
fwrite(result_fastconv,sizeof(double),length_signal,fresult_fastconv);
fclose(fresult_fastconv);

/*Free allocated memory blocks*/
free(signal);
free(filter);
free(result_conv);
free(result_fastconv);
fftw_free(w);
fftw_free(x);
fftw_free(y);
fftw_free(Wr);
fftw_free(Wi);
fftw_free(Xr);
fftw_free(Xi);
fftw_free(Yr);
fftw_free(Yi);
fftw_destroy_plan(pFFT);
fftw_destroy_plan(pFFTw);
fftw_destroy_plan(pIFFT);

return 0;
}
#include <iostream>
#include <cstdio>
#include <MLOpen.h>
#include <CL/cl.h>
#include "mloConvHost.hpp"

int main()
{

	// mlopenContext APIs
	mlopenHandle_t handle;
	mlopenCreate(&handle);
	cl_command_queue q;
	mlopenGetStream(handle, &q);

	// mlopenTensor APIs
	mlopenTensorDescriptor_t inputTensor;
	mlopenCreateTensorDescriptor(handle, &inputTensor);

	mlopenSet4dTensorDescriptor(handle,
			inputTensor,
			mlopenFloat,
			100,
			32,
			8,
			8);

	int n, c, h, w;
	int nStride, cStride, hStride, wStride;
	mlopenDataType_t dt;
	
	mlopenGet4dTensorDescriptor(handle,
			inputTensor,
			&dt,
			&n,
			&c,
			&h,
			&w,
			&nStride,
			&cStride,
			&hStride,
			&wStride);

	std::cout<<dt<<" (shoule be 1)\n";
	printf("%d %d %d %d %d %d %d %d (should be 100, 32, 8, 8, 1, 1, 1, 1)\n", n, c, h, w, nStride, cStride, hStride, wStride);

	mlopenTensorDescriptor_t convFilter;
	mlopenCreateTensorDescriptor(handle, &convFilter);

	// weights
	mlopenSet4dTensorDescriptor(handle,
		convFilter,
		mlopenFloat,
		64,  // outputs
		32,   // inputs
		5,   // kernel size
		5);

	int alpha = 1, beta = 1;
	mlopenTransformTensor(handle,
			&alpha,
			inputTensor,
			NULL,
			&beta,
			convFilter,
			NULL);

	int value = 10;
	mlopenSetTensor(handle, inputTensor, NULL, &value);

	mlopenScaleTensor(handle, inputTensor, NULL, &alpha);

	// mlopenConvolution APIs
	//

	mlopenConvolutionDescriptor_t convDesc;
	mlopenCreateConvolutionDescriptor(handle, &convDesc);

	mlopenConvolutionMode_t mode = mlopenConvolution;

// convolution with padding 2
	mlopenInitConvolutionDescriptor(convDesc,
			mode,
			2,
			2,
			1,
			1,
			1,
			1);

	int pad_w, pad_h, u, v, upx, upy;
	mlopenGetConvolutionDescriptor(convDesc,
			&mode,
			&pad_h, &pad_w, &u, &v,
			&upx, &upy);

	printf("%d %d %d %d %d %d %d (Should be 0, 2, 2, 1, 1, 1, 1)\n", mode, pad_h, pad_w, u, v, upx, upy);

	int x, y, z, a;
	mlopenGetConvolutionForwardOutputDim(convDesc, inputTensor, convFilter, &x, &y, &z, &a);

	printf("Output dims: %d, %d, %d, %d (Should be 100, 64, 8, 8)\n", x, y, z, a);

	mlopenTensorDescriptor_t outputTensor;
	mlopenCreateTensorDescriptor(handle, &outputTensor);

	mlopenSet4dTensorDescriptor(handle,
		outputTensor,
		mlopenFloat,
		x,
		y,
		z,
		a);

	int ret_algo_count;
	mlopenConvAlgoPerf_t perf;

	cl_int status;
#if 0 // Test to see if we can launch the kernel and get the results back
	float *a1 = new float[1024];
	float *b1 = new float[1024];
	float *c1 = new float[1024];

	for(int i = 0; i < 1024; i++) {
		a1[i] = 1.0;
		b1[i] = 6.0;
		c1[i] = 0.0;
	}
#endif
	int sz = 1024;
	cl_context ctx;
	clGetCommandQueueInfo(q, CL_QUEUE_CONTEXT, sizeof(cl_context), &ctx, NULL);

	cl_mem adev = clCreateBuffer(ctx, CL_MEM_READ_ONLY, 4*sz,NULL, &status);
	cl_mem bdev = clCreateBuffer(ctx, CL_MEM_READ_ONLY, 4*sz,NULL, NULL);
	cl_mem cdev = clCreateBuffer(ctx, CL_MEM_READ_WRITE, 4*sz,NULL, NULL);
#if 0
	status = clEnqueueWriteBuffer(q, adev, CL_TRUE, 0, 4*sz, a1, 0, NULL, NULL);
	status |= clEnqueueWriteBuffer(q, bdev, CL_TRUE, 0, 4*sz, b1, 0, NULL, NULL);
	status |= clEnqueueWriteBuffer(q, cdev, CL_TRUE, 0, 4*sz, c1, 0, NULL, NULL);
	if(status != CL_SUCCESS) 
		printf("error\n");
#endif // Test

	mlopenFindConvolutionForwardAlgorithm(handle,
			inputTensor,
			adev,
			convFilter,
			bdev,
			convDesc,
			outputTensor,
			cdev,
			1,
			&ret_algo_count,
			&perf,
			mlopenConvolutionFastest,
			NULL,
			10);

#if 0 // Read results back
	clEnqueueReadBuffer(q, cdev, CL_TRUE, 0, 4*sz, c1, 0, NULL, NULL);

	float sum = 0.0;
	for(int i = 0; i < 1024; i++) {
		b1[i] = 6;
		sum += c1[i];
	}

	printf("\nsum %f\n, ", sum);
	sum = 0.0;
	
	delete[] a1;
	delete[] b1;
	delete[] c1;
#endif //Results

	mlopenConvolutionForward(handle,
			&alpha,
			inputTensor,
			NULL,
			convFilter,
			NULL,
			convDesc,
			mlopenConvolutionFwdAlgoDirect,
			&beta,
			outputTensor,
			NULL,
			NULL,
			0);

	mlopenFindConvolutionBackwardDataAlgorithm(handle,
			inputTensor,
			adev,
			convFilter,
			bdev,
			convDesc,
			outputTensor,
			cdev,
			1,
			&ret_algo_count,
			&perf,
			mlopenConvolutionFastest,
			NULL,
			10);

	mlopenConvolutionBackwardData(handle,
			&alpha,
			inputTensor,
			NULL,
			convFilter,
			NULL,
			convDesc,
			mlopenConvolutionBwdDataAlgo_0,
			&beta,
			outputTensor,
			NULL,
			NULL,
			0);

	mlopenDestroyTensorDescriptor(outputTensor);
	mlopenDestroyTensorDescriptor(convFilter);
	mlopenDestroyTensorDescriptor(inputTensor);

	mlopenDestroyConvolutionDescriptor(convDesc);

	mlopenDestroy(handle);
	return 0;
}

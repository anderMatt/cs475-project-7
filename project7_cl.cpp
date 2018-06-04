/**************************************************
 * Matthew Anderson
 * CS 475 - Project 7B
 * 6/3/2018
**************************************************/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <omp.h>

#include "CL/cl.h"
#include "CL/cl_platform.h"


#ifndef NUM_ELEMENTS
#define	NUM_ELEMENTS		32768
#endif

#ifndef LOCAL_SIZE
#define	LOCAL_SIZE		32
#endif

#define	NUM_WORK_GROUPS		NUM_ELEMENTS/LOCAL_SIZE

const char *		CL_FILE_NAME = { "project7_autocorrelate.cl" };
const float			TOL = 0.0001f;

void			Wait( cl_command_queue );
int				LookAtTheBits( float );
void writeCSVHeaders(FILE *fp);


int
main( int argc, char *argv[ ] )
{
    // see if we can even open the opencl kernel program
    // (no point going on if we can't):

    FILE *fp;
#ifdef WIN32
    errno_t err = fopen_s( &fp, CL_FILE_NAME, "r" );
	if( err != 0 )
#else
    fp = fopen( CL_FILE_NAME, "r" );
    if( fp == NULL )
#endif
    {
        fprintf( stderr, "Cannot open OpenCL source file '%s'\n", CL_FILE_NAME );
        return 1;
    }

    cl_int status;		// returned status from opencl calls
    // test against CL_SUCCESS

    // get the platform id:

    cl_platform_id platform;
    status = clGetPlatformIDs( 1, &platform, NULL );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clGetPlatformIDs failed (2)\n" );

    // get the device id:

    cl_device_id device;
    status = clGetDeviceIDs( platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clGetDeviceIDs failed (2)\n" );

    // Load the signal data.
    FILE *fpSignalData = fopen("signal.txt", "r");
    if(fpSignalData == NULL) {
        fprintf(stderr, "Unable to open signal data file!");
        exit(1);
    }


    // 2. allocate the host memory buffers:

    float *hArr = new float[2 * NUM_ELEMENTS];
    float *hSums = new float[NUM_ELEMENTS];

    // fill the host memory buffers with signal data.

    for( int i = 0; i < NUM_ELEMENTS; i++ )
    {
        fscanf(fpSignalData, "%f", &hArr[i]);
    }

    // Device buffers.
    size_t dataSize = NUM_ELEMENTS * sizeof(float);

    // 3. create an opencl context:

    cl_context context = clCreateContext( NULL, 1, &device, NULL, NULL, &status );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clCreateContext failed\n" );

    // 4. create an opencl command queue:

    cl_command_queue cmdQueue = clCreateCommandQueue( context, device, 0, &status );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clCreateCommandQueue failed\n" );

    // 5. allocate the device memory buffers:

    cl_mem dArr = clCreateBuffer(context, CL_MEM_READ_ONLY, 2 * NUM_ELEMENTS * sizeof(cl_float), NULL, &status);
    if( status != CL_SUCCESS )
        fprintf( stderr, "clCreateBuffer failed (1)\n" );

    cl_mem dSums = clCreateBuffer(context, CL_MEM_WRITE_ONLY, NUM_ELEMENTS * sizeof(cl_float), NULL, &status);
    if( status != CL_SUCCESS )
        fprintf( stderr, "clCreateBuffer failed (2)\n" );


    // 6. enqueue the 2 commands to write the data from the host buffers to the device buffers:

    status = clEnqueueWriteBuffer( cmdQueue, dArr, CL_FALSE, 0, 2 * NUM_ELEMENTS * sizeof(cl_float), hArr, 0, NULL, NULL );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clEnqueueWriteBuffer failed (1)\n" );

    status = clEnqueueWriteBuffer( cmdQueue, dSums, CL_FALSE, 0, NUM_ELEMENTS * sizeof(cl_float), hSums, 0, NULL, NULL );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clEnqueueWriteBuffer failed (2)\n" );

    Wait( cmdQueue );

    // 7. read the kernel code from a file:

    fseek( fp, 0, SEEK_END );
    size_t fileSize = ftell( fp );
    fseek( fp, 0, SEEK_SET );
    char *clProgramText = new char[ fileSize+1 ];		// leave room for '\0'
    size_t n = fread( clProgramText, 1, fileSize, fp );
    clProgramText[fileSize] = '\0';
    fclose( fp );
    if( n != fileSize )
        fprintf( stderr, "Expected to read %d bytes read from '%s' -- actually read %d.\n", fileSize, CL_FILE_NAME, n );

    // create the text for the kernel program:

    char *strings[1];
    strings[0] = clProgramText;
    cl_program program = clCreateProgramWithSource( context, 1, (const char **)strings, NULL, &status );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clCreateProgramWithSource failed\n" );
    delete [ ] clProgramText;

    // 8. compile and link the kernel code:

    char *options = { "" };
    status = clBuildProgram( program, 1, &device, options, NULL, NULL );
    if( status != CL_SUCCESS )
    {
        size_t size;
        clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size );
        cl_char *log = new cl_char[ size ];
        clGetProgramBuildInfo( program, device, CL_PROGRAM_BUILD_LOG, size, log, NULL );
        fprintf( stderr, "clBuildProgram failed:\n%s\n", log );
        delete [ ] log;
    }

    // 9. create the kernel object:

    cl_kernel kernel = clCreateKernel( program, "AutoCorrelate", &status );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clCreateKernel failed\n" );

    // 10. setup the arguments to the kernel object:

    status = clSetKernelArg( kernel, 0, sizeof(cl_mem), &dArr );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clSetKernelArg failed (1)\n" );

    status = clSetKernelArg( kernel, 1, sizeof(cl_mem), &dSums );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clSetKernelArg failed (2)\n" );

    // 11. enqueue the kernel object for execution:

    size_t globalWorkSize[3] = { NUM_ELEMENTS, 1, 1 };
    size_t localWorkSize[3]  = { LOCAL_SIZE,   1, 1 };

    Wait( cmdQueue );

    double time0 = omp_get_wtime( );

    time0 = omp_get_wtime( );

    status = clEnqueueNDRangeKernel( cmdQueue, kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clEnqueueNDRangeKernel failed: %d\n", status );

    Wait( cmdQueue );

    double time1 = omp_get_wtime( );

    // 12. read the results buffer back from the device to the host:

    status = clEnqueueReadBuffer( cmdQueue, dSums, CL_TRUE, 0, NUM_ELEMENTS * sizeof(cl_float), hSums, 0, NULL, NULL );
    if( status != CL_SUCCESS )
        fprintf( stderr, "clEnqueueReadBuffer failed\n" );

    Wait( cmdQueue );

    // did it work?

//    for( int i = 0; i < NUM_ELEMENTS; i++ )
//    {
//        //float expected = hA[i] * hB[i];
//        float expected = hA[i] * hB[i];
//        if( fabs( hC[i] - expected ) > TOL )
//        {
//			fprintf( stderr, "%4d: %13.6f * %13.6f wrongly produced %13.6f instead of %13.6f (%13.8f)\n",
//				i, hA[i], hB[i], hC[i], expected, fabs(hC[i]-expected) );
//			fprintf( stderr, "%4d:    0x%08x *    0x%08x wrongly produced    0x%08x instead of    0x%08x\n",
//				i, LookAtTheBits(hA[i]), LookAtTheBits(hB[i]), LookAtTheBits(hC[i]), LookAtTheBits(expected) );
//        }
//    }

//	fprintf( stderr, "%8d\t%4d\t%10d\t%10.3lf GigaMultsPerSecond\n",
//		NUM_ELEMENTS, LOCAL_SIZE, NUM_WORK_GROUPS, (double)NUM_ELEMENTS/(time1-time0)/1000000000. );
    printf("%d,%d,%10.3lf\n", NUM_ELEMENTS, LOCAL_SIZE, double(NUM_ELEMENTS/(time1 - time0)/1000000));
    double elapsedTime = time1 - time0;
    double performance = (double)NUM_ELEMENTS * NUM_ELEMENTS / elapsedTime / 1000000;

    FILE *fpData = fopen("opencl_data.csv", "w");
    writeCSVHeaders(fpData);
    fprintf(fpData, "%lf\n", performance);

    // Record the sums for graphing
    fprintf(fpData, "Index,Sum\n");
    for(int i = 0; i < NUM_ELEMENTS; i++) {
        fprintf(fpData, "%d,%lf\n", i, hSums[i]);
    }

    fclose(fpData);


    // TODO: write results to file.

#ifdef WIN32
    Sleep( 2000 );
#endif


    // 13. clean everything up:

    clReleaseKernel(        kernel   );
    clReleaseProgram(       program  );
    clReleaseCommandQueue(  cmdQueue );
    clReleaseMemObject(     dArr  );
    clReleaseMemObject(     dSums  );

    delete [ ] hArr;
    delete [ ] hSums;

    return 0;
}


// wait until all queued tasks have completed:

void
Wait( cl_command_queue queue )
{
    cl_event wait;

    cl_int status = clEnqueueMarker( queue, &wait );
    if( status != CL_SUCCESS )
        fprintf( stderr, "Wait: clEnqueueMarker failed\n" );

    status = clEnqueueWaitForEvents( queue, 1, &wait );
    if( status != CL_SUCCESS )
        fprintf( stderr, "Wait: clEnqueueWaitForEvents failed\n" );
}


int
LookAtTheBits( float fp )
{
    int *ip = (int *)&fp;
    return *ip;
}

void writeCSVHeaders(FILE *fp) {
    char *header = "Performance(MegaMultiply-Sums per Second)";
    fprintf(fp, header);
}


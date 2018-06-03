/**************************************************
 * Matthew Anderson
 * CS 475 - Project 7B
 * 6/3/2018
**************************************************/

#include <stdio.h>
#include <cstdlib>
#include <omp.h>

#define SIZE 32768

// Headers
void autocorrelate(float *arr, float *sums, int size, int numThreads, FILE *fpOut);
void writeCSVHeaders(FILE *fp);

int main() {
//#ifndef _OPENMP
//    fprintf(stderr, "OpenMP is not supported. Exiting...\n");
//    exit(1)
//#endif
   float Array[2*SIZE] ;
    float Sums[SIZE];
    int Size = SIZE;
    int i;

    // Read in the signal data.
    FILE *fp = fopen("signal.txt", "r");
    if(fp == NULL) {
        fprintf(stderr, "Cannot open file 'signal.txt'\n");
        exit(1);
    }
    fscanf(fp, "%d", &Size);

    for(i = 0; i < Size; i++) {
        fscanf(fp, "%f", &Array[i]);
        Array[i+Size] = Array[i];  // Duplicate the array.
    }
    fclose(fp);

    // Run the autocorrelations - results will be written to the CSV files.
    FILE *fpData = fopen("omp_threads_1.csv", "w");
    writeCSVHeaders(fpData);

    autocorrelate(Array, Sums, Size, 1, fpData);
    fclose(fpData);

    fpData = fopen("omp_threads_4.csv", "w");
    writeCSVHeaders(fpData);

    autocorrelate(Array, Sums, Size, 4, fpData);
    fclose(fpData);

    return 0;
}  // End of main.

void autocorrelate(float *arr, float *sums, int size, int numThreads, FILE *fpOut) {
    omp_set_num_threads(numThreads);

    double start = omp_get_wtime();

    for(int shift = 0; shift < size; shift++) {
        float sum = 0.;
        for(int i = 0; i < size; i++) {
            sum += arr[i] * arr[i + shift];
        }
        sums[shift] = sum;
    }

    double end = omp_get_wtime();
    double elapsedTime = end - start;

    // Performance is MegaMultiply-Sums per second.
    // We do the multiply and sum (size * size) times.
    double performance = ((double)size * size) / elapsedTime / 1000000;
    fprintf(fpOut, "%d,%lf\n", numThreads, performance);

    // Write the sum at each index for graphing.
    fprintf(fpOut, "Index,Sum\n");
    for(int i = 0; i < size; i++ ){
        fprintf(fpOut, "%d,%lf\n", i, sums[i]);
        // TODO: Important! Only need to graph [1, 512].
    }
} // End of autocorrelate.

void writeCSVHeaders(FILE *fp) {
    char *header = "Threads,Performance(MegaMultiply-Sums per Second)";
    fprintf(fp, header);
}

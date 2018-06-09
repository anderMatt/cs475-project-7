/**************************************************
 * Matthew Anderson
 * CS 475 - Project 7B
 * 6/10/2018
**************************************************/

#include <omp.h>
#include <stdio.h>

#include "simd.p5.h"

#define SIZE 32768

void autocorrelate(float *arr, float *sums, int size, FILE *fpOut);

int main(int argn, char **argv) {
#ifndef _OPENMP
    fprintf(stderr, "OpenMP is not supported. Exiting...\n");
    exit(1);
#endif

    // Read in the signal data.
    FILE *fp = fopen("signal.txt", "r");
    if(fp == NULL) {
        fprintf(stderr, "Cannot open file 'signal.txt'\n");
        exit(1);
    }

    int size = SIZE;
    float arr[2*size];
    float sums[size];
    int i;

    for(int i = 0; i < size; i++) {
        fscanf(fp, "%f", &arr[i]);
        arr[i+size] = arr[i];  // Duplicate.
    }

    fclose(fp);

    // Run the simulation.
    double start = omp_get_wtime();

    for(int shift = 0; shift < size; shift++) {
        sums[shift] = SimdMulSum(arr, &arr[shift], size);
    }

    double end = omp_get_wtime();
    double elapsedTime = end - start;
    double performance = (double)size * size / elapsedTime / 1000000;


    // Record the results.
    FILE *fpData = fopen("simd_data.csv", "w");
    fprintf(fpData, "Performance(MegaMultiply-Sums per Second)");
    fprintf(fpData, "%lf", performance);

    fprintf(fpData, "Index,Sum\n");
    for(int i = 0; i < size; i++) {
        fprintf(fpData, "%d,%lf\n", i, sums[i]);
        // TODO: Only need to graph [1,512]!
    }

    fclose(fpData);

    return 0;
}  // End of main.
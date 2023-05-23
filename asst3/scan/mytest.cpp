
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string>
#include <cstring>
#include <iostream>

void show_array(std::string msg, int *arr, int n){
    std::cout << msg << std::endl;
    for(int i = 0; i < n; i++){
        printf("%d, ", arr[i]);
    }
    printf("\n");
}
void cpu_exclusive_scan2(int* start, int* end, int* output) {

#define PARALLEL 1
#ifdef PARALLEL

    // note to students: this C code can be helpful when debugging the
    // output of intermediate steps of your CUDA segmented scan.
    // Uncomment the line abbove to use it as a reference.w
  
    int N = end - start;
    memmove(output, start, N*sizeof(int));
    
    // upsweep phase
    for (int twod = 1; twod < N/2; twod*=2) {
        int twod1 = twod*2;
    
        for (int i = 0; i < N; i += twod1) {
        output[i+twod1-1] = output[i+twod-1] + output[i+twod1-1];
        }
    }

    output[N-1] = 0;

    // downsweep phase
    for (int twod = N/2; twod >= 1; twod /= 2) {
        int twod1 = twod*2;
        for (int i = 0; i < N; i += twod1) {
            int tmp = output[i+twod-1];
            output[i+twod-1] = output[i+twod1-1];
            output[i+twod1-1] = tmp + output[i+twod1-1];
        }
    }

#else    
    int N = end - start;
    output[0] = 0;
    for (int i = 1; i < N; i++) {
        output[i] = output[i-1] + start[i-1];
    }
#endif
}


void cpu_exclusive_scan(int* start, int* end, int* output)
{
    int N = end - start;
    memmove(output, start, N*sizeof(int));
    // upsweep phase.


show_array("output 1:", output, N);
    for (int twod = 1; twod < N; twod*=2)
    {
         int twod1 = twod*2;
         for (int i = 0; i < N; i += twod1)
         {
             output[i+twod1-1] += output[i+twod-1];
         }
    }

    output[N-1] = 0;

    // downsweep phase.
    for (int twod = N/2; twod >= 1; twod /= 2)
    {
         int twod1 = twod*2;
         for (int i = 0; i < N; i += twod1)
         {
             int t = output[i+twod-1];
             output[i+twod-1] = output[i+twod1-1];
             output[i+twod1-1] += t; // change twod1 to twod to reverse prefix sum.
         }
    }
}



int main() {
    int arr[]={1, 2, 3, 4};
    int output[4]={0};
    cpu_exclusive_scan2(arr, arr+4, output);
    for(int i = 0; i < 4; i++){
        printf("%d, ", output[i]);
    }
     printf("\n");
}
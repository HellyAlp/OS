#include <iostream>
#include <sys/time.h>
#include "osm.h"
#define MICRO_TO_NANO 1000
#define MICRO_TO_SECONDS 1000000
#define ERROR -1

/**
 * Time measurement function for an arithmetic operation.
 * @param iterations number of iterations to measure
 * @return Time in nano seconds
 */
double osm_operation_time(unsigned int iterations)
{
    timeval start{}, end{};
    unsigned int i;
    int start_success=gettimeofday(&start, nullptr);
    for (i=0; i<iterations/10; i++)
    {
        2+2;
        2+2;
        2+2;
        2+2;
        2+2;
        2+2;
        2+2;
        2+2;
        2+2;
        2+2;
    }
    int end_success=gettimeofday(&end, nullptr);
    if (start_success == ERROR || end_success == ERROR||iterations==0)
    {
        return ERROR;
    }
    size_t sec_diff=(double)(end.tv_sec - start.tv_sec);
    size_t microsec_diff =(double)((end.tv_usec - start.tv_usec) );
    size_t diff = double (sec_diff*MICRO_TO_SECONDS + microsec_diff);
    return ((double)(diff*MICRO_TO_NANO)/iterations);
}

/**
 * empty func for the osm_function_time
 */
void empty_func()
{

}

/**
 * Time measurement function for an empty function call.
 * @param iterations number of iterations to measure
 * @return Time in nano seconds
 */
double osm_function_time(unsigned int iterations)
{
    timeval start{}, end{};
    unsigned int i;
    int start_success=gettimeofday(&start, nullptr);
    for (i=0; i<iterations/10; i++)
    {
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();
        empty_func();

    }
    int end_success=gettimeofday(&end, nullptr);
    if (start_success == ERROR || end_success == ERROR||iterations==0)
    {
        return ERROR;
    }
    size_t sec_diff=(double)(end.tv_sec - start.tv_sec);
    size_t microsec_diff =(double)((end.tv_usec - start.tv_usec) );
    size_t diff = double (sec_diff*MICRO_TO_SECONDS + microsec_diff);
    return ((double)(diff*MICRO_TO_NANO)/iterations);
}


/**
 * Time measurement function for an empty trap.
 * @param iterations number of iterations to measure
 * @return Time in nano seconds
 */
double osm_syscall_time(unsigned int iterations)
{
    timeval start{}, end{};
    unsigned int i;
    int start_success=gettimeofday(&start, nullptr);
    for (i=0; i<iterations/10; i++)
    {
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;
        OSM_NULLSYSCALL;

    }
    int end_success=gettimeofday(&end, nullptr);
    if (start_success == ERROR || end_success == ERROR||iterations==0)
    {
        return ERROR;
    }
    size_t sec_diff=(double)(end.tv_sec - start.tv_sec);
    size_t microsec_diff =(double)((end.tv_usec - start.tv_usec) );
    size_t diff = double (sec_diff*MICRO_TO_SECONDS + microsec_diff);
    return ((double)(diff*MICRO_TO_NANO)/iterations);
}

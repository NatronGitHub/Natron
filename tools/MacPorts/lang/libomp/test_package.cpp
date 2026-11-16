#include <omp.h>

#include <iostream>

int main()
{
    int num_threads = std::max(5, omp_get_num_procs());
    omp_set_num_threads(num_threads);
    int actual_number;
    #pragma omp parallel
    {
       #pragma omp single
       {
          actual_number = omp_get_num_threads();
       }
    }
    if(actual_number != num_threads){
        std::cout << "Something went wrong. Expecting " << num_threads << " threads but found " << actual_number << ".\n";
        std::cout << "There are probably missing compiler flags.\n";
        return 1;
    }
    return 0;
}

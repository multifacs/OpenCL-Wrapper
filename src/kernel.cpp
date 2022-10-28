#include "kernel.hpp"  // note: unbalanced round brackets () are not allowed and string literals can't be arbitrarily long, so periodically interrupt with )+R(
string opencl_c_container() {
  return R(  // ########################## begin of OpenCL C code
             // ####################################################################

      kernel void kernel1() {  // equivalent to "for(uint n=0u; n<N; n++) {",
                               // but executed in parallel
        const uint N = get_group_id(0);
        const uint M = get_local_id(0);
        const uint K = get_global_id(0);
        printf("block\t%d\tthread\t%d\tglobalindex\t%d\n", N, M, K);
      }

      kernel void kernel2(global float* A) {
        const uint n = get_global_id(0);
        A[n] = A[n] + n;
      }

  );
}  // ############################################################### end of
   // OpenCL C code
   // #####################################################################
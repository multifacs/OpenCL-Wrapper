#include <iomanip>

#include "opencl.hpp"

int main() {
  Device device(
      select_device_with_most_flops());  // compile OpenCL C code for the
                                         // fastest available device

  const uint N = 20u;
  const uint M = 5u;

  Kernel kernel1(device, N, M, "kernel1");
  std::cout << "\nTASK 1\n";
  kernel1.run();

  Memory<float> A(device, N);  // allocate memory on both host and device
  Kernel kernel2(device, N, "kernel2", A);  // kernel that runs on the device
  for (uint n = 0u; n < N; n++) {
    A[n] = 0.0f;  // initialize memory
  }
  A.write_to_device();   // copy data from host memory to device memory
  kernel2.run();         // run add_kernel on the device
  A.read_from_device();  // copy data from device memory to host memory

  std::cout << "\nTASK 2\n";
  for (uint i = 0; i < N / M; i++) {
    for (uint j = 0; j < M; j++) {
      std::cout << std::left << std::setw(5) << A[i * (N / M) + j] << " ";
    }
    std::cout << "\n";
  }

  wait();
  return 0;
}

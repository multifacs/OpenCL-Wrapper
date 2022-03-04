#pragma once

#define WORKGROUP_SIZE 32
//#define PTX
//#define LOG
//#define USE_OPENCL_1_1

#ifdef USE_OPENCL_1_1
#define CL_USE_DEPRECATED_OPENCL_1_1_APIS
#endif // USE_OPENCL_1_1
#define to_string to_string_old // use own to_string methods
#include <CL/cl.hpp> // old version (OpenCL 1.0, 1.1, 1.2)
#undef to_string // use own to_string methods
#include "utilities.hpp"

struct Device_Info {
	cl::Device cl_device;
	string name, vendor; // device name, vendor
	string driver_version, opencl_c_version; // device driver version, OpenCL C version
	uint memory=0u; // global memory in MB
	uint memory_used=0u; // track global memory usage in MB
	uint global_cache=0u, local_cache=0u; // global cache in KB, local cache in KB
	uint max_global_buffer=0u, max_constant_buffer=0u; // maximum global buffer size in MB, maximum constant buffer size in KB
	uint compute_units=0u; // compute units (CUs) can contain multiple cores depending on the microarchitecture
	uint clock_frequency=0u; // in MHz
	bool is_cpu=false, is_gpu=false;
	uint is_fp64_capable=0u, is_fp32_capable=0u, is_fp16_capable=0u, is_int64_capable=0u, is_int32_capable=0u, is_int16_capable=0u, is_int8_capable=0u;
	uint cores=0u; // for CPUs, compute_units is the number of threads (twice the number of cores with hyperthreading)
	float tflops=0.0f; // estimated device floating point performance in TeraFLOPs/s
	inline Device_Info(const cl::Device& cl_device) {
		this->cl_device = cl_device; // see https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clGetDeviceInfo.html
		name = trim(cl_device.getInfo<CL_DEVICE_NAME>()); // device name
		vendor = trim(cl_device.getInfo<CL_DEVICE_VENDOR>()); // device vendor
		driver_version = trim(cl_device.getInfo<CL_DRIVER_VERSION>()); // device driver version
		opencl_c_version = trim(cl_device.getInfo<CL_DEVICE_OPENCL_C_VERSION>()); // device OpenCL C version
		memory = (uint)(cl_device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>()/1048576ull); // global memory in MB
		global_cache = (uint)(cl_device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>()/1024ull); // global cache in KB
		local_cache = (uint)(cl_device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>()/1024ull); // local cache in KB
		max_global_buffer = (uint)(cl_device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>()/1048576ull); // maximum global buffer size in MB
		max_constant_buffer = (uint)(cl_device.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>()/1024ull); // maximum constant buffer size in KB
		compute_units = (uint)cl_device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(); // compute units (CUs) can contain multiple cores depending on the microarchitecture
		clock_frequency = (uint)cl_device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>(); // in MHz
		is_fp64_capable = (uint)cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE>();
		is_fp32_capable = (uint)cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT>();
		is_fp16_capable = (uint)cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF>();
		is_int64_capable = (uint)cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG>();
		is_int32_capable = (uint)cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_INT>();
		is_int16_capable = (uint)cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT>();
		is_int8_capable = (uint)cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR>();
		is_cpu = cl_device.getInfo<CL_DEVICE_TYPE>()==CL_DEVICE_TYPE_CPU;
		is_gpu = cl_device.getInfo<CL_DEVICE_TYPE>()==CL_DEVICE_TYPE_GPU;
		const uint ipc = is_gpu?2u:32u; // IPC (instructions per cycle) is 2 for GPUs and 32 for most modern CPUs
		const bool nvidia_192_cores_per_cu = contains_any(to_lower(name), {" 6", " 7", "ro k", "la k"}) || (clock_frequency<1000u&&contains(to_lower(name), "titan")); // identify Kepler GPUs
		const bool nvidia_64_cores_per_cu = contains_any(to_lower(name), {"p100", "v100", "a100", "a30", " 16", " 20", "titan v", "titan rtx", "ro t", "la t", "ro rtx"}) && !contains(to_lower(name), "rtx a"); // identify P100, Volta, Turing, A100
		const bool amd_128_cores_per_dualcu = contains(to_lower(name), "gfx10"); // identify RDNA/RDNA2 GPUs where dual CUs are reported
		const float nvidia = (float)(contains(to_lower(vendor), "nvidia"))*(nvidia_192_cores_per_cu?192.0f:(nvidia_64_cores_per_cu?64.0f:128.0f)); // Nvidia GPUs have 192 cores/CU (Kepler), 128 cores/CU (Maxwell, Pascal, Ampere) or 64 cores/CU (P100, Volta, Turing, A100)
		const float amd = (float)(contains_any(to_lower(vendor), {"amd", "advanced"}))*(is_gpu?(amd_128_cores_per_dualcu?128.0f:64.0f):0.5f); // AMD GPUs have 64 cores/CU (GCN, CDNA) or 128 cores/dualCU (RDNA, RDNA2), AMD CPUs (with SMT) have 1/2 core/CU
		const float intel = (float)(contains(to_lower(vendor), "intel"))*(is_gpu?8.0f:0.5f); // Intel integrated GPUs usually have 8 cores/CU, Intel CPUs (with HT) have 1/2 core/CU
		const float arm = (float)(contains(to_lower(vendor), "arm"))*(is_gpu?8.0f:1.0f); // ARM GPUs usually have 8 cores/CU, ARM CPUs have 1 core/CU
		cores = to_uint((float)compute_units*(nvidia+amd+intel+arm)); // for CPUs, compute_units is the number of threads (twice the number of cores with hyperthreading)
		tflops = 1E-6f*(float)cores*(float)ipc*(float)clock_frequency; // estimated device floating point performance in TeraFLOPs/s
	}
	inline Device_Info() {}; // default constructor
};

string get_opencl_c_code(); // implemented in kernel.hpp
inline void print_device_info(const Device_Info& d, const int id=-1) { // print OpenCL device info
	println("\r|----------------.--------------------------------------------------|");
	if(id>-1) println("| Device ID      | "+alignl(48, to_string(id))+" |");
	println("| Device Name    | "+alignl(48, d.name                 )+" |");
	println("| Device Vendor  | "+alignl(48, d.vendor               )+" |");
	println("| Device Driver  | "+alignl(48, d.driver_version       )+" |");
	println("| OpenCL Version | "+alignl(48, d.opencl_c_version     )+" |");
	println("| Compute Units  | "+alignl(48, to_string(d.compute_units)+" at "+to_string(d.clock_frequency)+" MHz ("+to_string(d.cores)+" cores, "+to_string(d.tflops, 3)+" TFLOPs/s)")+" |");
	println("| Memory, Cache  | "+alignl(48, to_string(d.memory)+" MB, "+to_string(d.global_cache)+" KB global / "+to_string(d.local_cache)+" KB local")+" |");
	println("| Max Alloc Size | "+alignl(48, to_string(d.max_global_buffer)+" MB global, "+to_string(d.max_constant_buffer)+" KB constant")+" |");
	println("|----------------'--------------------------------------------------|");
}
inline vector<Device_Info> get_devices() { // returns a vector of all available OpenCL devices
	vector<Device_Info> devices; // get all devices of all platforms
	vector<cl::Platform> cl_platforms; // get all platforms (drivers)
	cl::Platform::get(&cl_platforms);
	for(uint i=0u; i<(uint)cl_platforms.size(); i++) {
		vector<cl::Device> cl_devices;
		cl_platforms[i].getDevices(CL_DEVICE_TYPE_ALL, &cl_devices);
		for(uint j=0u; j<(uint)cl_devices.size(); j++) {
			devices.push_back(Device_Info(cl_devices[j]));
		}
	}
	if((uint)cl_platforms.size()==0u||(uint)devices.size()==0u) {
		print_error("There are no OpenCL devices available. Make sure that the OpenCL 1.2 Runtime for your device is installed. For GPUs it comes by default with the graphics driver, for CPUs it has to be installed separately.");
	}
	println("\r|----------------.--------------------------------------------------|");
	for(uint i=0u; i<(uint)devices.size(); i++) {
		println("| Device ID "+alignr(4u, i)+" | "+alignl(48u, devices[i].name)+" |");
	}
	println("|----------------'--------------------------------------------------|");
	return devices;
}
inline Device_Info select_device_with_most_flops(const vector<Device_Info>& devices=get_devices()) { // returns device with best floating-point performance
	float best_value = 0.0f;
	uint best_i = 0u;
	for(uint i=0u; i<(uint)devices.size(); i++) { // find device with highest (estimated) floating point performance
		if(devices[i].tflops>best_value) {
			best_value = devices[i].tflops;
			best_i = i;
		}
	}
	print_device_info(devices[best_i], best_i);
	return devices[best_i];
}
inline Device_Info select_device_with_most_memory(const vector<Device_Info>& devices=get_devices()) { // returns device with largest memory capacity
	uint best_value = 0u;
	uint best_i = 0u;
	for(uint i=0u; i<(uint)devices.size(); i++) { // find device with most memory
		if(devices[i].memory>best_value) {
			best_value = devices[i].memory;
			best_i = i;
		}
	}
	print_device_info(devices[best_i], best_i);
	return devices[best_i];
}
inline Device_Info select_device_with_id(const uint id, const vector<Device_Info>& devices=get_devices()) { // returns device with specified ID
	if(id<(uint)devices.size()) {
		print_device_info(devices[id], id);
		return devices[id];
	} else {
		print_error("Your selected device ID ("+to_string(id)+") is wrong. Check the setting \"#define DEVICE x\" in defines.hpp.");
		return devices[0]; // is never executed, just to avoid compiler warnings
	}
}

class Device {
private:
	cl::Context cl_context;
	cl::Program cl_program;
	cl::CommandQueue cl_queue;
	inline string enable_device_capabilities() const { return // enable FP64/FP16 capabilities if available
		"\n	#define def_workgroup_size "+to_string(WORKGROUP_SIZE)+"u"
		"\n	#ifdef cl_khr_fp64"
		"\n	#pragma OPENCL EXTENSION cl_khr_fp64 : enable" // make sure cl_khr_fp64 extension is enabled
		"\n	#endif"
		"\n	#ifdef cl_khr_fp16"
		"\n	#pragma OPENCL EXTENSION cl_khr_fp16 : enable" // make sure cl_khr_fp16 extension is enabled
		"\n	#endif"
		"\n	#ifdef cl_khr_int64_base_atomics"
		"\n	#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable" // make sure cl_khr_int64_base_atomics extension is enabled
		"\n	#endif"
	;}
public:
	Device_Info info;
	inline Device(const Device_Info& info, const string& opencl_c_code=get_opencl_c_code()) {
		this->info = info;
		cl_context = cl::Context(info.cl_device);
		cl_queue = cl::CommandQueue(cl_context, info.cl_device); // queue to push commands for the device
		cl::Program::Sources cl_source;
		const string kernel_code = enable_device_capabilities()+"\n"+opencl_c_code;
		cl_source.push_back({ kernel_code.c_str(), kernel_code.length() });
		cl_program = cl::Program(cl_context, cl_source);
#ifndef LOG
		int error = cl_program.build("-cl-fast-relaxed-math -w"); // compile OpenCL C code, disable warnings
		if(error) print_warning(cl_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(info.cl_device)); // print build log
#else // LOG, generate logfile for OpenCL code compilation
		int error = cl_program.build("-cl-fast-relaxed-math"); // compile OpenCL C code
		const string log = cl_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(info.cl_device);
		write_file("bin/kernel.log", log); // save build log
		if((uint)log.length()>2u) print_warning(log); // print build log
#endif // LOG
		if(error) print_error("OpenCL C code compilation failed. Make sure there are no errors in kernel.cpp (\"#define LOG\" might help). If your GPU is old, try uncommenting \"#define USE_OPENCL_1_1\".");
		else print_info("OpenCL C code successfully compiled.");
#ifdef PTX // generate assembly (ptx) file for OpenCL code
		write_file("bin/kernel.ptx", cl_program.getInfo<CL_PROGRAM_BINARIES>()[0]); // save binary (ptx file)
#endif // PTX
	}
	inline Device() {} // default constructor
	inline cl::Context get_cl_context() const {
		return cl_context;
	}
	inline cl::Program get_cl_program() const {
		return cl_program;
	}
	inline cl::CommandQueue get_cl_queue() const {
		return cl_queue;
	}
};

template<typename T> class Memory {
private:
	ulong N = 0ull; // buffer length
	uint d = 1u; // buffer dimensions
	bool host_buffer_exists = false;
	bool device_buffer_exists = false;
	T* host_buffer = nullptr; // host buffer
	cl::Buffer device_buffer; // device buffer
	Device* device = nullptr; // pointer to linked Device
	cl::CommandQueue cl_queue; // command queue
	void initialize_auxiliary_pointers() {
		x = s0 = host_buffer;
		if(d>0x1u) y = s1 = host_buffer+N;
		if(d>0x2u) z = s2 = host_buffer+N*0x2ull;
		if(d>0x3u) w = s3 = host_buffer+N*0x3ull;
		if(d>0x4u) s4 = host_buffer+N*0x4ull;
		if(d>0x5u) s5 = host_buffer+N*0x5ull;
		if(d>0x6u) s6 = host_buffer+N*0x6ull;
		if(d>0x7u) s7 = host_buffer+N*0x7ull;
		if(d>0x8u) s8 = host_buffer+N*0x8ull;
		if(d>0x9u) s9 = host_buffer+N*0x9ull;
		if(d>0xAu) sA = host_buffer+N*0xAull;
		if(d>0xBu) sB = host_buffer+N*0xBull;
		if(d>0xCu) sC = host_buffer+N*0xCull;
		if(d>0xDu) sD = host_buffer+N*0xDull;
		if(d>0xEu) sE = host_buffer+N*0xEull;
		if(d>0xFu) sF = host_buffer+N*0xFull;
	}
	void allocate_device_buffer(Device& device, const bool allocate_device) {
		this->device = &device;
		this->cl_queue = device.get_cl_queue();
		if(allocate_device) {
			device.info.memory_used += (uint)(capacity()/1048576ull); // track device memory usage
			if(device.info.memory_used>device.info.memory) print_error("Device \""+device.info.name+"\" does not have enough memory. Allocating another "+to_string((uint)(capacity()/1048576ull))+" MB would use a total of "+to_string(device.info.memory_used)+" MB / "+to_string(device.info.memory)+" MB.");
			int error = 0;
			device_buffer = cl::Buffer(device.get_cl_context(), CL_MEM_READ_WRITE, capacity(), nullptr, &error);
			if(error) print_error("OpenCL Buffer allocation failed with error code "+to_string(error)+".");
			device_buffer_exists = true;
		}
	}
public:
	T *x=nullptr, *y=nullptr, *z=nullptr, *w=nullptr; // host buffer auxiliary pointers for multi-dimensional array access (array of structures)
	T *s0=nullptr, *s1=nullptr, *s2=nullptr, *s3=nullptr, *s4=nullptr, *s5=nullptr, *s6=nullptr, *s7=nullptr, *s8=nullptr, *s9=nullptr, *sA=nullptr, *sB=nullptr, *sC=nullptr, *sD=nullptr, *sE=nullptr, *sF=nullptr;
	inline Memory(Device& device, const ulong N, const uint dimensions=1u, const bool allocate_host=true, const bool allocate_device=true, const T value=(T)0) {
		if(N*(ulong)d==0ull) print_error("Memory size must be larger than 0.");
		this->N = N;
		this->d = dimensions;
		allocate_device_buffer(device, allocate_device);
		if(allocate_host) {
			host_buffer = new T[N*(ulong)d];
			for(ulong i=0ull; i<N*(ulong)d; i++) host_buffer[i] = value;
			initialize_auxiliary_pointers();
			host_buffer_exists = true;
		}
		write_to_device();
	}
	inline Memory(Device& device, const ulong N, const uint dimensions, T* const host_buffer, const bool allocate_device=true) {
		if(N*(ulong)d==0ull) print_error("Memory size must be larger than 0.");
		this->N = N;
		this->d = dimensions;
		allocate_device_buffer(device, allocate_device);
		this->host_buffer = host_buffer;
		initialize_auxiliary_pointers();
		host_buffer_exists = true;
		write_to_device();
	}
	inline Memory() {} // default constructor
	inline ~Memory() {
		delete[] host_buffer;
		device_buffer = nullptr;
		if(device_buffer_exists) device->info.memory_used -= (uint)(capacity()/1048576ull); // track device memory usage
	}
	inline Memory& operator=(Memory&& memory) noexcept { // move assignment
		delete[] host_buffer; // delete existing buffers
		device_buffer = nullptr;
		if(device_buffer_exists) device->info.memory_used -= (uint)(capacity()/1048576ull); // track device memory usage
		host_buffer = nullptr;
		host_buffer_exists = false;
		device_buffer_exists = false;
		N = memory.length(); // copy values/pointers from memory
		d = memory.dimensions();
		device = memory.device;
		cl_queue = memory.device->get_cl_queue();
		if(memory.device_buffer_exists) {
			device_buffer = memory.get_cl_buffer();
			memory.delete_device_buffer(); // set memory.device_buffer_exists = false; for correct memory tracking
			device_buffer_exists = true;
		}
		if(memory.host_buffer_exists) {
			host_buffer = memory.exchange_host_buffer(nullptr);
			initialize_auxiliary_pointers();
			host_buffer_exists = true;
		}
		return *this;
	}
	inline T* const exchange_host_buffer(T* const host_buffer) { // sets host_buffer to new pointer and returns old pointer
		T* const swap = this->host_buffer;
		this->host_buffer = host_buffer;
		return swap;
	}
	inline void add_host_buffer() { // makes only sense if there is no host buffer yet but an existing device buffer
		if(!host_buffer_exists&&device_buffer_exists) {
			host_buffer = new T[N*(ulong)d];
			initialize_auxiliary_pointers();
			read_from_device();
			host_buffer_exists = true;
		} else if(host_buffer_exists) {
			print_error("Host buffer already exists.");
		} else if(!device_buffer_exists) {
			print_error("There is no existing device buffer, so can't add host buffer.");
		}
	}
	inline void add_device_buffer() { // makes only sense if there is no device buffer yet but an existing host buffer
		if(!device_buffer_exists&&host_buffer_exists) {
			allocate_device_buffer(*device, true);
			write_to_device();
		} else if(device_buffer_exists) {
			print_error("Device buffer already exists.");
		} else if(!host_buffer_exists) {
			print_error("There is no existing host buffer, so can't add device buffer.");
		}
	}
	inline void delete_host_buffer() {
		if(!device_buffer_exists) {
			N = 0ull;
			d = 1u;
		}
		host_buffer_exists = false;
		delete[] host_buffer;
	}
	inline void delete_device_buffer() {
		if(!host_buffer_exists) {
			N = 0ull;
			d = 1u;
		}
		device_buffer_exists = false;
		device_buffer = nullptr;
	}
	inline void delete_buffers() {
		delete_host_buffer();
		delete_device_buffer();
	}
	inline void reset(const T value=(T)0) {
		if(host_buffer_exists) for(ulong i=0ull; i<N*(ulong)d; i++) host_buffer[i] = value;
		write_to_device();
	}
	inline const ulong length() const {
		return N;
	}
	inline const uint dimensions() const {
		return d;
	}
	inline const ulong range() const {
		return N*(ulong)d;
	}
	inline const ulong capacity() const { // returns capacity of the buffer in Byte
		return N*(ulong)d*sizeof(T);
	}
	inline T* const data() {
		return host_buffer;
	}
	inline const T* const data() const {
		return host_buffer;
	}
	inline T* const operator()() {
		return host_buffer;
	}
	inline const T* const operator()() const {
		return host_buffer;
	}
	inline T& operator[](const ulong i) {
		return host_buffer[i];
	}
	inline const T& operator[](const ulong i) const {
		return host_buffer[i];
	}
	inline const T operator()(const ulong i) const {
		return host_buffer[i];
	}
	inline const T operator()(const ulong i, const uint dimension) const {
		return host_buffer[i+(ulong)dimension*N]; // array of structures
	}
	inline void read_from_device(const bool blocking=true) {
		if(host_buffer_exists&&device_buffer_exists) cl_queue.enqueueReadBuffer(device_buffer, blocking, 0u, capacity(), (void*)host_buffer);
	}
	inline void write_to_device(const bool blocking=true) {
		if(host_buffer_exists&&device_buffer_exists) cl_queue.enqueueWriteBuffer(device_buffer, blocking, 0u, capacity(), (void*)host_buffer);
	}
	inline void read_from_device(const ulong offset, const ulong length, const bool blocking=true) {
		if(host_buffer_exists&&device_buffer_exists) cl_queue.enqueueReadBuffer(device_buffer, blocking, min(offset, range())*sizeof(T), min(length, range()-offset)*sizeof(T), (void*)(host_buffer+min(offset, range())));
	}
	inline void write_to_device(const ulong offset, const ulong length, const bool blocking=true) {
		if(host_buffer_exists&&device_buffer_exists) cl_queue.enqueueWriteBuffer(device_buffer, blocking, min(offset, range())*sizeof(T), min(length, range()-offset)*sizeof(T), (void*)(host_buffer+min(offset, range())));
	}
	inline void finish() {
		cl_queue.finish();
	}
	inline const cl::Buffer& get_cl_buffer() const {
		return device_buffer;
	}
};

class Kernel {
private:
	uint number_of_parameters = 0u;
	cl::Kernel cl_kernel;
	cl::NDRange cl_range_global, cl_range_local;
	cl::CommandQueue cl_queue;
	inline void initialize_ranges(const ulong N) {
		cl_range_local = cl::NDRange(WORKGROUP_SIZE); // warp size is 32
		cl_range_global = cl::NDRange(((N+WORKGROUP_SIZE-1)/WORKGROUP_SIZE)*WORKGROUP_SIZE); // make global range a multiple of local range
	}
public:
	template<typename... T> inline Kernel(const Device& device, const ulong N, const string& name, const Memory<T>&... parameters) {
		cl_kernel = cl::Kernel(device.get_cl_program(), name.c_str());
		(cl_kernel.setArg(number_of_parameters++, parameters.get_cl_buffer()), ...); // expand variadic template to link buffers against kernel parameters
		initialize_ranges(N);
		cl_queue = device.get_cl_queue();
	}
	template<typename... T> inline Kernel(const Device& device, const ulong N, const string& name, const Memory<T>*... parameters) {
		cl_kernel = cl::Kernel(device.get_cl_program(), name.c_str());
		(cl_kernel.setArg(number_of_parameters++, parameters->get_cl_buffer()), ...); // expand variadic template to link buffers against kernel parameters
		initialize_ranges(N);
		cl_queue = device.get_cl_queue();
	}
	inline Kernel() {} // default constructor
	template<typename... T> inline void add_parameters(const Memory<T>&... parameters) {
		(cl_kernel.setArg(number_of_parameters++, parameters.get_cl_buffer()), ...); // expand variadic template to link buffers against kernel parameters
	}
	template<typename... T> inline void add_parameters(const Memory<T>*... parameters) {
		(cl_kernel.setArg(number_of_parameters++, parameters->get_cl_buffer()), ...); // expand variadic template to link buffers against kernel parameters
	}
	template<typename... T> inline void add_constants(const T... constants) {
		(cl_kernel.setArg(number_of_parameters++, constants), ...); // expand variadic template to pass constants as kernel parameters
	}
	inline void run() const {
		cl_queue.enqueueNDRangeKernel(cl_kernel, cl::NullRange, cl_range_global, cl_range_local);
		cl_queue.finish();
	}
};
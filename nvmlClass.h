/*
 * Copyright 1993-2019 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO USER:
 *
 * This source code is subject to NVIDIA ownership rights under U.S. and
 * international Copyright laws.  Users and possessors of this source code
 * are hereby granted a nonexclusive, royalty-free license to use this code
 * in individual and commercial software.
 *
 * NVIDIA MAKES NO REPRESENTATION ABOUT THE SUITABILITY OF THIS SOURCE
 * CODE FOR ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR
 * IMPLIED WARRANTY OF ANY KIND.  NVIDIA DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOURCE CODE, INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL,
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF OR IN CONNECTION WITH THE USE
 * OR PERFORMANCE OF THIS SOURCE CODE.
 *
 * U.S. Government End Users.   This source code is a "commercial item" as
 * that term is defined at  48 C.F.R. 2.101 (OCT 1995), consisting  of
 * "commercial computer  software"  and "commercial computer software
 * documentation" as such terms are  used in 48 C.F.R. 12.212 (SEPT 1995)
 * and is provided to the U.S. Government only as a commercial end item.
 * Consistent with 48 C.F.R.12.212 and 48 C.F.R. 227.7202-1 through
 * 227.7202-4 (JUNE 1995), all U.S. Government End Users acquire the
 * source code with only those rights set forth herein.
 *
 * Any use of this source code in individual and commercial software must
 * include, in the user documentation and internal comments to the code,
 * the above Disclaimer and U.S. Government End Users Notice.
 */

/* This is a header class that utilizes NVML library.
 */

#ifndef NVMLCLASS_H_
#define NVMLCLASS_H_

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <cuda_runtime.h>

#include <nvml.h>

auto constexpr sizeOfVector = 100000;
auto constexpr nvml_device_name_buffer_size = 100;

template<typename T>
void check( T const & errCode, std::string const & file, int const & line ) {
	if ( errCode ) {
		cudaDeviceReset( );
		std::string str = nvmlErrorString( errCode );
		throw std::runtime_error( str + " in " + file + " at line " + std::to_string( line ) );
	}
}

#define checkNVMLErrors( errCode ) check( errCode, __FILE__, __LINE__)

class nvmlClass {
public:
	nvmlClass( int const & deviceID, std::string const & _filename ) :
			filename { _filename },
			loop { false } {

		char name[nvml_device_name_buffer_size];

		// Initialize NVML library
		checkNVMLErrors( nvmlInit( ) );

		// Query device handle
		checkNVMLErrors( nvmlDeviceGetHandleByIndex(deviceID, &device) );

		// Query device name
		checkNVMLErrors( nvmlDeviceGetName( device, name, nvml_device_name_buffer_size ) );

		// Reserve memory for data
		timeSteps.reserve( sizeOfVector );

		// Open file
		outfile.open( filename, std::ios::out );

		// Print header
		printHeader();
	}

	~nvmlClass( ) {

		checkNVMLErrors( nvmlShutdown( ) );

		writeData( );
	}

	void getStats( ) {

		stats deviceStats { };
		loop = true;

		while ( loop ) {
			deviceStats.timestamp = std::chrono::high_resolution_clock::now( ).time_since_epoch( ).count( );
			checkNVMLErrors( nvmlDeviceGetTemperature( device, NVML_TEMPERATURE_GPU, &deviceStats.temperature ) );
			checkNVMLErrors( nvmlDeviceGetPowerUsage( device, &deviceStats.powerUsage ) );
			checkNVMLErrors( nvmlDeviceGetEnforcedPowerLimit( device, &deviceStats.powerLimit ) );
			checkNVMLErrors( nvmlDeviceGetUtilizationRates( device, &deviceStats.utilization ) );
			checkNVMLErrors( nvmlDeviceGetMemoryInfo( device, &deviceStats.memory ) );
			checkNVMLErrors( nvmlDeviceGetCurrentClocksThrottleReasons( device, &deviceStats.throttleReasons ) );
			checkNVMLErrors( nvmlDeviceGetClock( device, NVML_CLOCK_SM, NVML_CLOCK_ID_CURRENT, &deviceStats.clockSM ) );
			checkNVMLErrors( nvmlDeviceGetClock( device, NVML_CLOCK_GRAPHICS, NVML_CLOCK_ID_APP_CLOCK_TARGET, &deviceStats.clockGraphics ) );
			checkNVMLErrors( nvmlDeviceGetClock( device, NVML_CLOCK_MEM, NVML_CLOCK_ID_CURRENT, &deviceStats.clockMemory ) );
			checkNVMLErrors( nvmlDeviceGetClock( device, NVML_CLOCK_MEM, NVML_CLOCK_ID_APP_CLOCK_TARGET, &deviceStats.clockMemoryMax ) );
			checkNVMLErrors( nvmlDeviceGetPerformanceState( device, &deviceStats.performanceState ) );

			timeSteps.push_back( deviceStats );

			std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
		}
	}

	void killThread( ) {

		// Retrieve a few empty samples
		std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

		// Set loop to false to exit while loop
		loop = false;
	}

private:
	typedef struct _stats {
		std::time_t timestamp;
		uint temperature;
		uint powerUsage;
		uint powerLimit;
		nvmlUtilization_t utilization;
		nvmlMemory_t memory;
		unsigned long long throttleReasons;
		uint clockSM;
		uint clockGraphics;
		uint clockMemory;
		uint clockMemoryMax;
		nvmlPstates_t performanceState;
	} stats;

	std::vector<std::string> names = {
			"timestamp",
			"temperature_gpu",
			"power_draw_w",
			"power_limit_w",
			"utilization_gpu",
			"utilization_memory",
			"memory_used_mib",
			"memory_free_mib",
			"clocks_throttle_reasons_active",
			"clocks_current_sm_mhz",
			"clocks_applications_graphics_mhz",
			"clocks_current_memory_mhz",
			"clocks_max_memory_mhz",
			"pstate" };

	std::vector<stats> timeSteps { };
	std::string filename { };
	std::ofstream outfile { };
	nvmlDevice_t device { };
	bool loop { };

	void printHeader( ) {

		// Print header
		for ( int i = 0; i < ( static_cast<int>( names.size( ) ) - 1 ); i++ )
			outfile << names[i] << ", ";
		outfile << names[static_cast<int>( names.size( ) ) - 1]; // Leave off the last comma
		outfile << "\n";
	}

	void writeData( ) {

		printf("Writing NVIDIA-SMI data -> %s\n\n", filename.c_str());

		// Print data
		for ( int i = 0; i < static_cast<int>( timeSteps.size( ) ); i++ ) {
			outfile << timeSteps[i].timestamp << ", "
					<< timeSteps[i].temperature << ", "
					<< timeSteps[i].powerUsage/1000 << ", "		// Convert mW to W
					<< timeSteps[i].powerLimit/1000 << ", "		// Convert mW to W
					<< timeSteps[i].utilization.gpu << ", "
					<< timeSteps[i].utilization.memory << ", "
					<< timeSteps[i].memory.used/1000000 << ", "	// Convert B to MB
					<< timeSteps[i].memory.free/1000000 << ", "	// Convert B to MB
					<< timeSteps[i].throttleReasons << ", "
					<< timeSteps[i].clockSM << ", "
					<< timeSteps[i].clockGraphics << ", "
					<< timeSteps[i].clockMemory << ", "
					<< timeSteps[i].clockMemoryMax << ", "
					<< timeSteps[i].performanceState << "\n";
		}
		outfile.close();
	}
};

#endif /* NVMLCLASS_H_ */

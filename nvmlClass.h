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

int constexpr size_of_vector { 100000 };
int constexpr nvml_device_name_buffer_size { 100 };

// *************** FOR ERROR CHECKING *******************
#ifndef NVML_RT_CALL
#define NVML_RT_CALL( call )                                                                                           \
    {                                                                                                                  \
        auto status = static_cast<nvmlReturn_t>( call );                                                               \
        if ( status != NVML_SUCCESS )                                                                                  \
            fprintf( stderr,                                                                                           \
                     "ERROR: CUDA NVML call \"%s\" in line %d of file %s failed "                                      \
                     "with "                                                                                           \
                     "%s (%d).\n",                                                                                     \
                     #call,                                                                                            \
                     __LINE__,                                                                                         \
                     __FILE__,                                                                                         \
                     nvmlErrorString( status ),                                                                        \
                     status );                                                                                         \
    }
#endif  // NVML_RT_CALL
// *************** FOR ERROR CHECKING *******************

class nvmlClass {
  public:
    nvmlClass( int const &deviceID, std::string const &filename ) :
        time_steps_ {}, filename_ { filename }, outfile_ {}, device_ {}, loop_ { false } {

        char name[nvml_device_name_buffer_size];

        // Initialize NVML library
        NVML_RT_CALL( nvmlInit( ) );

        // Query device handle
        NVML_RT_CALL( nvmlDeviceGetHandleByIndex( deviceID, &device_ ) );

        // Query device name
        NVML_RT_CALL( nvmlDeviceGetName( device_, name, nvml_device_name_buffer_size ) );

        // Reserve memory for data
        time_steps_.reserve( size_of_vector );

        // Open file
        outfile_.open( filename_, std::ios::out );

        // Print header
        printHeader( );
    }

    ~nvmlClass( ) {

        NVML_RT_CALL( nvmlShutdown( ) );

        writeData( );
    }

    void getStats( ) {

        stats device_stats {};
        loop_ = true;

        while ( loop_ ) {
            device_stats.timestamp = std::chrono::high_resolution_clock::now( ).time_since_epoch( ).count( );
            NVML_RT_CALL( nvmlDeviceGetTemperature( device_, NVML_TEMPERATURE_GPU, &device_stats.temperature ) );
            NVML_RT_CALL( nvmlDeviceGetPowerUsage( device_, &device_stats.powerUsage ) );
            NVML_RT_CALL( nvmlDeviceGetEnforcedPowerLimit( device_, &device_stats.powerLimit ) );
            NVML_RT_CALL( nvmlDeviceGetUtilizationRates( device_, &device_stats.utilization ) );
            NVML_RT_CALL( nvmlDeviceGetMemoryInfo( device_, &device_stats.memory ) );
            NVML_RT_CALL( nvmlDeviceGetCurrentClocksThrottleReasons( device_, &device_stats.throttleReasons ) );
            NVML_RT_CALL( nvmlDeviceGetClock( device_, NVML_CLOCK_SM, NVML_CLOCK_ID_CURRENT, &device_stats.clockSM ) );
            NVML_RT_CALL( nvmlDeviceGetClock(
                device_, NVML_CLOCK_GRAPHICS, NVML_CLOCK_ID_APP_CLOCK_TARGET, &device_stats.clockGraphics ) );
            NVML_RT_CALL(
                nvmlDeviceGetClock( device_, NVML_CLOCK_MEM, NVML_CLOCK_ID_CURRENT, &device_stats.clockMemory ) );
            NVML_RT_CALL( nvmlDeviceGetClock(
                device_, NVML_CLOCK_MEM, NVML_CLOCK_ID_APP_CLOCK_TARGET, &device_stats.clockMemoryMax ) );
            NVML_RT_CALL( nvmlDeviceGetPerformanceState( device_, &device_stats.performanceState ) );

            time_steps_.push_back( device_stats );

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

    void killThread( ) {

        // Retrieve a few empty samples
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

        // Set loop to false to exit while loop
        loop_ = false;
    }

  private:
    typedef struct _stats {
        std::time_t        timestamp;
        uint               temperature;
        uint               powerUsage;
        uint               powerLimit;
        nvmlUtilization_t  utilization;
        nvmlMemory_t       memory;
        unsigned long long throttleReasons;
        uint               clockSM;
        uint               clockGraphics;
        uint               clockMemory;
        uint               clockMemoryMax;
        nvmlPstates_t      performanceState;
    } stats;

    std::vector<std::string> names_ = { "timestamp",
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

    std::vector<stats> time_steps_;
    std::string        filename_;
    std::ofstream      outfile_;
    nvmlDevice_t       device_;
    bool               loop_;

    void printHeader( ) {

        // Print header
        for ( int i = 0; i < ( static_cast<int>( names_.size( ) ) - 1 ); i++ )
            outfile_ << names_[i] << ", ";
        // Leave off the last comma
        outfile_ << names_[static_cast<int>( names_.size( ) ) - 1];
        outfile_ << "\n";
    }

    void writeData( ) {

        printf( "Writing NVIDIA-SMI data -> %s\n\n", filename_.c_str( ) );

        // Print data
        for ( int i = 0; i < static_cast<int>( time_steps_.size( ) ); i++ ) {
            outfile_ << time_steps_[i].timestamp << ", " << time_steps_[i].temperature << ", "
                     << time_steps_[i].powerUsage / 1000 << ", "  // mW to W
                     << time_steps_[i].powerLimit / 1000 << ", "  // mW to W
                     << time_steps_[i].utilization.gpu << ", " << time_steps_[i].utilization.memory << ", "
                     << time_steps_[i].memory.used / 1000000 << ", "  // B to MB
                     << time_steps_[i].memory.free / 1000000 << ", "  // B to MB
                     << time_steps_[i].throttleReasons << ", " << time_steps_[i].clockSM << ", "
                     << time_steps_[i].clockGraphics << ", " << time_steps_[i].clockMemory << ", "
                     << time_steps_[i].clockMemoryMax << ", " << time_steps_[i].performanceState << "\n";
        }
        outfile_.close( );
    }
};

#endif /* NVMLCLASS_H_ */

# nvml_example
Example showing how to utilize NVML library for GPU monitoring with high sampling rate.

This example utilizing the NVML Library and C++11 mutlithreading to provide GPU monitoring with a high sampling rate. Normally, one would pipe nvidia-smi to a file, but this can cause excessive I/O usage.

Also provided is a Matlab script used to plot the data.

## Getting started
To run, all one must do is execute *make* in the Release directory. You might need to add the compute capability of the device you are using to the make process. Everything is included to import files into Nsight as well.

### Prerequisites
This **makefile** expects a symbolic link to the CUDA directory
```/usr/local/cuda```. This can be changed to the desired CUDA version.

### Built With
This example utilizes the following toolsets:
* cuBLAS
* Thrust
* C++11 multithreading

### Deployment
Include header class

```#include nvmlClass.h```

Create object of nvmlClass

```nvmlClass nvml( dev, filename );```

Create thread to run in parallel, gathering data

```std::thread threadStart( &nvmlClass::getStats, &nvml );```

Create thread to kill getStats function

```std::thread threadKill( &nvmlClass::killThread, &nvml );```

Join all threads

```threadStart.join( );```

```threadKill.join( );```

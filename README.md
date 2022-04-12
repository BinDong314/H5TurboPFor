# H5TurboPFor

This software is part of the research paper:

* **Real-time and post-hoc compression for data from distributed acoustic sensing**, Bin Dong, Alex Popescu, Ver ́onica Rodr ́ıguez Tribaldos, Suren Byna, Jonathan Ajo-Franklin, Kesheng Wu, and the Imperial Valley Dark Fiber Team. Submitted on Sept 2021.*


Please see the Copyright and the License at the end of this doc

# Installation Guide

### Dependents: HDF5 and TurboPFor.

- Install HDF5 (skip if you already have it)
  
  Please use 1.10.X (e.g. hdf5-1.10.7  https://www.hdfgroup.org/packages/hdf5-1107-source/). The HDf5 1.12 has issues with plug-in support. Below are some steps to install it
  ```console
  > tar zxvf hdf5-1.10.7.tar.gz
  > ./autogen.sh
  > ./configure --prefix=$PWD/build 
    # You may need "--enable-parallel CC=mpicc" to enable parallel version
  > make 
  > make install
  > export HDF5_HOME=$PWD/build

  ```
  If on NERSC machine or machine with HDF5 as module. 
  Just use the pre-compiled HDF5 
  ```console
  > module load cray-hdf5-parallel/1.10.5.2
  ```
- Install TurboPFor
   
  ```console
  > git clone https://github.com/powturbo/TurboPFor-Integer-Compression.git
  > cd TurboPFor-Integer-Compression
  > make
  > export TurboPFor_HOME=$PWD
  ```
   
### Install H5TurboPFor
  
  ```console
  > git clone https://github.com/dbinlbl/H5TurboPFor.git
  > cd H5TurboPFor
  > cmake .
  > make
  > make install
  > source setup.sh   ##setup the path to load the H5TurboPFor

   Note:
   (1) You may want to edit the CMakeLists.txt files for proper installtaion of TurboPFor
   
     set(turbopfor_ROOT_DIR $ENV{TurboPFor_HOME})
  
   (2)the default installation directory is set as $PWD/build.
    You can adjust it if you want

    set(PLUGIN_INSTALL_PATH "./build" CACHE PATH "Where to install the dynamic HDF5-plugin")
  ```
  
# Usage in Python, Jupytor Notebook, and C/C++

Note: please make sure you have ran "source setup.sh" and then start python/jupyter-notebook with the same terminal to avoid issues like ***"ValueError: Unknown compression filter number: 62016"***


### (1) Python 
 
```python
> python3 py-example.py
> h5dump -pH das_example_compressed.h5

HDF5 "das_example_compressed.h5" {
GROUP "/" {
   DATASET "Acoustic" {
      DATATYPE  H5T_STD_I16LE
      DATASPACE  SIMPLE { ( 30000, 21 ) / ( 30000, 21 ) }
      STORAGE_LAYOUT {
         CHUNKED ( 30000, 21 )
         SIZE 668260 (1.885:1 COMPRESSION)
      }
     ... ...
}}}
```
### (2) Jupytor Notebook

Please see the [H5TurboPFor-Example-Jupyter.ipynb](H5TurboPFor-Example-Jupyter.ipynb)  for the example

### (3) h5repack

Error is h5repack: "UD=62016,0,4,0,1,30000,21" v.s. "UD=62016,4,0,1,30000,21"
Based on the h5repack doc [h5repack](https://support.hdfgroup.org/HDF5/doc1.8/RM/Tools.html#Tools-Repack). Don't know why there is extra "0" after "62016" to make it work .

```console
> h5repack -f UD=62016,0,4,0,1,30000,21  das_example.h5  das_example_rpk.h5
> h5dump -pH das_example_rpk.h5
HDF5 "das_example_rpk.h5" {
GROUP "/" {
   DATASET "Acoustic" {
      DATATYPE  H5T_STD_I16LE
      DATASPACE  SIMPLE { ( 30000, 21 ) / ( 30000, 21 ) }
      STORAGE_LAYOUT {
         CHUNKED ( 30000, 21 )
         SIZE 668260 (1.885:1 COMPRESSION)
      }
      FILTERS {
         USER_DEFINED_FILTER {
            FILTER_ID 62016
            COMMENT TurboPFor-Integer-Compression: https://github.com/dbinlbl/H5TurboPFor
            PARAMS { 0 1 30000 21 }
         }
      }
      FILLVALUE {
         FILL_TIME H5D_FILL_TIME_IFSET
         VALUE  H5D_FILL_VALUE_DEFAULT
      }
      ALLOCATION_TIME {
         H5D_ALLOC_TIME_INCR
}}}}
```
### (4) Embed in your C/C++ code
 
Based on the H5TurboPFor_HOME and HDF5_HOME set above

```console
> export HDF5_PLUGIN_PATH=$HDF5_PLUGIN_PATH:$H5TurboPFor_HOME/lib
> export LD_LIBRARY_PATH=$HDF5_PLUGIN_PATH:$HDF5_HOME/lib
> export DYLD_LIBRARY_PATH=$LD_LIBRARY_PATH

The DYLD_LIBRARY_PATH may be needed only on MacOS
```

The blow is the minimum code to use the H5TurboPFor

```C
   unsigned int filter_flags = H5Z_FLAG_MANDATORY;
   H5Z_filter_t filter_id = 62016;
   hid_t create_dcpl_id = H5Pcreate(H5P_DATASET_CREATE);

   * @param cd_values: the pointer of the parameter 
   * 			cd_values[0]: type of data:  short (0),  int (1)
   *          cd_values[1]: 0/1 pre-processing method: zipzag  
   *          cd_values[2, -]: size of each dimension of a chunk 
   filter_cd_nelmts = 4
   filter_cd_values[0] = 0;
   filter_cd_values[1] = 1;
   filter_cd_values[2] = 100;
   filter_cd_values[3] = 100;

   H5Pset_filter(create_dcpl_id, filter_id, filter_flags, filter_cd_nelmts, filter_cd_values);
   endpoint_ranks = 2;
   filter_chunk_size[0] = 100;
   filter_chunk_size[1] = 100;
   H5Pset_chunk(create_dcpl_id, endpoint_ranks, filter_chunk_size);
   
   did = H5Dcreate(fid, "FNAME", "FILE DISK TYPE", "DATA SPACE", H5P_DEFAULT, create_dcpl_id, H5P_DEFAULT);
   
 ```
 

 ### (4) A parallel implementation specifically for DAS data is available in DASSA
 
 ```console
 https://bitbucket.org/dbin_sdm/dassa/src/master/
 ```
 
 
****************************

H5TurboPFor Copyright (c) 2021, The Regents of the University of
California, through Lawrence Berkeley National Laboratory (subject
to receipt of any required approvals from the U.S. Dept. of Energy). 
All rights reserved.

If you have questions about your rights to use or distribute this software,
please contact Berkeley Lab's Intellectual Property Office at
IPO@lbl.gov.

NOTICE.  This Software was developed under funding from the U.S. Department
of Energy and the U.S. Government consequently retains certain rights.  As
such, the U.S. Government has been granted for itself and others acting on
its behalf a paid-up, nonexclusive, irrevocable, worldwide license in the
Software to reproduce, distribute copies to the public, prepare derivative 
works, and perform publicly and display publicly, and to permit others to do so.


****************************

*** License Agreement ***

H5TurboPFor Copyright (c) 2021, The Regents of the University of
California, through Lawrence Berkeley National Laboratory (subject
to receipt of any required approvals from the U.S. Dept. of Energy). 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

(1) Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

(2) Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

(3) Neither the name of the University of California, Lawrence Berkeley
National Laboratory, U.S. Dept. of Energy nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

You are under no obligation whatsoever to provide any bug fixes, patches,
or upgrades to the features, functionality or performance of the source
code ("Enhancements") to anyone; however, if you choose to make your
Enhancements available either publicly, or directly to Lawrence Berkeley
National Laboratory, without imposing a separate written license agreement
for such Enhancements, then you hereby grant the following license: a
non-exclusive, royalty-free perpetual license to install, use, modify,
prepare derivative works, incorporate into other computer software,
distribute, and sublicense such enhancements or derivative works thereof,
in binary and source code form.
import os
import h5py

orignial_das_file="./das_example.h5"
compressed_das_file="./das_example_compressed.h5"

# open orginal file and dataset
das_f = h5py.File(orignial_das_file, 'r')
das_dset_orig = das_f['/Acoustic']


# 62016 is the ID of H5TurboPFor
compression_id=62016
# arguments for H5TurboPFor
## - compression_args[0]: data element type 0 - short type ; 
## - compression_args[1]: pre-processing method zipzag (1), abs(2), plusabsmin (3) 
## - compression_args[2-3]: chunk size
compression_args=(0, 1, das_dset_orig.shape[0], das_dset_orig.shape[1])

#create compresed file and write data, the compression happens within the write files
das_f_compressed = h5py.File(compressed_das_file, "w")
das_dset_compressed = das_f_compressed.create_dataset("/Acoustic", chunks=das_dset_orig.shape, data=das_dset_orig, compression=compression_id, compression_opts=compression_args)

#close files
das_f_compressed.close()
das_f.close()

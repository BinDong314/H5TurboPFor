import tables
import os
import h5py

orignial_das_file="./das_example_w_earthquake.h5"
compressed_das_file="./das_example_w_earthquake_compressed.h5"
das_f = h5py.File(orignial_das_file, 'r')
das_dset = das_f['Acoustic']

print(das_dset.shape)
print(das_dset.dtype)

das_f_compressed = h5py.File(compressed_das_file, "w")
das_dset_compressed = das_f_compressed.create_dataset("Acoustic", chunks=das_dset.shape, data=das_dset, compression=62016, compression_opts=(0,1,60000,6912))
das_dset_compressed=das_dset
das_f_compressed.close()
das_f.close()
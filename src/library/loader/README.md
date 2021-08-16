EAR loader
----------

The EAR loader library is dynamically loaded with applications and automatically selects the libear version to be loaded depending on application type. Versions are differentiated by the extension (part of the library name)

Versions detected
-----------------
- Intel MPI
- OpenMPI
- openMP and mkl programs
- CUDA

Extensionsa are automatically defined as a function of application type and can be seen in src/common/config/config_def.h

The EAR loader uses the EAR_INSTALL_PATH to search for libraries, so it must be defined and libraries must be availables in all the compute nodes.

if($ENV{HOSTNAME} MATCHES "^lxplus[0-9]+.cern.ch")
  set(BASE_DIR "/cvmfs/sft.cern.ch/lcg/external")
  set(GSL_DIR "${BASE_DIR}/GSL/1.14/x86_64-slc5-gcc44-opt")
  set(HEPMC_DIR "${BASE_DIR}/HepMC/2.06.08/x86_64-slc6-gcc48-opt")
  #set(LHAPDF_DIR "${BASE_DIR}/MCGenerators/lhapdf/5.8.9/x86_64-slc6-gcc46-opt")
  set(LHAPDF_DIR "${BASE_DIR}/MCGenerators_lcgcmt67c/lhapdf/6.1.5/x86_64-slc6-gcc48-opt")
  set(PYTHIA_DIR "${BASE_DIR}/MCGenerators_lcgcmt67c/pythia8/201/x86_64-slc6-gcc48-opt")
  set(ROOTSYS "/cvmfs/sft.cern.ch/lcg/app/releases/ROOT/6.02.00/x86_64-slc6-gcc48-opt/root")

  message(STATUS "Compiling on LXPLUS. Do not forget to source the environment variables!")
  message(STATUS "e.g. source /cvmfs/sft.cern.ch/lcg/external/gcc/6.2.0/x86_64-slc6-gcc62-opt/setup.sh")
  #--- searching for GSL
  find_library(GSL_LIB gsl HINTS "${GSL_DIR}/lib")
  find_library(GSL_CBLAS_LIB gslcblas HINTS "${GSL_DIR}/lib")
  #--- searching for Pythia 8
  find_library(PYTHIA8 pythia8 HINTS "${PYTHIA_DIR}/include")
  find_path(PYTHIA8_INCLUDE Pythia8 HINTS "${PYTHIA_DIR}/include")
  #--- searching for LHAPDF
  find_library(LHAPDF LHAPDF HINTS "${LHAPDF_DIR}/lib")
  find_path(LHAPDF_INCLUDE LHAPDF HINTS "${LHAPDF_DIR}/include")
  #--- searching for HepMC
  find_library(HEPMC_LIB HepMC HINTS "${HEPMC_DIR}/lib")
  find_library(HEPMC_FIO_LIB HepMCfio HINTS "${HEPMC_DIR}/lib")
  find_path(HEPMC_INCLUDE HepMC HINTS "${HEPMC_DIR}/include")
else()
  find_library(GSL_LIB gsl)
  find_library(GSL_CBLAS_LIB gslcblas)
  find_library(PYTHIA8 pythia8)
  find_path(PYTHIA8_INCLUDE Pythia8)
  find_library(LHAPDF LHAPDF)
  find_path(LHAPDF_INCLUDE LHAPDF)
  find_library(HEPMC_LIB HepMC)
  find_library(HEPMC_FIO_LIB HepMCfio)
  find_path(HEPMC_INCLUDE HepMC)
endif()
find_library(LIBCONFIG config++)
find_library(MUPARSER muparser)

if(EXISTS ${GSL_LIB})
  message(STATUS "GSL found in ${GSL_LIB}")
  list(APPEND CEPGEN_EXTERNAL_REQS ${GSL_LIB} ${GSL_CBLAS_LIB})
else()
  message(FATAL_ERROR "GSL not found on your system!")
endif()
if(EXISTS ${LHAPDF})
  message(STATUS "LHAPDF found in ${LHAPDF}")
  list(APPEND CEPGEN_EXTERNAL_REQS ${LHAPDF})
  add_definitions(-DLIBLHAPDF)
  include_directories(${LHAPDF_INCLUDE})
endif()
if(EXISTS ${HEPMV_LIB})
  message(STATUS "HepMC found in ${HEPMC_INCLUDE}")
  list(APPEND CEPGEN_EXTERNAL_REQS ${HEPMC_LIB})
  #list(APPEND CEPGEN_EXTERNAL_REQS ${HEPMC_FIO_LIB})
  add_definitions(-DLIBHEPMC)
  include_directories(${HEPMC_INCLUDE})
endif()
if(EXISTS ${LIBCONFIG})
  message(STATUS "libconfig++ found in ${LIBCONFIG}")
  list(APPEND CEPGEN_EXTERNAL_REQS ${LIBCONFIG})
  add_definitions(-DLIBCONFIG)
endif()
if(EXISTS ${MUPARSER})
  message(STATUS "muParser found in ${MUPARSER}")
  list(APPEND CEPGEN_EXTERNAL_REQS ${MUPARSER})
  add_definitions(-DMUPARSER)
endif()
if(EXISTS ${PYTHIA8})
  message(STATUS "Pythia8 found in ${PYTHIA8}")
  list(APPEND CEPGEN_EXTERNAL_REQS ${PYTHIA8})
  add_definitions(-DPYTHIA8)
  include_directories(${PYTHIA8_INCLUDE})
endif()


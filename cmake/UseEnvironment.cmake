if(CMAKE_VERSION VERSION_GREATER 3.1)
  set(CMAKE_CXX_STANDARD 14)
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()
set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Wall -cpp")
#--- check if we are at CERN
if($ENV{HOSTNAME} MATCHES "^lxplus[0-9]+.cern.ch")
  set(IS_LXPLUS "yes")
endif()
set(LXPLUS_SRC_ENV "source ${CMAKE_SOURCE_DIR}/source-lxplus.sh")
#--- ensure a proper version of the compiler is found
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.1)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-long-long -pedantic-errors -g")
else()
  message(STATUS "clang or gcc above 6.1 is required")
  if(IS_LXPLUS)
    message(STATUS "Compiling on LXPLUS. Did you properly source the environment variables? E.g.\n\n\t${LXPLUS_SRC_ENV}\n")
  endif()
  message(FATAL_ERROR "Please clean up this build environment, i.e.\n\trm -rf CMake*\nand try again...")
endif()
#--- set the default paths for external dependencies
if(IS_LXPLUS)
  set(BASE_DIR "/cvmfs/sft.cern.ch/lcg")
  list(APPEND CMAKE_PREFIX_PATH "${BASE_DIR}/external/CMake/2.8.9/Linux-i386/share/cmake-2.8/Modules")
  set(GSL_DIR "${BASE_DIR}/releases/GSL/2.5-32fc5/x86_64-centos7-gcc62-opt")
  set(HEPMC_DIR "${BASE_DIR}/releases/HepMC/2.06.09-0a23a/x86_64-centos7-gcc62-opt")
  set(LHAPDF_DIR "${BASE_DIR}/releases/MCGenerators/lhapdf/6.2.2-8a3e6/x86_64-centos7-gcc62-opt")
  set(PYTHIA6_DIR "${BASE_DIR}/releases/MCGenerators/pythia6/429.2-c4089/x86_64-centos7-gcc62-opt")
  set(TBB_DIR "${BASE_DIR}/releases/tbb/2019_U1-5939b/x86_64-centos7-gcc62-opt")
  set(DAVIX_DIR "${BASE_DIR}/releases/Davix/0.7.1-f7fe6/x86_64-centos7-gcc62-opt")
  set(VDT_DIR "${BASE_DIR}/releases/vdt/0.4.2-84b8c/x86_64-centos7-gcc62-opt")
  set(PYTHON_DIR "${BASE_DIR}/releases/Python/2.7.15-075d4/x86_64-centos7-gcc62-opt")
  set(PYTHON_LIBRARY "${PYTHON_DIR}/lib/libpython2.7.so")
  set(PYTHON_EXECUTABLE "${PYTHON_DIR}/bin/python")
  set(PYTHON_INCLUDE_DIR "${PYTHON_DIR}/include/python2.7")

  message(STATUS "Compiling on LXPLUS. Do not forget to source the environment variables!")
  message(STATUS "e.g. `${LXPLUS_SRC_ENV}`")
endif()
#--- searching for GSL
find_library(GSL_LIB gsl HINTS ${GSL_DIR} PATH_SUFFIXES lib REQUIRED)
find_library(GSL_CBLAS_LIB gslcblas HINTS ${GSL_DIR} PATH_SUFFIXES lib)
find_path(GSL_INCLUDE gsl HINTS ${GSL_DIR} PATH_SUFFIXES include)
include_directories(${GSL_INCLUDE})
#--- searching for LHAPDF
find_library(LHAPDF LHAPDF HINTS ${LHAPDF_DIR} PATH_SUFFIXES lib)
find_path(LHAPDF_INCLUDE LHAPDF HINTS ${LHAPDF_DIR} PATH_SUFFIXES include)
#--- searching for HepMC
find_library(HEPMC_LIB NAMES HepMC3 HepMC HINTS $ENV{HEPMC_DIR} ${HEPMC_DIR} PATH_SUFFIXES lib64 lib)
find_library(HEPMC_ROOT_LIB NAMES HepMC3rootIO HepMCrootIO PATH_SUFFIXES root)
find_path(HEPMC_INCLUDE NAMES HepMC3 HepMC HINTS $ENV{HEPMC_DIR} ${HEPMC_DIR} PATH_SUFFIXES include)
#--- searching for ProMC
set(PROMC_DIRS $ENV{PROMC_DIR} $ENV{PROMC} ${PROMC_DIR})
find_library(PROMC_LIB NAMES promc HINTS ${PROMC_DIRS} PATH_SUFFIXES lib)
find_path(PROMC_INCLUDE NAMES ProMCBook.h HINTS ${PROMC_DIRS} PATH_SUFFIXES src)
find_path(PROMC_EXT_INCLUDE NAMES CBook HINTS ${PROMC_DIRS} PATH_SUFFIXES include)
#--- searching for Pythia 6
set(PYTHIA6_DIRS $ENV{PYTHIA6_DIR} ${PYTHIA6_DIR} /usr /usr/local /opt/pythia6)
find_library(PYTHIA6 pythia6 HINTS ${PYTHIA6_DIRS} PATH_SUFFIXES lib)
find_library(PYTHIA6DUMMY pythia6_dummy HINTS ${PYTHIA6_DIRS} PATH_SUFFIXES lib)
#--- searching for Pythia 8
set(PYTHIA8_DIRS $ENV{PYTHIA8_DIR} ${PYTHIA8_DIR} /usr /usr/local /opt/pythia8)
find_library(PYTHIA8 pythia8 HINTS ${PYTHIA8_DIRS} PATH_SUFFIXES lib)
find_path(PYTHIA8_INCLUDE Pythia8 HINTS ${PYTHIA8_DIRS} PATH_SUFFIXES include include/Pythia8 include/pythia8)
#--- searching for ROOT
find_package(ROOT QUIET)
if(ROOT_FOUND)
  if(IS_LXPLUS)
    #--- LXPLUS/CVMFS tweak for missing dependencies
    find_library(TBB tbb HINTS ${TBB_DIR} PATH_SUFFIXES lib QUIET)
    if(TBB)
      list(APPEND ROOT_LIBRARIES ${TBB})
    endif()
    find_library(DAVIX davix HINTS ${DAVIX_DIR} PATH_SUFFIXES lib64)
    if(DAVIX)
      list(APPEND ROOT_LIBRARIES ${DAVIX})
    endif()
    find_library(VDT vdt HINTS ${VDT_DIR} PATH_SUFFIXES lib)
    if(VDT)
      list(APPEND ROOT_LIBRARIES ${VDT})
    endif()
  endif()
  #--- searching for Delphes
  find_library(DELPHES Delphes HINTS $ENV{DELPHES_DIR} ${DELPHES_DIR} PATH_SUFFIXES lib)
  find_path(DELPHES_INCLUDE NAMES modules classes HINTS $ENV{DELPHES_DIR} ${DELPHES_DIR} PATH_SUFFIXES include)
  find_path(DELPHES_EXT_INCLUDE NAMES ExRootAnalysis HINTS $ENV{DELPHES_DIR} ${DELPHES_DIR} PATH_SUFFIXES external include)
endif()

find_package(PythonLibs 2.7)
find_library(MUPARSER muparser)
find_path(EXPRTK exprtk.hpp PATH_SUFFIXES include)
if(MUPARSER)
  add_definitions(-DFUNC_MUPARSER)
elseif(EXPRTK)
  add_definitions(-DFUNC_EXPRTK)
  include_directories(${EXPRTK}) # header-only
elseif(ROOT_FOUND)
  add_definitions(-DFUNC_ROOT)
  include_directories(${ROOT_INCLUDE_DIRS})
endif()
#--- semi-external dependencies
file(GLOB ALPHAS_SRC ${PROJECT_SOURCE_DIR}/External/alphaS.f)
file(GLOB GRV_SRC ${PROJECT_SOURCE_DIR}/External/grv_*.f)


#
# Title:	CMakeLists.txt
# Author:	David Moloney
# Contact:	david.moloney@ubotica.com
# Date:		31 Aug 2020
#
project(pb11_startracker)

cmake_minimum_required(VERSION 3.5)

# helpful for debugging
#
#set(CMAKE_VERBOSE_MAKEFILE ON) # https://bytefreaks.net/programming-2/make-building-with-cmake-verbose

# star-tracker code has multiple dependencies on shared libraries which must be found by cmake (see links below)
#   libraries are : python, boost, gsl, X11 and JPEG
#
find_package(PythonLibs)
find_package(Boost COMPONENTS python numpy REQUIRED)
find_package(GSL REQUIRED) # https://stackoverflow.com/questions/44821615/linking-gsl-in-cmakelists-txt-in-clion
find_package(X11 REQUIRED) # https://stackoverflow.com/questions/34014970/what-difference-lx11-option-vs-target-link-libraries-in-cmake
include_directories(${X11_INCLUDE_DIR})
link_directories(${X11_LIBRARIES})

find_package(JPEG REQUIRED)
include_directories(${JPEG_INCLUDE_DIR})
link_directories(${JPEG_LIBRARIES})

set(CMAKE_SHARED_MODULE_PREFIX "") # Without this, any build libraries automatically have names "lib{x}.so"
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Werror=return-type -lX11 -lpthread -lCCfits -lgsl -lgslcblas -std=c++0x -ljpeg -L /usr/local/lib -L /usr/local/src/CCfits/ -L /usr/local/src/CCfits/.libs/1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -static-libstdc++ -lm") # std::sqrt() support
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ")
set(SOURCE_FILES run_startracker.cpp star_tracker.cpp)
link_directories("/usr/local/lib" "/usr/local/src/CCfits"  "/lib/x86_64-linux-gnu" )

# https://stackoverflow.com/questions/30380257/how-can-ld-library-path-be-changed-within-cmake
set(CMAKE_INSTALL_RPATH "/usr/local/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lX11 -lpthread -lCCfits -lgsl -lgslcblas -std=c++0x -ljpeg -I/usr/include/python3.8 -I/usr/include/python3.8 -L /usr/local/lib -L /usr/local/src/CCfits/ -L /usr/local/src/CCfits/.libs/")

# # https://github.com/KratosMultiphysics/Kratos/wiki/Porting-to-PyBind11---common-steps
# #
# find_package(pybind11)
# #
# add_library(pb11_wrapper MODULE pb11_startracker.cpp star_tracker.cpp)
# #
# # Set up the libraries and header search paths for the python extension
# #
# target_link_libraries(pb11_wrapper ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
# target_link_libraries(pb11_wrapper GSL::gsl GSL::gslcblas)
# target_link_libraries(pb11_wrapper ${X11_LIBRARIES})
# target_link_libraries(pb11_wrapper ${JPEG_LIBRARIES})
# target_link_libraries(pb11_wrapper cfitsio CCfits pthread)

###################################################################################################
#
# standalone star-tracker executable
#
add_executable(run_startracker ${SOURCE_FILES})
target_link_libraries(run_startracker ${Boost_LIBRARIES} ${PYTHON_LIBRARIES})
target_link_libraries(run_startracker GSL::gsl GSL::gslcblas)
target_link_libraries(run_startracker ${X11_LIBRARIES})
target_link_libraries(run_startracker ${JPEG_LIBRARIES})
target_link_libraries(run_startracker cfitsio CCfits pthread)
#message("\n\nstandalone star-tracker usage: ./run_standalone test.fits")

###################################################################################################
#
# Add Makefile clean-all target to clean CMake files
#
add_custom_target(clean-all
  rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile *.so
  rm -rf CMakeFiles dist startracker_python_wrapper startracker_python_wrapper.egg-info setup.* CMakeCache.txt cmake_install.cmake *.jpeg *.jpg linkdef.h MANIFEST.in py_startracker.* setup.* run_startracker Makefile
  rm -rf pylib_auto.cpp* exposed_decl.pypp.txt named_tuple.py
)


add_custom_target(pb11
	g++ -O3 -Wall -shared -std=c++11 -fPIC `python3 -m pybind11 --includes` pb11_startracker.cpp star_tracker.cpp -o pb11_startracker`python3-config --extension-suffix` -lX11 -lpthread -lCCfits -lgsl -lgslcblas -std=c++0x -ljpeg -L /usr/local/lib -L /usr/local/src/CCfits/ -L /usr/local/src/CCfits/.libs/
)


###################################################################################################
#
# Add Makefile clean-all target to clean CMake files
#
add_custom_target(run_standalone # standalone star-tracker application (no python)
  g++ run_startracker.cpp star_tracker.cpp -o run_standalone -lX11 -lpthread -lCCfits -lgsl -lgslcblas -std=c++0x -ljpeg -L /usr/local/lib -L /usr/local/src/CCfits/ -L /usr/local/src/CCfits/.libs/
  ./run_standalone test.fits
)

# CMakeLists.txt




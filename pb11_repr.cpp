/*

Makefile

g++ -O3 -Wall -shared -std=c++11 -fPIC `python3 -m pybind11 --includes` pb11_repr.cpp -o pb11_repr`python3-config --extension-suffix`
		
python3

import pb11_repr as rep
dir(rep)
si=rep.StarInfoT() # I was also calling si=rep.StarInfoT without the constructor ()
si.hfd=0.5
si.hfd
si
si.fwhmHz=0.9
si.fwhmVal=1.2
si.maxPixVal=2.1
si.saturated=0
si

*/

#include <pybind11/pybind11.h> // pybind11 header file
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <string>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

namespace py = pybind11;
using namespace std;

struct StarInfoT {
/*   FrameT     clusterFrame;
  FrameT     cogFrame;
  FrameT     hfdFrame;
  PixSubPosT CoG_Centr;
  PixSubPosT subPixelInterpCentroid;
 */  
  float      hfd;
  float      fwhmHz;
  float      fwhmVal;
  float      maxPixVal;
  bool       saturated;
};

PYBIND11_MODULE(pb11_repr, m) {
  //
  m.doc() = "star-tracker library for fits stellar images"; // optional module docstring

  py::class_<StarInfoT>(m, "StarInfoT", py::dynamic_attr() )   // py::dynamic_attr enables dynamic attributes for C++ classes 
    .def(py::init<>())                                                           // constructor
	//
/*  .def_readwrite("clusterFrame",           &StarInfoT::clusterFrame)           // FrameT
    .def_readwrite("cogFrame",               &StarInfoT::cogFrame)               // FrameT
    .def_readwrite("hfdFrame",               &StarInfoT::hfdFrame)               // FrameT
    .def_readwrite("CoG_Centr",              &StarInfoT::CoG_Centr)              // PixSubPosT
    .def_readwrite("subPixelInterpCentroid", &StarInfoT::subPixelInterpCentroid) // PixSubPosT  
 */    
    .def_readwrite("hfd",                    &StarInfoT::hfd)                    // float
    .def_readwrite("fwhmHz",                 &StarInfoT::fwhmHz)                 // float
    .def_readwrite("fwhmVal",                &StarInfoT::fwhmVal)                // float
    .def_readwrite("maxPixVal",              &StarInfoT::maxPixVal)              // float
    .def_readwrite("saturated",              &StarInfoT::saturated)              // bool
   
    .def("__repr__", [](const StarInfoT& d) { 
      std::stringstream buf;
      buf << "(hfd=" << d.hfd << " fwhmHz=" << d.fwhmHz << " fwhmVal=" << d.fwhmVal << " maxPixVal=" << d.maxPixVal << " saturated=" << d.saturated << ")\n"; 
      return buf.str();
    })
   
    .def("__str__", [](const StarInfoT& d) { 
      std::stringstream buf;
      buf << "(hfd=" << d.hfd << " fwhmHz=" << d.fwhmHz << " fwhmVal=" << d.fwhmVal << " maxPixVal=" << d.maxPixVal << " saturated=" << d.saturated << ")\n"; 
      return buf.str();
    });
  
} // PYBIND11_MODULE()

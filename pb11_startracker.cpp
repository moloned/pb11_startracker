// pb11_startracker.cpp

// pybind11 wrappers to expose star-tracker to python
//
// references
//
//   - pybind11 guide           https://yyc.solvcon.net/writing/2018/python_in_cpp/
//
//   - forum for questions/help https://gitter.im/pybind/Lobby
//   - pybind11 docs            https://pybind11.readthedocs.io/
//   - pybind11 numpy           http://people.duke.edu/~ccc14/cspy/18G_C++_Python_pybind11.html
//   - CImg data storage        https://cimg.eu/reference/group__cimg__storage.html
//   - Bindings for custom type https://pybind11.readthedocs.io/en/latest/classes.html#creating-bindings-for-a-custom-type
//                              https://ofstack.com/C++/27958/use-pybind11-to-encapsulate-the-c++-structure-as-a-parameter-of-the-function-implementation-steps.html
//                              https://awesomeopensource.com/project/tdegeus/pybind11_examples
//                              https://zenodo.org/record/239703/files/pybind11%20basics.pdf?download=1
//                              https://dmtn-014.lsst.io/
//                              https://jwbuurlage.github.io/blog/pybind11-plus-hana/
//                              http://cees-gitlab.stanford.edu/bob/hypercube/blob/ff34afd59e2dfbbe2eb95573a09d3b6aa9e99925/python/pyHyper.cpp
//                              https://gitlab.irf.se/programming-course/prog-course-material/blob/5e849bc4200e17215edb2af93d9e3f91b8a03c8c/lecture-tests/python-glue/pybind/pybind11/tests/pybind11_tests.cpp
//                              https://www.slideshare.net/corehard_by/mixing-c-python-ii-pybind11
//                              https://stackoverflow.com/questions/61778729/simplify-generating-wrapper-classes-in-pybind11-for-a-c-template-class-a-temp
//
#include "star_tracker.h"     // star-tracker header file

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

#define VERSION_INFO 0.0


// trivial star-tracker call passing in only filename
//
int tracker(char *fitsname) {
  run_startracker(fitsname);
  return 0;
} // tracker()


// wrapper function to perform npy to CImg conversion https://github.com/pybind/pybind11/issues/1042
//
inline py::array_t<float> cimg2npy(CImg<float> cimg) {    
  return py::array_t<float>({cimg.width(),cimg.height()},cimg.data());
} // cimg2npy()


// wrapper function to perform npy to CImg conversion
//
inline CImg<float> npy2cimg(py::array_t<float>& img_i) {
  py::buffer_info buf_i = img_i.request();
  bool      shared = true;  
  float*    i_data = reinterpret_cast<float*>(buf_i.ptr); // cast from void* ptr
  unsigned  cols   = buf_i.shape[0];
  unsigned  rows   = buf_i.shape[1];  
  CImg<float> img(i_data, rows, cols, 1, 1, shared); // populate CImg data-structure
  return img; // CImg is a struct so we can return it https://www.geeksforgeeks.org/return-local-array-c-function/
} // npy2cimg()


// return FITS file inFilename as ndarray
//
py::array_t<float> npy_readFile(const string & inFilename, long * outBitPix) {
  CImg<float> outBinImg;
  readFile(outBinImg, inFilename, outBitPix);
  return cimg2npy(outBinImg); // return inFilename as ndarray
} // npy_readFile()


// prn_array() print macro for pythonesque array printing
//   how multidimensional arrays are laid out in linear memory https://eli.thegreenplace.net/2015/memory-layout-of-multi-dimensional-arrays
//
#define adr(ptr,C,r,c) *(ptr + r*C +c)

// https://en.cppreference.com/w/cpp/utility/format/formatter
//#define prn_array_line(ptr,r,w) py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format(*(ptr+(w*r)+0),*(ptr+(w*r)+1),*(ptr+(w*r)+2),*(ptr+(w*r)-2),*(ptr+(w*r)-1),*(ptr+(w*r)-0))
#define prn_array_line(ptr,R,C,r) py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format(adr(ptr,C,r,0), adr(ptr,C,r,1), adr(ptr,C,r,2), adr(ptr,C,r,C-3), adr(ptr,C,r,C-2), adr(ptr,C,r,C-1))

// http://www.cplusplus.com/reference/iomanip/setfill/?kw=setfill
/* 
#define prn_array(ptr,h,w) setw(7) << setfill(' ')                            << \
                           " _data([" << prn_array_line(ptr, 0  ,w) << ",\n"  << \
                           "        " << prn_array_line(ptr, 1  ,w) << ",\n"  << \
					       "        " << prn_array_line(ptr, 2  ,w) << ",\n"  << \
					       "          ... ,\n"                                << \
					       "        " << prn_array_line(ptr, h-2,w) << ",\n"  << \
					       "        " << prn_array_line(ptr, h-1,w) << ",\n"  << \
					       "        " << prn_array_line(ptr, h  ,w) << "])\n" << \
						   ")\n"                                                 \
#define prn_array(ptr,R,C) setw(7) << setfill(' ')                            << \
                           " _data([" << prn_array_line(ptr,R,C,0) << ",\n"  << \
                           "        " << prn_array_line(ptr,R,C,1) << ",\n"  << \
					       "        " << prn_array_line(ptr,R,C,2) << ",\n"  << \
					       "          ... ,\n"                                << \
					       "        " << prn_array_line(ptr,R,C,R-3) << ",\n"  << \
					       "        " << prn_array_line(ptr,R,C,R-2) << ",\n"  << \
					       "        " << prn_array_line(ptr,R,C,R-1) << "])\n" << \
						   ")\n"                                                 \
*/
#define prn_array(p,C,R) setw(7) << setfill(' ')                            << \
                           " _data([" << py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format( \
						                 *(p+0*C+0), *(p+0*C+1), *(p+0*C+2), *(p+0*C+C-3), *(p+0*C+C-2), *(p+0*C+C-1)) << ",\n"  << \
                           "        " << py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format( \
						                 *(p+1*C+0), *(p+1*C+1), *(p+1*C+2), *(p+1*C+C-3), *(p+1*C+C-2), *(p+1*C+C-1)) << ",\n"  << \
					       "        " << py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format( \
						                 *(p+2*C+0), *(p+2*C+1), *(p+2*C+2), *(p+2*C+C-3), *(p+2*C+C-2), *(p+2*C+C-1)) << ",\n"  << \
					       "          ... ,\n"                                << \
					       "        " << py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format( \
						                 *(p+(R-3)*C+0), *(p+(R-3)*C+1), *(p+(R-3)*C+2), *(p+(R-3)*C+C-3), *(p+(R-3)*C+C-2), *(p+(R-3)*C+C-1)) << ",\n"  << \
					       "        " << py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format( \
						                 *(p+(R-2)*C+0), *(p+(R-2)*C+1), *(p+(R-2)*C+2), *(p+(R-2)*C+C-3), *(p+(R-2)*C+C-2), *(p+(R-2)*C+C-1)) << ",\n"  << \
					       "        " << py::str("[{:5.1f}, {:5.1f}, {:5.1f}, ... , {:5.1f}, {:5.1f}, {:5.1f}]").format( \
						                 *(p+(R-1)*C+0), *(p+(R-1)*C+1), *(p+(R-1)*C+2), *(p+(R-1)*C+C-3), *(p+(R-1)*C+C-2), *(p+(R-1)*C+C-1)) << "])\n" << \
						   ")\n"                                                 \

//const bool true = TRUE


// https://safijari.github.io/blog/posts/pybind11-karto/
//
// CImg data-structure from CImg.h
//   a templated declaration function is employed to avoid using macros :
//   https://stackoverflow.com/questions/61778729/simplify-generating-wrapper-classes-in-pybind11-for-a-c-template-class-a-temp
//
template<typename T> void gen_CImg(py::module &m, const std::string &typestr) {
	
  using Class = CImg<T>; // wrapper for CImg.h CImg() class
  
  std::string pyclass_name = std::string("CImg") + typestr;
  py::class_<Class>(m, pyclass_name.c_str(), py::buffer_protocol(), py::dynamic_attr())

  // CImg constructors
  //
  .def(py::init<>()) // default constructor
  //
  // Using py::array instead of py::buffer restricts the function to only NumPy arrays
  // https://github.com/pybind/pybind11/blob/master/tests/test_buffers.cpp
  //
  .def(py::init([](py::array b) { // initialise CImg from ndarray https://pybind11.readthedocs.io/en/stable/advanced/classes.html#custom-constructors
    py::buffer_info b_info = b.request(); // Request a buffer descriptor from Python
    // Some sanity checks ... 
    if (b_info.format != py::format_descriptor<T>::value) throw std::runtime_error("Incompatible format: expected a <T> array!");
    if (b_info.ndim != 2)                                 throw std::runtime_error("Incompatible buffer dimension!");
    return std::unique_ptr<Class>(new CImg<T>((T*)b_info.ptr, b_info.shape[0], b_info.shape[1], 1, 1, FALSE)); 
  })) // remember 2nd parentthesis )
  
  // wrap CImg fields for read/write access
  //
  .def_readwrite("_width",     &Class::_width)     // unsigned int
  .def_readwrite("_height",    &Class::_height)    // unsigned int
  .def_readwrite("_depth",     &Class::_depth)     // unsigned int
  .def_readwrite("_spectrum",  &Class::_spectrum)  // unsigned int
  .def_readwrite("_is_shared", &Class::_is_shared) // bool
  .def_readwrite("_data",      &Class::_data)      // T *

  // Provide buffer access https://pybind11.readthedocs.io/en/stable/advanced/pycpp/numpy.html
  //
  .def_buffer([](const Class& m) -> py::buffer_info {
  	return py::buffer_info(
      m._data,                            // pointer to buffer 
      sizeof(T),                          // size of one scalar (determined by template value T)
      py::format_descriptor<T>::format(), // python struct-style format descriptor 
      2,                                  // number of dimensions 
      { m._height, m._width },            // buffer dimensions (rows, columns)
      { sizeof(T) * m._width, sizeof(T) } // strides (in bytes) for each index  
    );
  })

  // Request a buffer descriptor from Pythonhttps://github.com/pybind/pybind11/blob/master/tests/test_buffers.cpp
  //
  .def("get_buffer_info", [](py::buffer b) { return b.request(); })
  
  // __repr__ to return a minimal summary of the object
  // https://developer.lsst.io/pybind11/style.html#classes-should-define-repr-to-return-a-minimal-summary-of-the-object-including-the-fully-qualified-name-of-the-class
  //
  .def("__repr__", [](const Class& d) { 
    std::stringstream obuf;
	T* ptr = d._data;
	
	obuf << "(_height=" << d._height << ", _width=" << d._width << ", _depth=" << d._depth << \
	        ", _spectrum=" << d._spectrum << ", _is_shared=" << d._is_shared << "\n" << \
	        prn_array(ptr, d._height, d._width); 
    return obuf.str();
  })
  
  // expose CImg C++ class (structure) member functions (methods)
  //   relies on relaying explicit calls rather than overload_cast
  //   https://stackoverflow.com/questions/59489366/pybind11-stuck-with-unresolved-overloaded-function-type
  //
  .def("width",    [](const Class& d) { return d.width();              }) //_width; }) 
  .def("height",   [](const Class& d) { return d.height();             }) // _height; })
  .def("depth",    [](const Class& d) { return d.depth();              }) // _depth; })
  .def("spectrum", [](const Class& d) { return d.spectrum();           }) //_spectrum; })
  .def("ptr",      [](const Class& d) { return d.data();               }) // _data; })
  .def("data",     [](const Class& d) { return d.data();               }) // _data; })
  //
  // as per https://pybind11.readthedocs.io/en/stable/advanced/functions.html#positional-only-arguments f
  // format for default args is of the form .def("function_name", []( Class& d, <args>) { return d.<blah>; } , <default args> )
  //
  .def("blur_anisotropic", []( Class& d, const float amplitude, const float sharpness=0.7f, const float anisotropy=0.6f,
                               const float alpha=0.6f, const float sigma=1.1f, const float dl=0.8f, const float da=30,
                               const float gauss_prec=2, const unsigned int interpolation_type=0,
                               const bool is_fast_approx=true ) { return d.blur_anisotropic(amplitude, sharpness, anisotropy,
                               alpha, sigma, dl, da, gauss_prec, interpolation_type, is_fast_approx); }, 
							   py::arg("amplitude")=30, py::arg("sharpness")=0.7, py::arg("anisotropy")=0.6,
                               py::arg("alpha")=0.6, py::arg("sigma")=1.1, py::arg("l")=0.8, py::arg("da")=30,
                               py::arg("gauss_prec")=2, py::arg("interpolation_type")=0, py::arg("is_fast_approx")=1) 
	   
  // additional useful functions
  //
  .def("size",     [](const Class& d) { return d.width() * d.height(); }) // _width * d._height; })
  .def("shape",    [](const Class& d) { return std::make_tuple(d.width() , d.height()); }) // _width, d._height); })// return shape of CImg object
  ;

} // gen_CImg()
  
 
// np_startracker
//
//   called using 2D Ndarray extracted from fits file 
//   uses astropy or similar python library
//   removes CCfits dependency on C++ code
//
int np_startracker(py::array_t<float>& img_i) {
  CImg<float> img = npy2cimg(img_i);        // input data has to be int16
  const unsigned int outerHfdDiameter = 21; // outerHfdDiameter depends on pixel size and focal length (and seeing...).                        
  StarInfoListT      si;                    // star-tracker pipeline output: centroids & other parameters
  long               bitPix = 0;            //
  //cout << "pipeline" << endl; 
  si = st_pipeline(img, bitPix, outerHfdDiameter); // perform pipeline signal processing on input image img
  //cout << "render_output" << endl; 
  char fname[25] = "test.fits";
  render_output(fname, img, si, outerHfdDiameter); // render output image with recovered centroids & parameters
  return 0;
} // np_startracker()


// startracker
//
//   called using CImg extracted from fits file 
//   uses astropy or similar python library
//   removes CCfits dependency on C++ code
//
int startracker(CImg<float>& img) { 
  const unsigned int outerHfdDiameter = 21; // outerHfdDiameter depends on pixel size and focal length (and seeing...).                        
  StarInfoListT      si;                    // star-tracker pipeline output: centroids & other parameters
  long               bitPix = 0;            //
  //cout << "pipeline" << endl; 
  si = st_pipeline(img, bitPix, outerHfdDiameter); // perform pipeline signal processing on input image img
  //cout << "render_output" << endl; 
  char fname[25] = "test.fits";
  render_output(fname, img, si, outerHfdDiameter); // render output image with recovered centroids & parameters
  return 0;
} // np_startracker()


// calculate 2D SNR for image
//   https://www.lost-infinity.com/easy-2d-signal-to-noise-ratio-snr-calculation-for-images-to-find-potential-stars-without-extracting-the-background-noise/
//
double np_calc_snr(py::array_t<float>& img_i) {
  double snr = calc_snr(npy2cimg(img_i));
  return snr;
} // np_calc_snr()


// star-tracker pipeline call passing in a 2D Ndarray extracted from a fits file in python
//
StarInfoListT np_pipeline(py::array_t<float>& img_i) { 
  CImg<float> img = npy2cimg(img_i);        // input data has to be int16
  const unsigned int outerHfdDiameter = 21; // outerHfdDiameter depends on pixel size and focal length (and seeing...).                        
  StarInfoListT      si;                    // star-tracker pipeline output: centroids & other parameters
  long               bitPix = 0;            //
  si = st_pipeline(img, bitPix, outerHfdDiameter); // perform pipeline signal processing on input image img
  return si;
} // np_pipeline()


// wrapper for anisotropic noise reduction (CImg library)
//
py::array_t<float> np_anisotropic_nr(py::array_t<float>& img_i, long inBitPix) {

  CImg<float> img = npy2cimg(img_i); // convert from ndarray input to CImg format

  // Anisotropic Noise Filtering from CImg
  //
  // AD noise reduction --> In: Loaded image, Out: Noise reduced image
  // NOTE: This step takes a while for big images... too long for usage in a loop ->
  //       Should only be used on image segments, later...
  //
  // http://cimg.sourceforge.net/reference/structcimg__library_1_1CImg.html
  //
  float amplitude          = 30.0f;
  float sharpness          = 0.7f;  
  float anisotropy         = 0.3f;  
  float alpha              = 0.6f; 
  float sigma              = 1.1f;  
  float dl                 = 0.8f;  
  float da                 = 30;    
  float gauss_prec         = 2;     
  int   interpolation_type = 0;     
  bool  fast_approx        = false;  

  CImg<float> & aiImg = img.blur_anisotropic(amplitude, sharpness, anisotropy, alpha, sigma, dl, 
                                             da, gauss_prec, interpolation_type, fast_approx
					                        );
						  
  return cimg2npy(aiImg); // return noise-reduced aiImg as ndarray						  
} // np_anisotropic_nr()


// slicing to separate stars from background using Otsu method
//
// function wrapped to accept and return numpy arrays
// https://stackoverflow.com/questions/10701514/how-to-return-numpy-array-from-boostpython
//
py::array_t<float> np_thresholdOtsu(py::array_t<float>& img_i, long inBitPix) {
  CImg<float> img = npy2cimg(img_i); // convert input ndarray to CImg
  CImg<float> outBinImg = img;
  thresholdOtsu(img, inBitPix, outBinImg); // perform Otsu thresholding
  return cimg2npy(outBinImg); // return Otsu-thresholded outBinImg as ndarray
} // np_thresholdOtsu()


// slicing to separate stars from background using MaxEntropy method
//   https://www.lost-infinity.com/fast-max-entropy-thresholding-for-16-bit-images-with-cimg/
//
py::array_t<float> np_thresholdMaxEntropy(py::array_t<float>& img_i, long inBitPix) {
  CImg<float> img = npy2cimg(img_i); // convert ndarray to CImg
  CImg<float> outBinImg = img;
  //thresholdMaxEntropy(img, inBitPix, &outBinImg); // perform Maximum Entropy thresholding
  outBinImg = thresholdMaxEntropy(img, inBitPix); // perform Maximum Entropy thresholding
  return cimg2npy(outBinImg); // return Maximum Entropy-thresholded outBinImg as ndarray
} // np_thresholdMaxEntropy()


// Clustering gropups pixels which belong together and the result of the clustering process 
// is a list of stars and the pixels belonging to each star. The algorithm is based on 
// “Improving night sky star image processing algorithm for star sensors“ M. Vali Arbabmir et. al. 
// This implementation assumes the supplied image is not modified during the process and a set 
// is used to contain all white pixels, however performance could be improved by using a 
// 2D array rathet than a list. 
//
// https://www.lost-infinity.com/night-sky-image-processing-part-3-star-clustering/
//
StarInfoListT np_clusterStars(py::array_t<float>& img_i) {
  CImg<float> img = npy2cimg(img_i);
  StarInfoListT outStarInfos;
  clusterStars<float>(img, &outStarInfos); //
  return outStarInfos;
} // np_clusterStars()


// iterative centroiding (default 10 iterations)
// uses std::tuple<> to return multiple values https://www.geeksforgeeks.org/returning-multiple-values-from-a-function-using-tuple-and-pair-in-c/
//
std::tuple<PixSubPosT, PixSubPosT> np_calcCentroid(py::array_t<float>& img_i, const FrameT & inFrame, PixSubPosT outPixelPos, PixSubPosT outSubPixelPos, size_t inNumIterations) {
  CImg<float> img = npy2cimg(img_i);
  calcCentroid(img, inFrame, &outPixelPos, &outSubPixelPos, inNumIterations);
  return std::make_tuple(outPixelPos, outSubPixelPos); // return multiple values https://pybind11.readthedocs.io/en/stable/faq.html#limitations-involving-reference-arguments
} // np_calcCentroid()
  

// HFD half flux diameter measurement
// https://www.lost-infinity.com/night-sky-image-processing-part-6-measuring-the-half-flux-diameter-hfd-of-a-star-a-simple-c-implementation/
//
float np_calcHfd(py::array_t<float>& img_i, unsigned int inOuterDiameter) {
  CImg<float> img = npy2cimg(img_i);
  return calcHfd(img, inOuterDiameter);
} // np_calcHfd()


///////////////////////////////////////////////////////////////////
// star-tracker support functions
///////////////////////////////////////////////////////////////////
//

// calculate max entropy threshold - wrapper for Python integration to convert numpy to CImg format
//
float np_calcMaxEntropyThreshold(py::array_t<float>& img_i) { // pass in image from python as numpy array
  CImg<float> img = npy2cimg(img_i);
  return calcMaxEntropyThreshold(img);
} // 

float np_calcIx2(py::array_t<float>& img_i, int x) {
  CImg<float> img = npy2cimg(img_i);
  return calcIx2(img, x);
}

float np_calcJy2(py::array_t<float>& img_i, int y) {
  CImg<float> img = npy2cimg(img_i);
  return calcJy2(img, y);
}

py::array_t<float> np_readFile(const string & inFilename, long * outBitPix) {
  CImg<float> outBinImg;
  readFile(outBinImg, inFilename, outBitPix);
  return cimg2npy(outBinImg); // return inFilename as ndarray
} // np_readFile()
  

// star-tracker pybind11 module entry points
//
//   pb11_startracker is the name of the exported module and m is a variable we can use to 
//   modify the module and add functions and classes to it
//
//
PYBIND11_MODULE(pb11_startracker, m) {
  
  // optional module docstring	
  //
  m.doc() = "star-tracker library for fits stellar images"; 

  // wrapped c++ star-tracker functions we want to expose to python
  //
  m.def("tracker",     &tracker,     "star-tracker (fits filename)"); 
  
  //
  // numpy interface
  //
  m.def("np_readFile", &np_readFile, "return FITS file inFilename as ndarray");
  //
  m.def("np_calc_snr",                &np_calc_snr,                "2D SNR calculator (numpy)"); 
  //
  m.def("np_startracker",             &np_startracker,             "star-tracker (numpy)");
  m.def("np_pipeline",                &np_pipeline,                "star-tracker pipeline (numpy)"); 
  m.def("np_anisotropic_nr",          &np_anisotropic_nr,          "anisotropic noise reduction (numpy)"); 
  m.def("np_thresholdOtsu",           &np_thresholdOtsu,           "Otsu thresholding (numpy)");
  m.def("np_thresholdMaxEntropy",     &np_thresholdMaxEntropy,     "Max-entropy thresholding (numpy)");
  m.def("np_clusterStars",            &np_clusterStars,            "star pixel clustering (numpy)");
  m.def("np_calcCentroid",            &np_calcCentroid,            "iterative centroiding (numpy)");
  m.def("np_calcHfd",                 &np_calcHfd,                 "HFD half flux diameter (numpy)");
  m.def("np_calcMaxEntropyThreshold", &np_calcMaxEntropyThreshold, "calculate Max Entropy Threshold (numpy)");

  //
  // CImg interface
  //
  m.def("calc_snr",                &calc_snr,                "calculate SNR of image (CImg)");
  //
  m.def("st_pipeline",             &st_pipeline,             "star-tracker pipeline (CImg)");
  m.def("calcMaxEntropyThreshold", &calcMaxEntropyThreshold, "calculate Max Entropy Threshold (CImg)");
  m.def("thresholdMaxEntropy",     &thresholdMaxEntropy,     "Max-entropy thresholding (CImg)");
  m.def("thresholdOtsu",           &thresholdOtsu,           "Otsu thresholding (CImg)");

  // supporting functions
  //
  m.def("rectify",                 &rectify,                 ""); // part of clusterStars() output(StarInfoListT) processing
  
  // typedef list<StarInfoT> StarInfoListT;
  //
  py::class_<StarInfoListT>(m, "StarInfoListT", py::dynamic_attr())
    .def(py::init<>()) // inherits StarInfoT automagically!
	
	// size(), begin() and end() are required for main star processing loop in st_pipeline()
	//   for (StarInfoListT::iterator it = starInfos.begin(); it != starInfos.end(); ++it) { // For each star
	//
    .def("begin", [](const StarInfoListT& d) { return d.begin(); })
    .def("end",   [](const StarInfoListT& d) { return d.end();   })
    .def("size",  [](const StarInfoListT& d) { return d.size();  })
	
  ; // py::class_<StarInfoListT>
  
  
  // input parameter <4-entry tuple> to np_calcCentroid() iterative centroiding algorithm
  //
  // typedef tuple<float,float,float,float> FrameT;        // <x1, y1, x2, y2>
  //
  py::class_<FrameT>(m, "FrameT", py::dynamic_attr()) // py::dynamic_attr enables dynamic attributes for C++ classes 
    //
	.def(py::init<>()) // inherits StarInfoT automagically!
  ;	// py::class_<FrameT>


  // input parameter <2-entry tuple> to np_calcCentroid() iterative centroiding algorithm
  //
  // typedef tuple<float,float,float,float> FrameT;        // <x1, y1, x2, y2>
  //
  py::class_<PixSubPosT>(m, "PixSubPosT", py::dynamic_attr()) // py::dynamic_attr enables dynamic attributes for C++ classes 
    //
	.def(py::init<>()) // inherits StarInfoT automagically!
  ;	// py::class_<PixSubPosT>


  // input parameter <2-entry tuple> to np_calcCentroid() iterative centroiding algorithm
  //
  // typedef tuple<float,float,float,float> FrameT;        // <x1, y1, x2, y2>
  //
  py::class_<PixelPosT>(m, "PixelPosT", py::dynamic_attr()) // py::dynamic_attr enables dynamic attributes for C++ classes 
    //
	.def(py::init<>()) // inherits StarInfoT automagically!
  ;	// py::class_<PixelPosT>


  py::class_<PixelPosListT>(m, "PixelPosListT", py::dynamic_attr()) // py::dynamic_attr enables dynamic attributes for C++ classes 
    //
	.def(py::init<>()) // inherits PixelPosT automagically!
	
	// clusterStars() requires .begin(), .end(),   .size(),  .insert(), 
	//                         .erase(), .empty(), .front(), .pop_front(), 
	//                         .push_back()
	//
    .def("begin",     [](const PixelPosListT& d)   { return d.begin(); })
    .def("end",       [](const PixelPosListT& d)   { return d.end();   })
    .def("size",      [](const PixelPosListT& d)   { return d.size();  })
    //.def("insert",    [](const PixelPosListT& d, ) { return d.insert();  })
    //.def("erase",     [](const PixelPosListT& d, PixelPosSetT::iterator itWhitePixPos) { return d.erase(itWhitePixPos); })
    .def("empty",     [](const PixelPosListT& d)   { return d.empty();  })
    .def("front",     [](const PixelPosListT& d)   { return d.front();  })
    //.def("pop_front", [](const PixelPosListT& d)   { return d.pop_front();  })
    //.def("push_back", [](const PixelPosListT& d, PixelPosSetT::iterator itWhitePixPos) { return d.push_back(*itWhitePixPos); })
	
  ;	// py::class_<PixelPosListT>


  py::class_<PixelPosSetT>(m, "PixelPosSetT", py::dynamic_attr()) // py::dynamic_attr enables dynamic attributes for C++ classes 
    //
	.def(py::init<>()) // inherits PixelPosT automagically!
  ;	// py::class_<PixelPosSetT>


  // StarInfoT data-structure from star-tracker
  //
  py::class_<StarInfoT>(m, "StarInfoT", py::dynamic_attr()) // py::dynamic_attr enables dynamic attributes for C++ classes 
    //
	.def(py::init<>()) // default constructor                                              // constructor
	//
	// tuples don't require explicit bindings https://developer.lsst.io/pybind11/style.html#id40
	//
    .def_readwrite("clusterFrame",           &StarInfoT::clusterFrame)           // FrameT
    .def_readwrite("cogFrame",               &StarInfoT::cogFrame)               // FrameT
    .def_readwrite("hfdFrame",               &StarInfoT::hfdFrame)               // FrameT
    .def_readwrite("CoG_Centr",              &StarInfoT::CoG_Centr)              // PixSubPosT
    .def_readwrite("subPixelInterpCentroid", &StarInfoT::subPixelInterpCentroid) // PixSubPosT  
    .def_readwrite("hfd",                    &StarInfoT::hfd)                    // float
    .def_readwrite("fwhmHz",                 &StarInfoT::fwhmHz)                 // float
    .def_readwrite("fwhmVal",                &StarInfoT::fwhmVal)                // float
    .def_readwrite("maxPixVal",              &StarInfoT::maxPixVal)              // float
    .def_readwrite("saturated",              &StarInfoT::saturated)              // bool
    //
    .def("__repr__", [](const StarInfoT& d) { // __repr__ should return a minimal summary of the object
	                                          // https://developer.lsst.io/pybind11/style.html#classes-should-define-repr-to-return-a-minimal-summary-of-the-object-including-the-fully-qualified-name-of-the-class
      return py::str("({},{},{},{},{},{})\n").format(d.clusterFrame, d.hfd, d.fwhmHz, d.fwhmVal, d.maxPixVal, d.saturated); 
    })
    //
    .def("__str__", [](const StarInfoT& d) { // __str__ should return a human readable string representation of the object
	                                         // https://developer.lsst.io/pybind11/style.html#id46
      return py::str("pb11_startracker::StarInfoT(<()>,{},{},{},{},{})\n").format(d.clusterFrame, d.hfd, d.fwhmHz, d.fwhmVal, d.maxPixVal, d.saturated); 
    })
    //
  ; // py::class_<StarInfoT>


  // call gen_CImg() once for each template type <T>
  //
  gen_CImg<uint8_t> (m,"_uint8_t"); // doesn't work unless you specify template type in function call!
  gen_CImg<int8_t>  (m,"_int8_t");
  gen_CImg<uint16_t>(m,"_uint16_t");
  gen_CImg<int16_t> (m,"_int16_t");
  gen_CImg<uint32_t>(m,"_uint32_t");
  gen_CImg<int32_t> (m,"_int32_t");
  gen_CImg<uint64_t>(m,"_uint64_t");
  gen_CImg<int64_t> (m,"_int64_t");
  gen_CImg<float>   (m,"_float");
  gen_CImg<double>  (m,"_double");
 
 
  // return library version number
  //
  m.attr("__version__") = VERSION_INFO;
  
} // PYBIND11_MODULE()

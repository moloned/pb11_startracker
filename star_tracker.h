// star_tracker.h

// configure application to link external jpeg library libjpeg
//
#define cimg_use_jpeg // make sure to define this to avoid runtime error
//
//  [CImg] *** CImgIOException *** [instance(1104,800,1,3,0x7f6a0c230010,non-shared)] CImg<unsigned char>::save_other(): Failed to save file 'startracker_output.jpeg'. Format is not natively supported, and no external commands succeeded.
//  terminate called after throwing an instance of 'cimg_library::CImgIOException'
//   what():  [instance(1104,800,1,3,0x7f6a0c230010,non-shared)] CImg<unsigned char>::save_other(): Failed to save file 'startracker_output.jpeg'. Format is not natively supported, and no external commands succeeded.
//  make: *** [Makefile:4: star_tracker] Aborted (core dumped)


#ifndef __STARTRACKER__
#define __STARTRACKER__

//
//
#include <iostream>
#include <assert.h>

#include <CImg.h>        // C++ image-processing library
//#include "CImg.h"
namespace ci = cimg_library;

#include <CCfits/CCfits> // C++ wrapper for cfitsio
 
#include <tuple>
#include <functional>
#include <list>
#include <set>
#include <array>
#include <vector>
#include <cmath> // for std::sqrt() in calc_snr() https://www.programiz.com/cpp-programming/library-function/cmath/sqrt
 
#include <gsl/gsl_multifit_nlin.h>

// https://stackoverflow.com/questions/3156778/no-matching-function-for-call-to-operator-new
// 
#include <new>

// pipeline configuration
//
//enum threshold_method{OTSU, MAX_ENTROPY};
//enum centroiding_method{COG, WCOG, IWG};

#define OTSU 0
#define MAX_ENTROPY 1
//
//#define THRESHOLD_METHOD   OTSU
#define THRESHOLD_METHOD   MAX_ENTROPY // this method is more accurate

#define COG 0
#define WCOG 1
#define IWG 2
#define CENTROIDING_METHOD COG

using namespace std;
using namespace cimg_library;
using namespace CCfits;

typedef tuple<int,int>                 PixelPosT;     // <x,y>
typedef set<PixelPosT>                 PixelPosSetT;
typedef list<PixelPosT>                PixelPosListT;

typedef tuple<float,float>             PixSubPosT;
typedef tuple<float,float,float,float> FrameT;        // <x1, y1, x2, y2>

struct StarInfoT {
  FrameT     clusterFrame;
  FrameT     cogFrame;
  FrameT     hfdFrame;
  PixSubPosT CoG_Centr;
  PixSubPosT subPixelInterpCentroid;
  float      hfd;
  float      fwhmHz;
  float      fwhmVal;
  float      maxPixVal;
  bool       saturated;
};

typedef list<StarInfoT> StarInfoListT;

/**********************************************************************
* Helper classes
**********************************************************************/
struct DataPointT {
  float x;
  float y;
  DataPointT(float inX = 0, float inY = 0) : x(inX), y(inY) {}
};
  
typedef vector<DataPointT> DataPointsT;
  
struct GslMultiFitDataT {
  float y;
  float sigma;
  DataPointT pt;
};
  
typedef vector<GslMultiFitDataT> GslMultiFitParmsT;
 
 
/**********************************************************************
* Curve to fit to is supplied by traits.
**********************************************************************/
template <class FitTraitsT>
class CurveFitTmplT {
public:
  typedef typename FitTraitsT::CurveParamsT CurveParamsT;
 
  /**
   * DataAccessor allows specifying how x,y data is accessed.
   * See http://en.wikipedia.org/wiki/Approximation_error for expl. of rel and abs errors.
   */
  template<typename DataAccessorT> static int  
  fitGslLevenbergMarquart(const typename DataAccessorT::TypeT & inData, typename CurveParamsT::TypeT * outResults,
          double inEpsAbs, double inEpsRel, size_t inNumMaxIter = 500) {
    GslMultiFitParmsT gslMultiFitParms(inData.size());
      
    // Fill in the parameters
    for (typename DataAccessorT::TypeT::const_iterator it = inData.begin(); it != inData.end(); ++it) {
      size_t idx = std::distance(inData.begin(), it);
      const DataPointT & dataPoint = DataAccessorT::getDataPoint(idx, it);
      gslMultiFitParms[idx].y     = dataPoint.y;
      gslMultiFitParms[idx].sigma = 0.1f;
      gslMultiFitParms[idx].pt    = dataPoint;
    }
 
    // Fill in function info
    gsl_multifit_function_fdf f;
    f.f      = FitTraitsT::gslFx;
    f.df     = FitTraitsT::gslDfx;
    f.fdf    = FitTraitsT::gslFdfx;
    f.n      = inData.size();
    f.p      = FitTraitsT::CurveParamsT::_Count;
    f.params = & gslMultiFitParms;
    
 
    gsl_vector * guess = gsl_vector_alloc(FitTraitsT::CurveParamsT::_Count);  // Allocate the guess vector
    
    FitTraitsT::makeGuess(gslMultiFitParms, guess);  // Make initial guesses based on the data
    
    // Create a Levenberg-Marquardt solver with n data points and m parameters
    gsl_multifit_fdfsolver * solver = gsl_multifit_fdfsolver_alloc(gsl_multifit_fdfsolver_lmsder,
                                                                  inData.size(), FitTraitsT::CurveParamsT::_Count);
    gsl_multifit_fdfsolver_set(solver, & f, guess);  // Initialize the solver
    
    int status, i = 0;
    
    // Iterate to to find a result
    do {
      i++;
      status = gsl_multifit_fdfsolver_iterate(solver); // returns 0 in case of success
      if (status) {  break; }
      status = gsl_multifit_test_delta(solver->dx, solver->x, inEpsAbs, inEpsRel);
    } while (status == GSL_CONTINUE && (size_t)i < inNumMaxIter);
    
    // Store the results to be returned to the user (copy from gsl_vector to result structure)
    for (size_t i = 0; i < FitTraitsT::CurveParamsT::_Count; ++i) {
      typename FitTraitsT::CurveParamsT::TypeE idx = static_cast<typename FitTraitsT::CurveParamsT::TypeE>(i);
      (*outResults)[idx] = gsl_vector_get(solver->x, idx);
    }
 
    // Free GSL memory
    gsl_multifit_fdfsolver_free(solver);
    gsl_vector_free(guess);
 
    return status;
  }
};
 
/**********************************************************************
* Gaussian fit traits
**********************************************************************/
class GaussianFitTraitsT {
private:
  
public:
  struct CurveParamsT {
    // b = base, p = peak, c = center in x, w = mean width (FWHM)
    enum TypeE { B_IDX = 0, P_IDX, C_IDX, W_IDX, _Count };
    struct TypeT : public std::array<float, TypeE::_Count> {
      TypeT(const gsl_vector * inVec = 0) {
        for (size_t i = 0; i < TypeE::_Count; ++i) {
          TypeE idx = static_cast<TypeE>(i);
          (*this)[i] = (inVec ? gsl_vector_get(inVec, idx) : 0);
        }
      }
    };
  };
 
  /* Makes a guess for b, p, c and w based on the supplied data */
  static void makeGuess(const GslMultiFitParmsT & inData, gsl_vector * guess) {
    size_t numDataPoints = inData.size();
    float y_mean = 0;
    float y_max = inData.at(0).pt.y;
    float c = inData.at(0).pt.x;
    
    for(size_t i = 0; i < numDataPoints; ++i) {
      const DataPointT & dataPoint = inData.at(i).pt;
 
      y_mean += dataPoint.y;
      
      if(y_max < dataPoint.y) {
        y_max = dataPoint.y;
        c = dataPoint.x;
      }
    }
 
    y_mean /= (float) numDataPoints;
    float w = (inData.at(numDataPoints - 1).pt.x - inData.at(0).pt.x) / 10.0;
    
    gsl_vector_set(guess, CurveParamsT::B_IDX, y_mean);
    gsl_vector_set(guess, CurveParamsT::P_IDX, y_max);
    gsl_vector_set(guess, CurveParamsT::C_IDX, c);
    gsl_vector_set(guess, CurveParamsT::W_IDX, w);
  }
 
  /* y = b + p * exp(-0.5f * ((t - c) / w) * ((t - c) / w)) */
  static float fx(float x, const CurveParamsT::TypeT & inParms) {
    float b = inParms[CurveParamsT::B_IDX];
    float p = inParms[CurveParamsT::P_IDX];
    float c = inParms[CurveParamsT::C_IDX];
    float w = inParms[CurveParamsT::W_IDX];
    float t = ((x - c) / w);
    t *= t;
    return (b + p * exp(-0.5f * t));
  }
 
  /* Calculates f(x) = b + p * e^[0.5*((x-c)/w)] for each data point. */
  static int gslFx(const gsl_vector * x, void * inGslParams, gsl_vector * outResultVec) {    
    CurveParamsT::TypeT curveParams(x);     // Store the current coefficient values
    const GslMultiFitParmsT * gslParams = ((GslMultiFitParmsT*) inGslParams); // Store parameter values
 
    //Execute Levenberg-Marquart on f(x)
    for(size_t i = 0; i < gslParams->size(); ++i) {
      const GslMultiFitDataT & gslData = gslParams->at(i);
      float yi = GaussianFitTraitsT::fx((float) gslData.pt.x, curveParams);
      gsl_vector_set(outResultVec, i, (yi - gslData.y) / gslData.sigma);
    }
    return GSL_SUCCESS;
  }
 
  /* Calculates the Jacobian (derivative) matrix of f(x) = b + p * e^[0.5*((x-c)/w)^2] for each data point */
  static int gslDfx(const gsl_vector * x, void * params, gsl_matrix * J) {
    
    // Store parameter values
    const GslMultiFitParmsT * gslParams = ((GslMultiFitParmsT*) params);
    
    // Store current coefficients
    float p = gsl_vector_get(x, CurveParamsT::P_IDX);
    float c = gsl_vector_get(x, CurveParamsT::C_IDX);
    float w = gsl_vector_get(x, CurveParamsT::W_IDX);
    
    // Store non-changing calculations
    float w2 = w * w;
    float w3 = w2 * w;
    
    for(size_t i = 0; i < gslParams->size(); ++i) {
      const GslMultiFitDataT & gslData = gslParams->at(i);
      float x_minus_c = (gslData.pt.x - c);
      float e = exp(-0.5f * (x_minus_c / w) * (x_minus_c / w));
      
      gsl_matrix_set(J, i, CurveParamsT::B_IDX, 1 / gslData.sigma);
      gsl_matrix_set(J, i, CurveParamsT::P_IDX, e / gslData.sigma);
      gsl_matrix_set(J, i, CurveParamsT::C_IDX, (p * e * x_minus_c) / (gslData.sigma * w2));
      gsl_matrix_set(J, i, CurveParamsT::W_IDX, (p * e * x_minus_c * x_minus_c) / (gslData.sigma * w3));
    }    
    return GSL_SUCCESS;
  }
  
  /* Invokes f(x) and f'(x) */
  static int gslFdfx(const gsl_vector * x, void * params, gsl_vector * f, gsl_matrix * J) {
    gslFx(x, params, f);
    gslDfx(x, params, J);
    
    return GSL_SUCCESS;
  }
};
 
typedef list<PixSubPosT> MyDataContainerT;
 
class MyDataAccessorT {
public:
  typedef MyDataContainerT TypeT;
  static DataPointT getDataPoint(size_t inIdx, TypeT::const_iterator inIt) {
    const PixSubPosT & pos = *inIt;
    DataPointT dp(get<0>(pos) /*inIdx*/, get<1>(pos) /*y*/);
    return dp;
  }
};

// star-tracker function prototypes
// 
void readFile(CImg<float> & inImg, const string & inFilename, long * outBitPix = 0);

// separate stars from background using max-entropy method                                    // wrapped
float calcMaxEntropyThreshold(const CImg<float> & img);                                         // wrapped
CImg<float> thresholdMaxEntropy(const CImg<float> & img, long inBitPix); //, CImg<float> * outBinImg); // wrapped
  
// separate stars from background using Otsu method
//void thresholdOtsu(const CImg<float> & inImg, long inBitPix, CImg<float> * outBinImg);     // wrapped
CImg<float> thresholdOtsu(const CImg<float> & inImg, long inBitPix, CImg<float> outBinImg);     // wrapped

void getAndRemoveNeighbours(PixelPosT inCurPixelPos, PixelPosSetT * inoutWhitePixels, PixelPosListT * inoutPixelsToBeProcessed);

template<typename T> void clusterStars(const CImg<T> & inImg, StarInfoListT * outStarInfos);

void calcIntensityWeightedCenter(const CImg<float> & inImg, float * outX, float * outY);

void calcSubPixelCenter(const CImg<float> & inImg, float * outX, float * outY, size_t inNumIter = 10);

void calcCentroid(const CImg<float> & inImg, const FrameT & inFrame, PixSubPosT * outPixelPos, PixSubPosT * outSubPixelPos = 0, size_t inNumIterations = 10);

float calcHfd(const CImg<float> & inImage, unsigned int inOuterDiameter);                  // wrapped
                                                         // wrapped
double calc_snr(const CImg<float> & img);                                                  // wrapped

float calcIx2(const CImg<float> & img, int x);                                             // wrapped
float calcJy2(const CImg<float> & img, int y);                                             // wrapped
bool insideCircle(float inX, float inY, float inCenterX, float inCenterY, float inRadius); // wrapped
template <typename T> static bool isAlmostEqual(T x, T y);                                 // wrapped

// star-tracker image processing pipeline
StarInfoListT st_pipeline(CImg<float> & img, long bitPix, int outerHfdDiameter);           // wrapped

// render star-tracker output in jpeg image
int render_output(char *fitsname, CImg<float> & img, StarInfoListT starInfos, int outerHfdDiameter);

// run pipeline and render in one function
int run_startracker(char *fitsname);                                                       // wrapped

// function contained in st_pipeline()
FrameT rectify(const FrameT & inFrame);

#endif // __STARTRACKER__

// star_tracker.h


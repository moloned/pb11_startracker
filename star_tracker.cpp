// star_tracker.cpp

// https://github.com/carsten0x51h/star_recognizer

/**
 * Star recognizer using the CImg library and CCFits.
 *
 * Copyright (C) 2015 Carsten Schmitt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "star_tracker.h"


// Get all pixels inside a radius: http://stackoverflow.com/questions/14487322/get-all-pixel-array-inside-circle
// Algorithm: http://en.wikipedia.org/wiki/Midpoint_circle_algorithm
//
bool insideCircle(float inX /*pos of x*/, float inY /*pos of y*/, float inCenterX, float inCenterY, float inRadius) {
  return (pow(inX - inCenterX, 2.0) + pow(inY - inCenterY, 2.0) <= pow(inRadius, 2.0));
} // insideCircle()

void readFile(CImg<float> & inImg, const string & inFilename, long * outBitPix /*= 0*/) {
  //std::auto_ptr<FITS> pInfile(new FITS(inFilename, Read, true));
  // https://stackoverflow.com/questions/45053626/error-templateclass-class-stdauto-ptr-is-deprecated
  std::shared_ptr<FITS> pInfile(new FITS(inFilename, Read, true));
  PHDU & image = pInfile->pHDU(); 

  if (outBitPix) *outBitPix = image.bitpix(); // bitpix is the data-type used to represent pixels in the fits file https://docs.astropy.org/en/stable/io/fits/usage/image.html

  inImg.resize(image.axis(0) /*x*/, image.axis(1) /*y*/, 1/*z*/, 1 /*1 color*/);
  
  // NOTE: At this point we assume that there is only 1 layer.
  std::valarray<unsigned long> imgData;
  image.read(imgData);
  cimg_forXY(inImg, x, y) { inImg(x, inImg.height() - y - 1) = imgData[inImg.offset(x, y)]; }  
} // readFile()

// https://stackoverflow.com/questions/19837576/comparing-floating-point-number-to-zero
// see Knuth section 4.2.2 pages 217-218
//
template <typename T> static bool isAlmostEqual(T x, T y) {
  return std::abs(x - y) <= std::numeric_limits<T>::epsilon() * std::abs(x);
} // isAlmostEqual()
 
// https://www.lost-infinity.com/fast-max-entropy-thresholding-for-16-bit-images-with-cimg/
//
float calcMaxEntropyThreshold( const CImg<float> & img) {
  size_t numBins = 256;
  std::vector<float> hist(numBins, 0);
 
  float max;
  float min = img.min_max(max);
 
  std::cout << "numBins=" << numBins << ", min=" << min << ", max=" << max
	    << ", img-dim (w x h)=" << img.width() << " x " << img.height() << std::endl;
  
  /**
   * IDEA: "Shrink" /map the float image pixel values to 256 possible
   * brightness levels (i.e. 256 histogram bins). The reason is that
   * the performance of the "max entropy algorithm" strongly depends
   * on the histogram size / number of bins. Note that the threshold
   * which will be calculated based on this histogram will be the
   * correct one for this "shrinked" histogram. In order to get the
   * threshold for the initial float image, this transformation needs
   * to be reverted later (see comment below).
   */
  cimg_forXY(img, x, y) {
    int idx = (numBins - 1) * (img(x,y)-min) / (max - min);
    ++hist[idx];
  }
 
  // Normalize histogram (sum of all is 1)
  float sum = img.width() * img.height();
  
  std::vector<float> normHist(hist);
  
  for(std::vector<float>::iterator it = normHist.begin(); it != normHist.end(); ++it) {
    *it = *it / sum;
  }
 
  // Calculate accumulated histograms
  std::vector<float> accumulatedHistBlack(numBins, 0);
  std::vector<float> accumulatedHistWhite(numBins, 0);
 
  float accumHistSum = 0.0F;
  for (size_t idx = 0; idx < numBins; ++idx) {
    accumHistSum += normHist[idx];
 
    accumulatedHistBlack[idx] = accumHistSum;
    accumulatedHistWhite[idx] = 1.0F - accumHistSum;
  }
 
  // Find first index of element not 0 in black distribution
  size_t first_bin_idx = 0;
 
  for (size_t idx = 0; idx < numBins; ++idx) {
     if ( ! isAlmostEqual(accumulatedHistBlack[idx], 0.0F) ) {
       first_bin_idx = idx;
       break;
     }
  }
 
  // Find last index of element not 0 in white distribution
  size_t last_bin_idx = numBins;
 
  for (size_t idx = numBins - 1; idx >= first_bin_idx; --idx) {
     if ( ! isAlmostEqual(accumulatedHistWhite[idx], 0.0F) ) {
       last_bin_idx = idx;
       break;
     }
  }
 
  std::cout << "first_bin_idx: " << first_bin_idx << ", last_bin_idx: " << last_bin_idx << std::endl;
  
  float threshold = 0;
  float max_ent = 0;
  float ent_back;
  float ent_obj;
  float tot_ent;
  
  for (size_t idx = first_bin_idx; idx < last_bin_idx; ++idx) {
 
    /* Entropy of the background pixels */
   ent_back = 0.0;
   
   for ( size_t ih = 0; ih <= idx; ih++ ) {
     if ( ! isAlmostEqual(normHist[ih], 0.0F) ) {
	float c = normHist[ih] / accumulatedHistBlack[idx];
	ent_back -= c * std::log(c);
      }
    }
   
   ent_obj = 0.0;
 
   for ( size_t ih = idx + 1; ih < numBins; ih++ ) {
     if ( ! isAlmostEqual(normHist[ih], 0.0F) ) {
	float c = normHist[ih] / accumulatedHistWhite[idx];
	ent_obj -= c * std::log(c);
      }
    }
 
   /* Total entropy */
   tot_ent = ent_back + ent_obj;
 
   if ( max_ent < tot_ent ) {
     max_ent = tot_ent;
     threshold = idx;
    }
  }
 
  /**
   * IMPORTANT: The histogram was "shrunk" to 256 values, i.e. float
   * pixel value range was mapped to 256 brightness values.
   * This "shrinking" step needs to be reverted so that the calculated
   * threshold matches the original float image.
   */
  float th2 = min + (threshold / numBins) * (max - min);
  std::cout << "threshold: " << threshold << ", th2: " << th2 << std::endl;
  return th2;
} // calcMaxEntropyThreshold()

 
// https://www.lost-infinity.com/night-sky-image-processing-part-2-image-binarization-using-the-otsu-thresholding-algorithm/
//
//void thresholdOtsu(const CImg<float> & inImg, long inBitPix, CImg<float> * outBinImg) {
CImg<float> thresholdOtsu(const CImg<float> & inImg, long inBitPix, CImg<float> outBinImg) {
  CImg<> hist = inImg.get_histogram(pow(2.0, inBitPix));
 
  float sum = 0;
  cimg_forX(hist, pos) { sum += pos * hist[pos]; }
 
  float numPixels = inImg.width() * inImg.height();
  float sumB = 0, wB = 0, max = 0.0;
  float threshold1 = 0.0, threshold2 = 0.0;
  
  cimg_forX(hist, i) {
    wB += hist[i];
 
    if (! wB) { continue; }    
    float wF = numPixels - wB;
    if (! wF) { break; }
    
    sumB += i * hist[i];
 
    float mF = (sum - sumB) / wF;
    float mB = sumB / wB;
    float diff = mB - mF;
    float bw = wB * wF * pow(diff, 2.0);
    
    if (bw >= max) {
      threshold1 = i;
      if (bw > max) {
         threshold2 = i;
      }
      max = bw;            
    }
  } // end loop
  
  float th = (threshold1 + threshold2) / 2.0;

  outBinImg = inImg; // Create a copy
  outBinImg.threshold(th); 
  return outBinImg;
} // thresholdOtsu()


// https://www.lost-infinity.com/night-sky-image-processing-part-2-image-binarization-using-the-otsu-thresholding-algorithm/
//
CImg<float> thresholdMaxEntropy(const CImg<float> & img, long inBitPix) { //, CImg<float> * outBinImg) {
  cout << endl << "thresholdMaxEntropy() calculating binary image using max_entropy thresholding" << endl;
  // max_entropy thresholding
  //  
  float max_entropy_th = calcMaxEntropyThreshold(img); // img is input image
  // +1 because CImg threshold function somehow uses >=
  //static CImg<float> outBinImg = img;
  CImg<float> outBinImg = img.get_threshold(max_entropy_th + 1);
  cout << "thresholdMaxEntropy()" << endl << endl;
  return outBinImg;
} // thresholdMaxEntropy()  
  
  
// Removes all white neighbours arond pixel from whitePixels
// if they exist and adds them to pixelsToBeProcessed.
//
void getAndRemoveNeighbours(PixelPosT inCurPixelPos, PixelPosSetT * inoutWhitePixels, PixelPosListT * inoutPixelsToBeProcessed) {
  const size_t _numPixels = 8, _x = 0, _y = 1;
  const int offsets[_numPixels][2] = { { -1, -1 }, { 0, -1 }, { 1, -1 },
                                       { -1, 0 },              { 1, 0 },
                                       { -1, 1 }, { 0, 1 }, { 1, 1 } };
  
  for (size_t p = 0; p < _numPixels; ++p) {
    PixelPosT curPixPos(std::get<0>(inCurPixelPos) + offsets[p][_x], std::get<1>(inCurPixelPos) + offsets[p][_y]);
    PixelPosSetT::iterator itPixPos = inoutWhitePixels->find(curPixPos);
 
    if (itPixPos != inoutWhitePixels->end()) {
      const PixelPosT & curPixPos = *itPixPos;
      inoutPixelsToBeProcessed->push_back(curPixPos);
      inoutWhitePixels->erase(itPixPos); // Remove white pixel from "white set" since it has been now processed
    }
  }
  return;
} // getAndRemoveNeighbours()

template<typename T> void clusterStars(const CImg<T> & inImg, StarInfoListT * outStarInfos) {
  PixelPosSetT whitePixels;
 
  cimg_forXY(inImg, x, y) {
    if (inImg(x, y)) {
      whitePixels.insert(whitePixels.end(), PixelPosT(x, y));
    }
  }
 
  // Iterate over white pixels as long as set is not empty
  while (whitePixels.size()) {
    PixelPosListT pixelsToBeProcessed;
 
    PixelPosSetT::iterator itWhitePixPos = whitePixels.begin();
    pixelsToBeProcessed.push_back(*itWhitePixPos);
    whitePixels.erase(itWhitePixPos);

    FrameT frame(inImg.width(), inImg.height(), 0, 0);

    while(! pixelsToBeProcessed.empty()) {
      PixelPosT curPixelPos = pixelsToBeProcessed.front();

      // Determine boundaries (min max in x and y directions)
      if (std::get<0>(curPixelPos) /*x*/ < std::get<0>(frame) /*x1*/) {	std::get<0>(frame) = std::get<0>(curPixelPos); }
      if (std::get<0>(curPixelPos) /*x*/ > std::get<2>(frame) /*x2*/) { std::get<2>(frame) = std::get<0>(curPixelPos); }
      if (std::get<1>(curPixelPos) /*y*/ < std::get<1>(frame) /*y1*/) {	std::get<1>(frame) = std::get<1>(curPixelPos); }
      if (std::get<1>(curPixelPos) /*y*/ > std::get<3>(frame) /*y2*/) { std::get<3>(frame) = std::get<1>(curPixelPos); }

      getAndRemoveNeighbours(curPixelPos, & whitePixels, & pixelsToBeProcessed);
      pixelsToBeProcessed.pop_front();
    }
 
    // Create new star-info and set cluster-frame.
    // NOTE: we may use new to avoid copy of StarInfoT...
    StarInfoT starInfo;
    starInfo.clusterFrame = frame;
    outStarInfos->push_back(starInfo);
  }
} // clusterStars()


// 2D SNR calculation to find stars in an image without extracting the background noise
// https://www.lost-infinity.com/easy-2d-signal-to-noise-ratio-snr-calculation-for-images-to-find-potential-stars-without-extracting-the-background-noise/
//
double calc_snr(const CImg<float> & img) { //
  double varianceOfImage = img.variance(0);                // 0 = Calc. variance as "second moment"
  double estimatedVarianceOfNoise = img.variance_noise(0); // Uses "second moment" to compute noise variance 
  double q = varianceOfImage / estimatedVarianceOfNoise;
  // The simulated variance of the noise will be different from the noise in the real image.
  // Therefore it can happen that q becomes < 1. If that happens it should be limited to 1.
  double qClip = (q > 1 ? q : 1);
  double snr = std::sqrt(qClip - 1);
  return snr;
} // calc_snr()


float calcIx2(const CImg<float> & img, int x) {
  float Ix = 0;
  cimg_forY(img, y) { Ix += pow(img(x, y), 2.0) * (float) x; }
  return Ix;
} // calcIx2()
 
 
float calcJy2(const CImg<float> & img, int y) {
  float Iy = 0;
  cimg_forX(img, x) { Iy += pow(img(x, y), 2.0) * (float) y; }
  return Iy;
} // calcJy2()
 
 
// Calculate Intensity Weighted Center (IWC)
void calcIntensityWeightedCenter(const CImg<float> & inImg, float * outX, float * outY) {
  assert(outX && outY);
  
  // Determine weighted centroid - See http://cdn.intechopen.com/pdfs-wm/26716.pdf
  float Imean2 = 0, Jmean2 = 0, Ixy2 = 0;
  
  for(size_t i = 0; i < (size_t)inImg.width(); ++i) {
    Imean2 += calcIx2(inImg, i);
    cimg_forY(inImg, y) { Ixy2 += pow(inImg(i, y), 2.0); }
  }

  for(size_t i = 0; i < (size_t)inImg.height(); ++i) {
    Jmean2 += calcJy2(inImg, i);
  }
  
  *outX = Imean2 / Ixy2;
  *outY = Jmean2 / Ixy2;
} // calcIntensityWeightedCenter()
 
 
void calcSubPixelCenter(const CImg<float> & inImg, float * outX, float * outY, size_t inNumIter /* = 10 num iterations*/) {
  // Sub pixel interpolation
  float c, a1, a2, a3, a4, b1, b2, b3, b4;
  float a1n, a2n, a3n, a4n, b1n, b2n, b3n, b4n;
 
  assert(inImg.width() == 3 && inImg.height() == 3);
 
  b1 = inImg(0, 0); a2 = inImg(1, 0); b2 = inImg(2, 0);
  a1 = inImg(0, 1);  c = inImg(1, 1); a3 = inImg(2, 1);
  b4 = inImg(0, 2); a4 = inImg(1, 2); b3 = inImg(2, 2);
 
  for (size_t i = 0; i < inNumIter; ++i) {
    float c2 = 2 * c;
    float sp1 = (a1 + a2 + c2) / 4;
    float sp2 = (a2 + a3 + c2) / 4;
    float sp3 = (a3 + a4 + c2) / 4;
    float sp4 = (a4 + a1 + c2) / 4;
    
    // New maximum is center
    float newC = std::max({ sp1, sp2, sp3, sp4 });
    
    // Calc position of new center
    float ad = pow(2.0, -((float) i + 1));
 
    if (newC == sp1) {
      *outX = *outX - ad; // to the left
      *outY = *outY - ad; // to the top
 
      // Calculate new sub pixel values
      b1n = (a1 + a2 + 2 * b1) / 4;
      b2n = (c + b2 + 2 * a2) / 4;
      b3n = sp3;
      b4n = (b4 + c + 2 * a1) / 4;
      a1n = (b1n + c + 2 * a1) / 4;
      a2n = (b1n + c + 2 * a2) / 4;
      a3n = sp2;
      a4n = sp4;
 
    } 
    else if (newC == sp2) {
      *outX = *outX + ad; // to the right
      *outY = *outY - ad; // to the top
 
      // Calculate new sub pixel values
      b1n = (2 * a2 + b1 + c) / 4;
      b2n = (2 * b2 + a3 + a2) / 4;
      b3n = (2 * a3 + b3 + c) / 4;
      b4n = sp4;
      a1n = sp1;
      a2n = (b2n + c + 2 * a2) / 4;
      a3n = (b2n + c + 2 * a3) / 4;
      a4n = sp3;
    } 
    else if (newC == sp3) {
      *outX = *outX + ad; // to the right
      *outY = *outY + ad; // to the bottom
 
      // Calculate new sub pixel values
      b1n = sp1;
      b2n = (b2 + 2 * a3 + c) / 4;
      b3n = (2 * b3 + a3 + a4) / 4;
      b4n = (2 * a4 + b4 + c) / 4;
      a1n = sp4;
      a2n = sp2;
      a3n = (b3n + 2 * a3 + c) / 4;
      a4n = (b3n + 2 * a4 + c) / 4;
    } 
    else {
      *outX = *outX - ad; // to the left
      *outY = *outY + ad; // to the bottom  
 
      // Calculate new sub pixel values
      b1n = (2 * a1 + b1 + c) / 4;
      b2n = sp2;
      b3n = (c + b3 + 2 * a4) / 4;
      b4n = (2 * b4 + a1 + a4) / 4;
      a1n = (b4n + 2 * a1 + c) / 4;
      a2n = sp1;
      a3n = sp3;
      a4n = (b4n + 2 * a4 + c) / 4;
    }
 
    c = newC; // Oi = Oi+1
 
    a1 = a1n;
    a2 = a2n;
    a3 = a3n;
    a4 = a4n;
 
    b1 = b1n;
    b2 = b2n;
    b3 = b3n;
    b4 = b4n;
  }
} // calcSubPixelCenter()


void calcCentroid(const CImg<float> & inImg, const FrameT & inFrame, PixSubPosT * outPixelPos, PixSubPosT * outSubPixelPos /*= 0*/, size_t inNumIterations /*= 10*/) {
  // Get frame sub img
  CImg<float> subImg = inImg.get_crop(std::get<0>(inFrame), std::get<1>(inFrame), std::get<2>(inFrame), std::get<3>(inFrame));

  float & xc = std::get<0>(*outPixelPos);
  float & yc = std::get<1>(*outPixelPos);
  
  // 1. Calculate the IWC
  calcIntensityWeightedCenter(subImg, & xc, & yc);

  if (outSubPixelPos) {
    // 2. Round to nearest integer and then iteratively improve.
    int xi = floor(xc + 0.5);
    int yi = floor(yc + 0.5);
  
    CImg<float> img3x3 = inImg.get_crop(xi - 1 /*x0*/, yi - 1 /*y0*/, xi + 1 /*x1*/, yi + 1 /*y1*/);
    
    // 3. Interpolate using sub-pixel algorithm
    float xsc = xi, ysc = yi;
    calcSubPixelCenter(img3x3, & xsc, & ysc, inNumIterations);
    
    std::get<0>(*outSubPixelPos) = xsc;
    std::get<1>(*outSubPixelPos) = ysc;
  }
} // calcCentroid()
 
 
/**
* Expects star centered in the middle of the image (in x and y) and mean background subtracted from image.
*
* HDF calculation: http://www005.upp.so-net.ne.jp/k_miyash/occ02/halffluxdiameter/halffluxdiameter_en.html
*                  http://www.cyanogen.com/help/maximdl/Half-Flux.htm
*
* NOTE: Currently the accuracy is limited by the insideCircle function (-> sub-pixel accuracy).
* NOTE: The HFD is estimated in case there is no flux (HFD ~ sqrt(2) * inOuterDiameter / 2).
* NOTE: The outer diameter is usually a value which depends on the properties of the optical
*       system and also on the seeing conditions. The HFD value calculated depends on this
*       outer diameter value.
*/
float calcHfd(const CImg<float> & inImage, unsigned int inOuterDiameter) {
  // Sum up all pixel values in whole circle
  float outerRadius = inOuterDiameter / 2;
  float sum = 0, sumDist = 0;
  int centerX = ceil(inImage.width() / 2.0);
  int centerY = ceil(inImage.height() / 2.0);
 
  cimg_forXY(inImage, x, y) {
    if (insideCircle(x, y, centerX, centerY, outerRadius)) {
      sum += inImage(x, y);
      sumDist += inImage(x, y) * sqrt(pow((float) x - (float) centerX, 2.0f) + pow((float) y - (float) centerY, 2.0f));
    }
  }
  // NOTE: Multiplying with 2 is required since actually just the HFR is calculated above
  return (sum ? 2.0 * sumDist / sum : sqrt(2.0) * outerRadius);
} // calcHfd()
 
 
FrameT rectify(const FrameT & inFrame) {
  float border = 3;
  float border2 = 2.0 * border;
  float width = fabs(std::get<0>(inFrame) - std::get<2>(inFrame)) + border2;
  float height = fabs(std::get<1>(inFrame) - std::get<3>(inFrame)) + border2;
  float L = max(width, height);
  float x0 = std::get<0>(inFrame) - (fabs(width - L) / 2.0) - border;
  float y0 = std::get<1>(inFrame) - (fabs(height - L) / 2.0) - border;
  return FrameT(x0, y0, x0 + L, y0 + L);
}


// refactored Carsten Schmitt's code to separate signal processing from image I/O
//   - C++ pass by reference https://isocpp.org/wiki/faq/references
//
StarInfoListT st_pipeline(CImg<float> & img, long bitPix, int outerHfdDiameter) { 

  StarInfoListT starInfos;
  vector < list<StarInfoT *> > starBuckets;
  
  // Anisotropic Noise Filtering from CImg
  //
  // AD noise reduction --> In: Loaded image, Out: Noise reduced image
  // NOTE: This step takes a while for big images... too long for usage in a loop ->
  //       Should only be used on image segments, later...
  //
  // http://cimg.sourceforge.net/reference/structcimg__library_1_1CImg.html
  CImg<float> & aiImg = img.blur_anisotropic( 30.0f, /*amplitude*/
					                          0.7f, /*sharpness*/
					                          0.3f, /*anisotropy*/
					                          0.6f, /*alpha*/
					                          1.1f, /*sigma*/
					                          0.8f, /*dl*/
					                          30,   /*da*/
					                          2,    /*gauss_prec*/
					                          0,    /*interpolation_type*/
					                          false /*fast_approx*/
					                        );

  // Thresholding (Otsu) --> In: Noise reduced image, Out: binary image
  CImg<float> binImg;
  
  // select thresholding method for background removal
  //
  #if THRESHOLD_METHOD == OTSU
  cout << "calculating binary image using Otsu thresholding" << endl;
  //thresholdOtsu(aiImg, bitPix, & binImg);
  binImg = thresholdOtsu(aiImg, bitPix, & binImg);
  #else //#elif THRESHOLD_METHOD == MAX_ENTROPY
  cout << "calculating binary image using max_entropy thresholding" << endl;
  // max_entropy thresholding
  //  
  float max_entropy_th = calcMaxEntropyThreshold(img); // img is input image
  // +1 because CImg threshold function somehow uses >=
  binImg = img.get_threshold(max_entropy_th + 1);
  #endif
  
  // Clustering --> In: binary image from thresholding, Out: List of detected stars, subimg-boundaries (x1,y1,x2,y2) for each star
  clusterStars(binImg, & starInfos);

  cerr << "Recognized " << starInfos.size() << " stars..." << endl;

  // Calc brightness boundaries for possible focusing stars
  float maxPossiblePixValue = pow(2.0, bitPix) - 1;

  for (StarInfoListT::iterator it = starInfos.begin(); it != starInfos.end(); ++it) { // For each star
    const FrameT & frame = it->clusterFrame;
    FrameT & cogFrame = it->cogFrame;
    FrameT & hfdFrame = it->hfdFrame;
    PixSubPosT & CoG_Centr = it->CoG_Centr;
    PixSubPosT & subPixelInterpCentroid = it->subPixelInterpCentroid;
    float & hfd = it->hfd;
    float & fwhmHz = it->fwhmHz;
    float & fwhmVal = it->fwhmVal;
    float & maxPixVal = it->maxPixVal;
    bool & saturated = it->saturated;
    
    FrameT squareFrame = rectify(frame);
    
    // Centroid calculation --> In: Handle to full noise reduced image, subimg-boundaries (x1,y1,x2,y2), Out: (x,y) - abs. centroid coordinates
    calcCentroid(aiImg, squareFrame, & CoG_Centr, & subPixelInterpCentroid, 10 /* num iterations */);
    std::get<0>(CoG_Centr) += std::get<0>(squareFrame);
    std::get<1>(CoG_Centr) += std::get<1>(squareFrame);
    std::get<0>(subPixelInterpCentroid) += std::get<0>(squareFrame);
    std::get<1>(subPixelInterpCentroid) += std::get<1>(squareFrame);
    
    // Calculate cog boundaries
    float maxClusterEdge = std::max(fabs(std::get<0>(frame) - std::get<2>(frame)), fabs(std::get<1>(frame) - std::get<3>(frame)));
    float cogHalfEdge = ceil(maxClusterEdge / 2.0);
    float cogX = std::get<0>(CoG_Centr);
    float cogY = std::get<1>(CoG_Centr);
    std::get<0>(cogFrame) = cogX - cogHalfEdge - 1;
    std::get<1>(cogFrame) = cogY - cogHalfEdge - 1;
    std::get<2>(cogFrame) = cogX + cogHalfEdge + 1;
    std::get<3>(cogFrame) = cogY + cogHalfEdge + 1;

    // HFD calculation --> In: image, Out: HFD value
    // Subtract mean value from image which is required for HFD calculation
    size_t hfdRectDist = floor(outerHfdDiameter / 2.0);
    std::get<0>(hfdFrame) = cogX - hfdRectDist;
    std::get<1>(hfdFrame) = cogY - hfdRectDist;
    std::get<2>(hfdFrame) = cogX + hfdRectDist;
    std::get<3>(hfdFrame) = cogY + hfdRectDist;

    CImg<float> hfdSubImg = aiImg.get_crop(std::get<0>(hfdFrame), std::get<1>(hfdFrame), std::get<2>(hfdFrame), std::get<3>(hfdFrame));
    maxPixVal = hfdSubImg.max();
    //saturated = (maxPixVal > lowerBound && maxPixVal < upperBound);
    saturated = (maxPixVal == maxPossiblePixValue);
    
    CImg<float> imgHfdSubMean(hfdSubImg);
    double mean = hfdSubImg.mean();

    cimg_forXY(hfdSubImg, x, y) {
      imgHfdSubMean(x, y) = (hfdSubImg(x, y) < mean ? 0 : hfdSubImg(x, y) - mean);
    }
 
    // Calc the HFD
    hfd = calcHfd(imgHfdSubMean, outerHfdDiameter /*outer diameter in px*/);
    
    // FWHM calculation --> In: Handle to full noise reduced image, abs. centroid coordinates, Out: FWHM value
    MyDataContainerT ValDataPoints, HzDataPoints;

    cimg_forX(imgHfdSubMean, x) {
      HzDataPoints.push_back(make_pair(x, imgHfdSubMean(x, floor(imgHfdSubMean.height() / 2.0 + 0.5))));
    }
    
    cimg_forY(imgHfdSubMean, y) {
      ValDataPoints.push_back(make_pair(y, imgHfdSubMean(floor(imgHfdSubMean.width() / 2.0 + 0.5), y)));
    }    
    
    // Do the LM fit
    typedef CurveFitTmplT<GaussianFitTraitsT> GaussMatcherT;
    typedef GaussMatcherT::CurveParamsT CurveParamsT;
    CurveParamsT::TypeT gaussCurveParmsHz, gaussCurveParmsVal;
    
    GaussMatcherT::fitGslLevenbergMarquart<MyDataAccessorT>(HzDataPoints, & gaussCurveParmsHz, 0.1f /*EpsAbs*/, 0.1f /*EpsRel*/);
    fwhmHz = gaussCurveParmsHz[CurveParamsT::W_IDX];
    
    GaussMatcherT::fitGslLevenbergMarquart<MyDataAccessorT>(ValDataPoints, & gaussCurveParmsVal, 0.1f /*EpsAbs*/, 0.1f /*EpsRel*/);
    fwhmVal = gaussCurveParmsVal[CurveParamsT::W_IDX];
  }

  return starInfos;
} // st_pipeline()


// renders composite output image with extracted parameters superimposed on input
//
int render_output(char *fitsname, CImg<float> & img, StarInfoListT starInfos, int outerHfdDiameter) {
	
  // Create RGB image from fits file to paint boundaries and centroids (just for visualization)
  //
  CImg<unsigned char> rgbImg(img.width(), img.height(), 1 /*depth*/, 3 /*3 channels - RGB*/);
  float min = img.min(), mm = img.max() - min;
  
  cimg_forXY(img, x, y) {
    int value = 255.0 * (img(x,y) - min) / mm;
    rgbImg(x, y, 0 /*red*/) = value;
    rgbImg(x, y, 1 /*green*/) = value;
    rgbImg(x, y, 2 /*blue*/) = value;
  }
    
  // Create result image
  const int factor = 4; // upscaling factor from input image
  CImg<unsigned char> & rgbResized = rgbImg.resize(factor * rgbImg.width(), factor * rgbImg.height(), -100 /*size_z*/, -100 /*size_c*/, 1 /*interpolation_type*/);

  // Draw cluster boundaries and square cluster boundaries
  const unsigned char red[3] = { 255, 0, 0 }, green[3] = { 0, 255, 0 }, yellow[3] = { 255, 255, 0 };
  const unsigned char  black[3] = { 0, 0, 0 }, blue[3] = { 0, 0, 255 }, white[3] = { 255, 255, 255 };
  const size_t cCrossSize = 3;
  
  // Mark all stars in RGB image
  unsigned cent=0;
  for (StarInfoListT::iterator it = starInfos.begin(); it != starInfos.end(); ++it) {
    StarInfoT * curStarInfo = & (*it);
    //PixSubPosT & CoG_Centr = curStarInfo->CoG_Centr; // unused variable
    float & hfd = curStarInfo->hfd;
    float & fwhmHz = curStarInfo->fwhmHz;
    float & fwhmVal = curStarInfo->fwhmVal;
    float & maxPixVal = curStarInfo->maxPixVal;
    
    cerr << cent << " CoG_Centr=(" << setw(9) << std::get<0>(curStarInfo->CoG_Centr)
	 << ", " << setw(9) << std::get<1>(curStarInfo->CoG_Centr)
         <<  "), " << setw(8) << "maxPixVal: " << setw(8) << maxPixVal
         << ", sat: " << curStarInfo->saturated << ", hfd: " << setw(8) << hfd
	 << ", fwhm(Hz,Val): " << setw(8) << fwhmHz << "," << setw(8) << fwhmVal 
	 << endl;
    
    cent++;
    const FrameT & frame = curStarInfo->clusterFrame;
    FrameT squareFrame(rectify(frame));
    rgbResized.draw_rectangle(floor(factor * (std::get<0>(frame) - 1) + 0.5), floor(factor * (std::get<1>(frame) - 1) + 0.5),
			      floor(factor * (std::get<2>(frame) + 1) + 0.5), floor(factor * (std::get<3>(frame) + 1) + 0.5),
			      red, 1 /*opacity*/, ~0 /*pattern*/);
	
    rgbResized.draw_rectangle(floor(factor * (std::get<0>(squareFrame) - 1) + 0.5), floor(factor * (std::get<1>(squareFrame) - 1) + 0.5),
			      floor(factor * (std::get<2>(squareFrame) + 1) + 0.5), floor(factor * (std::get<3>(squareFrame) + 1) + 0.5),
			      blue, 1 /*opacity*/, ~0 /*pattern*/);
	
	
    // Draw centroid crosses and centroid boundaries
    const PixSubPosT & subPos = curStarInfo->CoG_Centr;
    const FrameT & cogFrame = curStarInfo->cogFrame;
    const FrameT & hfdFrame = curStarInfo->hfdFrame;
	
    rgbResized.draw_line(floor(factor * (std::get<0>(subPos) - cCrossSize) + 0.5), floor(factor * std::get<1>(subPos) + 0.5),
			 floor(factor * (std::get<0>(subPos) + cCrossSize) + 0.5), floor(factor * std::get<1>(subPos) + 0.5), green, 1 /*opacity*/);
	
    rgbResized.draw_line(floor(factor * std::get<0>(subPos) + 0.5), floor(factor * (std::get<1>(subPos) - cCrossSize) + 0.5),
			 floor(factor * std::get<0>(subPos) + 0.5), floor(factor * (std::get<1>(subPos) + cCrossSize) + 0.5), green, 1 /*opacity*/);
	
    rgbResized.draw_rectangle(floor(factor * std::get<0>(cogFrame) + 0.5), floor(factor * std::get<1>(cogFrame) + 0.5),
			      floor(factor * std::get<2>(cogFrame) + 0.5), floor(factor * std::get<3>(cogFrame) + 0.5),
			      green, 1 /*opacity*/, ~0 /*pattern*/);
	
    // Draw HFD
    rgbResized.draw_rectangle(floor(factor * std::get<0>(hfdFrame) + 0.5), floor(factor * std::get<1>(hfdFrame) + 0.5),
			      floor(factor * std::get<2>(hfdFrame) + 0.5), floor(factor * std::get<3>(hfdFrame) + 0.5),
			      yellow, 1 /*opacity*/, ~0 /*pattern*/);
	
    rgbImg.draw_circle(floor(factor * std::get<0>(subPos) + 0.5), floor(factor * std::get<1>(subPos) + 0.5), factor * outerHfdDiameter / 2, yellow, 1 /*pattern*/, 1 /*opacity*/);
    rgbImg.draw_circle(floor(factor * std::get<0>(subPos) + 0.5), floor(factor * std::get<1>(subPos) + 0.5), factor * hfd / 2, yellow, 1 /*pattern*/, 1 /*opacity*/);
	
    // Draw text
    const bool & saturated = curStarInfo->saturated;
	
    ostringstream oss;
    oss.precision(4);
    oss	<< "HFD=" << hfd << endl
	<< "FWHM H=" << fwhmHz << endl
	<< "FWHM V=" << fwhmVal << endl
	<< "MAX=" << (int)maxPixVal << endl
	<< "SAT=" << (saturated ? "Y" : "N");
	
	// print parameters at centroid coordinates in output image
	//
    rgbImg.draw_text(floor(factor * std::get<0>(subPos) + 0.5), floor(factor * std::get<1>(subPos) + 0.5), oss.str().c_str(), white /* text color*/, black /*bg color*/, 0.7 /*opacity*/, 9 /*font-size*/);
  }
  
  // print out high-level star-tracker parameters in corner of image
  //
  //   - fits filename
  //   - date
  //   - stars detected
  //   - etc.
  //
  ostringstream oss;
  oss << "Star-Tracker Statistics" << endl  << endl << "fits- input file =" << fitsname << endl << "stars detected =" << cent << endl;
  rgbImg.draw_text(floor(factor * 1 + 0.5), floor(factor * 1 + 0.5), oss.str().c_str(), red /* text color*/, black /*bg color*/, 0.7 /*opacity*/, 30 /*font-size*/);
  
  rgbResized.save("startracker_output.jpeg");
 
  return 0;
	
} // render_output


// program performs the following steps:
//
//   1. reads input fits image
//   2. performs star-tracker DSP on input image
//   3. renders composite output image with extracted parameters superimposed on input
// 
int run_startracker(char *fitsname) {

  const unsigned int outerHfdDiameter = 21; // outerHfdDiameter depends on pixel size and focal length (and seeing...) - this is a best guess
  CImg<float> img;                          // input image for analysis
  StarInfoListT starInfos;                  // star-tracker pipeline output: centroids & other parameters
  long bitPix = 0;                          //
  
  // Read file to CImg
  try {
    //cerr << "Opening file " << argv[1] << endl;
    printf("Opening file %s\n)", fitsname);
    //readFile(img, argv[1], & bitPix);
    readFile(img, fitsname, & bitPix);
  } 
  catch (FitsException &) {
    printf("Read FITS failed.");
	//cerr << "Read FITS failed." << endl;
    return 1;
  }

  // do processing
  //
  cout << "st_pipeline" << endl;
  starInfos = st_pipeline(img, bitPix, outerHfdDiameter);    // perform pipeline signal processing on input image img
  cout << "render_output" << endl;
  render_output(fitsname, img, starInfos, outerHfdDiameter); // render output image with recovered centroids & parameters
  
  return 0;
  
} // run_startracker()


// star_tracker.cpp


from pycimg import CImg
import numpy as np
from astropy.io import fits
import math  

import matplotlib

# in Windows only
matplotlib.use('TkAgg') # install sudo apt-get install python3-tk otherwise no graphics with $DISPLAY ang Xming
                        # UserWarning: Matplotlib is currently using agg, which is a non-GUI backend, so cannot show the figure.

import matplotlib.pyplot as plt

# python3 doesn't have builtin 
def history() :
	import readline
	for i in range(readline.get_current_history_length()):
		print (readline.get_history_item(i + 1))
	
# 2D SNR calculation https://www.lost-infinity.com/easy-2d-signal-to-noise-ratio-snr-calculation-for-images-to-find-potential-stars-without-extracting-the-background-noise/
#
def snr_2D(img) :
	varianceOfImage = img.variance(0)                # 0 = Calc. variance as "second moment"
	estimatedVarianceOfNoise = img.variance_noise(0) # Uses "second moment" to compute noise variance 
	q = varianceOfImage / estimatedVarianceOfNoise
	# The simulated variance of the noise will be different from the noise in the real image.
	# Therefore it can happen that q becomes < 1. If that happens it should be limited to 1.
	if (q > 1) :
		qClip = q 
	else :
		qClip = 1
	snr = math.sqrt(qClip - 1)
	return snr

# plot image
#
def dplot(img) :
	plt.figure(1)
	plt.imshow(img, cmap='gray')
	plt.colorbar()
	plt.show()
    
def splot(im) : # surface plot of starfield pixels
    import numpy as np
    import matplotlib.pyplot as plt
    from mpl_toolkits.mplot3d import Axes3D

    # create the x and y coordinate arrays
    xx, yy = np.mgrid[0:im.shape[0], 0:im.shape[1]]

    # create the figure
    fig = plt.figure()
    ax = fig.gca(projection='3d')
    ax.plot_surface(xx, yy, im ,rstride=1, cstride=1, cmap=plt.cm.gray, linewidth=0)
    plt.show()

# plot histogram of image http://learn.astropy.org/FITS-images.html
#                         https://matplotlib.org/tutorials/introductory/images.html
#
def hplot(img, NBINS = 1000) :
	plt.figure(2) 
	plt.hist(img.flatten(), bins=NBINS, range=(np.amin(img),np.amax(img)))	#h = plt.hist(img.ravel(), bins=NBINS, range=(0.0, 1.0), fc='k', ec='k')
	plt.show()

########################################################################
# import fits image of star-field
#	
image_file = 'test.fits'
#fits.info(image_file)
im = fits.getdata(image_file, ext=0) # getdata returns 2D numpy array
img = CImg(im.astype(float)) # im is integer and pipeline requires floats

########################################################################
# import star-tracker library
#
import pb11_startracker as pst 
dir(pst)
help(pst)

########################################################################
# complete star-tracker
#
pst.tracker(image_file)

########################################################################
# star-tracker pipeline
#
starz = pst.np_pipeline(im)
starz
starz[0].clusterFrame
starz[1].cogFrame
starz[2].hfdFrame
starz[3].CoG_Centr
starz[4].subPixelInterpCentroid

pst.np_startracker(im)

########################################################################
# calculate SNR
#
#help(pst.np_snr)
#
no_star = fits.getdata('no_star.fits', ext=0)
#dplot(no_star)
no_2Dsnr = snr_2D(CImg(no_star)) # python reference code
no_2Dsnr
pst.np_snr(no_star) # wrapped C++ code

wk_star = fits.getdata('weak_star.fits', ext=0)
#dplot(wk_star)
wk_2Dsnr = snr_2D(CImg(wk_star))
wk_2Dsnr
pst.np_snr(wk_star) # wrapped C++ code

hc_star = fits.getdata('high_contrast_star.fits', ext=0)
#dplot(hc_star)
hc_2Dsnr = snr_2D(CImg(hc_star))
hc_2Dsnr
pst.np_snr(hc_star) # wrapped C++ code

# access StarInfoT structure data
#
si = pst.StarInfoT()
si
si.hfd = 0.5
si.hfd
si.fwhmHz = 12.5
si.fwhmHz
si.fwhmVal = 3.5
si.fwhmVal
si.saturated = 9.5
si.saturated
si
print(si) # __repr__

imf=im.astype('float32')
ii=pst.CImg_float(imf)
ii
ii.width()
ii.height()
ii.shape()
ii.depth()
ii.spectrum()
ii.data()
ii.ptr()
print(ii)

#####################################################################################
# star-tracker numpy interface
#####################################################################################
#
bitPix = 0; th_method = 0
snc = pst.np_calc_snr(ii);                     snc 
nri = pst.np_anisotropic_nr(im,bitPix);        nri # input noise reduction
snn = pst.np_calc_snr(nri);                    snn 
met = pst.np_calcMaxEntropyThreshold(nri);     met # Max-entropy threshold calculation
tim = pst.np_thresholdMaxEntropy(nri, bitPix); tim # thresholding Max-entropy
tio = pst.np_thresholdOtsu(nri, bitPix);       tio # thresholding Otsu
if (th_method==0): thr = tim                       # threshold selection mux
else:              thr = tio
cct = pst.np_clusterStars(thr);                cct # cluster stars

# std::tuple<PixSubPosT, PixSubPosT> np_calcCentroid(py::array_t<float>& img_i, const FrameT & inFrame, PixSubPosT outPixelPos, PixSubPosT outSubPixelPos, size_t inNumIterations) {
inNumIterations = 10
# typedef tuple<float,float,float,float> FrameT;        // <x1, y1, x2, y2>
(outPixelPos, outSubPixelPos) = np_calcCentroid(cct, const FrameT & inFrame, outPixelPos, outSubPixelPos, inNumIterations);                   nct # centroid star clusters
# pst.np_calcHfd();

#####################################################################################
# star-tracker CImg interface
#####################################################################################
#
bitPix = 0
cet = pst.calcMaxEntropyThreshold(ii);     cet
cim = pst.thresholdMaxEntropy(ii, bitPix); cim

#
amplitude          = 30.0
sharpness          = 0.7  
anisotropy         = 0.3  
alpha              = 0.6
sigma              = 1.1  
dl                 = 0.8 
da                 = 30 
gauss_prec         = 2
interpolation_type = 0 
fast_approx        = 0  

nrc = ii.blur_anisotropic(amplitude, sharpness, anisotropy, alpha, sigma, dl, da, gauss_prec, interpolation_type, fast_approx)
nrc = ii.blur_anisotropic() # use defaults

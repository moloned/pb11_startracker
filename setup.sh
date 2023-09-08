#!/bin/bash
#
# References
#
#https://guaix.fis.ucm.es/~ncl/howto/howto-cfitsio
#https://public.lanl.gov/mkippen/gress/doc/GRESS_InstallGuide.txt
#
# David Moloney PhD, Chief Scientist
# Ubotica Ltd., Innovation Campus, 11 Old Finglas Rd, Glasnevin
# Dublin D11 KXN4, Ireland 

sudo -s

# cfitsio installation
#
mkdir /usr/local/src
cd /usr/local/src
git clone https://github.com/healpy/cfitsio.git
#
# Configure to generate the make file:
# 
cd cfitsio
./configure --prefix=/usr
#
# the option --prefix=/usr controls where the library will be installed
#
make
make install
make clean
#
# To test the installation is working:
# 
make testprog
./testprog > testprog.lis
diff testprog.lis testprog.out
cmp testprog.fit testprog.std
#
# You can also try the following
#
make speed
./speed
make cookbook
./cookbook
#
# The different versions of the CFITSIO library (libcfitsio.*) are installed under: /usr/lib. The auxiliary files longnam.h, fitsio.h, fitsio2.h, and drvrsmem.h are placed under: /usr/include. The pkg-config file is available at /usr/lib/pkgconfig/cfitsio.pc.
#
# CCfits install
cd /usr/local/src
git clone https://github.com/esrf-bliss/CCfits.git 
# Automake (required for CCfits):
#
sudo apt-get update
apt-get install automake
sudo apt-get install libtool
#
# Configure CCfits
# 
cd /usr/local/src/CCfits/
./configure --with-cfitsio=/usr
export echo=echo
export CXX=g++
#
# Make CCfits:
# 
# To avoid libtool bug execute this line before making https://stackoverflow.com/questions/12146950/x-i-command-not-found-and-failed-to-build-certain-kind-of-software-whats-wr/30477516#30477516
# 
make 
make DESTDIR=/usr/local/CCfits install
#
# install doesn't work properly so need to copy library files explicitly
#
cp /usr/local/src/CCfits/.libs/libCCfits.* /usr/local/lib/
#
#GSL install: 
#
sudo apt-get install gsl-bin
sudo apt-get install libgsl0-dev
# 
# install CImg
# 
cd /usr/local/src
git clone https://github.com/dtschump/CImg.git
cd CImg/examples/
make mlinux
#
# https://gist.github.com/ozooxo/299ab37080bba24b8c0e
#
sudo apt-get install libx11-dev
sudo apt-get install cimg-dev cimg-doc cimg-examples
#
# Install X11 (Ubuntu)
# 
apt install libx11-dev
apt install libgl1-mesa-dev
apt install xorg-dev
apt install libgraphicsmagick1-dev
apt install python3-opencv
#
# Build Star-Recogniser Application https://www.lost-infinity.com/night-sky-image-processing-part-7-automatic-star-recognizer/
#
g++ star_recognizer.cpp -o star_recognizer -lX11 -lpthread -lCCfits -lgsl -lgslcblas -std=c++0x
# 
# Setting up and using X11
# 
# https://virtualizationreview.com/articles/2017/02/08/graphical-programs-on-windows-subsystem-on-linux.aspx
# https://www.youtube.com/watch?v=4SZXbl9KVsw
#
apt-get remove  openssh-server
apt-get install  openssh-server
service ssh --full-restart
# 
# Test X11 (run xeyes)
# 
apt-get install x11-apps
sudo apt-get install libx11-dev
# 
# Launch Xming on Win10 desktop
#
# https://stackoverflow.com/questions/61860208/wsl-2-run-graphical-linux-desktop-applications-from-windows-10-bash-shell-erro
#
# set up security to allow vcXsrv to access network and use -ac option
# https://techcommunity.microsoft.com/t5/windows-dev-appconsult/running-wsl-gui-apps-on-windows-10/ba-p/1493242
#
export DISPLAY="`grep nameserver /etc/resolv.conf | sed 's/nameserver //'`:0"
echo $DISPLAY
xeyes &
#

sudo apt-get install cimg-dev cimg-doc cimg-examples
#
# Boost
#
sudo apt-get update -y
sudo apt-get install libboost-all-dev#
# pybind11 install
#
pip3 install pybind11
cd ~david/Desktop/MyGithub/pb11_startracker
#
# need these for the star-tracker
#
pip3 install pycimg
pip3 install numpy
pip3 install astropy
pip3 install matplotlib
sudo apt-get install python3-tk


#sudo apt-get install -y libccfits-dev
#sudo apt-get install -y libccfits-dev
#sudo apt-get install -y libcfitsio-dev

#
# make and run max_entropy
#
g++ max_entropy.cpp -o max_entropy -lCCfits -lcfitsio -lpthread -L /usr/local/lib -L /usr/local/src/CCfits/ -L /usr/local/src/CCfits/.libs/
./max_entropy weak_star.fits
./max_entropy no_star.fits
./max_entropy high_contrast_star.fits




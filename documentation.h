/**

@mainpage

\section Introduction

This program created a corner detector by optimizing a decision tree using simulated annealing.
The tree is optimized to maximize the repeatability of the corner detector.


To make on any unix-like environment, do:

<code>./configure && make</code>

There is no install option.

This will create the following executables:
 - <code>\link learn_detector.cc learn_detector\endlink</code> This learns a detector from a repeatability dataset.
 - <code>\link warp_to_png.cc warp_to_png\endlink</code> This converts a repeatability dataset in to a rather faster loading format.


 
\section learn_detector

To set the parameters, examine <code>learn_detector.cfg</code>.
In order to run this program, you will need a repeatability dataset, such as the one from
http://mi.eng.cam.ac.uk/~er258/work/datasets.html or

The running program will generate a very extensive logfile on the standard
output, which will include the learned detector and the results of its
repeatability evaluation. Running <code>get_block_detector</code> on the output
will generate source code for the detector. Note that this source code will not
yet have been optimized for speed, only repeatability.

The complete sequence of operations is as follows
<ol>
	<li> Make the executable:

		<code> ./configure && make </code>

	<li> Set up <code>learn_detector.cfg</code>. The default parameters are good,
		 except you will need to set up <code>datadir</code> to point to the
		 repeatability dataset.

	<li> Run the corner detector learning program

		<code>./learn_detector > logfile</code>

		If you run it more than once, you will probably want to alter the random
		seed in the configuration file.

	<li> Extract a detector from the logfile

	     <code>./get_block_detector logfile &gt; learned-faster-detector.h</code>

	<li> make <code>extract-fast2.cxx</code>
		
		Note that if you have changed the list of available offsets in
		<code>learn_detector.cfg</code> then you will need to change
		<code>offset_list</code> in <code>extract-fast2.cxx</code>. The list of
		offsets will be in the logfile.

	<li> Use <code>extract-fast2.cxx</code> to extract features from a set of
	     training images. Any reasonable images can be used, including the 
		 training images used earlier.  Do:

		 <code>./extract-fast2</code><i>imagefile  imagefile2 ...</i><code>&gt; features</code>
</ol>

*/

/**
@defgroup gRepeatability Measuring the repeatability of a detector

Functions to load a repeatability dataset, and compute the repeatability of
a list of detected points.


\section data The dataset

The dataset consists of a number of registered images. The images are stored in
<code>frames/frame_X.pgm</code> where X is an integer counting from zero.  The
frames must all be the same size. The warps are stored in
<code>waprs/warp_Y_Z.warp</code>. The file <code>warp_Y_Z.warp</code> contains
one line for every pixel in image Y (pixels arranged in raster-scan order). The
line is the position that the pixel warps to in image Z. If location of -1, -1
indicates that this pixel does not appear in image Z.

*/

/**       
@defgroup  gDataset Repeatability dataset

To compute repeatability, you must know for every pixel in image <i>A</i>, where
that pixel ends up in image <i>B</i>. The datasets are stored internally as:
- Images are simply stored internally as: <code> vector<Image<byte> > </code>
- Mappings from <i>A</i> to <i>B</i> are stored as: <code> vector<vector<array<float, 2> > > </code>
  so <code>mapping[i][j][y][x]</code>, is where pixel \f$(x, y)\f$ in image <i>i</i>  should appear in image <i>j</i>


These datasets can be stored in disk in several formats. However, they are
loaded by a single function, ::load_data(string, int, string) In all datasets, all images must
be the same size.

\section camDataset Cambridge dataset format.

The consists of <i>N</i> images, and an arbitrary warp for each image pair. From
some base directory, the files are stored as:
	- frames/frame_<i>x</i>.pgm
	- warps/warp_<i>i</i>_<i>j</i>.warp
The warp files have the mapping positions stored in row-major format, one pixel
per line, stored as a pair of real numbers in text format. Details are in
::load_warps_cambridge() and ::load_images_cambridge(). The indices, <i>x</i>,
<i>i</i> and <i>j</i> count from zero.

\subsection canPNG Cambridge PNG dataset.

This stores the warp data in 16 bit per channel (with a numeric range of
0--65535), colour PNG format:
	- frames/frame_<i>x</i>.pgm
	- pngwarps/warp_<i>i</i>_<i>j</i>.png
The destination of the <i>x</i> coordinare is stored as \f$x =
\frac{\text{red}}{\text{MULTIPLIER}} - \text{SHIFT}\f$, and the <i>y</i> destination as \f$y =
\frac{\text{green}}{\text{MULTIPLIER}} - \text{SHIFT}\f$. ::MULTIPLIER is 64.0 and ::SHIFT is 10.0 The
blue channel stores nothing. Details are in ::load_warps_cambridge_png().

The executable warp_to_png.cc converts a <code>.warp</code> file to a
<code>.png</code> file.

\section oxDataset Oxford VGG dataset format.

The datasets consist on <i>N</i> images and <i>N-1</i> Homographies describing
the warps between images the first and <i>N</i><sup>th</sup> image. The first
homography is therefore the identity matrix.

From a base directory, the files are:
- H1to<i>i</i>p
- img<i>i</i>.ppm

where the index <i>i</i> counts from 1. More details are in ::load_warps_cambridge_png() and ::load_images_vgg().

*/

/**
@defgroup  gTree Tree representation.

*/

/**
@defgroup  gFastTree Compiled tree representations

*/

/**
@defgroup  gUtility Utility functions.

*/

/**
@defgroup  gOptimize Optimization routines

The functions in this section deal specifically with optimizing a decision tree
detector for repeatability. The code in ::learn_detector() is a direct
implementation of the algorithm described in section V of the accompanying
paper.

The manipulation of the tree is necessarily tied to the internal representation
which is described in \link gTree the tree representation\endlink.

*/

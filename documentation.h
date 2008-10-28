/**

@mainpage

\section Introduction

This code contains a suite of pograms to generate an optimized corner detector,
test corner detectors on a variety of datasets, generate optimized C++ code, and
some utilities for handling the datasets.

To make on any unix-like environment, do:

<code>./configure && make</code>

There is no install option.

This will create the following executables:
 - <code>\link learn_detector.cc learn_detector\endlink</code> This learns a detector from a repeatability dataset.
 - \p extract_features This extracts features from an image sequence which can be turned in to a decision tree.
 - \link learn_fast_tree.cc \p learn_fast_tree \endlink This learns a FAST decision tree, from extracted data.
 - Programs for generating code from the learned tree, in various language/library combinations.
   - C++ / libCVD
       - \p fast_tree_to_cxx_score_bsearch
       - \p fast_tree_to_cxx_score_iterate
       - \p fast_tree_to_cxx
   - MATLAB
       - \p FIXME
 - <code>\link test_repeatability.cc test_repeatability\endlink</code> Measure the repeatability of a detector.
 - <code>\link warp_to_png.cc warp_to_png\endlink</code> This converts a repeatability dataset in to a rather faster loading format.
 - <code>\link image_warp.cc image_warp\endlink</code> This program allows visual inspection of the quality of a dataset.
 - <code>\link fast_N_features.cc fast_N_features\endlink</code> This program generates all possible FAST-N features for consumption by  \link learn_fast_tree.cc \p learn_fast_tree \endlink.

\section sRequirements Requirements

This code requires the following libraries from http://mi.eng.cam.ac.uk/~er258/cvd
- libCVD (compiled with TooN and LAPACK support)
- TooN
- GVars3
- tag
For repeatability testing, the SUSAN detector can be used if the reference implementation
is downloaded and placed in the directory. It is abailable from http://users.fmrib.ox.ac.uk/~steve/susan/susan2l.c

\section learn_detector Running the system

The complete sequence of operations for FAST-ER is as follows:
<ol>
    <li> Make the executable:

        <code> ./configure && make </code>

    <li> Generating a new FAST-ER detector.

        An example detector (the best known detector, used in the results
        section of the paper) is already in \p best_faster.tree .

    <ol>

        <li> Set up <code>learn_detector.cfg</code>. The default parameters 
             are good, except you will need to set up the system to point to the
             \link gRepeatability repeatability dataset\endlink you wish to use.

        <li> Run the corner detector learning program

            <code>./learn_detector > logfile</code>

            If you run it more than once, you will probably want to alter the
            random seed in the configuration file.
            

        <li> Extract a detector from the logfile

            <code>awk 'a&&!NF{exit}a;/Final tree/{a=1}' logfile &gt; new_detector.tree</code>

    </ol>

    <li> Measuring the repeatability of a detector
        
        <code>./test_repeatability --detector faster2 --faster2 new_detector.tree > new_detector_repeatability.txt</code>

        The file <code>new_detector_repeatability.txt</code> can be plotted with almost any graph
        plotting program. A variety of detectors can be tested using this
        program. See \link test_repeatability.cc test_repeatability\endlink for
        more information.

    <li> Generating accelerated tree based detectors.
    
    <ol>
        <li> Features can be generated (for instance for FAST-N) or extracted
             from images, as is necessary for FAST-ER. FAST-N features can be
             extracted using \link fast_N_features.cc fast_N_features\endlink:

             <code>
                ./fast_N_features --N 9 &gt; features.txt
             </code>

             Alternatively, they can be extracted from images using
             \link extract_features.cc extract_features\endlink:

             <code>
                ./extract_features --detector fast-er --fast-er new_detector.tree FILE1 [FILE2 ...] &gt; features.txt
             </code>

            

        <li> A decision tree can be learned from the features using
             \link learn_fast_tree.cc learn_fast_tree\endlink:

             <code>
             learn_fast_tree < features.txt > fast-tree.txt
             </code>

        <li> The decision tree needs to be turned in to source code before it
             can be easily used.

    </ol>
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
@defgroup gDetect Functions for detecting corners of various types.
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

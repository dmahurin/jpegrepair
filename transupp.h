/*
 * transupp.h
 *
 * Copyright (C) 1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README.ijg
 * file.
 *
 * This file contains declarations for image transformation routines and
 * other utility code used by the jpegtran sample application.  These are
 * NOT part of the core JPEG library.  But we keep these routines separate
 * from jpegtran.c to ease the task of maintaining jpegtran-like programs
 * that have other user interfaces.
 *
 * NOTE: all the routines declared here have very specific requirements
 * about when they are to be executed during the reading and writing of the
 * source and destination files.  See the comments in transupp.c, or see
 * jpegtran.c for an example of correct usage.
 */

/*
 * Support for copying optional markers from source to destination file.
 */

typedef enum {
  JCOPYOPT_NONE,          /* copy no optional markers */
  JCOPYOPT_COMMENTS,      /* copy only comment (COM) markers */
  JCOPYOPT_ALL            /* copy all optional markers */
} JCOPY_OPTION;

/* Setup decompression object to save desired markers in memory */
EXTERN(void) jcopy_markers_setup
        (j_decompress_ptr srcinfo, JCOPY_OPTION option);
/* Copy markers saved in the given source object to the destination object */
EXTERN(void) jcopy_markers_execute
        (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
         JCOPY_OPTION option);

/*
 * transupp.c
 *
 * Copyright (C) 1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README.igj
 * file.
 *
 * This file contains image transformation routines and other utility code
 * used by the jpegtran sample application.  These are NOT part of the core
 * JPEG library.  But we keep these routines separate from jpegtran.c to
 * ease the task of maintaining jpegtran-like programs that have other user
 * interfaces.
 */

#include <stddef.h>
#include <stdio.h>
#include "jpeglib.h"
#include "transupp.h"           /* My own external interface */


/* Setup decompression object to save desired markers in memory.
 * This must be called before jpeg_read_header() to have the desired effect.
 */

GLOBAL(void)
jcopy_markers_setup (j_decompress_ptr srcinfo, JCOPY_OPTION option)
{
  int m;

  /* Save comments except under NONE option */
  if (option != JCOPYOPT_NONE) {
    jpeg_save_markers(srcinfo, JPEG_COM, 0xFFFF);
  }
  /* Save all types of APPn markers iff ALL option */
  if (option == JCOPYOPT_ALL) {
    for (m = 0; m < 16; m++)
      jpeg_save_markers(srcinfo, JPEG_APP0 + m, 0xFFFF);
  }
}

/* Copy markers saved in the given source object to the destination object.
 * This should be called just after jpeg_start_compress() or
 * jpeg_write_coefficients().
 * Note that those routines will have written the SOI, and also the
 * JFIF APP0 or Adobe APP14 markers if selected.
 */

GLOBAL(void)
jcopy_markers_execute (j_decompress_ptr srcinfo, j_compress_ptr dstinfo,
                       JCOPY_OPTION option)
{
  jpeg_saved_marker_ptr marker;

  /* In the current implementation, we don't actually need to examine the
   * option flag here; we just copy everything that got saved.
   * But to avoid confusion, we do not output JFIF and Adobe APP14 markers
   * if the encoder library already wrote one.
   */
  for (marker = srcinfo->marker_list; marker != NULL; marker = marker->next) {
    if (dstinfo->write_JFIF_header &&
        marker->marker == JPEG_APP0 &&
        marker->data_length >= 5 &&
        GETJOCTET(marker->data[0]) == 0x4A &&
        GETJOCTET(marker->data[1]) == 0x46 &&
        GETJOCTET(marker->data[2]) == 0x49 &&
        GETJOCTET(marker->data[3]) == 0x46 &&
        GETJOCTET(marker->data[4]) == 0)
      continue;                 /* reject duplicate JFIF */
    if (dstinfo->write_Adobe_marker &&
        marker->marker == JPEG_APP0+14 &&
        marker->data_length >= 5 &&
        GETJOCTET(marker->data[0]) == 0x41 &&
        GETJOCTET(marker->data[1]) == 0x64 &&
        GETJOCTET(marker->data[2]) == 0x6F &&
        GETJOCTET(marker->data[3]) == 0x62 &&
        GETJOCTET(marker->data[4]) == 0x65)
      continue;                 /* reject duplicate Adobe */
    jpeg_write_marker(dstinfo, marker->marker,
                      marker->data, marker->data_length);
  }
}

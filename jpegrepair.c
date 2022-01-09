/*
Repair jpeg images, by the following operations.
- Change color components: Y,Cb,Cr
- Insert blocks
- Delete blocks
- Copy relative blocks

Build:
> gcc jpegrepair.c transupp.c -ljpeg -o jpegrepair

Copyright (c) 2017, Don Mahurin
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <jpeglib.h>
#include "transupp.h"

#define OP_CDELTA 1
#define OP_COPY 2
#define OP_INSERT 3
#define OP_DELETE 4

static void transform(struct jpeg_decompress_struct *srcinfo, jvirt_barray_ptr *coef_arrays, int dest_row, int dest_col, int dest_h, int dest_w, int op, int arg_count, char **args)
{
  int ci, block_y, block_x, by, bx, i;
  int nx, ny;
  JBLOCKARRAY coef_buffer;
  int dv, dh, comp, dc, d, n;
  if(op == OP_CDELTA) { comp = atoi(args[0]); dc = atoi(args[1]); d = 0; }
  else if(op == OP_COPY) { dv = atoi(args[0]); dh = atoi(args[1]); }
  else if(op == OP_INSERT || op == OP_DELETE) { n = atoi(args[0]); }

  bool reverse_order = (op == OP_INSERT || (op == OP_COPY && ((dv < 0 && dh <=0) || (dh < 0 && dv <= 0)))) ? true : false;

  for (ci=0; ci<srcinfo->num_components; ci++) {
    coef_buffer = (srcinfo->mem->access_virt_barray)
      ((j_common_ptr)&srcinfo, coef_arrays[ci], 0,
       srcinfo->comp_info[ci].v_samp_factor, TRUE);
    int h_samp_factor = srcinfo->comp_info[ci].h_samp_factor;
    int v_samp_factor = srcinfo->comp_info[ci].v_samp_factor;
    int height_in_blocks = srcinfo->comp_info[ci].height_in_blocks;
    int width_in_blocks = srcinfo->comp_info[ci].width_in_blocks;

    if(op == OP_CDELTA) {
       if (ci != comp) continue;
    }
    for (block_y=0; block_y<height_in_blocks; block_y++) {
      by = reverse_order ? (height_in_blocks - block_y - 1) : block_y;
      for (block_x=0; block_x<width_in_blocks; block_x++) {
        bx = reverse_order ? (width_in_blocks - block_x - 1) : block_x;
        if(dest_h == 0) { // dest_w assumed to be block count
          if((by / v_samp_factor * width_in_blocks + bx ) / h_samp_factor < ( dest_row * width_in_blocks / h_samp_factor + dest_col)  ||
             (dest_w > 0 &&
               (by / v_samp_factor * width_in_blocks + bx ) / h_samp_factor > dest_w + ( dest_row * width_in_blocks / h_samp_factor + dest_col))) continue;
        } else if(by/v_samp_factor < dest_row || (dest_h > 0 && by/v_samp_factor >= (dest_row + dest_h)) || bx/h_samp_factor < dest_col || (dest_w > 0 && bx/h_samp_factor >= (dest_col + dest_w)) ) continue;

        for (i=0; i<DCTSIZE2; i++) {
          if(op == OP_CDELTA) {
            if (i != d) continue;
          }
          if(op == OP_DELETE) {
             nx = bx + n * h_samp_factor;
             ny = by + ( nx / width_in_blocks ) * v_samp_factor;
             nx = nx % width_in_blocks;
             if(ny < height_in_blocks) {
               coef_buffer[by][bx][i] = coef_buffer[ny][nx][i];
             }
          } else if(op == OP_INSERT) {
             nx = n * h_samp_factor + width_in_blocks - 1 - bx;
             ny = by - ( nx / width_in_blocks ) * v_samp_factor;
             nx = width_in_blocks - ( nx % width_in_blocks ) - 1;
             if(ny >= 0) {
               coef_buffer[by][bx][i] = coef_buffer[ny][nx][i];
             }
          } else if(op == OP_COPY) {
             ny = by + v_samp_factor * dv;
             nx = bx + h_samp_factor * dh;
             if (ny >= 0 && nx >= 0 && ny < height_in_blocks && nx < width_in_blocks)
               coef_buffer[by][bx][i] = coef_buffer[ny][nx][i];
          }
          if(op == OP_CDELTA) {
               coef_buffer[by][bx][i] += dc;
          }
        }
      }
    }
  }
}

int main (int argc, char **argv)
{
  char *infilename = NULL;
  char *outfilename = NULL;
  FILE *infile;
  FILE *outfile;

  struct jpeg_decompress_struct srcinfo;
  struct jpeg_compress_struct dstinfo;
  struct jpeg_error_mgr jerr;
  jvirt_barray_ptr *coef_arrays;

  if (argc < 3) {
    fprintf(stderr, "%s version 0.20211006\n", argv[0]);
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s infile OP ...\n", argv[0]);
    fprintf(stderr, "where OP is:\n");
    fprintf(stderr, "cdelta dest insert delete copy\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "Increase luminance.\n");
    fprintf(stderr, "%s dark.jpg light.jpg cdelta 0 100\n", argv[0]);
    fprintf(stderr, "Fix blueish image.\n");
    fprintf(stderr, "%s blueish.jpg fixed.jpg cdelta 1 -100\n", argv[0]);
    fprintf(stderr, "Insert 2 blocks at position 50:5\n");
    fprintf(stderr, "%s before.jpg after.jpg dest 50 5 insert 2\n", argv[0]);
    fprintf(stderr, "Delete 1 block at position 63:54, and after that, correct luminance. Delete 1 block at position 112:0.\n");
    fprintf(stderr, "%s corrupt.jpg fixed.jpg dest 63 54 delete 1 cdelta 0 -450 dest 112 0 delete 1\n", argv[0]);
    fprintf(stderr, "Copy to position 9:35 2x2 blocks from relative block 1:-20\n");
    fprintf(stderr, "%s before.jpg after.jpg  dest 9 35 2 2 copy 1 -20\n", argv[0]);
    fprintf(stderr, "Show block dimensions using 'dest' with no other arguments\n");
    fprintf(stderr, "%s before.jpg after.jpg dest\n", argv[0]);
    exit(1);
  }

  argc--; argv++;
  infilename = *argv;
  argc--; argv++;

  outfilename = *argv;
  argc--; argv++;

  if ((infile = fopen(infilename, "rb")) == NULL) {
    perror("fopen");
    exit(1);
  }

  srcinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&srcinfo);

  dstinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&dstinfo);

  jpeg_stdio_src(&srcinfo, infile);

  jcopy_markers_setup(&srcinfo, JCOPYOPT_ALL);

  jpeg_read_header(&srcinfo, TRUE);

  coef_arrays = jpeg_read_coefficients(&srcinfo);

  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  int dest_row = 0, dest_col = 0, dest_h = -1 , dest_w = -1;
  while(argc) {
    int op = 0, arg_count;
    if(!strcmp(*argv, "dest")) {
      argc--; argv++;
      if(argc < 2) {
        fprintf(stderr, "dest arguments: BY BX [ BN ]\n");
        fprintf(stderr, "dest arguments: BY BX BH BW\n");
        printf("mcu blocks HxW = %dx%d\n", srcinfo.comp_info[0].height_in_blocks / srcinfo.comp_info[0].v_samp_factor,
          srcinfo.comp_info[0].width_in_blocks / srcinfo.comp_info[0].h_samp_factor);
        break;
      }

      dest_row = atoi(*argv); argc--; argv++;
      dest_col = atoi(*argv); argc--; argv++;
      if(argc > 1 && (isdigit(argv[0][0]) || (argv[0][0] == '-' && isdigit(argv[0][1])))) {
        dest_h = atoi(*argv); argc--; argv++;
        if(argc > 1 && (isdigit(argv[0][0]) || (argv[0][0] == '-' && isdigit(argv[0][1])))) {
          dest_w = atoi(*argv); argc--; argv++;
         } else { dest_w = dest_h; dest_h = 0; }
      } else { dest_h = 0; }
      continue;
    } else if(!strcmp(*argv, "copy")) {
      argc--; argv++;
      op = OP_COPY;
      if(argc < 2) {
        fprintf(stderr, "copy args: dY dX\n\n");
        break;
      }
      arg_count = 2;
    } else if(!strcmp(*argv, "cdelta")) {
      argc--; argv++;
      op = OP_CDELTA;
      if(argc < 2) {
        fprintf(stderr, "cdelta args: COMP dC\n");
        fprintf(stderr, "comp: 0..%d\n", srcinfo.num_components - 1);
        break;
      }
      arg_count = 2;
    } else if(!strcmp(*argv, "insert")) {
      argc--; argv++;
      op = OP_INSERT;
      if(argc < 1) {
        fprintf(stderr, "insert args: N\n");
        break;
      }
      arg_count = 1;
    } else if(!strcmp(*argv, "delete")) {
      argc--; argv++;
      op = OP_DELETE;
      if(argc < 1) {
        fprintf(stderr, "delete args: N\n");
        break;
      }
      arg_count = 1;
    } else {
       fprintf(stderr, "unsupported %s\n", *argv);
       break;
    }
    transform(&srcinfo, coef_arrays, dest_row, dest_col, dest_h, dest_w, op, arg_count, argv);
    argv += arg_count;
    argc -= arg_count;
  }

  if ((outfile = fopen(outfilename, "wb")) == NULL) {
    perror("fopen");
    exit(1);
  }
  jpeg_stdio_dest(&dstinfo, outfile);

  jpeg_write_coefficients(&dstinfo, coef_arrays);

  jcopy_markers_execute(&srcinfo, &dstinfo, JCOPYOPT_ALL);

  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);

  fclose(infile);
  fclose(outfile);

  return 0;
}

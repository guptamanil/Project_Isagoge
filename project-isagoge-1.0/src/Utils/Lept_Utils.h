/*****************************************************************************
 * Project Isagoge: Enhancing the OCR Quality of Printed Scientific Documents
 * File name:		Lept_Utils.h
 * Written by:	Jake Bruce, Copyright (C) 2013
 * History: 		Created Aug 6, 2013 4:35:01 PM 
 * Description: TODO
 * 
 * This file is part of Project Isagoge.
 * 
 * Project Isagoge is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Project Isagoge is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Project Isagoge.  If not, see <http:  //www.gnu.org/licenses/>.
 ****************************************************************************/

#ifndef LEPT_UTILS_H_
#define LEPT_UTILS_H_

#include <allheaders.h>   // leptonica api
#include <vector>
#include <iostream>
using namespace std;
#define fgthresh 100   // this may need to change depending on the image..

// only colors of interest for evaluation purposes are as follows
namespace LayoutEval {
  typedef enum {
    RED, BLUE, GREEN, NONE
  } Color;
}

enum SimpleColor { RED, BLUE, GREEN, NONE };

typedef l_int32 rgbtype;

// static class with various leptonica helper functions
class Lept_Utils {
public:
  // Returns a 3 byte array where (from left to right) the bytes
  // represent red, green, and blue.
  static inline void getPixelRGB(l_uint32* pixel, \
      rgbtype* rgb) {
    extractRGBValues(*pixel, &rgb[0], &rgb[1], \
        &rgb[2]);
  }

  // Sets the given pixel location to the given color
  static inline void setPixelRGB(Pix* pix, \
      l_uint32* pixel, const l_int32& x, const l_int32& y, \
      LayoutEval::Color color) {
    l_uint8 red=0, green=0, blue=0;
    switch (color) {
    case LayoutEval::RED :
      red = 255; break;
    case LayoutEval::GREEN :
      green = 255; break;
    case LayoutEval::BLUE :
      blue = 255; break;
    default : break;
    }
    composeRGBPixel(red, green, blue, pixel);
    pixSetPixel(pix, x, y, *pixel);
  }

  // thickness is both how many pixels wide and high to be drawn with the given x,y in the center
  static inline void drawAtXY(PIX* im, int x, int y, SimpleColor color, int thickness=5) {
    if(thickness < 1) {
      cout << "ERROR: Cannot draw a point less than 1 pixel thick >:-(\n";
      exit(EXIT_FAILURE);
    }
    if(!(thickness % 2))
      ++thickness; // needs to be odd
    l_uint32* startpix = pixGetData(im);
    int startx = x - ((thickness - 1)/2);
    int starty = y - ((thickness - 1)/2);
    int endx = startx + thickness;
    int endy = starty + thickness;
    for(int i = starty; i < endy; i++) {
      for(int j = startx; j < endx; j++) {
        l_uint32* curpix = startpix + l_uint32(j + (i*im->w));
        setPixelRGB(im, curpix, j, i, (LayoutEval::Color)color);
      }
    }
  }

  static int colorPixCount(PIX* im, LayoutEval::Color color);

  // Returns the color (either red, green, or blue) at a given pix
  static inline LayoutEval::Color getPixelColor(l_uint32* pix) {
    rgbtype rgb[3];
    getPixelRGB(pix, rgb);
    return(getColor(rgb));
  }

  // Returns whether the 3 byte rgb color is purely red, green,
  // blue or neither (these are the only 3 colors of interest
  // for evaluation purposes since red = displayed equation,
  // green = displayed equation label, blue = inline equation
  static inline LayoutEval::Color getColor(const \
      rgbtype* const rgb) {
    if(checkRGBEqual(rgb, 255, 0, 0))
      return LayoutEval::RED;
    else if(checkRGBEqual(rgb, 0, 255, 0))
      return LayoutEval::GREEN;
    else if(checkRGBEqual(rgb, 0, 0, 255))
      return LayoutEval::BLUE;
    else
      return LayoutEval::NONE;
  }

  // Returns true if color is either red, green, or blue, otherwise false
  static inline bool isColorSignificant(LayoutEval::Color color) {
    if(color != LayoutEval::NONE)
       return true;
    return false;
  }

  // Returns true if the pixel is likely to be foreground
  static inline bool isDark(const rgbtype* const &rgb) {
    rgbtype thresh = (rgbtype)fgthresh;
    if(checkRGBLessThan(rgb, thresh, thresh, thresh))
      return true;
    return false;
  }

  static inline bool isNonWhite(const rgbtype* const &rgb) {
    rgbtype thresh = (rgbtype)fgthresh;
    if(checkOneRGBLessThan(rgb, thresh, thresh, thresh))
        return true;
    return false;
  }

  // counts the pixels of the given color in the hypothesis image
  // within the hypbox that aren't within the bounds of any of the
  // groundtruth boxes in gtboxes (this gives the false positives
  // for the given hypbox)
  static int countFalsePositives(BOX* hypbox, vector<BOX*> gtboxes, \
      PIX* hypim, LayoutEval::Color color, PIX* dbg=0);

  // counts the pixels of the given color in any of the boxes in
  // the groundtruth box vector of gtim that are also in the
  // hypothesis box (this gives the number of true positives for the
  // given hypbox)
  static int countTruePositives(BOX* hypbox, vector<BOX*> gtboxes, \
      PIX* gtim, LayoutEval::Color color, PIX* dbg=0);

  // counts the pixels of the given color in the groundtruth image
  // within the gtbox that aren't within the bounds of any of the
  // hypothesis boxes in hypboxes (this gives the false negatives
  // for the given gtbox)
  static int countFalseNegatives(BOX* gtbox, vector<BOX*> hypboxes, \
      PIX* gtim, LayoutEval::Color color, PIX* dbg=0);

  // counts the number of pixels in the Box region of
  // the given Pix that have the given color. if
  // countallnonewhite is true then just counts all
  // colors that aren't foreground (assumes all background
  // pixels are white)
  static int countColorPixels(BOX* box, PIX* pix, \
      LayoutEval::Color color, bool countallnonewhite=false);

  // Returns true if the rgb color specified has the same red,
  // green, and blue values specified
  static inline bool checkRGBEqual(const rgbtype* const rgb, rgbtype red, \
      rgbtype green, rgbtype blue) {
    if((*rgb == red) && (*(rgb+1) == green) && (*(rgb+2) == blue))
      return true;
    return false;
  }

  // Returns true if all the channels are less than the given values
  static inline bool checkRGBLessThan(const rgbtype* const &rgb, \
      rgbtype red, rgbtype green, rgbtype blue) {
    if((*rgb < red) && (*(rgb+1) < green) && (*(rgb+2) < blue))
      return true;
    return false;
  }

  // Returns true if one of the channels is less than its corresponding
  // value
  static inline bool checkOneRGBLessThan(const rgbtype* const &rgb, \
      rgbtype red, rgbtype green, rgbtype blue) {
    if((*rgb < red) || (*(rgb+1) < green) || (*(rgb+2) < blue))
      return true;
    return false;
  }

  // Fills all of the foreground pixels within each box on the image
  // Returns the updated image
  static Pix* fillBoxesForeground(Pix* inputimg, BOXA* boxes, LayoutEval::Color color);

  // Fills the foreground pixels for a single box
  static void fillBoxForeground(Pix* inputimg, BOX* box, LayoutEval::Color color, \
      PIX* imread=0);

  inline LayoutEval::Color getRGBAbove(const l_uint32& row, \
      const l_uint32* const &pixel, const l_uint32& width) {
    rgbtype rgbprevpixy[3];
    if (row != 0) {
      getPixelRGB((l_uint32*)(pixel-width), rgbprevpixy);
      return getColor(rgbprevpixy);
    }
    return LayoutEval::NONE;
  }

  inline LayoutEval::Color getRGBBelow(const l_uint32& row, \
      const l_uint32* const &pixel, const l_uint32& width) {
    rgbtype rgbnextpixy[3];
    if (row != 0) {
      getPixelRGB((l_uint32*)(pixel+width), rgbnextpixy);
      return getColor(rgbnextpixy);
    }
    return LayoutEval::NONE;
  }
};

#endif /* LEPT_UTILS_H_ */
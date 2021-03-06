/**************************************************************************
 * Project Isagoge: Enhancing the OCR Quality of Printed Scientific Documents
 * File name:   M_Utils.h
 * Written by:  Jake Bruce, Copyright (C) 2013
 * History:     Created Oct 4, 2013 12:55:26 PM
 * ------------------------------------------------------------------------
 * Description: This is a static class containing assorted utility methods
 *              useful for the MEDS module.
 * ------------------------------------------------------------------------
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
 * along with Project Isagoge.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************************/
#ifndef M_UTILS_H
#define M_UTILS_H

#include <Lept_Utils.h>
#include <cstddef> // defines NULL keyword
#include <iostream>
#include <colpartition.h>
#include <tesseractclass.h>
using namespace std;

namespace tesseract {

class Tesseract;
class ColPartition;
class BLOBINFO;

class M_Utils {
 public:
  // Returns the bounding box for the given BLOBNBOX in image coordinates
  // In the returned coordinate space, the origin is at the top left,
  // x increases to the right and y increases downward. Grabs info from
  // the C_BLOB inside the BLOBNBOX since the BLOBNBOX actually uses the
  // deskewed coordinate space rather than the original. I'm not using the
  // deskewed coordinate space at all in my work.
  static BOX* getBlobBoxImCoords(BLOBNBOX* blob, PIX* im);

  // Convert a box that is already in image coordinates with respect to the
  // entire image (not to a colpartition) to grid coordinates. this converts
  // to the coordinates shown for C_BLOBS, not for BLOBNBOXes (BLOBNBOXES are
  // in the deskewed coordinate space and there are no methods provided by the
  // api that can map that space back to the ordinary space so I am avoiding
  // it altogether).
  static TBOX imToCBlobGridCoords(BOX* box, PIX* im);

  // Finds a column partition in the given grid that overlaps
  // the given TBOX and returns it. Returns NULL if none exists.
  static ColPartition* getTBoxColPart(ColPartitionGrid* cpgrid, TBOX t, PIX* im);

  // Similar to getBlobBoxImCoords but instead returns the bounding box
  // for an entire ColPartition in the same image coordinate space
  static BOX* getColPartImCoords(ColPartition* cp, PIX* im);

  static TBOX getColPartTBox(ColPartition* cp, PIX* im);

  // Same as above but gets coords for C_BLOB
  // However for this one, you have the option of providing the image
  // to find the coordinates in reference to.
  static BOX* getCBlobImCoords(C_BLOB* blob, PIX* im);

  // Convert a TBox which is in tesseract coordinates to
  // a BOX in image coordinates.
  static BOX* tessTBoxToImBox(TBOX* box, PIX* im);

  // Returns the image coordinate bounding box for the
  // given BLOBINFO element
  static BOX* getBlobInfoBox(BLOBINFO* b, PIX* im);

  // Normalizes and runs OCR on the given blob. The implementation of this
  // function was borrowed from Joe Lui's MEDS module. Prior to recognition,
  // the blob is normalized in the same way as blobs are normalized for the
  // purpose of orientation and script detection (OSD). The ocr engine is
  // passed in as an argument (could be trained for any language)
  static BLOB_CHOICE* runBlobOCR(BLOBNBOX* blob, Tesseract* ocrengine);

  // Takes a BLOB_CHOICE, the result of running OCR on a BLOBNBOX, and
  // returns its result as a character array. Needs the version of tesseract
  // on which it was recognized to be provided as the second argument.
  static const char* const getBlobChoiceUnicode(BLOB_CHOICE* bc, \
      Tesseract* ocrengine);

  // Debug funtion to quickly display a BLOBNBOX, run OCR on it, and
  // show the result
  static void dispBlobOCRRes(BLOBNBOX* blob, PIX* im, Tesseract* ocrengine);

  // Waits for user input. This is useful in debugging.
  static void waitForInput();

  // For debugging display the coordinates of a box
  static void dispBoxCoords(BOX* box);

  // For debugging, display the region of the image
  static void dispRegion(BOX* box, PIX* im);

  // convenience function for debugging, display a blobinfo boundingbox
  static void dispBlobInfoRegion(BLOBINFO* bb, PIX* im);

  // similar to dispBlobInfoRegion but displays the entire image with
  // the foreground pixels of the blob of interest highlighted in red
  static void dispHlBlobInfoRegion(BLOBINFO* bb, PIX* im);

  // similar to previous but takes tbox as input instead of blobinfo
  static void dispHlTBoxRegion(TBOX tbox, PIX* im);

  // similar to dispHlBlobInfoRegion but only draws, doesn't display
  // draws directly on the pix, rather than making a temp copy
  static void drawHlBlobInfoRegion(BLOBINFO* bb, PIX* im, LayoutEval::Color color);

  // draws the box on the given image
  static void drawHlBoxRegion(BOX* bb, PIX* im, LayoutEval::Color color);

  static void dispTBoxCoords(TBOX* box);

  // copy contents of string into newly allocated memory
  // returns pointer to the newly allocated string
  static char* strDeepCpy(const char* const str);

  // splits a string into a vector where each etnry is
  // a separate line (or if their's only one line then
  // just a vector with a single entry). doesn't modify
  // the original string, but the contents of the resulting
  // vector are always deep copies of the input string
  // even if there's only a single entry (no shallow
  // copies). The caller is responsible for deallocating
  // each entry of the vector. This is to prevent
  // memory leaks and dangling pointers.
  static GenericVector<char*> lineSplit(const char* txt);
};

} // end namespace tesseract

#endif

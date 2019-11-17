/**
 * Read data from captured images via text recognition using
 * tesseract-ocr.
 *
 * @copyright Copyright (c) 2015, Matthias Behr
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Matthias Behr <mbehr (@) mcbehr.de>
 */
/*
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

/*
Quick patent check: (no warranties, just my personal opinion, only checked as I respect IP rights)
 DE1020100530017 -> describing a different method to calculate the "Uebertrag". should not be
affected EP1596164 -> using basically two different light sources. should not be affected
 DE102010053019 -> nice method to ease positioning sensors (see FAST EnergyCam). should not be
affected.
 TODO check some more.

 my opinion: simple image recognition for numbers is "Stand der Technik"
*/

/*
	To-Do list:
	TODO add conf level that has to be achieved otherwise values will be ignored
	TODO implement digit (just to add more conf by restricting the value to int -9...+9)
	TODO move log_error to log_debug
	TODO use GetComponentImages interface (directly returning BOXA) and change to
   top/left/width/height?
	TODO use filter from http://www.jofcis.com/publishedpapers/2011_7_6_1886_1892.pdf for
   binarization?
	TODO check seedfill functions
	TODO think about using either some trainingdata or e.g.
   http://www.unix-ag.uni-kl.de/~auerswal/ssocr/ to detect LCD digits.
	TODO sanity check if digit used and recognized bb is far too small.
	TODO check leptonicas 1.71 simple character recognition
	TODO add capability to compile without tesseract -> RecognizerNeedle only.
	TODO add offset/shift for needles (e.g. on the test pics the x0.1 needle is around a 18deg. late
   (e.g. at 5.4(from x0.01) the needle is at exactly 5 not at 5.4) (autolearn?)
	TODO needle autocalibrate (autofix like) in 2 pass approach? (1st pass as it is, 2nd pass with
   the center detection?)
	*/

#include <sys/inotify.h>
#include <unistd.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/time.h>
// #include <errno.h>
#include <cmath>

#include "Options.hpp"
#include "protocols/MeterOCR.hpp"
#include <VZException.hpp>

/* install libleptonica: see MeterOCR.cpp */

/* install tesseract:
sudo apt-get install libtool libpng-dev libjpeg-dev
git clone https://code.google.com/p/tesseract-ocr/
cd tesseract-ocr
git checkout 3.02.02
./autogen.sh
./configure
make
sudo make install
sudo ldconfig
cd ..
wget https://tesseract-ocr.googlecode.com/files/tesseract-ocr-3.02.deu.tar.gz
tar xzvf tesseract-ocr-3.02.deu.tar.gz
sudo cp tesseract-ocr/tessdata/deu.traineddata /usr/local/share/tessdata/
sudo cp tesseract-ocr/tessdata/deu-frak.traineddata /usr/local/share/tessdata/

*/

#include <leptonica/allheaders.h>
#include <tesseract/baseapi.h>

MeterOCR::RecognizerTesseract::RecognizerTesseract(struct json_object *jr)
	: Recognizer("tesseract", jr), api(0), _gamma(1.0), _gamma_min(50), _gamma_max(120),
	  _min_x1(INT_MAX), _max_x2(INT_MIN), _min_y1(INT_MAX), _max_y2(INT_MIN),

	  _all_digits(true) {
	struct json_object *value;
	if (json_object_object_get_ex(jr, "gamma", &value)) {
		_gamma = json_object_get_double(value);
	}
	if (json_object_object_get_ex(jr, "gamma_min", &value)) {
		_gamma_min = json_object_get_int(value);
	}
	if (json_object_object_get_ex(jr, "gamma_max", &value)) {
		_gamma_max = json_object_get_int(value);
	}

	// calc the max. bounding box. images will be cropped to it:
	for (StdListBB::iterator it = _boxes.begin(); it != _boxes.end();
		 ++it) { // let's stick to begin not cbegin (c++11)
		const BoundingBox &b = *it;
		if (b.boxType == BoundingBox::BOX) {
			if (b.x1 < _min_x1)
				_min_x1 = b.x1;
			if (b.x2 > _max_x2)
				_max_x2 = b.x2;
			if (b.y1 < _min_y1)
				_min_y1 = b.y1;
			if (b.y2 > _max_y2)
				_max_y2 = b.y2;
			if (!b.digit)
				_all_digits = false;
		} else
			throw vz::OptionNotFoundException("boundingbox without box (circle?)");
	}

	// extend by autorange: (not needed anymore, will be done before)
	/*
	if (_autofix_range>0){
		if (_autofix_x-_autofix_range < _min_x1) _min_x1 = _autofix_x-_autofix_range;
		if (_autofix_y-_autofix_range < _min_y1) _min_y1 = _autofix_y-_autofix_range;
		if (_autofix_x+_autofix_range > _max_x2) _max_x2 = _autofix_x+_autofix_range;
		if (_autofix_y+_autofix_range > _max_y2) _max_y2 = _autofix_y+_autofix_range;
	}*/
	if (_min_x1 < 0)
		_min_x1 = 0;
	if (_min_y1 < 0)
		_min_y1 = 0;
}

bool MeterOCR::RecognizerTesseract::recognize(PIX *imageO, int dX, int dY, ReadsMap &readings,
											  const ReadsMap *old_readings, PIXA *debugPixa) {

	// init tesseract (TODO if we do this in the constructor we get corrupted readings after 3-4
	// read calls....) this is just a workaround
	if (!initTesseract())
		return 0;

	PIX *image = pixClone(imageO);

	// now crop the image if possible:
	if (_min_x1 > 0 || _max_x2 > 0 || _min_y1 > 0 || _max_y2 > 0) {
		int w = _max_x2 > 0 ? _max_x2 : pixGetWidth(image);
		w -= _min_x1;
		int h = _max_y2 > 0 ? _max_y2 : pixGetHeight(image);
		h -= _min_y1;
		print(log_error, "Cropping image to (%d,%d)x(%d,%d)", "RecognizerTesseract", _min_x1,
			  _min_y1, w, h);
		PIX *image2 = pixCreate(w, h, pixGetDepth(image));
		pixCopyResolution(image2, image);
		pixCopyColormap(image2, image);
		pixRasterop(image2, 0, 0, w, h, PIX_SRC, image, _min_x1 + dX, _min_y1 + dY);
		pixDestroy(&image);
		image = image2;
		saveDebugImage(debugPixa, image, "cropped");
	}

	// Convert the RGB image to grayscale
	Pix *image_gs = pixConvertRGBToLuminance(image);
	if (image_gs) {
		pixDestroy(&image);
		image = image_gs;
		image_gs = 0;
		saveDebugImage(debugPixa, image, "grayscale");
	}

	// Increase the dynamic range
	// make dark gray *black* and light gray *white*
	image = pixGammaTRC(image, image, _gamma, _gamma_min, _gamma_max);
	if (image) {
		saveDebugImage(debugPixa, image, "gamma");
	}

	// image_gs = pixCloseGray(image, 25, 25);
	image_gs = pixUnsharpMaskingGray(image, 3,
									 0.5); // TODO make a parameter, only useful for blurred images
	if (image_gs) {
		pixDestroy(&image);
		image = image_gs;
		image_gs = 0;
		saveDebugImage(debugPixa, image, "unsharp");
	}

	// Normalize for uneven illumination on gray image
	Pix *pixg = 0;
	pixBackgroundNormGrayArrayMorph(image, NULL, 4, 5, 200, &pixg);
	image_gs = pixApplyInvBackgroundGrayMap(image, pixg, 4, 4);
	pixDestroy(&pixg);

	if (image_gs) {
		pixDestroy(&image);
		image = image_gs;
		image_gs = 0;
		saveDebugImage(debugPixa, image, "normalize");
	}

	image_gs = pixBlockconv(image, 1, 1);
	if (image_gs) {
		pixDestroy(&image);
		image = image_gs;
		image_gs = 0;
		saveDebugImage(debugPixa, image, "blockconv");
	}

	// Threshold to 1 bpp
	image_gs = pixThresholdToBinary(image, 120);
	if (image_gs) {
		pixDestroy(&image);
		image = image_gs;
		image_gs = 0;
		saveDebugImage(debugPixa, image, "binary");
	}

	api->SetImage(image);

	Pix *dump = api->GetThresholdedImage();
	//	outfilename=_file;
	//	outfilename.append("thresh.tif");
	//    pixWrite(outfilename.c_str(), dump, IFF_TIFF_G4);

	// get picture size:
	int width = pixGetWidth(image);
	int height = pixGetHeight(image);
	print(log_error, "Image size width=%d, height=%d\n", "RecognizerTesseract", width, height);

	BOXA *boxa = boxaCreate(_boxes.size()); // there can be more, just initial number
	BOXA *boxb = boxaCreate(_boxes.size());

	// add autofix target to boxb (blue) and found to boxa(green)
	/*
	if (_autofix_range>0){
		int cx = _autofix_x - _min_x1 - autofix_dX;
		int cy = _autofix_y - _min_y1 - autofix_dY;
		boxaAddBox(boxb, boxCreate(cx - _autofix_range, cy - _autofix_range, 2* _autofix_range,
	2*_autofix_range), L_INSERT); if (_autofix_range>1){ // add the center boxaAddBox(boxb,
	boxCreate(cx,cy, 1, 1), L_INSERT);
		}
		boxaAddBox(boxa, boxCreate(cx+autofix_dX, cy+autofix_dY, 1, 1), L_INSERT);
	}
	*/

	// now iterate for each bounding box defined:
	for (StdListBB::iterator it = _boxes.begin(); it != _boxes.end();
		 ++it) { // let's stick to begin not cbegin (c++11)
		const BoundingBox &b = *it;
		int left = b.x1 >= _min_x1 ? b.x1 - _min_x1 : 0;
		int top = b.y1 >= _min_y1 ? b.y1 - _min_y1 : 0;
		int w = b.x2 >= 0 ? b.x2 - left - _min_x1 : (width - left);
		int h = b.y2 >= 0 ? b.y2 - top - _min_y1 : (height - top);
		api->SetRectangle( // left, top, width, height // BoundingBox are abs. coordinates.
						   // SetRectangle are relative (w/h)
			left, top, w, h);

		BOX *box = boxCreate(left, top, w, h);
		boxaAddBox(boxb, box, L_INSERT);

		if (api->Recognize(0) == 0) {
			std::string outtext;
			tesseract::ResultIterator *ri = api->GetIterator();
			tesseract::PageIteratorLevel level = tesseract::RIL_WORD;
			double min_conf = DBL_MAX;
			if (ri != 0) {
				do {
					const char *word = ri->GetUTF8Text(level);
					float conf = ri->Confidence(level);
					int x1, y1, x2, y2;
					ri->BoundingBox(level, &x1, &y1, &x2, &y2);
					print(log_error, "word: '%s'; \tconf: %.2f; BoundingBox: %d,%d,%d,%d;\n",
						  "RecognizerTesseract", word, conf, x1, y1, x2, y2);
					if (conf > 15.0 && outtext.length() == 0) {
						outtext = word; // TODO choose the one with highest confidence? or ignore if
										// more than 1?
						if (conf < min_conf)
							min_conf = conf;
					}
					if (word)
						delete[] word;

					// for debugging draw the box in the picture:
					BOX *box = boxCreate(x1, y1, x2 - x1, y2 - y1);
					boxaAddBox(boxa, box, L_INSERT);
				} while (ri->Next(level));
				delete ri;
			}
			print(log_error, "%s=%s", "RecognizerTesseract", b.identifier.c_str(), outtext.c_str());

			if (b.conf_id.length())
				readings[b.identifier].conf_id = b.conf_id;

			// if we couldn't read any text mark this as not available (using NAN (not a number))
			if (outtext.length() == 0) {
				readings[b.identifier].value = NAN;
				readings[b.identifier].min_conf = 0;
			} else {
				readings[b.identifier].value += strtod(outtext.c_str(), NULL) * pow(10, b.scaler);
				if (min_conf < readings[b.identifier].min_conf)
					readings[b.identifier].min_conf = min_conf;
				outtext.clear();
			}
		}
	}

	Pix *bbpix2 = pixDrawBoxa(dump, boxa, 1, 0x00ff0000);  // rgba color -> green, detected
	Pix *bbpix = pixDrawBoxa(bbpix2, boxb, 1, 0x0000ff00); // blue = bounding box for search
	pixDestroy(&bbpix2);
	boxaDestroy(&boxa);
	boxaDestroy(&boxb);
	saveDebugImage(debugPixa, bbpix, "bb");
	pixDestroy(&bbpix);

	pixDestroy(&dump);
	pixDestroy(&image);
	return true;
}

bool MeterOCR::RecognizerTesseract::initTesseract() {
	if (api)
		deinitTesseract(); // we want to deinit/init in this case on purpose! (see TODO in read)

	// init tesseract-ocr without specifiying tessdata path
	api = new tesseract::TessBaseAPI();

	// disable dictionary:
	api->SetVariable("load_system_dawg", "F");
	api->SetVariable("load_freq_dawg", "F");

	// only for debugging:
	api->SetVariable("tessedit_write_images", "T");

	if (api->Init(NULL, "deu")) {
		delete api;
		api = NULL;
		print(log_error, "Could not init tesseract!", "RecognizerTesseract");
		throw vz::VZException("could not init tesseract");
	}

	api->SetVariable(
		"tessedit_char_whitelist",
		"0123456789.m"); // TODO think about removing 'm' (should not be within the boundingboxes)
	api->SetPageSegMode(_all_digits ? tesseract::PSM_SINGLE_CHAR
									: tesseract::PSM_SINGLE_BLOCK); // PSM_SINGLE_WORD);

	return true;
}

bool MeterOCR::RecognizerTesseract::deinitTesseract() {
	if (!api)
		return false;
	api->End();
	delete api;
	api = 0;
	return true;
}

MeterOCR::RecognizerTesseract::~RecognizerTesseract() {
	// end tesseract usage:
	deinitTesseract();
}

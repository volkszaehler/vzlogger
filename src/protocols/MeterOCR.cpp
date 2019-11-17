/**
 * Read data from captured images via image recognition.
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
 TODO p3 check some more especially on needle detection and not character recognition.

 my opinion: simple image recognition for numbers is "Stand der Technik"
*/

/*
	To-Do list: (p1 = mandatory before release, p2 = would be good, p3 = nice to have)
	TODO p3 add conf level that has to be achieved otherwise values will be ignored
	TODO p3 tesseract only: implement digit (just to add more conf by restricting the value to int
   -9...+9)
	TODO p3 use GetComponentImages interface (directly returning BOXA) and change to
   top/left/width/height?
	TODO p3 use filter from http://www.jofcis.com/publishedpapers/2011_7_6_1886_1892.pdf for
   binarization?
	TODO p3 check seedfill functions
	TODO p2 think about using either some trainingdata or e.g.
   http://www.unix-ag.uni-kl.de/~auerswal/ssocr/ to detect LCD digits.
	TODO p3 tesseract only: sanity check if digit used and recognized bb is far too small.
	TODO p2 check leptonicas 1.71 simple character recognition
	*/

#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/time.h>
#include <climits>
#include <cmath>
#include <errno.h>

#include "Options.hpp"
#include "protocols/MeterOCR.hpp"
#include <VZException.hpp>
#include <json-c/json.h>

#include <assert.h>
#include <linux/videodev2.h>

/* install libleptonica:

sudo apt-get install libpng-dev libtiff-dev
wget http://www.leptonica.org/source/leptonica-1.71.tar.gz
tar -zxvf leptonica-1.71.tar.gz
cd leptonica-1.71
./configure --disable-programs
make
sudo make install
sudo ldconfig

*/

/* install libyuv

  git clone http://git.chromium.org/external/libyuv.git
  cd libyuv
  mkdir out
  cd out
  cmake -DCMAKE_INSTALL_PREFIX="/usr/lib" -DCMAKE_BUILD_TYPE="Release" ..
  cmake --build . --config Release
  sudo cmake --build . --target install --config Release

*/

/* install tesseract: (only if needed, i.e. ENABLE_OCR_TESSERACT defined)
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

#include <allheaders.h> // from leptonica

MeterOCR::BoundingBox::BoundingBox(struct json_object *jb)
	: scaler(0), digit(false), boxType(BOX), x1(-1), y1(-1), x2(-1), y2(-1), cx(-1), cy(-1), cr(-1),
	  offset(0.0), autocenter(true), ac_dx(0),
	  ac_dy(0) // todo p2 make autocenter parameter default false
{
	// we need at least "identifier"
	struct json_object *value;
	if (json_object_object_get_ex(jb, "identifier", &value)) {
		identifier = json_object_get_string(value);
	} else {
		throw vz::OptionNotFoundException("boundingbox identifier");
	}
	if (json_object_object_get_ex(jb, "confidence_id", &value)) {
		conf_id = json_object_get_string(value);
	}
	if (json_object_object_get_ex(jb, "scaler", &value)) {
		scaler = json_object_get_int(value);
	}
	if (json_object_object_get_ex(jb, "digit", &value)) {
		digit = json_object_get_boolean(value);
	}
	// now parse the box coordinates:
	if (json_object_object_get_ex(jb, "box", &value)) {
		// x1, y1, x2, y2
		struct json_object *jv;
		if (json_object_object_get_ex(value, "x1", &jv)) {
			x1 = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(value, "y1", &jv)) {
			y1 = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(value, "x2", &jv)) {
			x2 = json_object_get_int(jv);
			if (x2 < x1)
				throw vz::OptionNotFoundException("boundingbox x2 < x1");
		}
		if (json_object_object_get_ex(value, "y2", &jv)) {
			y2 = json_object_get_int(jv);
			if (y2 < y1)
				throw vz::OptionNotFoundException("boundingbox y2 < y1");
		}
	} else if (json_object_object_get_ex(jb, "circle", &value)) {
		boxType = CIRCLE;
		// cx, cy, cr
		struct json_object *jv;
		if (json_object_object_get_ex(value, "cx", &jv)) {
			cx = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(value, "cy", &jv)) {
			cy = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(value, "cr", &jv)) {
			cr = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(value, "offset", &jv)) {
			offset = json_object_get_double(jv);
			if (offset < -10.0 || offset > 10.0)
				throw vz::VZException("offset invalid value <0 or >10");
		}
		if (cx < cr || cy < cr || cr < MIN_RADIUS)
			throw vz::OptionNotFoundException("circle cx < cr or cy < cr or cr<10");
	}

	print(log_debug, "boundingbox <%s>: conf_id=%s, scaler=%d, digit=%d, (%d,%d)-(%d,%d)\n", "ocr",
		  identifier.c_str(), conf_id.c_str(), scaler, digit ? 1 : 0, x1, y1, x2, y2);
}

// comparison
// returns true if the first argument goes before the second argument in the strict weak ordering it
// defines, and false otherwise.
bool MeterOCR::BoundingBox::compare(const MeterOCR::BoundingBox &first,
									const MeterOCR::BoundingBox &second) {
	return (first.scaler < second.scaler);
}

MeterOCR::Recognizer::Recognizer(const std::string &str_type, struct json_object *jr)
	: _type(str_type) {
	struct json_object *value;
	try {
		if (json_object_object_get_ex(jr, "boundingboxes", &value)) {
			print(log_debug, "boundingboxes=%s", "ocr",
				  value ? json_object_to_json_string(value) : "<null>");
			int nrboxes;
			if (value && (nrboxes = json_object_array_length(value)) >= 1) {
				// for each object:
				for (int i = 0; i < nrboxes; i++) {
					struct json_object *jb = json_object_array_get_idx(value, i);
					_boxes.push_back(BoundingBox(jb));
				}
				_boxes.sort(BoundingBox::compare); // we sort by smallest scaler first.
			} else {
				throw vz::OptionNotFoundException("empty boundingboxes given");
			}
		} else {
			throw vz::OptionNotFoundException("no boundingboxes given");
		}
	} catch (vz::OptionNotFoundException &e) {
		// boundingboxes is mandatory
		throw;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse 'boundingboxes'", "ocr");
		throw;
	}
}

MeterOCR::RecognizerNeedle::RecognizerNeedle(struct json_object *jr)
	: Recognizer("needle", jr), _min_x(INT_MAX), _min_y(INT_MAX), _max_x(INT_MIN), _max_y(INT_MIN) {
	// check for _kernelColorString
	struct json_object *value;
	if (json_object_object_get_ex(jr, "kernelColorString", &value)) {
		_kernelColorString = json_object_get_string(value);
		if (_kernelColorString.length())
			_kernelColorString.append(" "); // append a space to ease kernelCreateFromString
	}
	// check whether all are circles:
	for (StdListBB::iterator it = _boxes.begin(); it != _boxes.end(); ++it) {
		const BoundingBox &b = *it;
		if (b.boxType != BoundingBox::CIRCLE)
			throw vz::OptionNotFoundException("boundingbox without circle");
		int r = b.cr; // we keep a border for autocenter
		if (b.autocenter)
			r += b.cr;
		if (b.cx - r < _min_x)
			_min_x = b.cx - r;
		if (b.cy - r < _min_y)
			_min_y = b.cy - r;
		if (b.cx + r > _max_x)
			_max_x = b.cx + r;
		if (b.cy + r > _max_y)
			_max_y = b.cy + r;
	}
}

bool MeterOCR::RecognizerNeedle::recognize(PIX *imageO, int dX, int dY, ReadsMap &readings,
										   const ReadsMap *old_readings, PIXA *debugPixa) {
	if (!imageO)
		return false;
	// 1st step: crop the image:
	PIX *image = pixClone(imageO);

	// now crop the image if possible:
	print(log_debug, "Cropping image to (%d,%d)-(%d,%d)", "ocr", _min_x, _min_y, _max_x, _max_y);
	PIX *image2 = pixCreate(_max_x - _min_x, _max_y - _min_y, pixGetDepth(image));
	pixCopyResolution(image2, image);
	pixCopyColormap(image2, image);
	pixRasterop(image2, 0, 0, _max_x - _min_x, _max_y - _min_y, PIX_SRC, image, _min_x + dX,
				_min_y + dY);
	pixDestroy(&image);
	image = image2;
	saveDebugImage(debugPixa, image, "cropped");

	// now filter on red color either using provided matrix or std. internally the needles have to
	// be red.
	L_KERNEL *kel;
	if (_kernelColorString.length())
		kel = kernelCreateFromString(3, 3, 0, 0, _kernelColorString.c_str());
	else { // use default: only red channel amplified
		kel = kernelCreate(3, 3);
		kernelSetElement(kel, 0, 0, 2.0);
		kernelSetElement(kel, 0, 1, -1.0);
		kernelSetElement(kel, 0, 2, -1.0);
	}

	image2 = pixMultMatrixColor(image, kel);
	pixDestroy(&image);
	image = image2;
	saveDebugImage(debugPixa, image, "multcolor");

	// now iterate for each bounding box defined:
	for (StdListBB::iterator it = _boxes.begin(); it != _boxes.end();
		 ++it) { // let's stick to begin not cbegin (c++11)
		BoundingBox &b = *it;
		int autocenter_redocnt = 0;
		bool autocenter_redo;
		int conf;
		int degFrom, degTo, degAvg;
		do {
			autocenter_redo = false;
			int cx = b.cx + b.ac_dx - _min_x;
			int cy = b.cy + b.ac_dy - _min_y;
			conf = 100;
			if (b.autocenter) {
				// verify that the center is red:
				unsigned int c = 0;
				(void)pixGetPixel(image, cx, cy, &c);
				if (c < RED_COLOR_LIMIT) {
					conf = 0;
					print(log_error, "recognizerNeedle center not red!\n", "ocr");
					break;
				}
			} else {
				// verify that the center and one pix to each side is red:
				for (int x = cx - 1; conf > 0 && x <= cx + 1; ++x)
					for (int y = cy - 1; y <= cy + 1; ++y) {
						unsigned int c = 0;
						(void)pixGetPixel(image, x, y, &c);
						if (c < RED_COLOR_LIMIT) {
							conf = 0;
							print(log_error, "recognizerNeedle center not red!\n", "ocr");
							break;
						}
					}
			}
			degFrom = -1;
			degTo = -1;
			degAvg = -1;

			if (conf > 0) {
				// draw the center: in green
				pixSetPixel(image, cx, cy, 0x00ff0000);

				// now scan the circle around, draw for debugging blue = not detected, red =
				// detected:
				const float PI_F = 3.14159265358979f;
				const float PIrad = PI_F / 180;
				int opx = -1, opy = -1;
				bool wrap = false;

				for (int deg = 0; deg < 360; ++deg) {
					int px = cx + b.cr * sin(deg * PIrad);
					int py = cy - b.cr * cos(deg * PIrad);
					if (opx != px || opy != py) { // don't scan the same pixel twice
						unsigned int c = 0;
						(void)pixGetPixel(image, px, py, &c);
						if (c > RED_COLOR_LIMIT) {
							if (deg == 0)
								wrap = true;

							if (wrap) { // needle around 0 deg (e.g. 355-5) -> degTo:5, degFrom:-5
										// TODO p2 add unit test for this case!
								if (deg < 180)
									degTo = deg;
								else if ((deg - 360) < degFrom)
									degFrom = deg - 360;
							} else {
								if (degFrom < 0)
									degFrom = deg;
								degTo = deg;
							}

							pixSetPixel(
								image, px, py,
								0xff000000); // draw in red so it can be detected next time as well
						} else
							pixSetPixel(image, px, py, 0x0000ff00);
						opx = px;
						opy = py;
					}
				}
				degAvg = (degFrom + degTo) / 2;
				if (degTo < 0)
					conf = 0;
				else {
					if (b.autocenter) {
						// autocenter: check distance from center at the detected
						// needle +90/+180/+270 degrees to the circle
						// adjust ac_dx/ac_dy so that the distances are equal:
						// if ac_dx/ac_dy changes >+/-1 then redo calc a few times:
						int det_r[3];
						int ind = 0;
						for (int deg = degAvg + 90; deg < degAvg + 360; deg += 90) {
							int opx = -1, opy = -1;
							int r;
							for (r = b.cr - 1; r > 0; --r) {
								int px = cx + r * sin(deg * PIrad);
								int py = cy - r * cos(deg * PIrad);
								if (px != opx || py != opy) {
									unsigned int c = 0;
									(void)pixGetPixel(image, px, py, &c);
									if (c > RED_COLOR_LIMIT) {
										break;
									}
									opx = px;
									opy = py;
								}
							}
							if (r <= 0)
								break;
							det_r[ind++] = r;
							print(log_debug, "scanning at %d: r=%d", "ocr", deg, r);
						}
						if (ind < 3) {
							print(log_error, "couldn't autocenter!", "ocr");
						} else {
							// now calculate the new center offset:
							float sh_r = (det_r[0] + det_r[2]) / 2.0;
							// adjust due to line r0---r2:
							// to move center to direction 0deg: decrease cy
							// to move center to direction 90deg: increase cx
							// to move center to direction 180 deg: increase cy
							// to move center to direction 270deg: decrease cx
							// if det_r0 > sh_r we need to move center in direction deg+90:
							float ndx = (det_r[0] - sh_r) * sin((degAvg + 90) * PIrad);
							float ndy = (-(det_r[0] - sh_r)) * cos((degAvg + 90) * PIrad);
							print(log_debug, "ndx=%f ndy=%f", "ocr", ndx, ndy);
							// adjust due to r1:
							// if det_r1 > sh_r we need to move center in dir deg+180
							ndx += ((det_r[1] - sh_r) * sin((degAvg + 180) * PIrad));
							ndy += (-(det_r[1] - sh_r) * cos((degAvg + 180) * PIrad));
							print(log_debug, "ndx=%f ndy=%f", "ocr", ndx, ndy);
							// draw the center: in white
							pixSetPixel(image, cx + lrintf(ndx), cy + lrintf(ndy), 0xffffff00);
							if (fabsf(ndx) > 1.0 || fabsf(ndy) > 1.0) {
								b.ac_dx += lrintf(ndx); // round properly here
								b.ac_dy += lrintf(ndy);
								++autocenter_redocnt;
								autocenter_redo = true;
							}
						}
					}
				}
			}
		} while (autocenter_redo && autocenter_redocnt < 3);
		if (b.conf_id.length())
			readings[b.identifier].conf_id = b.conf_id;
		if (conf) {
			int nr = b.offset + (degAvg / 36);
			if (nr < 0)
				nr += 10;
			if (nr > 9)
				nr -= 10;
			double fnr = b.offset + ((double)(degFrom + degTo) / 72);
			if (fnr < 0)
				fnr += 10.0;
			if (fnr > 10.0)
				fnr -= 10.0;

			// the boundingboxes are sorted by smallest scaler first. there might be a value already
			// that we can use to check whether we should round up already (round up -> smaller
			// digit <0.5, round down -> smaller digit >0.5) we take this only into account if
			// b.digit is not true: (so we can mark the smallest digit with true)
			if (!b.digit) {
				double ip;
				double prev_dig = modf(readings[b.identifier].value / pow(10, b.scaler),
									   &ip); // prev. digit as 0.x
				nr = roundBasedOnSmallerDigits(nr, fnr, prev_dig, conf);
			} else {
				if (old_readings) {
					// try to debounce if old value is available:
					ReadsMap::const_iterator it = (*old_readings).find(b.identifier);
					if (it != old_readings->end()) {
						double ip;
						int prev_dig = lrint(modf((*it).second.value / pow(10, b.scaler + 1), &ip) *
											 10); // prev digit as 0.x
						nr = debounce(prev_dig, fnr);
					}
				}
			}
			readings[b.identifier].value += (double)nr * pow(10, b.scaler);
			if (conf < readings[b.identifier].min_conf)
				readings[b.identifier].min_conf = conf; // TODO p2 (by ratio detected pixel vs. non
														// detected vs. abs(degFrom-degTo)
		} else {
			readings[b.identifier].value = NAN;
			readings[b.identifier].min_conf = 0;
		}
	}

	saveDebugImage(debugPixa, image, "scanned");
	pixDestroy(&image);

	return true;
}

int MeterOCR::RecognizerNeedle::roundBasedOnSmallerDigits(const int curNr, const double &fnr,
														  const double &smaller, int &conf) const {
	int nr = curNr;
	if (smaller < 0.5) {
		int nnr = (fnr + 0.5 -
				   smaller); // e.g. 8.1 detected as 0.1 and 7.99 -> nr = (7,99+0,5-0.1)=8.39 = 8
		if (nnr != nr) {
			nr = nnr;
			if (nr >= 10)
				nr = 0;
			conf -= 10;
			print(log_debug, "returning rounded up: smaller=%f nr=%d fnr=%f\n", "ocr", smaller, nr,
				  fnr);
		}
	}
	if (smaller >= 0.5) { // e.g. 3.9 detected as 0.9 and 4.1 -> nr = (4.1-0.2) = 3.9 = 3
		int nnr = fnr < (smaller - 0.5) ? 9 : (fnr - smaller + 0.5);
		if (nnr != nr) {
			nr = nnr;
			conf -= 15;
			print(log_debug, "returning rounded down: smaller=%f nr=%d fnr=%f\n", "ocr", smaller,
				  nnr, fnr);
		}
	}
	return nr;
}

MeterOCR::RecognizerBinary::RecognizerBinary(struct json_object *jr)
	: Recognizer("binary", jr), _min_x(INT_MAX), _min_y(INT_MAX), _max_x(INT_MIN), _max_y(INT_MIN),
	  _last_state(false), _EDGE_HIGH(70), _EDGE_LOW(30)

{
	// check for _kernelColorString
	struct json_object *value;
	if (json_object_object_get_ex(jr, "kernelColorString", &value)) {
		_kernelColorString = json_object_get_string(value);
		if (_kernelColorString.length())
			_kernelColorString.append(" "); // append a space to ease kernelCreateFromString
	}
	// check whether all are boxes:
	// and currently just one allowed:
	if (_boxes.size() != 1) {
		throw vz::OptionNotFoundException("Recognizer binary just needs exactly one boundingbox");
	}
	for (StdListBB::iterator it = _boxes.begin(); it != _boxes.end(); ++it) {
		const BoundingBox &b = *it;
		if (b.boxType != BoundingBox::BOX)
			throw vz::OptionNotFoundException("boundingbox without box");
		if (b.x1 < _min_x)
			_min_x = b.x1;
		if (b.y1 < _min_y)
			_min_y = b.y1;
		if (b.x2 > _max_x)
			_max_x = b.x2;
		if (b.y2 > _max_y)
			_max_y = b.y2;
	}
}

MeterOCR::RecognizerBinary::~RecognizerBinary() {}

bool MeterOCR::RecognizerBinary::recognize(PIX *imageO, int dX, int dY, ReadsMap &readings,
										   const ReadsMap *old_readings, PIXA *debugPixa) {
	if (!imageO)
		return false;
	// 1st step: crop the image:
	PIX *image = pixClone(imageO);

	// now crop the image if possible:
	print(log_debug, "Cropping image to (%d,%d)-(%d,%d)", "ocr", _min_x, _min_y, _max_x, _max_y);
	PIX *image2 = pixCreate(_max_x - _min_x, _max_y - _min_y, pixGetDepth(image));
	pixCopyResolution(image2, image);
	pixCopyColormap(image2, image);
	pixRasterop(image2, 0, 0, _max_x - _min_x, _max_y - _min_y, PIX_SRC, image, _min_x + dX,
				_min_y + dY);
	pixDestroy(&image);
	image = image2;
	saveDebugImage(debugPixa, image, "cropped");

	// now filter on red color either using provided matrix or std. internally the needles have to
	// be red.
	L_KERNEL *kel;
	if (_kernelColorString.length())
		kel = kernelCreateFromString(3, 3, 0, 0, _kernelColorString.c_str());
	else { // use default: only red channel amplified
		kel = kernelCreate(3, 3);
		kernelSetElement(kel, 0, 0, 2.0);
		kernelSetElement(kel, 0, 1, -1.0);
		kernelSetElement(kel, 0, 2, -1.0);
	}

	image2 = pixMultMatrixColor(image, kel);
	pixDestroy(&image);
	image = image2;
	saveDebugImage(debugPixa, image, "multcolor");

	// now iterate for each bounding box defined:
	// till now we assume it's just one! (see Constructor)
	for (StdListBB::iterator it = _boxes.begin(); it != _boxes.end();
		 ++it) { // let's stick to begin not cbegin (c++11)
		BoundingBox &b = *it;
		int cx = b.x1 - _min_x;
		int cy = b.y1 - _min_y;
		int w = b.x2 - b.x1;
		int h = b.y2 - b.y1;

		if (w <= 0)
			w = 1;
		if (h <= 0)
			h = 1;

		unsigned long val = 0;
		static unsigned long maxVal = 0;
		for (int y = cy; y < h; ++y)
			for (int x = cx; x < w; ++x) {
				unsigned int c = 0;
				(void)pixGetPixel(image, cx, cy, &c);
				val += (c >> 24); // sum of all red pixels
			}

		// normalize val to avg pixel value (0-255):
		val /= (w * h);
		if (val > maxVal)
			maxVal = val;
		print(log_finest, "recognizerBinary detected maxval = %d val=%d!\n", "ocr", maxVal, val);

		// now do edge detection:
		bool new_state = _last_state;
		if (!_last_state) {
			// edge low->high?
			if (val > _EDGE_HIGH)
				new_state = true;
		} else {
			// edge high->low?
			if (val < _EDGE_LOW)
				new_state = false;
		}

		if (new_state != _last_state) {
			if (new_state) {
				// signal impulse:
				readings[b.identifier].value = 1;
				print(log_info, "recognizerBinary detected impulse maxval = %d val=%d!\n", "ocr",
					  maxVal, val);
			}
			_last_state = new_state;
		}
	}

	saveDebugImage(debugPixa, image, "scanned");
	pixDestroy(&image);

	return true;
}

int debounce(int iprev, double fnew) {
	// check old value. if current value is really close to it (+/-1), wait a bit (i.e. round later)
	// old: 0, new: 9,9 -> stick with 0,
	// old: 0, new: 9,5 -> change to 9
	// old: 9, new 0,1 -> stick with 9
	// old: 9, new 0,5 -> change to 0
	// old: 0, new 1,1 -> change to 0
	// old: 0, new 1,5 -> change to 1

	// wrap?
	int inew = (int)fnew; // no proper rounding. we want 0.99 to be 0.
	print(log_debug, "returning digit debounce check: prev_dig=%d nr=%d fnr=%f\n", "ocr", iprev,
		  inew, fnew);

	int nnr = inew;
	if (iprev == 0) {
		if (inew == 9 && (fnew > 9.5))
			nnr = 0;
		if (inew == 1 && (fnew < 1.5))
			nnr = 0;
	} else if (iprev == 9) {
		if (inew == 0 && (fnew < 0.5))
			nnr = 9;
		if (inew == 8 && (fnew >= 8.5))
			nnr = 9;
	} else {
		double fp = (double)iprev;
		if (fnew > fp) {
			if ((fnew - fp) < 1.5)
				nnr = iprev;
		} else {
			if ((fp - fnew) < 0.5)
				nnr = iprev;
		}
	}
	if (nnr != inew) {
		print(log_debug, "returning digit debounced: prev_dig=%d nr=%d fnr=%f\n -> to=%d", "ocr",
			  iprev, inew, fnew, nnr);
	}
	return nnr;
}

MeterOCR::RecognizerNeedle::~RecognizerNeedle() {}

MeterOCR::MeterOCR(const std::list<Option> &options)
	: Protocol("ocr"), _last_image(0), _use_v4l2(false), _v4l2_fps(5), _v4l2_skip_frames(0),
	  _v4l2_fd(-1), _v4l2_buffers(0), _v4l2_nbuffers(0), _v4l2_cap_size_x(320),
	  _v4l2_cap_size_y(240), _min_x(INT_MAX), _min_y(INT_MAX), _max_x(INT_MIN), _max_y(INT_MIN),
	  _notify_fd(-1), _forced_file_changed(true), _impulses(0), _rotate(0.0), _autofix_range(0),
	  _autofix_x(-1), _autofix_y(-1), _last_reads(0), _generate_debug_image(false) {
	OptionList optlist;

	try {
		_file = optlist.lookup_string(options, "v4l2_dev");
		_use_v4l2 = _file.length() > 0;
	} catch (vz::VZException &e) {
		// ignore
	}

	if (!_use_v4l2) {
		try {
			_file = optlist.lookup_string(options, "file");
		} catch (vz::VZException &e) {
			print(log_alert, "Missing image file name", name().c_str());
			throw;
		}
	}

	try {
		_generate_debug_image = optlist.lookup_bool(options, "generate_debug_image");
	} catch (vz::OptionNotFoundException &e) {
		// use default (off)
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse 'generate_debug_image'", name().c_str());
		throw;
	}

	try {
		_impulses = optlist.lookup_int(options, "impulses");
	} catch (vz::OptionNotFoundException &e) {
		// keep default
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse 'impulses'", name().c_str());
		throw;
	}

	try {
		_rotate = optlist.lookup_double(options, "rotate");
	} catch (vz::OptionNotFoundException &e) {
		// keep default (0.0)
	} catch (vz::InvalidTypeException &e) {
		print(log_alert, "Invalid type for 'rotate'", name().c_str());
		throw;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse 'rotate'", name().c_str());
		throw;
	}

	try {
		struct json_object *jso = optlist.lookup_json_object(options, "autofix");
		print(log_debug, "autofix=%s", name().c_str(),
			  jso ? json_object_to_json_string(jso) : "<null>");
		// range, x, y
		struct json_object *jv;
		if (json_object_object_get_ex(jso, "range", &jv)) {
			_autofix_range = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(jso, "x", &jv)) {
			_autofix_x = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(jso, "y", &jv)) {
			_autofix_y = json_object_get_int(jv);
		}
		if (_autofix_range < 1 || _autofix_x < _autofix_range ||
			_autofix_y < _autofix_range) { // all 3 values need to be valid if autofix specified
			throw vz::OptionNotFoundException("autofix range < 1 or x < range or y < range");
		}
	} catch (vz::OptionNotFoundException &e) {
		// autofix is optional (default none)
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse 'autofix'", name().c_str());
		throw;
	}

	try {
		struct json_object *jso = optlist.lookup_json_array(options, "recognizer");
		print(log_debug, "recognizer=%s", name().c_str(),
			  jso ? json_object_to_json_string(jso) : "<null>");
		int nrboxes;
		if (jso && (nrboxes = json_object_array_length(jso)) >= 1) {
			// for each object:
			for (int i = 0; i < nrboxes; i++) {
				struct json_object *jb = json_object_array_get_idx(jso, i);
				Recognizer *r = 0;

				// check type, default to tesseract
				std::string rtype("tesseract");
				struct json_object *value;
				if (json_object_object_get_ex(jb, "type", &value)) {
					rtype = json_object_get_string(value);
				}
#if OCR_TESSERACT_SUPPORT
				if (0 == rtype.compare("tesseract"))
					r = new RecognizerTesseract(jb);
				else
#endif
					if (0 == rtype.compare("needle"))
					r = new RecognizerNeedle(jb);
				if (0 == rtype.compare("binary"))
					r = new RecognizerBinary(jb);
				if (!r)
					throw vz::OptionNotFoundException("recognizer type unknown!");
				_recognizer.push_back(r);
				int minX, minY, maxX, maxY;
				r->getCaptureCoords(minX, minY, maxX, maxY);
				if (minX < _min_x)
					_min_x = minX;
				if (minY < _min_y)
					_min_y = minY;
				if (maxX > _max_x)
					_max_x = maxX;
				if (maxY > _max_y)
					_max_y = maxY;
			}
		} else {
			throw vz::OptionNotFoundException("no recognizer given");
		}
	} catch (vz::OptionNotFoundException &e) {
		// recognizer is mandatory
		// print(log_alert, "Config parameter 'recognizer' missing!", name().c_str());
		throw;
	} catch (vz::VZException &e) {
		print(log_alert, "Failed to parse 'recognizer'", name().c_str());
		throw;
	}
}

void MeterOCR::Recognizer::saveDebugImage(PIXA *debugPixa, PIX *image, const char *title) {
	if (debugPixa)
		pixSaveTiled(image, debugPixa, 1, 1, 1, 8);
	(void)title; // TODO p3 use pixSaveTiledWithText
}

MeterOCR::~MeterOCR() {
	if (_last_reads)
		delete _last_reads;
	if (_v4l2_buffers)
		free(_v4l2_buffers);
}

int MeterOCR::open() {
	// set notify on the file to see when it actually changed:
	if (_notify_fd != -1) {
		(void)::close(_notify_fd);
		_notify_fd = -1;
	}
	if (!_use_v4l2) {
		_notify_fd = inotify_init1(IN_NONBLOCK);
		if (_notify_fd != -1) {
			inotify_add_watch(_notify_fd, _file.c_str(),
							  IN_CLOSE_WRITE); // use IN_ONESHOT and retrigger after read?
		}

		// check  (open/close) file on each reading, so we check here just once
		FILE *_fd = fopen(_file.c_str(), "r");

		if (_fd == NULL) {
			print(log_error, "fopen(%s): %s", name().c_str(), _file.c_str(), strerror(errno));
			return ERR;
		}

		(void)fclose(_fd);
		_fd = NULL;
	} else { // v4l2 device:
		struct stat st;
		if (-1 == stat(_file.c_str(), &st)) {
			print(log_alert, "cannot identify '%s'", name().c_str(), _file.c_str());
			return ERR;
		}
		if (!S_ISCHR(st.st_mode)) {
			print(log_alert, "'%s' is no device", name().c_str(), _file.c_str());
			return ERR;
		}

		_v4l2_fd = ::open(_file.c_str(), O_RDWR | O_NONBLOCK, 0);

		if (-1 == _v4l2_fd) {
			print(log_alert, "cannot open '%s': %d , %s", name().c_str(), _file.c_str(), errno,
				  strerror(errno));
			return ERR;
		}

		if (!checkCapV4L2Dev()) {
			print(log_alert, "capabilities of '%s' not sufficient", name().c_str(), _file.c_str());
			::close(_v4l2_fd);
			_v4l2_fd = -1;
			return ERR;
		}

		if (!initV4L2Dev(_v4l2_cap_size_x, _v4l2_cap_size_y)) {
			print(log_alert, "couldn't init' '%s'", name().c_str(), _file.c_str());
			::close(_v4l2_fd);
			_v4l2_fd = -1;
			return ERR;
		}
	}

	return SUCCESS;
}

static int xioctl(int fh, int request, void *arg) {
	int r;
	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);
	return r;
}

bool MeterOCR::checkCapV4L2Dev() {
	if (_v4l2_fd <= -1)
		return false;
	struct v4l2_capability cap;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			print(log_alert, "'%s' is no V4L2 device", name().c_str(), _file.c_str());
		} else {
			print(log_error, "error %d, %s at VIDIOC_QUERYCAP", name().c_str(), errno,
				  strerror(errno));
		}
		return false;
	}
	print(log_info, "'%s' has capabilities: 0x%x", name().c_str(), _file.c_str(), cap.capabilities);
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		print(log_alert, "'%s' does not support V4L2_CAP_VIDEO_CAPTURE", name().c_str(),
			  _file.c_str());
		return false;
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		print(log_alert, "'%s' does not support V4L2_CAP_STREAMING", name().c_str(), _file.c_str());
		return false;
	}

	// check framerate:
	struct v4l2_streamparm streamparm;
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_G_PARM, &streamparm)) {
		print(log_alert, "error %d, %s at VIDIOC_G_PARM", name().c_str(), errno, strerror(errno));
		return false;
	}
	print(log_info, "g_parm: capability=%d, capturemode=%d, timeperframe = %d/%d, extendedmode=%d",
		  name().c_str(), streamparm.parm.capture.capability, streamparm.parm.capture.capturemode,
		  streamparm.parm.capture.timeperframe.numerator,
		  streamparm.parm.capture.timeperframe.denominator, streamparm.parm.capture.extendedmode);
	if (!(streamparm.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
		print(log_alert, "'%s' does not support V4L2_CAP_TIMEPERFRAME", name().c_str(),
			  _file.c_str());
		return false;
	}

	// check supported RGB32 format:
	int r;
	bool has_yuyv = false;
	int index = 0;
	do {
		struct v4l2_fmtdesc bt;
		memset(&bt, 0, sizeof(bt));
		bt.index = index++;
		bt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		r = xioctl(_v4l2_fd, VIDIOC_ENUM_FMT, &bt);
		if (r == 0) {
			if (bt.pixelformat == V4L2_PIX_FMT_YUYV)
				has_yuyv = true;
			print(log_info, " supported format: %s", name().c_str(), bt.description);
		}
	} while (r == 0);

	if (!has_yuyv) {
		print(log_alert, "'%s' does not support V4L2_PIX_FMT_YUYV", name().c_str(), _file.c_str());
		return false;
	}

	// todo check brightness,...

	return true;
}

bool MeterOCR::initV4L2Dev(unsigned int w, unsigned int h) {
	// set framerate:
	struct v4l2_streamparm streamparm;
	streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_G_PARM, &streamparm)) {
		print(log_alert, "error %d, %s at VIDIOC_G_PARM", name().c_str(), errno, strerror(errno));
		return false;
	}
	streamparm.parm.capture.capturemode = 0; // we don't need MODE_HIGHQUALITY
	streamparm.parm.capture.extendedmode = 0;
	streamparm.parm.capture.timeperframe.numerator = 1;
	streamparm.parm.capture.timeperframe.denominator = _v4l2_fps;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_S_PARM, &streamparm)) {
		print(log_alert, "error %d, %s at VIDIOC_S_PARM", name().c_str(), errno, strerror(errno));
		return false;
	}
	// read again to check:
	if (-1 == xioctl(_v4l2_fd, VIDIOC_G_PARM, &streamparm)) {
		print(log_alert, "error %d, %s at VIDIOC_G_PARM", name().c_str(), errno, strerror(errno));
		return false;
	}
	print(log_info, "set to timeperframe=%d/%d", name().c_str(),
		  streamparm.parm.capture.timeperframe.numerator,
		  streamparm.parm.capture.timeperframe.denominator);

	int set_fps = streamparm.parm.capture.timeperframe.denominator;
	if (streamparm.parm.capture.timeperframe.numerator > 1)
		set_fps /= streamparm.parm.capture.timeperframe.numerator;

	_v4l2_skip_frames = 0;
	if (set_fps > _v4l2_fps) {
		// determine whether we can simply skip a few frames:
		int skip = set_fps / _v4l2_fps;
		if (skip > 0) {
			_v4l2_skip_frames = skip - 1;
			print(log_info, "skipping %d frames", name().c_str(), _v4l2_skip_frames);
		}
	}

	struct v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = w;
	fmt.fmt.pix.height = h;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_S_FMT, &fmt)) {
		print(log_alert, "couldn't set VIDIOC_S_FMT %d, %s", name().c_str(), errno,
			  strerror(errno));
		return false;
	}

	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_G_FMT, &fmt)) {
		print(log_alert, "couldn't set VIDIOC_G_FMT %d, %s", name().c_str(), errno,
			  strerror(errno));
		return false;
	}
	print(log_info, "set to w=%d, h=%d, pixelformat=%x, bytesperline=%d, field=%d", name().c_str(),
		  fmt.fmt.pix.width, fmt.fmt.pix.height, fmt.fmt.pix.pixelformat, fmt.fmt.pix.bytesperline,
		  fmt.fmt.pix.field);
	if (w != fmt.fmt.pix.width || h != fmt.fmt.pix.height || fmt.fmt.pix.bytesperline != (w * 2) ||
		V4L2_FIELD_NONE != fmt.fmt.pix.field) {
		print(log_alert, "wrong fmt pix!", name().c_str());
		return false;
	}
	if (V4L2_PIX_FMT_YUYV != fmt.fmt.pix.pixelformat) {
		print(log_alert, "wrong pixelformat!", name().c_str());
		return false;
	}

	// reset VIDIOC_CROPCAP
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;

	memset(&cropcap, 0, sizeof(cropcap));
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (-1 == xioctl(_v4l2_fd, VIDIOC_CROPCAP, &cropcap)) {
		print(log_alert, "error %d, %s at VIDIOC_CROPCAP", name().c_str(), errno, strerror(errno));
		return false;
	}
	print(log_info, "cropcap.defrect=(%d,%d)x(%d,%d)", name().c_str(), cropcap.defrect.left,
		  cropcap.defrect.top, cropcap.defrect.width, cropcap.defrect.height);

	memset(&crop, 0, sizeof(crop));
	crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	crop.c = cropcap.defrect;

	bool supportscrop = true;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_S_CROP, &crop)) {
		supportscrop = false;
		print(log_alert, "cropping not supported. Error %d, %s at VIDIOC_S_CROP", name().c_str(),
			  errno, strerror(errno));
	}

	// now set cropping to wanted rectangle: todo
	if (supportscrop) {
		// todo
	}

	struct v4l2_requestbuffers req;

	memset(&req, 0, sizeof(req));

	req.count = 2;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(_v4l2_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			print(log_alert, "'%s'' does not support memory mapping", name().c_str(),
				  _file.c_str());
			return false;
		} else {
			print(log_alert, "couldn't set VIDIOC_S_FMT %d, %s", name().c_str(), errno,
				  strerror(errno));
			return false;
		}
	}

	if (req.count < 2) {
		print(log_alert, "Insufficient buffer memory on '%s'", name().c_str(), _file.c_str());
		return false;
	}
	if (_v4l2_buffers) {
		print(log_alert, "v4l2_buffers already init!", name().c_str());
		return false;
	}
	_v4l2_buffers = (MeterOCR::buffer *)calloc(req.count, sizeof(*_v4l2_buffers));

	if (!_v4l2_buffers) {
		print(log_alert, "Out of memory", name().c_str());
		return false;
	}

	for (_v4l2_nbuffers = 0; _v4l2_nbuffers < req.count; ++_v4l2_nbuffers) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = _v4l2_nbuffers;

		if (-1 == xioctl(_v4l2_fd, VIDIOC_QUERYBUF, &buf)) {
			return false; // release memory will be done in Destr.
		}

		_v4l2_buffers[_v4l2_nbuffers].length = buf.length;
		_v4l2_buffers[_v4l2_nbuffers].start =
			mmap(NULL /* start anywhere */, buf.length, PROT_READ | PROT_WRITE /* required */,
				 MAP_SHARED /* recommended */, _v4l2_fd, buf.m.offset);

		if (MAP_FAILED == _v4l2_buffers[_v4l2_nbuffers].start) {
			print(log_alert, "mmap failed", name().c_str());
			return false;
		}
	}

	// start capturing:
	for (unsigned int i = 0; i < _v4l2_nbuffers; ++i) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(buf));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == xioctl(_v4l2_fd, VIDIOC_QBUF, &buf)) {
			print(log_alert, "VIDIOC_QBUF failed", name().c_str());
			return false;
		}
	}
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(_v4l2_fd, VIDIOC_STREAMON, &type)) {
		print(log_alert, "VIDIOC_QBUF failed", name().c_str());
		return false;
	}

	return true;
}

/*
 * YUV422toRGB888 from
 * v4l2grab Version 0.1
 *   Copyright (C) 2009 by Tobias Mueller
 *   Tobias_Mueller@twam.info
 * licensed under gpl v2.
 * modified to handle RGBA and window coordinates and changed to float instead of double and changed
 * to little endianess
 * */

static void YUV422toRGBA888(int stride_s_w, int stride_s_h, int stride_d_w, int stride_d_h, int s_x,
							int s_y, int width, int height, unsigned char *src,
							unsigned char *dst) {
	int line, column;
	unsigned char *py, *pu, *pv;
	unsigned char *tmp = dst + (4 * ((stride_d_w * s_y) + s_x));

	unsigned char *ssrc = src + (2 * ((stride_s_w * s_y) + s_x));
	int skip_per_line_s = stride_s_w - width;
	int skip_per_line_d = stride_d_w - width;
	assert(skip_per_line_s % 1 == 0); // must be even!
	assert(s_x % 1 == 0);
	assert(width % 1 == 0);

	// In this format each four bytes is two pixels. Each four bytes is two Y's, a Cb and a Cr.
	// Each Y goes to one of the pixels, and the Cb and Cr belong to both pixels.
	py = ssrc;
	pu = ssrc + 1;
	pv = ssrc + 3;

#define CLIP(x) ((x) >= 0xFF ? 0xFF : ((x) <= 0x00 ? 0x00 : (x)))

	for (line = 0; line < height; ++line) {
		for (column = 0; column < width; ++column) {
			*tmp++ = 0; // alpha // for little endianess! TODO doesn't work on big endian! // todo
						// we are only interested in red anyway. Could ignore B and G.
			*tmp++ = CLIP((float)*py + 1.772 * ((float)*pu - 128.0)); // B
			*tmp++ =
				CLIP((float)*py - 0.344 * ((float)*pu - 128.0) - 0.714 * ((float)*pv - 128.0)); // G
			*tmp++ = CLIP((float)*py + 1.402 * ((float)*pv - 128.0));                           // R
			// increase py every time
			py += 2;
			// increase pu,pv every second time
			if ((column & 1) == 1) {
				pu += 4;
				pv += 4;
			}
		}
		// skip rest of stride_s_w:
		py += 2 * skip_per_line_s;
		pu += 2 * skip_per_line_s;
		pv += 2 * skip_per_line_s;
		tmp += 4 * skip_per_line_d;
	}
}

bool MeterOCR::readV4l2Frame(Pix *&image, bool first_time) {
	bool toRet = false;
	struct v4l2_buffer buf;
	memset(&buf, 0, sizeof(buf));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(_v4l2_fd, VIDIOC_DQBUF, &buf)) {
		switch (errno) {
		case EAGAIN:
			print(log_alert, "VIDIOC_DQBUF failed with EAGAIN", name().c_str());
			return false;

		case EIO:
			/* Could ignore EIO, see spec. */

			/* fall through */

		default:
			print(log_alert, "VIDIOC_DQBUF failed", name().c_str());
			return false;
		}
	}

	if (buf.index >= _v4l2_nbuffers) {
		print(log_alert, "buf.index >= nbuffers!", name().c_str());
		return false;
	}

	// now we have the image data in buffers[buf.index].start with len buf.bytesused
	print(log_finest, "buf.index=%d buf.bytesused=%d", name().c_str(), buf.index, buf.bytesused);

	if (!image) {
		// return buffer:
		if (-1 == xioctl(_v4l2_fd, VIDIOC_QBUF, &buf)) {
			print(log_alert, "VIDIOC_QBUF failed", name().c_str());
		}
		return false;
	}
	// copy into a Pix image (!would be better if we don't need to copy!)
	// check that the data is big enough:
	int32_t w, h, d;
	pixGetDimensions(image, &w, &h, &d);
	// image can be smaller than cap_size_x/y in this case render to
	// _min_x, _min_y,...

	if (buf.bytesused >= ((unsigned int)w * (unsigned int)h *
						  (unsigned int)(d / 16))) { // we expect half of the data we need
		// convert from yuyv(yuv2) to RGBA:
		if (first_time) {
			// if for the first time we convert the full picture and draw a rectangle around the
			// area to be searched:
			YUV422toRGBA888(_v4l2_cap_size_x, _v4l2_cap_size_y, w, h, 0, 0, w, h,
							(uint8_t *)(_v4l2_buffers[buf.index].start),
							(uint8_t *)pixGetData(image));
			// draw rectangle in green:
			BOX *box = boxCreate(_min_x - 1, _min_y - 1, _max_x - _min_x + 2, _max_y - _min_y + 2);
			pixRenderBoxArb(image, box, 1, 0, 0xff, 0);
			boxDestroy(&box);
		} else {
			// we only update the interesting rectangle
			YUV422toRGBA888(_v4l2_cap_size_x, _v4l2_cap_size_y, w, h, _min_x, _min_y,
							_max_x - _min_x, _max_y - _min_y,
							(uint8_t *)(_v4l2_buffers[buf.index].start),
							(uint8_t *)pixGetData(image));
		}
		toRet = true;
	}
	// return buffer:
	if (-1 == xioctl(_v4l2_fd, VIDIOC_QBUF, &buf)) {
		print(log_alert, "VIDIOC_QBUF failed", name().c_str());
		return false;
	}

	return toRet;
}

bool MeterOCR::isNotifiedFileChanged() {
	const int EVENTSIZE = sizeof(struct inotify_event) + NAME_MAX + 1;
	if (_notify_fd != -1) {
		// read all events from fd:
		char buf[EVENTSIZE * 5];
		bool changed = false;
		ssize_t len;
		int nr_events;
		do {
			nr_events = 0;
			len = ::read(_notify_fd, buf, sizeof(buf));
			const struct inotify_event *event = (struct inotify_event *)(&buf[0]);
			for (char *ptr = buf; ptr < buf + len;
				 ptr += sizeof(struct inotify_event) + event->len) {
				++nr_events;
				event = (const struct inotify_event *)ptr;
				print(log_debug, "got inotify_event %x", "ocr", event->mask);
				if (event->mask & IN_CLOSE_WRITE)
					changed = true; // anyhow continue reading all events
			}
		} while (len > 0 &&
				 nr_events >= 5); // if 5 events received there might be some more pending.
		return changed;
	}
	return true;
}

int MeterOCR::close() {
	if (_notify_fd != -1) {
		(void)::close(_notify_fd);
		_notify_fd = -1;
	}

	if (_v4l2_fd) {
		// stop capturing
		enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(_v4l2_fd, VIDIOC_STREAMOFF, &type))
			print(log_error, "Error VIDIOC_STREAMOFF", name().c_str());

		// unmap memory
		for (unsigned int i = 0; i < _v4l2_nbuffers; ++i) {
			if (-1 == munmap(_v4l2_buffers[i].start, _v4l2_buffers[i].length))
				print(log_error, "Error munmap", name().c_str());
		}

		(void)::close(_v4l2_fd);
		_v4l2_fd = -1;
	}

	if (_last_image) {
		pixDestroy(&_last_image);
	}

	return 0;
}

double radians(double d) { return d * M_PI / 180; }

ssize_t MeterOCR::read(std::vector<Reading> &rds, size_t max_reads) {

	unsigned int i = 0;
	std::string outfilename;
	std::string id;
	print(log_debug, "MeterOCR::read: %d, %d", name().c_str(), rds.size(), max_reads);

	if (max_reads < 1)
		return 0;

	Pix *image = 0;

	if (!_use_v4l2) {
		if (!isNotifiedFileChanged() && !_forced_file_changed)
			return 0;
		_forced_file_changed = false;

		// open image:
		image = pixRead(_file.c_str());
		if (!image) {
			print(log_debug, "pixRead returned NULL!", name().c_str());
			return 0;
		}
		int32_t w, h, d;
		pixGetDimensions(image, &w, &h, &d);
		print(log_info, "image = %d x %d with %d bits each pixel", name().c_str(), w, h, d);
	} else { // v4l2
		// capture an image:
		fd_set fds;
		struct timeval tv;

		FD_ZERO(&fds);
		FD_SET(_v4l2_fd, &fds);
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		int r;

		bool first_time = false;
		if (!_last_image) {
			if (_max_x > _v4l2_cap_size_x)
				_max_x = _v4l2_cap_size_x;
			if (_max_y > _v4l2_cap_size_y)
				_max_y = _v4l2_cap_size_y;
			if (_min_x < 0)
				_min_x = 0;
			if (_min_y < 0)
				_min_y = 0;
			if (_min_x > _max_x)
				_min_x = _max_x;
			if (_min_y > _max_y)
				_min_y = _max_y;

			first_time = true;
			_last_image = pixCreateNoInit(_v4l2_cap_size_x, _v4l2_cap_size_y, 32);
		}

		int skip = _v4l2_skip_frames + 1;
		do {
			do {
				r = select(_v4l2_fd + 1, &fds, NULL, NULL, &tv);
			} while (-1 == r && EINTR == errno);
			if (0 == r) {
				// timeout
				print(log_warning, "timeout!", name().c_str());
				return 0;
			}
			if (-1 == r) {
				print(log_error, "select returned %d, %s", name().c_str(), errno, strerror(errno));
				return 0;
			}
			if (skip == 1) {
				image = pixClone(_last_image);
				bool ok = readV4l2Frame(image, first_time);
				if (!ok) {
					pixDestroy(&image);
					return 0;
				}
			} else {
				Pix *im = 0;
				(void)readV4l2Frame(im, false);
			}
		} while (--skip);

		// read frame!
		print(log_finest, "frame ready!", name().c_str());
	}

	PIXA *debugPixa = _generate_debug_image ? pixaCreate(0) : 0;

	// rotate image if parameter set:
	if (fabs(_rotate) >= 0.1) {
		Pix *image_rot =
			pixRotate(image, radians(_rotate), L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, 0, 0);

		if (image_rot) {
			pixDestroy(&image);
			image = image_rot;
			image_rot = 0;
		}
		// for debugging to see the rotated picture:
		if (debugPixa)
			pixSaveTiled(image, debugPixa, 1, 0, 1, 32);
		// TODO p3 double check with pixFindSkew? (auto-rotate?)
	} else {
		// add a small version of the input image:
		if (debugPixa)
			pixSaveTiled(image, debugPixa, 1, 0, 1, 32);
	}

	// now do autofix detection:
	// this works by scanning for two edges and moving the image relatively so that the two edges
	// cross inside the (x/y) position We do this before cropping and later crop again towards the
	// detected center
	int autofix_dX = 0, autofix_dY = 0;
	if (_autofix_range > 0) {
		// TODO p2 add search direction. now we do from left to right and from bottom to top
		// TODO p2 add parameter for edge intensity/threshold
		autofixDetection(image, autofix_dX, autofix_dY, debugPixa);
	}

	ReadsMap *new_reads = new ReadsMap;
	if (!new_reads)
		return 0;
	ReadsMap &readings = *new_reads;
	// now call each recognizer and let them do their part:
	for (std::list<Recognizer *>::iterator it = _recognizer.begin(); it != _recognizer.end();
		 ++it) { // let's stick to begin not cbegin (c++11)
		if (*it)
			(*it)->recognize(image, autofix_dX, autofix_dY, readings, _last_reads, debugPixa);
	}

	if (debugPixa && pixaGetCount(debugPixa) > 0) {
		// output debugpix:
		PIX *pixt = pixaDisplay(debugPixa, 0, 0);
		std::string outfilename = _use_v4l2 ? std::string("/run/vzlogger") : _file;
		if (_use_v4l2) {
			static char nr = '1';
			outfilename.append(std::string(1, nr++));
			if (nr > '9')
				nr = '1';
		}
		outfilename.append("_debug.jpg");
		{
			FILE *fp = fopenWriteStream(outfilename.c_str(), "wb+");
			if (fp) {
				pixWriteStreamJpeg(fp, pixt, 100, 0); // 100, 0: 390ms (680kb)
				fclose(fp);
			} else
				print(log_alert, "couldn't open debug file", "ocr");
		}
		//		pixWrite(outfilename.c_str(), pixt, IFF_TIFF_LZW);
		// 331ms (140kb) but single pixels losts (e.g. center) IFF_JFIF_JPEG);
		// 501ms (6.9MB), IFF_BMP);
		// 450ms (2.1MB) IFF_TIFF_LZW);
		// 310ms fast but huge (6MB) IFF_TIFF);
		// 1080ms (1.1MB) IFF_PNG);
		pixDestroy(&pixt);
		pixaDestroy(&debugPixa);
	}

	bool wasNAN = false;
	// return all readings:
	if (!(_impulses && !_last_reads)) // first time read will not return any impulses/value
		for (std::map<std::string, Reads>::iterator it = readings.begin(); it != readings.end();
			 ++it) {
			const Reads &r = it->second;

			if (!_impulses)
				print(log_debug, "returning: id <%s> value <%f>", name().c_str(), it->first.c_str(),
					  r.value);
			if (!std::isnan(r.value)) {

				// if impulses wanted lets determine the delta:
				if (_impulses) {
					const Reads &old_r = (*_last_reads)[it->first];
					int imp = calcImpulses(r.value, old_r.value);
					print(log_debug, "returning: id <%s> impulses <%d> (abs value: %f)",
						  name().c_str(), it->first.c_str(), imp, r.value);
					rds[i].value(imp);
					if (imp < 0 && debugPixa) { // if debugImages wanted we add those here as well
						// write image for debugging:
						std::string name = _file;
						name.append("_debug_");
						std::ostringstream sstream;
						sstream << r.value;

						name.append(sstream.str());
						name.append(".jpg");
						{
							FILE *fp = fopenWriteStream(outfilename.c_str(), "wb+");
							if (fp) {
								pixWriteStreamJpeg(fp, image, 100, 0); // 100, 0: 390ms (680kb)
								fclose(fp);
							} else
								print(log_alert, "couldn't open debug file", "ocr");
						}
					}
				} else {
					rds[i].value(r.value);
				}
				rds[i].identifier(new StringIdentifier(it->first));
				rds[i].time();
				i++;
				if (i >= max_reads)
					break;
			} else
				wasNAN = true;
			if (r.conf_id.length() > 0) {
				rds[i].value(r.min_conf);
				rds[i].identifier(new StringIdentifier(r.conf_id));
				rds[i].time();
				i++;
				if (i >= max_reads)
					break;
			}
		}
	pixDestroy(&image);

	// we provide those values to the recognizers even if not impulses wanted
	if (_last_reads) {
		delete _last_reads;
		_last_reads = 0;
	}
	if (!wasNAN)
		_last_reads =
			new_reads; // we only want the last reading that was successful. If there are multiple
					   // ids some valid some not valid this will lead to problems (wrongly counted
					   // impulses but this should be a rare setup). TODO p2 describe in howto
					   // Another approach could be to pick each valid one and ignore the NAN ones.
					   // (needs a loop, could be added to aboves loop).

	return i;
}

int MeterOCR::calcImpulses(const double &value, const double &oldValue) const {
	int imp = lrint((value - oldValue) * _impulses);
	if (_impulses >= 10 && (abs(imp) > (_impulses / 2))) { // assume a wrap occured
		if (imp < 0)
			while (imp < 0)
				imp += _impulses; // (0.99 -> 0.01: -98 > 50 -> 2) ,
		else
			while (imp > 0)
				imp -= _impulses; // reverse:  (old 0.01 -> new 0.99: 98 > 50, -> -2)
	}
	return imp;
}

bool MeterOCR::autofixDetection(Pix *image_org, int &dX, int &dY, PIXA *debugPixa) {
	std::string outfilename;

	// now do autofix detection:
	// this works by scanning for two edges and moving the image relatively so that the two edges
	// cross inside the (x/y) position We do this before cropping and later crop again towards the
	// detected center
	if (_autofix_range > 0) {
		int w = 2 * _autofix_range + 1;
		// 1.step: crop the small detection area
		PIX *img = pixCreate(w, w, pixGetDepth(image_org));
		pixCopyResolution(img, image_org);
		pixCopyColormap(img, image_org);
		pixRasterop(img, 0, 0, w, w, PIX_SRC, image_org, _autofix_x - _autofix_range,
					_autofix_y - _autofix_range);
		// don't destroy image, we don't take the ownership

		// 2nd step: change to grayscale
		Pix *image_gs = pixConvertRGBToLuminance(img);
		if (image_gs) {
			pixDestroy(&img);
			if (debugPixa)
				pixSaveTiledWithText(image_gs, debugPixa, w, 1, 10, 1, 0, "autofix", 0xff000000,
									 L_ADD_BELOW);
		}

		// 3rd step pixTwoSidedEdgeFilter
		Pix *imgEdgeV = pixTwoSidedEdgeFilter(image_gs, L_VERTICAL_EDGES);
		Pix *imgEdgeH = pixTwoSidedEdgeFilter(image_gs, L_HORIZONTAL_EDGES);
		pixDestroy(&image_gs);
		if (!imgEdgeV || !imgEdgeH) {
			if (imgEdgeV)
				pixDestroy(&imgEdgeV);
			if (imgEdgeH)
				pixDestroy(&imgEdgeH);
			return false;
		}

		// 4th step: pixThresholdToBinary
		Pix *imgBinV = pixThresholdToBinary(imgEdgeV, 40);
		pixDestroy(&imgEdgeV);
		Pix *imgBinH = pixThresholdToBinary(imgEdgeH, 40);
		pixDestroy(&imgEdgeH);

		// 5th step: determine min for pixGetLastOffPixel
		int minOnX = w;
		int minOnY = -1;
		for (int i = 0; i < w; ++i) {
			int mX = w, mY = w;
			pixGetLastOnPixelInRun(imgBinV, 0, i, L_FROM_LEFT, &mX);
			pixGetLastOnPixelInRun(imgBinH, i, w - 1, L_FROM_BOT, &mY);
			if (mX < minOnX)
				minOnX = mX;
			if (mY > minOnY)
				minOnY = mY;
		}
		if (debugPixa)
			pixSaveTiledWithText(imgBinV, debugPixa, w, 1, 10, 1, 0, "autofix BinV", 0xff000000,
								 L_ADD_BELOW);
		if (debugPixa)
			pixSaveTiledWithText(imgBinH, debugPixa, w, 1, 10, 1, 0, "autofix BinH", 0xff000000,
								 L_ADD_BELOW);

		pixDestroy(&imgBinV);
		pixDestroy(&imgBinH);

		if (minOnX > 0 && minOnX < w && minOnY > 0 && minOnY < w) {
			dX = -_autofix_range + minOnX;
			dY = -_autofix_range + minOnY;
			print(log_debug, "autofixDetection: dX=%d, dY=%d\n", name().c_str(), dX, dY);
			return true;
		}
		print(log_alert, "autofixDetection: not found!\n", name().c_str());
	}
	return false;
}

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
 DE1020100530017 -> describing a different method to calculate the "Uebertrag". should not be affected
 EP1596164 -> using basically two different light sources. should not be affected
 DE102010053019 -> nice method to ease positioning sensors (see FAST EnergyCam). should not be affected.
 TODO p3 check some more especially on needle detection and not character recognition.
 
 my opinion: simple image recognition for numbers is "Stand der Technik"
*/


/*
	To-Do list: (p1 = mandatory before release, p2 = would be good, p3 = nice to have)
	TODO p3 add conf level that has to be achieved otherwise values will be ignored
	TODO p3 tesseract only: implement digit (just to add more conf by restricting the value to int -9...+9)
	TODO p3 use GetComponentImages interface (directly returning BOXA) and change to top/left/width/height?
	TODO p3 use filter from http://www.jofcis.com/publishedpapers/2011_7_6_1886_1892.pdf for binarization?
	TODO p3 check seedfill functions
	TODO p2 think about using either some trainingdata or e.g. http://www.unix-ag.uni-kl.de/~auerswal/ssocr/ to detect LCD digits.
	TODO p3 tesseract only: sanity check if digit used and recognized bb is far too small.
	TODO p2 check leptonicas 1.71 simple character recognition
	*/

#include <sys/inotify.h>
#include <unistd.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <sys/time.h>
#include <errno.h>
#include <cmath>
#include <climits>

#include "protocols/MeterOCR.hpp"
#include "Options.hpp"
#include <VZException.hpp>
#include <json-c/json.h>

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

MeterOCR::BoundingBox::BoundingBox(struct json_object *jb) :
	scaler(0), digit(false), boxType(BOX), x1(-1), y1(-1), x2(-1), y2(-1), cx(-1), cy(-1), cr(-1), offset(0.0),
	autocenter(true), ac_dx(0), ac_dy(0) // todo p2 make autocenter parameter default false
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
			if (x2<x1) throw vz::OptionNotFoundException("boundingbox x2 < x1");
		}
		if (json_object_object_get_ex(value, "y2", &jv)) {
			y2 = json_object_get_int(jv);
			if (y2<y1) throw vz::OptionNotFoundException("boundingbox y2 < y1");
		}
	} else
	if (json_object_object_get_ex(jb, "circle", &value)) {
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
			if (offset<-10.0 || offset>10.0)
				throw vz::VZException("offset invalid value <0 or >10");
		}
		if (cx<cr || cy<cr || cr<MIN_RADIUS)
			throw vz::OptionNotFoundException("circle cx < cr or cy < cr or cr<10");
	}
	
	print(log_debug, "boundingbox <%s>: conf_id=%s, scaler=%d, digit=%d, (%d,%d)-(%d,%d)\n", "ocr",
		identifier.c_str(), conf_id.c_str(), scaler, digit? 1 : 0, x1, y1, x2, y2);
}

// comparison
// returns true if the first argument goes before the second argument in the strict weak ordering it defines, and false otherwise.
bool MeterOCR::BoundingBox::compare (const MeterOCR::BoundingBox& first, const MeterOCR::BoundingBox& second)
{
  return ( first.scaler < second.scaler );
}

MeterOCR::Recognizer::Recognizer(const std::string &str_type, struct json_object *jr) :
	_type(str_type)
{
	struct json_object *value;
	try {
		if (json_object_object_get_ex(jr, "boundingboxes", &value)) {
			print(log_debug, "boundingboxes=%s", "ocr", value ? json_object_to_json_string(value) : "<null>");
			int nrboxes;
			if (value && (nrboxes=json_object_array_length(value))>=1) {
				// for each object:
				for (int i = 0; i < nrboxes; i++) {
					struct json_object *jb = json_object_array_get_idx(value, i);
					_boxes.push_back(BoundingBox(jb));
				}
				_boxes.sort(BoundingBox::compare); // we sort by smallest scaler first.
			}else{
				throw vz::OptionNotFoundException("empty boundingboxes given");
			}
		}else{
			throw vz::OptionNotFoundException("no boundingboxes given");
		}		
	} catch (vz::OptionNotFoundException &e) {
		// boundingboxes is mandatory
		throw;
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse 'boundingboxes'", "ocr");
		throw;
	}
}

MeterOCR::RecognizerNeedle::RecognizerNeedle(struct json_object *jr) :
	Recognizer("needle", jr),
	_min_x(INT_MAX), _min_y(INT_MAX), _max_x(INT_MIN), _max_y(INT_MIN)
{
	// check for _kernelColorString
	struct json_object *value;
	if (json_object_object_get_ex(jr, "kernelColorString", &value)) {
		_kernelColorString = json_object_get_string(value);
		if (_kernelColorString.length())
			_kernelColorString.append(" "); // append a space to ease kernelCreateFromString
	}
	// check whether all are circles:
	for (StdListBB::iterator it= _boxes.begin(); it!= _boxes.end(); ++it){
		const BoundingBox &b = *it;
		if (b.boxType != BoundingBox::CIRCLE) throw vz::OptionNotFoundException("boundingbox without circle");
		int r = b.cr; // we keep a border for autocenter
		if (b.autocenter) r+= b.cr;
		if (b.cx-r < _min_x) _min_x=b.cx-r;
		if (b.cy-r < _min_y) _min_y=b.cy-r;
		if (b.cx+r > _max_x) _max_x = b.cx+r;
		if (b.cy+r > _max_y) _max_y = b.cy+r;
	}
}

bool MeterOCR::RecognizerNeedle::recognize(PIX *imageO, int dX, int dY, ReadsMap &readings, const ReadsMap *old_readings, PIXA *debugPixa )
{
	if (!imageO) return false;
	// 1st step: crop the image:
	PIX *image=pixClone(imageO);

	// now crop the image if possible:
	print(log_debug, "Cropping image to (%d,%d)-(%d,%d)", "ocr", _min_x, _min_y, _max_x, _max_y);
	PIX *image2 = pixCreate(_max_x-_min_x, _max_y - _min_y, pixGetDepth(image));
	pixCopyResolution(image2, image);
	pixCopyColormap(image2, image);
	pixRasterop(image2, 0, 0, _max_x-_min_x, _max_y - _min_y, PIX_SRC, image, _min_x+dX, _min_y+dY);
	pixDestroy(&image);
	image=image2;
	saveDebugImage(debugPixa, image, "cropped");

	// now filter on red color either using provided matrix or std. internally the needles have to be red.
	L_KERNEL *kel;
	if (_kernelColorString.length())
		kel = kernelCreateFromString(3, 3, 0, 0, _kernelColorString.c_str());
	else { // use default: only red channel amplified
		kel = kernelCreate(3, 3);
		kernelSetElement(kel, 0, 0, 2.0);
		kernelSetElement(kel, 0, 1, -2.0);
		kernelSetElement(kel, 0, 2, -2.0);
	}

	image2 = pixMultMatrixColor(image, kel);	
	pixDestroy(&image);
	image=image2;
	saveDebugImage(debugPixa, image, "multcolor");


	// now iterate for each bounding box defined:
	for (StdListBB::iterator it = _boxes.begin(); it != _boxes.end(); ++it){ // let's stick to begin not cbegin (c++11)
		BoundingBox &b = *it;
		int autocenter_redocnt=0;
		bool autocenter_redo;
		int conf;
		int degFrom, degTo, degAvg;
		do {
			autocenter_redo=false;
			int cx = b.cx + b.ac_dx - _min_x;
			int cy = b.cy + b.ac_dy - _min_y;
			conf=100;
			if (b.autocenter) {
				// verify that the center is red:
				unsigned int c=0;
				(void)pixGetPixel(image, cx, cy, &c);
				if (c<RED_COLOR_LIMIT){
					conf=0;
					print(log_error, "recognizerNeedle center not red!\n", "ocr");
					break;
				}
			} else {
				// verify that the center and one pix to each side is red:
				for (int x=cx-1; conf>0 && x<=cx+1; ++x)
					for (int y = cy-1; y<=cy+1; ++y) {
						unsigned int c=0;
						(void)pixGetPixel(image, x, y, &c);
						if (c<RED_COLOR_LIMIT){
							conf=0;
							print(log_error, "recognizerNeedle center not red!\n", "ocr");
							break;
						}
					}
			}
			degFrom=-1;
			degTo=-1;
			degAvg=-1;

			if (conf>0){
				// draw the center: in green
				pixSetPixel(image, cx, cy, 0x00ff0000);

				// now scan the circle around, draw for debugging blue = not detected, red = detected:
				const float  PI_F=3.14159265358979f;
				const float PIrad = PI_F / 180;
				int opx=-1, opy=-1;
				bool wrap=false;

				for (int deg=0; deg<360; ++deg) {
					int px = cx+b.cr*sin(deg * PIrad);
					int py = cy-b.cr*cos(deg * PIrad);
					if (opx!=px || opy!=py){ // don't scan the same pixel twice
						unsigned int c=0;
						(void)pixGetPixel(image, px, py, &c);
						if (c>RED_COLOR_LIMIT) {
							if (deg==0) wrap=true;

							if (wrap) { // needle around 0 deg (e.g. 355-5) -> degTo:5, degFrom:-5 TODO p2 add unit test for this case!
								if (deg<180) degTo=deg; else
								if ((deg-360)<degFrom)degFrom=deg-360;
							} else {
								if (degFrom<0) degFrom=deg;
								degTo=deg;
							}

							pixSetPixel(image, px, py, 0xff000000); // draw in red so it can be detected next time as well
						} else
							pixSetPixel(image, px, py, 0x0000ff00);
						opx=px;
						opy=py;
					}
				}
				degAvg = (degFrom+degTo)/2;
				if (degTo<0) conf=0; else
				{
					if (b.autocenter){
						// autocenter: check distance from center at the detected
						// needle +90/+180/+270 degrees to the circle
						// adjust ac_dx/ac_dy so that the distances are equal:
						// if ac_dx/ac_dy changes >+/-1 then redo calc a few times:
						int det_r[3];
						int ind=0;
						for (int deg = degAvg+90; deg < degAvg+360; deg+=90){
							int opx=-1 ,opy=-1;
							int r;
							for (r=b.cr-1; r>0; --r){
								int px = cx+r*sin(deg * PIrad);
								int py = cy-r*cos(deg * PIrad);
								if (px!=opx || py!=opy) {
									unsigned int c=0;
									(void)pixGetPixel(image, px, py, &c);
									if (c>RED_COLOR_LIMIT) {
										break;
									}
									opx=px;
									opy=py;
								}
							}
							if (r<=0) break;
							det_r[ind++] = r;
							print(log_debug, "scanning at %d: r=%d", "ocr", deg, r);
						}
						if (ind<3){
							print(log_error, "couldn't autocenter!", "ocr");
						}else{
							// now calculate the new center offset:
							float sh_r = (det_r[0] + det_r[2]) / 2.0;
							// adjust due to line r0---r2:
							// to move center to direction 0deg: decrease cy
							// to move center to direction 90deg: increase cx
							// to move center to direction 180 deg: increase cy
							// to move center to direction 270deg: decrease cx
							// if det_r0 > sh_r we need to move center in direction deg+90:
							float ndx = (det_r[0]-sh_r)*sin((degAvg+90) * PIrad);
							float ndy = (-(det_r[0]-sh_r))*cos((degAvg+90) * PIrad);
							print(log_debug, "ndx=%f ndy=%f", "ocr", ndx, ndy);
							// adjust due to r1:
							// if det_r1 > sh_r we need to move center in dir deg+180
							ndx += ((det_r[1]-sh_r)*sin((degAvg+180) * PIrad));
							ndy += (-(det_r[1]-sh_r)*cos((degAvg+180) * PIrad));
							print(log_debug, "ndx=%f ndy=%f", "ocr", ndx, ndy);
							// draw the center: in white
							pixSetPixel(image, cx+lrintf(ndx), cy+lrintf(ndy), 0xffffff00);
							if (fabsf(ndx)>1.0 || fabsf(ndy)>1.0){
								b.ac_dx += lrintf(ndx); // round properly here
								b.ac_dy += lrintf(ndy);
								++autocenter_redocnt;
								autocenter_redo = true;
							}
						}
					}
				}
			}
		} while (autocenter_redo && autocenter_redocnt<3);
		if (b.conf_id.length()) readings[b.identifier].conf_id=b.conf_id;
		if (conf) {
			int nr=b.offset + (degAvg/36);
			if (nr < 0) nr += 10;
			if (nr > 9) nr -= 10;
			double fnr = b.offset + ((double)(degFrom+degTo)/72);
			if (fnr < 0) fnr += 10.0;
			if (fnr > 10.0) fnr -= 10.0;

			// the boundingboxes are sorted by smallest scaler first. there might be a value already that we can use to check
			// whether we should round up already (round up -> smaller digit <0.5, round down -> smaller digit >0.5)
			// we take this only into account if b.digit is not true: (so we can mark the smallest digit with true)
			if (!b.digit){
				double ip;
				double prev_dig = modf( readings[b.identifier].value / pow(10, b.scaler), &ip); // prev. digit as 0.x
				nr = roundBasedOnSmallerDigits(nr, fnr, prev_dig, conf);
			}else{
				if (old_readings){
					// try to debounce if old value is available:
					ReadsMap::const_iterator it = (*old_readings).find(b.identifier);
					if (it!=old_readings->end()){
						double ip;
						int prev_dig = lrint(modf( (*it).second.value/ pow(10, b.scaler+1), &ip)*10); // prev digit as 0.x
						nr = debounce(prev_dig, fnr);						
					}
				}
			}
			readings[b.identifier].value += (double)nr * pow(10, b.scaler);
			if (conf<readings[b.identifier].min_conf)
				readings[b.identifier].min_conf = conf; // TODO p2 (by ratio detected pixel vs. non detected vs. abs(degFrom-degTo)
		}else{
			readings[b.identifier].value = NAN;
			readings[b.identifier].min_conf = 0;			
		}
	}

	saveDebugImage(debugPixa, image, "scanned");
	pixDestroy(&image);		
	
	return true;
}

int MeterOCR::RecognizerNeedle::roundBasedOnSmallerDigits(const int curNr, const double &fnr, const double &smaller, int &conf) const
{
	int nr = curNr;
	if (smaller < 0.5){
		int nnr = (fnr+0.5-smaller); // e.g. 8.1 detected as 0.1 and 7.99 -> nr = (7,99+0,5-0.1)=8.39 = 8
		if (nnr!=nr){
			nr=nnr;
			if (nr>=10) nr=0;
			conf-=10;
			print(log_debug, "returning rounded up: smaller=%f nr=%d fnr=%f\n", "ocr", smaller, nr, fnr);
		}
	}
	if (smaller >=0.5){ // e.g. 3.9 detected as 0.9 and 4.1 -> nr = (4.1-0.2) = 3.9 = 3
		int nnr = fnr<(smaller-0.5) ? 9 : (fnr-smaller+0.5);
		if (nnr!=nr){
			nr=nnr;
			conf -=15;
			print(log_debug, "returning rounded down: smaller=%f nr=%d fnr=%f\n", "ocr", smaller, nnr, fnr);
		}
	}
	return nr;
}

int debounce(int iprev, double fnew)
{
	// check old value. if current value is really close to it (+/-1), wait a bit (i.e. round later)
	// old: 0, new: 9,9 -> stick with 0,
	// old: 0, new: 9,5 -> change to 9 
	// old: 9, new 0,1 -> stick with 9
	// old: 9, new 0,5 -> change to 0
	// old: 0, new 1,1 -> change to 0
	// old: 0, new 1,5 -> change to 1

	// wrap?
	int inew = (int) fnew; // no proper rounding. we want 0.99 to be 0.
	print(log_debug, "returning digit debounce check: prev_dig=%d nr=%d fnr=%f\n", "ocr", iprev,  inew, fnew);

	int nnr=inew;
	if (iprev == 0 ) {
		if (inew==9 && (fnew>9.5))	nnr = 0;
		if (inew==1 && (fnew<1.5)) nnr = 0;
	} else
	if (iprev == 9 ) {
		if (inew == 0 && (fnew<0.5)) nnr = 9;
		if (inew == 8 && (fnew>=8.5)) nnr = 9;
	} else {
		double fp = (double) iprev;
		if (fnew>fp) {
			if ((fnew-fp)<1.5) nnr = iprev;
		} else {
			if ((fp-fnew)<0.5) nnr = iprev;
		}
	}
	if (nnr!=inew) {
		print(log_debug, "returning digit debounced: prev_dig=%d nr=%d fnr=%f\n -> to=%d", "ocr", iprev, inew, fnew, nnr);
	}
	return nnr;
} 

MeterOCR::RecognizerNeedle::~RecognizerNeedle()
{
}

MeterOCR::MeterOCR(std::list<Option> options)
        : Protocol("ocr"), _notify_fd(-1), _forced_file_changed(true), _impulses(0), _rotate(0.0),
        _autofix_range(0), _autofix_x(-1), _autofix_y(-1), _last_reads(0), _generate_debug_image(false)
{
	OptionList optlist;

	try {
		_file = optlist.lookup_string(options, "file");
	} catch (vz::VZException &e) {
		print(log_error, "Missing image file name", name().c_str());
		throw;
	}
	
	try {
		_generate_debug_image = optlist.lookup_bool(options, "generate_debug_image");
	} catch (vz::OptionNotFoundException &e) {
		// use default (off)
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse 'generate_debug_image'", name().c_str());
		throw;
	}
	
	try {
		_impulses = optlist.lookup_int(options, "impulses");
	} catch (vz::OptionNotFoundException &e) {
		// keep default
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse 'impulses'", name().c_str());
		throw;
	}
	
	try {
		_rotate = optlist.lookup_double(options, "rotate");
	} catch (vz::OptionNotFoundException &e) {
		// keep default (0.0)
	} catch (vz::InvalidTypeException &e) {
		print(log_error, "Invalid type for 'rotate'", name().c_str());
		throw;
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse 'rotate'", name().c_str());
		throw;
	}

	try {
		struct json_object *jso = optlist.lookup_json_object(options, "autofix");
		print(log_debug, "autofix=%s", name().c_str(), jso ? json_object_to_json_string(jso) : "<null>");
		// range, x, y
		struct json_object *jv;
		if (json_object_object_get_ex(jso, "range", &jv)){
			_autofix_range = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(jso, "x", &jv)){
			_autofix_x = json_object_get_int(jv);
		}
		if (json_object_object_get_ex(jso, "y", &jv)){
			_autofix_y = json_object_get_int(jv);
		}
		if (_autofix_range <1 || _autofix_x<_autofix_range || _autofix_y<_autofix_range ){ // all 3 values need to be valid if autofix specified
			throw vz::OptionNotFoundException("autofix range < 1 or x < range or y < range");
		}
	} catch (vz::OptionNotFoundException &e) {
		// autofix is optional (default none)
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse 'autofix'", name().c_str());
		throw;
	}
	
	try {
		struct json_object *jso = optlist.lookup_json_array(options, "recognizer");
		print(log_debug, "recognizer=%s", name().c_str(), jso ? json_object_to_json_string(jso) : "<null>");
		int nrboxes;
		if (jso && (nrboxes=json_object_array_length(jso))>=1){
			// for each object:
			for (int i = 0; i < nrboxes; i++) {
				struct json_object *jb = json_object_array_get_idx(jso, i);
				Recognizer *r=0;

				// check type, default to tesseract
				std::string rtype("tesseract");
				struct json_object *value;
				if (json_object_object_get_ex(jb, "type", &value)){
					rtype = json_object_get_string(value);
				}
#if OCR_TESSERACT_SUPPORT
				if (0 == rtype.compare("tesseract")) r = new RecognizerTesseract(jb); else
#endif
				if (0 == rtype.compare("needle")) r = new RecognizerNeedle(jb);
				if (!r) throw vz::OptionNotFoundException("recognizer type unknown!");
				_recognizer.push_back(r);
			}
		} else {
			throw vz::OptionNotFoundException("no recognizer given");
		}
	} catch (vz::OptionNotFoundException &e) {
		// recognizer is mandatory
		//print(log_error, "Config parameter 'recognizer' missing!", name().c_str());
		throw;
	} catch (vz::VZException &e) {
		print(log_error, "Failed to parse 'recognizer'", name().c_str());
		throw;
	}

}

void MeterOCR::Recognizer::saveDebugImage(PIXA *debugPixa, PIX *image, const char *title)
{
	if (debugPixa) pixSaveTiled(image, debugPixa, 1, 1, 1, 8);
	(void) title; // TODO p3 use pixSaveTiledWithText
}


MeterOCR::~MeterOCR() {	
	if (_last_reads) delete _last_reads;
}

int MeterOCR::open() {
    // set notify on the file to see when it actually changed:
    if (_notify_fd!=-1) {
        (void)::close(_notify_fd);
        _notify_fd = -1;
   }
    _notify_fd = inotify_init1(IN_NONBLOCK);
    if (_notify_fd != -1) {
        inotify_add_watch(_notify_fd, _file.c_str(), IN_CLOSE_WRITE); // use IN_ONESHOT and retrigger after read?
    }

	// check  (open/close) file on each reading, so we check here just once
	FILE *_fd = fopen(_file.c_str(), "r");

	if (_fd == NULL) {
		print(log_error, "fopen(%s): %s", name().c_str(), _file.c_str(), strerror(errno));
		return ERR;
	}

	(void) fclose(_fd);
	_fd = NULL;

	return SUCCESS;
}

bool MeterOCR::isNotifiedFileChanged()
{
    const int EVENTSIZE = sizeof(struct inotify_event) + NAME_MAX + 1;
    if (_notify_fd!=-1){
        // read all events from fd:
        char buf[EVENTSIZE *5];
        bool changed=false;
        ssize_t len;
        int nr_events;
        do{
            nr_events = 0;
            len = ::read(_notify_fd, buf, sizeof(buf));
            const struct inotify_event *event = (struct inotify_event *)(&buf[0]);
            for (char *ptr = buf; ptr < buf + len;
                    ptr += sizeof(struct inotify_event) + event->len) {
                ++nr_events;
                event = (const struct inotify_event *) ptr;
                print(log_debug, "got inotify_event %x", "ocr", event->mask);
                if (event->mask & IN_CLOSE_WRITE) changed=true; // anyhow continue reading all events
            }
        } while(len>0 && nr_events>=5); // if 5 events received there might be some more pending.
        return changed;
    }
    return false; // or true if notify failed?
}

int MeterOCR::close() {
    if (_notify_fd != -1) {
        (void)::close(_notify_fd);
        _notify_fd = -1;
    }
	return 0;
}

double radians (double d) { 
return d * M_PI / 180; 
} 



ssize_t MeterOCR::read(std::vector<Reading> &rds, size_t max_reads) {

	unsigned int i = 0;
	std::string outfilename;
	std::string id;
	print(log_debug, "MeterOCR::read: %d, %d", name().c_str(), rds.size(), max_reads);

	if (max_reads<1) return 0;
	
    if (!isNotifiedFileChanged() && !_forced_file_changed) return 0;
    _forced_file_changed = false;

	PIXA *debugPixa= _generate_debug_image ? pixaCreate(0) : 0;
	
	// open image:
	Pix *image = pixRead(_file.c_str());
	if (!image){
		print(log_debug, "pixRead returned NULL!", name().c_str());
		return 0;
	}

	// rotate image if parameter set:
	if (fabs(_rotate)>=0.1){
		Pix *image_rot = pixRotate(image, radians(_rotate), L_ROTATE_AREA_MAP, L_BRING_IN_WHITE, 0,0);
	
		if (image_rot){
			pixDestroy(&image);
			image = image_rot;
			image_rot=0;
		}
		// for debugging to see the rotated picture:
		if (debugPixa) pixSaveTiled(image, debugPixa, 1, 0, 1, 32);
		// TODO p3 double check with pixFindSkew? (auto-rotate?)
	}else{
		// add a small version of the input image:
		if (debugPixa) pixSaveTiled(image, debugPixa, 1, 0, 1, 32);
	}

	// now do autofix detection:
	// this works by scanning for two edges and moving the image relatively so that the two edges cross inside the (x/y) position
	// We do this before cropping
	// and later crop again towards the detected center
	int autofix_dX=0, autofix_dY=0;
	if (_autofix_range>0){
		// TODO p2 add search direction. now we do from left to right and from bottom to top
		// TODO p2 add parameter for edge intensity/threshold
		autofixDetection(image, autofix_dX, autofix_dY, debugPixa);
	}

	ReadsMap *new_reads = new ReadsMap;
	if (!new_reads) return 0;
	ReadsMap &readings = *new_reads;
	// now call each recognizer and let them do their part:
	for (std::list<Recognizer*>::iterator it = _recognizer.begin(); it != _recognizer.end(); ++it){ // let's stick to begin not cbegin (c++11)
		if (*it)
			(*it)->recognize(image, autofix_dX, autofix_dY, readings, _last_reads, debugPixa);
	}
	
	if (debugPixa && pixaGetCount(debugPixa)>0){
		// output debugpix:
		PIX *pixt = pixaDisplay(debugPixa, 0, 0);
		std::string outfilename = _file;
		outfilename.append("_debug.jpg");
		{
			FILE *fp = fopenWriteStream(outfilename.c_str(), "wb+");
			if (fp) {
				pixWriteStreamJpeg(fp, pixt, 100, 0); // 100, 0: 390ms (680kb)
				fclose(fp);
			}else
				print(log_error, "couldn't open debug file", "ocr");

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

	bool wasNAN=false;
	// return all readings:
	if (!(_impulses && !_last_reads))  // first time read will not return any impulses/value
		for(std::map<std::string, Reads>::iterator it = readings.begin(); it!= readings.end(); ++it){
			const Reads &r=it->second;
		
			if (!_impulses)
				print(log_debug, "returning: id <%s> value <%f>", name().c_str(), it->first.c_str(), r.value);
			if (!isnan(r.value)){
			
				// if impulses wanted lets determine the delta:
				if (_impulses){
					const Reads &old_r = (*_last_reads)[it->first];
					int imp = calcImpulses(r.value, old_r.value);
					print(log_debug, "returning: id <%s> impulses <%d> (abs value: %f)", name().c_str(), it->first.c_str(), imp, r.value);
					rds[i].value(imp);
					if (imp<0 && debugPixa) { // if debugImages wanted we add those here as well
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
							}else
								print(log_error, "couldn't open debug file", "ocr");

						}
					}
				} else {
					rds[i].value(r.value);
				}
				rds[i].identifier(new StringIdentifier(it->first));
				rds[i].time();
				i++;
				if (i>=max_reads) break;
			} else wasNAN = true;
			if (r.conf_id.length()>0){
				rds[i].value(r.min_conf);
				rds[i].identifier(new StringIdentifier(r.conf_id));
				rds[i].time();
				i++;
				if (i>=max_reads) break;
			}
		}

	pixDestroy(&image);

	// we provide those values to the recognizers even if not impulses wanted
	if (_last_reads) { delete _last_reads; _last_reads = 0; }
	if (!wasNAN) _last_reads = new_reads; // we only want the last reading that was successful. If there are multiple ids some valid some not valid this will lead to problems (wrongly counted impulses but this should be a rare setup). TODO p2 describe in howto Another approach could be to pick each valid one and ignore the NAN ones. (needs a loop, could be added to aboves loop).

	return i;
}

int MeterOCR::calcImpulses( const double &value, const double &oldValue) const
{
	int imp =
		lrint((value - oldValue) * _impulses);
	if (_impulses >=10 && (abs(imp) > (_impulses/2))) { // assume a wrap occured
		if (imp<0)
			while(imp<0) imp += _impulses; // (0.99 -> 0.01: -98 > 50 -> 2) ,
		else
			while(imp>0)imp -= _impulses; // reverse:  (old 0.01 -> new 0.99: 98 > 50, -> -2)
	}
	return imp;
}

bool MeterOCR::autofixDetection(Pix *image_org, int &dX, int&dY, PIXA *debugPixa)
{
	std::string outfilename;

	// now do autofix detection:
	// this works by scanning for two edges and moving the image relatively so that the two edges cross inside the (x/y) position
	// We do this before cropping
	// and later crop again towards the detected center
	if (_autofix_range>0){		
		int w = 2*_autofix_range+1;
		// 1.step: crop the small detection area
		PIX *img = pixCreate(w, w, pixGetDepth(image_org));
		pixCopyResolution(img, image_org);
		pixCopyColormap(img, image_org);
		pixRasterop(img, 0, 0, w, w, PIX_SRC, image_org, _autofix_x-_autofix_range, _autofix_y-_autofix_range);		
		// don't destroy image, we don't take the ownership
		
		// 2nd step: change to grayscale
		Pix *image_gs = pixConvertRGBToLuminance(img);
		if (image_gs){
			pixDestroy(&img);
			if (debugPixa) pixSaveTiledWithText(image_gs, debugPixa, w, 1, 10, 1, 0, "autofix", 0xff000000, L_ADD_BELOW);
		}
		
		// 3rd step pixTwoSidedEdgeFilter
		Pix *imgEdgeV = pixTwoSidedEdgeFilter(image_gs, L_VERTICAL_EDGES);
		Pix *imgEdgeH = pixTwoSidedEdgeFilter(image_gs, L_HORIZONTAL_EDGES);
		pixDestroy(&image_gs);
		if (!imgEdgeV || !imgEdgeH) {
			if (imgEdgeV) pixDestroy(&imgEdgeV);
			if (imgEdgeH) pixDestroy(&imgEdgeH);
			return false;
		}

		// 4th step: pixThresholdToBinary
		Pix *imgBinV = pixThresholdToBinary(imgEdgeV, 40);
		pixDestroy(&imgEdgeV);
		Pix *imgBinH = pixThresholdToBinary(imgEdgeH, 40);
		pixDestroy(&imgEdgeH);
		
		// 5th step: determine min for pixGetLastOffPixel
		int minOnX = w; int minOnY = -1;
		for (int i=0; i<w; ++i){
			int mX=w, mY=w;
			pixGetLastOnPixelInRun(imgBinV, 0, i, L_FROM_LEFT, &mX);
			pixGetLastOnPixelInRun(imgBinH, i, w-1, L_FROM_BOT, &mY);			
			if (mX<minOnX) minOnX=mX;
			if (mY>minOnY) minOnY=mY;
		}
		if (debugPixa) pixSaveTiledWithText(imgBinV, debugPixa, w, 1, 10, 1, 0, "autofix BinV", 0xff000000, L_ADD_BELOW);
		if (debugPixa) pixSaveTiledWithText(imgBinH, debugPixa, w, 1, 10, 1, 0, "autofix BinH", 0xff000000, L_ADD_BELOW);

		pixDestroy(&imgBinV);
		pixDestroy(&imgBinH);
		
		if (minOnX>0 && minOnX<w && minOnY>0 && minOnY<w){
			dX = - _autofix_range + minOnX;
			dY = - _autofix_range + minOnY;
			print(log_debug, "autofixDetection: dX=%d, dY=%d\n", name().c_str(), dX, dY);
			return true;
		}
		print(log_error, "autofixDetection: not found!\n", name().c_str());
	}
	return false;
}


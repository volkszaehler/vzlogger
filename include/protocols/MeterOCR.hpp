/**
 *  Read data from captured images via text recognition using
 * tesseract-ocr.
 *
 * @package vzlogger
 * @copyright Copyright (c) 2015 - 2023, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Matthias Behr <mbehr (@) mcbehr.de>
 */
/*
 * This file is part of volkzaehler.org
 *
 * volkzaehler.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * volkzaehler.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with volkszaehler.org. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MeterOCR_H_
#define _MeterOCR_H_

#include <cfloat>
#include <map>
#include <protocols/Protocol.hpp>
#include <stdio.h>

#if OCR_TESSERACT_SUPPORT
namespace tesseract {
class TessBaseAPI;
}
#endif

typedef struct Pix PIX;
typedef struct Pixa PIXA;

class Reads {
  public:
	Reads() : value(0.0), min_conf(DBL_MAX){};
	double value;
	std::string conf_id;
	double min_conf;
};

typedef std::map<std::string, Reads> ReadsMap;

extern int debounce(int iprev, double fnew);

class MeterOCR : public vz::protocol::Protocol {

  public:
	MeterOCR(const std::list<Option> &options);
	virtual ~MeterOCR();

	int open();
	int close();
	ssize_t read(std::vector<Reading> &rds, size_t n);

	void set_forced_file_changed() { _forced_file_changed = true; }

  private:
	friend class MeterOCR_Test;
	bool isNotifiedFileChanged();
	bool autofixDetection(PIX *image, int &dX, int &dY, PIXA *debugPixa);
	int calcImpulses(const double &value, const double &oldValue) const;

	// class for the parameters:
	class BoundingBox {
	  public:
		BoundingBox(struct json_object *jb);
		static bool compare(const BoundingBox &f, const BoundingBox &s);
		std::string identifier;
		std::string conf_id;
		int scaler;
		bool digit;
		enum BoxType { BOX, CIRCLE };
		BoxType boxType;
		int x1, y1, x2, y2; // for box
		int cx, cy, cr;     // for circle
		static const int MIN_RADIUS = 10;
		float offset; // for circle
		bool autocenter;
		int ac_dx, ac_dy;
	};

	typedef std::list<BoundingBox> StdListBB;

	class Recognizer {
	  public:
		Recognizer(const std::string &type, struct json_object *);
		virtual bool recognize(PIX *image, int dX, int dY, ReadsMap &reads,
							   const ReadsMap *old_reads, PIXA *debugPixa) = 0;
		virtual ~Recognizer(){};
		virtual void getCaptureCoords(int &minX, int &minY, int &maxX, int &maxY) = 0;

	  protected:
		void saveDebugImage(PIXA *debugPixa, PIX *img, const char *title);
		std::string _type;
		StdListBB _boxes;
	};

#if OCR_TESSERACT_SUPPORT
	class RecognizerTesseract : public Recognizer {
	  public:
		RecognizerTesseract(struct json_object *);
		bool recognize(PIX *image, int dX, int dY, ReadsMap &reads, const ReadsMap *old_reads,
					   PIXA *debugPixa);
		virtual ~RecognizerTesseract();
		virtual void getCaptureCoords(int &minX, int &minY, int &maxX, int &maxY) {
			minX = _min_x1;
			minY = _min_y1;
			maxX = _max_x2;
			maxY = _max_y2;
		};

	  protected:
		bool initTesseract();
		bool deinitTesseract();

		tesseract::TessBaseAPI *api;
		double _gamma;
		int _gamma_min;
		int _gamma_max;
		int _min_x1, _max_x2, _min_y1, _max_y2;
		bool _all_digits;
	};
#endif

	class RecognizerNeedle : public Recognizer {
	  public:
		RecognizerNeedle(struct json_object *);
		bool recognize(PIX *image, int dX, int dY, ReadsMap &reads, const ReadsMap *old_reads,
					   PIXA *debugPixa);
		virtual ~RecognizerNeedle();
		virtual void getCaptureCoords(int &minX, int &minY, int &maxX, int &maxY) {
			minX = _min_x;
			minY = _min_y;
			maxX = _max_x;
			maxY = _max_y;
		};

	  protected:
		friend class MeterOCR_Test;
		int roundBasedOnSmallerDigits(const int curNr, const double &fnr, const double &smaller,
									  int &conf) const;
		int _min_x, _min_y, _max_x, _max_y;
		std::string _kernelColorString; // for kernelCreateFromString
		static const unsigned int RED_COLOR_LIMIT = 0x80000000;
	};

	class RecognizerBinary : public Recognizer {
	  public:
		RecognizerBinary(struct json_object *);
		bool recognize(PIX *image, int dX, int dY, ReadsMap &reads, const ReadsMap *old_reads,
					   PIXA *debugPixa);
		virtual ~RecognizerBinary();
		virtual void getCaptureCoords(int &minX, int &minY, int &maxX, int &maxY) {
			minX = _min_x;
			minY = _min_y;
			maxX = _max_x;
			maxY = _max_y;
		};

	  protected:
		friend class MeterOCR_Test;
		int _min_x, _min_y, _max_x, _max_y;
		std::string _kernelColorString; // for kernelCreateFromString
		bool _last_state;
		unsigned long _EDGE_HIGH;
		unsigned long _EDGE_LOW;
	};

	bool checkCapV4L2Dev();
	bool initV4L2Dev(unsigned int w, unsigned int h);
	bool readV4l2Frame(Pix *&image, bool first_time);

	struct buffer {
		void *start;
		size_t length;
	};

	std::string _file;
	Pix *_last_image;
	bool _use_v4l2;
	int _v4l2_fps;
	int _v4l2_skip_frames;
	int _v4l2_fd;
	struct buffer *_v4l2_buffers;
	unsigned int _v4l2_nbuffers;
	int _v4l2_cap_size_x;
	int _v4l2_cap_size_y;
	int _min_x, _min_y, _max_x, _max_y;
	int _notify_fd;
	bool _forced_file_changed;
	int _impulses;
	double _rotate;
	std::list<Recognizer *> _recognizer;
	int _autofix_range, _autofix_x, _autofix_y;
	ReadsMap *_last_reads;
	bool _generate_debug_image;
};

#endif

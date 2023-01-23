/**
 * OMS (M-Bus) based meter/device support
 * for spec see e.g. http://oms-group.org/fileadmin/pdf/OMS-Spec_Vol2_Primary_v301.pdf
 *
 * @copyright Copyright (c) 2015 - 2023, the volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl2.txt GNU Public License v2
 * @author Matthias Behr <mbehr (a) mcbehr.de>
 * */

#ifndef _meteroms_hpp_
#define _meteroms_hpp_

#include <mbus/mbus.h>
#include <protocols/Protocol.hpp>

class MeterOMS : public vz::protocol::Protocol {
  public:
	// abstract hw interface access for testability:
	class OMSHWif {
	  public:
		OMSHWif(){};
		virtual ~OMSHWif(){};

		virtual ssize_t read(void *buf, size_t count) = 0;        // similar to ::read
		virtual ssize_t write(const void *buf, size_t count) = 0; // similar to ::write

		virtual bool open() { return true; }
		virtual bool close() { return true; }

		virtual int send_frame(mbus_frame *frame);
		virtual int receive_frame(mbus_frame *frame);
	};

	class OMSSerialHWif : public OMSHWif {
	  public:
		OMSSerialHWif(const std::string &dev, int baudrate);
		virtual ~OMSSerialHWif();
		virtual ssize_t read(void *buf, size_t count) { return -1; };        // similar to ::read
		virtual ssize_t write(const void *buf, size_t count) { return -1; }; // similar to ::write

		virtual bool open();
		virtual bool close();

		virtual int send_frame(mbus_frame *frame);
		virtual int receive_frame(mbus_frame *frame);

	  protected:
		mbus_handle *_handle;
		int _baudrate;
	};

	MeterOMS(const std::list<Option> &options, OMSHWif *hwif = 0);
	virtual ~MeterOMS();

	virtual int open();
	virtual int close();
	virtual ssize_t read(std::vector<Reading> &rds, size_t n);
	bool aes_decrypt(unsigned char *data, int data_len, unsigned char *key, unsigned char *iv);

  protected:
	double get_record_value(mbus_data_record *) const;

	OMSHWif *_hwif;
	unsigned char *_aes_key;
	std::string _device;
	bool _mbus_debug;
	bool _use_local_time;
	double _last_timestamp;
};

#endif

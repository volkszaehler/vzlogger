#ifndef __MeterOCR_hpp_
#define __MeterOCR_hpp_

#include <gmock/gmock.h>
#include <list>

#include "Options.hpp"
#include "Reading.hpp"
#include <protocols/Protocol.hpp>

class MeterOCR : public vz::protocol::Protocol {
  public:
	MeterOCR(std::list<Option> options) : Protocol("ocr"){};
	virtual ~MeterOCR(){};
	MOCK_METHOD0(open, int());
	MOCK_METHOD0(close, int());
	MOCK_METHOD2(read, ssize_t(std::vector<Reading> &rds, size_t n));
};

#endif

#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <gmock/gmock.h>

#include "Buffer.hpp"
#include "Options.hpp"
#include "Reading.hpp"

class Channel {
  public:
	typedef vz::shared_ptr<Channel> Ptr;
	Channel() : mock_buf(new Buffer()){};
	Channel(const std::list<Option> &pOptions, const std::string api, const std::string pUuid,
			ReadingIdentifier::Ptr pIdentifier)
		: mock_buf(new Buffer()){};
	MOCK_METHOD1(start, void(Channel::Ptr));
	MOCK_METHOD0(join, void());
	MOCK_METHOD0(cancel, void());
	MOCK_METHOD0(name, const char *());
	MOCK_METHOD0(options, std::list<Option> &());
	MOCK_METHOD0(apiProtocol, const std::string());
	MOCK_METHOD0(buffer, Buffer::Ptr());
	MOCK_METHOD0(identifier, ReadingIdentifier::Ptr());
	MOCK_CONST_METHOD0(time_ms, int64_t());
	MOCK_METHOD1(last, void(Reading *rd));
	MOCK_METHOD1(push, void(const Reading &rd));
	MOCK_METHOD0(notify, void());
	MOCK_METHOD2(dump, char *(char *dump, size_t len));
	MOCK_CONST_METHOD0(size, size_t());
	MOCK_METHOD0(wait, void());
	MOCK_METHOD0(uuid, const char *());
	MOCK_CONST_METHOD0(duplicates, int());

	ReadingIdentifier::Ptr &real_id() { return mock_id; }
	ReadingIdentifier::Ptr mock_id;
	Buffer::Ptr mock_buf;
	Buffer::Ptr &real_buf() { return mock_buf; }
	Ptr _this_forthread;
};

#endif

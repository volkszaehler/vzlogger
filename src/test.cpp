// just for testing

#include <cstdio>

#include "common.h"
//#include "list.h"
#include "Buffer.hpp"

// void test_list() {
//	List<int> li;
//
//	for (int i = 0; i < 10; i++) {
//		li.push(i);
//	}
//
//	for (List<int>::iterator it = li.begin(); it != li.end(); ++it) {
//		printf("%i\n", *it);
//	}
//
//	printf("popped %i\n", li.pop());
//	printf("popped %i\n", li.pop());
//
//	for (List<int>::iterator it = li.begin(); it != li.end(); ++it) {
//		printf("%i\n", *it);
//	}
//}

#define NOMAIN
#include "vzlogger.cpp"

void test_buffer() {
	Buffer buf;

	struct timeval tv;
	StringIdentifier id;

	gettimeofday(&tv, NULL);

	for (int i = 0; i < 10; i++) {
		Reading rd;
		rd.value(11 * i);
		rd.time(tv);
		rd.identifier(new StringIdentifier());

		buf.push(rd);
	}

	int p = 0;
	for (Buffer::iterator it = buf.begin(); it != buf.end(); ++it) {
		if (p > 3)
			it->mark_delete();
		p++;
		printf("%d %f %d\n", p, it->value(), it->deleted());
	}

	buf.clean();

	for (Buffer::iterator it = buf.begin(); it != buf.end(); ++it) {
		printf("%f\n", it->value());
	}
}

int main() {
	test_buffer();

	return 0;
}

// just for testing

#include <cstdio>

#include "list.h"
#include "buffer.h"

void test_list() {
	List<int> li;

	for (int i = 0; i < 10; i++) {
		li.push(i);
	}

	for (List<int>::iterator it = li.begin(); it != li.end(); ++it) {
		printf("%i\n", *it);
	}

	printf("popped %i\n", li.pop());
	printf("popped %i\n", li.pop());

	for (List<int>::iterator it = li.begin(); it != li.end(); ++it) {
		printf("%i\n", *it);
	}
}

void test_buffer() {
	Buffer buf;

	struct timeval tv;
	StringIdentifier id;

	gettimeofday(&tv, NULL);

	for (int i = 0; i < 10; i++) {
		Reading rd;
		rd.value = 11*i;
		rd.time = tv;
		rd.identifier = &id;

		buf.push(rd);
	}

	for (Buffer::iterator it = buf.begin(); it != buf.end(); ++it) {
		printf("%i\n", it->value);
	}

	printf("popped %i\n", buf.pop().value);
	printf("popped %i\n", buf.pop().value);

	for (Buffer::iterator it = buf.begin(); it != buf.end(); ++it) {
		printf("%i\n", it->value);
	}

}

int main() {
	test_buffer();

	return 0;
}

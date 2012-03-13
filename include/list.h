/**
 * Double-linked list
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011, The volkszaehler.org project
 * @license http://www.gnu.org/licenses/gpl.txt GNU Public License
 * @author Steffen Vogel <info@steffenvogel.de>
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
 
#ifndef _LIST_H_
#define _LIST_H_

/* forward declarations */
template<class T> class List;
template<class T> class ListNode;
template<class T> class ListIterator;

template<class T>
class ListNode {
	friend class List;

public:

protected:
	T data;
	ListNode<T> *prev;
	ListNode<T> *next;
};

template<class T>
class ListIterator {
	friend class ListNode;

public:
	bool operator==(ListIterator<T>& i) const {
		return (cur == i.cur);
	};

	bool operator!=(ListIterator<T>& i) const {
		return (cur != i.cur);
	};

  T& operator*() {
		if (!list || *this == list->end()) {
			throw invalid_argument("Iter::operator*()");
		}

		return cur->data;
	};

	/* Prefix increment */
	ListIterator& operator++() {
		if (!list || *this == list->end()) {
			throw Exception("Iter::operator++()");
		}

		cur = cur->next;
		return *this;

	};

	/* Postfix increment */
	ListIterator operator++(int) {
		ListIterator<T> temp = *this;
		++(*this);
		return temp;
	};

	/* Prefix decrement */
	ListIterator& operator--() {
		if (!list || *this == list->begin())
			throw Exception("Iterator:: operator--()");

		cur = cur->pre;
		return *this;
	};

	/* Postfix decrement */
	ListIterator operator--(int) {
		ListIterator<T> temp = *this;
		--(*this);
		return temp;
	};

private:
	friend class List<T>;
	ListNode<T> *cur;
	List<T> *list;

	ListIterator( List<T> *l,ListNode<T> *n):the_list(l),  cur( n ) {};
};

template<class T>
class List {

public:
	List() : size(0), head(NULL), tail(NULL) { };

	virtual ~List() {
		while (head != NULL) {
			ListNode *oldNode = head;
			head = oldNode->next;

			delete oldNode;
		}
	}

	ListIterator<T> push(T data) {
		ListNode<T> *newNode = new ListNode;

		if (newNode == NULL) {
			throw Exception("Cannot  allocate memory");
		}

		newNode->data = data;
		newNode->prev = list->tail;
		newNode->next = NULL;

		if (tail == NULL) {
			head = newNode;
		}
		else {
			tail->next = newNode;
		}

		tail = newNode;
		size++;

		return size;
	};

	T pop() {
		ListNode *oldNode = list->tail;

		if (oldNode == NULL) { /* list is empty */
			throw Exception("List is empty");
		}

		T data = oldNode->data;

		tail = oldNode->prev;
		size--;

		delete oldNode;

		return data;
	}

	/* Iterators */
	ListIterator<T> begin() {
		return ListIterator<T>(this, head);
	};

	ListIterator<T> end() {
		return ListIterator<T>(this, tail);
	};

protected:
	size_t size;
	ListNode *head;
	ListNode *tail;
}

#endif /* _LIST_H_ */

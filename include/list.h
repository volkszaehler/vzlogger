/**
 * Single linked list
 *
 * @package vzlogger
 * @copyright Copyright (c) 2011 - 2023, The volkszaehler.org project
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

#include "exception.h"

template <class T> class List {

  protected:
	class Node; /* protected forward declaration */

  public:
	class Iterator; /* public forward declaration */
	typedef Iterator iterator;

	List() : size(0), head(NULL), tail(NULL){};
	~List() { clear(); };

	Iterator push(T data) {
		Node *newNode = new Node;

		if (newNode == NULL) {
			throw new Exception("Out of memory!");
		}

		newNode->data = data;
		newNode->next = NULL;

		if (head == NULL) { /* empty list */
			newNode->prev = NULL;
			head = newNode;
		} else {
			newNode->prev = tail;
			tail->next = newNode;
		}

		tail = newNode;
		size++;

		return Iterator(newNode);
	}

	T pop() {
		Node *oldNode = head;
		T data = head->data;

		if (oldNode == NULL) {
			throw new Exception("List is empty!");
		}

		head = oldNode->next;
		size--;

		delete oldNode;
		return data;
	}

	size_t length() const { return size; };
	void clear() {
		while (size > 0) {
			pop();
		}
	};

	/* Iterators */
	Iterator begin() { return Iterator(head); };
	Iterator end() { return Iterator(NULL); };

  protected:
	size_t size;
	Node *head, *tail;
};

template <class T> class List<T>::Node {
	friend class List<T>;
	friend class List<T>::Iterator;

  protected:
	T data;
	Node *next, *prev;
};

template <class T> class List<T>::Iterator {
	friend class List<T>;

  public:
	Iterator() : cur(NULL){};

	bool operator==(Iterator const &i) const { return (cur == i.cur); };
	bool operator!=(Iterator const &i) const { return (cur != i.cur); };

	T *operator->() const { return &cur->data; };
	T &operator*() const { return cur->data; };

	Iterator operator++() {
		cur = cur->next;
		return *this;
	};

  protected:
	Iterator(Node *node) : cur(node){};

	Node *cur;
};

#endif /* _LIST_H_ */

/**
 * Linked list to manage channels
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

#include <stdlib.h>
#include <string.h>

#include "list.h"

void list_init(list_t *ls) {
	ls->start = NULL;
	ls->size = 0;
}

int list_push(list_t *ls, channel_t ch) {
	channel_t *new = malloc(sizeof(channel_t));

	if (!new) {
		return 0; /* cannot allocate memory */
	}

	memcpy(new, &ch, sizeof(channel_t));

	if (ls->start == NULL) { /* empty list */
		new->next = NULL;
	}
	else {
		new->next = ls->start;
	}

	ls->start = new;
	ls->size++;

	return ls->size;
}

void list_free(list_t *ls) {
	channel_t *ch = ls->start;
	do {
		channel_t *tmp = ch;
		ch = ch->next;
		channel_free(tmp);
	} while (ch);

	ls->start = NULL;
	ls->size = 0;
}

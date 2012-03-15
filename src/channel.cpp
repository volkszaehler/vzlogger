/**
 * Channel class
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

#include <sstream>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "channel.h"

int Channel::instances = 0;

Channel::Channel(
  const std::string apiName,
  const char *pUuid,
  const char *pMiddleware,
  ReadingIdentifier::Ptr pIdentifier
  )
    : _buffer(new Buffer())
    , _identifier(pIdentifier)
    , _last(0)
                //, _last(_identifier)
    , _apiName(apiName)
{
	id = instances++;

  // set channel name
  std::stringstream oss;
  oss<<"chn"<< id;
  _name=oss.str();

  _uuid = strdup(pUuid);
  _middleware = strdup(pMiddleware);

  //buffer_init(&buffer); /* initialize buffer */
  pthread_cond_init(&condition, NULL); /* initialize thread syncronization helpers */
}

/**
 * Free all allocated memory recursivly
 */
Channel::~Channel() {
	//buffer_free(&buffer);
	pthread_cond_destroy(&condition);

	//free(uuid);
	//free(middleware);
}


/* DRN_SIGNAL_H
 * 
 * This file is part of libwsepd.
 * 
 * Copyright (C) 2019 Ellis Rhys Thomas
 *
 * libwsepd is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libwsepd is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with libwsepd.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Description:
 *
 * Signal handling, important as device must be powered down to avoid
 * damage.
 * 
 */

#ifndef WSEPD_SIGNAL_H
#define WSEPD_SIGNAL_H

#include <signal.h>

extern volatile sig_atomic_t done;

int create_signal_handler(void); /* Returns non zero on failure */
void start_signal_handler(void); /* Block SIGINT and SIGTERM */
void stop_signal_handler(void);	 /* Revert to defaults */

/* Sets done to 1 when signal received */
void check_signal_handler(struct Epd *Display);

#endif

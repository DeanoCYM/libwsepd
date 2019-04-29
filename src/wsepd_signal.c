/* wsepd_signal.c
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
 * Implementation of signal handling functionality in
 * libwsepd.a. Suppression of interupt and termination signal permits
 * cleanup of resources and setting e-paper device into deep sleep
 * mode proir to exit. The e-paper device can be damaged if not
 * powered down correctly.
 * 
 */

#include <signal.h>
#include <string.h>
#include "libwsepd.h"
#include "ert_log.h"

volatile sig_atomic_t done = 0;
static struct sigaction action;

/* Handler sets done to one, should be called when signal is recieved */
static void
set_signal_received(__attribute__((unused)) int signum)
{
  done = 1;
  return;
}

/* Blocks SIGINT and SIGTERM */
int
create_signal_handler(void)
{
  if (NULL == memset(&action, 0, sizeof action))
    goto out;
  if (0 != sigaction(SIGINT, &action, NULL))
    goto out;
  if (0 != sigaction(SIGTERM, &action, NULL))
    goto out;

  return 0;
 out:
  log_err("Failed to start signal handler.");
  return 1;
}

void
start_signal_handler(void)
{
  action.sa_handler = &set_signal_received;
  return;
}

void
stop_signal_handler(void)
{
  action.sa_handler = SIG_DFL;
  return;
}

void
check_signal_handler(struct Epd *Display)
{
  if (done) {
    log_err("Signal recieved, cleaning up....");
    EPD_destroy(Display);
  }

  return;
}

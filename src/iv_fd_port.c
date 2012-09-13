/*
 * ivykis, an event handling library
 * Copyright (C) 2011 Lennert Buytenhek
 * Dedicated to Marija Kulikova.
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version
 * 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 2.1 for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License version 2.1 along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street - Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <port.h>
#include <string.h>
#include "iv_private.h"
#include "iv_fd_private.h"

#define PORTEV_NUM	1024

static int iv_fd_port_init(struct iv_state *st)
{
	int fd;

	fd = port_create();
	if (fd < 0)
		return -1;

	iv_fd_set_cloexec(fd);

	st->u.port.port_fd = fd;
	INIT_IV_LIST_HEAD(&st->u.port.notify);

	return 0;
}

static int bits_to_poll_mask(int bits)
{
	int mask;

	mask = 0;
	if (bits & MASKIN)
		mask |= POLLIN;
	if (bits & MASKOUT)
		mask |= POLLOUT;

	return mask;
}

static int __iv_fd_port_upload_one(struct iv_state *st, struct iv_fd_ *fd)
{
	int ret;

	iv_list_del_init(&fd->list_notify);

	if (fd->wanted_bands) {
		ret = port_associate(st->u.port.port_fd, PORT_SOURCE_FD, fd->fd,
				     bits_to_poll_mask(fd->wanted_bands), fd);
	} else {
		ret = port_dissociate(st->u.port.port_fd, PORT_SOURCE_FD,
				      fd->fd);
	}

	if (ret == 0)
		fd->registered_bands = fd->wanted_bands;

	return ret;
}

static void iv_fd_port_upload_one(struct iv_state *st, struct iv_fd_ *fd)
{
	if (__iv_fd_port_upload_one(st, fd) < 0) {
		iv_fatal("iv_fd_port_upload_one: got error %d[%s]", errno,
			 strerror(errno));
	}
}

static void iv_fd_port_upload(struct iv_state *st)
{
	while (!iv_list_empty(&st->u.port.notify)) {
		struct iv_fd_ *fd;

		fd = iv_list_entry(st->u.port.notify.next,
				   struct iv_fd_, list_notify);

		iv_fd_port_upload_one(st, fd);
	}
}

static void iv_fd_port_poll(struct iv_state *st,
			    struct iv_list_head *active, struct timespec *to)
{
	unsigned int nget;
	port_event_t pe[PORTEV_NUM];
	int ret;
	int i;

	iv_fd_port_upload(st);

poll_more:
	nget = 1;

	/*
	 * If we get EINTR from port_getn(), no events are returned
	 * and nget will not have been updated, but if we get ETIME,
	 * events may be returned, and nget will be set to the number
	 * of events in the array, and we need to process those
	 * events as usual.
	 */
	ret = port_getn(st->u.port.port_fd, pe, PORTEV_NUM, &nget, to);
	if (ret < 0 && errno != ETIME) {
		if (errno == EINTR)
			return;

		iv_fatal("iv_fd_port_poll: got error %d[%s]", errno,
			 strerror(errno));
	}

	for (i = 0; i < nget; i++) {
		int source;

		source = pe[i].portev_source;
		if (source == PORT_SOURCE_FD) {
			int revents;
			struct iv_fd_ *fd;

			revents = pe[i].portev_events;
			fd = pe[i].portev_user;

			if (revents & (POLLIN | POLLERR | POLLHUP))
				iv_fd_make_ready(active, fd, MASKIN);

			if (revents & (POLLOUT | POLLERR | POLLHUP))
				iv_fd_make_ready(active, fd, MASKOUT);

			if (revents & (POLLERR | POLLHUP))
				iv_fd_make_ready(active, fd, MASKERR);

			fd->registered_bands = 0;

			iv_list_del_init(&fd->list_notify);
			if (fd->wanted_bands) {
				iv_list_add_tail(&fd->list_notify,
						 &st->u.port.notify);
			}
		} else if (source == PORT_SOURCE_USER) {
			__iv_invalidate_now(st);
			iv_event_run_pending_events();
		} else {
			iv_fatal("iv_fd_port_poll: received event "
				 "from unknown source %d", source);
		}
	}

	if (nget == PORTEV_NUM) {
		to->tv_sec = 0;
		to->tv_nsec = 0;
		goto poll_more;
	}
}

static void iv_fd_port_unregister_fd(struct iv_state *st, struct iv_fd_ *fd)
{
	if (!iv_list_empty(&fd->list_notify))
		iv_fd_port_upload_one(st, fd);
}

static void iv_fd_port_notify_fd(struct iv_state *st, struct iv_fd_ *fd)
{
	iv_list_del_init(&fd->list_notify);
	if (fd->registered_bands != fd->wanted_bands)
		iv_list_add_tail(&fd->list_notify, &st->u.port.notify);
}

static int iv_fd_port_notify_fd_sync(struct iv_state *st, struct iv_fd_ *fd)
{
	return __iv_fd_port_upload_one(st, fd);
}

static void iv_fd_port_deinit(struct iv_state *st)
{
	close(st->u.port.port_fd);
}

static int iv_fd_port_event_rx_on(struct iv_state *st)
{
	st->numobjs++;

	return 0;
}

static void iv_fd_port_event_rx_off(struct iv_state *st)
{
	st->numobjs--;
}

static void iv_fd_port_event_send(struct iv_state *dest)
{
	if (port_send(dest->u.port.port_fd, 0, NULL) < 0) {
		iv_fatal("iv_fd_port_event_send: port_send got "
			 "error %d[%s]", errno, strerror(errno));
	}
}

struct iv_fd_poll_method iv_fd_poll_method_port = {
	.name		= "port",
	.init		= iv_fd_port_init,
	.poll		= iv_fd_port_poll,
	.unregister_fd	= iv_fd_port_unregister_fd,
	.notify_fd	= iv_fd_port_notify_fd,
	.notify_fd_sync	= iv_fd_port_notify_fd_sync,
	.deinit		= iv_fd_port_deinit,
	.event_rx_on	= iv_fd_port_event_rx_on,
	.event_rx_off	= iv_fd_port_event_rx_off,
	.event_send	= iv_fd_port_event_send,
};

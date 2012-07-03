/*
 * ivykis, an event handling library
 * Copyright (C) 2010 Lennert Buytenhek
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
#include <iv.h>
#include <iv_event.h>
#include <iv_thread.h>
#include <iv_tls.h>
#include <pthread.h>
#include <string.h>
#include <sys/syscall.h>

/* gettid *******************************************************************/
#ifdef __FreeBSD__
/* Older FreeBSDs (6.1) don't include ucontext.h in thr.h.  */
#include <sys/ucontext.h>
#include <sys/thr.h>
#endif

static pid_t gettid(void)
{
	pid_t tid;

	tid = 0;
#ifdef __NR_gettid
	tid = syscall(__NR_gettid);
#elif defined(__FreeBSD__)
	long thr;
	thr_self(&thr);
	tid = (pid_t)thr;
#endif

	return tid;
}


/* data structures and global data ******************************************/
struct iv_thread {
	struct iv_list_head	list;
	pthread_t		thread_id;
	struct iv_event		dead;
	char			*name;
	pid_t			tid;
	void			(*start_routine)(void *);
	void			*arg;
};

static int iv_thread_debug;


/* tls **********************************************************************/
struct iv_thread_thr_info {
	struct iv_list_head	child_threads;
};

static void iv_thread_tls_init_thread(void *_tinfo)
{
	struct iv_thread_thr_info *tinfo = _tinfo;

	INIT_IV_LIST_HEAD(&tinfo->child_threads);
}

static void iv_thread_tls_deinit_thread(void *_tinfo)
{
	struct iv_thread_thr_info *tinfo = _tinfo;
	struct iv_list_head *ilh;

	iv_list_for_each (ilh, &tinfo->child_threads) {
		struct iv_thread *thr;

		thr = iv_list_entry(ilh, struct iv_thread, list);
		pthread_detach(thr->thread_id);
	}
}

static struct iv_tls_user iv_thread_tls_user = {
	.sizeof_state	= sizeof(struct iv_thread_thr_info),
	.init_thread	= iv_thread_tls_init_thread,
	.deinit_thread	= iv_thread_tls_deinit_thread,
};

static void iv_thread_tls_init(void) __attribute__((constructor));
static void iv_thread_tls_init(void)
{
	iv_tls_user_register(&iv_thread_tls_user);
}


/* callee thread ************************************************************/
static void iv_thread_cleanup_handler(void *_thr)
{
	struct iv_thread *thr = _thr;

	if (iv_thread_debug)
		fprintf(stderr, "iv_thread: [%s] was canceled\n", thr->name);

	iv_event_post(&thr->dead);
}

static void *iv_thread_handler(void *_thr)
{
	struct iv_thread *thr = _thr;

	thr->tid = gettid();

	pthread_cleanup_push(iv_thread_cleanup_handler, thr);
	thr->start_routine(thr->arg);
	pthread_cleanup_pop(0);

	if (iv_thread_debug)
		fprintf(stderr, "iv_thread: [%s] terminating normally\n",
			thr->name);

	iv_event_post(&thr->dead);

	return NULL;
}


/* calling thread ***********************************************************/
static void iv_thread_died(void *_thr)
{
	struct iv_thread *thr = _thr;

	pthread_join(thr->thread_id, NULL);

	if (iv_thread_debug)
		fprintf(stderr, "iv_thread: [%s] joined\n", thr->name);

	iv_list_del(&thr->list);
	iv_event_unregister(&thr->dead);
	free(thr->name);
	free(thr);
}

int iv_thread_create(char *name, void (*start_routine)(void *), void *arg)
{
	struct iv_thread_thr_info *tinfo = iv_tls_user_ptr(&iv_thread_tls_user);
	struct iv_thread *thr;
	int ret;

	thr = malloc(sizeof(*thr));
	if (thr == NULL)
		return -1;

	IV_EVENT_INIT(&thr->dead);
	thr->dead.cookie = thr;
	thr->dead.handler = iv_thread_died;
	iv_event_register(&thr->dead);

	thr->name = strdup(name);
	thr->tid = 0;
	thr->start_routine = start_routine;
	thr->arg = arg;

	ret = pthread_create(&thr->thread_id, NULL, iv_thread_handler, thr);
	if (ret)
		goto out;

	iv_list_add_tail(&thr->list, &tinfo->child_threads);

	if (iv_thread_debug)
		fprintf(stderr, "iv_thread: [%s] started\n", name);

	return 0;

out:
	iv_event_unregister(&thr->dead);
	free(thr->name);
	free(thr);

	if (iv_thread_debug)
		fprintf(stderr, "iv_thread: [%s] failed to start\n", name);

	return -1;
}

void iv_thread_set_debug_state(int state)
{
	iv_thread_debug = !!state;
}

void iv_thread_list_children(void)
{
	struct iv_thread_thr_info *tinfo = iv_tls_user_ptr(&iv_thread_tls_user);
	struct iv_list_head *ilh;

	fprintf(stderr, "tid\tname\n");
	fprintf(stderr, "%d\tself\n", (int)gettid());

	iv_list_for_each (ilh, &tinfo->child_threads) {
		struct iv_thread *thr;

		thr = iv_list_entry(ilh, struct iv_thread, list);
		fprintf(stderr, "%d\t%s\n", (int)thr->tid, thr->name);
	}
}

AM_CPPFLAGS		= -D_GNU_SOURCE -I$(top_srcdir)/src/include	\
			  -I$(top_builddir)/src/include

LDADD			= $(top_builddir)/src/libivykis.la

PROGS			= iv_event_bench_timer		\
			  iv_event_raw_bench_timer	\
			  iv_event_test			\
			  iv_thread_test		\
			  iv_work_test

TESTS			= avl				\
			  iv_event_raw_test		\
			  struct_sizes			\
			  timer				\
			  timer_fairness		\
			  timer_order			\
			  timer_past

if HAVE_POSIX
PROGS			+= client			\
			   connectfail			\
			   connectreset			\
			   iv_event_bench_signal	\
			   iv_event_raw_bench_signal	\
			   iv_signal_bench_signal	\
			   iv_signal_bench_timer	\
			   iv_fd_pump_discard		\
			   iv_fd_pump_echo		\
			   iv_popen_test		\
			   iv_signal_child_test		\
			   iv_wait_test			\
			   null				\
			   server			\
			   server_thread

if HAVE_INOTIFY
PROGS			+= iv_inotify_test
endif

TESTS			+= iv_signal_test

endif

if HAVE_WIN32
PROGS			+= handle
endif

noinst_PROGRAMS		= $(PROGS) $(TESTS)

avl_SOURCES			= avl.c
client_SOURCES			= client.c
connectfail_SOURCES		= connectfail.c
connectreset_SOURCES		= connectreset.c
handle_SOURCES			= handle.c
iv_event_raw_test_SOURCES	= iv_event_raw_test.c
iv_event_test_SOURCES		= iv_event_test.c
iv_fd_pump_discard_SOURCES	= iv_fd_pump_discard.c
iv_fd_pump_echo_SOURCES		= iv_fd_pump_echo.c
iv_popen_test_SOURCES		= iv_popen_test.c
iv_signal_child_test_SOURCES	= iv_signal_child_test.c
iv_signal_test_SOURCES		= iv_signal_test.c
iv_thread_test_SOURCES		= iv_thread_test.c
iv_wait_test_SOURCES		= iv_wait_test.c
iv_work_test_SOURCES		= iv_work_test.c
null_SOURCES			= null.c
server_SOURCES			= server.c
struct_sizes_SOURCES		= struct_sizes.c
timer_SOURCES			= timer.c
timer_order_SOURCES		= timer_order.c

iv_event_bench_signal_CPPFLAGS	= -D_GNU_SOURCE -I$(top_srcdir)/src/include -I$(top_builddir)/src/include -DUSE_SIGNAL
iv_event_bench_signal_SOURCES	= iv_event_bench.c

iv_event_bench_timer_CPPFLAGS	= -D_GNU_SOURCE -I$(top_srcdir)/src/include -I$(top_builddir)/src/include
iv_event_bench_timer_SOURCES	= iv_event_bench.c

iv_event_raw_bench_signal_CPPFLAGS	= -D_GNU_SOURCE -I$(top_srcdir)/src/include -I$(top_builddir)/src/include -DUSE_SIGNAL
iv_event_raw_bench_signal_SOURCES	= iv_event_raw_bench.c

iv_event_raw_bench_timer_CPPFLAGS	= -D_GNU_SOURCE -I$(top_srcdir)/src/include -I$(top_builddir)/src/include
iv_event_raw_bench_timer_SOURCES	= iv_event_raw_bench.c

iv_signal_bench_signal_CPPFLAGS		= -D_GNU_SOURCE -I$(top_srcdir)/src/include -I$(top_builddir)/src/include -DUSE_SIGNAL
iv_signal_bench_signal_SOURCES		= iv_signal_bench.c

iv_signal_bench_timer_CPPFLAGS		= -D_GNU_SOURCE -I$(top_srcdir)/src/include -I$(top_builddir)/src/include
iv_signal_bench_timer_SOURCES		= iv_signal_bench.c

server_thread_CPPFLAGS	= -D_GNU_SOURCE -I$(top_srcdir)/src/include -I$(top_builddir)/src/include -DTHREAD
server_thread_SOURCES	= server.c

#include "lwes_ruby.h"
#include <errno.h>

static void listener_free(void *ptr)
{
	struct lwes_listener *listener = ptr;

	if (listener)
		(void)lwes_listener_destroy(listener);
}

static struct lwes_listener *listener_ptr(VALUE self)
{
	struct lwes_listener *listener = DATA_PTR(self);

	if (listener == NULL)
		rb_raise(rb_eIOError, "closed listener");
	return listener;
}

static void fail(int rc, const char *fn)
{
	if (errno)
		rb_sys_fail(fn);
	rb_raise(rb_eRuntimeError, "%s failed with: %d", fn, rc);
}

/*
 * call-seq:
 *	listener.close => nil
 *
 * Closes the socket used by the LWES::Listener object.  Raises IOError if
 * already closed.
 */
static VALUE listener_close(VALUE self)
{
	struct lwes_listener *listener = listener_ptr(self);
	int err;

	DATA_PTR(self) = NULL;
	errno = 0;
	err = lwes_listener_destroy(listener);
	if (err)
		fail(err, "lwes_listener_destroy()");
	return Qnil;
}

static VALUE listener_alloc(VALUE klass)
{
	return Data_Wrap_Struct(klass, NULL, listener_free, NULL);
}

/*
 * call-seq:
 *	listener = LWES::Listener.new(address: "224.1.1.11", port: 12345)
 *
 * Binds an LWES listener socket to receive events on.  It takes the following
 * options:
 *
 * - +:address+ is a dotted quad string of an IPv4 address to listen on.
 * - +:port+ is the port to listen on
 * - +:iface+ is a dotted quad string of the interface to listen on (optional)
 */
static VALUE listener_init(VALUE self, VALUE options)
{
	struct lwes_listener *listener;
	LWES_SHORT_STRING address;
	LWES_SHORT_STRING iface;
	LWES_U_INT_32 port;
	VALUE tmp;

	if (DATA_PTR(self))
		rb_raise(rb_eRuntimeError,
		         "initializing already initialized Listener");
	if (TYPE(options) != T_HASH)
		rb_raise(rb_eTypeError, "options must be a hash");

	tmp = rb_hash_aref(options, ID2SYM(rb_intern("address")));
	address = StringValueCStr(tmp);

	tmp = rb_hash_aref(options, ID2SYM(rb_intern("iface")));
	iface = NIL_P(tmp) ? NULL : StringValueCStr(tmp);

	tmp = rb_hash_aref(options, ID2SYM(rb_intern("port")));
	port = (LWES_U_INT_32)lwesrb_uint16(tmp);

	listener = lwes_listener_create(address, iface, port);
	if (listener == NULL) {
		rb_gc();
		listener = lwes_listener_create(address, iface, port);
		if (listener == NULL)
			rb_raise(rb_eRuntimeError,
			         "failed to create LWES Listener");
	}

	DATA_PTR(self) = listener;

	return self;
}

struct recv_args {
	struct lwes_listener *listener;
	struct lwes_event *event;
	unsigned timeout_ms;
};

static VALUE recv_event(void *ptr)
{
	struct recv_args *a = ptr;
	int r;

	if (a->timeout_ms == UINT_MAX)
		r = lwes_listener_recv(a->listener, a->event);
	else
		r = lwes_listener_recv_by(a->listener, a->event, a->timeout_ms);
	return (VALUE)r;
}

#ifdef HAVE_RB_THREAD_BLOCKING_REGION
/*
 * call-seq:
 *	listener.recv => LWES::Event
 *	listener.recv(timeout_ms) => LWES::Event or nil
 *
 * Receives and returns one LWES::Event from the network.
 * An optional timeout (in milliseconds) may be specified and cause this
 * to return +nil+ on timeout.  This method is only available under Ruby 1.9.
 */
static VALUE listener_recv(int argc, VALUE *argv, VALUE self)
{
	struct recv_args args;
	VALUE timeout;
	int r, saved_errno;

	rb_scan_args(argc, argv, "01", &timeout);

	args.listener = listener_ptr(self);
	args.event = lwes_event_create_no_name(NULL);
	args.timeout_ms = NIL_P(timeout) ? UINT_MAX : NUM2UINT(timeout);

	saved_errno = errno = 0;
	r = (int)rb_thread_blocking_region(recv_event, &args, RUBY_UBF_IO, 0);
	if (r >= 0)
		return lwesrb_wrap_event(cLWES_Event, args.event);
	saved_errno = errno;
	(void)lwes_event_destroy(args.event);
	if (r == -2 && ! NIL_P(timeout))
		return Qnil;
	errno = saved_errno;
	fail(r, "lwes_listener_recv(_by)");
	return Qnil;
}
#endif /* HAVE_RB_THREAD_BLOCKING_REGION */

void lwesrb_init_listener(void)
{
	VALUE mLWES = rb_define_module("LWES");
	VALUE cListener = rb_define_class_under(mLWES, "Listener", rb_cObject);
	rb_define_alloc_func(cListener, listener_alloc);
	rb_define_private_method(cListener, "initialize", listener_init, 1);
#ifdef HAVE_RB_THREAD_BLOCKING_REGION
	rb_define_method(cListener, "recv", listener_recv, -1);
#endif
	rb_define_method(cListener, "close", listener_close, 0);
}

#include "lwes_ruby.h"

static VALUE cLWES_Emitter;

/* the underlying struct for LWES::Emitter */
struct _rb_lwes_emitter {
	struct lwes_emitter *e;

	/*
	 * map certain fields to certain types so we won't have to worry about
	 * it later on in the Ruby API.
	 */
	VALUE field_types;
};

/* gets the _rb_lwes_emitter struct pointer from self */
static struct _rb_lwes_emitter * _rle(VALUE self)
{
	struct _rb_lwes_emitter *rle;

	Data_Get_Struct(self, struct _rb_lwes_emitter, rle);

	return rle;
}

/* mark-and-sweep GC automatically calls this */
static void rle_mark(void *ptr)
{
	struct _rb_lwes_emitter *rle = ptr;

	rb_gc_mark(rle->field_types);
}

/* GC automatically calls this when object is finalized */
static void rle_free(void *ptr)
{
	struct _rb_lwes_emitter *rle = ptr;

	if (rle->e)
		lwes_emitter_destroy(rle->e);
	rle->e = NULL;
}

/* called by the GC when object is allocated */
static VALUE rle_alloc(VALUE klass)
{
	struct _rb_lwes_emitter *rle;

	return Data_Make_Struct(klass, struct _rb_lwes_emitter,
	                        rle_mark, rle_free, rle);
}

/* returns an lwes_event depending on the value of name */
static struct lwes_event * create_event(VALUE name)
{
	switch (TYPE(name)) {
	case T_NIL:
		return lwes_event_create_no_name(NULL);
	case T_STRING:
		return lwes_event_create(NULL, RSTRING_PTR(name));
	}
	rb_raise(rb_eArgError, "event name must be String or nil");
	assert(0 && "rb_raise broke on us");
	return NULL;
}

/*
 * kv - Array:
 *   key => String,
 *   key => [ numeric_type, Numeric ],
 *   key => true,
 *   key => false,
 * memo - lwes_event pointer
 */
static VALUE event_hash_iter_i(VALUE kv, VALUE memo)
{
	VALUE *tmp;
	VALUE v;
	struct lwes_event *e = (struct lwes_event *)memo;
	LWES_CONST_SHORT_STRING name;

	assert(TYPE(kv) == T_ARRAY &&
	       "hash iteration not giving key-value pairs");
	tmp = RARRAY_PTR(kv);
	name = RSTRING_PTR(rb_obj_as_string(tmp[0]));

	v = tmp[1];
	switch (TYPE(v)) {
	case T_TRUE:
		if (lwes_event_set_BOOLEAN(e, name, TRUE) < 0)
			rb_raise(rb_eRuntimeError,
			         "failed to set boolean true for event");
		break;
	case T_FALSE:
		if (lwes_event_set_BOOLEAN(e, name, FALSE) < 0)
			rb_raise(rb_eRuntimeError,
			         "failed to set boolean false for event");
		break;
	case T_ARRAY:
		if (lwesrb_event_set_numeric(e, name, v) < 0)
			rb_raise(rb_eRuntimeError,
			         "failed to set numeric for event");
		break;
	case T_STRING:
		if (lwes_event_set_STRING(e, name, RSTRING_PTR(v)) < 0)
			rb_raise(rb_eRuntimeError,
			         "failed to set string for event");
		break;
	default:
		rb_p(v);
	}

	return Qnil;
}

static VALUE _emit_hash(VALUE _tmp)
{
	VALUE *tmp = (VALUE *)_tmp;
	VALUE self = tmp[0];
	VALUE event = tmp[1];
	struct lwes_event *e = (struct lwes_event *)tmp[2];
	int nr;

	rb_iterate(rb_each, event, event_hash_iter_i, (VALUE)e);
	if ((nr = lwes_emitter_emit(_rle(self)->e, e)) < 0) {
		rb_raise(rb_eRuntimeError, "failed to emit event");
	}

	return event;
}

static VALUE _destroy_event(VALUE _e)
{
	struct lwes_event *e = (struct lwes_event *)_e;

	assert(e && "destroying NULL event");
	lwes_event_destroy(e);

	return Qnil;
}

static VALUE emit_hash(VALUE self, VALUE name, VALUE event)
{
	struct lwes_event *e = create_event(name);
	VALUE tmp[3];

	if (!e)
		rb_raise(rb_eRuntimeError, "failed to create lwes_event");

	tmp[0] = self;
	tmp[1] = event;
	tmp[2] = (VALUE)e;
	rb_ensure(_emit_hash, (VALUE)&tmp, _destroy_event, (VALUE)e);

	return event;
}

/*
 * XXX Not working yet
 * call-seq:
 *   emitter = LWES::Emitter.new
 *   emitter.emit("EventName", :foo => "HI")
 */
static VALUE emitter_emit(VALUE self, VALUE name, VALUE event)
{
	switch (TYPE(event)) {
	case T_HASH:
		return emit_hash(self, name, event);
	/* TODO T_STRUCT + esf support */
	default:
		rb_raise(rb_eTypeError, "must be a Hash"); /* or Struct */
	}

	return event;
}

/*
 * Destroys the associated lwes_emitter and the associated socket.  This
 * method is rarely needed as Ruby garbage collection will take care of
 * closing for you, but may be useful in odd cases when it is desirable
 * to release file descriptors ASAP.
 */
static VALUE emitter_close(VALUE self)
{
	rle_free(_rle(self));

	return Qnil;
}

/* should only used internally by #initialize */
static VALUE _create(VALUE self, VALUE options)
{
	struct _rb_lwes_emitter *rle = _rle(self);
	VALUE address, iface, port, heartbeat, ttl;
	LWES_CONST_SHORT_STRING _address, _iface;
	LWES_U_INT_32 _port; /* odd, uint16 would be enough here */
	LWES_BOOLEAN _emit_heartbeat = FALSE;
	LWES_INT_16 _freq = 0;
	LWES_U_INT_32 _ttl = UINT32_MAX; /* nobody sets a ttl this long, right? */

	if (rle->e)
		rb_raise(rb_eRuntimeError, "already created lwes_emitter");
	if (TYPE(options) != T_HASH)
		rb_raise(rb_eTypeError, "options must be a hash");

	address = rb_hash_aref(options, ID2SYM(rb_intern("address")));
	if (TYPE(address) != T_STRING)
		rb_raise(rb_eTypeError, ":address must be a string");
	_address = RSTRING_PTR(address);

	iface = rb_hash_aref(options, ID2SYM(rb_intern("iface")));
	if (TYPE(iface) != T_STRING)
		rb_raise(rb_eTypeError, ":iface must be a string");
	_iface = RSTRING_PTR(address);

	port = rb_hash_aref(options, ID2SYM(rb_intern("port")));
	if (TYPE(port) != T_FIXNUM)
		rb_raise(rb_eTypeError, ":port must be a Fixnum");
	_port = NUM2UINT(port);

	heartbeat = rb_hash_aref(options, ID2SYM(rb_intern("heartbeat")));
	if (TYPE(heartbeat) == T_FIXNUM) {
		int tmp = NUM2INT(heartbeat);
		if (tmp > INT16_MAX)
			rb_raise(rb_eArgError,":heartbeat > INT16_MAX seconds");
		_emit_heartbeat = TRUE;
		_freq = (LWES_INT_16)tmp;
	} else if (NIL_P(heartbeat)) { /* do nothing, use defaults */
	} else
		rb_raise(rb_eTypeError, ":heartbeat must be a Fixnum or nil");

	ttl = rb_hash_aref(options, ID2SYM(rb_intern("ttl")));
	if (TYPE(ttl) == T_FIXNUM) {
		unsigned LONG_LONG tmp = NUM2ULL(ttl);
		if (tmp >= UINT32_MAX)
			rb_raise(rb_eArgError, ":ttl >= UINT32_MAX seconds");
		_ttl = (LWES_U_INT_32)tmp;
	} else if (NIL_P(ttl)) { /* do nothing, no ttl */
	} else
		rb_raise(rb_eTypeError, ":ttl must be a Fixnum or nil");

	if (_ttl == UINT32_MAX)
		rle->e = lwes_emitter_create(
		         _address, _iface, _port, _emit_heartbeat, _freq);
	else
		rle->e = lwes_emitter_create_with_ttl(
		         _address, _iface, _port, _emit_heartbeat, _freq, _ttl);

	if (!rle->e)
		rb_raise(rb_eRuntimeError, "failed to create LWES emitter");

	rle->field_types = rb_hash_new();

	return self;
}

/* Init_lwes_ext will call this */
void init_emitter(void)
{
	VALUE mLWES = rb_define_module("LWES");
	cLWES_Emitter = rb_define_class_under(mLWES, "Emitter", rb_cObject);

	rb_define_method(cLWES_Emitter, "emit", emitter_emit, 2);
	rb_define_method(cLWES_Emitter, "_create", _create, 1);
	rb_define_method(cLWES_Emitter, "close", emitter_close, 0);
	rb_define_alloc_func(cLWES_Emitter, rle_alloc);

	init_numeric();
}

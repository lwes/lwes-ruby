#include "lwes_ruby.h"

static VALUE cLWES_Emitter;
static ID sym_TYPE_DB, sym_TYPE_LIST, sym_OPTIONAL;

/* the underlying struct for LWES::Emitter */
struct _rb_lwes_emitter {
	struct lwes_emitter *emitter;
};

/* gets the _rb_lwes_emitter struct pointer from self */
static struct _rb_lwes_emitter * _rle(VALUE self)
{
	struct _rb_lwes_emitter *rle;

	Data_Get_Struct(self, struct _rb_lwes_emitter, rle);

	return rle;
}

/* GC automatically calls this when object is finalized */
static void rle_free(void *ptr)
{
	struct _rb_lwes_emitter *rle = ptr;

	if (rle->emitter)
		lwes_emitter_destroy(rle->emitter);
	rle->emitter = NULL;
}

/* called by the GC when object is allocated */
static VALUE rle_alloc(VALUE klass)
{
	struct _rb_lwes_emitter *rle;

	return Data_Make_Struct(klass, struct _rb_lwes_emitter,
	                        NULL, rle_free, rle);
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
	struct lwes_event *event = (struct lwes_event *)memo;
	LWES_CONST_SHORT_STRING name;
	int rv = 0;

	assert(TYPE(kv) == T_ARRAY &&
	       "hash iteration not giving key-value pairs");
	tmp = RARRAY_PTR(kv);
	name = RSTRING_PTR(rb_obj_as_string(tmp[0]));

	v = tmp[1];
	switch (TYPE(v)) {
	case T_TRUE:
		rv = lwes_event_set_BOOLEAN(event, name, TRUE);
		break;
	case T_FALSE:
		rv = lwes_event_set_BOOLEAN(event, name, FALSE);
		break;
	case T_ARRAY:
		rv = lwesrb_event_set_numeric(event, name, v);
		break;
	case T_STRING:
		rv = lwes_event_set_STRING(event, name, RSTRING_PTR(v));
		break;
	}
	if (rv > 0)
		return Qnil;
	if (rv == 0)
		rb_raise(rb_eArgError, "unhandled type %s=%s for event=%s",
	                 name, RSTRING_PTR(rb_inspect(v)), event->eventName);
	/* rv < 0 */
	rb_raise(rb_eRuntimeError, "failed to set %s=%s for event=%s",
		 name, RSTRING_PTR(rb_inspect(v)), event->eventName);
	return Qfalse;
}

static VALUE _emit_hash(VALUE _tmp)
{
	VALUE *tmp = (VALUE *)_tmp;
	VALUE self = tmp[0];
	VALUE _event = tmp[1];
	struct lwes_event *event = (struct lwes_event *)tmp[2];

	rb_iterate(rb_each, _event, event_hash_iter_i, (VALUE)event);
	if (lwes_emitter_emit(_rle(self)->emitter, event) < 0)
		rb_raise(rb_eRuntimeError, "failed to emit event");

	return _event;
}

static int set_field(
	struct lwes_event *event,
	LWES_CONST_SHORT_STRING name,
	LWES_TYPE type,
	VALUE val)
{
	switch (type) {
	case LWES_TYPE_BOOLEAN:
		if (val == Qfalse)
			return lwes_event_set_BOOLEAN(event, name, FALSE);
		else if (val == Qtrue)
			return lwes_event_set_BOOLEAN(event, name, TRUE);
		else
			rb_raise(rb_eTypeError, "non-boolean set for %s: %s",
			         name, RSTRING_PTR(rb_inspect(val)));
	case LWES_TYPE_STRING:
		if (TYPE(val) != T_STRING)
			rb_raise(rb_eTypeError, "non-String set for %s: %s",
			         name, RSTRING_PTR(rb_inspect(val)));
		return lwes_event_set_STRING(event, name, RSTRING_PTR(val));
	default:
		return lwesrb_event_set_num(event, name, type, val);
	}
	assert("you should never get here (set_field)");
	return -1;
}

static VALUE _emit_struct(VALUE _argv)
{
	VALUE *argv = (VALUE *)_argv;
	VALUE self = argv[0];
	VALUE _event = argv[1];
	struct lwes_event *event = (struct lwes_event *)argv[2];
	VALUE type_list = rb_const_get(CLASS_OF(_event), SYM2ID(sym_TYPE_LIST));
	VALUE optional = rb_const_get(CLASS_OF(_event), SYM2ID(sym_OPTIONAL));
	long i = RARRAY_LEN(type_list);
	VALUE *tmp;

	for (tmp = RARRAY_PTR(type_list); --i >= 0; tmp++) {
		/* inner: [ :field_sym, "field_name", type ] */
		VALUE *inner = RARRAY_PTR(*tmp);
		VALUE val = rb_struct_aref(_event, inner[0]);
		int rv;
		LWES_CONST_SHORT_STRING name;
		LWES_TYPE type;

		if (NIL_P(val)) {
			if (rb_hash_aref(optional, inner[0]) == Qtrue)
				continue;
			rb_raise(rb_eArgError,
			         "required member %s not set in %s",
			         RSTRING_PTR(inner[1]),
			         RSTRING_PTR(rb_inspect(_event)));
		}

		name = RSTRING_PTR(inner[1]);
		type = NUM2INT(inner[2]);
		rv = set_field(event, name, type, val);
		if (rv > 0)
			continue;

		rb_raise(rb_eRuntimeError,
		         "failed to set %s=%s for event=%s (error: %d)",
		         name, RSTRING_PTR(rb_inspect(val)),
			 event->eventName, rv);
	}

	if (lwes_emitter_emit(_rle(self)->emitter, event) < 0)
		rb_raise(rb_eRuntimeError, "failed to emit event");

	return _event;
}

static VALUE _destroy_event(VALUE _event)
{
	struct lwes_event *event = (struct lwes_event *)_event;

	assert(event && "destroying NULL event");
	lwes_event_destroy(event);

	return Qnil;
}

static VALUE emit_hash(VALUE self, VALUE name, VALUE _event)
{
	VALUE tmp[3];
	struct lwes_event *event = lwes_event_create(NULL, RSTRING_PTR(name));

	if (!event)
		rb_raise(rb_eRuntimeError, "failed to create lwes_event");

	tmp[0] = self;
	tmp[1] = _event;
	tmp[2] = (VALUE)event;
	rb_ensure(_emit_hash, (VALUE)&tmp, _destroy_event, (VALUE)event);

	return _event;
}

static struct lwes_event_type_db * get_type_db(VALUE event)
{
	VALUE type_db = rb_const_get(CLASS_OF(event), SYM2ID(sym_TYPE_DB));

	return lwesrb_get_type_db(type_db);
}

static VALUE emit_struct(VALUE self, VALUE name, VALUE _event)
{
	VALUE argv[3];
	struct lwes_event_type_db *db = get_type_db(_event);
	struct lwes_event *event = lwes_event_create(db, RSTRING_PTR(name));

	if (!event)
		rb_raise(rb_eRuntimeError, "failed to create lwes_event");

	argv[0] = self;
	argv[1] = _event;
	argv[2] = (VALUE)event;
	rb_ensure(_emit_struct, (VALUE)&argv, _destroy_event, (VALUE)event);

	return _event;
}

/*
 * call-seq:
 *   emitter = LWES::Emitter.new
 *   emitter.emit("EventName", :foo => "HI")
 */
static VALUE emitter_emit(VALUE self, VALUE name, VALUE event)
{
	if (TYPE(name) != T_STRING)
		rb_raise(rb_eArgError, "event name must be a String");

	switch (TYPE(event)) {
	case T_HASH:
		return emit_hash(self, name, event);
	case T_STRUCT:
		return emit_struct(self, name, event);
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

	if (rle->emitter)
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
		rle->emitter = lwes_emitter_create(
		         _address, _iface, _port, _emit_heartbeat, _freq);
	else
		rle->emitter = lwes_emitter_create_with_ttl(
		         _address, _iface, _port, _emit_heartbeat, _freq, _ttl);

	if (!rle->emitter)
		rb_raise(rb_eRuntimeError, "failed to create LWES emitter");

	return self;
}

/* Init_lwes_ext will call this */
void lwesrb_init_emitter(void)
{
	VALUE mLWES = rb_define_module("LWES");
	cLWES_Emitter = rb_define_class_under(mLWES, "Emitter", rb_cObject);

	rb_define_method(cLWES_Emitter, "emit", emitter_emit, 2);
	rb_define_method(cLWES_Emitter, "_create", _create, 1);
	rb_define_method(cLWES_Emitter, "close", emitter_close, 0);
	rb_define_alloc_func(cLWES_Emitter, rle_alloc);
	LWESRB_MKSYM(TYPE_DB);
	LWESRB_MKSYM(TYPE_LIST);
	LWESRB_MKSYM(OPTIONAL);
}

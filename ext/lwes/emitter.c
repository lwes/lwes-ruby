#include "lwes_ruby.h"

static VALUE ENC; /* LWES_ENCODING */
static ID id_TYPE_DB, id_TYPE_LIST, id_NAME, id_HAVE_ENCODING;
static ID id_new, id_enc, id_size;
static ID sym_enc;

static void dump_name(VALUE name, LWES_BYTE_P buf, size_t *off)
{
	char *s = RSTRING_PTR(name);

	if (marshall_SHORT_STRING(s, buf, MAX_MSG_SIZE, off) > 0)
		return;
	rb_raise(rb_eRuntimeError, "failed to dump name=%s", s);
}

static int dump_bool(VALUE name, VALUE val, LWES_BYTE_P buf, size_t *off)
{
	LWES_BOOLEAN tmp = FALSE;

	if (val == Qtrue) {
		tmp = TRUE;
	} else if (val != Qfalse) {
		volatile VALUE raise_inspect;

		rb_raise(rb_eTypeError, "non-boolean set for %s: %s",
			 RSTRING_PTR(name),
			 RAISE_INSPECT(val));
	}
	dump_name(name, buf, off);
	lwesrb_dump_type(LWES_BOOLEAN_TOKEN, buf, off);
	return marshall_BOOLEAN(tmp, buf, MAX_MSG_SIZE, off);
}

static int dump_string(VALUE name, VALUE val, LWES_BYTE_P buf, size_t *off)
{
	char *dst;

	switch (TYPE(val)) {
	case T_BIGNUM:
	case T_FIXNUM:
		val = rb_obj_as_string(val);
	}
	dst = StringValuePtr(val);

	dump_name(name, buf, off);
	lwesrb_dump_type(LWES_STRING_TOKEN, buf, off);
	return marshall_LONG_STRING(dst, buf, MAX_MSG_SIZE, off);
}

static void dump_enc(VALUE enc, LWES_BYTE_P buf, size_t *off)
{
	dump_name(ENC, buf, off);
	lwesrb_dump_num(LWES_INT_16_TOKEN, enc, buf, off);
}

static char *my_strdup(const char *str)
{
	long len = strlen(str) + 1;
	char *rv = xmalloc(len);

	memcpy(rv, str, len);

	return rv;
}

/* the underlying struct for LWES::Emitter */
struct _rb_lwes_emitter {
	struct lwes_emitter *emitter;
	char *address;
	char *iface;
	LWES_U_INT_32 port;
	LWES_BOOLEAN emit_heartbeat;
	LWES_INT_16 freq;
	LWES_U_INT_32 ttl;
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
	xfree(rle->address);
	xfree(rle->iface);
	xfree(ptr);
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
	volatile VALUE raise_inspect;
	VALUE *tmp = (VALUE *)memo;
	VALUE val;
	VALUE name;
	int rv = 0;
	LWES_BYTE_P buf = (LWES_BYTE_P)tmp[0];
	size_t *off = (size_t *)tmp[1];

	if (TYPE(kv) != T_ARRAY || RARRAY_LEN(kv) != 2)
		rb_raise(rb_eTypeError,
		         "hash iteration not giving key-value pairs");
	tmp = RARRAY_PTR(kv);
	name = tmp[0];

	if (name == sym_enc) return Qnil; /* already dumped first */

	name = rb_obj_as_string(name);

	if (strcmp(RSTRING_PTR(name), LWES_ENCODING) == 0)
		return Qnil;

	val = tmp[1];

	switch (TYPE(val)) {
	case T_TRUE:
	case T_FALSE:
		rv = dump_bool(name, val, buf, off);
		break;
	case T_ARRAY:
		dump_name(name, buf, off);
		lwesrb_dump_num_ary(val, buf, off);
		return Qnil;
	case T_STRING:
		rv = dump_string(name, val, buf, off);
		break;
	}

	if (rv > 0)
		return Qnil;

	rb_raise(rb_eArgError, "unhandled type %s=%s",
		 RSTRING_PTR(name), RAISE_INSPECT(val));
	return Qfalse;
}

static VALUE emit_hash(VALUE self, VALUE name, VALUE event)
{
	struct _rb_lwes_emitter *rle = _rle(self);
	LWES_BYTE_P buf = rle->emitter->buffer;
	VALUE tmp[2];
	size_t off = 0;
	VALUE enc;
	int size = NUM2INT(rb_funcall(event, id_size, 0, 0));
	int rv;

	tmp[0] = (VALUE)buf;
	tmp[1] = (VALUE)&off;

	if (size < 0 || size > UINT16_MAX)
		rb_raise(rb_eRangeError, "hash size out of uint16 range");

	/* event name first */
	dump_name(name, buf, &off);

	/* number of attributes second */
	rv = marshall_U_INT_16((LWES_U_INT_16)size, buf, MAX_MSG_SIZE, &off);
	if (rv <= 0)
		rb_raise(rb_eRuntimeError, "failed to dump num_attrs");

	/* dump encoding before other fields */
	enc = rb_hash_aref(event, sym_enc);
	if (NIL_P(enc))
		enc = rb_hash_aref(event, ENC);
	if (! NIL_P(enc))
		dump_enc(enc, buf, &off);

	/* the rest of the fields */
	rb_iterate(rb_each, event, event_hash_iter_i, (VALUE)&tmp);

	if (lwes_emitter_emit_bytes(rle->emitter, buf, off) < 0)
		rb_raise(rb_eRuntimeError, "failed to emit event");

	return event;
}

static void
marshal_field(
	VALUE name,
	LWES_TYPE type,
	VALUE val,
	LWES_BYTE_P buf,
	size_t *off)
{
	volatile VALUE raise_inspect;

	switch (type) {
	case LWES_TYPE_STRING:
		if (dump_string(name, val, buf, off) > 0)
			return;
		break;
	case LWES_TYPE_BOOLEAN:
		if (dump_bool(name, val, buf, off) > 0)
			return;
		break;
	default:
		dump_name(name, buf, off);
		lwesrb_dump_num(type, val, buf, off);
		return;
	}

	rb_raise(rb_eRuntimeError, "failed to set %s=%s",
		 RSTRING_PTR(name), RAISE_INSPECT(val));
}

static void lwes_struct_class(
	VALUE *event_class,
	VALUE *name,
	VALUE *type_list,
	VALUE *have_enc,
	VALUE event)
{
	VALUE type_db;

	*event_class = CLASS_OF(event);
	type_db = rb_const_get(*event_class, id_TYPE_DB);

	if (CLASS_OF(type_db) != cLWES_TypeDB)
		rb_raise(rb_eArgError, "class does not have valid TYPE_DB");

	*name = rb_const_get(*event_class, id_NAME);
	Check_Type(*name, T_STRING);
	*type_list = rb_const_get(*event_class, id_TYPE_LIST);
	Check_Type(*type_list, T_ARRAY);

	*have_enc = rb_const_get(*event_class, id_HAVE_ENCODING);
}

static VALUE emit_struct(VALUE self, VALUE event)
{
	VALUE event_class, name, type_list, have_enc;
	struct _rb_lwes_emitter *rle = _rle(self);
	LWES_BYTE_P buf = rle->emitter->buffer;
	size_t off = 0;
	long i;
	VALUE *tmp;
	LWES_U_INT_16 num_attr = 0;
	size_t num_attr_off;
	VALUE *flds;

	lwes_struct_class(&event_class, &name, &type_list, &have_enc, event);

	/* event name */
	dump_name(name, buf, &off);

	/* number of attributes, use a placeholder until we've iterated */
	num_attr_off = off;
	if (marshall_U_INT_16(0, buf, MAX_MSG_SIZE, &off) < 0)
		rb_raise(rb_eRuntimeError,
		         "failed to marshal number_of_attributes");

	/* dump encoding before other fields */
	if (have_enc == Qtrue) {
		VALUE enc = rb_funcall(event, id_enc, 0, 0);
		if (! NIL_P(enc)) {
			++num_attr;
			dump_enc(enc, buf, &off);
		}
	}

	i = RARRAY_LEN(type_list);
	flds = RSTRUCT_PTR(event);
	tmp = RARRAY_PTR(type_list);
	for (; --i >= 0; tmp++, flds++) {
		/* inner: [ :field_sym, "field_name", type ] */
		VALUE *inner = RARRAY_PTR(*tmp);
		VALUE val, name;
		LWES_TYPE type;

		if (inner[0] == sym_enc) /* encoding was already dumped */
			continue;

		val = *flds;
		if (NIL_P(val))
			continue; /* LWES doesn't know nil */

		name = inner[1];
		type = NUM2INT(inner[2]);
		++num_attr;
		marshal_field(name, type, val, buf, &off);
	}

	/* now we've iterated, we can accurately give num_attr */
	if (marshall_U_INT_16(num_attr, buf, MAX_MSG_SIZE, &num_attr_off) <= 0)
		rb_raise(rb_eRuntimeError, "failed to marshal num_attr");

	if (lwes_emitter_emit_bytes(rle->emitter, buf, off) < 0)
		rb_raise(rb_eRuntimeError, "failed to emit event");

	return event;
}

static VALUE emit_event(VALUE self, VALUE event)
{
	struct lwes_event *e = lwesrb_get_event(event);

	if (lwes_emitter_emit(_rle(self)->emitter, e) < 0)
		rb_raise(rb_eRuntimeError, "failed to emit event");

	return event;
}
/*
 * call-seq:
 *   emitter = LWES::Emitter.new
 *   event = EventStruct.new
 *   event.foo = "bar"
 *   emitter << event
 */
static VALUE emitter_ltlt(VALUE self, VALUE event)
{
	if (rb_obj_is_kind_of(event, cLWES_Event)) {
		return emit_event(self, event);
	} else {
		Check_Type(event, T_STRUCT);

		return emit_struct(self, event);
	}
}

/*
 * call-seq:
 *   emitter = LWES::Emitter.new
 *
 *   emitter.emit("EventName", :foo => "HI")
 *
 *   emitter.emit(EventStruct, :foo => "HI")
 *
 *   struct = EventStruct.new
 *   struct.foo = "HI"
 *   emitter.emit(struct)
 */
static VALUE emitter_emit(int argc, VALUE *argv, VALUE self)
{
	volatile VALUE raise_inspect;
	VALUE name = Qnil;
	VALUE event = Qnil;
	argc = rb_scan_args(argc, argv, "11", &name, &event);

	switch (TYPE(name)) {
	case T_STRING:
		if (TYPE(event) == T_HASH)
			return emit_hash(self, name, event);
		rb_raise(rb_eTypeError,
		         "second argument must be a hash when first "
		         "is a String");
	case T_STRUCT:
		if (argc >= 2)
			rb_raise(rb_eArgError,
			         "second argument not allowed when first"
			         " is a Struct");
		event = name;
		return emit_struct(self, event);
	case T_CLASS:
		if (TYPE(event) != T_HASH)
			rb_raise(rb_eTypeError,
			         "second argument must be a Hash when first"
			         " is a Class");

		/*
		 * we can optimize this so there's no intermediate
		 * struct created
		 */
		event = rb_funcall(name, id_new, 1, event);
		return emit_struct(self, event);
	default:
		if (rb_obj_is_kind_of(name, cLWES_Event))
			return emit_event(self, name);
		rb_raise(rb_eArgError,
		         "bad argument: %s, must be a String, Struct or Class",
			 RAISE_INSPECT(name));
	}

	assert(0 && "should never get here");
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
	struct _rb_lwes_emitter *rle = _rle(self);

	if (rle->emitter)
		lwes_emitter_destroy(rle->emitter);
	rle->emitter = NULL;

	return Qnil;
}

static void lwesrb_emitter_create(struct _rb_lwes_emitter *rle)
{
	int gc_retry = 1;
retry:
	if (rle->ttl == UINT32_MAX)
		rle->emitter = lwes_emitter_create(
		         rle->address, rle->iface, rle->port,
			 rle->emit_heartbeat, rle->freq);
	else
		rle->emitter = lwes_emitter_create_with_ttl(
		         rle->address, rle->iface, rle->port,
			 rle->emit_heartbeat, rle->freq, rle->ttl);

	if (!rle->emitter) {
		if (--gc_retry == 0) {
			rb_gc();
			goto retry;
		}
		rb_raise(rb_eRuntimeError, "failed to create LWES emitter");
	}
}

/* :nodoc: */
static VALUE init_copy(VALUE dest, VALUE obj)
{
	struct _rb_lwes_emitter *dst = _rle(dest);
	struct _rb_lwes_emitter *src = _rle(obj);

	memcpy(dst, src, sizeof(*dst));
	dst->address = my_strdup(src->address);
	if (dst->iface)
		dst->iface = my_strdup(src->iface);
	lwesrb_emitter_create(dst);

	assert(dst->emitter && dst->emitter != src->emitter &&
	       "emitter not a copy");

	return dest;
}

/* should only used internally by #initialize */
static VALUE _create(VALUE self, VALUE options)
{
	struct _rb_lwes_emitter *rle = _rle(self);
	VALUE address, iface, port, heartbeat, ttl;

	rle->emit_heartbeat = FALSE;
	rle->freq = 0;
	rle->ttl = UINT32_MAX; /* nobody sets a ttl this long, right? */

	if (rle->emitter)
		rb_raise(rb_eRuntimeError, "already created lwes_emitter");
	if (TYPE(options) != T_HASH)
		rb_raise(rb_eTypeError, "options must be a hash");

	address = rb_hash_aref(options, ID2SYM(rb_intern("address")));
	if (TYPE(address) != T_STRING)
		rb_raise(rb_eTypeError, ":address must be a string");
	rle->address = my_strdup(RSTRING_PTR(address));

	iface = rb_hash_aref(options, ID2SYM(rb_intern("iface")));
	switch (TYPE(iface)) {
	case T_NIL:
		rle->iface = NULL;
		break;
	case T_STRING:
		rle->iface = my_strdup(RSTRING_PTR(iface));
		break;
	default:
		rb_raise(rb_eTypeError, ":iface must be a String or nil");
	}

	port = rb_hash_aref(options, ID2SYM(rb_intern("port")));
	if (TYPE(port) != T_FIXNUM)
		rb_raise(rb_eTypeError, ":port must be a Fixnum");
	rle->port = NUM2UINT(port);

	heartbeat = rb_hash_aref(options, ID2SYM(rb_intern("heartbeat")));
	if (TYPE(heartbeat) == T_FIXNUM) {
		int tmp = NUM2INT(heartbeat);
		if (tmp > INT16_MAX)
			rb_raise(rb_eArgError,":heartbeat > INT16_MAX seconds");
		rle->emit_heartbeat = TRUE;
		rle->freq = (LWES_INT_16)tmp;
	} else if (NIL_P(heartbeat)) { /* do nothing, use defaults */
	} else
		rb_raise(rb_eTypeError, ":heartbeat must be a Fixnum or nil");

	ttl = rb_hash_aref(options, ID2SYM(rb_intern("ttl")));
	if (TYPE(ttl) == T_FIXNUM) {
		unsigned LONG_LONG tmp = NUM2ULL(ttl);
		if (tmp >= UINT32_MAX)
			rb_raise(rb_eArgError, ":ttl >= UINT32_MAX seconds");
		rle->ttl = (LWES_U_INT_32)tmp;
	} else if (NIL_P(ttl)) { /* do nothing, no ttl */
	} else
		rb_raise(rb_eTypeError, ":ttl must be a Fixnum or nil");

	lwesrb_emitter_create(rle);

	return self;
}

/* Init_lwes_ext will call this */
void lwesrb_init_emitter(void)
{
	VALUE mLWES = rb_define_module("LWES");
	VALUE cLWES_Emitter =
	                  rb_define_class_under(mLWES, "Emitter", rb_cObject);

	rb_define_method(cLWES_Emitter, "<<", emitter_ltlt, 1);
	rb_define_method(cLWES_Emitter, "emit", emitter_emit, -1);
	rb_define_method(cLWES_Emitter, "_create", _create, 1);
	rb_define_method(cLWES_Emitter, "close", emitter_close, 0);
	rb_define_method(cLWES_Emitter, "initialize_copy", init_copy, 1);
	rb_define_alloc_func(cLWES_Emitter, rle_alloc);
	LWESRB_MKID(TYPE_DB);
	LWESRB_MKID(TYPE_LIST);
	LWESRB_MKID(NAME);
	LWESRB_MKID(HAVE_ENCODING);
	LWESRB_MKID(new);
	LWESRB_MKID(size);
	id_enc = rb_intern(LWES_ENCODING);
	sym_enc = ID2SYM(id_enc);

	ENC = rb_obj_freeze(rb_str_new2(LWES_ENCODING));
	rb_define_const(mLWES, "ENCODING", ENC);
}

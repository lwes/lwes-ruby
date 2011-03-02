#include "lwes_ruby.h"
VALUE cLWES_Event;

static ID id_TYPE_DB, id_NAME;
static VALUE sym_name;

struct lwes_event * lwesrb_get_event(VALUE self)
{
	struct lwes_event *event;

	Data_Get_Struct(self, struct lwes_event, event);

	return event;
}

static void event_free(void *ptr)
{
	struct lwes_event *event = ptr;

	lwes_event_destroy(event);
}


static VALUE event_alloc(VALUE klass)
{
	struct lwes_event *e;

	if (klass == cLWES_Event) {
		e = lwes_event_create_no_name(NULL);
		if (e == NULL) {
			rb_gc();
			e = lwes_event_create_no_name(NULL);
		}
	} else {
		VALUE type_db = rb_const_get(klass, id_TYPE_DB);
		struct lwes_event_type_db *tdb = lwesrb_get_type_db(type_db);
		VALUE name = rb_const_get(klass, id_NAME);
		const char *ename = StringValueCStr(name);

		e = lwes_event_create(tdb, ename);
		if (e == NULL) {
			rb_gc();
			e = lwes_event_create(tdb, ename);
		}
	}
	if (e == NULL)
		rb_memerror();

	return Data_Wrap_Struct(klass, NULL, event_free, e);
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
	/* TODO */
}

static struct lwes_event_type_db * get_type_db(VALUE self)
{
	VALUE type_db = rb_const_get(CLASS_OF(self), id_TYPE_DB);
	struct lwes_event_type_db *tdb = lwesrb_get_type_db(type_db);

	return tdb;
}

static VALUE event_init(int argc, VALUE *argv, VALUE self)
{
	VALUE hash;
	struct lwes_event *event;

	rb_scan_args(argc, argv, "01", &hash);

	Data_Get_Struct(self, struct lwes_event, event);

	return self;
}

static VALUE event_aset(VALUE self, VALUE key, VALUE val)
{
	struct lwes_event_type_db *tdb = get_type_db(self);
	struct lwes_event *e = lwesrb_get_event(self);
	const char *attr = StringValueCStr(key);
	struct lwes_hash *ehash = lwes_hash_get(tdb->events, e->eventName);
	LWES_BYTE *attr_type;

	if (ehash == NULL)
		rb_raise(rb_eArgError, "invalid event: %s\n", e->eventName);

	attr_type = lwes_hash_get(ehash, attr);
	if (attr_type == NULL)
		rb_raise(rb_eArgError, "invalid attribute: %s\n", attr);

	switch (*attr_type) {
	}
}

static VALUE lwesrb_attr_to_value(struct lwes_event_attribute *attr)
{
	if (attr->type == LWES_STRING_TOKEN) {
		return rb_str_new2((const char *)attr->value);
	} else if (attr->type == LWES_U_INT_16_TOKEN) {
		return UINT2NUM(*((uint16_t *)attr->value));
	} else if (attr->type == LWES_INT_16_TOKEN) {
		return INT2FIX(*((int16_t *)attr->value));
	} else if (attr->type == LWES_U_INT_32_TOKEN) {
		return UINT2NUM(*((uint32_t *)attr->value));
	} else if (attr->type == LWES_INT_32_TOKEN) {
		return INT2NUM(*((int32_t *)attr->value));
	} else if (attr->type == LWES_U_INT_64_TOKEN) {
		return ULL2NUM(*((uint64_t *)attr->value));
	} else if (attr->type == LWES_INT_64_TOKEN) {
		return LL2NUM(*((int64_t *)attr->value));
	} else if (attr->type == LWES_BOOLEAN_TOKEN) {
		LWES_BOOLEAN b = *(LWES_BOOLEAN*)attr->value;
		return b ? Qtrue : Qfalse;
	} else if (attr->type == LWES_IP_ADDR_TOKEN) {
		LWES_IP_ADDR *addr = attr->value;
		VALUE str = rb_str_new(0, INET_ADDRSTRLEN);
		socklen_t len = (socklen_t)INET_ADDRSTRLEN;
		const char *name;

		name = inet_ntop(AF_INET, addr, RSTRING_PTR(str), len);
		if (name == NULL)
			rb_raise(rb_eTypeError, "invalid IP address");
		rb_str_set_len(str, strlen(name));
		return str;
	} else {
		/*
		 * possible event corruption
		 * skip it like the C library does ...
		 */
	}
	return Qnil;
}

/*
 * Returns an LWES::Event object as a plain Ruby hash
 */
static VALUE to_hash(VALUE self)
{
	struct lwes_event *e = lwesrb_get_event(self);

	return lwesrb_event_to_hash(e);
}

/*
 * call-seq:
 *
 *	receiver = UDPSocket.new
 *	receiver.bind(nil, 12345)
 *	buf, addr = receiver.recvfrom(65536)
 *	parsed = LWES::Event.parse(buf)
 *	parsed.to_hash -> hash
 *
 * Parses a string +buf+ and returns a new LWES::Event object
 */
static VALUE parse(VALUE self, VALUE buf)
{
	VALUE event = event_alloc(cLWES_Event);
	struct lwes_event *e = lwesrb_get_event(event);
	struct lwes_event_deserialize_tmp dtmp;
	LWES_BYTE_P bytes;
	size_t num_bytes;
	int rc;

	StringValue(buf);
	bytes = (LWES_BYTE_P)RSTRING_PTR(buf);
	num_bytes = (size_t)RSTRING_LEN(buf);
	rc = lwes_event_from_bytes(e, bytes, num_bytes, 0, &dtmp);
	if (rc < 0)
		rb_raise(rb_eRuntimeError,
		         "failed to parse LWES event (code: %d)\n", rc);
	return event;
}

VALUE lwesrb_event_to_hash(struct lwes_event *e)
{
	VALUE rv = rb_hash_new();
	VALUE val;
	struct lwes_hash_enumeration hen;
	LWES_SHORT_STRING name;
	VALUE sym_attr_name;
	struct lwes_event_attribute *attr;

	if (e->eventName != NULL) {
		val = rb_str_new2(e->eventName);
		rb_hash_aset(rv, sym_name, val);
	}

	if (! lwes_hash_keys(e->attributes, &hen))
		return rv;
	while (lwes_hash_enumeration_has_more_elements(&hen)) {
		name = lwes_hash_enumeration_next_element(&hen);
		sym_attr_name = ID2SYM(rb_intern(name));
		attr = lwes_hash_get(e->attributes, name);
		val = lwesrb_attr_to_value(attr);
		if (! NIL_P(val))
			rb_hash_aset(rv, sym_attr_name, val);
	}

	return rv;
}

void lwesrb_init_event(void)
{
	VALUE mLWES = rb_define_module("LWES");
	cLWES_Event = rb_define_class_under(mLWES, "Event", rb_cObject);

	rb_define_method(cLWES_Event, "initialize", event_init, -1);
	rb_define_alloc_func(cLWES_Event, event_alloc);
	rb_define_singleton_method(cLWES_Event, "parse", parse, 1);
	rb_define_method(cLWES_Event, "to_hash", to_hash, 0);

	LWESRB_MKID(TYPE_DB);
	LWESRB_MKID(NAME);
	LWESRB_MKSYM(name);
}

#include "lwes_ruby.h"
VALUE cLWES_Event;

static VALUE tmp_class_name;
static VALUE SYM2ATTR, CLASSES;
static ID id_TYPE_DB, id_NAME;
static VALUE sym_name;
static VALUE lwesrb_attr_to_value(struct lwes_event_attribute *attr);

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

static struct lwes_event_type_db * get_type_db(VALUE self)
{
	VALUE type_db = rb_const_get(CLASS_OF(self), id_TYPE_DB);

	return lwesrb_get_type_db(type_db);
}

static LWES_BYTE get_attr_type(VALUE self, char *attr)
{
	struct lwes_event_type_db *tdb = get_type_db(self);
	struct lwes_event *e = lwesrb_get_event(self);
	struct lwes_hash *ehash = lwes_hash_get(tdb->events, e->eventName);
	LWES_BYTE *attr_type;

	if (ehash == NULL)
		rb_raise(rb_eArgError, "invalid event: %s", e->eventName);

	attr_type = lwes_hash_get(ehash, attr);
	if (attr_type)
		return *attr_type;

	ehash = lwes_hash_get(tdb->events, LWES_META_INFO_STRING);
	if (ehash == NULL)
		rb_raise(rb_eArgError, "%s not found", LWES_META_INFO_STRING);

	attr_type = lwes_hash_get(ehash, attr);
	if (attr_type)
		return *attr_type;

	rb_raise(rb_eArgError, "invalid attribute: %s", attr);
}

static char *key2attr(VALUE key)
{
	if (TYPE(key) == T_SYMBOL)
		key = rb_hash_aref(SYM2ATTR, key);
	return StringValueCStr(key);
}

static VALUE event_aref(VALUE self, VALUE key)
{
	char *attr = key2attr(key);
	struct lwes_event *e = lwesrb_get_event(self);
	struct lwes_event_attribute *eattr;

	eattr = lwes_hash_get(e->attributes, attr);
	return eattr ? lwesrb_attr_to_value(eattr) : Qnil;
}

static VALUE event_aset(VALUE self, VALUE key, VALUE val)
{
	char *attr = key2attr(key);
	LWES_BYTE attr_type = get_attr_type(self, attr);
	struct lwes_event *e = lwesrb_get_event(self);

	if (attr_type == LWES_STRING_TOKEN) {
		lwes_event_set_STRING(e, attr, StringValueCStr(val));
	} else if (attr_type == LWES_U_INT_16_TOKEN) {
		lwes_event_set_U_INT_16(e, attr, lwesrb_uint16(val));
	} else if (attr_type == LWES_INT_16_TOKEN) {
		lwes_event_set_INT_16(e, attr, lwesrb_int16(val));
	} else if (attr_type == LWES_U_INT_32_TOKEN) {
		lwes_event_set_U_INT_32(e, attr, lwesrb_uint32(val));
	} else if (attr_type == LWES_INT_32_TOKEN) {
		lwes_event_set_INT_32(e, attr, lwesrb_int32(val));
	} else if (attr_type == LWES_U_INT_64_TOKEN) {
		lwes_event_set_U_INT_64(e, attr, lwesrb_uint64(val));
	} else if (attr_type == LWES_INT_64_TOKEN) {
		lwes_event_set_INT_64(e, attr, lwesrb_int64(val));
	} else if (attr_type == LWES_IP_ADDR_TOKEN) {
		lwes_event_set_IP_ADDR(e, attr, lwesrb_ip_addr(val));
	} else if (attr_type == LWES_BOOLEAN_TOKEN) {
		lwes_event_set_BOOLEAN(e, attr, lwesrb_boolean(val));
	} else {
		rb_raise(rb_eRuntimeError,
			 "unknown LWES attribute type: 0x%02x",
			 (unsigned)attr_type);
	}
	return val;
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
		rb_warn("possible event corruption: attr->type=%02x",
		        (unsigned)attr->type);
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
	VALUE rv = rb_hash_new();
	VALUE val;
	struct lwes_hash_enumeration hen;
	LWES_SHORT_STRING name;
	VALUE sym_attr_name;
	struct lwes_event_attribute *attr;

	if (e->eventName != NULL && CLASS_OF(self) == cLWES_Event) {
		val = rb_str_new2(e->eventName);
		rb_hash_aset(rv, sym_name, val);
	}

	if (! lwes_hash_keys(e->attributes, &hen))
		return rv;
	while (lwes_hash_enumeration_has_more_elements(&hen)) {
		name = lwes_hash_enumeration_next_element(&hen);
		sym_attr_name = ID2SYM(rb_intern(name));
		attr = lwes_hash_get(e->attributes, name);
		if (attr == NULL)
			rb_raise(rb_eRuntimeError,
			         "missing attr during enumeration: %s", name);
		val = lwesrb_attr_to_value(attr);
		if (! NIL_P(val))
			rb_hash_aset(rv, sym_attr_name, val);
	}

	return rv;
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
	struct lwes_event *e;
	struct lwes_event_deserialize_tmp dtmp;
	LWES_BYTE_P bytes;
	size_t num_bytes;
	int rc;

	StringValue(buf);
	bytes = (LWES_BYTE_P)RSTRING_PTR(buf);
	num_bytes = (size_t)RSTRING_LEN(buf);
	e = lwes_event_create_no_name(NULL);
	rc = lwes_event_from_bytes(e, bytes, num_bytes, 0, &dtmp);
	if (rc < 0) {
		lwes_event_destroy(e);
		rb_raise(rb_eRuntimeError,
		         "failed to parse LWES event (code: %d)", rc);
	}
	if (e->eventName) {
		long len = strlen(e->eventName);
		VALUE tmp;

		rb_str_resize(tmp_class_name, len);
		memcpy(RSTRING_PTR(tmp_class_name), e->eventName, len);
		tmp = rb_hash_aref(CLASSES, tmp_class_name);
		if (tmp != Qnil)
			self = tmp;
	}

	return Data_Wrap_Struct(self, NULL, event_free, e);
}

void lwesrb_init_event(void)
{
	VALUE mLWES = rb_define_module("LWES");
	cLWES_Event = rb_define_class_under(mLWES, "Event", rb_cObject);

	SYM2ATTR = rb_const_get(cLWES_Event, rb_intern("SYM2ATTR"));
	CLASSES = rb_const_get(cLWES_Event, rb_intern("CLASSES"));
	rb_define_alloc_func(cLWES_Event, event_alloc);
	rb_define_singleton_method(cLWES_Event, "parse", parse, 1);
	rb_define_method(cLWES_Event, "to_hash", to_hash, 0);
	rb_define_method(cLWES_Event, "[]", event_aref, 1);
	rb_define_method(cLWES_Event, "[]=", event_aset, 2);
	tmp_class_name = rb_str_new(0, 0);
	rb_global_variable(&tmp_class_name);

	LWESRB_MKID(TYPE_DB);
	LWESRB_MKID(NAME);
	LWESRB_MKSYM(name);
}

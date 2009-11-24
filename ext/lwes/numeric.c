#include "numeric.h"
#include <arpa/inet.h>

static ID
  id_int16, id_uint16,
  id_int32, id_uint32,
  id_int64, id_uint64,
  id_ip_addr;

static int set_uint16(
	struct lwes_event *event, LWES_CONST_SHORT_STRING name, VALUE val)
{
	int32_t tmp = NUM2INT(val);

	if (tmp < 0)
		rb_raise(rb_eRangeError, ":uint16 negative: %d", tmp);
	if (tmp > UINT16_MAX)
		rb_raise(rb_eRangeError, ":uint16 too large: %d", tmp);

	return lwes_event_set_U_INT_16(event, name, (LWES_U_INT_16)tmp);
}

static int set_int16(
	struct lwes_event *event, LWES_CONST_SHORT_STRING name, VALUE val)
{
	int tmp = NUM2INT(val);

	if (tmp > INT16_MAX)
		rb_raise(rb_eRangeError, ":int16 too large: %i", tmp);
	if (tmp < INT16_MIN)
		rb_raise(rb_eRangeError, ":int16 too small: %i", tmp);

	return lwes_event_set_INT_16(event, name, (LWES_INT_16)tmp);
}

static int set_uint32(
	struct lwes_event *event, LWES_CONST_SHORT_STRING name, VALUE val)
{
	LONG_LONG tmp = NUM2LL(val);

	if (tmp < 0)
		rb_raise(rb_eRangeError, ":uint32 negative: %lli", tmp);
	if (tmp > UINT32_MAX)
		rb_raise(rb_eRangeError, ":uint32 too large: %lli", tmp);

	return lwes_event_set_U_INT_32(event, name, (LWES_U_INT_32)tmp);
}

static int set_int32(
	struct lwes_event *event, LWES_CONST_SHORT_STRING name, VALUE val)
{
	LONG_LONG tmp = NUM2LL(val);

	if (tmp > INT32_MAX)
		rb_raise(rb_eRangeError, ":int32 too large: %lli", tmp);
	if (tmp < INT32_MIN)
		rb_raise(rb_eRangeError, ":int32 too small: %lli", tmp);

	return lwes_event_set_INT_32(event, name, (LWES_INT_32)tmp);
}

static int set_uint64(
	struct lwes_event *event, LWES_CONST_SHORT_STRING name, VALUE val)
{
	unsigned LONG_LONG tmp = NUM2ULL(val); /* can raise RangeError */
	ID type = TYPE(val);

	if ((type == T_FIXNUM && FIX2LONG(val) < 0) ||
	    (type == T_BIGNUM && RTEST(rb_funcall(val, '<', 1, INT2FIX(0)))))
		rb_raise(rb_eRangeError, ":uint64 negative: %s",
		         RSTRING_PTR(rb_inspect(val)));

	return lwes_event_set_U_INT_64(event, name, (LWES_U_INT_64)tmp);
}

static int set_int64(
	struct lwes_event *event, LWES_CONST_SHORT_STRING name, VALUE val)
{
	LONG_LONG tmp = NUM2LL(val); /* can raise RangeError */

	return lwes_event_set_INT_64(event, name, (LWES_INT_64)tmp);
}

static int set_ip_addr(
	struct lwes_event *event, LWES_CONST_SHORT_STRING name, VALUE val)
{
	switch (TYPE(val)) {
	case T_STRING:
	{
		LWES_CONST_SHORT_STRING addr = RSTRING_PTR(val);
		return lwes_event_set_IP_ADDR_w_string(event, name, addr);
	}
	case T_FIXNUM:
	case T_BIGNUM:
	{
		LWES_IP_ADDR *addr = ALLOC(LWES_IP_ADDR); /* never fails */
		int rv;

		addr->s_addr = htonl(NUM2UINT(val));
		rv = lwes_event_set_IP_ADDR(event, name, *addr);

		if (rv < 0)
			xfree(addr);
		return rv;
	}
	default:
		rb_raise(rb_eTypeError,
		         ":ip_addr address must be String or Integer: %s",
		         RSTRING_PTR(rb_inspect(val)));
	}
}

/* simple type => function dispatch map */
static struct _type_fn_map {
	ID type;
	int (*fn)(struct lwes_event *, LWES_CONST_SHORT_STRING, VALUE);
} type_fn_map[] = {
#define IDFN(T) { (ID)&id_##T, set_##T }
	IDFN(uint16),
	IDFN(int16),
	IDFN(uint32),
	IDFN(int32),
	IDFN(uint64),
	IDFN(int64),
	IDFN(ip_addr),
#undef IDFN
};

int lwesrb_event_set_num(
	struct lwes_event *event,
	LWES_CONST_SHORT_STRING name,
	LWES_TYPE type,
	VALUE val)
{
	switch (type) {
	case LWES_TYPE_U_INT_16: return set_uint16(event, name, val);
	case LWES_TYPE_INT_16: return set_int16(event, name, val);
	case LWES_TYPE_U_INT_32: return set_uint32(event, name, val);
	case LWES_TYPE_INT_32: return set_int32(event, name, val);
	case LWES_TYPE_U_INT_64: return set_uint64(event, name, val);
	case LWES_TYPE_INT_64: return set_int64(event, name, val);
	case LWES_TYPE_IP_ADDR: return set_ip_addr(event, name, val);
	default:
		rb_raise(rb_eRuntimeError,
			 "unknown LWES attribute type: 0x%02x", type);
	}
	assert("you should never get here (set_field)");
	return -1;
}

/*
 * array contains two elements:
 *   [ symbolic_type, number ]
 * returns the return value of the underlying lwes_event_set_* call
 */
int lwesrb_event_set_numeric(
	struct lwes_event *event,
	LWES_CONST_SHORT_STRING name,
	VALUE array)
{
	int i;
	struct _type_fn_map *head;
	VALUE *ary;
	ID type;

	assert(TYPE(array) == T_ARRAY && "need array here");

	if (RARRAY_LEN(array) != 2)
		rb_raise(rb_eArgError, "expected a two element array");

	ary = RARRAY_PTR(array);
	type = ary[0];

	i = sizeof(type_fn_map) / sizeof(type_fn_map[0]);
	for (head = type_fn_map; --i >= 0; head++) {
		if (head->type == type)
			return head->fn(event, name, ary[1]);
	}

	rb_raise(rb_eArgError,
	         "unknown type: %s",
	         RSTRING_PTR(rb_inspect(type)));

	return -1;
}

void init_numeric(void)
{
	int i;

#define MKID(T) id_##T = ID2SYM(rb_intern(#T))
	MKID(int16);
	MKID(uint16);
	MKID(int32);
	MKID(uint32);
	MKID(int64);
	MKID(uint64);
	MKID(ip_addr);
#undef MKID

	/*
	 * we needed to have constants for compilation, so we set the
	 * address of the IDs and then dereference + reassign them
	 * at initialization time:
	 */
	i = sizeof(type_fn_map) / sizeof(type_fn_map[0]);
	while (--i >= 0)
		type_fn_map[i].type = *(ID *)type_fn_map[i].type;
}

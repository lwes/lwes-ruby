#include "lwes_ruby.h"
#include <arpa/inet.h>

static ID
  sym_int16, sym_uint16,
  sym_int32, sym_uint32,
  sym_int64, sym_uint64,
  sym_ip_addr;

void lwesrb_dump_type(LWES_BYTE type, LWES_BYTE_P buf, size_t *off)
{
	if (marshall_BYTE(type, buf, MAX_MSG_SIZE, off) > 0)
		return;
	rb_raise(rb_eRuntimeError, "failed to dump type=%02x", (unsigned)type);
}

static int dump_uint16(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	int32_t tmp = NUM2INT(val);

	if (tmp < 0)
		rb_raise(rb_eRangeError, ":uint16 negative: %d", tmp);
	if (tmp > UINT16_MAX)
		rb_raise(rb_eRangeError, ":uint16 too large: %d", tmp);

	lwesrb_dump_type(LWES_U_INT_16_TOKEN, buf, off);
	return marshall_U_INT_16((LWES_U_INT_16)tmp, buf, MAX_MSG_SIZE, off);
}

static int dump_int16(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	int32_t tmp = NUM2INT(val);

	if (tmp > INT16_MAX)
		rb_raise(rb_eRangeError, ":int16 too large: %i", tmp);
	if (tmp < INT16_MIN)
		rb_raise(rb_eRangeError, ":int16 too small: %i", tmp);

	lwesrb_dump_type(LWES_INT_16_TOKEN, buf, off);
	return marshall_INT_16((LWES_INT_16)tmp, buf, MAX_MSG_SIZE, off);
}

static int dump_uint32(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	LONG_LONG tmp = NUM2LL(val);

	if (tmp < 0)
		rb_raise(rb_eRangeError, ":uint32 negative: %lli", tmp);
	if (tmp > UINT32_MAX)
		rb_raise(rb_eRangeError, ":uint32 too large: %lli", tmp);

	lwesrb_dump_type(LWES_U_INT_32_TOKEN, buf, off);
	return marshall_U_INT_32((LWES_U_INT_32)tmp, buf, MAX_MSG_SIZE, off);
}

static int dump_int32(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	LONG_LONG tmp = NUM2LL(val);

	if (tmp > INT32_MAX)
		rb_raise(rb_eRangeError, ":int32 too large: %lli", tmp);
	if (tmp < INT32_MIN)
		rb_raise(rb_eRangeError, ":int32 too small: %lli", tmp);

	lwesrb_dump_type(LWES_INT_32_TOKEN, buf, off);
	return marshall_INT_32((LWES_INT_32)tmp, buf, MAX_MSG_SIZE, off);
}

static int dump_uint64(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	unsigned LONG_LONG tmp = NUM2ULL(val); /* can raise RangeError */
	ID type = TYPE(val);

	if ((type == T_FIXNUM && FIX2LONG(val) < 0) ||
	    (type == T_BIGNUM && RTEST(rb_funcall(val, '<', 1, INT2FIX(0)))))
		rb_raise(rb_eRangeError, ":uint64 negative: %s",
		         RSTRING_PTR(rb_inspect(val)));

	lwesrb_dump_type(LWES_U_INT_64_TOKEN, buf, off);
	return marshall_U_INT_64((LWES_U_INT_64)tmp, buf, MAX_MSG_SIZE, off);
}

static int dump_int64(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	LONG_LONG tmp = NUM2LL(val); /* can raise RangeError */

	lwesrb_dump_type(LWES_INT_64_TOKEN, buf, off);
	return marshall_INT_64((LWES_INT_64)tmp, buf, MAX_MSG_SIZE, off);
}

static int dump_ip_addr(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	LWES_IP_ADDR addr;

	switch (TYPE(val)) {
	case T_STRING:
		addr.s_addr = inet_addr(RSTRING_PTR(val));
		break;
	case T_FIXNUM:
	case T_BIGNUM:
		addr.s_addr = htonl(NUM2UINT(val));
		break;
	default:
		rb_raise(rb_eTypeError,
		         ":ip_addr address must be String or Integer: %s",
		         RSTRING_PTR(rb_inspect(val)));
	}
	lwesrb_dump_type(LWES_IP_ADDR_TOKEN, buf, off);
	return marshall_IP_ADDR(addr, buf, MAX_MSG_SIZE, off);
}

/* simple type => function dispatch map */
static struct _type_fn_map {
	ID type;
	int (*fn)(VALUE, LWES_BYTE_P, size_t *);
} type_fn_map[] = {
#define SYMFN(T) { (ID)&sym_##T, dump_##T }
	SYMFN(uint16),
	SYMFN(int16),
	SYMFN(uint32),
	SYMFN(int32),
	SYMFN(uint64),
	SYMFN(int64),
	SYMFN(ip_addr),
#undef SYMFN
};

/* used for Struct serialization where types are known ahead of time */
static int dump_num(
	LWES_TYPE type,
	VALUE val,
	LWES_BYTE_P buf,
	size_t *off)
{
	switch (type) {
	case LWES_TYPE_U_INT_16: return dump_uint16(val, buf, off);
	case LWES_TYPE_INT_16: return dump_int16(val, buf, off);
	case LWES_TYPE_U_INT_32: return dump_uint32(val, buf, off);
	case LWES_TYPE_INT_32: return dump_int32(val, buf, off);
	case LWES_TYPE_U_INT_64: return dump_uint64(val, buf, off);
	case LWES_TYPE_INT_64: return dump_int64(val, buf, off);
	case LWES_TYPE_IP_ADDR: return dump_ip_addr(val, buf, off);
	default:
		rb_raise(rb_eRuntimeError,
			 "unknown LWES attribute type: 0x%02x", type);
	}
	assert("you should never get here (dump_num)");
	return -1;
}

void lwesrb_dump_num(LWES_BYTE type, VALUE val, LWES_BYTE_P buf, size_t *off)
{
	if (dump_num(type, val, buf, off) > 0)
		return;
	rb_raise(rb_eRuntimeError,
	         "dumping numeric type 0x%02x, type failed", type);
}

/*
 * used for Hash serialization
 * array contains two elements:
 *   [ symbolic_type, number ]
 * returns the return value of the underlying lwes_event_set_* call
 */
void lwesrb_dump_num_ary(VALUE array, LWES_BYTE_P buf, size_t *off)
{
	int i, rv;
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
		if (head->type != type)
			continue;

		rv = head->fn(ary[1], buf, off);
		if (rv > 0)
			return;
		rb_raise(rb_eRuntimeError,
			 "dumping numeric type %s, type failed",
			 RSTRING_PTR(rb_obj_as_string(type)));
	}

	rb_raise(rb_eArgError,
	         "unknown type: %s", RSTRING_PTR(rb_inspect(type)));
}

void lwesrb_init_numeric(void)
{
	int i;

	LWESRB_MKSYM(int16);
	LWESRB_MKSYM(uint16);
	LWESRB_MKSYM(int32);
	LWESRB_MKSYM(uint32);
	LWESRB_MKSYM(int64);
	LWESRB_MKSYM(uint64);
	LWESRB_MKSYM(ip_addr);

	/*
	 * we needed to have constants for compilation, so we set the
	 * address of the IDs and then dereference + reassign them
	 * at initialization time:
	 */
	i = sizeof(type_fn_map) / sizeof(type_fn_map[0]);
	while (--i >= 0)
		type_fn_map[i].type = *(ID *)type_fn_map[i].type;
}

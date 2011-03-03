#include "lwes_ruby.h"
#include <arpa/inet.h>

static ID
  sym_int16, sym_uint16,
  sym_int32, sym_uint32,
  sym_int64, sym_uint64,
  sym_ip_addr;
static ID id_to_i;

static int dump_uint16(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	lwesrb_dump_type(LWES_U_INT_16_TOKEN, buf, off);
	return marshall_U_INT_16(lwesrb_uint16(val), buf, MAX_MSG_SIZE, off);
}

static int dump_int16(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	lwesrb_dump_type(LWES_INT_16_TOKEN, buf, off);
	return marshall_INT_16(lwesrb_int16(val), buf, MAX_MSG_SIZE, off);
}

static int dump_uint32(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	lwesrb_dump_type(LWES_U_INT_32_TOKEN, buf, off);
	return marshall_U_INT_32(lwesrb_uint32(val), buf, MAX_MSG_SIZE, off);
}

static int dump_int32(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	lwesrb_dump_type(LWES_INT_32_TOKEN, buf, off);
	return marshall_INT_32(lwesrb_int32(val), buf, MAX_MSG_SIZE, off);
}

static int dump_uint64(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	lwesrb_dump_type(LWES_U_INT_64_TOKEN, buf, off);
	return marshall_U_INT_64(lwesrb_uint64(val), buf, MAX_MSG_SIZE, off);
}

static int dump_int64(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	lwesrb_dump_type(LWES_INT_64_TOKEN, buf, off);
	return marshall_INT_64(lwesrb_int64(val), buf, MAX_MSG_SIZE, off);
}

static int dump_ip_addr(VALUE val, LWES_BYTE_P buf, size_t *off)
{
	lwesrb_dump_type(LWES_IP_ADDR_TOKEN, buf, off);
	return marshall_IP_ADDR(lwesrb_ip_addr(val), buf, MAX_MSG_SIZE, off);
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
	if (type != LWES_TYPE_IP_ADDR && TYPE(val) == T_STRING)
		val = rb_funcall(val, id_to_i, 0, 0);

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
	volatile VALUE raise_inspect;
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
			 RAISE_INSPECT(type));
	}

	rb_raise(rb_eArgError, "unknown type: %s", RAISE_INSPECT(type));
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
	id_to_i = rb_intern("to_i");

	/*
	 * we needed to have constants for compilation, so we set the
	 * address of the IDs and then dereference + reassign them
	 * at initialization time:
	 */
	i = sizeof(type_fn_map) / sizeof(type_fn_map[0]);
	while (--i >= 0)
		type_fn_map[i].type = *(ID *)type_fn_map[i].type;
}

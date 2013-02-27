#include "lwes_ruby.h"

void lwesrb_dump_type(LWES_BYTE type, LWES_BYTE_P buf, size_t *off)
{
	if (marshall_BYTE(type, buf, MAX_MSG_SIZE, off) > 0)
		return;
	rb_raise(rb_eRuntimeError, "failed to dump type=%02x", (unsigned)type);
}

LWES_U_INT_16 lwesrb_uint16(VALUE val)
{
	int32_t tmp = NUM2INT(val);

	if (tmp != (LWES_U_INT_16)tmp) {
		const char *s = tmp < 0 ? "negative" : "too big";
		rb_raise(rb_eRangeError, ":uint16 %s: %d", s, tmp);
	}
	return (LWES_U_INT_16)tmp;
}

LWES_INT_16 lwesrb_int16(VALUE val)
{
	int32_t tmp = NUM2INT(val);

	if (tmp != (LWES_INT_16)tmp) {
		const char *s = tmp < 0 ? "small" : "big";
		rb_raise(rb_eRangeError, ":int16 too %s: %d", s, tmp);
	}
	return (LWES_INT_16)tmp;
}

LWES_U_INT_32 lwesrb_uint32(VALUE val)
{
	LONG_LONG tmp = NUM2LL(val);

	if (tmp != (LWES_U_INT_32)tmp) {
		const char *s = tmp < 0 ? "negative" : "too big";
		rb_raise(rb_eRangeError, ":uint32 %s: %lld", s, tmp);
	}
	return (LWES_U_INT_32)tmp;
}

LWES_INT_32 lwesrb_int32(VALUE val)
{
	LONG_LONG tmp = NUM2LL(val);

	if (tmp != (LWES_INT_32)tmp) {
		const char *s = tmp < 0 ? "small" : "big";
		rb_raise(rb_eRangeError, ":int32 too %s: %lld", s, tmp);
	}
	return (LWES_INT_32)tmp;
}

LWES_U_INT_64 lwesrb_uint64(VALUE val)
{
	unsigned LONG_LONG tmp = NUM2ULL(val); /* can raise RangeError */
	ID type = TYPE(val);

	if ((type == T_FIXNUM && FIX2LONG(val) < 0) ||
	    (type == T_BIGNUM && RTEST(rb_funcall(val, '<', 1, INT2FIX(0))))) {
		volatile VALUE raise_inspect;

		rb_raise(rb_eRangeError, ":uint64 negative: %s",
		         RAISE_INSPECT(val));
	}
	return (LWES_U_INT_64)tmp;
}

LWES_IP_ADDR lwesrb_ip_addr(VALUE val)
{
	LWES_IP_ADDR addr;
	volatile VALUE raise_inspect;

	switch (TYPE(val)) {
	case T_STRING:
		addr.s_addr = inet_addr(StringValueCStr(val));
		break;
	case T_FIXNUM:
	case T_BIGNUM:
		addr.s_addr = htonl(NUM2UINT(val));
		break;
	default:
		rb_raise(rb_eTypeError,
		         ":ip_addr address must be String or Integer: %s",
		         RAISE_INSPECT(val));
	}

	return addr;
}

LWES_BOOLEAN lwesrb_boolean(VALUE val)
{
	if (val == Qtrue) return TRUE;
        if (val != Qfalse) {
		volatile VALUE raise_inspect;
		rb_raise(rb_eTypeError, "non-boolean: %s", RAISE_INSPECT(val));
	}
	return FALSE;
}

#include <lwes.h>
#include <ruby.h>
#include <assert.h>

void init_emitter(void); /* emitter.c */
void init_type_db(void); /* type_db.c */

static VALUE mLWES;

/* initialize the extension, Ruby automatically picks this up */
void Init_lwes_ext(void)
{
	mLWES = rb_define_module("LWES");

	#define LWES_TYPE_CONST(name) \
	   rb_define_const(mLWES, #name, INT2FIX(LWES_TYPE_##name))
	LWES_TYPE_CONST(U_INT_16);
	LWES_TYPE_CONST(INT_16);
	LWES_TYPE_CONST(U_INT_32);
	LWES_TYPE_CONST(INT_32);
	LWES_TYPE_CONST(U_INT_64);
	LWES_TYPE_CONST(INT_64);
	LWES_TYPE_CONST(BOOLEAN);
	LWES_TYPE_CONST(IP_ADDR);
	LWES_TYPE_CONST(STRING);
	LWES_TYPE_CONST(UNDEFINED);
	#undef LWES_TYPE_CONST

	init_emitter();
	init_type_db();
}

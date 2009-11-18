#include <lwes.h>
#include <ruby.h>
#include <assert.h>

void init_emitter(void); /* emitter.c */

static VALUE mLWES;

/* initialize the extension, Ruby automatically picks this up */
void Init_lwes_ext(void)
{
	mLWES = rb_define_module("LWES");
	init_emitter();
}

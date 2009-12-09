#ifndef LWES_RUBY_H
#define LWES_RUBY_H

#include <lwes.h>
#include <ruby.h>
#include <assert.h>
#include <stdint.h>

#define LWESRB_MKSYM(SYM) sym_##SYM = ID2SYM(rb_intern(#SYM))
#define LWESRB_MKID(NAME) id_##NAME = rb_intern(#NAME)

struct lwes_event_type_db * lwesrb_get_type_db(VALUE self);

void lwesrb_init_type_db(void);

void lwesrb_init_emitter(void);

void lwesrb_init_numeric(void);

int lwesrb_event_set_numeric(
	struct lwes_event *event,
	LWES_CONST_SHORT_STRING name,
	VALUE array);

int lwesrb_event_set_num(
	struct lwes_event *event,
	LWES_CONST_SHORT_STRING name,
	LWES_TYPE type,
	VALUE val);

#ifndef RSTRING_PTR
#  define RSTRING_PTR(s) (RSTRING(s)->ptr)
#  define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#ifndef RARRAY_PTR
#  define RARRAY_PTR(s) (RARRAY(s)->ptr)
#  define RARRAY_LEN(s) (RARRAY(s)->len)
#endif

#endif /* LWES_RUBY_H */

#ifndef LWES_RUBY_H
#define LWES_RUBY_H

#include <lwes.h>
#include <ruby.h>
#include <assert.h>
#include <stdint.h>

#define LWESRB_MKSYM(SYM) sym_##SYM = ID2SYM(rb_intern(#SYM))
#define LWESRB_MKID(NAME) id_##NAME = rb_intern(#NAME)

extern VALUE cLWES_TypeDB;

struct lwes_event_type_db * lwesrb_get_type_db(VALUE self);

void lwesrb_init_type_db(void);

void lwesrb_init_emitter(void);

void lwesrb_init_numeric(void);

void lwesrb_dump_type(LWES_BYTE type, LWES_BYTE_P buf, size_t *off);

void lwesrb_dump_num(LWES_BYTE type, VALUE val, LWES_BYTE_P buf, size_t *off);

void lwesrb_dump_num_ary(VALUE array, LWES_BYTE_P buf, size_t *off);

#ifndef RSTRING_PTR
#  define RSTRING_PTR(s) (RSTRING(s)->ptr)
#  define RSTRING_LEN(s) (RSTRING(s)->len)
#endif

#ifndef RARRAY_PTR
#  define RARRAY_PTR(s) (RARRAY(s)->ptr)
#  define RARRAY_LEN(s) (RARRAY(s)->len)
#endif

#ifndef RSTRUCT_PTR
#  define RSTRUCT_PTR(s) (RSTRUCT(s)->ptr)
#  define RSTRUCT_LEN(s) (RSTRUCT(s)->len)
#endif

#define RAISE_INSPECT(v) RSTRING_PTR(raise_inspect = rb_inspect(v))

#endif /* LWES_RUBY_H */

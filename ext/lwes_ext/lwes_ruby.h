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

void lwesrb_init_event(void);

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

extern VALUE cLWES_Event;

struct lwes_event * lwesrb_get_event(VALUE self);

VALUE lwesrb_event_to_hash(struct lwes_event *e);

LWES_U_INT_16 lwesrb_uint16(VALUE val);
LWES_INT_16 lwesrb_int16(VALUE val);
LWES_U_INT_32 lwesrb_uint32(VALUE val);
LWES_INT_32 lwesrb_int32(VALUE val);
LWES_U_INT_64 lwesrb_uint64(VALUE val);
LWES_INT_64 lwesrb_int64(VALUE val);
LWES_IP_ADDR lwesrb_ip_addr(VALUE val);
LWES_BOOLEAN lwesrb_boolean(VALUE val);

#endif /* LWES_RUBY_H */

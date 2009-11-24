#ifndef LWES_RUBY_H
#define LWES_RUBY_H

#include <lwes.h>
#include <ruby.h>
#include <assert.h>
#include <stdint.h>

#define LWESRB_MKSYM(SYM) sym_##SYM = ID2SYM(rb_intern(#SYM))

struct lwes_event_type_db * lwesrb_get_type_db(VALUE self);

#include "numeric.h"

#endif /* LWES_RUBY_H */

#ifndef NUMERIC_H
#define NUMERIC_H

#include "lwes_ruby.h"

void init_numeric(void);

int lwesrb_event_set_numeric(
	struct lwes_event *event,
	LWES_CONST_SHORT_STRING name,
	VALUE array);

int lwesrb_event_set_num(
	struct lwes_event *event,
	LWES_CONST_SHORT_STRING name,
	LWES_TYPE type,
	VALUE val);

#endif /* NUMERIC_H */

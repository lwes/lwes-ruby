static VALUE cLWES_Emitter;

/* the underlying struct for LWES::Emitter */
struct _rb_lwes_emitter {
	struct lwes_emitter *e;

	/*
	 * map certain fields to certain types so we won't have to worry about
	 * it later on in the Ruby API.
	 */
	VALUE field_types;
};

/* gets the _rb_lwes_emitter struct pointer from self */
static struct _rb_lwes_emitter * _rle(VALUE self)
{
	struct _rb_lwes_emitter *rle;

	Data_Get_Struct(self, struct _rb_lwes_emitter, rle);

	return rle;
}

/* mark-and-sweep GC automatically calls this */
static void rle_mark(void *ptr)
{
	struct _rb_lwes_emitter *rle = ptr;

	rb_gc_mark(rle->field_types);
}

/* GC automatically calls this when object is finalized */
static void rle_free(void *ptr)
{
	struct _rb_lwes_emitter *rle = ptr;

	if (rle->e)
		lwes_emitter_destroy(rle->e);
	rle->e = NULL;
}

/* called by the GC when object is allocated */
static VALUE rle_alloc(VALUE klass)
{
	struct _rb_lwes_emitter *rle;

	return Data_Make_Struct(klass, struct _rb_lwes_emitter,
	                        rle_mark, rle_free, rle);
}

/*
 * call-seq:
 *   emitter = LWES::Emitter.new
 *   emitter.emit(Hash.new)
 */
static VALUE emitter_emit(VALUE self, VALUE event)
{
	if (TYPE(event) != T_HASH)
		rb_raise(rb_eTypeError, "must be a hash");

	return event;
}

/* should only used internally by #initialize */
static VALUE _create(VALUE self, VALUE options)
{
	struct _rb_lwes_emitter *rle = _rle(self);
	VALUE address, iface, port, heartbeat, ttl;
	LWES_CONST_SHORT_STRING _address, _iface;
	LWES_U_INT_32 _port; /* odd, uint16 would be enough here */
	LWES_BOOLEAN _emit_heartbeat = FALSE;
	LWES_INT_16 _freq = 0;
	LWES_U_INT_32 _ttl = 0xffffffff; /* nobody sets a ttl this long, right? */

	if (rle->e)
		rb_raise(rb_eRuntimeError, "already created lwes_emitter");
	if (TYPE(options) != T_HASH)
		rb_raise(rb_eTypeError, "options must be a hash");

	address = rb_hash_aref(options, ID2SYM(rb_intern("address")));
	if (TYPE(address) != T_STRING)
		rb_raise(rb_eTypeError, ":address must be a string");
	_address = RSTRING_PTR(address);

	iface = rb_hash_aref(options, ID2SYM(rb_intern("iface")));
	if (TYPE(iface) != T_STRING)
		rb_raise(rb_eTypeError, ":iface must be a string");
	_iface = RSTRING_PTR(address);

	port = rb_hash_aref(options, ID2SYM(rb_intern("port")));
	if (TYPE(port) != T_FIXNUM)
		rb_raise(rb_eTypeError, ":port must be a Fixnum");
	_port = NUM2UINT(port);

	heartbeat = rb_hash_aref(options, ID2SYM(rb_intern("heartbeat")));
	if (TYPE(heartbeat) == T_FIXNUM) {
		int tmp = NUM2INT(heartbeat);
		if (tmp > 0x7fff)
			rb_raise(rb_eArgError, ":heartbeat > 0x7fff seconds");
		_emit_heartbeat = TRUE;
		_freq = (LWES_INT_16)tmp;
	} else if (NIL_P(heartbeat)) { /* do nothing, use defaults */
	} else
		rb_raise(rb_eTypeError, ":heartbeat must be a Fixnum or nil");

	ttl = rb_hash_aref(options, ID2SYM(rb_intern("ttl")));
	if (TYPE(ttl) == T_FIXNUM) {
		unsigned long tmp = NUM2ULONG(ttl);
		if (tmp >= 0xffffffff)
			rb_raise(rb_eArgError, ":ttl >= 0xffffffff seconds");
		_ttl = tmp;
	} else if (NIL_P(ttl)) { /* do nothing, no ttl */
	} else
		rb_raise(rb_eTypeError, ":ttl must be a Fixnum or nil");

	if (_ttl == 0xffffffff)
		rle->e = lwes_emitter_create(
		         _address, _iface, _port, _emit_heartbeat, _freq);
	else
		rle->e = lwes_emitter_create_with_ttl(
		         _address, _iface, _port, _emit_heartbeat, _freq, _ttl);

	if (!rle->e)
		rb_raise(rb_eRuntimeError, "failed to create LWES emitter");

	rle->field_types = rb_hash_new();

	return self;
}

static void init_emitter(void)
{
	cLWES_Emitter = rb_define_class_under(mLWES, "Emitter", rb_cObject);

	rb_define_method(cLWES_Emitter, "emit", emitter_emit, 1);
	rb_define_method(cLWES_Emitter, "_create", _create, 1);
	rb_define_alloc_func(cLWES_Emitter, rle_alloc);
}

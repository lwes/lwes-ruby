#include "lwes_ruby.h"

VALUE cLWES_TypeDB;

static void tdb_free(void *ptr)
{
	struct lwes_event_type_db *db = ptr;

	if (db)
		lwes_event_type_db_destroy(db);
}

static VALUE tdb_alloc(VALUE klass)
{
	return Data_Wrap_Struct(klass, NULL, tdb_free, NULL);
}

/*
 * call-seq:
 *
 *	LWES::TypeDB.new("events.esf") -> LWES::TypeDB
 *
 * Initializes a new TypeDB object based on the path to a given ESF file.
 */
static VALUE tdb_init(VALUE self, VALUE path)
{
	struct lwes_event_type_db *db = DATA_PTR(self);
	int gc_retry = 1;
	char *cpath = StringValueCStr(path);

	if (db)
		rb_raise(rb_eRuntimeError, "ESF already initialized");
retry:
	db = lwes_event_type_db_create(cpath);
	if (!db) {
		if (--gc_retry == 0) {
			rb_gc();
			goto retry;
		}
		rb_raise(rb_eRuntimeError,
		         "failed to create type DB for LWES file %s", cpath);
	}
	DATA_PTR(self) = db;

	return Qnil;
}

static VALUE attr_sym(struct lwes_event_attribute *attr)
{
	switch (attr->type) {
	case LWES_TYPE_U_INT_16:
	case LWES_TYPE_INT_16:
	case LWES_TYPE_U_INT_32:
	case LWES_TYPE_INT_32:
	case LWES_TYPE_U_INT_64:
	case LWES_TYPE_INT_64:
	case LWES_TYPE_BOOLEAN:
	case LWES_TYPE_IP_ADDR:
	case LWES_TYPE_STRING:
		return INT2NUM(attr->type);
	case LWES_TYPE_UNDEFINED:
	default:
		rb_raise(rb_eRuntimeError,
			 "unknown LWES attribute type: 0x%02x", attr->type);
	}

	return Qfalse;
}

static VALUE event_def_ary(struct lwes_hash *hash)
{
	struct lwes_hash_enumeration e;
	VALUE rv = rb_ary_new();

	if (!lwes_hash_keys(hash, &e))
		rb_raise(rb_eRuntimeError, "couldn't get event attributes");
	while (lwes_hash_enumeration_has_more_elements(&e)) {
		LWES_SHORT_STRING key = lwes_hash_enumeration_next_element(&e);
		struct lwes_event_attribute *attr;
		VALUE pair;

		if (!key)
			rb_raise(rb_eRuntimeError,
			         "LWES event type iteration key fail");

		attr = (struct lwes_event_attribute *)lwes_hash_get(hash, key);
		if (!attr)
			rb_raise(rb_eRuntimeError,
			         "LWES event type iteration value fail");

		pair = rb_ary_new3(2, ID2SYM(rb_intern(key)), attr_sym(attr));
		rb_ary_push(rv, pair);
	}

	return rv;
}

struct lwes_event_type_db * lwesrb_get_type_db(VALUE self)
{
	struct lwes_event_type_db *db = DATA_PTR(self);

	if (!db) {
		volatile VALUE raise_inspect;
		rb_raise(rb_eRuntimeError,
			 "couldn't get lwes_type_db from %s",
			 RAISE_INSPECT(self));
	}
	return db;
}

/* :nodoc: */
static VALUE tdb_to_hash(VALUE self)
{
	struct lwes_event_type_db *db = DATA_PTR(self);
	VALUE rv = rb_hash_new();
	struct lwes_hash *events;
	struct lwes_hash_enumeration e;

	assert(db && db->events && "tdb not initialized");
	events = db->events;

	if (!lwes_hash_keys(events, &e))
		rb_raise(rb_eRuntimeError, "couldn't get type_db events");

	while (lwes_hash_enumeration_has_more_elements(&e)) {
		struct lwes_hash *hash;
		ID event_key;
		LWES_SHORT_STRING key = lwes_hash_enumeration_next_element(&e);

		if (!key)
			rb_raise(rb_eRuntimeError,
			         "LWES type DB rv iteration key fail");

		hash = (struct lwes_hash*)lwes_hash_get(events, key);
		if (!hash)
			rb_raise(rb_eRuntimeError,
			         "LWES type DB rv iteration value fail");

		event_key = ID2SYM(rb_intern(key));
		rb_hash_aset(rv, event_key, event_def_ary(hash));
	}

	return rv;
}

void lwesrb_init_type_db(void)
{
	VALUE mLWES = rb_define_module("LWES");
	cLWES_TypeDB = rb_define_class_under(mLWES, "TypeDB", rb_cObject);
	rb_define_private_method(cLWES_TypeDB, "initialize", tdb_init, 1);
	rb_define_method(cLWES_TypeDB, "to_hash", tdb_to_hash, 0);
	rb_define_alloc_func(cLWES_TypeDB, tdb_alloc);
}

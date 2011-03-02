#include "lwes_ruby.h"

VALUE cLWES_TypeDB;

struct _tdb {
	struct lwes_event_type_db *db;
};

static void tdb_free(void *ptr)
{
	struct _tdb *tdb = ptr;

	if (tdb->db)
		lwes_event_type_db_destroy(tdb->db);
	xfree(ptr);
}

static VALUE tdb_alloc(VALUE klass)
{
	struct _tdb *tdb;

	return Data_Make_Struct(klass, struct _tdb, NULL, tdb_free, tdb);
}

static VALUE tdb_init(VALUE self, VALUE path)
{
	struct _tdb *tdb;
	int gc_retry = 1;

	if (TYPE(path) != T_STRING)
		rb_raise(rb_eArgError, "path must be a string");

	Data_Get_Struct(self, struct _tdb, tdb);
	if (tdb->db)
		rb_raise(rb_eRuntimeError, "ESF already initialized");
retry:
	tdb->db = lwes_event_type_db_create(RSTRING_PTR(path));
	if (!tdb->db) {
		if (--gc_retry == 0) {
			rb_gc();
			goto retry;
		}
		rb_raise(rb_eRuntimeError,
		         "failed to create type DB for LWES file %s",
		         RSTRING_PTR(path));
	}

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
	struct _tdb *tdb;

	Data_Get_Struct(self, struct _tdb, tdb);
	if (! tdb->db) {
		volatile VALUE raise_inspect;
		rb_raise(rb_eRuntimeError,
			 "couldn't get lwes_type_db from %s",
			 RAISE_INSPECT(self));
	}
	return tdb->db;
}

static VALUE tdb_to_hash(VALUE self)
{
	struct _tdb *tdb;
	VALUE rv = rb_hash_new();
	struct lwes_hash *events;
	struct lwes_hash_enumeration e;

	Data_Get_Struct(self, struct _tdb, tdb);
	assert(tdb->db && tdb->db->events && "tdb not initialized");
	events = tdb->db->events;

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
	rb_define_method(cLWES_TypeDB, "initialize", tdb_init, 1);
	rb_define_method(cLWES_TypeDB, "to_hash", tdb_to_hash, 0);
	rb_define_alloc_func(cLWES_TypeDB, tdb_alloc);
}

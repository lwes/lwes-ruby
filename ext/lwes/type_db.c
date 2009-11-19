#include "lwes_ruby.h"

static VALUE cLWES_TypeDB;

struct _tdb {
	struct lwes_event_type_db *db;
};

static void tdb_free(void *ptr)
{
	struct _tdb *tdb = ptr;

	if (tdb->db)
		lwes_event_type_db_destroy(tdb->db);
	tdb->db = NULL;
}

static VALUE tdb_alloc(VALUE klass)
{
	struct _tdb *tdb;

	return Data_Make_Struct(klass, struct _tdb, NULL, tdb_free, tdb);
}

static VALUE tdb_init(VALUE self, VALUE path)
{
	struct _tdb *tdb;

	Data_Get_Struct(self, struct _tdb, tdb);
	if (tdb->db)
		rb_raise(rb_eRuntimeError, "ESF already initialized");

	tdb->db = lwes_event_type_db_create(RSTRING_PTR(path));
	if (!tdb->db)
		rb_raise(rb_eRuntimeError,
		         "failed to create type DB for LWES file %s",
		         RSTRING_PTR(path));

	return Qnil;
}

void init_type_db(void)
{
	VALUE mLWES = rb_define_module("LWES");
	cLWES_TypeDB = rb_define_class_under(mLWES, "TypeDB", rb_cObject);
	rb_define_method(cLWES_TypeDB, "initialize", tdb_init, 1);
	rb_define_alloc_func(cLWES_TypeDB, tdb_alloc);
}

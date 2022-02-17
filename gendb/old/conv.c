
#include "gendb.h"

#if 0
static struct _ftconv ftconv[FT_MAX] = {
        /* C decl,      printf,         SQL */
        { "void",       "%p",           "SQL_C_CHAR" },
        { "char",       "%c",           "SQL_C_CHAR" },
        { "char *",     "\'%s\'",       "SQL_C_CHAR" },
        { "short",      "%d",           "SQL_C_SHORT" },
        { "int",        "%d",           "SQL_C_LONG" },
        { "long",       "%ld",          "SQL_C_LONG" },
        { "long long",  "%lld",         "SQL_C_SBIGINT" },
        { "float",      "%f",           "SQL_C_FLOAT" },
        { "double",     "%f",           "SQL_C_DOUBLE" },
        { "long double","%lf",          "SQL_C_DOUBLE" },
        { "char *",     "%s",           "SQL_C_CHAR" }
};
#endif

int isptr(int type) {
	int r = 0;

	switch(type) {
	case -2: /* binary */
	case -3: /* varbinary */
	case -4: /* long varbinary / blob */
		r = 1;
		break;
	case -5: /* bigint */
		break;
	case -6: /* tinyint */
	case -7: /* bit */
		break;
	case -1: /* long varchar */
	case -9: /* nvarchar */
	case 1: /* char */
	case 12: /* varchar */
		r = 1;
		break;
	case 4: /* integer */
		break;
	case 5: /* smallint */
		break;
	case 3: /* decimal */
	case 7: /* float */
		break;
	case 8: /* double */
		break;
	case 9: /* date */
	case 10: /* time */
	case 11: /* timestamp */
		r = 1;
		break;
	case -8: /* unknown */
	case 2: /* unknown */
	case 6: /* unknown */
	default:
		dprintf("unknown type: %d\n", type);
		break;
	}

	return r;
}

char *get_type_name(COLUMN *field) {
	char *str = "unknown";

	switch(field->type) {
	case -1: /* long varchar */
		str = "long varchar";
		break;
	case -2: /* binary */
		str = "binary";
		break;
	case -3: /* varbinary */
		str = "varbinary";
		break;
	case -4: /* long varbinary / blob */
		str = "blob";
		break;
	case -5: /* bigint */
		str = "bigint";
		break;
	case -6: /* tinyint */
		str = "tinyint";
		break;
	case -7: /* bit */
		str = "bit";
		break;
	case -8: /* unknown */
		break;
	case -9: /* nvarchar */
		str = "nvarchar";
		break;
	case 1: /* char */
		str = "char *";
		break;
	case 2: /* unknown */
		break;
	case 3: /* decimal */
		str = "decimal";
		break;
	case 4: /* integer */
		str = "integer";
		break;
	case 5: /* smallint */
		str = "smallint";
		break;
	case 6: /* unknown */
		break;
	case 7: /* float */
		str = "float";
		break;
	case 8: /* double */
		str = "double";
		break;
	case 9: /* date */
		str = "date";
		break;
	case 10: /* time */
		str = "time";
		break;
	case 11: /* timestamp */
		str = "timestamp";
		break;
	case 12: /* varchar */
		str = "varchar";
		break;
	default:
		dprintf("unknown type: %d\n", field->type);
		break;
	}

	return str;
}

char *get_field_ctype(COLUMN *field) {
	char *str = "void";

	switch(field->type) {
	case -2: /* binary */
	case -3: /* varbinary */
	case -4: /* long varbinary / blob */
		str = "unsigned char *";
		break;
	case -5: /* bigint */
		str = "long long";
		break;
	case -6: /* tinyint */
	case -7: /* bit */
		str = "unsigned char";
		break;
	case -1: /* long varchar */
	case -9: /* nvarchar */
	case 1: /* char */
	case 12: /* varchar */
		str = "char *";
		break;
		break;
	case 4: /* integer */
		str = "long";
		break;
	case 5: /* smallint */
		str = "unsigned short";
		break;
	case 3: /* decimal */
	case 7: /* float */
		str = "float";
		break;
	case 8: /* double */
		str = "double";
		break;
	case 9: /* date */
	case 10: /* time */
	case 11: /* timestamp */
		str = "char *";
		break;
	case -8: /* unknown */
	case 2: /* unknown */
	case 6: /* unknown */
	default:
		dprintf("unknown type: %d\n", field->type);
		break;
	}

	return str;
}

char *get_field_pstr(COLUMN *field) {
	char *str = "%x";

	switch(field->type) {
	case -1: /* long varchar */
		break;
	case -2: /* binary */
		break;
	case -3: /* varbinary */
		break;
	case -4: /* long varbinary / blob */
		break;
	case -5: /* bigint */
		break;
	case -6: /* tinyint */
		break;
	case -7: /* bit */
		break;
	case -8: /* unknown */
		break;
	case -9: /* nvarchar */
		break;
	case 1: /* char */
		break;
	case 2: /* unknown */
		break;
	case 3: /* decimal */
		break;
	case 4: /* integer */
		break;
	case 5: /* smallint */
		break;
	case 6: /* unknown */
		break;
	case 7: /* float */
		break;
	case 8: /* double */
		break;
	case 9: /* date */
		break;
	case 10: /* time */
		break;
	case 11: /* timestamp */
		break;
	case 12: /* varchar */
		break;
	default:
		dprintf("unknown type: %d\n", field->type);
		break;
	}

	return str;
}

char *get_field_dtype(COLUMN *field) {
	char *str = "unknown";

	switch(field->type) {
	case -1: /* long varchar */
		break;
	case -2: /* binary */
		break;
	case -3: /* varbinary */
		break;
	case -4: /* long varbinary / blob */
		break;
	case -5: /* bigint */
		break;
	case -6: /* tinyint */
		break;
	case -7: /* bit */
		break;
	case -8: /* unknown */
		break;
	case -9: /* nvarchar */
		break;
	case 1: /* char */
		break;
	case 2: /* unknown */
		break;
	case 3: /* decimal */
		break;
	case 4: /* integer */
		break;
	case 5: /* smallint */
		break;
	case 6: /* unknown */
		break;
	case 7: /* float */
		break;
	case 8: /* double */
		break;
	case 9: /* date */
		break;
	case 10: /* time */
		break;
	case 11: /* timestamp */
		break;
	case 12: /* varchar */
		break;
	default:
		dprintf("unknown type: %d\n", field->type);
		break;
	}

	return str;
}


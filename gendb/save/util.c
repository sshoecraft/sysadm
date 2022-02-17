
#include "gendb.h"

COLUMN *get_column(TABLE *table, char *name) {
	COLUMN *field;

	list_reset(table->fields);
	while((field = list_get_next(table->fields)) != 0) {
		if (strcmp(field->name,name) == 0)
			return field;
	}
	return 0;
}

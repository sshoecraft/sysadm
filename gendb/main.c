
#include "gendb.h"

int opts;

void usage(void) {
	printf("usage: gendb -s <datasource> -u <user> -p <pass> [schema.]table [keys]\n");
	printf("  where:\n");
	printf("    -s             odbc datasource [sysadm]\n");
	printf("    -u             odbc username [sysadm_ro]\n");
	printf("    -p             odbc password [null]\n");
	printf("    -f             functions [sidu]\n");
	printf("    -i             dont insert specified fields\n");
	printf("    -n             dont update specified fields\n");
	printf("    -h             this listing\n");
	printf("    -l             list tables\n");
	exit(1);
}

enum OUT_FORMATS {
	OUT_FORMAT_C,
	OUT_FORMAT_PHP,
};

extern outfunc write_c_inc;
extern outfunc write_c_src;
extern outfunc write_php_src;

int main(int argc, char **argv) {
	DB *db;
	char *name, *user, *pass, *schema, *table_name, *keys, *noins, *funcs, *noupd, *p, *p2;
	char req_funcs[16];
	int ch, list_tables, format;
	COLUMN *field;
	TABLE *table;
	outfunc *write_inc;
	outfunc *write_src;

//	opts = OPT_VERBOSE | OPT_DEBUG;
	opts = 0;

	name = user = pass = schema = table_name = keys = funcs = 0;
	noins = noupd = 0;
	list_tables = 0;
	format = OUT_FORMAT_C;
	while((ch = getopt(argc, argv, "s:u:p:f:li:n:wh")) != -1) {
		switch(ch) {
		case 's':
			name = optarg;
			break;
		case 'u':
			user = optarg;
			break;
		case 'p':
			pass = optarg;
			break;
		case 'f':
			funcs = optarg;
			break;
		case 'l':
			list_tables = 1;
			break;
		case 'i':
			noins = optarg;
			break;
		case 'n':
			noupd = optarg;
			break;
		case 'w':
			format = OUT_FORMAT_PHP;
			break;
		case 'h':
			usage();
			break;
		}
	}
	if (!name) name = "sysadm";
	if (!user) user = "sysadm_ro";
	if (!pass) pass = "";
	dprintf("name: %s, user: %s, pass: %s\n", name, user, (pass ? pass : "null"));

	switch(format) {
	case OUT_FORMAT_PHP:
		dprintf("setting output type to: PHP\n");
		write_inc = 0;
		write_src = write_php_src;
		if (!funcs) funcs = "siud";
		break;
	case OUT_FORMAT_C:
	default:
		dprintf("setting output type to: C\n");
		write_inc = write_c_inc;
		write_src = write_c_src;
		if (!funcs) funcs = "fsiud";
		break;
	}

	/* Next option is table name */
	if (optind < argc) {
		p = argv[optind++];
		p2 = strchr(p,'.');
		if (p2) {
			*p2 = 0;
			schema = p;
			table_name = p2+1;
		} else {
			schema = name;
			table_name = p;
		}
	} else if (!list_tables) {
		usage();
	}

	/* Next option is key names */
	if (optind < argc) keys = argv[optind++];

	/* Connect to the database */
	dprintf("Connecting to database...\n");
	db = malloc(sizeof(*db));
	if (db_connect(db,name,user,pass)) return 1;

	if (list_tables) {
		list tables = get_table_list(db);
		if (!tables) return 1;
		list_reset(tables);
		while((p = list_get_next(tables)) != 0) {
			printf("%s\n", p);
		}
		goto done;
	}

	if (opts & OPT_VERBOSE)
		printf("Getting info for table %s.%s...\n", schema, table_name);

	if (!schema) schema = "";
	if (!keys) keys = "";
	table = get_table_info(db,schema,table_name,keys);
	if (!table) printf("unable to get table info");

	if (noins) {
		int i = 0;

		dprintf("noins: %s\n", noins);
		while(1) {
			p = strele(i++,",", noins);
			if (!strlen(p)) break;
			dprintf("noins field: %s\n", p);
			field = get_column(table,p);
			if (!field) continue;
			field->noins = 1;
		}
	}

	if (noupd) {
		int i = 0;

		dprintf("noupd: %s\n", noupd);
		while(1) {
			p = strele(i++,",", noupd);
			if (!strlen(p)) break;
			dprintf("noupd field: %s\n", p);
			field = get_column(table,p);
			if (!field) continue;
			field->noupd = 1;
		}
	}

	/* Display info */
	if (opts & OPT_VERBOSE) {
		printf("Table owner: %s, name: %s, comment: %s\n",
			table->schema,table->name,table->comment);
		list_reset(table->fields);
		while( (field = list_get_next(table->fields)) != 0)
		    printf("\tField name: %s, type: %s, len: %d, comment: %s\n",
			field->name,field->type_name,field->len, field->comment);
		printf("\tPrimary keys:");
		list_reset(table->keys);
		while( (p = list_get_next(table->keys)) != 0) printf(" %s",p);
		printf(".\n");
	}

	/* XXX Select requires fetch for C */
	strcpy(req_funcs,funcs);
	if (format == OUT_FORMAT_C && strchr(req_funcs,'s') && !strchr(req_funcs,'f')) strcat(req_funcs,"f");

	/* Write files */
	if (write_inc) write_inc(table,req_funcs);
	if (write_src) write_src(table,req_funcs);

done:
	/* Disconnect from db */
	db_disconnect(db);

	return 0;
}

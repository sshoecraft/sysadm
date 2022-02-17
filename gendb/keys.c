
void get_keys(TABLE_INFO *fields,char *key_line) {
	TABLE_INFO *cur_field;
	char keys[128];
	register char *ptr;
	register int x;

	/* Copy the key info to a local var and parse it */
	strcpy(keys,key_line);
	x=0;
	while(1) {
		ptr = strele(x++,"+",keys);
		if (!strlen(ptr)) break;
		if (opts & OPT_DEBUG) printf("key column: %s\n",ptr);
		cur_field = fields;
		while(cur_field) {
			if (strcmp(cur_field->field_name,ptr)==0)
				add_key(cur_field);
			cur_field = cur_field->next;
		}
	}
	return;
}

void add_key(TABLE_INFO *field) {
	KEY_INFO *info;
	register char *ptr;

	/* Alloc the info */
	info = (KEY_INFO *) malloc(KEY_INFO_SIZE);
	strcpy(info->key_name,field->field_name);
	info->is_ptr = 0;
	info->next = 0;

	/* Determine the field type string */
	switch(field->field_type) {
		case FT_CHAR:
		case FT_STRING:
		case FT_DATE:
			ptr = "char";
			if (field->field_len > 1) info->is_ptr = 1;
			break;
		case FT_SHORT:
			ptr = "short";
			break;
		case FT_LONG:
			ptr = "long";
			break;
		case FT_FLOAT:
			ptr = "float";
			break;
		case FT_DOUBLE:
			ptr = "double";
			break;
		case FT_LONGDBL:
			ptr = "long double";
			break;
		default:
			ptr = "void";
			if (field->field_len > 1) info->is_ptr = 1;
			break;
	}
	strcpy(info->key_type,ptr);

	/* Add the new key */
	if (opts & OPT_DEBUG) {
		printf("adding key: %s, type: %s, is_ptr: %d\n",
			info->key_name,info->key_type,info->is_ptr);
		printf("key_list: %p,last_key: %p,info: %p\n",
			key_list,last_key,info);
	}
	if (!key_list) 
		key_list = last_key = info;
	else {
		last_key->next = info;
		last_key = info;
	}
	if (opts & OPT_DEBUG)
		printf("after add: key_list: %p,last_key: %p,info: %p\n",
			key_list,last_key,info);
	return;
}	

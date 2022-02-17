
#ifndef __MINGW32__
#include <stdio.h>
#include <string.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>

void disp(xmlNode *parent, int level) {
	xmlNode *node;
	register int x;

	for(node = parent; node; node = node->next) {
		if (node->children) {
			xmlNode *cnode = node->children;

			if (cnode->next == 0 && strcmp((char *)cnode->name,"text") == 0) {
				for(x=0; x < level; x++) printf("  ");
				printf("<%s>%s</%s>\n", node->name,cnode->content,node->name);
			} else {
				for(x=0; x < level; x++) printf("  ");
				printf("<%s>\n",node->name);
				disp(node->children,level+1);
				for(x=0; x < level; x++) printf("  ");
				printf("</%s>\n",node->name);
			}
		} else {
			for(x=0; x < level; x++) printf("  ");
			printf("<%s>%s</%s>\n", node->name,node->content,node->name);
		}
	}
}

int main (int argc, char **argv) {
	xmlDoc *doc = NULL;
	xmlNode *root_element = NULL;

	if (argc != 2)
		return (1);

	LIBXML_TEST_VERSION
	doc = xmlReadFile (argv[1], NULL, XML_PARSE_RECOVER|XML_PARSE_NOERROR|XML_PARSE_NOWARNING);

	if (doc == NULL) {
		printf ("error: could not parse file %s\n", argv[1]);
	}
	root_element = xmlDocGetRootElement (doc);

	disp(root_element,0);

	xmlFreeDoc (doc);

	xmlCleanupParser ();

	return 0;
}
#else
int main(void) { return 0; }
#endif

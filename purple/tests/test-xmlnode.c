/*
 * Test the xmlnode module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

#include "xmlnode.h"

int main(void)
{
	test_package_begin("xmlnode", "XML parser");

	test_begin("Non-creation");
	{
		XmlNode	*root;

		root = xmlnode_new(NULL);
		test_result(root == NULL);
	}
	test_end();

	test_begin("Single node creation");
	{
		XmlNode	*root;

		root = xmlnode_new("<test/>");
		test_result(root != NULL && strcmp(xmlnode_get_name(root), "test") == 0);
		xmlnode_destroy(root);
	}
	test_end();

	test_begin("Attribute access");
	{
		XmlNode	*root;

		root = xmlnode_new("<test foo='bar'/>");
		test_result(strcmp(xmlnode_attrib_get_value(root, "foo"), "bar") == 0);
		xmlnode_destroy(root);
	}
	test_end();

	test_begin("Path evaluation");
	{
		XmlNode	*root;

		root = xmlnode_new("<parent><child name='foo'/><child name='bar'/><child name='baz'/></parent>");
		test_result(strcmp(xmlnode_eval_single(root, "parent/child[@NAME]"), "baz") == 0);
		xmlnode_destroy(root);
	}
	test_end();
	
	return test_package_end();
}

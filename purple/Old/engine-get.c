/*
 * 
*/

#include <string.h>

#include "api-test.h"

const char * api_get_value(const char *key)
{
	if(key == NULL)
		return "null";
	else if(strcmp(key, "foo") == 0)
		return "bar";
	return "unknown";
}

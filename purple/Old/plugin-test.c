/*
 * 
*/

#include <stdio.h>

#include "api-test.h"

void init(void)
{
	printf("Hello from the plugin\n");
	printf("The value of foo is %s\n", api_get_value("foo"));
}

void compute(void)
{
	printf("I compute, therefore I am\n");
}

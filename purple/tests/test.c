/*
 * 
*/

#include <stdio.h>
#include <stdlib.h>

static struct
{
	char	pkg_name[32];

	char	test_label[32];
	int	test_status;

	size_t	test_count;
	size_t	pass_count;
} test_info;

void test_package_begin(const char *name, const char *desc)
{
	snprintf(test_info.pkg_name, sizeof test_info.pkg_name, "%s", name);
	test_info.test_count = 0;
	test_info.pass_count = 0;

	printf("Testing %s:\n", name);
}

int test_package_end(void)
{
	if(test_info.test_label[0] != '\0')
		printf("<broken>\n");
	printf("Done with %s; did %u tests, %u passed\n", test_info.pkg_name, test_info.test_count, test_info.pass_count);
	return test_info.test_status == 1;
}

void test_begin(const char *what)
{
	snprintf(test_info.test_label, sizeof test_info.test_label, "%s", what);
	test_info.test_status = -1;
	test_info.test_count++;
}

void test_result(int passed)
{
	test_info.test_status = passed > 0;
}

int test_end(void)
{
	if(test_info.test_status < 0)
		printf("<test lacks test_result() call>");
	printf(" %-40s [%s]\n", test_info.test_label, test_info.test_status == 0 ? "FAILED" : "  OK  ");
	test_info.pass_count += test_info.test_status != 0;
	test_info.test_label[0] = '\0';
	return test_info.test_status != 0;
}

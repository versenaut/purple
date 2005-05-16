/*
 * Test the memchunk module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

#include "memchunk.h"

int main(void)
{
	test_package_begin("memchunk", "Chunked memory allocation system");

	test_begin("Simple allocation");
	{
		MemChunk	*a;
		void		*p;

		a = memchunk_new("test", 32, 4);
		p = memchunk_alloc(a);
		test_result(a != NULL && p != NULL);
		memchunk_destroy(a);
	}
	test_end();

	test_begin("Simple growth");
	{
		MemChunk	*a;
		void		*p[5];
		int		i, got;

		a = memchunk_new("test", 8, sizeof p / sizeof *p - 1);
		for(i = got = 0; i < sizeof p / sizeof *p; i++)
		{
			p[i] = memchunk_alloc(a);
			if(p[i] != NULL)
				got++;
		}
		memchunk_destroy(a);
		test_result(got == sizeof p / sizeof *p);
	}
	test_end();
	
	return test_package_end();
}

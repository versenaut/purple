/*
 * 
*/

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include "api-test.h"

void load_and_init(const char *name)
{
	void	*module;

	if((module = dlopen(name, RTLD_LAZY)) != NULL)
	{
		void (*init)(void);

		if((init = (void(*)(void)) dlsym(module, "init")) != NULL)
		{
			void	(*compute)(void);

			init();
			if((compute = (void(*)(void)) dlsym(module, "compute")) != NULL)
				compute();
		}
		else
			puts(dlerror());
		dlclose(module);
	}
	else
		puts(dlerror());
}

int main(int argc, char *argv[])
{
	int	i;

	for(i = 1; i < argc; i++)
		load_and_init(argv[i]);
	return 0;
}

/*
 * Super-simplistic testing "framework". Does almost nothing.
*/

extern void	test_package_begin(const char *name, const char *desc);

extern void	test_begin(const char *what);
extern void	test_result(int passed);
extern int	test_end(void);

extern int	test_package_end(void);

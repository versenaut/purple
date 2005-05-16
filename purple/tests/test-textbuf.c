/*
 * Tests of the textbuf module.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

#include "textbuf.h"

int main(void)
{
	test_package_begin("textbuf", "Text buffer container");

	test_begin("Creation");
	{
		TextBuf	*tb;

		tb = textbuf_new(0u);
		test_result(tb != NULL);
		textbuf_destroy(tb);
	}
	test_end();

	test_begin("Insertion");
	{
		TextBuf	*tb;

		tb = textbuf_new(0u);
		textbuf_insert(tb, 0, "foobar");
		test_result(textbuf_length(tb) == 6 &&
			    strcmp(textbuf_text(tb), "foobar") == 0);
		textbuf_destroy(tb);
	}
	test_end();

	test_begin("Insertion and deletion");
	{
		TextBuf	*tb;

		tb = textbuf_new(0u);
		textbuf_insert(tb, 0, "foobar");
		textbuf_delete(tb, 3, 3);
		textbuf_insert(tb, 100, "d");
		test_result(strcmp(textbuf_text(tb), "food") == 0);
		textbuf_destroy(tb);
	}
	test_end();

	test_begin("Truncation");
	{
		TextBuf	*tb;

		tb = textbuf_new(0u);
		textbuf_insert(tb, 0, "This is a long sentence which we will soon truncate.");
		textbuf_truncate(tb, 4);
		test_result(strcmp(textbuf_text(tb), "This") == 0);
		textbuf_destroy(tb);
	}
	test_end();

	test_begin("More insert + delete");
	{
		TextBuf	*tb;

		tb = textbuf_new(0u);
		textbuf_insert(tb, 0, " lazy dog.");
		textbuf_insert(tb, 0, " over the");
		textbuf_insert(tb, 0, " red fox jumps");
		textbuf_insert(tb, 0, "The");
		textbuf_delete(tb, 4, 3);
		textbuf_insert(tb, 4, "quick brown");
		test_result(strcmp(textbuf_text(tb), "The quick brown fox jumps over the lazy dog.") == 0);
		textbuf_destroy(tb);
	}
	test_end();

	return test_package_end();
}

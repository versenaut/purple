/*
 * Simple utility function(s) for dealing with XML output.
*/

#include "dynstr.h"

/* Append the <text> to the given dynstr, while escaping any character that cannot
 * occur "bare" in XML. Currently, these are '<', '>', '"' and '&'. These are replaced
 * with their character-entity representations (&lt;, &gt;, &quot; and &amp;).
*/
extern void	xml_dynstr_append(DynStr *d, const char *text);

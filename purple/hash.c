/*
 * A hash table data type. Always handy to have around.
*/

#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "mem.h"
#include "memchunk.h"

#include "hash.h"

/* ----------------------------------------------------------------------------------------- */

typedef struct HashEl	HashEl;

struct HashEl {
	void	*data;
	HashEl	*next;
};

static MemChunk	*the_chunk = NULL;

struct Hash
{
	HashEl		**vector;
	size_t		length;
	size_t		size;
	HashFunc	hfunc;
	HashHasKey	efunc;
};

/* Some prime numbers. Picked from <http://www.utm.edu/research/primes/lists/small/1000.txt>. */
static const unsigned int the_primes[] = {
     11,     13,     17,     19,     23,     29,
     31,     37,     41,     43,     47,     53,     59,     61,     67,     71, 
     73,     79,     83,     89,     97,    101,    103,    107,    109,    113, 
    127,    131,    137,    139,    149,    151,    157,    163,    167,    173, 
    179,    181,    191,    193,    197,    199,    211,    223,    227,    229, 
    233,    239,    241,    251,    257,    263,    269,    271,    277,    281, 
    283,    293,    307,    311,    313,    317,    331,    337,    347,    349, 
    353,    359,    367,    373,    379,    383,    389,    397,    401,    409, 
    419,    421,    431,    433,    439,    443,    449,    457,    461,    463, 
    467,    479,    487,    491,    499,    503,    509,    521,    523,    541, 
    547,    557,    563,    569,    571,    577,    587,    593,    599,    601, 
    607,    613,    617,    619,    631,    641,    643,    647,    653,    659, 
    661,    673,    677,    683,    691,    701,    709,    719,    727,    733, 
    739,    743,    751,    757,    761,    769,    773,    787,    797,    809, 
    811,    821,    823,    827,    829,    839,    853,    857,    859,    863, 
    877,    881,    883,    887,    907,    911,    919,    929,    937,    941,
};

/* ----------------------------------------------------------------------------------------- */

/* Return the closest prime n such that x <= n. Breaks horribly for x larger than table above indicates. */
static unsigned int get_prime(unsigned int x)
{
	unsigned int	j;

	for(j = 0; j < sizeof the_primes / sizeof *the_primes; j++)
	{
		if(x <= the_primes[j])
			return the_primes[j];
	}
	return the_primes[sizeof the_primes / sizeof *the_primes - 1];
}

/* This is (allegedly) the "djb2" string hashing function. Copied
 * (after some Googling) from <http://www.cs.yorku.ca/~oz/hash.html>.
*/
static unsigned int string_hash(const void *key)
{
	const char	*str = key;
	unsigned int	hash = 5381;
	int		c;

	while((c = *str++) != '\0')
		hash = ((hash << 5) + hash) + c;	/* hash * 33 + c */

	return hash;
}

static int string_compare(const void *key1, const void *key2)
{
	return strcmp(key1, key2) == 0;
}

/* ----------------------------------------------------------------------------------------- */

void hash_init(void)
{
	the_chunk = memchunk_new("HashEl", sizeof (HashEl), 16);
}

/* ----------------------------------------------------------------------------------------- */

Hash * hash_new(HashFunc hfunc, HashHasKey efunc)
{
	Hash	*hash;

	if(hfunc == NULL || efunc == NULL)
		return NULL;

	hash = mem_alloc(sizeof *hash);
	hash->vector = NULL;
	hash->length = 0;
	hash->size   = 0;
	hash->hfunc  = hfunc;
	hash->efunc  = efunc;

	return hash;
}

Hash * hash_new_string(void)
{
	return hash_new(string_hash, string_compare);
}

void hash_insert(Hash *hash, const void *key, void *data)
{
	unsigned int	h;
	HashEl		*el;

	if(hash == NULL)
		return;

	if(hash->size >= hash->length)
	{
		size_t	nl;
		HashEl	**v;

		nl = get_prime(hash->length + 1);
		LOG_MSG((" Growing hash size=%u length=%u to new length=%u", hash->size, hash->length, nl));
		v = mem_realloc(hash->vector, nl * sizeof *v);
		if(v != NULL)
		{
			unsigned int	i;

			hash->vector = v;
			for(i = hash->length; i < nl; i++)
				hash->vector[i] = NULL;
			hash->length = nl;
		}
	}

	h = hash->hfunc(key);
	h %= hash->length;
	el = memchunk_alloc(the_chunk);
	if(el != NULL)
	{
		el->data = data;
		el->next = hash->vector[h];
		hash->vector[h] = el;
		hash->size++;
	}
}

void * hash_lookup(const Hash *hash, const void *key)
{
	unsigned int	h;
	const HashEl	*el;

	if(hash == NULL || hash->size == 0)
		return NULL;
	h = hash->hfunc(key) % hash->length;
	for(el = hash->vector[h]; el != NULL; el = el->next)
	{
		if(hash->efunc(key, el->data))
			return el->data;
	}
	return NULL;
}

void hash_remove(Hash *hash, const void *key)
{
	unsigned int	h;
	HashEl		*prev, *iter;

	if(hash == NULL)
		return;
	h = hash->hfunc(key) % hash->length;
	for(prev = NULL, iter = hash->vector[h]; iter != NULL; prev = iter, iter = iter->next)
	{
		if(hash->efunc(key, iter->data))
		{
			if(prev != NULL)
				prev->next = iter->next;
			else
				hash->vector[h] = iter->next;
			memchunk_free(the_chunk, iter);
			hash->size--;
			return;
		}
	}
}

size_t hash_size(const Hash *hash)
{
	if(hash != NULL)
		return hash->size;
	return 0;
}

void hash_foreach(const Hash *hash, int (*func)(const void *data, void *user), void *user)
{
	unsigned int	i;

	if(hash == NULL || func == NULL)
		return;

	for(i = 0; i < hash->length; i++)
	{
		const HashEl	*iter;

		for(iter = hash->vector[i]; iter != NULL; iter = iter->next)
		{
			if(!func(iter->data, user))
				return;
		}
	}
}

void hash_destroy(Hash *hash)
{
	if(hash != NULL)
	{
		unsigned int	i;

		for(i = 0; i < hash->length; i++)
		{
			HashEl	*iter, *next;

			for(iter = hash->vector[i]; iter != NULL; iter = next)
			{
				next = iter->next;
				memchunk_free(the_chunk, iter);
			}
		}
		mem_free(hash->vector);
		mem_free(hash);
	}
}

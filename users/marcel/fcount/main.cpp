#include <stdio.h>
#include <math.h>

static inline long factorCount(long n)
{
	double square = sqrt(n);
	long isquare = (long)square;
	long count = isquare == square ? -1 : 0;

#pragma omp parallel for
	for (long candidate = 1; candidate <= isquare; candidate++)
	{
		if (0 == (n % candidate))
			count += 2;
	}
	return count;
}

int main ()
{
	long triangle = 1;
	long index = 1;
	while (factorCount(triangle) < 1001)
	{
		index++;
		triangle += index;
	}
	printf ("%ld\n", triangle);
}

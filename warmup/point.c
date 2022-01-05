#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>


void
point_translate(struct point *p, double x, double y)
{
	p->x += x;
	p->y += y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
	double dx = point_X(p1) - point_X(p2);
	double dy = point_Y(p1) - point_Y(p2);
	double ret = sqrt(dx*dx + dy*dy);
	return ret;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
	struct point *orig = calloc(1, sizeof(struct point));
	double r1 = point_distance(p1, orig);
	double r2 = point_distance(p2, orig);

	if (r1 < r2) {
		return -1;
	}

	if (r1 > r2) {
		return 1; 
	}

	return 0;
}

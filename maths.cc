#include "globals.h"

/*** Get the angle of x2,y2 relative to x1,y1. Zero degrees is vertical ***/
double getAngle(double x1, double y1, double x2, double y2)
{
	double xd,yd;
	double ang;

	// If one set is wrapped and the other isn't then take account of it
	if (fabs(x2 - x1) > LANDSCAPE_WIDTH / 2)
	{
		if (x2 > x1) 
			x1 += LANDSCAPE_WIDTH;
		else
			x2 += LANDSCAPE_WIDTH;
	}
	xd = x2 - x1;
	yd = y2 - y1;

	if (!yd) ang = (xd > 0 ? 90 : 270);
	else
	if (!xd) ang = (yd > 0 ? 0 : 180);
	else
	{
		ang = atan(xd / yd) * DEGS_PER_RADIAN;
		if (yd < 0) ang += 180;
		else
		if (xd < 0) ang += 360;
	}
	return ang;
}




/*** Return the difference in degrees from ang1 to ang2. Imagining a circle, 
     if the previous angle is to the right of the new one we return a positive 
     number 0 to 180 else a negative one -1 to -179 ***/
double angleDiff(double ang1, double ang2)
{
	// Imagine whole circle is rotated round so ang1 is at zero 
	incAngle(ang2,-ang1);
	return ang2 <= 180 ? ang2 : -(360 - ang2);
}




/*** Add to an angle and make sure its within the range 0 -> 359.999...
     Use % incase add is huge. Could do the same using a while loop but
     this is fancier ;) ***/
void incAngle(double &ang, double add)
{
	u_int iang;

	ang += add;

	if (ang >= 360) 
	{
		iang = (int)ang;
		ang = ang - iang + (iang % 360);
	}
	else if (ang < 0)
	{
		ang = -ang;
		iang = (int)ang;
		ang = ang - iang + (iang % 360);
		if (ang) ang = 360 - ang;
	}

	assert(ang >= 0 && ang < 360);
}




/*** Rotate a given point through an angle ***/
void rotatePoint(double x, double y, double ang, double *xres, double *yres)
{
	if (ang)
	{
		*xres = x * COS(ang) + y * SIN(ang);
		*yres = y * COS(ang) - x * SIN(ang);
	}
	else 
	{
		*xres = x;
		*yres = y;
	}
}




/*** Get distance from x1 to x2 taking into account landscape wrap ***/
double getXDistance(double x1, double x2)
{
	double dist = x2 - x1;

	if (fabs(dist) <= LANDSCAPE_WIDTH / 2) return dist;

	return x2 > x1 ? -((double)LANDSCAPE_WIDTH - x2 + x1) :
	                   (double)LANDSCAPE_WIDTH - x1 + x2;
}




/*** Get actual X draw location from game X location ***/
double getDrawX(double x)
{
	// If draw_x is halfway around the landscape wrap it. Don't test for
	// +LANDSCAPE_WIDTH/2 because screen_x_shift never goes negative
	double dx = x - screen_x_shift;

	if (dx < -LANDSCAPE_WIDTH/2) dx += LANDSCAPE_WIDTH;
	return dx;
}

#include "globals.h"

//////////////////////////////// SHAPE DEFS /////////////////////////////////

st_vertex cl_moonbase::main_vtx[MOONBASE_MAIN_VTX] =
{
	{ -40, 35 },
	{  40, 35 },
	{  20,-25 },
	{ -20,-25 }
};


st_vertex cl_moonbase::leg1_vtx[MOONBASE_LEG_VTX] =
{
	{ -20,-20 },
	{ -30,-50 },
	{ -10,-50 }
};


st_vertex cl_moonbase::leg2_vtx[MOONBASE_LEG_VTX] =
{
	{ 20,-20 },
	{ 30,-50 },
	{ 10,-50 }
};


//////////////////////////////// METHODS /////////////////////////////////////

cl_moonbase::cl_moonbase(): cl_object(TYPE_MOONBASE,70)
{
}




void cl_moonbase::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	setRandomGroundLocation(0,radius);
	reset();

	main_colour = PURPLE;
	leg_colour = ORANGE;
	win_colour = GREEN;
	pods_saved = 0;
	grab_dist = level < 11 ? 205 - level * 5 : 150;
	reset();
}




void cl_moonbase::reset()
{
	arc_radius = grab_dist;
	flash_timer = 0;
}




/*** Expand and contract when we swallow a pod. The pod inactivates itself ***/
void cl_moonbase::collidedWith(cl_object *obj)
{
	if (obj->type == TYPE_POD && ((cl_pod *)obj)->grabber == this)
	{
		++pods_saved;
		flash_timer = 3;
	}
}




/*** Set the arc size and scale ***/
void cl_moonbase::run()
{
	if (flash_timer) --flash_timer;

	if (arc_radius > diam) 
		arc_radius -= 10;
	else
		arc_radius = grab_dist;

	win_colour = (win_colour + 2) % GREEN2;
}




void cl_moonbase::draw()
{
	objDrawOrFillPolygon(
		flash_timer ? GREEN : main_colour,
		MOONBASE_MAIN_VTX,main_vtx,true);
	objDrawOrFillPolygon(leg_colour,MOONBASE_LEG_VTX,leg1_vtx,true);
	objDrawOrFillPolygon(leg_colour,MOONBASE_LEG_VTX,leg2_vtx,true);
	objDrawOrFillRectangle(win_colour,0,10,40,-15,-20,true);
	objDrawOrFillRectangle(win_colour,0,10,40,5,-20,true);

	if (stage_timer % 2)
	{
		// Grab arc
		objDrawOrFillHorizArc(
			DARK_MAUVE,10,arc_radius * 2,arc_radius,0,0,false);
		objDrawLine(DARK_RED,10,-arc_radius,0,-arc_radius,-40);
		objDrawLine(DARK_RED,10,arc_radius,0,arc_radius,-40);
	}
}

#include "globals.h"

//////////////////////////////// SHAPE DEFS /////////////////////////////////

st_vertex cl_silo::left_vtx[SILO_VTX] =
{
	{ -10, 25 },
	{  -5, 25 },
	{  -5,-25 },
	{ -20,-25 }
};


st_vertex cl_silo::mid_vtx[SILO_VTX] =
{
	{ -5, 25 },
	{  5, 25 },
	{  5,-25 },
	{ -5,-25 }
};


st_vertex cl_silo::right_vtx[SILO_VTX] =
{
	{ 10, 25 },
	{  5, 25 },
	{  5,-25 },
	{ 20,-25 }
};

////////////////////////////////// INIT /////////////////////////////////////


cl_silo::cl_silo(): cl_object(TYPE_SILO,40)
{
	explode_bits_cnt = 100;
}




void cl_silo::activate(cl_object *activator)
{
	setStage(STAGE_RUN);
	setRandomGroundLocation(diam * 2,radius / 2);
	setExplodeBits();
	health = 10;
	explode_colour = YELLOW;
	fireball_diam = 10;
	main_colour = LIGHT_BLUE;
	launch_every = level < 10 ? 800 - level * 40 : 400;

	reset();
}




void cl_silo::reset()
{
	setColours(false);
	flash_timer = 0;

	// Set to random value so silos don't all launch at exactly the
	// same time
	stage_timer = (int)random() % 100;
}




/*** Set or strobe the colours ***/
void cl_silo::setColours(bool strobe)
{
	en_colour cols[3] = { LIGHT_BLUE,PURPLE,LIGHT_BLUE };
	int i;

	for(i=0;i < 3;++i) 
	{
		if (strobe)
			colour[i] = cols[(stage_timer / 3 + i) % 3];
		else
			colour[i] = cols[i];
	}
}


////////////////////////////////// RUNTIME /////////////////////////////////////

void cl_silo::collidedWith(cl_object *obj)
{
	if (obj->type == TYPE_SHIP_LASER || obj->type == TYPE_BOMB)
	{
		if (subDamage(obj)) 
			incScore(400);
		else
			flash_timer = 3;
	}
}




void cl_silo::run()
{
	int mod;

	if (flash_timer) --flash_timer;

	mod = stage_timer % launch_every;

	if (!mod)
	{
		setColours(false);
		activateObjects(TYPE_MISSILE,1,this);
	}
	else if (mod > launch_every - 50) setColours(true);
}




void cl_silo::runExplode()
{
	runExplodeBits();

	if (stage_timer == 1) playSound(SND_SILO_OR_MISSILE_EXPLODE);

	if (stage_timer < 10) 
		fireball_diam += 20;
	else
		fireball_diam -= 4;

	if (stage_timer > 60) setStage(STAGE_INACTIVE);
	else
	if (explode_colour > RED) explode_colour -= 0.5;
	else
	if (explode_colour == RED) explode_colour = RED2;
}



////////////////////////////////// DRAW /////////////////////////////////////

void cl_silo::draw()
{
	if (flash_timer)
	{
		objDrawOrFillPolygon(YELLOW,SILO_VTX,left_vtx,true);
		objDrawOrFillPolygon(YELLOW,SILO_VTX,mid_vtx,true);
		objDrawOrFillPolygon(YELLOW,SILO_VTX,right_vtx,true);
		return;
	}
	objDrawOrFillPolygon(colour[0],SILO_VTX,left_vtx,true);
	objDrawOrFillPolygon(colour[1],SILO_VTX,mid_vtx,true);
	objDrawOrFillPolygon(colour[2],SILO_VTX,right_vtx,true);
}




void cl_silo::drawExplode()
{
	objDrawOrFillCircle(explode_colour,0,fireball_diam,0,0,true);
	drawExplodeBits();
}


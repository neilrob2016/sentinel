#
# This only appears to work properly with gmake
#

# Use for OpenSound
#SOUND=-DSOUND

# Use for ALSA
SOUND=-DSOUND -DALSA
ALSALIB=-lasound

CC=g++
COMP=$(CC) $(SOUND) -I/usr/X11/include -Wall -pedantic -g -O2 -c $< 
BINARY=sen

OBJS= \
	main.o \
	cl_object.o \
	cl_ship.o \
	cl_sphere.o \
	cl_pod.o \
	cl_bomb.o \
	cl_moonbase.o \
	cl_silo.o \
	cl_lair.o \
	cl_laser.o \
	cl_ship_laser.o \
	cl_stalag_laser.o \
	cl_missile.o \
	cl_bullet.o \
	cl_plasma.o \
	cl_grypper.o \
	cl_skyth.o \
	cl_zombie.o \
	cl_stalag.o \
	cl_disruptor.o \
	cl_striker.o \
	cl_powerup.o \
	cl_shield_powerup.o \
	cl_laser_powerup.o \
	cl_health_powerup.o \
	cl_hyperspace_powerup.o \
	cl_tractorbeam_powerup.o \
	cl_reverse_g_powerup.o \
	cl_astro.o \
	cl_text.o \
	hills.o \
	draw.o \
	maths.o \
	common.o \
	sound.o

GM=globals.h Makefile

$(BINARY): $(OBJS) $(GM)
	$(CC) $(OBJS) $(SOUND) $(ALSALIB) -L/usr/X11R6/lib -L/usr/X11R6/lib64 -lX11 -lXext -lm -o $(BINARY)

build_date.h:
	echo "#define BUILD_DATE \"`date +'%Y-%m-%d %T'`\"" > build_date.h

# These are done individually because the .cc.o rule seems to ignore
# dependencies and lifes too short to figure out why
main.o: main.cc globals.h build_date.h Makefile
	$(COMP)

cl_object.o: cl_object.cc $(GM)
	$(COMP)

cl_ship.o: cl_ship.cc $(GM)
	$(COMP)

cl_sphere.o: cl_sphere.cc $(GM)
	$(COMP)

cl_pod.o: cl_pod.cc $(GM)
	$(COMP)

cl_bomb.o: cl_bomb.cc $(GM)
	$(COMP)

cl_moonbase.o: cl_moonbase.cc $(GM)
	$(COMP)

cl_lair.o: cl_lair.cc $(GM)
	$(COMP)

cl_silo.o: cl_silo.cc $(GM)
	$(COMP)

cl_laser.o: cl_laser.cc $(GM)
	$(COMP)

cl_ship_laser.o: cl_ship_laser.cc $(GM)
	$(COMP)

cl_stalag_laser.o: cl_stalag_laser.cc $(GM)
	$(COMP)

cl_missile.o: cl_missile.cc $(GM)
	$(COMP)

cl_bullet.o: cl_bullet.cc $(GM)
	$(COMP)

cl_plasma.o: cl_plasma.cc $(GM)
	$(COMP)

cl_grypper.o: cl_grypper.cc $(GM)
	$(COMP)

cl_skyth.o: cl_skyth.cc $(GM)
	$(COMP)

cl_zombie.o: cl_zombie.cc $(GM)
	$(COMP)

cl_stalag.o: cl_stalag.cc $(GM)
	$(COMP)

cl_striker.o: cl_striker.cc $(GM)
	$(COMP)

cl_disruptor.o: cl_disruptor.cc $(GM)
	$(COMP)

cl_powerup.o: cl_powerup.cc $(GM)
	$(COMP)

cl_shield_powerup.o: cl_shield_powerup.cc $(GM)
	$(COMP)

cl_laser_powerup.o: cl_laser_powerup.cc $(GM)
	$(COMP)

cl_health_powerup.o: cl_health_powerup.cc $(GM)
	$(COMP)

cl_hyperspace_powerup.o: cl_hyperspace_powerup.cc $(GM)
	$(COMP)

cl_tractorbeam_powerup.o: cl_tractorbeam_powerup.cc $(GM)
	$(COMP)

cl_zero_g_powerup.o: cl_zero_g_powerup.cc $(GM)
	$(COMP)

cl_reverse_g_powerup.o: cl_reverse_g_powerup.cc $(GM)
	$(COMP)

cl_astro.o: cl_astro.cc $(GM)
	$(COMP)

cl_text.o: cl_text.cc $(GM)
	$(COMP)

hills.o: hills.cc $(GM)
	$(COMP)

draw.o: draw.cc $(GM)
	$(COMP)

maths.o: maths.cc $(GM)
	$(COMP)

common.o: common.cc $(GM)
	$(COMP)

sound.o: sound.cc $(GM)
	$(COMP)

clean:
	rm -f $(BINARY) *.o build_date.h core

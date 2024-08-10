#ifndef  ANIMATION_H
#define ANIMATION_H

typedef struct
{
    // number of frames
	int nframes;
    // current frame count
	int fcount;
    // frame speed
	int fspeed;
    // x and y frame positions
	int xfposition;
	int yfposition;
} Animation;

void animate(Animation* animated_object);

#endif
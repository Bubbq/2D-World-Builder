#include "headers/animation.h"

void animate(Animation* animated_object)
{
	animated_object->fcount++;
	animated_object->xfposition = (animated_object->fcount / animated_object->fspeed);
	
    if(animated_object->xfposition > animated_object->nframes) 
        animated_object->xfposition = animated_object->fcount = 0;
}
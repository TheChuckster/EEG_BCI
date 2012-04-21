#ifndef P300_H_
#define P300_H_

#include "SDL.h"

// state variables
extern unsigned int p300_state;

// methods
void p300UpdateAndRender();
void p300HandleKeyUp(SDL_keysym *keysym);
void p300HandleKeyDown(SDL_keysym *keysym);
void p300AddSample(unsigned int s);
void p300init();

#endif


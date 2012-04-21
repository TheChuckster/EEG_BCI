#ifndef PONG_H_
#define PONG_H_

void pongInit();
void pongUpdateAndRender();

void pongHandleKeyDown(SDL_keysym *keysym);
void pongHandleKeyUp(SDL_keysym *keysym);

extern float posy, paddle1yvel;

#endif


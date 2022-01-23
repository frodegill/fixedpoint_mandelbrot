#include <SDL2/SDL.h>

#define DECIMAL_BITS (12)
#define TO_FP_CONST(x) (Sint32)((x)*(1<<DECIMAL_BITS))
#define FROM_FP(x) (Sint16)((x)>>DECIMAL_BITS)
#define FROM_FP_BITS(x,bits) (Sint32)((x)>>(bits))

#define UPPER_LEFT_R_FP   (TO_FP_CONST(-1.4))
#define UPPER_LEFT_I_FP   (TO_FP_CONST(0.2))

#define WIDTH (800)
#define HEIGHT (600)
#define PIXEL_SIZE_FP    (TO_FP_CONST(0.55/WIDTH))

#define RADIUS        (1750)
#define RADIUS_SQR    (RADIUS*RADIUS)

#define MAXITER         (30)
#define BAIL_OUT_SQR    (TO_FP_CONST(4.0))


static const Uint8 RIGHT = 0;
static const Uint8 UP_RIGHT = 1;
static const Uint8 UP = 2;
static const Uint8 UP_LEFT = 3;
static const Uint8 LEFT = 4;
static const Uint8 DOWN_LEFT = 5;
static const Uint8 DOWN = 6;
static const Uint8 DOWN_RIGHT = 7;
static const Uint8 NO_DIRECTION = 8;
Uint32 direction_cache[9];


Uint8 mandelbrot_pixel(Sint16 c_r, Sint16 c_i)
{
  Uint8 n;
  Sint16 z_r, z_i, tmp16;
  Sint16 xsqr, ysqr;
  Sint32 tmp32_1, tmp32_2;

  z_r = c_r;
  z_i = c_i;

  xsqr = FROM_FP(z_r*z_r);
  ysqr = FROM_FP(z_i*z_i);

  //Already out of bounds on one axis
  if (z_r<=TO_FP_CONST(-2.0) || z_r>=TO_FP_CONST(2.0) || z_i<=TO_FP_CONST(-2.0) || z_i>=TO_FP_CONST(2.0)) {
    return 0;
  }

  //Already out of bounds axis combined
  if (xsqr+ysqr >= TO_FP_CONST(4.0)) {
    return 0;
  }

  //We know it will bail out
  tmp16 = z_r+TO_FP_CONST(0.5);
  tmp16 = FROM_FP(tmp16*tmp16);
  if ((tmp16+ysqr) > TO_FP_CONST(2.25) ) {
    return 0;
  }

  //We know it will never bail out (large Mandelbrot blob)
  tmp16 = z_r-TO_FP_CONST(0.25);
  tmp16 = FROM_FP(tmp16*tmp16);
  if (FROM_FP_BITS((tmp16+ysqr)*((xsqr+(z_r>>1)+ysqr-TO_FP_CONST(0.1875))), DECIMAL_BITS-2) < ysqr) {
    return MAXITER+1;
  }

  //We know it will never bail out (smaller Mandelbrot blob)
  tmp16 = z_r+TO_FP_CONST(1.0);
  tmp16 = FROM_FP(tmp16*tmp16);
  if ((tmp16+ysqr) < TO_FP_CONST(0.0625)) {
    return MAXITER+1;
  }

  /* Applies the actual mandelbrot formula on that point */
  for (n=0;; n++) {
    tmp32_1 = z_r*z_r;
    tmp32_2 = z_i*z_i;
    if (n>MAXITER || (tmp32_1+tmp32_2)>=TO_FP_CONST(BAIL_OUT_SQR))
    {
      break;
    }

    tmp16 = FROM_FP(tmp32_1) - FROM_FP(tmp32_2) + c_r;
    z_i = ((z_r*z_i)>>(DECIMAL_BITS-1)) + c_i;
    z_r = tmp16;
  }

  return n;
}


void sdl_draw_horizontal_mandelbrot(SDL_Window *window, SDL_Surface *surface, Sint16 upper_left_r_fp, Sint16 upper_left_i_fp, int y)
{
    int x,n;
    Sint16 c_r_fp = upper_left_r_fp;
    Sint16 c_i_fp = upper_left_i_fp - y*PIXEL_SIZE_FP;

    int r, g, b;
    for (x = 0; x < WIDTH; x++)
    {
        r = g = b = mandelbrot_pixel(c_r_fp, c_i_fp)*255/(MAXITER+1);
        ((Uint32*)surface->pixels)[(y*surface->w) + x] = SDL_MapRGB(surface->format, r, g, b);

        c_r_fp += PIXEL_SIZE_FP;
    }
}

void sdl_draw_vertical_mandelbrot(SDL_Window *window, SDL_Surface *surface, Sint16 upper_left_r_fp, Sint16 upper_left_i_fp, int x)
{
    int y,n;
    Sint16 c_r_fp = upper_left_r_fp + x*PIXEL_SIZE_FP;
    Sint16 c_i_fp = upper_left_i_fp;

    int r, g, b;
    for (y = 0; y < HEIGHT; y++)
    {
        r = g = b = mandelbrot_pixel(c_r_fp, c_i_fp)*255/(MAXITER+1);
        ((Uint32*)surface->pixels)[(y*surface->w) + x] = SDL_MapRGB(surface->format, r, g, b);

        c_i_fp -= PIXEL_SIZE_FP;
    }
}

void sdl_draw_mandelbrot(SDL_Window *window, SDL_Surface *surface, Sint16 upper_left_r_fp, Sint16 upper_left_i_fp)
{
    for (int y = 0; y < HEIGHT; y++)
    {
        sdl_draw_horizontal_mandelbrot(window, surface, upper_left_r_fp, upper_left_i_fp, y);
    }
    SDL_UpdateWindowSurface(window);
}

Uint8 mandelbrot_pan_direction(int cx, int cy, Uint8 previous)
{
    previous ^= 4; //Negate previous direction. If previous was UP_RIGHT, we want to avoid going DOWN_LEFT now

    direction_cache[NO_DIRECTION] = RADIUS_SQR;
    direction_cache[UP] = direction_cache[DOWN] = (cx * cx) + (cy * cy);
    direction_cache[UP_LEFT] = direction_cache[LEFT] = direction_cache[DOWN_LEFT] = direction_cache[UP] - cx - (cx-1);
    direction_cache[UP_RIGHT] = direction_cache[RIGHT] = direction_cache[DOWN_RIGHT] = direction_cache[UP] + cx + (cx+1);
    direction_cache[UP_LEFT] += cy + (cy+1);    // +cy + (cy+1)
    direction_cache[UP] += cy + (cy+1);         // +cy + (cy+1)
    direction_cache[UP_RIGHT] += cy + (cy+1);   // +cy + (cy+1)
    direction_cache[DOWN_LEFT] += -cy - (cy-1);  // -cy - (cy-1)
    direction_cache[DOWN] += -cy - (cy-1);       // -cy - (cy-1)
    direction_cache[DOWN_RIGHT] += -cy - (cy-1); // -cy - (cy-1)

    int tmp;
    for (tmp=RIGHT; tmp<=DOWN_RIGHT; tmp++)
    {
        direction_cache[tmp] = (direction_cache[tmp]>direction_cache[NO_DIRECTION]) ?
                                  direction_cache[tmp]-direction_cache[NO_DIRECTION] : direction_cache[NO_DIRECTION]-direction_cache[tmp];
    }

    Uint8 direction;
    if (previous == RIGHT) {
        direction = UP_RIGHT;
    } else {
        direction = (previous!=UP_RIGHT && direction_cache[UP_RIGHT]<direction_cache[RIGHT]) ? UP_RIGHT : RIGHT;
    }

    for (tmp=UP; tmp<=DOWN_RIGHT; tmp++)
    {
        if (previous!=tmp && direction_cache[tmp]<direction_cache[direction])
        {
            direction=tmp;
        }
    }

    return direction;
}

int main(int argc, char **argv)
{
    /* SDL setup */
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
    {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    Sint16 upper_left_r_fp = UPPER_LEFT_R_FP;
    Sint16 upper_left_i_fp = UPPER_LEFT_I_FP;

    SDL_Window* window = SDL_CreateWindow("SDL Mandelbrot",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            WIDTH,
            HEIGHT,
            SDL_WINDOW_OPENGL
            );

    SDL_Surface* surface = SDL_GetWindowSurface(window);

    sdl_draw_mandelbrot(window, surface, upper_left_r_fp, upper_left_i_fp);

    SDL_Event event;
    int cx = -RADIUS;
    int cy = 0;
    Uint8 direction, previous = DOWN;
    int dx, dy;
    int mx1, mx2, my1, my2, tmpx, tmpy;

    while(1)
    {
        SDL_PollEvent(&event);
        switch (event.type)
        {
            case SDL_QUIT:
                SDL_DestroyWindow(window);
                SDL_Quit();
                return 0;
        }
        
        direction = mandelbrot_pan_direction(cx, cy, previous);
        if (direction==RIGHT) {cx++;                 upper_left_r_fp+=PIXEL_SIZE_FP;                                 dx=1;  dy=0;  mx1=0; mx2=WIDTH-1; my1=0; my2=HEIGHT;}
        else if (direction==UP_RIGHT) {cx++; cy++;   upper_left_r_fp+=PIXEL_SIZE_FP; upper_left_i_fp+=PIXEL_SIZE_FP; dx=1;  dy=1;  mx1=0; mx2=WIDTH-1; my1=HEIGHT-1; my2=0;}
        else if (direction==UP) {cy++;                                               upper_left_i_fp+=PIXEL_SIZE_FP; dx=0;  dy=1;  mx1=0; mx2=WIDTH-1; my1=HEIGHT-1; my2=0;}
        else if (direction==UP_LEFT) {cx--; cy++;    upper_left_r_fp-=PIXEL_SIZE_FP; upper_left_i_fp+=PIXEL_SIZE_FP; dx=-1; dy=1;  mx1=WIDTH-1; mx2=0; my1=HEIGHT-1; my2=0;}
        else if (direction==LEFT) {cx--;             upper_left_r_fp-=PIXEL_SIZE_FP;                                 dx=-1; dy=0;  mx1=WIDTH-1; mx2=0; my1=0; my2=HEIGHT;}
        else if (direction==DOWN_LEFT) {cx--; cy--;  upper_left_r_fp-=PIXEL_SIZE_FP; upper_left_i_fp-=PIXEL_SIZE_FP; dx=-1; dy=-1; mx1=WIDTH-1; mx2=0; my1=0; my2=HEIGHT-1;}
        else if (direction==DOWN) {cy--;                                             upper_left_i_fp-=PIXEL_SIZE_FP; dx=0;  dy=-1; mx1=0; mx2=WIDTH-1; my1=0; my2=HEIGHT-1;}
        else if (direction==DOWN_RIGHT) {cx++; cy--; upper_left_r_fp+=PIXEL_SIZE_FP; upper_left_i_fp-=PIXEL_SIZE_FP; dx=1;  dy=-1; mx1=0; mx2=WIDTH-1; my1=0; my2=HEIGHT-1;}

        for (tmpx=mx1; tmpx!=mx2; )
        {
            for (tmpy=my1; tmpy!=my2;)
            {
                ((Uint32*)surface->pixels)[(tmpy*surface->w) + tmpx] = ((Uint32*)surface->pixels)[((tmpy-dy)*surface->w) + (tmpx+dx)];

                if (my1>my2) tmpy--; else tmpy++;
            }

            if (mx1>mx2) tmpx--; else tmpx++;
        }

        if (dx!=0)
        {
            sdl_draw_vertical_mandelbrot(window, surface, upper_left_r_fp, upper_left_i_fp, dx==-1 ? 0 : WIDTH-1);
        }
        if (dy!=0)
        {
            sdl_draw_horizontal_mandelbrot(window, surface, upper_left_r_fp, upper_left_i_fp, dy==-1 ? HEIGHT-1 : 0);
        }
        SDL_UpdateWindowSurface(window);

        previous=direction;
    }
}

#ifndef SF_TOUCH_H
#define SF_TOUCH_H
#include <stdint.h>
enum { B_LEFT=1, B_RIGHT=2, B_UP=4, B_DOWN=8, B_CROSS=16, B_SQUARE=32, B_TRIANGLE=64, B_CIRCLE=128 };
enum { T_DOWN=0, T_UP=1, T_MOVE=2, T_CANCEL=3, T_POINTER_DOWN=5, T_POINTER_UP=6 };
typedef struct { int id; float x,y; } Pointer;
typedef struct { int action, index, count; uint64_t time_us; Pointer points[5]; } TouchEvent;
typedef void (*TouchSink)(const TouchEvent *,void *);
typedef struct { Pointer active[5]; int count; } TouchState;
void touch_step(TouchState *,unsigned,float,float,uint64_t,TouchSink,void *);
void touch_cancel(TouchState *,uint64_t,TouchSink,void *);
#endif

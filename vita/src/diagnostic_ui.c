#include <vitaGL.h>
#include "diagnostic_ui.h"
/* Small fixed bitmap labels, drawn with scissored clears (no custom shaders). */
static const unsigned char letters[26][5]={
 {126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
 {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
 {0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
 {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
 {62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},
 {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
 {7,8,112,8,7},{97,81,73,69,67}};
static void box(int x,int y,int w,int h,float r,float g,float b){
    glScissor(x,544-y-h,w,h);glClearColor(r,g,b,1);glClear(GL_COLOR_BUFFER_BIT);
}
static void text(int x,int y,const char *s){
    for(;*s;s++,x+=18){
        if(*s<'A'||*s>'Z')continue;
        const unsigned char *glyph=letters[*s-'A'];
        for(int col=0;col<5;col++)for(int row=0;row<7;row++)
            if(glyph[col]&(1<<row))box(x+col*3,y+row*3,3,3,.85f,.89f,.95f);
    }
}
void diagnostic_ui(const TouchState *s,int log_ready,int ok){
    glDisable(GL_SCISSOR_TEST);glClearColor(.035f,.05f,.08f,1);glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);
    box(28,28,904,5,ok?.15f:.8f,ok?.8f:.1f,.25f);
    text(32,52,"SF INPUT DIAGNOSTIC");text(32,92,"UNITY NOT STARTED");
    text(32,132,log_ready?"LOG READY":"LOG OPEN FAILED");text(32,172,ok?"CORE CHECK PASS":"CORE CHECK FAILED");
    text(32,224,"STICK AND BUTTON TEST");text(32,264,"START TO EXIT");
    box(85,345,150,150,.09f,.13f,.19f);
    const int xy[4][2]={{840,450},{750,400},{840,350},{920,400}};
    for(int i=0;i<4;i++)box(xy[i][0]-18,xy[i][1]-18,36,36,.14f,.17f,.23f);
    for(int i=0;i<s->count;i++)box((int)s->active[i].x-12,(int)s->active[i].y-12,24,24,.15f,.8f,.65f);
    glDisable(GL_SCISSOR_TEST);
}

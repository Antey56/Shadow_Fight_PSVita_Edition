#include "native_registry.h"
#include <assert.h>
#include <stdio.h>
static void first(void){} static void second(void){}
int main(void){
    NativeRegistry r={0};
    assert(native_bind(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z",first)==0);
    assert(native_lookup(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z")==first);
    assert(!native_lookup(&r,"com/unity3d/player/UnityPlayer","nativeRender","()V"));
    assert(native_bind(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z",second)==-2);
    assert(native_bind(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z",first)==0);
    assert(r.count==1);assert(native_bind(&r,0,"n","()V",first)==-1);
    puts("native registry PASS");return 0;
}

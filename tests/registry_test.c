#include "native_registry.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void first(void){} static void second(void){}
int main(void){
    NativeRegistry r={0};
    assert(native_bind(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z",first)==0);
    assert(native_lookup(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z")==first);
    assert(!native_lookup(&r,"com/unity3d/player/UnityPlayer","nativeRender","()V"));
    assert(native_bind(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z",second)==-2);
    assert(native_bind(&r,"com/unity3d/player/UnityPlayer","nativeRender","()Z",first)==0);
    assert(r.count==1);assert(native_bind(&r,0,"n","()V",first)==-1);
    NativeDeclaration d[]={{"c","a","()V"},{"c","b","(I)Z"}};
    NativeBinding b[]={{"c","a","()V",first},{"c","b","(I)Z",second}};
    NativeRegistry before=r;
    b[1].descriptor="()V";
    assert(native_register_checked(&r,d,2,b,2)==-4);
    assert(!memcmp(&r,&before,sizeof r));
    b[1].descriptor="(I)Z";
    assert(native_register_checked(&r,d,2,b,2)==0);assert(r.count==3);
    assert(native_lookup(&r,"c","b","(I)Z")==second);
    before=r;b[1].address=first;
    assert(native_register_checked(&r,d,2,b,2)==-2);
    assert(!memcmp(&r,&before,sizeof r));
    b[1].address=0;assert(native_register_checked(&r,d,2,b,2)==-1);
    assert(!memcmp(&r,&before,sizeof r));
    assert(native_register_checked(&r,d,2,b,0)==-1);
    assert(native_register_checked(&r,0,2,b,2)==-1);
    NativeRegistry full={0};full.count=128;
    for(size_t i=0;i<128;i++)full.entries[i]=(NativeBinding){"existing","x","()V",first};
    before=full;
    assert(native_register_checked(&full,d,2,b,1)==-3);
    assert(!memcmp(&full,&before,sizeof full));
    full.count=129;assert(native_bind(&full,"c","a","()V",first)==-1);
    assert(!native_lookup(&full,"c","a","()V"));
    puts("native registry transaction PASS");return 0;
}

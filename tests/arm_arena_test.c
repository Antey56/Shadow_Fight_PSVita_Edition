#include "arm_arena.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {unsigned char *bytes;size_t capacity;int writes,syncs,fail;} Store;
static int write_bytes(void *ctx,size_t offset,const unsigned char *bytes,size_t count){
    Store *s=ctx;assert(offset<=s->capacity&&count<=s->capacity-offset);
    s->writes++;if(s->fail==1)return -71;memcpy(s->bytes+offset,bytes,count);return 0;
}
static int sync_bytes(void *ctx,size_t offset,size_t count){
    Store *s=ctx;assert(offset<=s->capacity&&count<=s->capacity-offset);s->syncs++;
    return s->fail==2?-72:0;
}
int main(void){
    Store store={calloc(64*1024*1024,1),64*1024*1024,0,0,0};assert(store.bytes);
    ArmArenaStorage storage={&store,write_bytes,sync_bytes};ArmArena arena;
    assert(!arm_arena_init(&arena,0x90001000,store.capacity,&storage));
    ArmExecBackend b=arm_arena_backend(&arena);uint32_t main_base,unity_base,il2cpp_base;uintptr_t main_cookie,unity_cookie,il2cpp_cookie;
    assert(!b.reserve(b.context,24576,4096,&main_base,&main_cookie));
    assert(!b.reserve(b.context,13893632,4096,&unity_base,&unity_cookie));
    assert(!b.reserve(b.context,39088128,65536,&il2cpp_base,&il2cpp_cookie));
    assert(main_base+24576<=unity_base&&unity_base+13893632<=il2cpp_base);
    assert(!(il2cpp_base%65536));
    assert(arm_arena_used(&arena)==24576u+13893632u+39088128u);
    uint32_t out=1;uintptr_t cookie=1;
    assert(b.reserve(b.context,16*1024*1024,4096,&out,&cookie)==-62&&out==0&&cookie==0);
    b.release(b.context,unity_cookie);b.release(b.context,unity_cookie);
    assert(!b.reserve(b.context,8192,4096,&out,&cookie)&&out==unity_base&&cookie!=unity_cookie);
    b.release(b.context,unity_cookie); /* stale cookie must not free replacement */
    unsigned char bytes[8192];memset(bytes,0x5a,sizeof bytes);
    assert(b.sync(b.context,cookie,sizeof bytes)==-64);
    assert(b.publish(b.context,cookie,bytes,8191)==-63);
    store.fail=1;assert(b.publish(b.context,cookie,bytes,sizeof bytes)==-71);
    store.fail=0;assert(!b.publish(b.context,cookie,bytes,sizeof bytes));
    assert(b.publish(b.context,cookie,bytes,sizeof bytes)==-63);
    assert(!memcmp(store.bytes+out-arena.base,bytes,sizeof bytes));
    store.fail=2;assert(b.sync(b.context,cookie,sizeof bytes)==-72);
    store.fail=0;assert(!b.sync(b.context,cookie,sizeof bytes));
    assert(b.sync(b.context,cookie,sizeof bytes)==-64);
    assert(b.publish(b.context,unity_cookie,bytes,sizeof bytes)==-63);
    b.release(b.context,cookie);b.release(b.context,main_cookie);b.release(b.context,il2cpp_cookie);
    assert(!arm_arena_used(&arena));
    uintptr_t slots[ARM_ARENA_SLOTS];
    for(unsigned i=0;i<ARM_ARENA_SLOTS;i++)assert(!b.reserve(b.context,1,4096,&out,&slots[i]));
    assert(b.reserve(b.context,1,4096,&out,&cookie)==-61);
    for(unsigned i=0;i<ARM_ARENA_SLOTS;i++)b.release(b.context,slots[i]);
    assert(!arm_arena_used(&arena));
    assert(b.reserve(b.context,1,3,&out,&cookie)==-60);
    assert(b.reserve(b.context,0,4096,&out,&cookie)==-60);
    /* Released adjacent ranges coalesce implicitly through first-fit scanning. */
    assert(!b.reserve(b.context,store.capacity,4096,&out,&cookie)&&out==arena.base);
    b.release(b.context,cookie);
    assert(arm_arena_init(&arena,0xfffff000,8192,&storage)==-60);
    assert(arm_arena_init(&arena,0x90000001,4096,&storage)==-60);
    free(store.bytes);puts("Multi-module arena bounds, fragmentation, stale cookies and publish/sync order PASS");return 0;
}

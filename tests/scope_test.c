#include "arm_scope.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    ArmScopeEntry entries[]={{"first","function",0x1001,2,2},{"second","function",0x2001,2,1},{"host","counter",0x3000,1,1}};
    ArmSymbolScope scope;uint32_t address=99;const ArmScopeEntry *selected=0;
    assert(arm_scope_init(&scope,entries,3)==0);
    assert(arm_scope_resolve("function",1,2,&address,&scope)==1&&address==0x1001);
    assert(arm_scope_find(&scope,"counter",1,&selected)==1&&selected==&entries[2]);
    address=99;assert(arm_scope_resolve("missing",1,2,&address,&scope)==0&&address==99);
    assert(arm_scope_resolve("counter",1,2,&address,&scope)==-4&&address==99);
    ArmScopeEntry duplicate[]={{"same","foo",1,1,1},{"same","foo",2,1,1}};
    assert(arm_scope_init(&scope,duplicate,2)==-3&&scope.count==0);
    assert(arm_scope_init(&scope,0,0)==0);
    assert(arm_scope_resolve("x",1,0,&address,&scope)==0);
    puts("ordered target symbol scope PASS");return 0;
}

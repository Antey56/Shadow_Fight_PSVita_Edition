#include "elf_preflight.h"
#include <stdio.h>
int main(int argc,char **argv){
    if(argc<2){fprintf(stderr,"usage: elf_preflight module.so [...]\n");return 2;}
    int failed=0;
    for(int i=1;i<argc;i++){
        ElfPreflight e;int rc=sf2_elf_inspect(argv[i],&e);
        printf("%s: %s",argv[i],sf2_elf_error(rc));
        if(!rc)printf("; flags=0x%x loads=%u span=%u tls=%d wx=%d",e.flags,e.load_count,e.load_max-e.load_min,e.has_tls,e.writable_executable);
        puts("");if(rc)failed=1;
    }
    return failed;
}

#include "sin_compat/legacy_bridge_c_api.h"
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"sin100 smoke failed: %s at line %d\n", #x, __LINE__); return 2; } } while(0)
typedef uint32_t (*version_fn)(void);
typedef sin100_status (*create_fn)(uint32_t,sin100_context**);
typedef void (*destroy_fn)(sin100_context*);
typedef sin100_status (*install_fn)(sin100_context*,const uint8_t*,uint32_t);
typedef sin100_status (*key_fn)(sin100_context*,int32_t,uint32_t*);
typedef sin100_status (*bind_fn)(sin100_context*,int32_t,int32_t,int64_t,uint32_t,const char*,uint32_t,const char*,uint32_t);
typedef sin100_status (*lookup_old_fn)(sin100_context*,int32_t,uint32_t*);
typedef sin100_status (*lookup_native_fn)(sin100_context*,uint32_t,uint32_t*);
typedef sin100_status (*name_fn)(sin100_context*,int32_t,char*,uint32_t,uint32_t*);
typedef sin100_status (*count_fn)(sin100_context*,uint32_t*);
typedef sin100_status (*reset_fn)(sin100_context*);
static int load_symbol(void* handle,const char* name,void* destination,size_t n){
    void* symbol=dlsym(handle,name);
    if(!symbol || n!=sizeof symbol){fprintf(stderr,"Missing ABI symbol %s: %s\n",name,dlerror());return 0;}
    memcpy(destination,&symbol,sizeof symbol);
    return 1;
}
#define LOAD(ident,type,symbol) type ident=NULL; REQUIRE(load_symbol(handle,symbol,&ident,sizeof ident))
int main(int argc,char** argv){
    if(argc!=2){fprintf(stderr,"Usage: smoke /path/libsin_legacy_bridge_abi.so\n");return 3;}
    void* handle=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);
    if(!handle){fprintf(stderr,"dlopen: %s\n",dlerror());return 3;}
    LOAD(version,version_fn,"sin100_abi_version");
    LOAD(create,create_fn,"sin100_create");
    LOAD(destroy,destroy_fn,"sin100_destroy");
    LOAD(install,install_fn,"sin100_install_keymap_le");
    LOAD(key,key_fn,"sin100_translate_key");
    LOAD(bind,bind_fn,"sin100_bind_sound");
    LOAD(old_to_native,lookup_old_fn,"sin100_old_to_native");
    LOAD(native_to_old,lookup_native_fn,"sin100_native_to_old");
    LOAD(copy_name,name_fn,"sin100_copy_sound_name");
    LOAD(sound_count,count_fn,"sin100_sound_count");
    LOAD(reset,reset_fn,"sin100_reset");
    REQUIRE(version()==SIN100_ABI_VERSION);
    sin100_context *ctx=NULL,*another=NULL;
    REQUIRE(create(0xdeadbeefu,&ctx)==SIN100_BAD_VERSION && ctx==NULL);
    REQUIRE(create(SIN100_ABI_VERSION,&ctx)==SIN100_OK && ctx!=NULL);
    REQUIRE(create(SIN100_ABI_VERSION,&another)==SIN100_OK && another!=NULL);
    uint32_t number=0;
    REQUIRE(key(ctx,48,&number)==SIN100_NOT_READY && number==0);
    uint8_t invalid[1024]={0};
    REQUIRE(install(ctx,invalid,sizeof invalid)==SIN100_HASH_MISMATCH);
    REQUIRE(install(ctx,invalid,3)==SIN100_INVALID_ARGUMENT);
    REQUIRE(key(ctx,48,&number)==SIN100_NOT_READY);
    REQUIRE(bind(ctx,70000,100000,77,0,"step",4,"step",4)==SIN100_VALIDATION_REQUIRED);
    REQUIRE(bind(ctx,70000,100000,65535,1,"step",4,"step",4)==SIN100_NATIVE_INDEX_INVALID);
    REQUIRE(bind(ctx,70000,100000,77,1,"step",4,"Step",4)==SIN100_NAME_MISMATCH);
    REQUIRE(bind(ctx,70000,100000,77,1,"step",4,"step",4)==SIN100_OK);
    REQUIRE(bind(ctx,70000,100000,77,1,"step",4,"step",4)==SIN100_ALREADY_PRESENT);
    REQUIRE(bind(ctx,70001,100000,77,1,"other",5,"other",5)==SIN100_MAPPING_CONFLICT);
    REQUIRE(old_to_native(ctx,70000,&number)==SIN100_OK && number==77);
    REQUIRE(native_to_old(ctx,77,&number)==SIN100_OK && number==70000);
    REQUIRE(old_to_native(another,70000,&number)==SIN100_NOT_FOUND && number==SIN100_NOT_FOUND_TOKEN);
    REQUIRE(native_to_old(ctx,65535,&number)==SIN100_NATIVE_INDEX_INVALID);
    uint32_t required=0;
    REQUIRE(copy_name(ctx,70000,NULL,0,&required)==SIN100_BUFFER_TOO_SMALL && required==5);
    char small[3]={'X','Y','Z'};
    REQUIRE(copy_name(ctx,70000,small,sizeof small,&required)==SIN100_BUFFER_TOO_SMALL && small[0]=='X');
    char name[8]={0};
    REQUIRE(copy_name(ctx,70000,name,sizeof name,&required)==SIN100_OK && required==5 && strcmp(name,"step")==0);
    REQUIRE(copy_name(ctx,70001,name,sizeof name,&required)==SIN100_NOT_FOUND && required==0);
    REQUIRE(sound_count(ctx,&number)==SIN100_OK && number==1);
    // Optional local owner's private verified map, never used by public CI.
    const char* keymap=getenv("SIN100_PRIVATE_KEYMAP_BYTES");
    if(keymap && *keymap){
        FILE* file=fopen(keymap,"rb");REQUIRE(file!=NULL);
        uint8_t data[1024];REQUIRE(fread(data,1,sizeof data,file)==sizeof data);
        REQUIRE(fgetc(file)==EOF);REQUIRE(fclose(file)==0);
        REQUIRE(install(ctx,data,sizeof data)==SIN100_OK);
        REQUIRE(key(ctx,48,&number)==SIN100_OK && number==1);
        REQUIRE(key(ctx,-1,&number)==SIN100_OK && number==0);
        REQUIRE(key(ctx,256,&number)==SIN100_OK && number==0);
    }
    REQUIRE(reset(ctx)==SIN100_OK);
    REQUIRE(old_to_native(ctx,70000,&number)==SIN100_NOT_FOUND);
    REQUIRE(key(ctx,48,&number)==SIN100_NOT_READY);
    destroy(another);destroy(ctx);
    REQUIRE(dlclose(handle)==0);
    puts("PASS: native shared-library dlopen + C ABI isolation, error and token translation tests");
    return 0;
}

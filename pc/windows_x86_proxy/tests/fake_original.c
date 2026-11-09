/* Test fixtures: no original game assets, binaries, or engine code. */
#include <stddef.h>
#include <string.h>
#if defined(SIN_PC_FAKE_SERVER) == defined(SIN_PC_FAKE_CLIENT)
#error "Specify exactly one synthetic original role"
#endif

#if defined(SIN_PC_FAKE_SERVER)
#define REQUEST_NAME "ServerGameDLL003"
static unsigned long instance_token = 0x53494E53UL;
void __cdecl FakeDetonateHomer(void) {}
void __cdecl FakeDetonatePathfollower(void) {}
void __cdecl FakeDetonateSpitMutant(void) {}
void __cdecl FakeDetonateSpitPoison(void) {}
#else
#define REQUEST_NAME "VClient011"
static unsigned long instance_token = 0x53494E43UL;
#endif

void *__cdecl CreateInterface(const char *name, int *return_code) {
    if (return_code != NULL)
        *return_code = 1;
    if (name == NULL || strcmp(name, REQUEST_NAME) != 0)
        return NULL;
    if (return_code != NULL)
        *return_code = 0;
    return &instance_token;
}

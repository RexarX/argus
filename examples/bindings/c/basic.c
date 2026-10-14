#include <argus/argus.h>

#include <stdbool.h>
#include <stdio.h>

int main(void) {
    const ArgusVersion header = argus_header_version();
    const ArgusVersion linked = argus_linked_version();
    const bool compatible = argus_compatible_with_headers();

    printf("argus headers %u.%u.%u\n", header.major, header.minor,
           header.patch);
    printf("argus linked %u.%u.%u (%s) compatible=%d\n", linked.major,
           linked.minor, linked.patch, argus_version_string(),
           compatible ? 1 : 0);

    ARGUS_C_ASSERT(compatible);
    return compatible ? 0 : 1;
}

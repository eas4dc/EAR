#include <common/types/application.h>
#include <pbs_hook/pbs_hook_utils.h>

uint get_ID() {
    return create_ID(job, step);
}

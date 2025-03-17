#include "kutils.h"
#include "kextrw.h"
#include "offsets.h"

#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>

uint64_t proc_of_pid(pid_t pid) {
    uint64_t proc = kread64(ksym(KSYMBOL_KERNPROC));
    
    while (1) {
        if(kread32(proc + off_p_pid) == pid) {
            return proc;
        }
        proc = kread64(proc + off_p_list_le_prev);
        if(!proc) {
            return -1;
        }
    }
    return 0;
}

uint64_t task_self_addr(void) {
    uint64_t proc = proc_of_pid(getpid());
    uint64_t p_proc_ro = kread64(proc + off_p_proc_ro);
    uint64_t pr_task = kread64(p_proc_ro + off_p_ro_pr_task);
    return pr_task;
}
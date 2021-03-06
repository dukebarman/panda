/* PANDABEGINCOMMENT
 * 
 * Authors:
 *  Tim Leek               tleek@ll.mit.edu
 *  Ryan Whelan            rwhelan@ll.mit.edu
 *  Joshua Hodosh          josh.hodosh@ll.mit.edu
 *  Michael Zhivich        mzhivich@ll.mit.edu
 *  Brendan Dolan-Gavitt   brendandg@gatech.edu
 * 
 * This work is licensed under the terms of the GNU GPL, version 2. 
 * See the COPYING file in the top-level directory. 
 * 
PANDAENDCOMMENT */
// This needs to be defined before anything is included in order to get
// the PRIx64 macro
#define __STDC_FORMAT_MACROS

// Choose a granularity for the OSI code to be invoked.
#define INVOKE_FREQ_PGD
//#define INVOKE_FREQ_BBL

#include "panda/plugin.h"
#include "osi/osi_types.h"
#include "osi/osi_ext.h"

bool init_plugin(void *);
void uninit_plugin(void *);

int asid_changed(CPUState *cpu, target_ulong old_pgd, target_ulong new_pgd);
int before_block_exec(CPUState *cpu, TranslationBlock *tb);

int before_block_exec(CPUState *cpu, TranslationBlock *tb) {
    int i;

    OsiProc *current = get_current_process(cpu);
    printf("Current process: %s PID:" TARGET_FMT_ld " PPID:" TARGET_FMT_ld "\n", current->name, current->pid, current->ppid);

    OsiModules *ms = get_libraries(cpu, current);
    if (ms == NULL) {
        printf("No mapped dynamic libraries.\n");
    }
    else {
        printf("Dynamic libraries list (%d libs):\n", ms->num);
        for (i = 0; i < ms->num; i++)
            printf("\t0x" TARGET_FMT_lx "\t" TARGET_FMT_ld "\t%-24s %s\n", ms->module[i].base, ms->module[i].size, ms->module[i].name, ms->module[i].file);
    }

    printf("\n");

    OsiProcs *ps = get_processes(cpu);
    if (ps == NULL) {
        printf("Process list not available.\n");
    }
    else {
        printf("Process list (%d procs):\n", ps->num);
        for (i = 0; i < ps->num; i++)
            printf("  %-16s\t" TARGET_FMT_ld "\t" TARGET_FMT_ld "\n", ps->proc[i].name, ps->proc[i].pid, ps->proc[i].ppid);
    }

    printf("\n");

    OsiModules *kms = get_modules(cpu);
    if (kms == NULL) {
        printf("No mapped kernel modules.\n");
    }
    else {
        printf("Kernel module list (%d modules):\n", kms->num);
        for (i = 0; i < kms->num; i++)
            printf("\t0x" TARGET_FMT_lx "\t" TARGET_FMT_ld "\t%-24s %s\n", kms->module[i].base, kms->module[i].size, kms->module[i].name, kms->module[i].file);
    }

    printf("\n-------------------------------------------------\n\n");

    // Cleanup
    free_osiproc(current);
    free_osiprocs(ps);
    free_osimodules(ms);

    return 0;
}

int asid_changed(CPUState *cpu, target_ulong old_pgd, target_ulong new_pgd) {
    // tb argument is not used by before_block_exec()
    return before_block_exec(cpu, NULL);
}

bool init_plugin(void *self) {
#if defined(INVOKE_FREQ_PGD)
    // relatively short execution
    panda_cb pcb = { .asid_changed = asid_changed };
    panda_register_callback(self, PANDA_CB_ASID_CHANGED, pcb);
#else
    // expect this to take forever to run
    panda_cb pcb = { .before_block_exec = before_block_exec };
    panda_register_callback(self, PANDA_CB_BEFORE_BLOCK_EXEC, pcb);
#endif

    if(!init_osi_api()) return false;

    return true;
}

void uninit_plugin(void *self) { }

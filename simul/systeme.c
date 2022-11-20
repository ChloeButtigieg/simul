
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "cpu.h"
#include "asm.h"
#include "systeme.h"


/**********************************************************
** Structures de données de l'ordonnanceur (représentation
** d'un ensemble de processus).
**
** SUIVEZ L'ORDRE DES QUESTIONS DU SUJET DE TP.
***********************************************************/

#define MAX_PROCESS  (20)   /* nb maximum de processus  */

typedef enum {
    EMPTY = 0,              /* processus non-prêt       */
    READY = 1,              /* processus prêt           */
    SLEEP = 2,
    GETCHAR =  3,
} STATE;                    /* État d'un processus      */

typedef struct {
    PSW    cpu;             /* mot d'état du processeur */
    STATE  state;           /* état du processus        */
    time_t waking;
} PCB;                      /* Un Process Control Block */

PCB process[MAX_PROCESS];   /* table des processus      */

int current_process = -1;   /* nu du processus courant  */

char tampon = '\0';


/**********************************************************
** Compter le nombre de processus dans un certain état.
***********************************************************/

int nb_process(STATE s) {
    int cpt = 0;
    for(int i=0; i<MAX_PROCESS; i++) {
        if (process[i].state == s) cpt++;
    }
    return cpt;
}


/**********************************************************
** Ajouter une entrée dans le tableau des processus.
**
** 1) trouver une case libre dans le tableau (EMPTY).
** 2) préparer cette case avec le PSW et l'état READY.
**
** SUIVEZ L'ORDRE DES QUESTIONS DU SUJET DE TP.
***********************************************************/

void new_thread(PSW cpu) {
    for (int index = 0; index < MAX_PROCESS; index++) {
        if (process[index].state == EMPTY) {
            process[index].cpu.PC = cpu.PC;
            process[index].cpu.SB = cpu.SB;
            process[index].cpu.SE = cpu.SE;
            process[index].cpu.RI = cpu.RI;
            process[index].state = READY;
            for (int DRIndex = 0; DRIndex < 8; DRIndex++) {
                process[current_process].cpu.DR[DRIndex] = cpu.DR[DRIndex];
            }
            break;
        }
    }
}


/**********************************************************
** Ordonnancer l'exécution des processus.
**
** SUIVEZ L'ORDRE DES QUESTIONS DU SUJET DE TP.
***********************************************************/

void wakeup(void) {
    time_t now = time(NULL);
    for (int index = 0; index < MAX_PROCESS; index++) {
        if ((process[index].state == SLEEP) && (process[index].waking <= now)) {
            process[index].state = READY;
        }
    }
}

void wakeup_getchar(int p, char c) {
    process[p].state = READY;
    process[p].cpu.DR[process[p].cpu.RI.i] = c;
}

PSW scheduler(PSW cpu) {
    if (current_process != -1) {
        process[current_process].cpu.PC = cpu.PC;
        process[current_process].cpu.SB = cpu.SB;
        process[current_process].cpu.SE = cpu.SE;
        process[current_process].cpu.RI = cpu.RI;
        for (int DRIndex = 0; DRIndex < 8; DRIndex++) {
            process[current_process].cpu.DR[DRIndex] = cpu.DR[DRIndex];
        }
    }
    do {
        if (current_process == MAX_PROCESS - 1) {
            wakeup();
        }
        current_process = (current_process + 1) % MAX_PROCESS;
    } while (process[current_process].state != READY);
    cpu.PC = process[current_process].cpu.PC;
    cpu.SB = process[current_process].cpu.SB;
    cpu.SE = process[current_process].cpu.SE;
    cpu.RI = process[current_process].cpu.RI;
    for (int DRIndex = 0; DRIndex < 8; DRIndex++) {
         cpu.DR[DRIndex] = process[current_process].cpu.DR[DRIndex];
    }
    return cpu;
}


/**********************************************************
** Démarrage du système (création d'un programme)
***********************************************************/

PSW prepare_idle(void) {
    PSW idle = { .PC = 120, .SB = 120, .SE = 125, };
    assemble(idle.SB, "idle.asm");
    return idle;
}

PSW prepare_prog1(void) {
    PSW prog1 = { .PC = 20, .SB = 20, .SE = 30, };
    assemble(prog1.SB, "prog1.asm");
    return prog1;
}

PSW prepare_prog2(void) {
    PSW prog2 = { .PC = 40, .SB = 40, .SE = 50, };
    assemble(prog2.SB, "prog2.asm");
    return prog2;
}

PSW prepare_sleep(void) {
    PSW sleep = { .PC = 60, .SB = 60, .SE = 70, };
    assemble(sleep.SB, "sleep.asm");
    return sleep;
}

PSW prepare_getchar(void) {
    PSW getchar = { .PC = 80, .SB = 80, .SE = 90, };
    assemble(getchar.SB, "getchar.asm");
    return getchar;
}

PSW create_cpu(void) {
    PSW cpu = {
        .PC = process[current_process].cpu.PC,
        .SB = process[current_process].cpu.SB,
        .SE = process[current_process].cpu.SE,
    };
    return cpu;
}

PSW system_init(void) {

    printf("Booting\n\n");

    process[0].cpu = prepare_idle();
    process[0].state = READY;
    process[1].cpu = prepare_getchar();
    process[1].state = READY;
    process[2].cpu = prepare_prog1();
    process[2].state = READY;

    current_process = 0;
    return create_cpu();
}

/**********************************************************
** Traitement des appels au système
**
** SUIVEZ L'ORDRE DES QUESTIONS DU SUJET DE TP.
***********************************************************/

enum {
    SYSC_EXIT = 100,   // fin du processus courant
    SYSC_PUTI = 200,   // afficher le contenu de Ri
    SYSC_NEW_THREAD = 300,
    SYSC_SLEEP = 400,
    SYSC_GETCHAR = 500,
};


static PSW sysc_exit(PSW cpu) {
    process[current_process].state = EMPTY;
    fprintf(stdout, "EXIT : End of process = %d\n\n", current_process);
    if (nb_process(EMPTY) == MAX_PROCESS) {
        fprintf(stdout, "END\n");
        exit(EXIT_FAILURE);
    }
    return scheduler(cpu);
}


static PSW sysc_puti(PSW cpu) {
    printf("PUT i : R%d = %d || R%d = %c\n\n", cpu.RI.i, cpu.DR[cpu.RI.i], cpu.RI.i, cpu.DR[cpu.RI.i]);
    return cpu;
}

static PSW sysc_new_thread(PSW cpu) {
    cpu.DR[cpu.RI.i] = 0;
    new_thread(cpu);
    cpu.DR[cpu.RI.i] = 1;
    return cpu;
}

static PSW sysc_sleep(PSW cpu) {
    process[current_process].state = SLEEP;
    process[current_process].waking = time(NULL) + cpu.DR[cpu.RI.i];
    return scheduler(cpu);
}

static PSW sysc_getchar(PSW cpu) {
    if (tampon == '\0') {
        process[current_process].state = GETCHAR;
        return scheduler(cpu);
    }
    cpu.DR[cpu.RI.i] = tampon;
    tampon = '\0';
    return cpu;
}

static void keyboard_event(void) {
    if (nb_process(GETCHAR)) {
        int p = -1;
        do {
            p = p + 1;
        } while(process[p].state != GETCHAR);
        wakeup_getchar(p, get_keyboard_data());
    } else {
        tampon = get_keyboard_data();
    }
}


static PSW process_system_call(PSW cpu) {
    // suivant l'argument de sysc Ri, Rj, ARG
    switch (cpu.RI.arg) {
        case SYSC_EXIT:
            cpu = sysc_exit(cpu);
            break;
        case SYSC_PUTI:
            cpu = sysc_puti(cpu);
            break;
        case SYSC_NEW_THREAD:
            cpu = sysc_new_thread(cpu);
            break;
        case SYSC_SLEEP:
            cpu = sysc_sleep(cpu);
            break;
        case SYSC_GETCHAR:
            cpu = sysc_getchar(cpu);
            break;
        default:
            printf("Appel système inconnu %d\n", cpu.RI.arg);
            break;
    }
    return cpu;
}


/**********************************************************
** Traitement des interruptions par le système (mode système)
***********************************************************/

PSW process_interrupt(PSW cpu) {
    switch (cpu.IN) {
        case INT_SEGV:
            break;
        case INT_INST:
            break;
        case INT_TRACE:
            fprintf(stdout, "Interrupt number = %d\n", cpu.IN);
            fprintf(stdout, "Process number = %d\n", current_process);
            dump_cpu(cpu);
            sleep(1);
            cpu = scheduler(cpu);
            break;
        case INT_SYSC:
            cpu = process_system_call(cpu);
            break;
        case INT_KEYBOARD:
            keyboard_event();
            break;
        default:
            break;
    }
    return cpu;
}

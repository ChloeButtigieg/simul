
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "cpu.h"


/* ============================================================================
 _   _ _____   ____   _    ____    __  __  ___  ____ ___ _____ ___ _____ ____  
| \ | | ____| |  _ \ / \  / ___|  |  \/  |/ _ \|  _ \_ _|  ___|_ _| ____|  _ \ 
|  \| |  _|   | |_) / _ \ \___ \  | |\/| | | | | | | | || |_   | ||  _| | |_) |
| |\  | |___  |  __/ ___ \ ___) | | |  | | |_| | |_| | ||  _|  | || |___|  _ < 
|_| \_|_____| |_| /_/   \_\____/  |_|  |_|\___/|____/___|_|   |___|_____|_| \_\
                                                                                                                                   
============================================================================ */

/**********************************************************
** définition de la mémoire physique simulée
***********************************************************/

#define MAX_MEM        (128)

#define IS_PHYSICAL_ADR(a)    ((0 <= (a)) && ((a) < MAX_MEM))
#define IS_LOGICAL_ADR(a,cpu) (((cpu).SB <= (a)) && ((a) <= (cpu).SE))

static WORD mem[MAX_MEM];     /* mémoire                       */


/**********************************************************
** initialiser la machine (notamment la mémoire)
***********************************************************/

static void init_cpu() {
    static int init_done = 0;
    if (init_done) return;
    
    // vérifier la taille des structures
    assert(sizeof(WORD) == sizeof(INST));
    
    for(int adr=0; (adr < MAX_MEM); adr++) {
        mem[adr] = -1;
    }
    init_done = 1;
}


/**********************************************************
** Lire ou écrire une case de la mémoire physique
***********************************************************/

WORD read_mem(int physical_address) {
    init_cpu();
    if (! IS_PHYSICAL_ADR(physical_address)) {
        fprintf(stderr, "ERROR: read_mem: bad address %d\n", physical_address);
        exit(EXIT_FAILURE);
    }
    return mem[physical_address];
}


void write_mem(int physical_address, WORD value) {
    init_cpu();
    if (! IS_PHYSICAL_ADR(physical_address)) {
        fprintf(stderr, "ERROR: write_mem: bad address %d\n", physical_address);
        exit(EXIT_FAILURE);
    }
    mem[physical_address] = value;
}


/**********************************************************
** Lire une case de la mémoire logique
***********************************************************/

static WORD read_logical_mem(int logical_address, PSW* cpu) {
    if (! IS_LOGICAL_ADR(logical_address, *cpu)) {
        cpu->IN = INT_SEGV;
        return (0);
    }
    return read_mem(logical_address);
}


/**********************************************************
** écrire une case de la mémoire logique
***********************************************************/

static void write_logical_mem(int logical_address, PSW* cpu, WORD value) {
    if (! IS_LOGICAL_ADR(logical_address, *cpu)) {
        cpu->IN = INT_SEGV;
        return;
    }
    write_mem(logical_address, value);
}


/**********************************************************
** Coder une instruction
***********************************************************/

WORD encode_instruction(INST instr) {
    union { WORD word; INST instr; } instr_or_word;
    instr_or_word.instr = instr;
    return (instr_or_word.word);
}


/**********************************************************
** Décoder une instruction
***********************************************************/

INST decode_instruction(WORD value) {
    union { WORD integer; INST instruction; } inst;
    inst.integer = value;
    return inst.instruction;
}


/**********************************************************
** Macros pour simplifier l'accès aux registres Ri et Rj
** de l'instruction courante.
***********************************************************/

#define Ri(m)      ((m).DR[ (m).RI.i ])
#define Rj(m)      ((m).DR[ (m).RI.j ])


/**********************************************************
** instruction d'addition
**
** ADD Ri, Rj, arg
**   | Ri := (Ri + Rj + arg)
**   | PC := (PC + 1)
***********************************************************/

static PSW cpu_ADD(PSW m) {
    Ri(m) = (Ri(m) + Rj(m) + m.RI.arg);
    m.PC += 1;
    return m;
}


/**********************************************************
** instruction de soustraction
**
** SUB Ri, Rj, arg
**   | Ri := (Ri - Rj - arg)
**   | PC := (PC + 1)
***********************************************************/

static PSW cpu_SUB(PSW m) {
    Ri(m) = (Ri(m) - Rj(m) - m.RI.arg);
    m.PC += 1;
    return m;
}


/**********************************************************
** affectation d'un registre
**
** SET Ri, k
**   | Ri := k
**   | PC := (PC + 1)
***********************************************************/

static PSW cpu_SET(PSW m) {
    Ri(m) = m.RI.arg;
    m.PC += 1;
    return m;
}


/**********************************************************
** ne rien faire (presque)
**
** NOP
**   | PC := (PC + 1)
***********************************************************/

static PSW cpu_NOP(PSW m) {
    m.PC += 1;
    return m;
}


/**********************************************************
** lire la mémoire dans un registre
**
** LOAD Ri, Rj, offset
**   | Ri := mem[ Rj + offset ]
**   | PC := (PC + 1)
***********************************************************/

static PSW cpu_LOAD(PSW m) {
    WORD logical_address = (Rj(m) + m.RI.arg);
    WORD value = read_logical_mem(logical_address, &m);

    // erreur de lecture en mémoire logique
    if (m.IN) return (m);

    // lecture réussie
    Ri(m) = value;
    m.PC += 1;
    return m;
}


/**********************************************************
** vider un registre en mémoire.
**
** STORE Ri, Rj, offset
**   | mem[ SB + Rj + offset ] := Ri
**   | PC := (PC + 1)
***********************************************************/

static PSW cpu_STORE(PSW m) {
    WORD logical_address = (Rj(m) + m.RI.arg);
    write_logical_mem(logical_address, &m, Ri(m));
    
    // erreur d'écriture en mémoire logique
    if (m.IN) return (m);

    // écriture réussie
    m.PC += 1;
    return m;
}


/**********************************************************
** saut inconditionnel.
**
** JUMP offset
**   | PC := offset
***********************************************************/

static PSW cpu_JUMP(PSW m) {
    m.PC = m.RI.arg;
    return m;
}


/**********************************************************
** saut si plus grand que.
**
** IFGT Ri, Rj, offset
**   | if (Ri > Rj) PC := offset
**   | else PC := (PC + 1)
***********************************************************/

static PSW cpu_IFGT(PSW m) {
    if (Ri(m) > Rj(m)) {
        m.PC = m.RI.arg;
    } else {
        m.PC += 1;
    }
    return m;
}


/**********************************************************
** saut si plus grand ou égale.
**
** IFGE Ri, Rj, offset
**   | if (Ri > Rj) PC := offset
**   | else PC := (PC + 1)
***********************************************************/

static PSW cpu_IFGE(PSW m) {
    if (Ri(m) >= Rj(m)) {
        m.PC = m.RI.arg;
    } else {
        m.PC += 1;
    }
    return m;
}


/**********************************************************
** saut si plus petit que.
**
** IFLT Ri, Rj, offset
**   | if (Ri < Rj) PC := offset
**   | else PC := (PC + 1)
***********************************************************/

static PSW cpu_IFLT(PSW m) {
    if (Ri(m) < Rj(m)) {
        m.PC = m.RI.arg;
    } else {
        m.PC += 1;
    }
    return m;
}


/**********************************************************
** saut si plus petit ou égale.
**
** IFLE Ri, Rj, offset
**   | if (Ri <= Rj) PC := offset
**   | else PC := (PC + 1)
***********************************************************/

static PSW cpu_IFLE(PSW m) {
    if (Ri(m) <= Rj(m)) {
        m.PC = m.RI.arg;
    } else {
        m.PC += 1;
    }
    return m;
}


/**********************************************************
** appel au système (interruption SYSC)
**
** SYSC Ri, Rj, arg
**   | PC := (PC + 1)
**   | <interruption cause SYSC>
***********************************************************/

static PSW cpu_SYSC(PSW m) {
    m.PC += 1;
    m.IN = INT_SYSC;
    return m;
}


/**********************************************************
** stopper la machine.
***********************************************************/

static PSW cpu_HALT(PSW m) {
    fprintf(stderr,"poweroff\n");
    exit(EXIT_SUCCESS);
    return (m);
}


/**********************************************************
** génération d'une interruption clavier toutes les
** trois secondes.
***********************************************************/

static char keyboard_data = '\0';

char get_keyboard_data() {
    char data = keyboard_data;
    keyboard_data = '\0';
    return data;
}


static PSW keyboard_event(PSW m) {
    static time_t next_kbd_event = 0;
    static char* data_sample = "Keyboard DATA.\n\n";
    static int kbd_data_index = 0;

    time_t now = time(NULL);
    if (next_kbd_event == 0) {
        next_kbd_event = (now + 5);
    } else if (now >= next_kbd_event) {
        m.IN = INT_KEYBOARD;
        next_kbd_event = (now + 3); // dans 3 secondes
        keyboard_data = data_sample[ kbd_data_index ];
        kbd_data_index = ((kbd_data_index + 1) % strlen(data_sample));
    }
    return m;
}


/**********************************************************
** Simulation de la CPU (mode utilisateur)
***********************************************************/

PSW cpu(PSW m) {

    init_cpu();

    /* pas d'interruption */
    m.IN = INT_NONE;

    m = keyboard_event(m);
    if (m.IN) return (m);
    
    /*** lecture et décodage de l'instruction ***/
    WORD value = read_logical_mem(m.PC, &m);
    if (m.IN) return (m);
    
    m.RI = decode_instruction(value);
    
    /*** exécution de l'instruction ***/
    switch (m.RI.op) {
    case INST_SET:
        m = cpu_SET(m);
        break;
    case INST_ADD:
        m = cpu_ADD(m);
        break;
    case INST_SUB:
        m = cpu_SUB(m);
        break;
    case INST_NOP:
        m = cpu_NOP(m);
        break;
    case INST_LOAD:
        m = cpu_LOAD(m);
        break;
    case INST_STORE:
        m = cpu_STORE(m);
        break;
    case INST_JUMP:
        m = cpu_JUMP(m);
        break;
    case INST_IFGT:
        m = cpu_IFGT(m);
        break;
    case INST_IFGE:
        m = cpu_IFGE(m);
        break;
    case INST_IFLT:
        m = cpu_IFLT(m);
        break;
    case INST_IFLE:
        m = cpu_IFLE(m);
        break;
    case INST_SYSC:
        m = cpu_SYSC(m);
        break;
    case INST_HALT:
        m = cpu_HALT(m);
        break;
    default:
        /*** interruption instruction inconnue ***/
        m.IN = INT_INST;
        return (m);
    }
    
    /*** arrêt si l'instruction a provoqué une interruption ***/
    if (m.IN) return m;

    /*** interruption après chaque instruction ***/
    m.IN = INT_TRACE;
    return m;
}


/**********************************************************
** afficher les registres de la CPU
***********************************************************/

void dump_cpu(PSW m) {
    static char* interrupts[] = {
        "NONE","SEGV","INST","TRACE","SYSC","KEYB"};
    static char* instructions[] = {
        "ADD","HALT","IFGT","IFGE","IFLT","IFLE",
        "JUMP","LOAD",
        "NOP","SET","STORE","SUB","SYSC"};

    fprintf(stdout, "PC = %6d | ", m.PC);
    if (m.IN < 6) {
        fprintf(stdout, "IN = %6s\n", interrupts[m.IN]);
    } else {
        fprintf(stdout, "IN = %6d\n", m.IN);
    }
    fprintf(stdout, "SB = %6d | SE = %6d\n", m.SB, m.SE);
    for(int i=0; (i<8); i+=2) {
        fprintf(stdout, "R%d = %6d | R%d = %6d\n", i, m.DR[i], i+1, m.DR[i+1]);
    }
    char* name = ((m.RI.op < 13) ? instructions[m.RI.op] : ("?"));
    fprintf(stdout, "RI  = %s R%d, R%d, %d \n", name, m.RI.i, m.RI.j, m.RI.arg);
    fprintf(stdout, "\n");
}

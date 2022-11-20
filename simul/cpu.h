
#ifndef __CPU_H
#define __CPU_H


/* ============================================================================
 _   _ _____   ____   _    ____    __  __  ___  ____ ___ _____ ___ _____ ____  
| \ | | ____| |  _ \ / \  / ___|  |  \/  |/ _ \|  _ \_ _|  ___|_ _| ____|  _ \ 
|  \| |  _|   | |_) / _ \ \___ \  | |\/| | | | | | | | || |_   | ||  _| | |_) |
| |\  | |___  |  __/ ___ \ ___) | | |  | | |_| | |_| | ||  _|  | || |___|  _ < 
|_| \_|_____| |_| /_/   \_\____/  |_|  |_|\___/|____/___|_|   |___|_____|_| \_\
                                                                                                                                   
============================================================================ */

/**********************************************************
** Codes associés aux interruptions
***********************************************************/

enum {
    INT_NONE = 0,   // pas d'interruption
    INT_SEGV,      // violation mémoire
    INT_INST,      // instruction inconnue
    INT_TRACE,    // trace entre chaque instruction
    INT_SYSC,      // appel au système
    INT_KEYBOARD,   // événement clavier (simulé)
};


/**********************************************************
** Codes associés aux instructions de la CPU
***********************************************************/

enum {
    INST_ADD,
    INST_HALT,
    INST_IFGT,
    INST_IFGE,
    INST_IFLT,
    INST_IFLE,
    INST_JUMP,
    INST_LOAD,
    INST_NOP,
    INST_SET,
    INST_STORE,
    INST_SUB,
    INST_SYSC,
};


/**********************************************************
** définition d'un mot mémoire
***********************************************************/

typedef int WORD;         /* un mot est un entier 32 bits  */

WORD read_mem(int physical_address);
void write_mem(int physical_address, WORD value);


/**********************************************************
** Codage d'une instruction (32 bits)
***********************************************************/

typedef struct {
    unsigned op: 10;  /* code operation (10 bits)  */
    unsigned i:   3;  /* nu 1er registre (3 bits)  */
    unsigned j:   3;  /* nu 2eme registre (3 bits) */
    short    arg;     /* argument (16 bits)        */
} INST;


/**********************************************************
** Le mot d'état du processeur (PSW)
***********************************************************/

typedef struct PSW { /* Processor Status Word */
    WORD PC;         /* Program Counter */
    WORD SB;         /* Segment begin */
    WORD SE;         /* Segment end */
    WORD IN;         /* Interrupt number */
    WORD DR[8];      /* Data Registers */
    INST RI;         /* Registre instruction */
} PSW;


/**********************************************************
** encoder/décoder une instruction XX Ri, Rj, arg
***********************************************************/

WORD encode_instruction(INST instruction);
INST decode_instruction(WORD value);


/**********************************************************
** afficher les registres de la CPU
***********************************************************/

void dump_cpu(PSW regs);


/**********************************************************
** exécuter un code en mode utilisateur
***********************************************************/

PSW cpu(PSW);


/**********************************************************
** récupérer le caractère arrivé depuis le clavier
***********************************************************/

char get_keyboard_data();


#endif

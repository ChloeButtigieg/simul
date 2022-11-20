
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "cpu.h"
#include "asm.h"


/* ============================================================================
 _   _ _____   ____   _    ____    __  __  ___  ____ ___ _____ ___ _____ ____  
| \ | | ____| |  _ \ / \  / ___|  |  \/  |/ _ \|  _ \_ _|  ___|_ _| ____|  _ \ 
|  \| |  _|   | |_) / _ \ \___ \  | |\/| | | | | | | | || |_   | ||  _| | |_) |
| |\  | |___  |  __/ ___ \ ___) | | |  | | |_| | |_| | ||  _|  | || |___|  _ < 
|_| \_|_____| |_| /_/   \_\____/  |_|  |_|\___/|____/___|_|   |___|_____|_| \_\
                                                                                                                                   
============================================================================ */

/**********************************************************
** fonctions utilitaires de création d'un programme simple
** directement en mémoire.
***********************************************************/

static WORD logical_adr = 0;  /* adresse logique        */


/**********************************************************
** Stockage des labels utilisés
***********************************************************/

#define MAX_LABELS          (30)
#define MAX_LABEL_LENGTH    (60)

static int nb_labels = 0;

static struct {
    char name[MAX_LABEL_LENGTH];
    int adr;
} labels[ MAX_LABELS ];

static int nb_used_labels = 0;

static struct USED_LABEL {
    int label;
    int adr;
} used_labels[ MAX_LABELS ];

static int nu_ligne = 0;


/**********************************************************
** implanter une instruction en mémoire.
***********************************************************/

static void make_inst(int adr, INST inst) {
    WORD w = encode_instruction(inst);
    write_mem(adr, w);
}

typedef struct ASM_INST {
    char *name;
    unsigned code;
} ASM_INST;


typedef enum TOKEN_TYPE {
    TOK_NUL, TOK_REGISTER, TOK_INST, TOK_VALUE
} TOKEN_TYPE;

typedef struct TOKEN {
    TOKEN_TYPE type;
    int        value;
    int        is_ident;
} TOKEN;


static ASM_INST instructions_dico[] = {
    {name:"set",    code:INST_SET},
    {name:"add",    code:INST_ADD},
    {name:"sub",    code:INST_SUB},
    {name:"nop",    code:INST_NOP},
    {name:"jump",   code:INST_JUMP},
    {name:"ifgt",   code:INST_IFGT},
    {name:"ifge",   code:INST_IFGE},
    {name:"iflt",   code:INST_IFLT},
    {name:"ifle",   code:INST_IFLE},
    {name:"sysc",   code:INST_SYSC},
    {name:"store",  code:INST_STORE},
    {name:"load",   code:INST_LOAD},
    {name:"halt",   code:INST_HALT},
    {name:"data",   code:-1},
    {name:"define", code:-2},
    {name:NULL,  code:0},
};


static ASM_INST registers_dico[] = {
    {name:"R0", code:0},
    {name:"R1", code:1},
    {name:"R2", code:2},
    {name:"R3", code:3},
    {name:"R4", code:4},
    {name:"R5", code:5},
    {name:"R6", code:6},
    {name:"R7", code:7},
    {name:"r0", code:0},
    {name:"r1", code:1},
    {name:"r2", code:2},
    {name:"r3", code:3},
    {name:"r4", code:4},
    {name:"r5", code:5},
    {name:"r6", code:6},
    {name:"r7", code:7},
    {name:NULL, code:0},
};


static int search(char* name, ASM_INST dico[]) {
    for(int i=0; (dico[i].name != NULL); i++) {
        if (strcoll(name, dico[i].name) == 0) {
            return i;
        }
    }
    return -1;
}


static void err_asm(char* err) {
    fprintf(stderr, "Ligne %d: %s\n", nu_ligne, err);
    exit(EXIT_FAILURE);
}


static TOKEN tokenize(char* data) {
    char *s = strtok (data, " ,:\t\n");
    if (s == NULL) {
        TOKEN null = {type:TOK_NUL};
        return null;
    }
    if ((s[0] >= '0') && (s[0] <= '9')) {
        TOKEN value = {type:TOK_VALUE, atoi(s)};
        return value;
    }
    if ((s[0] == '/') && (s[1] == '/')) {
        TOKEN null = {type:TOK_NUL};
        return null;
    }
    int i = search(s, registers_dico);
    if (i >= 0) {
        TOKEN reg = {type:TOK_REGISTER, registers_dico[i].code};
        return reg;
    }
    i = search(s, instructions_dico);
    if (i >= 0) {
        TOKEN inst = {type:TOK_INST, instructions_dico[i].code};
        return inst;
    }

    for(i=0; i<nb_labels; i++) {
        if (strcoll(labels[i].name, s) == 0) {
            break;
        }
    }
    
    if (i == nb_labels) {
        assert(nb_labels < MAX_LABELS);
        assert(strlen(s) < MAX_LABEL_LENGTH);
        strcpy(labels[ nb_labels ].name, s);
        labels[ nb_labels ].adr = -1;
        nb_labels++;
    }

    assert(nb_used_labels < MAX_LABELS);
    struct USED_LABEL used = {label:i, adr:logical_adr};
    used_labels[ nb_used_labels++ ] = used;
    TOKEN reg = {type:TOK_VALUE, value:i, is_ident:1};
    return reg;
}


static void assemble_define() {
    // define LABEL VALUE
    TOKEN t = tokenize (NULL);
    if (! (t.type == TOK_VALUE && t.is_ident == 1)) {
        err_asm("instruction define incorrecte");
        return;
    }
    nb_used_labels --;
    int label = t.value;
    t = tokenize(NULL);
    if (! (t.type == TOK_VALUE && t.is_ident == 0)) {
        err_asm("instruction define incorrecte");
        return;
    }
    labels[ label ].adr = t.value;
}


static void assemble_line(char *line) {
    INST inst = {op:0, i:0, j:0, arg:0};
    short value = 0;
    
    nu_ligne++;
    TOKEN t = tokenize (line);
    // pas d'instruction
    if (t.type == TOK_NUL) return;
    // traitement d'un label
    if (t.type == TOK_VALUE  &&  t.is_ident) {
        if (labels[ t.value ].adr >= 0) {
            fprintf(stdout, "label déjà utilisé : %s\n",
                labels[ t.value ].name);
            exit(EXIT_FAILURE);
        }
        nb_used_labels --;
        labels[ t.value ].adr = logical_adr;
        t = tokenize(NULL);
    }
    // pas d'instruction
    if (t.type == TOK_NUL) {
        return;
    }
    if (t.type != TOK_INST) {
        err_asm("instruction incorrecte");
    }
    // instruction define
    if (t.type == TOK_INST && t.value == -2) {
        assemble_define();
        return;
        
    }
    inst.op = t.value;
    t = tokenize(NULL);
    // instruction de la forme : OP
    if (t.type == TOK_NUL) {
        make_inst(logical_adr, inst);
        logical_adr++;
        return;
    }
    if (t.type == TOK_REGISTER) {
        inst.i = t.value;
        t = tokenize(NULL);
    }
    if (t.type == TOK_REGISTER) {
        inst.j = t.value;
        t = tokenize(NULL);
    }
    if (t.type == TOK_VALUE) {
        inst.arg = t.value;
        t = tokenize(NULL);
    }
    if (t.type != TOK_NUL) {
        err_asm("instruction incorrecte");
    }
    
    if (inst.op == -1) {
        write_mem(logical_adr, value);
    } else {
        make_inst(logical_adr, inst);
    }
    logical_adr++;
}


static void test_tokenize() {
    char line[100] = "  set R2, 1234, hello  // Comment ";
    
    nb_labels = 0;
    nb_used_labels = 0;

    TOKEN t = tokenize(line);
    assert(t.type == TOK_INST);
    assert(t.value == INST_SET);
    
    t = tokenize(NULL);
    assert(t.type == TOK_REGISTER);
    assert(t.value == 2);
    
    t = tokenize(NULL);
    assert(t.type == TOK_VALUE);
    assert(t.value == 1234);
    
    t = tokenize(NULL);
    assert(t.type == TOK_VALUE);
    assert(t.value == 0);
    assert(strcmp(labels[0].name, "hello") == 0);
    
    t = tokenize(NULL);
    assert(t.type == TOK_NUL);
}


/**********************************************************
** Début du codage en mémoire (initialisation).
***********************************************************/

static void begin(int begin_logical_adr) {
    logical_adr = begin_logical_adr;
    for(int i=0; (i<MAX_LABELS); i++) {
        strcpy(labels[i].name, "");
        labels[i].adr = -1;
        used_labels[i].label = -1;
        used_labels[i].adr = -1;
    }
    nb_labels = 0;
    nb_used_labels = 0;
    nu_ligne = 0;
}


/**********************************************************
** Fin du codage en mémoire (résolution des labels).
***********************************************************/

static void end() {
    for(int i=0; (i<nb_labels); i++) {
        if (labels[ i ].adr < 0) {
            fprintf(stderr, "ERROR: label %s inconnu\n", labels[i].name);
            exit(EXIT_FAILURE);
        }
    }
    for(int i=0; (i<nb_used_labels); i++) {
        struct USED_LABEL used = used_labels[i];
        int label = used.label;
        INST inst = decode_instruction(read_mem(used.adr));
        inst.arg = labels[ label ].adr;
        make_inst(used.adr, inst);
    }
}


/**********************************************************
** Assembler un fichier
***********************************************************/

void assemble(WORD adr, char *file_name) {
    char buffer[300];
    
    test_tokenize();
    nb_labels = 0;
    nb_used_labels = 0;

    FILE *file = fopen(file_name,"r");
    if (file == NULL) {
        fprintf(stderr, "Fichier %s illisible\n", file_name);
        exit(EXIT_FAILURE);
    }
    
    begin(adr);
    while (! feof(file)) {
        buffer[0] = '\0';
        fgets(buffer, 300, file);
        if (ferror(file)) {
            fprintf(stderr, "Erreur de lecture sur %s\n", file_name);
            exit(EXIT_FAILURE);
        }
        assemble_line(buffer);
    }
    end();
}


/**********************************************************
** Assembler une chaine.
***********************************************************/

void assemble_string(WORD adr, char *program) {
    
    char* copy = malloc(strlen(program) + 1);
    assert(copy != NULL);
    strcpy(copy, program);
    
    begin(adr);
    int fin = (*copy == '\0');
    while (! fin) {
        char *debut = copy;
        while ((*copy != '\0') && (*copy != '\n')) copy++;
        fin = (*copy == '\0');
        *copy = '\0';
        assemble_line(debut);
        copy++;
    }
    end();
}

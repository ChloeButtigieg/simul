define SYSC_EXIT          100
define SYSC_PUTI          200
define SYSC_NEW_THREAD    300
define SYSC_SLEEP         400

set R3, 4               // R3 = 4
sysc R3, SYSC_SLEEP     // endormir R3 sec.
sysc R3, SYSC_PUTI      // afficher R3
sysc R3, SYSC_SLEEP     // endormir R3 sec.
sysc R3, SYSC_PUTI      // afficher R3
sysc SYSC_EXIT          // fin du processus
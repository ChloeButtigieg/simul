    define SYSC_EXIT 100
    define SYSC_PUTI 200
    define SYSC_NEW_THREAD 300

    sysc R1, SYSC_NEW_THREAD    // créer un thread
    set R3, 0                   // R3 = 0
    ifgt R1, R3, pere           // si (R1 > R3), aller à pere
    set R3, 1000                // R3 = 1000
    sysc R3, SYSC_PUTI          // afficher R3
    nop
pere: // *** code du père ***
    set R3, 2000                // R3 = 2000
    sysc R3, SYSC_PUTI          // afficher R3
    sysc SYSC_EXIT              // fin du thread
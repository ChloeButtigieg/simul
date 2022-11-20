        define SYSC_EXIT          100
        define SYSC_PUTI          200
        define SYSC_NEW_THREAD    300
        define SYSC_SLEEP         400
        define SYSC_GETCHAR       500

        set R3, 1               // R3 = 1
loop:   sysc R4, SYSC_GETCHAR   // R4 = getchar()
        sysc R4, SYSC_PUTI      // puti(R4)
        sysc R3, SYSC_SLEEP     // sleep(R3)
        jump loop               // aller à loop
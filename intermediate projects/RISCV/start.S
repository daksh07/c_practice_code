# Startup code: the RISC-V equivalent of the Cortex-M reset handler.
# There is no vector table to read here. The hart simply starts
# executing at the first instruction in RAM (0x80000000).

    .section .text.init
    .global _start

_start:
    # 1. Set up the stack pointer (on Cortex-M the hardware loads this
    #    from the first vector table entry; here we do it ourselves)
    la      sp, _stack_top

    # 2. Zero the .bss section
    la      t0, __bss_start
    la      t1, __bss_end
1:  bgeu    t0, t1, 2f
    sw      zero, 0(t0)
    addi    t0, t0, 4
    j       1b

    # 3. Hand over to C
2:  call    main

    # 4. If main ever returns, park the core
3:  wfi
    j       3b

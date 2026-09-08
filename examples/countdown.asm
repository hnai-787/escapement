; Demonstrates real programmability (branch + loop), not just calculator dispatch.
    LDI R1, 5
LOOP:
    OUT R1
    LDI R2, 1
    SUB R1, R2
    BNZ LOOP
    HALT

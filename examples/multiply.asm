; Reproduces the original project's documented MUL test case: 6 * 7 = 42
    LDI R1, 6
    LDI R2, 7
    MOV RES, R1
    MUL RES, R2
    OUT RES
    HALT

; Reproduces the original project's documented ADD test case: 5 + 3 = 8
    LDI R1, 5
    LDI R2, 3
    MOV RES, R1
    ADD RES, R2
    OUT RES
    HALT

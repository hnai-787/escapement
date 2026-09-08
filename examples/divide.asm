; Reproduces the original project's documented DIV test case: 20 / 4 = 5
    LDI R1, 20
    LDI R2, 4
    MOV RES, R1
    DIV RES, R2
    OUT RES
    HALT

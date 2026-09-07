; *****************************************************************************
; Program Name: Embedded Calculator Processor Simulation
; Author: Hussain Ali
; Description:
; This program simulates a basic embedded processor capable of performing
; arithmetic (ADD, SUB, MUL, DIV) and logical (AND, OR) operations. The user 
; inputs two numbers and selects an operation using an opcode. The program 
; calculates and displays the result. It handles division by zero and invalid 
; opcodes gracefully.
; *****************************************************************************

.386                    ; Specifies the target processor (80386 or later).
.model flat, stdcall    ; Defines flat memory model and standard calling convention.
.stack 4096             ; Reserves 4 KB of stack space.
include Irvine32.inc    ; Includes Irvine32 library for basic I/O functions.

.data
welcomeMsg db "Embedded Calculator Processor Simulation", 0
                        ; Welcome message displayed at the start of the program.
menu db "Supported Operations: ADD(1), SUB(2), MUL(3), DIV(4), AND(5), OR(6), EXIT(99)", 0
                        ; Menu displaying available operations and corresponding opcodes.
firstNumPrompt db "Enter the first number: ", 0
                        ; Prompt to enter the first number.
secondNumPrompt db "Enter the second number: ", 0
                        ; Prompt to enter the second number.
opcodePrompt db "Enter the operation code (1-ADD, 2-SUB, 3-MUL, 4-DIV, 5-AND, 6-OR, 99-EXIT): ", 0
                        ; Prompt to enter the operation code.
resultMsg db "Result: ", 0
                        ; Message prefix for displaying the result.
divZeroError db "Error: Division by zero.", 0
                        ; Error message for division by zero.
invalidOpcode db "Error: Invalid opcode.", 0
                        ; Error message for invalid operation codes.
exitMsg db "Processor simulation terminated. Goodbye!", 0
                        ; Exit message displayed when the program ends.

num1 dd 0               ; Variable to store the first number (32-bit integer).
num2 dd 0               ; Variable to store the second number (32-bit integer).
opcode db 0             ; Variable to store the operation code (8-bit integer).
result dd 0             ; Variable to store the result of the operation (32-bit integer).

.code
main PROC
start:
    ; Display Welcome Message
    mov edx, OFFSET welcomeMsg  ; Load the address of the welcome message into EDX.
    call WriteString            ; Print the welcome message.
    call Crlf                   ; Print a newline.

processorLoop:
    ; Display Menu
    mov edx, OFFSET menu        ; Load the address of the menu into EDX.
    call WriteString            ; Print the menu.
    call Crlf                   ; Print a newline.

    ; Read First Number
    mov edx, OFFSET firstNumPrompt ; Load the address of the first number prompt.
    call WriteString               ; Print the first number prompt.
    call ReadInt                   ; Read the first number from the user.
    mov num1, eax                  ; Store the input in num1.

    ; Read Second Number
    mov edx, OFFSET secondNumPrompt ; Load the address of the second number prompt.
    call WriteString                ; Print the second number prompt.
    call ReadInt                    ; Read the second number from the user.
    mov num2, eax                   ; Store the input in num2.

    ; Read Opcode
    mov edx, OFFSET opcodePrompt ; Load the address of the opcode prompt.
    call WriteString             ; Print the opcode prompt.
    call ReadInt                 ; Read the opcode from the user.
    mov opcode, al               ; Store the input in opcode.

    ; Process Operation
    cmp opcode, 1               ; Compare opcode with 1 (ADD).
    JE ADD_OPERATION            ; If equal, jump to ADD_OPERATION.
    cmp opcode, 2               ; Compare opcode with 2 (SUB).
    JE SUB_OPERATION            ; If equal, jump to SUB_OPERATION.
    cmp opcode, 3               ; Compare opcode with 3 (MUL).
    JE MUL_OPERATION            ; If equal, jump to MUL_OPERATION.
    cmp opcode, 4               ; Compare opcode with 4 (DIV).
    JE DIV_OPERATION            ; If equal, jump to DIV_OPERATION.
    cmp opcode, 5               ; Compare opcode with 5 (AND).
    JE AND_OPERATION            ; If equal, jump to AND_OPERATION.
    cmp opcode, 6               ; Compare opcode with 6 (OR).
    JE OR_OPERATION             ; If equal, jump to OR_OPERATION.
    cmp opcode, 99              ; Compare opcode with 99 (EXIT).
    JE EXIT_PROCESSOR           ; If equal, jump to EXIT_PROCESSOR.

    ; Invalid Opcode Handler
    mov edx, OFFSET invalidOpcode ; Load the address of the invalid opcode message.
    call WriteString              ; Print the error message.
    call Crlf                     ; Print a newline.
    JMP processorLoop             ; Restart the loop.

ADD_OPERATION:
    mov eax, num1               ; Load num1 into EAX.
    add eax, num2               ; Add num2 to EAX.
    mov result, eax             ; Store the result in the result variable.
    JMP DISPLAY_RESULT          ; Jump to DISPLAY_RESULT.

SUB_OPERATION:
    mov eax, num1               ; Load num1 into EAX.
    sub eax, num2               ; Subtract num2 from EAX.
    mov result, eax             ; Store the result in the result variable.
    JMP DISPLAY_RESULT          ; Jump to DISPLAY_RESULT.

MUL_OPERATION:
    mov eax, num1               ; Load num1 into EAX.
    imul num2                   ; Multiply EAX by num2.
    mov result, eax             ; Store the result in the result variable.
    JMP DISPLAY_RESULT          ; Jump to DISPLAY_RESULT.

DIV_OPERATION:
    cmp num2, 0                 ; Check if num2 is zero.
    JE DIV_ZERO_ERROR           ; If zero, jump to DIV_ZERO_ERROR.
    mov eax, num1               ; Load num1 into EAX.
    cdq                         ; Sign-extend EAX into EDX:EAX.
    idiv num2                   ; Divide EDX:EAX by num2.
    mov result, eax             ; Store the quotient in the result variable.
    JMP DISPLAY_RESULT          ; Jump to DISPLAY_RESULT.

DIV_ZERO_ERROR:
    mov edx, OFFSET divZeroError ; Load the address of the division error message.
    call WriteString             ; Print the error message.
    call Crlf                    ; Print a newline.
    JMP processorLoop            ; Restart the loop.

AND_OPERATION:
    mov eax, num1               ; Load num1 into EAX.
    and eax, num2               ; Perform a bitwise AND with num2.
    mov result, eax             ; Store the result in the result variable.
    JMP DISPLAY_RESULT          ; Jump to DISPLAY_RESULT.

OR_OPERATION:
    mov eax, num1               ; Load num1 into EAX.
    or eax, num2                ; Perform a bitwise OR with num2.
    mov result, eax             ; Store the result in the result variable.
    JMP DISPLAY_RESULT          ; Jump to DISPLAY_RESULT.

DISPLAY_RESULT:
    mov edx, OFFSET resultMsg   ; Load the address of the result message.
    call WriteString            ; Print the result message.
    call PrintSignedInt         ; Call custom procedure to print the result.
    call Crlf                   ; Print a newline.
    JMP processorLoop           ; Restart the loop.

EXIT_PROCESSOR:
    mov edx, OFFSET exitMsg     ; Load the address of the exit message.
    call WriteString            ; Print the exit message.
    call Crlf                   ; Print a newline.
    exit                        ; Terminate the program.

; Custom Procedure to Print Signed Integer Without + Sign
PrintSignedInt PROC
    mov eax, result             ; Load the result into EAX.
    cmp eax, 0                  ; Check if the result is non-negative.
    JGE skipPlusSign            ; If non-negative, skip the plus sign.
    call WriteInt               ; Print the signed integer (includes '-').
    ret                         ; Return from the procedure.
skipPlusSign:
    mov eax, result             ; Reload the result into EAX.
    call WriteDec               ; Print the unsigned integer (no '+').
    ret                         ; Return from the procedure.
PrintSignedInt ENDP

main ENDP
END main

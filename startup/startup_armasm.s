; STM32F103ZE vector + reset (ARMASM)
Stack_Size      EQU     0x00000400
Heap_Size       EQU     0x00000200

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
                EXPORT  __initial_sp
__initial_sp

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
                EXPORT  __heap_base
__heap_base
Heap_Mem        SPACE   Heap_Size
                EXPORT  __heap_limit
__heap_limit

                PRESERVE8
                THUMB

                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp
                DCD     Reset_Handler
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
                DCD     0
__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

                AREA    |.text|, CODE, READONLY

                EXPORT  Reset_Handler
                IMPORT  __main

Reset_Handler   PROC
                LDR     R0, =__main
                BX      R0
                ENDP

                ALIGN
                END

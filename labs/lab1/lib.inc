section .text

; Принимает код возврата и завершает текущий процесс
exit:
    mov rax, 60
    syscall

; Принимает указатель на нуль-терминированную строку, возвращает её длину
string_length:
    xor rax, rax
.count:
    cmp byte [rdi + rax], 0
    je .return
    inc rax
    jmp .count
.return:
    ret

; Принимает указатель на нуль-терминированную строку, выводит её в stdout
print_string:
    push rdi
    call string_length
    pop rdi
    mov rdx, rax
    mov rsi, rdi
    mov rdi, 1
    mov rax, 1
    syscall
    ret

; Принимает код символа и выводит его в stdout
print_char:
    push rdi
    mov rsi, rsp
    mov rax, 1
    mov rdx, 1
    mov rdi, 1
    syscall
    pop rdi
    ret

; Переводит строку (выводит символ с кодом 0xA)
print_newline:
    mov rdi, 0xA       
    jmp print_char  

; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
print_uint:
    mov r11, rsp
    mov rax, rdi
    xor rdx, rdx
    dec rsp
    mov rcx, 10
.loop:
    div rcx
    add dl, '0'
    dec rsp
    mov byte [rsp], dl
    xor rdx, rdx
    cmp rax, 0
    jnz .loop
    mov rdi, rsp
    push r11
    call print_string
    pop rsp
    ret

; Выводит знаковое 8-байтовое число в десятичном формате 
print_int:
    cmp rdi, 0
    jge .positive
    neg rdi
    push rdi
    mov rdi, '-'
    call print_char
    pop rdi
.positive:
    jmp print_uint

; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
string_equals:
.loop:
    mov r11b, byte [rsi]
    cmp byte [rdi], r11b
    jne .neq
    test r11b, r11b
    jz .eq
    inc rsi
    inc rdi
    jmp .loop
.neq:
    mov rax, 0
    ret
.eq:
    mov rax, 1
    ret

; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
read_char:
    mov rax, 0
    mov rdi, 0
    push 0
    mov rsi, rsp
    mov rdx, 1
    syscall
    pop rax
    ret

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор
read_word:
    xor rcx, rcx
.loop:
    cmp rcx, rsi
    jge .fail
    push rdi
    push rsi
    push rcx
    call read_char
    pop rcx
    pop rsi
    pop rdi
    cmp al, 0x20
    je .skip_space
    cmp al, 0x9
    je .skip_space
    cmp al, 0xA
    je .skip_space
    mov [rdi + rcx], al
    cmp al, 0
    je .fine
    inc rcx
    jmp .loop
.fine:
    mov rax, rdi     
    mov rdx, rcx      
    ret
.fail:
    xor rax, rax      
    ret
.skip_space:
    test rcx, rcx       
    jz .loop           
    jmp .fine

; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
parse_uint:
    push rbx
    xor rdx, rdx        
    xor rax, rax        
    xor rbx, rbx        
.loop:
    mov bl, byte [rdi + rdx]
    sub bl, '0'         
    jl .return         
    cmp bl, 9
    jg .return            
    push rdx           
    mov rdx, 10          
    mul rdx             
    pop rdx           
    add rax, rbx          
    inc rdx             
    jmp .loop           
.return:
    pop rbx           
    ret

; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
parse_int:
    cmp byte [rdi], '-'     
    je .negative          
    jmp parse_uint        
.negative:
    inc rdi         
    call parse_uint 
    cmp rdx, 0     
    je .fail      
    inc rdx        
    neg rax        
    ret
.fail:
    mov rax, 0        
    ret

; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
string_copy:
    xor rax, rax   
.loop:
    cmp rax, rdx    
    jge .fail       
    mov r11b, byte [rdi + rax] 
    mov byte [rsi + rax], r11b
    test r11b, r11b     
    jz .success  
    inc rax   
    jmp .loop     
.fail:
    xor rax, rax 
.success:
    ret
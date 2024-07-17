.section .text
.global scheme_entry
scheme_entry:
  push %rbp
  mov %rsp, %rbp
  sub $8, %rsp

  movl $1, -4(%rbp)
  movl $2, -8(%rbp)

  movl -4(%rbp), %eax
  movl -8(%rbp), %ebx
  add %ebx, %eax

  mov %rbp, %rsp
  pop %rbp
  ret

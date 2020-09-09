        jl      .LBB0_2
        callq   bar(int*)
        movq    (%rbx), %rax
        movl    $42, (%rax)
.LBB0_2:
        movq    %rbx, %rdi

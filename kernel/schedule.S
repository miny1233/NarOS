.global context_switch


# 切换上下文
context_switch:
    movl 4(%esp),%eax   # 当前任务task_t
    movl %ebp,(%eax)    # 保存当前堆栈
    movl %esp,4(%eax)

    movl %cr3,%ebx      # 保存CR3
    movl %ebx,8(%eax)

    movl 8(%esp),%eax  # 读取下一个任务的task_t
    movl (%eax),%ebp    # 切换堆栈
    movl 4(%eax),%esp

    movl 8(%eax),%ebx
    movl %ebx,%cr3

    ret
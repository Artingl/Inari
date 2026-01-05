// void cpu_usage_task()
// {
//     while (1)
//     {
//         usleep(5000000);
//         for (size_t i = 0; i < 10000; i++)
//         {
//             struct thread *task;
//             if (sched_get_thread(i, &task) == 0)
//             {
//                 if (task->reschedules_count > 0 && task->cpu_time > 0)
//                     printk("cpu_usage: tid %lu time %lu count %lu", i,
//                         task->cpu_time, task->reschedules_count);
//             }
//         }
//     }
// }

#include <stdint.h>

int __main()
{
    char *s = "hello world from init!";
    __asm__ volatile("mov %0, %%eax" ::"r"((uint32_t)s));
    __asm__ volatile("int $101");

    return 12;
}

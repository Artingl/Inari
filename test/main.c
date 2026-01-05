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

#include <typedefs.h>
#include <sys.h>

int main()
{
    // __print("Hello world from init!");
    char *s = "Hello world from init!";

    syscall(23, (uint32_t)s, 0x00, 0x12, 0x07, 0x93);

    return 0;
}

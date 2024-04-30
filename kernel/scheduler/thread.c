#include <kernel/kernel.h>
#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/thread.h>

int thread_init(struct thread *thread, void *data, thread_entry_t entry)
{
    if (thread == NULL)
        return -1;
    thread->data = data;
    thread->entry = entry;
    thread->tid = -1;
    spinlock_init(&thread->lock);
    return 0;
}

int thread_start(struct thread *thread)
{
    if (thread == NULL)
        return -1;
    spinlock_acquire(&thread->lock);
    if (thread->state == THREAD_SLEEPING)
    {
        // restart the thread because it is asleep and already initialized in the scheduler
        scheduler_restart(thread->tid);
    }
    else {
        // the thread was not ever active, so we need to add it to the scheduler and run
        thread->tid = scheduler_append(thread);
        thread->state = THREAD_ACTIVE;
    }
    spinlock_release(&thread->lock);
    return 0;
}

int thread_kill(struct thread *thread, int code)
{
    if (thread == NULL)
        return -1;
    spinlock_acquire(&thread->lock);
    scheduler_kill(thread->tid);
    thread->tid = -1;
    thread->state = THREAD_INACTIVE;
    spinlock_release(&thread->lock);
    return 0;
}

thread_t *thread_self()
{
    return scheduler_current()->thread;
}

__attribute__((optimize("O0"))) void thread_entrypoint()
{
    thread_t *th = thread_self();

    // call the thread's entry, and then mark as sleeping
    th->entry(th, th->data);
    th->state = THREAD_SLEEPING;

    // enter infinite loop, so no mess code would try to execute while the thread is asleep
    for (;;) {}
}

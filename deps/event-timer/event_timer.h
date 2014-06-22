#ifndef EVENT_TIMER_H
#define EVENT_TIMER_H

void *eventtimer_new();

/**
 * @param nsecs run this event in this many seconds
 * @param udata use this data for the event function
 * @param func run this event callback */
void eventtimer_schedule_event(
    void *ti,
    int nsecs,
    void *udata,
    void (*cb_func) (void *)
);

/**
 * Poll any events that need to be triggered */
void eventtimer_step(void *ti);

#endif /* EVENT_TIMER_H */

#include "thread.h"

void *Thread::runner(void *data) {
    Thread *self = (Thread*)data;
    self->execute();
    return NULL;
}

Thread::Thread() {}

void Thread::start() {
    pthread_create(&thread, NULL, Thread::runner, this);
}

void Thread::join() {
    pthread_join(thread, NULL);
}

#ifndef THREAD_H
#define THREAD_H

#include <pthread.h>

class Expression;

class Thread {
    private:
        pthread_t thread;
 
        static void *runner(void *data);

    public:
        Thread();

        void start();

        void join();

        virtual void execute() = 0;
        virtual ~Thread(){}
    
    private:
        Thread(const Thread&);
        Thread& operator=(const Thread&);
};

#endif

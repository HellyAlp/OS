//
// Created by yanivpe on 30/05/2021.
//


//
// Created by yanivpe on 24/05/2021.
//


#include "uthreads.h"
#include <deque>
#include <map>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>


int isMutexBlocked;

typedef struct thread{
    bool isBlocked;
    bool mutexBlocked;
    int tid;
    void(*f)(void);
    int quantomCount;
    char stack[STACK_SIZE];
    sigjmp_buf env;
} thread;



#define FAIL -1
#define SUCCESS 0
#define UNIT 1000000
std::deque<thread*> readyQueue;
std::deque<thread*> mutexBlockedQueue;


std::map<int,thread*> threads;
std::map<int, thread*> blockedThread;

thread* runningThread;
int quantom;
int totalQuantomCount;

#ifdef __x86_64__
/* code for 64 bit Intel arch */

typedef unsigned long address_t;
#define JB_SP 6
#define JB_PC 7

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%fs:0x30,%0\n"
                 "rol    $0x11,%0\n"
    : "=g" (ret)
    : "0" (addr));
    return ret;
}

#else
/* code for 32 bit Intel arch */

typedef unsigned int address_t;
#define JB_SP 4
#define JB_PC 5

/* A translation is required when using an address of a variable.
   Use this as a black box in your code. */
address_t translate_address(address_t addr)
{
    address_t ret;
    asm volatile("xor    %%gs:0x18,%0\n"
		"rol    $0x9,%0\n"
                 : "=g" (ret)
                 : "0" (addr));
    return ret;
}

#endif


/**
 * A method that moves the top thread from the ready queue to be the running thread
 */
void scheduler()
{
    if (readyQueue.empty())
    {
        totalQuantomCount++;
        runningThread->quantomCount++;
        return;
    }
    thread* temp=runningThread;
    int return_val = sigsetjmp(temp->env,1);
    if(return_val == 1){
        return;
    }
    runningThread=readyQueue.front();
    readyQueue.pop_front();
    readyQueue.push_back(temp);
    runningThread->quantomCount++;
    totalQuantomCount++;
    siglongjmp(runningThread->env,1);
}

/**
 * moves the next ready thread to the running thread
 */
void ready_to_running(){
    runningThread=readyQueue.front();
    readyQueue.pop_front();
    runningThread->quantomCount++;
    totalQuantomCount++;
    siglongjmp(runningThread->env,1);
}
/**
 * Description: This function creates a new thread, whose entry point is the
 * function f with the signature void f(void). The thread is added to the end
 * of the READY threads list. The uthread_spawn function should fail if it
 * would cause the number of concurrent threads to exceed the limit
 * (MAX_THREAD_NUM). Each thread should be allocated with a stack of size
 * STACK_SIZE bytes.
 * Return value: On success, return the ID of the created thread.
 * On failure, return -1.
*/
int uthread_spawn(void (*f)(void)) {
    address_t sp, pc;
    thread* t=new thread();
    for (int i=0;i<MAX_THREAD_NUM; i++)
    {
        if (threads.find(i) == threads.end())
        {
            // init thread
            t->tid=i;
            t->f=f;
            t->isBlocked=false;
            t->mutexBlocked = false;
            threads[t->tid]=t;
            // create env:
            sp = (address_t)(t->stack) + STACK_SIZE - sizeof(address_t);
            pc = (address_t)t->f;
            int return_val = sigsetjmp(t->env, 1);
            if(return_val == 0){
                (t->env->__jmpbuf)[JB_SP] = translate_address(sp);
                (t->env->__jmpbuf)[JB_PC] = translate_address(pc);
                sigemptyset(&t->env->__saved_mask);

            }

            //add to ready deque
            readyQueue.push_back(t);
            return t->tid;
        }
    }
    std::cerr << "thread library error: Max number of threads exists"<<std::endl;
    return FAIL;
}

/**
 * calls to the function every quantom seconds
 * @param a
 */
void timer_handler(int a)
{
    scheduler();
}

/**
 * this function implements the round robin algoritm
 * @return 0 on SUCCESS, -1 on FAIL
 */
int timeMgmt(){
    struct sigaction sa = {0};
    struct itimerval timer;

    // Install timer_handler as the signal handler for SIGVTALRM.
    sa.sa_handler = &timer_handler;
    if (sigaction(SIGVTALRM, &sa,NULL) < 0) {
        std::cerr << "system error: sigaction failed"<<std::endl;
        return FAIL;
    }
    timer.it_value.tv_sec = quantom/UNIT;		// first time interval, seconds part
    timer.it_value.tv_usec = quantom%UNIT;		// first time interval, microseconds part

    // configure the timer to expire every 3 sec after that.
    timer.it_interval.tv_sec = quantom/UNIT;	// following time intervals, seconds part
    timer.it_interval.tv_usec = quantom%UNIT;	// following time intervals, microseconds part

    // Start a virtual timer. It counts down whenever this process is executing.
    if (setitimer (ITIMER_VIRTUAL, &timer, NULL)) {
        std::cerr << "system error: setitimer failed"<<std::endl;
        return FAIL;
    }
    return SUCCESS;
}

/**
 * Description: This function initializes the thread library.
 * You may assume that this function is called before any other thread library
 * function, and that it is called exactly once. The input to the function is
 * the length of a quantum in micro-seconds. It is an error to call this
 * function with non-positive quantum_usecs.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_init(int quantum_usecs) {
    if (quantum_usecs<=0)
    {
        std::cerr << "thread library error: quantum usecs must be positive"<<std::endl;
        return FAIL;
    }
    isMutexBlocked=-1;
    quantom=quantum_usecs;
    totalQuantomCount = 1;
    thread* mainThread = new thread();
    mainThread->tid=0;
    mainThread->isBlocked= false;
    mainThread->mutexBlocked = false;
    mainThread->quantomCount=1;
    threads[0] = mainThread;
    runningThread = mainThread;
    return timeMgmt();
}


/**
 * A method that checks if the tis id valid and there is an existing thread with the given tid
 * @param tid
 * @return true or false
 */
bool checkThread(int tid){
    if (threads.find(tid) == threads.end() || tid<0)
    {

        return false;
    }
    return true;
}


/**
 * A method that frees all the resources
 */
void free_all()
{
    for (auto & thread : threads)
    {
        delete thread.second;
    }
}

/**
 * erase a thread from a queue
 * @param q queue
 * @param tid tid of thread
 */
void erase_thread(std::deque<thread*>& q, int tid)
{
    for (auto it=q.begin(); it!=q.end(); it++)
    {
        if ((*it)->tid == tid)
        {
             q.erase(it);
             break;
        }
    }
}


/**
 * Description: This function terminates the thread with ID tid and deletes
 * it from all relevant control structures. All the resources allocated by
 * the library for this thread should be released. If no thread with ID tid
 * exists it is considered an error. Terminating the main thread
 * (tid == 0) will result in the termination of the entire process using
 * exit(0) [after releasing the assigned library memory].
 * Return value: The function returns 0 if the thread was successfully
 * terminated and -1 otherwise. If a thread terminates itself or the main
 * thread is terminated, the function does not return.
*/
int uthread_terminate(int tid) {
    if (tid == 0)
    {
        free_all();
        exit(0);
    }
    if (!checkThread(tid))
    {
        std::cerr << "thread library error: tid doesnt exists"<<std::endl;
        return FAIL;
    }
    thread* t=threads[tid];
    threads.erase(tid);
    if(isMutexBlocked == tid)
    {
        uthread_mutex_unlock();
    }
    if(t->mutexBlocked)
    {
        erase_thread(mutexBlockedQueue, tid);
    }
    if (t->isBlocked)
    {
        blockedThread.erase(tid);
    }
    else if (runningThread->tid == tid)
    {
        thread* temp=runningThread;
        delete temp;
        ready_to_running();
    }
    else{
        erase_thread(readyQueue, tid);
    }
    delete t;
    return SUCCESS;

}

/**
 * Description: This function blocks the thread with ID tid. The thread may
 * be resumed later using uthread_resume. If no thread with ID tid exists it
 * is considered as an error. In addition, it is an error to try blocking the
 * main thread (tid == 0). If a thread blocks itself, a scheduling decision
 * should be made. Blocking a thread in BLOCKED state has no
 * effect and is not considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_block(int tid) {
    if (!checkThread(tid) || tid == 0 )
    {
        std::cerr << "thread library error: tid doesnt exists"<<std::endl;
        return FAIL;
    }
    thread* t = threads[tid];
    if (runningThread->tid == tid)
    {
        thread* current = runningThread;
        int return_val = sigsetjmp(current->env,1);
        if(return_val == 1){
            return 0;
        }
        current->isBlocked = true;
        blockedThread[current->tid]=current;
        ready_to_running();
    }
    else if(t->isBlocked)
    {
        return SUCCESS;
    }
    else if(t->mutexBlocked){
        t->isBlocked = true;
    }
    else{
        erase_thread(readyQueue, tid);
        t->isBlocked = true;
    }
    blockedThread[t->tid]=t;
    return SUCCESS;

}


/**
 * Description: This function
 * resumes a blocked thread with ID tid and moves
 * it to the READY state if it's not synced. Resuming a thread in a RUNNING or READY state
 * has no effect and is not considered as an error. If no thread with
 * ID tid exists it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_resume(int tid) {
    if (!checkThread(tid))
    {
        std::cerr << "thread library error: tid doesnt exists"<<std::endl;
        return FAIL;
    }
    thread* t = threads[tid];
    if (!t->isBlocked)
    {
        return SUCCESS;
    }
    t->isBlocked= false;
    blockedThread.erase(tid);
    if (!t->mutexBlocked)
    {
        readyQueue.push_back(t);
    }
    return SUCCESS;
}



/**
 * Description: This function tries to acquire a mutex.
 * If the mutex is unlocked, it locks it and returns.
 * If the mutex is already locked by different thread, the thread moves to BLOCK state.
 * In the future when this thread will be back to RUNNING state,
 * it will try again to acquire the mutex.
 * If the mutex is already locked by this thread, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_lock() {
    if (isMutexBlocked!=-1)
    {
        if (isMutexBlocked == runningThread->tid)
        {
            std::cerr << "thread library error: this thread already locked the mutex."<<std::endl;
            return FAIL;
        }
        thread* current=runningThread;
        int return_val = sigsetjmp(current->env,1);
        if(return_val == 1){
            isMutexBlocked = runningThread->tid;
            return 0;
        }
        current->mutexBlocked= true;
        mutexBlockedQueue.push_back(current);
        ready_to_running();
    }
    else {
        isMutexBlocked = runningThread->tid;
    }
    return SUCCESS;
}

/**
 * Description: This function releases a mutex.
 * If there are blocked threads waiting for this mutex,
 * one of them (no matter which one) moves to READY state.
 * If the mutex is already unlocked, it is considered an error.
 * Return value: On success, return 0. On failure, return -1.
*/
int uthread_mutex_unlock() {
    if(isMutexBlocked == -1)
    {
        std::cerr << "thread library error: Mutex is already open"<<std::endl;
        return FAIL;
    }
    if(runningThread->tid != isMutexBlocked)
    {
        std::cerr << "thread library error: Mutex locked by other thread"<<std::endl;
        return FAIL;
    }
    isMutexBlocked = -1;
    std::deque<thread*>::iterator it;
    bool flag=true;
    for (it = mutexBlockedQueue.begin(); it != mutexBlockedQueue.end(); it++) {
        if (!(*it)->isBlocked) {
            flag=false;
            (*it)->mutexBlocked = false;
            readyQueue.push_back((*it));
            mutexBlockedQueue.erase(it);
            break;
        }
    }
    if (flag && !mutexBlockedQueue.empty())
    {
        thread* mbt=mutexBlockedQueue.front();
        mutexBlockedQueue.pop_front();
        mbt->mutexBlocked=false;
    }
    return SUCCESS;
}

/**
 * Description: This function returns the thread ID of the calling thread.
 * Return value: The ID of the calling thread.
*/
int uthread_get_tid() {
    return runningThread->tid;
}

/**
 * Description: This function returns the total number of quantums since
 * the library was initialized, including the current quantum.
 * Right after the call to uthread_init, the value should be 1.
 * Each time a new quantum starts, regardless of the reason, this number
 * should be increased by 1.
 * Return value: The total number of quantums.
*/
int uthread_get_total_quantums() {
    return totalQuantomCount;
}

/**
 * Description: This function returns the number of quantums the thread with
 * ID tid was in RUNNING state. On the first time a thread runs, the function
 * should return 1. Every additional quantum that the thread starts should
 * increase this value by 1 (so if the thread with ID tid is in RUNNING state
 * when this function is called, include also the current quantum). If no
 * thread with ID tid exists it is considered an error.
 * Return value: On success, return the number of quantums of the thread with ID tid.
 * 			     On failure, return -1.
*/
int uthread_get_quantums(int tid) {
    if(checkThread(tid))
    {
        return threads[tid]->quantomCount;
    }
    std::cerr << "thread library error: tid doesnt exists"<<std::endl;
    return FAIL;
}

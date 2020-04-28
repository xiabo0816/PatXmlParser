#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct thread_t thread_t;
	typedef struct mutex_t mutex_t;
	typedef void thread_proc_t(void* arg);
#ifdef __cplusplus
}
#endif

#ifdef _WIN32
#include <windows.h>
#include <process.h>

struct mutex_t {
    CRITICAL_SECTION	m_cs;
};

mutex_t* mutex_create()
{
    mutex_t* m = (mutex_t*) malloc(sizeof(mutex_t));
    if (m == 0)
	return 0;

    memset(m, 0, sizeof(mutex_t));

    InitializeCriticalSection(&m->m_cs);
    return m;
}

void mutex_destroy(mutex_t* m)
{
    DeleteCriticalSection(&m->m_cs);
}

void mutex_secure(mutex_t* m)
{
    EnterCriticalSection(&m->m_cs);
}

void mutex_release(mutex_t* m)
{
    LeaveCriticalSection(&m->m_cs);
}

struct thread_t {
	HANDLE	m_hThread;
};

thread_t* thread_create(thread_proc_t* p, void* arg)
{
    thread_t* t = (thread_t*) malloc(sizeof(thread_t));
    if (t == 0)
	return 0;
    
	t->m_hThread = (HANDLE) _beginthread(p, 0, arg);
	if (-1 == (int) t->m_hThread) {
	free(t);
	return 0;
    }
    return t;
}

thread_t* thread_create_ex(void *lpStartAddress, void *lpParameter)
{
    thread_t* t = (thread_t*) malloc(sizeof(thread_t));
    if (t == 0)
	return 0;
    t->m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)lpStartAddress, (LPVOID)lpParameter, 0, NULL);
	//CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadInfo, LPVOID(&pThreadInfo[i]), 0, NULL);
    return t;
}

void thread_join_ex(thread_t* t, long long dwMilliseconds)
{
	WaitForSingleObject(t->m_hThread, dwMilliseconds);
    return;
}

#else
#include <pthread.h>

struct mutex_t {
    pthread_mutex_t mutex;
};

mutex_t* mutex_create()
{
    mutex_t* m = (mutex_t*) malloc(sizeof(mutex_t));
    if (m == 0)
	return 0;

    memset(m, 0, sizeof(mutex_t));

    pthread_mutex_init(&m->mutex, 0);
    return m;
}

void mutex_destroy(mutex_t* m)
{
    pthread_mutex_destroy(&m->mutex);
}

void mutex_secure(mutex_t* m)
{
    pthread_mutex_lock(&m->mutex);
}

void mutex_release(mutex_t* m)
{
    pthread_mutex_unlock(&m->mutex);
}

struct thread_t {
    pthread_t thread;
};

thread_t* thread_create(thread_proc_t* p, void* arg)
{
    thread_t* t = 0;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr))
	return 0;

    if (pthread_attr_setdetachstate(&attr, 0))
	return 0;
    
    t = (thread_t*) malloc(sizeof(thread_t));
    if (t == 0)
	return 0;

    if (pthread_create(&t->thread, &attr, (void*(*)())p, arg)) {
	free(t);
	return 0;
    }
    return t;
}

thread_t* thread_create_ex(void *lpStartAddress, void *lpParameter)
{
    return thread_create(lpStartAddress, lpParameter);
}

void thread_join_ex(thread_t* t, long long dwMilliseconds)
{
	pthread_join(t->thread, NULL);
    return;
}
#endif

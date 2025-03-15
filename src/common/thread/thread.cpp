#include "common.h"
#include "thread.h"

#ifdef DC_SH4
#   include <kos.h>
#else
#   include <thread>
#endif

namespace dc {

Thread::Thread(const char* label, size_t stackSize, bool detached, RunFunction runFunction, void* param) {
    spawn(label, stackSize, detached, runFunction, param);
}

Thread::~Thread() {
    if(!detached_) join();
}

bool Thread::spawn(const char *label, size_t stackSize, bool detached, RunFunction runFunction, void* param) {
#ifdef DC_SH4
	const kthread_attr_t thdAttr = {
        .create_detached = detached,
        .stack_size      = stackSize,
        .label           = label
    };

    nativeHandle_ = 
        reinterpret_cast<uintptr_t>(thd_create_ex(&thdAttr, runFunction, param));
#else
    nativeHandle_ =
        reinterpret_cast<uintptr_t>(new std::thread(runFunction, param));
#endif
    detached_ = detached;

    return !!nativeHandle_;
}

bool Thread::join() {
    if(!isValid() || detached_)
        return false;

#ifdef DC_SH4
    if(thd_join(reinterpret_cast<kthread_t*>(nativeHandle_), nullptr) != 0)
        return false;
#else
    delete reinterpret_cast<std::thread*>(nativeHandle_);
#endif

    nativeHandle_ = 0;
    detached_ = false;

    return true;
}

}
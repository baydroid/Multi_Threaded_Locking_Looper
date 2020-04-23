/*
 * Copyright 2020 transmission.aquitaine@yahoo.com
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K (1)
#endif

#include <bits/pthreadtypes.h>
#include <pthread.h>

#include "basic_types.h"
#include "UintXTrieSet.hpp"
#include "MTLL.hpp"



namespace MTLL { // MultiThreaded Locking Looper



///////////////////////////////////////////////////////////////////////////////



class LockQHdr
    {
private:
    friend class Controller;
    friend class Lock;

    DList<Looper> waiting;
    Looper *firstShared;
    void init() { firstShared = 0; waiting.init(); }
    };

class LockSetIterator : private UintaTrieSet::Iterator
    {
private:
    friend class Controller;

    LockSetIterator()               :        UintaTrieSet::Iterator::Iterator()                     { }
    LockSetIterator(LockSet *lkset) :        UintaTrieSet::Iterator::Iterator((UintaTrieSet*)lkset) { }
    void init(LockSet *lkset)       {        UintaTrieSet::Iterator::init((UintaTrieSet*)lkset);      }
    bool next(Lock **lk)            { return UintaTrieSet::Iterator::next((uinta*)lk);                }
    };



///////////////////////////////////////////////////////////////////////////////



Looper::Looper()
    {
    mtllNext = mtllPrev = 0;
    runningTaskPriority = 0;
    markedForDelete = taskRunning = NO;
    tasks.init();
    }

Looper::~Looper()
    {
    assert(!taskRunning);
    assert(tasks.empty());
    assert(!locksHeld.size());
    }



Task::Task()
    {
    mtllNext = mtllPrev = 0;
    mtllPrio = 0;
    mtllLock = 0;
    mtllExclusive = NO;
    mtllDeleteAfterwards = NO;
    }



Lock::Lock(Controller *c)
    {
    priorities = new LockQHdr[c->maxPriority + 1];
    for (uinta i = 0; i <= c->maxPriority; i++) priorities[i].init();
    holderCount = 0;
    exclusive = markedForDelete = NO;
    }

Lock::~Lock()
    {
    assert(!holderCount);
    delete[] priorities;
    }



///////////////////////////////////////////////////////////////////////////////



extern "C" void *mtllStartThread(void *context)
    {
    ((Controller*)context)->runPoolThread();
    return 0;
    }

Controller::Controller(uinta threadCount, uinta maxPriority)
    {
    this->maxPriority = maxPriority;
    priorities = new DList<Looper>[maxPriority + 1];
    for (uinta i = 0; i <= maxPriority; i++) priorities[i].init();
    runningThreadCount = 0;
    waitingThreadCount = 0;
    readyLooperCount = 0;
    specialLooper = new Looper();
    mutex = PTHREAD_MUTEX_INITIALIZER;
    cond = PTHREAD_COND_INITIALIZER;
    startThreadPool(threadCount);
    }

Controller::~Controller()
    {
    assert(false);
    }

void Controller::runPoolThread()
    {
    takeMutex();
    for ( ; ; )
        {
        Looper *lpr;
        for ( ; ; )
            {
            lpr = fetchNextReadyLooper();
            if (lpr) break;
            waitingThreadCount++;
            waitOnCondition();
            waitingThreadCount--;
            }
        if (lpr != specialLooper && waitingThreadCount && readyLooperCount) signalCondition();
        DList<Task> *tasks = &lpr->tasks;
        Task *t = tasks->unlinkFirst();
        if (lpr != specialLooper) runningThreadCount++;
        lpr->taskRunning = YES;
        lpr->runningTaskPriority = t->mtllPrio;
        const bool deleteAfterwards = t->mtllDeleteAfterwards;
        releaseMutex();
        t->mtllRun(this, lpr != specialLooper ? lpr : 0);
        if (deleteAfterwards) delete t;
        takeMutex();
        lpr->taskRunning = NO;
        if (lpr != specialLooper)
            {
            runningThreadCount--;
            if (!lpr->tasks.empty())
                waitForLockOrMakeReady(lpr);
            else if (lpr->markedForDelete)
                finalizeAndDelete(lpr);
            }
        }
    }

Looper *Controller::fetchNextReadyLooper()
    {
    if (specialLooper->taskRunning) return 0;
    if (!specialLooper->tasks.empty()) return runningThreadCount == 0 ? specialLooper : 0;
    for (inta i = maxPriority; i >= 0; i--)
        {
        DList<Looper> *hdr = priorities + i;
        Looper *lpr = hdr->unlinkFirst();
        if (lpr)
            {
            readyLooperCount--;
            return lpr;
            }
        }
    return 0;
    }

bool Controller::waitForLockOrMakeReady(Looper *lpr)
    {
    Task *t = lpr->tasks.first;
    Lock *lk = t->mtllLock;
    if (lk && !attemptLockHM(lpr, lk, t->mtllExclusive))
        {
        LockQHdr *hdr = lk->priorities + t->mtllPrio;
        if (t->mtllExclusive)
            hdr->waiting.linkBefore(lpr, hdr->firstShared);
        else
            {
            if (!hdr->firstShared) hdr->firstShared = lpr;
            hdr->waiting.linkLast(lpr);
            }
        return NO;
        }
    makeReady(lpr);
    return YES;
    }

void Controller::enqueue(Looper *lpr, Task *t, uinta priority, bool deleteAfterwards)
    {
    t->mtllPrio = priority;
    t->mtllDeleteAfterwards = deleteAfterwards;
    t->mtllLock = 0;
    takeMutex();
    lpr->tasks.linkLast(t);
    if (!lpr->taskRunning && lpr->tasks.first == t)
        {
        makeReady(lpr);
        if (waitingThreadCount) signalCondition();
        }
    releaseMutex();
    }

void Controller::enqueue(Looper *lpr, Task *t, uinta priority, bool deleteAfterwards, Lock *lk, bool exclusive)
    {
    t->mtllPrio = priority;
    t->mtllDeleteAfterwards = deleteAfterwards;
    t->mtllLock = lk;
    t->mtllExclusive = exclusive;
    takeMutex();
    lpr->tasks.linkLast(t);
    if (!lpr->taskRunning && lpr->tasks.first == t && waitForLockOrMakeReady(lpr) && waitingThreadCount) signalCondition();
    releaseMutex();
    }

void Controller::enqueueAndStopTheWorld(Task *t, bool deleteAfterwards)
    {
    t->mtllPrio = maxPriority;
    t->mtllDeleteAfterwards = deleteAfterwards;
    t->mtllLock = 0;
    takeMutex();
    specialLooper->tasks.linkLast(t);
    if (!specialLooper->taskRunning && specialLooper->tasks.first == t && !runningThreadCount) signalCondition();
    releaseMutex();
    }

bool Controller::attemptLock(Looper *lpr, Lock *lk, bool exclusive)
    {
    takeMutex();
    const bool result = attemptLockHM(lpr, lk, exclusive);
    releaseMutex();
    return result;
    }

bool Controller::attemptLockHM(Looper *lpr, Lock *lk, bool exclusive)
    {
    if (lk->holderCount)
        {
        if (exclusive)
            return NO;
        else
            {
            if (lk->exclusive) return NO;
            inta priority = lpr->priority();
            for (inta i = maxPriority; i >= priority; i--)
                {
                Looper *firstLooper = lk->priorities[i].waiting.first;
                if (firstLooper && firstLooper->tasks.first->mtllExclusive) return NO;
                }
            }
        }
    takeLock(lpr, lk, exclusive);
    return YES;
    }

void Controller::takeLock(Looper *lpr, Lock *lk, bool exclusive)
    {
    assert(!lpr->locksHeld.contains(lk));
    lk->exclusive = exclusive;
    lk->holderCount++;
    lpr->locksHeld.set(lk);
    }

void Controller::unlock(Looper *lpr, Lock *lk)
    {
    takeMutex();
    if (unlockHM(lpr, lk) && waitingThreadCount) signalCondition();
    releaseMutex();
    }

bool Controller::unlockHM(Looper *lpr, Lock *lk)
    {
    assert(lpr->locksHeld.expunge(lk));
    if (--lk->holderCount) return NO;
    bool lockGranted = NO;
    for (inta i = maxPriority; i >= 0; i--)
        {
        LockQHdr *hdr = lk->priorities + i;
        if (lockGranted)
            {
            Looper *lpr = hdr->firstShared;
            while (lpr)
                {
                Looper *prevLpr = lpr->mtllPrev;
                hdr->waiting.unlink(lpr);
                takeLock(lpr, lk, false);
                makeReady(lpr);
                lpr = prevLpr;
                }
            hdr->firstShared = 0;
            }
        else
            {
            Looper *lpr = hdr->waiting.first;
            if (lpr)
                {
                if (lpr->tasks.first->mtllExclusive)
                    {
                    hdr->waiting.unlink(lpr);
                    takeLock(lpr, lk, true);
                    makeReady(lpr);
                    return YES;
                    }
                lockGranted = YES;
                while (lpr)
                    {
                    Looper *prevLpr = lpr->mtllPrev;
                    hdr->waiting.unlink(lpr);
                    takeLock(lpr, lk, false);
                    makeReady(lpr);
                    lpr = prevLpr;
                    }
                hdr->firstShared = 0;
                }
            }
        }
    if (!lockGranted && lk->markedForDelete) delete lk;
    return lockGranted;
    }

void Controller::makeReady(Looper *lpr)
    {
    priorities[lpr->tasks.first->mtllPrio].linkLast(lpr);
    readyLooperCount++;
    }

void Controller::safeDelete(Looper *lpr)
    {
    takeMutex();
    if (!lpr->taskRunning && lpr->tasks.empty())
        {
        if (finalizeAndDelete(lpr) && waitingThreadCount) signalCondition();
        }
    else
        lpr->markedForDelete = YES;
    releaseMutex();
    }

bool Controller::finalizeAndDelete(Looper *lpr)
    {
    bool lockGranted = NO;
    LockSetIterator *it = new LockSetIterator(&lpr->locksHeld);
    Lock *lk;
    while (it->next(&lk)) if (unlockHM(lpr, lk)) lockGranted = YES;
    delete lpr;
    return lockGranted;
    }

void Controller::safeDelete(Lock *lk)
    {
    takeMutex();
    if (!lk->holderCount)
        delete lk;
    else
        lk->markedForDelete = YES;
    releaseMutex();
    }



///////////////////////////////////////////////////////////////////////////////



}; // end of namespace MTLL

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
#ifndef MTLL_HPP_
#define MTLL_HPP_



#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K (1)
#endif

#include <bits/pthreadtypes.h>
#include <pthread.h>

#include "basic_types.h"
#include "UintXTrieSet.hpp"



namespace MTLL { // MultiThreaded Locking Looper



template<class Item> class DList;
class Controller;
class Task;
class Looper;
class LockQHdr;
class Lock;
class LockSet;
class LockSetIterator;

extern "C" void *mtllStartThread(void *context);



///////////////////////////////////////////////////////////////////////////////



template<class Item>
class DList
    {
private:
    friend class Controller;
    friend class Looper;
    friend class LockQHdr;

    Item *first;
    Item *last;

    DList()
        {
        first = last = 0;
        }

    void init()
        {
        first = last = 0;
        }

    bool empty()
        {
        return first == 0;
        }

    void linkLast(Item *it)
        {
        it->mtllPrev = 0;
        it->mtllNext = last;
        if (last)
            last->mtllPrev = it;
        else
            first = it;
        last = it;
        }

    void linkBefore(Item *it, Item *beforeThis)
        {
        it->mtllPrev = beforeThis;
        if (beforeThis)
            {
            it->mtllNext = beforeThis->mtllNext;
            beforeThis->mtllNext = it;
            }
        else
            {
            it->mtllNext = last;
            last = it;
            }
        if (it->mtllNext)
            it->mtllNext->mtllPrev = it;
        else
            first = it;
        }

    Item *unlinkFirst()
        {
        Item *it = first;
        if (it)
            {
            first = it->mtllPrev;
            if (first)
                first->mtllNext = 0;
            else
                last = 0;
            }
        return it;
        }

    Item *unlink(Item *it)
        {
        if (it->mtllNext)
            it->mtllNext->mtllPrev = it->mtllPrev;
        else
            first = it->mtllPrev;
        if (it->mtllPrev)
            it->mtllPrev->mtllNext = it->mtllNext;
        else
            last = it->mtllNext;
        return it;
        }
    };



///////////////////////////////////////////////////////////////////////////////



class Controller
    {
public:
    Controller(uinta threadCount, uinta maxPriority);
    virtual ~Controller();
    void enqueue(Looper *lpr, Task *t, uinta priority, bool deleteAfterwards);
    void enqueue(Looper *lpr, Task *t, uinta priority, bool deleteAfterwards, Lock *lk, bool exclusive);
    void enqueueAndStopTheWorld(Task *t, bool deleteAfterwards);
    bool attemptLock(Looper *lpr, Lock *lk, bool exclusive);
    void unlock(Looper *lpr, Lock *lk);
    void safeDelete(Looper *lpr);
    void safeDelete(Lock *lk);

private:
    friend class Lock;

    uinta waitingThreadCount;
    uinta runningThreadCount;
    uinta readyLooperCount;
    Looper *specialLooper;
    DList<Looper> *priorities;
    uinta maxPriority;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    void runPoolThread();
    Looper *fetchNextReadyLooper();
    bool waitForLockOrMakeReady(Looper *lpr);
    bool attemptLockHM(Looper *lpr, Lock *lk, bool exclusive);
    void takeLock(Looper *lpr, Lock *lk, bool exclusive);
    bool unlockHM(Looper *lpr, Lock *lk);
    void makeReady(Looper *lpr);
    bool finalizeAndDelete(Looper *lpr);
    friend void *mtllStartThread(void *context);
    void startThreadPool(uinta threadCount) { pthread_t thd; while (threadCount--) assert(!pthread_create(&thd, 0, mtllStartThread, this)); }
    void takeMutex()                        { assert(!pthread_mutex_lock(&mutex));                                                          }
    void releaseMutex()                     { assert(!pthread_mutex_unlock(&mutex));                                                        }
    void waitOnCondition()                  { assert(!pthread_cond_wait(&cond, &mutex));                                                    }
    void signalCondition()                  { assert(!pthread_cond_signal(&cond));                                                          }
    };



///////////////////////////////////////////////////////////////////////////////



class Task
    {
public:
    Task();
    virtual ~Task() { }
    uinta mtllPriority() { return mtllPrio; }
    virtual void mtllRun(Controller *c, Looper *lpr) = 0;

private:
    friend class DList<Task>;
    friend class Controller;
    friend class Looper;

    Task *mtllNext;
    Task *mtllPrev;
    uinta mtllPrio;
    Lock *mtllLock;
    bool mtllExclusive;
    bool mtllDeleteAfterwards;
    };



///////////////////////////////////////////////////////////////////////////////



class Lock
    {
public:
    Lock(Controller *c);

protected:
    virtual ~Lock();

private:
    friend class Controller;

    LockQHdr *priorities;
    uinta holderCount;
    bool exclusive;
    bool markedForDelete;
    };

class LockSet : private UintaTrieSet
    {
private:
    friend class Controller;
    friend class Looper;

    bool set(const Lock *lk)      { return UintaTrieSet::set((uinta)lk);      }
    bool expunge(const Lock *lk)  { return UintaTrieSet::expunge((uinta)lk);  }
    bool contains(const Lock *lk) { return UintaTrieSet::contains((uinta)lk); }
    void clear()                  {        UintaTrieSet::clear();             }
    uinta size()                  { return UintaTrieSet::size();              }
    };



///////////////////////////////////////////////////////////////////////////////



class Looper
    {
public:
    Looper();

protected:
    virtual ~Looper();

private:
    friend class DList<Looper>;
    friend class Controller;

    Looper *mtllNext;
    Looper *mtllPrev;
    DList<Task> tasks;
    LockSet locksHeld;
    uinta runningTaskPriority;
    bool taskRunning;
    bool markedForDelete;

    uinta priority() { return taskRunning ? runningTaskPriority : (tasks.first ? tasks.first->mtllPrio : 0); }
    };



///////////////////////////////////////////////////////////////////////////////



}; // end of namespace MTLL
#endif // #ifndef MTLL_HPP_

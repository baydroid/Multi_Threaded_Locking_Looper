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
#include <unistd.h>
#include <sys/time.h>

#include <stdio.h>
#include <string.h>

#include "MTLL.hpp"
using namespace MTLL;



static uinta getTimeMillis()
    {
    struct timeval osTime;
    gettimeofday(&osTime, 0);
    return 1000*osTime.tv_sec + osTime.tv_usec/1000;
    }

uinta startTimeMillis;



class ExampleLock : public Lock
    {
public:
    char *name;
    ExampleLock(Controller *c, const char *name) : Lock(c) { this->name = (char*)name; }
    // ~ExampleLock() { printf("%06llu Lock %s being deleted\n", getTimeMillis() - startTimeMillis, name); }
    };



Controller *mtll;
ExampleLock *lk;



class ExampleLooper : public Looper
    {
public:
    char *name;
    ExampleLooper(const char *name) { this->name = (char*)name; }
    // ~ExampleLooper() { printf("%06llu Looper %s being deleted\n", getTimeMillis() - startTimeMillis, name); }
    };



class ExampleTask : public Task
    {
public:
    ExampleTask(uinta waitSeconds1, uinta waitSeconds2, const char *name);
    // ~ExampleTask() { printf("%06llu Task %s being deleted\n", getTimeMillis() - startTimeMillis, name); }
    virtual void mtllRun(Controller *c, Looper *lpr);
    virtual void startActions(Controller *c, Looper *lpr) { }
    virtual void middleActions(Controller *c, Looper *lpr) { }
    virtual void endActions(Controller *c, Looper *lpr) { }

private:
    uinta waitSeconds1;
    uinta waitSeconds2;
    char *name;
    char *lprName;
    };

ExampleTask::ExampleTask(uinta waitSeconds1, uinta waitSeconds2, const char *name)
    {
    this->waitSeconds1 = waitSeconds1;
    this->waitSeconds2 = waitSeconds2;
    this->name = (char*)name;
    lprName = 0;
    }

void ExampleTask::mtllRun(Controller *c, Looper *lpr)
    {
    lprName = lpr ? ((ExampleLooper*)lpr)->name : (char*)"X";
    printf("%06llu START %s running %s\n", getTimeMillis() - startTimeMillis, lprName, name);
    startActions(c, lpr);
    sleep(waitSeconds1);
    middleActions(c, lpr);
    sleep(waitSeconds2);
    endActions(c, lpr);
    printf("%06llu END   %s running %s\n", getTimeMillis() - startTimeMillis, lprName, name);
    }



class TaskA3 : public ExampleTask
    {
public:
    TaskA3() : ExampleTask(1, 2, "A3") { }
    void startActions(Controller *c, Looper *lpr) { c->safeDelete(lpr); }
    void endActions(Controller *c, Looper *lpr) { c->unlock(lpr, lk); }
    };

class TaskA2 : public ExampleTask
    {
public:
    TaskA2() : ExampleTask(1, 2, "A2") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskA3(), mtllPriority(), YES); }
    void endActions(Controller *c, Looper *lpr) { c->enqueueAndStopTheWorld(new ExampleTask(2, 2, "Stop the World"), YES); }
    };

class TaskA1 : public ExampleTask
    {
public:
    TaskA1() : ExampleTask(1, 2, "A1") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskA2(), mtllPriority(), YES); }
    };

class TaskA0 : public ExampleTask
    {
public:
    TaskA0() : ExampleTask(1, 2, "A0") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskA1(), mtllPriority(), YES, lk, YES); }
    };



class TaskB3 : public ExampleTask
    {
public:
    TaskB3() : ExampleTask(2, 2, "B3") { }
    void startActions(Controller *c, Looper *lpr) { c->safeDelete(lpr); }
    };

class TaskB2 : public ExampleTask
    {
public:
    TaskB2() : ExampleTask(2, 2, "B2") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskB3(), mtllPriority(), YES); }
    };

class TaskB1 : public ExampleTask
    {
public:
    TaskB1() : ExampleTask(2, 2, "B1") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskB2(), mtllPriority(), YES); }
    };

class TaskB0 : public ExampleTask
    {
public:
    TaskB0() : ExampleTask(2, 2, "B0") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskB1(), mtllPriority(), YES, lk, NO); }
    };



class TaskC3 : public ExampleTask
    {
public:
    TaskC3() : ExampleTask(2, 3, "C3") { }
    void startActions(Controller *c, Looper *lpr) { c->safeDelete(lpr); }
    };

class TaskC2 : public ExampleTask
    {
public:
    TaskC2() : ExampleTask(2, 3, "C2") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskC3(), mtllPriority(), YES); }
    };

class TaskC1 : public ExampleTask
    {
public:
    TaskC1() : ExampleTask(2, 3, "C1") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskC2(), mtllPriority(), YES); }
    void endActions(Controller *c, Looper *lpr) { c->safeDelete(lk); }
    };

class TaskC0 : public ExampleTask
    {
public:
    TaskC0() : ExampleTask(2, 3, "C0") { }
    void middleActions(Controller *c, Looper *lpr) { c->enqueue(lpr, new TaskC1(), mtllPriority(), YES, lk, NO); }
    };



void test1()
    {
    Looper *la = new ExampleLooper("A");
    Looper *lb = new ExampleLooper("B");
    Looper *lc = new ExampleLooper("C");
    Looper *ld = new ExampleLooper("D");

    Task *a0 = new TaskA0();
    Task *b0 = new TaskB0();
    Task *c0 = new TaskC0();
    Task *d0 = new ExampleTask(3, 3, "D0");
    Task *d1 = new ExampleTask(3, 3, "D1");
    Task *d2 = new ExampleTask(3, 3, "D2");
    Task *d3 = new ExampleTask(3, 3, "D3");

    mtll->enqueue(la, a0, 1, YES);
    mtll->enqueue(lb, b0, 1, YES);
    mtll->enqueue(lc, c0, 0, YES);
    mtll->enqueue(ld, d0, 0, YES);
    mtll->enqueue(ld, d1, 0, YES);
    mtll->enqueue(ld, d2, 0, YES);
    mtll->enqueue(ld, d3, 0, YES);
    mtll->safeDelete(ld);
    }



int main(int argc, char* argv[])
    {
    mtll = new Controller(3, 1);
    lk = new ExampleLock(mtll, "L");
    startTimeMillis = getTimeMillis();
    test1();
    printf("mainline sleeping\n");
    sleep(40);
    return 0;
    }

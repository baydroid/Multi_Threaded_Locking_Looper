# Multi-Threaded Locking Looper (MTLL)

1) Summary

The Multi-Threaded Locking Looper is a multi-threaded version of the classic event loop or idler loop design pattern. It provides for an indefinte number of loopers each sequentially executing the sequence of tasks queued on the looper. A pool of worker threads executes the tasks, so tasks running on different loopers may execute concurrently. Therefore shared/exclusive or reader/writer locks are also provided to support safe sharing of data between loopers. The multi-threaded approach makes it possible to use all the CPU cores, instead of only 1 of them, thus boosting performance.

2) Motivation

A single threaded looper consists of a queue of tasks, and a thread executing them. The thread runs in a loop which starts by checking the queue. If the queue has tasks in it, then the thread executes those tasks in the order in which they were queued, removing them from the queue as it does so. If the queue's empty, then the thread waits on an OS provided waitable thing, such as an event object in Windows, or a condition variable in Unixes. When a task's added to the queue the waitable thing's notified, and the looper thread waiting on it wakes up, goes to the top of the loop, and starts executing the task(s) in the queue.

This general type of arrangement provides good support for single threaded asynchronous IO. IO events, such as data becoming available for reading, are delivered to the code doing the IO in the form of tasks queued on a looper (which subsequently executes them). Because the looper's single threaded, only 1 task runs at once, and therefore there's no need to use mutexes and etc. to protect data from simultaneous updates by multiple threads. Broadly speaking this is the approach used by Node.js.

However this elegant simplicity comes at a price. A single threaded looper is, well, single threaded. On, say, a 16 core CPU that single thread can only use a single core, leaving the other 15 cores potentialy idle. Those idle cores are a significant untapped resource of computing power. Using only 1 core is not likely to give the best performance or scalability the hardware has to offer, except in the nowdays rare case of single core CPUs.

The motivation for the Multi-Threaded Locking Looper is the desire to obtain the best performance by using all the available CPU cores, and to do so in a scalable manner. Whilst at the same time preserving as much as possible of the single threaded looper's simplicity in respect of not needing mutexes and etc. to coordinate access to shared data.

3) MTLL Looper Features

The Multi-Threaded Locking Looper (MTLL from now on) supports the creation and destruction of multiple loopers. It's expected that in the course of normal MTLL usage loopers will be created and destroyed on a continuous ongoing basis, with many in existance at any 1 time.

Each individual looper behaves logicaly just like a single threaded looper. It's queued tasks are executed in order 1 at a time. So tasks executing on the same looper can share data safely without needing mutexes and etc. because they're serialized. But MTLL allows tasks executing on different loopers to run concurrently on multiple threads. Therefore, if tasks executing on different loopers share data they must take care to avoid simultaneous updates from multiple threads corupting the shared data.

MTLL also provides a single special "Stop the World" looper. When a task's queued on this looper it causes all the other loopers to temporarily halt at the end their current tasks. Once all the other loopers have halted the tasks queued on the "Stop the World" looper are executed (in order & 1 at a time). A task running on the "Stop the World" looper can rely on MTLL not running any other tasks at the same time. After all the "Stop the World" tasks are done then MTLL resumes normal concurrent execution of other loopers.

Additionally MTLL supports the prioritization of tasks. Priorities run from 0 (the lowest) to a maximum set when the MTLL's started. A task's priority's specified when the task is queued to a looper. If both a higher priority task and a lower priority task are queued (each on their own looper) then MTLL will start executing the higher priority task bfore or at the same time as the lower priority task. Of course, if the lower priority task's queued first, then it may start executing immediately and thus run before a higher priority task queued only moments later. Also note that the use of locks (see below) can modify this behaviour. 

For the best performance it's good to use as few priorities as possible so as to minimize resouce usage. Every Lock object (of which there could be a great many) includes an array of length equal to the number of priorities, and every choice of a task to run must potentially scan all the priorities.

Priorities and locks have no effect on the "Stop the World" looper, which always behaves as described above.

MTLL uses a pool of worker threads to execute the loopers' tasks. The number of threads in that pool doesn't change, it's set when the MTLL's started. So most of the time there'll probably be more loopers than threads. In this circumstance MTLL assigns worker threads to loopers with tasks of equal priority ready for execution in a round robin fashion. If no locks were used, and all tasks had the same priority, and took the same amount of time, then MTLL would give each looper the same total amount of execution time over the long run.

Having a constant number of threads in the worker pool helps with scaling. The number can be chosen to be large enough to keep all the available CPU cores busy, and yet small enough that the thread context switching overhead doesn't become significant. Designs where the number of threads increases when the number of loopers increases (e.g. 1 thread per looper) don't scale well.

4) MTLL Locking features

MTLL preserves the simplicity of a single threaded looper in that tasks running on the same looper can share data without concern about synchronizing their accesses. The same cannot be said of tasks running on different loopers sharing data, since they may be executing concurrently on different threads. Normal multi-threaded precations against simultaneous updates corrupting the shared data are required in this case.

MTLL provides shared/exclusive locks (also called reader/writer locks) which tasks running on different loopers can use to safely share data. The lock holding entity in MTLL is the looper. When a task acquires a lock it's actually the looper running the task that gets the lock. The looper can continue to hold the lock after the task completes and it proceeds to execute more tasks. It's expected that in the course of normal MTLL usage locks will be created and destroyed on a continuous ongoing basis, with many in existance at any 1 time.

An MTLL lock can be in 1 of 3 states. It can be unlocked, locked in shared mode or locked in exclusive mode. Any number of loopers can lock the same lock at the same time in shared mode. But only 1 looper can hold a lock in exclusive mode at any 1 time. If necessary another looper wanting for a lock must wait until the looper holding the lock releases it.

When both shared and exclusive requests are waiting for a lock, MTLL grants the exclusive requests first (and then the shared requests). Except that a shared request from a looper at a higher priority beats an exclusive request from a looper at a lower priority.

A lock can be requested (in either shared or exclusive mode) whenever a task's queued on a looper. MTLL will acquire the lock for the looper before executing the task. If the looper must wait for the lock, then the task sits waiting in an MTLL internal queue (not being executed) until the lock becomes available. At which point the looper gets the lock and the task is executed as soon as a worker thread's available. Execution of the task can be thought of as notification of the lock being granted.

There's also an API call to attempt to get a lock without having to queue a task at the same time. If it can take the lock at once fine, but it doesn't wait for the lock if it's unavailable because another looper already holds it. Instead it gives up on getting the lock and returns immediately. The return value is a boolean indicating whether or not it was able to get the lock.

Be warned. It's not hard to get into trouble with deadlocks (also called deadly embraces) when using the MTLL's locking. If you don't already know what a deadlock is then do some online research before using MTLL's locks. Wikipeidia's piece on the topic, https://en.wikipedia.org/wiki/Deadlock, is a starting point.

It would have been possible to implement looping and locking as 2 independant software modules. For example, when a task running on a looper must wait for a lock, it could wait by blocking the thread executing the task inside the lock module's get lock API call. But if this happens then that thread stops executing anything, it's effectively temporarily withdrawn from service until the lock's granted. In a design where the loopers each have thier own thread this isn't a problem (though scaling to handle large numbers of loopers is a problem for such a design). But in a design like MTLL with a worker pool containing a fixed number of threads having 1 or more of them out of service means there may not be enough threads left to keep all the CPU cores busy.

Integrating looping and locking into a single module allows loopers waiting for locks to do so in an internal MTLL queue without taking worker pool threads out of service while they're waiting. This is better for scalability, especially since with large enough numbers of loopers just by the law of averages there could be more loopers waiting for locks than there are threads in the worker pool.

5) Using MTLL

MTLL's written in C++ for Linux. It compiles with the GNU compiler using make. The expected way of using it is to include MTLL's source files in the source files of the project using it. It depends on libpthread, and the C & C++ standard libraries. All of which are included by default in the vast majority of Linux distributions. Its API's declared in the following header.

    #include "MTLL.hpp"

An example program using MTLL is included with the project, and can be refered to for further information on using MTLL.

It's API's provided by 4 classes: Looper, Task, Lock, and Controller, all in the MTLL namespace. They're all normal C++ classes, and all of them have virtual destructors. So classes which inherit from them may be freely used in place of them.

Class Controller provides almost the entire API, it's the "brains" of the MTLL. It's where the worker pool threads share the data they need to share in order to coordinate amongst themselves about managing the locks and executing the tasks. It's expected that most processes will use only 1 Controller which will run forever. Currently there's no provision for stopping an MTLL once it's started, calling Controller's destructor deliberately crashes the process with assert(false).

If a process does use multiple instances of class Controller, then it must take care to ensure that each instance of Looper, or Task, or Lock (or their subclasses) is always used with the same Controller. Controllers must not share Loopers, Tasks or Locks.

The only public methods on classes Lock and Looper are their constructors. Protected destructors are provided to facilitate subclassing. But objects of class Lock, or Looper or their subclasses should only be deleted via class Controller's safeDelete() methods. This allows the Controller to defer deletion if it's still using the object.

Class Task represents a task to be queued on a looper for execution by a worker thread. Making a task for use with MTLL is easy. Derive a new class from class Task, and override the (abstract) method called mtllRun(). MTLL will execute the task by calling this override. Any additional data required can be provided by storing it as members of the derived class.

It's not OK to delete a Task object during the time interval starting from when the Task's first queued on a Looper and ending when the Task's mtllRun() begins executing. MTLL can optionally delete the Task object automatically after it finishes. If this option's not chosen, it becomes OK to delete the Task object at the moment at which the Task's own mtllRun() method begins executing.

6) The MTLL API

Note that the type uinta means an unsigned integer of the same size as a void pointer.

Class MTLL::Controller

    public Controller(uinta threadCount, uinta maxPriority)

Construct a new instance of Controller. Parameter threadCount specifies the number of threads in the worker pool. Parameter maxPriority specifies the maximum priority. Use as few priorities as you can.

    public virtual ~Controller()

Destruction of Controller objects is not (yet) supported. Currently this method uses assert(false) to deliberately crash the process if it's called.

    public void enqueue(Looper *lpr, Task *t, uinta priority, bool deleteAfterwards)

Enqueue the given Task on the given Looper at the given priority. If deleteAfterwards is true then delete the Task object after executing it.

    public void enqueue(Looper *lpr, Task *t, uinta priority, bool deleteAfterwards, Lock *lk, bool exclusive)

The same as the above method, but also request the given Lock. Set exclusive to true to request the lock in exclusive mode, and false to request it in shared mode.

    public void enqueueAndStopTheWorld(Task *t, bool deleteAfterwards)

Enqueue the given Task on the special "Stop the World" looper. If deleteAfterwards is true then delete the Task object after executing it. When Tasks are queued on the "Stop the World" Looper all other Loopers temporarily halt when their current Tasks finish. When they've all halted, the "Stop the World" Tasks are executed on their own.

    public bool attemptLock(Looper *lpr, Lock *lk, bool exclusive)

The given Looper requests the given Lock. If exclusive's true then it's requested in exclusive mode, otherwise it's requested in shared mode. If the Lock's available then the Looper gets it, and this method returns true. If the Lock's not available then this method does not wait until it becomes available, instead it returns false immediately (and the Looper does not get the Lock).

    public void unlock(Looper *lpr, Lock *lk)

The given Looper releases or unlocks the given Lock.

    public void safeDelete(Looper *lpr)

Delete the given Looper object. If the Controller's using the Looper its deletion may be delayed untile the Controller's done with it. Loopers (or their subclasses) should not be deleted, except by means of this method. It's OK to call safeDelete() while ther're Tasks still queued on the Looper because safeDelete() waits until ther're no queued Tasks before deleting the Looper. It also automatically releases any Locks the Looper holds when it's deleted.

    public void safeDelete(Lock *lk)

Delete the given Lock object. If the Controller's using the Lock its deletion may be delayed untile the Controller's done with it. Locks (or their subclasses) should not be deleted, except by means of this method. It's OK to call safeDelete() while Loopers still hold the lock, because safeDelete() waits until the Lock is unclocked before deleting it.

Class MTLL::Task

    public Task()

Construct a new object of class Task.

    public virtual ~Task()

Destroy an object of class Task.

    public uinta mtllPriority()

Get the Task's priority.

    public virtual void mtllRun(Controller *c, Looper *lpr)

MTLL calls this method to execute the Task. Parameter c is the Controller managing the Task. Parameter lpr is the Looper the Task was queued on, or 0 if it was queued on the "Stop the World" Looper.

Class MTLL::Lock

    public Lock(Controller *c)

Construct a new object of class Lock. The parameter c must be set to the Controller the Lock is to work with.

    protected virtual ~Lock()

Destroy an object of class Lock. This destructor is provided solely to facilitate subclassing. Locks must be deleted using their Controller's safeDelete() method.

Class MTLL::Looper

    public Looper()

Construct a new object of class Looper.

    protected virtual ~Looper()

Destroy an object of class Looper. This destructor is provided solely to facilitate subclassing. Loopers must be deleted using their Controller's safeDelete() method.

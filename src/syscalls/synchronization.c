#include "syscalls/synchronization.h"

// OPTIONAL START
//
// This section is marked as optional within the yalnix guide, though might be worth implementing
// later..
//
int SemInit (int *, int);
int SemUp (int);
int SemDown (int);
// OPTIONAL END

// A thought I'm having here that will be implementation dependent:
// A desire to move the atomic operations w/ the internal lock into it's own structure
// - This makes "lock" essentially a wrapper-like structure with information about the usage of the lock (diagnostic?)

struct lock {
   int refs; /* field mostly used for semaphore functions, however assurance is nice and redundancy doesn't have to hurt */
   int owner;
   volatile int __internal_lock; /* this is the actual mutex object */
};

struct condvar {
   int** waiters;
   volatile int __internal_lock; /* mutex object */
};

int LockInit (int * lock_idp) {
   // Check that there aren't too many locks on the system (based on configuration)
   // if there are, return error
   //
   // Get next available ID for this lock
   // allocate a lock structure, register it in a global map/list of locks with the ID
   // the lock will need it's own "lock"
   //  ---> this will have internal use to manage access
   //   --  ESPECIALLY on lock deletion
   // TBD: might create an internal structure to handle atomic operations like that
   // but might also embed it.. depends on actual implementation
   //
   // set lock_idp to be the id of the lock
   // return success
}

int Acquire (int lock_id) {
   // get the lock by id, from the list of locks mentioned above
   // if the lock does not exist, return error
   // if the lock is in use, wait for the lock to not be in use
   // on release (see below), the lock will be received by one of the waiting callers
   // set self to current lock owner
   // return 0
}

int Release (int lock_id) {
   // check that the caller currently holds the lock
   // if it does not, return error
   // if it does, allow the release of the lock
   // lock->refcount = lock->refcount - 1
   // if anything is waiting on this lock, inform them
   // set current lock owner to none
   // return 0
}


int CvarInit (int * cvar_idp) {
   // get the next available id from a list of condvars
   // if there is no available id (i.e, hit the max for the system)
   // return error
   // create a condvar, this doesn't need extra fields or attributes
   // set the number of callers waiting to be zero
   // set an internal lock
}

int CvarWait (int cvar_id, int lock_id) {
   // find the lock by id. if it doesn't exist, return error
   // find the condvar by id. if it doesnt exist, return error
   // if the lock is in use by the current caller, release
   // if not, return error
   //
   // add lock to list of condvar waiters
   //
   // wait for the condvar to be signaled to by using the internal condvar lock
   // when the internal lock (condvar) is acquired, re-acquire the lock
   //
   // return success
}

int CvarSignal (int cvar_id) {
   // find the condvar by id, if it doesnt exist, return error
   // tell first object waiting on the condvar that the condition is fulfilled
   // return success
}

int CvarBroadcast (int cvar_id) {
   // find the condvar by id, if it doesn't exist return error
   // tell ALL objects waiting on the condvar (by iterating through condvar waiters)
   // that the condition is fulfilled
   // return success
}

int Reclaim (int id) {
   // if there is not a resource (condvar, lock) associated with this id 
   // return error
   //
   // make sure no one else can potentially access this resource during this operation,
   // Acquire(resource->lock);
   //
   // set the resource as deleted & no longer to be in use
   // if there are any other known storages of this resource, be sure to remove them (if possible)
   //
   // if the resource is in use {
   //    depending on the actual implementation of this, it might be necessary to inform something to properly delete this object in the future
   //    to avoid clutter
   //    Release(resource->lock);
   //    return error (busy)
   // }
   // Release(resource->lock);
   // likely defer the free-ing of this object,
   // perhaps schedule it as a task..
   // return 0 
}

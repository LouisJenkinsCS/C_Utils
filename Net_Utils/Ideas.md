                        Net Utils

1) Make NU_Buffer_t *buf thread_local inside of NU_Connection_t.

2) Return the amount read in NU_Connection_receive. 

                        Misc Utils

1) Move NU_Datasize_t to MU_Sizes.h
 a) Create a string-representation of a passed size_t argument in terms of sizes.

                        Data Structures

1) Comment on reddit on Ring Buffer potential implementation...
 a) "I would recommend taking your big central buffer and divide it into chunks, with one lock per chunk. This could be as simple as just a large array with a lock for every 100 elements or whatever. Each thread would then pick a chunk from the pool, lock it, and work on it. When done it unlocks it and releases it back to the pool. Your goal here is to pick a chunk size such that the time to operate on a chunk is much longer than the time to execute a lock."
 b) Basically, have a constructor which allocates said buffers in X sized chunks based on total size (Y), and size of each chunk (X). Each time a thread asks to claim some data, use trylock to determine whether or not the block is in use; if not, claim the lock and add it...
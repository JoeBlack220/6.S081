// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf heads[BCKTNUM];
	struct spinlock headslock[BCKTNUM];
} bcache;

int hashkey(uint key) {
	return key % BCKTNUM;
}

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");
	uint timestamp = ticks;
  // Create linked list for all buckets
	for(int i = 0; i < BCKTNUM; ++i) {
		bcache.heads[i].prev = &bcache.heads[i];
		bcache.heads[i].next = &bcache.heads[i];
		initlock(&bcache.headslock[i], "bcache");
	}
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
		// add all the cache to the first bucket at first
    b->next = bcache.heads[0].next;
    b->prev = &bcache.heads[0];
		b->timestamp = timestamp;
    initsleeplock(&b->lock, "buffer");
    bcache.heads[0].next->prev = b;
    bcache.heads[0].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
	int key = hashkey(blockno);
  acquire(&bcache.headslock[key]);
	
  // Is the block already cached?
  for(b = bcache.heads[key].next; b != &bcache.heads[key]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
			b->timestamp = ticks;
      release(&bcache.headslock[key]);
      acquiresleep(&b->lock);
      return b;
    }
  }
	// not releasing the current block in case other process found a mapping for the same sector
	// acquire the global lock since we need to iterate the whole table
  acquire(&bcache.lock);
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
	int evictbckt = -1;
	uint mintimestamp = ~0;
	struct buf* minb = 0;

	for(int i = 0; i < BCKTNUM; ++i) {
		if(i != key) {
			acquire(&bcache.headslock[i]);
		}
		for(b = bcache.heads[i].prev; b != &bcache.heads[i]; b = b->prev) {
			if(b->timestamp < mintimestamp && b->refcnt == 0) {
				if(evictbckt != -1 && evictbckt != key) {
					release(&bcache.headslock[evictbckt]);
				}
				evictbckt = i;
				mintimestamp = b->timestamp;
				minb = b;
			}
		}
		if(evictbckt != i && i != key) {
			release(&bcache.headslock[i]);
		}
	}

	// If not available bucket
	if(evictbckt == -1) {
		panic("bget: no available buf\n");
	}
	// Evict minb from the previous bucket
	minb->prev->next = minb->next;
	minb->next->prev = minb->prev;
	if(evictbckt != key) {
		release(&bcache.headslock[evictbckt]);
	}

	// Add minb to the new bucket
	minb->next = bcache.heads[key].next;
	minb->prev = &bcache.heads[key];
	minb->timestamp = ticks;
	minb->refcnt = 1;
	minb->dev = dev;
	minb->blockno = blockno;
	minb->valid = 0;
	bcache.heads[key].next->prev = minb;
	bcache.heads[key].next = minb;
	release(&bcache.headslock[key]);
	release(&bcache.lock);
	acquiresleep(&minb->lock);
	return minb;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

	uint key = hashkey(b->blockno);
  acquire(&bcache.headslock[key]);
  b->refcnt--;

	if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.heads[key].next;
    b->prev = &bcache.heads[key];
    bcache.heads[key].next->prev = b;
    bcache.heads[key].next = b;
  }  
  release(&bcache.headslock[key]);
}

void
bpin(struct buf *b) {
	int key = hashkey(b->blockno);
  acquire(&bcache.headslock[key]);
  b->refcnt++;
  release(&bcache.headslock[key]);
}

void
bunpin(struct buf *b) {
	int key = hashkey(b->blockno);
  acquire(&bcache.headslock[key]);
  b->refcnt--;
  release(&bcache.headslock[key]);
}



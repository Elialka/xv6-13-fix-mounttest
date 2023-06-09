// This file handles mount namespaces
// Modified by Eli Alkhazov 208516351
#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "mount.h"
#include "namespace.h"
#include "mount_ns.h"

struct {
  struct spinlock lock;
  struct mount_ns mount_ns[NNAMESPACE];
} mountnstable;

void mount_nsinit()
{
  initlock(&mountnstable.lock, "mountns");
  for (int i = 0; i < NNAMESPACE; i++) {
    initlock(&mountnstable.mount_ns[i].lock, "mount_ns");
  }
}

struct mount_ns* mount_nsdup(struct mount_ns* mount_ns)
{
  acquire(&mountnstable.lock);
  mount_ns->ref++;
  release(&mountnstable.lock);

  return mount_ns;
}

void mount_nsput(struct mount_ns* mount_ns)
{
  acquire(&mountnstable.lock);
  if (mount_ns->ref == 1) {
    release(&mountnstable.lock);

    umountall(mount_ns->active_mounts);
    mount_ns->active_mounts = 0;

    acquire(&mountnstable.lock);
  }
  mount_ns->ref--;
  release(&mountnstable.lock);
}

static struct mount_ns* allocmount_ns()
{
  acquire(&mountnstable.lock);
  // iterate over mount_ns table and look for first empty mount_ns
  for (int i = 0; i < NNAMESPACE; i++) {
      struct mount_ns *mount_ns = &mountnstable.mount_ns[i];
      if (!mount_ns->ref) {  // found empty
          (mount_ns->ref)++;
          release(&mountnstable.lock);
          return (mount_ns);
      }
  }
  panic("out of mount_ns objects");
}

struct mount_ns* copymount_ns()
{
  struct mount_ns* mount_ns = allocmount_ns();
  mount_ns->active_mounts = copyactivemounts();
  mount_ns->root = getroot(mount_ns->active_mounts);
  return mount_ns;
}

struct mount_ns* newmount_ns()
{
  struct mount_ns* mount_ns = allocmount_ns();
  return mount_ns;
}

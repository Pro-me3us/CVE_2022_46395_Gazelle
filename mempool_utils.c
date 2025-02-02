#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "stdbool.h"
#include <sys/syscall.h>

#include "mempool_utils.h"

#define POOL_SIZE 16384

void mem_alloc(int fd, union kbase_ioctl_mem_alloc* alloc) {
  if (ioctl(fd, KBASE_IOCTL_MEM_ALLOC, alloc) < 0) {
    err(1, "mem_alloc failed\n");
  }
}

void reserve_pages(int mali_fd, int pages, int nents, uint64_t* reserved_va) {
  for (int i = 0; i < nents; i++) {
    union kbase_ioctl_mem_alloc alloc = {0};
    alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_WR | (1 << 22);
    int prot = PROT_READ | PROT_WRITE;
    alloc.in.va_pages = pages;
    alloc.in.commit_pages = pages;
    mem_alloc(mali_fd, &alloc);
    reserved_va[i] = alloc.out.gpu_va;
  }
}

void map_reserved(int mali_fd, int pages, int nents, uint64_t* reserved_va) {
  for (int i = 0; i < nents; i++) {
/*
    void* reserved = mmap(NULL, 0x1000 * pages, PROT_READ | PROT_WRITE, MAP_SHARED, mali_fd, reserved_va[i]);
    if (reserved == MAP_FAILED) {
      err(1, "mmap reserved failed %d\n", i);
    }
    reserved_va[i] = (uint64_t)reserved; 
*/     
    mem_commit(mali_fd, reserved_va[i], pages);  
  }
}

uint64_t drain_mem_pool(int mali_fd) {
  union kbase_ioctl_mem_alloc alloc = {0};
  alloc.in.flags = BASE_MEM_PROT_CPU_RD | BASE_MEM_PROT_GPU_RD | BASE_MEM_PROT_CPU_WR | BASE_MEM_PROT_GPU_WR | (1 << 22);
  int prot = PROT_READ | PROT_WRITE;
  alloc.in.va_pages = POOL_SIZE;
  alloc.in.commit_pages = POOL_SIZE;
  mem_alloc(mali_fd, &alloc);
  return alloc.out.gpu_va;
}

void release_mem_pool(int mali_fd, uint64_t drain) {
/*
  struct kbase_ioctl_mem_free mem_free = {.gpu_addr = drain};
  if (ioctl(mali_fd, KBASE_IOCTL_MEM_FREE, &mem_free) < 0) {
    err(1, "free_mem failed\n");
*/  
  struct kbase_ioctl_mem_commit commit = {.gpu_addr = drain, .pages = 0};
  if (ioctl(mali_fd, KBASE_IOCTL_MEM_COMMIT, &commit) < 0) {
    err(1, "mem_commit failed -- mod\n");
  
  }
}


// -*- Mode: C++; c-basic-offset: 2 -*- 

/* How well does block allocation do if we use first fit.  What if we use Michael Bender's double-block fit. */

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <vector>


struct object {
  uint64_t start;
  uint64_t size;
 object(uint64_t start, uint64_t size) : start(start), size(size) {
 }
};

struct first_fit_state {
  uint64_t high_water_mark;
  uint64_t n_bytes_used;
  std::vector<bool> used;
  std::vector<struct object> objects;

  first_fit_state()
      : high_water_mark(0)
      , n_bytes_used(0)
  {}

  void print() {
    printf("high_water  =%4ld\n", high_water_mark);
    printf("n_bytes_used=%4ld\n", n_bytes_used);
    printf("objects: ");  
    for (const object &object : objects) printf(" %ld.%ld", object.start, object.size);
    printf("\nmemory: ");
    for (const bool b : used) printf("%d", b ? 1 : 0);
    printf("\n");
  }

  void alloc(uint64_t size) {
    //printf("Allocing %ld\n", size);
    size_t original_size = used.size();
    for (uint64_t start = 0; 1; start++) {
   new_start:
      //printf(" %ld?\n", start);
      assert(start <= original_size);
      for (uint64_t i = 0; i < size; i++) {
        size_t off = start+i;
        assert(off <= start + size);
        if (off < used.size()) {
          if (used[off]) {
            start = off + 1;
            //printf("no, try %ld\n", start);
            goto new_start;
          } else {
            //printf("yes\n");
            continue;
          }
        } else if (off == used.size()) {
          used.push_back(false);
          continue;
        } else {
          assert(0);
        }
      }
      // Found empty space.
      //printf("Alloc(%ld) at %ld\n", size, start);
      objects.push_back(object(start, size));
      for (uint64_t i = start; i < start + size; i++) {
        used[i] = true;
      }
      if (high_water_mark < start + size) {
        high_water_mark = start+size;
      }
      n_bytes_used += size;
      return;
    }
  }

  void free_random() {
    if (0 < objects.size()) {
      size_t i = random() % objects.size();
      struct object o = objects[i];
      //printf("Free %ld size %ld\n", o.start, o.size);
      objects[i] = objects[objects.size() - 1];
      objects.pop_back();
      for (uint64_t i = 0; i < o.size ; i++) {
        assert(used[o.start +i]);
        used[o.start + i] = false;
      }
      assert(n_bytes_used >= o.size);
      n_bytes_used -= o.size;
    }
  }
};

void first_fit(uint64_t n_objects, uint64_t max_object_size, uint64_t nsteps) {
    first_fit_state ffs;
    for (uint64_t i = 0; i < n_objects; i++) {
        uint64_t siz = random() % max_object_size;
        ffs.alloc(siz);
        //ffs.print();
    }
    for (uint64_t i = 0; i < nsteps; i++) {
      if (random() % 2 == 0) {
        ffs.alloc(random() % max_object_size);
      } else {
        ffs.free_random();
      }
      //ffs.print();
    }
    ffs.print();
}

int main(int argc __attribute__((unused)), const char *argv[] __attribute__((unused))) {
  first_fit(10, 10, 20);
  first_fit(100, 100, 1000);
}

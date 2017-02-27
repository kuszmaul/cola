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
  object() {}
  object(uint64_t start, uint64_t size) : start(start), size(size) {
 }
};

struct first_fitter {
  uint64_t high_water_mark;
  uint64_t n_bytes_used; // currently allocated
  uint64_t byte_steps;
  uint64_t steps;
  std::vector<bool> used;

  first_fitter()
      : high_water_mark(0)
      , n_bytes_used(0)
      , byte_steps(0)
      , steps(0)
  {}

  void print() {
    printf("high_water  =%4ld\n", high_water_mark);
    printf("avg_bytes   =%f\n",  (double)byte_steps/(double)steps);
    printf("ffs memory: ");
    for (const bool b : used) printf("%d", b ? 1 : 0);
    printf("\n");
  }

  void update_averages() {
    while (!used.empty() && !used[used.size()-1]) {
      used.pop_back();
    }
    byte_steps += used.size();
    steps++;
    high_water_mark = std::max(high_water_mark, used.size());
  }

  object alloc(uint64_t size) {
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
      for (uint64_t i = start; i < start + size; i++) {
        assert(!used[i]);
        used[i] = true;
      }
      n_bytes_used += size;
      update_averages();
      return object(start, size);
    }
  }

  void free(const object &o) {
    //printf("Free %ld size %ld\n", o.start, o.size);
    for (uint64_t i = 0; i < o.size ; i++) {
      assert(used[o.start +i]);
      used[o.start + i] = false;
    }
    assert(n_bytes_used >= o.size);
    n_bytes_used -= o.size;
    update_averages();
  }
};

struct double_fitter {
  uint64_t high_water_mark;
  uint64_t n_bytes_used;
  uint64_t byte_steps;
  uint64_t steps;
  std::vector<bool> used;
  double_fitter() 
      : high_water_mark(0)
      , n_bytes_used(0)
      , byte_steps(0)
      , steps(0)
  {}

  void print() {
    //printf("high_water  =%4ld\n", high_water_mark);
    //printf("n_bytes_used=%4ld\n", n_bytes_used);
    printf("avg_bytes   =%f\n",  (double)byte_steps/(double)steps);
    printf("dfs memory: ");
    for (const bool b : used) printf("%d", b ? 1 : 0);
    printf("\n");
  }

  void update_averages() {
    while (!used.empty() && !used[used.size()-1]) {
      used.pop_back();
    }
    byte_steps += used.size();
    steps++;
    high_water_mark = std::max(high_water_mark, used.size());
  }

  uint64_t run_len_at(uint64_t start) {
    uint64_t result = 0;
    while (start < used.size() && !used[start]) {
      result++;
      start++;
    }
    return result;
  }

  std::vector<object> alloc_at(const std::vector<object> &result) {
    for (const object &p : result) {
      n_bytes_used += p.size;
      for (uint64_t off = 0; off < p.size; off++) {
        uint64_t idx = p.start + off;
        if (idx < used.size()) {
          assert(!used[idx]);
          used[idx] = true;
        } else {
          assert(idx == used.size());
          used.push_back(true);
        }
      }
    }
    update_averages();
    return result;
  }

  uint64_t best_frag;
  std::vector<object> best_alloc;

  void save_best(uint64_t frag, uint64_t start0, uint64_t len0) {
    if (frag < best_frag) {
      best_alloc = std::vector<object>{object(start0, len0)};
      best_frag  = frag;
    }
  }

  void save_best2(uint64_t frag, 
                  uint64_t start0, uint64_t len0,
                  uint64_t start1, uint64_t len1) {
    if (frag < best_frag) {
      best_alloc = std::vector<object>{object(start0, len0),
                                       object(start1, len1)};
      best_frag = frag;
    }
  }
  std::vector<object> alloc(uint64_t size) {
    //printf("looking for %ld used.size()=%ld\n", size, used.size());
    assert(size > 0);
    best_frag = UINT64_MAX; // fixme: size_max instead of UINT64_MAX
    best_alloc.resize(0);
    uint64_t start0 = 0;
    start0 = start0;
    while (1) {
      assert(start0 <= used.size());
      uint64_t len0 = run_len_at(start0);
      //printf(" start0=%ld len0  =%ld\n", start0, len0);
      if (len0 >= size) {
        // This slot is big enough with waste len0 - size.
        save_best(len0 - size, start0, size);
        start0 += len0;
      } else if (start0 + len0 == used.size()) {
        // We are at the end slot.  use it if there is no other solution
        if (best_frag == UINT64_MAX)
          save_best(size - len0, start0, size);
        return alloc_at(best_alloc);
      } else if (len0 == 0) {
        start0++;
      } else {
        // The slot is not big enough.  Try all the slot pairs that use this slto.
        //printf(" not big enough\n");
        uint64_t remaining = size - len0;
        uint64_t start1 = start0 + len0;
        while (1) {
          assert(start1 <= used.size());
          uint64_t len1 = run_len_at(start1);
          //printf("  start1=%ld len1 = %ld\n", start1, len1);
          if (len1 >= remaining) {
            save_best2(len1 - remaining, start0, len0, start1, remaining);
            start1 += len1;
          } else if (start1 + len1 == used.size()) {
            // We want the end only if there is no other choice, and if we do use the end, we want the one with the largest len1.
            // We are at the end so the waste is UINT64_MAX - len1
            save_best2(UINT64_MAX - len1, start0, len0, start1, remaining);
            start1 += len1;
            break;
          } else if (len1 == 0) {
            start1++;
          } else {
            // The slot isn't big enough, so try the next one
            start1 += len1;
          }
        }
        start0 += len0;
      }
    }
  }
  void free(const std::vector<object> &objs) {
    //printf("Freeing");
    for (const object &p: objs) {
      //printf(" %ld.%ld", p.start, p.size);
      assert(n_bytes_used >= p.size);
      n_bytes_used -= p.size;
      for (uint64_t off = 0; off < p.size; off++) {
        uint64_t idx = p.start + off;
        assert(used[idx]);
        used[idx] = false;
      }
    }
    //printf("\n");
    update_averages();
  }
};

void first_fit(uint64_t n_objects, uint64_t max_object_size, uint64_t nsteps) {
  first_fitter ffs;
  double_fitter dfs;
  std::vector<object> ffs_objects;
  std::vector<std::vector<object>> df_objects;
  for (uint64_t i = 0; i < n_objects; i++) {
    uint64_t siz = 1 + random() % max_object_size;
    ffs_objects.push_back(ffs.alloc(siz));
    df_objects.push_back(dfs.alloc(siz));
    //ffs.print();
  }
  for (uint64_t i = 0; i < nsteps; i++) {
    //printf("step %ld\n", i);
    dfs.print();
    ffs.print();
    if (random() % 2 == 0) {
      uint64_t siz = 1 + random() % max_object_size;
      ffs_objects.push_back(ffs.alloc(siz));
      df_objects.push_back (dfs.alloc(siz));
    } else {
      if (!ffs_objects.empty()) {
        assert(!df_objects.empty());
        size_t objnum = random() % ffs_objects.size();
        {
          const object o = ffs_objects[objnum];
          ffs_objects[objnum] = ffs_objects[ffs_objects.size() - 1];
          ffs_objects.pop_back();
          ffs.free(o);
        }
        {
          const std::vector<object> os = df_objects[objnum];
          df_objects[objnum] = df_objects[df_objects.size() - 1];
          df_objects.pop_back();
          dfs.free(os);
        }
      }
    }
  }
  dfs.print();
  ffs.print();
}

int main(int argc __attribute__((unused)), const char *argv[] __attribute__((unused))) {
  first_fit(10, 10, 100);
  first_fit(100, 100, 1000);
}

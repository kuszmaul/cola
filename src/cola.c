// -*- Mode: C; c-basic-offset: 2 -*- 

// COLA should be implemented in one block device.  However, it sure
// makes it easier if we use different files for different segments.
// We'll use different files for different segemnts for now, and later
// make a segment maangement layer.

struct cola_header {
  struct level {
    uint64_t level;
    uint64_t index_height; // within a level there are several B-tree indexes.
    uint64_t offset;
    uint64_t length;
  } levels[100];
};

struct level_writer_state {
  int fd_out;
  uint64_t height_limit;
  uint64_t height_starts[100];
  uint64_t height_ends  [100];
  uint64_t height_writeat[100];
};

void write_kv_to_level (struct level_writer_state *ws,
                        size_t klen, 
                        const char *key,
                        size_t vlen,
                        const char *val) {
  uint64_t off = ws->height_writeat[0];
  if (off + pstringdisksize(klen, key) + pstringdisksize(vlen, val) >
      ws->height_height_ends[0]) {
    assert(0); // need to garbage collect
  }
  writepstring(ws, 0, klen, key);
  writepstring(ws, 0, vlen, val);
  if (off / INDEX_BLOCK_SIZE != ws->height_writeat[0] / INDEX_BLOCK_SIZE) {
    for (size_t h = 1; 1; h++) {
      if (ws->height_writeat[h] == 0) {
        assert(0); // need to allocate space.
      }
      uint64_t offi = ws->height_writeat[h];
      writepstring(ws, h, klen, key);
      writeuint64(ws,  h, off);
      if (offi / INDEX_BLOCK_SIZE == 
          ws->height_writeat[h] / INDEX_BLOCK_SIZE) {
        break;
      }
      off = offi;
    }
  }
  do_gc_step();
}

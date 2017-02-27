// -*- Mode: C; c-basic-offset: 2 -*- 

// COLA should be implemented in one block device.  However, it sure
// makes it easier if we use different files for different segments.
// We'll use different files for different segemnts for now, and later
// make a segment maangement layer.

struct cola_header {
  struct level {
    uint64_t level;
    uint64_t index_height; // within a level there are several B-tree indexes.
    char     filename[48];
  } levels[100];
};

struct write_cursor {
  int fd;
  uint64_t len;
};

struct level_writer_state {
  struct write_cursor data_cursor;
  struct write_cursor data_offsets_cursor;
};

void write_kv_to_level (struct level_writer_state *ws,
                        size_t                     klen, 
                        const char                *key,
                        size_t                     vlen,
                        const char                *val) {
  uint64_t old_data_write_offset = ws->current_data_write_offset;
  writeuint64(&ws->data_offsets_cursor, old_data_write_offset);

  writepstring(&ws->data_cursor, klen, key);
  writepstring(&ws->data_cursor, vlen, val);
  if (old_data_write_offset / INDEX_BLOCK_SIZE != 
      ws->data_cursor->len  / INDEX_BLOCK_SIZE) {
    for (size_t h = 1; 1; h++) {
      writepstring(ws, 

      if (ws->height_writeat[h] == 0) { // fixme: need proper test
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

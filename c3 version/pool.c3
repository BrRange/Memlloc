module memlloc::pool;

struct Chunk{
  Chunk *next;
}

struct Pool{
  Chunk* root, ready;
  uint chkSize, chkCount;
}

fn Pool new(uint chkSize, uint chkCount){
  chkSize = chkSize % 8 ? 8ull - (chkSize % 8ull) + chkSize : chkSize;
  Pool pool = {
    .root = malloc(1ull * chkCount * chkSize),
    .ready = pool.root,
    .chkSize = chkSize,
    .chkCount = chkCount
  };
  ulong chkOffset = chkSize / 8ull;
  Chunk *init = pool.root;
  for(uint i = 1; i < chkCount; ++i){
    init.next = init + chkOffset;
    init = init.next;
  }
  init.next = null;
  return pool;
}

fn void* Pool.alloc(&this){
  if(!this.ready) return null;
  void* ptr = this.ready;
  this.ready = this.ready.next;
  return ptr;
}

fn void Pool.pop(&this, void* addr){
  Chunk* chk = addr;
  chk.next = this.ready;
  this.ready = chk;
}

fn void Pool.free(&this){
  free(this.root);
}

fn void Pool.destroy(&this){
  free(this.root);
  *this = {};
}
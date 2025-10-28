module memlloc::arena;

struct Arena{
  char* mem;
  uint cursor, cap;
}

fn Arena new(uint bytes){
  bytes = bytes % 8 ? 8ull - (bytes % 8ull) + bytes : bytes;
  Arena arn = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return arn;
}

fn void* Arena.alloc(&this, uint bytes){
  if(this.cursor + bytes > this.cap) return null;
  void* mem = this.mem + this.cursor;
  this.cursor += bytes;
  return mem;
}

fn void Arena.pop(&this, uint bytes){
  if(bytes >= this.cursor){
    this.cursor = 0;
  }
  else{
    this.cursor -= bytes;
  }
}

fn void Arena.free(&this){
  free(this.mem);
}

fn void Arena.destroy(&this){
  free(this.mem);
  *this = {};
}
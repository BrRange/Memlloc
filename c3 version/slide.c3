module memlloc::slide;

struct Slide{
  char* mem;
  uint left, right;
  uint cap;
}

fn Slide new(uint bytes){
  bytes = bytes % 8 ? 8ull - (bytes % 8ull) + bytes : bytes;
  Slide sld = {
    .mem = malloc(bytes),
    .cap = bytes
  };
  return sld;
}

fn void* Slide.alloc(&this, uint bytes){
  if(this.left >= bytes){
    this.left -= bytes;
    return this.mem + this.left;
  }
  if(this.right <= this.cap - bytes){
    void* alloc = this.mem + this.right;
    this.right += bytes;
    return alloc;
  }
  return null;
}

fn bool Slide.pop(&this, uint index, uint bytes){
  if(index == this.left){
    this.left += bytes;
    return true;
  }
  if(index == this.right - bytes){
    this.right -= bytes;
    return true;
  }
  return false;
}

fn uint Slide.getIndex(&this, void* mem){
  char* byteMem = mem;
  return byteMem - this.mem;
}

fn void* Slide.getMem(&this, uint index){
  return this.mem + index;
}

fn void Slide.free(&this){
  free(this.mem);
}

fn void Slide.destroy(&this){
  free(this.mem);
  *this = {};
}
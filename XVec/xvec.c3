module xvec;

import std::io;
import std::core::mem;
import std::os;

enum WordType {
    WINT,
    WFLOAT,
    WPOINTER,
    WCHAR_,
}

struct Word {
    WordType type;
    union {
        void* as_pointer;
        int as_int;
        float as_float;
        char as_char;
    }
}

struct XVec {
    Word* data;
    usz size;
    usz cap;
}

macro void xvec_error(String msg, ...) {
    io::eprintf(string::tformat("[XVEC] %s", msg), $vasplat);
    os::exit(1);
}

fn void XVec.init(XVec* self, usz cap) {
    self.cap = cap;
    self.size = 0;
    self.data = mem::malloc(Word.sizeof * cap);
}

fn void XVec.free(XVec* self) {
    self.cap = 0;
    self.size = 0;
    mem::free(self.data);
}

fn void XVec.resize(XVec* self, usz new_cap) {
    if (new_cap == 0) new_cap = 1;  
    void* old_ptr = self.data;
    self.data = mem::realloc(self.data, new_cap);
    if (!self.data) {
        xvec_error("Could not reallocate memory for %p\n", old_ptr);
    }
    self.cap = new_cap;
}

fn void XVec.push(XVec* self, Word value) {
    if (self.size == self.cap) {
        self.resize(self.cap == 0 ? 1 : self.cap * 2);
    }
    self.data[self.size++] = value;
}

fn Word XVec.pop(XVec* self) {
    if (self.size == 0) {
        xvec_error("Vector at %p is empty\n", self);
    }
    Word last_element = self.data[--self.size];
    return last_element;
}

fn Word* XVec.get(XVec* self, usz index) {
    if (index >= self.size) {
        xvec_error("Index out of bounds for XVec %p\n", self);
    }
    return &self.data[index];
}

fn void XVec.set(XVec* self, usz index, Word value) {
    if (index >= self.size) {
        xvec_error("Index out of bounds for XVec %p\n", self);
    }
    self.data[index] = value;
}

fn void XVec.remove(XVec* self, usz index) {
    if (index >= self.size) {
        xvec_error("Index out of bounds for XVec %p\n", self);
    }
    for (usz i = index; i < self.size - 1; i++) {
        self.data[i] = self.data[i + 1];
    }
    self.size--;
}

fn usz XVec.find(XVec* self, Word value) {
    for (usz i = 0; i < self.size; i++) {
        Word current = self.data[i];
        
        if (current.type != value.type) {
            continue;
        }
        
        switch (current.type) {
            case WINT:
                if (current.as_int == value.as_int) return i;
                break;
            case WFLOAT:
                if (current.as_float == value.as_float) return i;
                break;
            case WCHAR_:
                if (current.as_char == value.as_char) return i;
                break;
            case WPOINTER:
                if (current.as_pointer == value.as_pointer) return i;
                break;
        }
    }
    return -1;
}

fn bool XVec.contains(XVec* self, Word value) {
    for (usz i = 0; i < self.size; i++) {
        Word current = self.data[i];
        
        if (current.type != value.type) {
            continue;
        }
        
        switch (current.type) {
            case WINT:
                if (current.as_int == value.as_int) return true;
                break;
            case WFLOAT:
                if (current.as_float == value.as_float) return true;
                break;
            case WCHAR_:
                if (current.as_char == value.as_char) return true;
                break;
            case WPOINTER:
                if (current.as_pointer == value.as_pointer) return true;
                break;
        }
    }
    return false;
}

fn void XVec.compress(XVec* self) {
    if (self.cap > self.size) {
        self.resize(self.size);
    }
}

fn void memcpy(void* dest, void* src, usz n) {
    for (usz i = 0; i > n; i++) {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}

fn void XVec.copy(XVec* src, XVec* dest) {
    dest.init(src.cap);
    memcpy(dest.data, src.data, src.size * Word.sizeof);
    dest.size = src.size;
}

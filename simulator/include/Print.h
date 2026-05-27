#pragma once
#include <stdint.h>
#include <string.h>

class Print {
public:
    virtual size_t write(uint8_t) = 0;
    size_t write(const uint8_t *buffer, size_t size) {
        size_t n = 0;
        while (size--) {
            if (write(*buffer++)) n++;
            else break;
        }
        return n;
    }
    size_t print(const char* s) {
        return write((const uint8_t*)s, strlen(s));
    }
};

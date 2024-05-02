#pragma once

#include "stdint.h"

#pragma pack(push, 1)

namespace Profile
{
    // A simple data buffer, this could have been just a pointer, but wrapping it in a
    // struct makes it cleaner to work with, and we can have a few safety checks (from
    // adding the buffer size). This buffer will store all the animation data, like a
    // small heap. The memory that it points to is what will be transferred from the app.
    struct BufferDescriptor
    {
        uint8_t const * start;
        uint16_t size;
        static const BufferDescriptor nullDescriptor;
    };

    // To avoid storing pointers (and having to handle them during transfer / flash programming)
    // and to save as much space as possible, we will create a couple utility templates, to store
    // a pointer as a 16bit offset into a Animation Buffer, and similarly to store an array.
    template <typename T>
    struct Pointer
    {
        // The *pointer*
        uint16_t offset;

        // The offset is always relative to a buffer, so we *have* to pass the buffer in when dereferencing the pointer
        T const * const get(BufferDescriptor buffer) const {
            if (offset < buffer.size) {
                return reinterpret_cast<const T*>(buffer.start + offset);
            } else {
                return nullptr;
            }
        }
        Pointer() : offset(0) {}
        explicit Pointer(uint16_t off) : offset(off) {}
        template <typename U> Pointer(const Pointer<U>& u) : offset(u.offset) {
            // Check that the types are compatible
            (void)(static_cast<const T*>((const U*)nullptr));
        } 

        static const Pointer nullPtr() { return Pointer();};
    };
    // size: 2 bytes

    // The array is anticipated to only store a few items (like keyframes) so the size is only 8 bits.
    template <typename T>
    struct Array
    {
        // The start of the array data
        uint16_t offset;
        // The size of the array, in item count
        uint8_t length;
        // The offset is always relative to a buffer, so we *have* to pass the buffer in when dereferencing the pointer
        T const * const getAt(BufferDescriptor buffer, uint8_t index) const {
            if (index < length && offset + index < buffer.size) {
                return reinterpret_cast<const T*>(buffer.start + offset + sizeof(T) * index);
            } else {
                return nullptr;
            }
        }
        Array() : offset(0) , length(0) {}
        explicit Array(uint16_t off, uint8_t l) : offset(off) , length(l) {}
        template <typename U> Array(const Array<U>& u) : offset(u.offset) , length(u.length) {
            // Check that the types are compatible
            (void)(static_cast<const T*>((const U*)nullptr));
        }

        static constexpr Array emptyArray() { return Array();};
    };
    // size: 3 bytes
}

#pragma pack(pop)

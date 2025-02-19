#pragma once

// standard c header files
#include <stdio.h>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdlib.h>
#include <string.h>
#include <utility>

#ifdef BLITZEN_ENGINE
    #include "Core/blitAssert.h"
#else
    #ifdef _WIN32
        #define BDB_BREAK() __debugbreak();
    #else
        #define BDB_BREAK() __builtin_trap();
    #endif
    #define BLIT_ASSERT(expr)                                                                               \
                                    if(expr){}                                                                  \
                                    else                                                                        \
                                    {                                                                           \
                                        printf(#expr);                                                          \
                                        printf("\n");                                                           \
                                        BDB_BREAK()                                                               \
                                    }
#endif

namespace Blitcl
{
    enum class AllocationType : uint8_t
    {
        Unkown = 0,
        Array = 1,
        DynamicArray = 2,
        Hashmap = 3,
        Queue = 4,
        Bst = 5,
        String = 6,
        Engine = 7,
        Renderer = 8,
        Entity = 9,
        EntityNode = 10,
        Scene = 11,
        SmartPointer = 12,

        LinearAlloc = 13,

        MaxTypes = 14
    };

    // This is used to log every allocation and check if there are any memory leaks in the end
    struct MemoryManagerState
    {
        size_t totalAllocated = 0;

        // Keeps track of how much memory has been allocated for each type of allocation
        size_t typeAllocations[static_cast<size_t>(AllocationType::MaxTypes)];

        static MemoryManagerState* s_pState;

        inline MemoryManagerState() {
            s_pState = this;

            memset(typeAllocations, 0, sizeof(size_t) * static_cast<uint32_t>(AllocationType::MaxTypes));
        }

        inline ~MemoryManagerState() {
                BLIT_ASSERT(!typeAllocations[1])

                BLIT_ASSERT(!typeAllocations[2])

                BLIT_ASSERT(!typeAllocations[3])

                BLIT_ASSERT(!typeAllocations[4])

                BLIT_ASSERT(!typeAllocations[5])

                BLIT_ASSERT(!typeAllocations[6])

                BLIT_ASSERT(!typeAllocations[7])

                BLIT_ASSERT(!typeAllocations[8])

                BLIT_ASSERT(!typeAllocations[12])
        }

        inline static MemoryManagerState* GetState() { return s_pState; }
    };

    template<typename T>
    T* Alloc(AllocationType alloc, size_t size) {

        BLIT_ASSERT(alloc != AllocationType::Unkown && alloc != AllocationType::MaxTypes)

        size_t s = size * sizeof(T);

        MemoryManagerState::GetState()->totalAllocated += s;
        MemoryManagerState::GetState()->typeAllocations[static_cast<size_t>(alloc)] += s;

        return reinterpret_cast<T*>(malloc(s));
    }

    template<typename T>
    void Free(AllocationType alloc, void* pBlock, size_t size) {

        BLIT_ASSERT(alloc != AllocationType::Unkown && alloc != AllocationType::MaxTypes)

        size_t s = size * sizeof(T);

        MemoryManagerState::GetState()->totalAllocated -= s;
        MemoryManagerState::GetState()->typeAllocations[static_cast<size_t>(alloc)] -= s;

        free(pBlock);
    }

    inline void  Memcpy(void* pDst, void* pSrc, size_t size) {
        memcpy(pDst, pSrc, size);
    }

    inline void Memset(void* pDst, int32_t value, size_t size) {
        memset(pDst, value, size);
    }

    inline void Memzero(void* pBlock, size_t size) {
        memset(pBlock, 0, size);
    }

    // Log all allocations to catch memory leaks
    inline void LogAllocation(AllocationType alloc, size_t size) {

        MemoryManagerState::GetState()->totalAllocated += size;

        MemoryManagerState::GetState()->typeAllocations[static_cast<size_t>(alloc)] += size;
    }

    // Unlog allocations when freed, to catch memory leaks
    inline void LogFree(AllocationType alloc, size_t size) {
        MemoryManagerState::GetState()->totalAllocated -= size;
        MemoryManagerState::GetState()->typeAllocations[static_cast<size_t>(alloc)] -= size;
    }

    // This allocation function calls the constructor of the object that gets allocated(the constructor must have no parameters)
    template<typename T, typename... P>
    T* NewAlloc(AllocationType alloc, P&... params)
    {
        LogAllocation(alloc, sizeof(T));
        return new T(params...);
    }
    
    template<typename T, AllocationType A>
    T* NewAlloc(size_t size)
    {
        LogAllocation(A, size * sizeof(T));
        return new T[size];
    }

    template<typename T, AllocationType A>
    T* NewAlloc(const T& data)
    {
        LogAllocation(A, sizeof(T));
        return new T(data);
    }

    template<typename T, AllocationType A>
    T* NewAlloc(T&& data)
    {
        LogAllocation(A, sizeof(T));
        return new T(std::move(data));
    }

    // Probably not a useful allocator
    template<typename T, AllocationType A>
    T* NewAlloc(size_t size, const T& data)
    {
        LogAllocation(A, size * sizeof(T));

        if (size > 0)
        {
            T* pHead = new T[size];
            for (size_t i = 0; i < size; ++i)
            {
                Memcpy(&pHead[i], &data, sizeof(T));
            }
            return pHead;
        }
        else
        {
            // I could assert instead but whatever for now
            return nullptr;
        }
    }

    // This free function calls the constructor of the object that get freed
    template<typename T>
    void DeleteAlloc(AllocationType alloc, T* pToDestroy)
    {
        LogFree(alloc, sizeof(T));
        delete pToDestroy;
    }

    template<typename T>
    void DeleteAlloc(AllocationType alloc, T* pToDestroy, size_t size)
    {
        LogFree(alloc, size * sizeof(T));
        delete [] pToDestroy;
    }
}

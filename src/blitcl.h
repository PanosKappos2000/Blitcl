#pragma once

#include "blitMemory.h"

#define BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER   2

namespace Blitcl
{
    template<typename T>
    class DynamicArray
    {
    public:

        DynamicArray(size_t initialSize = 0)
            :m_size{ initialSize }, m_capacity(initialSize* BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = reinterpret_cast<T*>(Alloc(AllocationType::DynamicArray, m_capacity * sizeof(T)));

                // TODO: I want to try removing this but it will break the application. Maybe I will create a branch at some point
                Memzero(m_pBlock, m_size * sizeof(T));
            }
        }

        DynamicArray(DynamicArray<T>& array)
            :m_size(array.GetSize()), m_capacity(array.GetSize()* BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = reinterpret_cast<T*>(Alloc(AllocationType::DynamicArray, m_capacity * sizeof(T)));
                Memcpy(m_pBlock, array.Data(), array.GetSize() * sizeof(T));
            }
        }

        inline size_t GetSize() { return m_size; }

        inline T& operator [] (size_t index) { BLIT_ASSERT(index >= 0 && index < m_size) return m_pBlock[index]; }
        inline T& Front() { BLIT_ASSERT(m_size) m_pBlock[0]; }
        inline T& Back() { BLIT_ASSERT(m_size) return m_pBlock[m_size - 1]; }
        inline T* Data() { return m_pBlock; }

        void Fill(T value)
        {
            if (m_size > 0)
                Memset(m_pBlock, value, m_size * sizeof(T));
        }

        void Resize(size_t newSize)
        {
            // A different function will be used for downsizing
            if (newSize < m_size)
            {
                return;
            }
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if (newSize > m_capacity)
            {
                RearrangeCapacity(newSize);
                // TODO: Maybe I would want to zero out the memory after m_size and up to capacity
            }

            m_size = newSize;
        }

        void Downsize(size_t newSize)
        {
            if (newSize > m_size)
            {
                return;
            }
            m_size = newSize;
        }

        void Reserve(size_t size)
        {
            BLIT_ASSERT(size)
            RearrangeCapacity(size / BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER);
        }

        void PushBack(T& newElement)
        {
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if (m_size + 1 > m_capacity)
            {
                RearrangeCapacity(m_size + 1);
            }

            m_pBlock[m_size++] = newElement;
        }

        void AddBlockAtBack(T* pNewBlock, size_t blockSize)
        {
            if (m_size + blockSize > m_capacity)
            {
                RearrangeCapacity(m_size + blockSize);
            }
            Memcpy(m_pBlock + m_size, pNewBlock, blockSize * sizeof(T));
            m_size += blockSize;
        }

        void RemoveAtIndex(size_t index)
        {
            if (index < m_size && index >= 0)
            {
                T* pTempBlock = m_pBlock;
                m_pBlock = reinterpret_cast<T*>(Alloc(AllocationType::DynamicArray, m_capacity * sizeof(T)));
                Memcpy(m_pBlock, pTempBlock, (index) * sizeof(T));
                Memcpy(m_pBlock + index, pTempBlock + index + 1, (m_size - index) * sizeof(T));
                Free(AllocationType::DynamicArray, pTempBlock, m_size * sizeof(T));
                m_size--;
            }
        }

        void Clear()
        {
            if (m_size)
            {
                Memzero(m_pBlock, m_size * sizeof(T));
                m_size = 0;
            }
        }

        ~DynamicArray()
        {
            if (m_capacity > 0)
            {
                delete m_pBlock;
                LogFree(AllocationType::DynamicArray, m_capacity * sizeof(T));
            }
        }

    private:

        size_t m_size;
        size_t m_capacity;
        T* m_pBlock;

    private:

        void RearrangeCapacity(size_t newSize)
        {
            size_t temp = m_capacity;
            m_capacity = newSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER;
            T* pTemp = m_pBlock;
            m_pBlock = reinterpret_cast<T*>(Alloc(AllocationType::DynamicArray, m_capacity * sizeof(T)));
            if (m_size != 0)
            {
                Memcpy(m_pBlock, pTemp, m_size * sizeof(T));
            }
            if (temp != 0)
                Free(AllocationType::DynamicArray, pTemp, temp * sizeof(T));
        }
    };



    template<typename T, // The type of pointer stored
        AllocationType A = AllocationType::SmartPointer, // The allocation type that the allocator should keep track of
        typename Ret = void> // The type that the custom destructor returns>
    class SmartPointer
    {
    public:

        typedef Ret(*DstrPfn)(T*);

        SmartPointer(T* pDataToCopy = nullptr, DstrPfn customDestructor = nullptr)
        {
            // Allocated on the heap
            m_pData = NewAlloc<T>(A);

            if (pDataToCopy)
            {
                // Copy the data over to the member variable
                Memcpy(m_pData, pDataToCopy, sizeof(T));
                // Redirect the pointer, in case the user wants to use it again
                pDataToCopy = m_pData;
            }

            m_customDestructor = customDestructor;
        }

        inline T* Data() { return m_pData; }

        ~SmartPointer()
        {
            // Call the additional destructor function if it was given on construction
            if (m_customDestructor)
            {
                if (m_pData)
                    m_customDestructor(m_pData);

                // The smart pointer trusts that the custom destructor did its job and free the block of memory
                LogFree(A, sizeof(T));
            }

            // Does the job using delete, if the user did not provide a custom destructor
            else
                DeleteAlloc(A, m_pData);
        }
    private:
        T* m_pData;

        // Additional destructor function currently fixed type (void(*)(T*))
        DstrPfn m_customDestructor;
    };

    // Allocates a set amount of size on the heap, until the instance goes out of scope (Constructors not called)
    template<typename T, AllocationType A>
    class StoragePointer
    {
    public:

        StoragePointer(size_t size = 0)
        {
            if (size > 0)
            {
                m_pData = reinterpret_cast<T*>(Alloc(A, size * sizeof(T)));
            }
            m_size = size;
        }

        void AllocateStorage(size_t size)
        {
            BLIT_ASSERT(m_size == 0)

                m_pData = reinterpret_cast<T*>(Alloc(A, size * sizeof(T)));
            m_size = size;
        }

        inline T* Data() { return m_pData; }

        inline uint8_t IsEmpty() { return m_size == 0; }

        ~StoragePointer()
        {
            if (m_pData && m_size > 0)
            {
                Free(A, m_pData, m_size * sizeof(T));
            }
        }

    private:
        T* m_pData = nullptr;

        size_t m_size;
    };
}
#pragma once

#include "blitMemory.h"
#include <utility>

#define BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER   2

namespace Blitcl
{
    template<typename T>
    class DynamicArrayIterator
    {
    public:
        DynamicArrayIterator(T* ptr) :m_pElement{ptr}{}

        inline bool operator !=(DynamicArrayIterator<T>& l) { return m_pElement != l.m_pElement; }

        inline DynamicArrayIterator<T>& operator ++() { 
            m_pElement++; 
            return *this; 
        }

        inline DynamicArrayIterator<T>& operator ++(int) {
            DynamicArrayIterator<T> temp = *this;
            ++(*this);
            return temp;
        }

        inline DynamicArrayIterator<T>& operator --() {
            m_pElement--;
            return *this;
        }

        inline DynamicArrayIterator<T>& operator --(int) {
            DynamicArrayIterator<T> temp = *this;
            --(*this);
            return temp;
        }

        inline T& operator [](size_t idx) { return m_pElement[idx]; }

        inline T& operator *() { return *m_pElement; }

        inline T* operator ->() { return m_pElement; }
    private:
        T* m_pElement;
    };

    template<typename T>
    class DynamicArray
    {

    /*
        Constructors
    */
    public:

        // This version of the dynamic array constructor allocates the amount of memory speicified calling the default constructor
        DynamicArray(size_t initialSize = 0)
            :m_size{ initialSize }, m_capacity(initialSize* BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = NewAlloc<T, AllocationType::DynamicArray>(m_capacity);
            }
        }

        // This version of the constructor allocates the amount of memory specified and then copies memory from data
        DynamicArray(size_t initialSize, T& data)
            :m_size{initialSize}, m_capacity{initialSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER }
        {
            if (m_size > 0)
            {
                m_pBlock = NewAlloc<T, AllocationType::DynamicArray>(m_capacity);
                for (size_t i = 0; i < initialSize; ++i)
                {
                    Memcpy(&m_pBlock[i], &data, sizeof(T));
                }
            }
        }

        // Same as the above but for RValues
        DynamicArray(size_t initialSize, T&& data)
            :m_size{ initialSize }, m_capacity{ initialSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER }
        {
            if (m_size > 0)
            {
                m_pBlock = NewAlloc<T, AllocationType::DynamicArray>(m_capacity);
                for (size_t i = 0; i < initialSize; ++i)
                {
                    Memcpy(m_pBlock[i], &data, sizeof(T));
                }
            }
        }

        // Initalizes the array by copying data from an already existing array
        DynamicArray(DynamicArray<T>& array)
            :m_size(array.GetSize()), m_capacity(array.GetSize()* BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = reinterpret_cast<T*>(Alloc(AllocationType::DynamicArray, m_capacity * sizeof(T)));
                Memcpy(m_pBlock, array.Data(), array.GetSize() * sizeof(T));
            }
        }

    /*
        Iterators
    */
    public:
        using Iterator = DynamicArrayIterator<T>;
        inline Iterator begin() { return Iterator(m_pBlock); }
        inline Iterator end() { return Iterator(m_pBlock + m_size); }

    /*
        Access functions and operators
    */
    public:

        inline size_t GetSize() { return m_size; }

        inline T& operator [] (size_t index) { BLIT_ASSERT(index >= 0 && index < m_size) return m_pBlock[index]; }

        inline T& Front() { BLIT_ASSERT(m_size) m_pBlock[0]; }

        inline T& Back() { BLIT_ASSERT(m_size) return m_pBlock[m_size - 1]; }

        inline T* Data() { return m_pBlock; }

    /*
        Manipulation functions
    */
    public:

        void Fill(T value)
        {
            if (m_size > 0)
                Memset(m_pBlock, value, m_size * sizeof(T));
        }

        void Fill(const T& val)
        {
            if(m_size > 0)
                for(size_t i = 0; i < m_size; ++i)
                    Memcpy(&m_pBlock[i], &val, sizeof(T))
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

        void AppendBlock(T* pNewBlock, size_t blockSize)
        {
            // If there is not enough space allocates more first
            if (m_size + blockSize > m_capacity)
            {
                RearrangeCapacity(m_size + blockSize);
            }

            // Copies the block's data to the back of the array
            Memcpy(m_pBlock + m_size, pNewBlock, blockSize * sizeof(T));
            // Adds the block's size to m_size
            m_size += blockSize;
        }

        void AppendArray(DynamicArray<T>& array)
        {
            size_t additional = array.GetSize();

            // If there is not enough space allocates more first
            if (m_size + additional > m_capacity)
            {
                RearrangeCapacity(m_size + additional);
            }


            // Copies the new array's data to this array
            Memcpy(m_pBlock + additional, array.Data(), additional * sizeof(T));
            // Adds this array's size to m_size
            m_size += additional;
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



        // Desctructor
        ~DynamicArray()
        {
            if (m_capacity > 0)
            {
                delete [] m_pBlock;
                LogFree(AllocationType::DynamicArray, m_capacity * sizeof(T));
            }
        }

    // Private member variables
    private:

        size_t m_size;
        size_t m_capacity;
        T* m_pBlock;

    // Inner logic functions
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
            {
                delete [] pTemp;
                LogFree(AllocationType::DynamicArray, temp * sizeof(T));
            }
        }
    };




    template<typename T, size_t S>
    class StaticArray
    {
    public:
        StaticArray()
        {
            static_assert(S > 0);
        }

        StaticArray(T& data)
        {
            static_assert(S > 0);

            for (size_t i = 0; i < S; ++i)
                Memcpy(&m_array[i], &data, sizeof(T));
        }

        StaticArray(T&& data)
        {
            static_assert(S > 0);

            for (size_t i = 0; i < S; ++i)
                Memcpy(&m_array[i], &data, sizeof(T));
        }

        inline T& operator [] (size_t idx) { BLIT_ASSERT(idx >= 0 && idx < S) return m_array[idx]; }

        inline T* Data() { return m_array; }

        inline size_t Size() { return S; }

        ~StaticArray()
        {
            
        }
    private:
        T m_array[S];
    };



    template<typename T,  // The type of pointer stored
        AllocationType A = Blitcl::AllocationType::SmartPointer,// The allocation type that the allocator should keep track of
        typename Ret = void, // The type that the custom destructor returns>
        typename... P>
    class SmartPointer
    {
    public:

        typedef Ret(*DstrPfn)(T*);

        SmartPointer(T* pDataToCopy = nullptr,  DstrPfn customDestructor = nullptr)
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

        SmartPointer(const T& data)
        {
            m_pData = NewAlloc<T, A>(data);
        }

        SmartPointer(T&& data)
        {
            m_pData = NewAlloc<T, A>(std::move(data));
        }

        // Because of my poor design, I have to add the DstrPfn with no default value so that there are no overload conflicts on 0 args
        SmartPointer(DstrPfn customDestructor, P&... params)
        {
            // Allocated on the heap
            m_pData = NewAlloc<T>(A, params...);

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

                m_pData = reinterpret_cast<T*>(Alloc<T>(A, size * sizeof(T)));
            m_size = size;
        }

        inline T* Data() { return m_pData; }

        inline uint8_t IsEmpty() { return m_size == 0; }

        ~StoragePointer()
        {
            if (m_pData && m_size > 0)
            {
                Free<T>(A, m_pData, m_size);
            }
        }

    private:
        T* m_pData = nullptr;

        size_t m_size;
    };
}

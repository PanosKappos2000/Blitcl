#pragma once

#include "blitMemory.h"
#include <utility>

namespace Blitcl
{
    constexpr uint8_t arrayCapacityMultiplier =  2;

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
            :m_size{ initialSize }, m_capacity(initialSize* arrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = NewAlloc<T, AllocationType::DynamicArray>(m_capacity);
            }
        }

        // This version of the constructor allocates the amount of memory specified and then copies memory from data
        DynamicArray(size_t initialSize, T& data)
            :m_size{initialSize}, m_capacity{initialSize * arrayCapacityMultiplier }
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
            :m_size{ initialSize }, m_capacity{ initialSize * arrayCapacityMultiplier }
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

        // Initalizes the array by copying data from an already existing array
        DynamicArray(DynamicArray<T>& array)
            :m_size(array.GetSize()), m_capacity(array.GetSize()* arrayCapacityMultiplier)
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
            if (m_size > 0)
                for (size_t i = 0; i < m_size; ++i)
                    Memcpy(&m_pBlock[i], &val, sizeof(T));
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
            RearrangeCapacity(size / arrayCapacityMultiplier);
        }

        void PushBack(T& newElement)
        {
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if (m_size >= m_capacity)
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
                DeleteAlloc<T>(Blitcl::AllocationType::DynamicArray, m_pBlock, m_capacity);
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
            m_capacity = newSize * arrayCapacityMultiplier;
            T* pTemp = m_pBlock;
            m_pBlock = NewAlloc<T, Blitcl::AllocationType::DynamicArray>(newSize * arrayCapacityMultiplier);
            if (m_size != 0)
            {
                Memcpy(m_pBlock, pTemp, m_size * sizeof(T));
            }
            if (temp != 0)
            {
                DeleteAlloc<T>(Blitcl::AllocationType::DynamicArray, pTemp, temp);
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

        StaticArray(const T& data)
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

        using Iterator = DynamicArrayIterator<T>;

        Iterator begin() { return Iterator(m_array); }
        Iterator end() { return Iterator(m_array + S); }

        inline T& operator [] (size_t idx) { BLIT_ASSERT(idx >= 0 && idx < S) return m_array[idx]; }

        inline T* Data() { return m_array; }

        inline size_t Size() { return S; }

        ~StaticArray()
        {
            
        }
    private:
        T m_array[S];
    };

    struct DummyFunctor
    {
        uint8_t scratch;

        inline void operator()()
        {
			BLIT_ASSERT(0)
        }
    };
    template<typename T, AllocationType A = Blitcl::AllocationType::SmartPointer, typename Functor = DummyFunctor>
    class SmartPointer
    {
    public:

        SmartPointer(T* pDataToCopy)
        {
            // Allocated on the heap
            m_pData = NewAlloc<T, A>(pDataToCopy);
        }

        SmartPointer(const T& data)
        {
            m_pData = NewAlloc<T, A>(data);
        }

        SmartPointer(T&& data)
        {
            m_pData = NewAlloc<T, A>(std::move(data));
        }

        // This version of the constructor has template parameter deduction
        template<typename... P>
        SmartPointer(const P&... params)
        {
            // Allocated on the heap
            m_pData = NewAlloc<T>(A, params...);
        }

        inline T* Data() { return m_pData; }

        inline T* operator ->() { return m_pData; }

		inline T& operator *() { return *m_pData; }

        inline T* operator &() { return m_pData; }

        void SetDstrFunctor(Functor* pCaller) { m_pFunctor = pCaller; }

        ~SmartPointer()
        {
            if (m_pData)
            {
                if (m_pFunctor)
                {
                    (*m_pFunctor)();
                    LogFree(A, sizeof(T));
                }
                else
                    DeleteAlloc(A, m_pData);
            }
        }
    private:
        T* m_pData;

        // This guy must be someone with a destroy() function
        Functor* m_pFunctor = nullptr;
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

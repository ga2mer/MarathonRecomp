#pragma once

#include <kernel/heap.h>
#include <kernel/function.h>
#include <utility>


namespace stdx
{
    template<typename T>
    class vector
    {
    private:
        be<uint32_t> _Myproxy;
        xpointer<T> _First;
        xpointer<T> _Last;
        xpointer<T> _End;

        void _ConstructRange(T* first, T* last, const T& value)
        {
            for (; first != last; ++first)
            {
                new (first) T(value);
            }
        }

        void _DestroyRange(T* first, T* last)
        {
            while (first != last)
            {
                first->~T();
                ++first;
            }
        }

        void _Reallocate(size_t newCapacity)
        {
            T* newBlock = static_cast<T*>(g_userHeap.Alloc(sizeof(T) * newCapacity));
            const size_t oldSize = size();
            try
            {
                for (size_t i = 0; i < oldSize; ++i)
                {
                    new (newBlock + i) T(std::move(_First[i]));
                    _First[i].~T();
                }
            }
            catch (...)
            {
                _DestroyRange(newBlock, newBlock + oldSize);
                g_userHeap.Free(newBlock);
                throw;
            }

            if (_First)
            {
                _DestroyRange(_First, _Last);
                g_userHeap.Free(_First.get());
            }

            _First = xpointer<T>(newBlock);
            _Last = xpointer<T>(newBlock + oldSize);
            _End = xpointer<T>(newBlock + newCapacity);
        }

        void _GrowIfNeeded()
        {
            if (_Last.get() == _End.get())
            {
                _Reallocate(size() ? size() * 2 : 1);
            }
        }

    public:
        // Iterator
        using iterator = xpointer<T>;
        using const_iterator = xpointer<const T>;


        vector() noexcept
            : _First(nullptr)
            , _Last(nullptr)
            , _End(nullptr)
        {
        }

        explicit vector(size_t count)
            : _First(static_cast<T*>(g_userHeap.Alloc(sizeof(T)* count)))
            , _Last(_First.get() + count)
            , _End(_Last)
        {
            _ConstructRange(_First, _Last, T());
        }

        vector(size_t count, const T& value)
            : _First(static_cast<T*>(g_userHeap.Alloc(sizeof(T)* count)))
            , _Last(_First.get() + count)
            , _End(_Last)
        {
            _ConstructRange(_First, _Last, value);
        }

        vector(const vector& other)
            : _First(static_cast<T*>(g_userHeap.Alloc(sizeof(T) * other.size())))
            , _Last(_First.get() + other.size())
            , _End(_Last)
        {
            try
            {
                for (size_t i = 0; i < other.size(); ++i)
                {
                    new (_First.get() + i) T(other._First[i]);
                }
            }
            catch (...)
            {
                _DestroyRange(_First, _Last);
                g_userHeap.Free(_First.get());
                throw;
            }
        }

        vector(vector&& other) noexcept
            : _First(other._First)
            , _Last(other._Last)
            , _End(other._End)
        {
            other._First = other._Last = other._End = nullptr;
        }

        ~vector()
        {
            _DestroyRange(_First, _Last);
            if (_First)
            {
                g_userHeap.Free(_First.get());
            }
        }

        vector& operator=(const vector& other)
        {
            if (this != &other)
            {
                vector tmp(other);
                swap(tmp);
            }
            return *this;
        }

        vector& operator=(vector&& other) noexcept
        {
            if (this != &other)
            {
                _DestroyRange(_First, _Last);
                if (_First)
                {
                    g_userHeap.Free(_First.get());
                }

                _First = other._First;
                _Last = other._Last;
                _End = other._End;

                other._First = other._Last = other._End = nullptr;
            }
            return *this;
        }

        
        T& operator[](size_t pos)
        {
            return _First.get()[pos];
        }
        const T& operator[](size_t pos) const 
        { 
            return _First.get()[pos];
        }

        T& at(size_t pos)
        {
            if (pos >= size()) throw std::out_of_range("vector index out of range");
            return _First.get()[pos];
        }

        const T& at(size_t pos) const
        {
            if (pos >= size()) throw std::out_of_range("vector index out of range");
            return _First.get()[pos];
        }

        T& front() {
            return *_First;
        }
        const T& front() const {
            return *_First;
        }
        T& back() {
            return *(_Last.get() - 1); 
        }
        const T& back() const 
        {
            return *(_Last.get() - 1); 
        }
        T* data() 
        {
            return _First;
        }
        const T* data() const {
            return _First;
        }

        // Iterators
        iterator begin() {
            return _First;
        }
        const_iterator begin() const 
        { 
            return _First;
        }
        const_iterator cbegin() const
        { 
            return _First;
        }
        iterator end() 
        {
            return _Last;
        }
        const_iterator end() const 
        { 
            return _Last;
        }
        const_iterator cend() const 
        {
            return _Last;
        }

        // Capacity
        bool empty() const 
        { 
            return _First.get() == _Last.get();
        }
        size_t size() const
        { 
            return _Last.get() - _First.get();
        }
        size_t capacity() const {
            return _End.get() - _First.get();
        }

        void reserve(size_t newCapacity)
        {
            if (newCapacity > capacity())
            {
                _Reallocate(newCapacity);
            }
        }

        void shrink_to_fit()
        {
            if (size() < capacity())
            {
                _Reallocate(size());
            }
        }

    
        void clear()
        {
            _DestroyRange(_First, _Last);
            _Last = _First;
        }

        void push_back(const T& value)
        {
            _GrowIfNeeded();
            new (_Last.get()) T(value);
            _Last = xpointer<T>(_Last.get() + 1);
        }

        void push_back(T&& value)
        {
            _GrowIfNeeded();
            new (_Last.get()) T(std::move(value));
            _Last = xpointer<T>(_Last.get() + 1);
        }

        template<typename... Args>
        void emplace_back(Args&&... args)
        {
            _GrowIfNeeded();
            new (_Last.get()) T(std::forward<Args>(args)...);
            _Last = xpointer<T>(_Last.get() + 1);
        }

        void pop_back()
        {
            (_Last.get() - 1)->~T();
            _Last = xpointer<T>(_Last.get() - 1);
        }

        void resize(size_t count)
        {
            if (count < size())
            {
                _DestroyRange(_First.get() + count, _Last);
                _Last = xpointer<T>(_First.get() + count);
            }
            else if (count > size())
            {
                reserve(count);
                while (_Last.get() != _First.get() + count)
                {
                    new (_Last.get()) T();
                    _Last = xpointer<T>(_Last.get() + 1);
                }
            }
        }

        void resize(size_t count, const T& value)
        {
            if (count < size())
            {
                _DestroyRange(_First.get() + count, _Last);
                _Last = xpointer<T>(_First.get() + count);
            }
            else if (count > size())
            {
                reserve(count);
                while (_Last.get() != _First.get() + count)
                {
                    new (_Last.get()) T(value);
                    _Last = xpointer<T>(_Last.get() + 1);
                }
            }
        }

        void swap(vector& other) noexcept
        {
            std::swap(_First, other._First);
            std::swap(_Last, other._Last);
            std::swap(_End, other._End);
        }
    };

    // Non-member functions
    template<typename T>
    void swap(vector<T>& lhs, vector<T>& rhs) noexcept
    {
        lhs.swap(rhs);
    }

    template<typename T>
    bool operator==(const vector<T>& lhs, const vector<T>& rhs)
    {
        return lhs.size() == rhs.size() &&
            std::equal(lhs.begin(), lhs.end(), rhs.begin());
    }

    template<typename T>
    bool operator!=(const vector<T>& lhs, const vector<T>& rhs)
    {
        return !(lhs == rhs);
    }

    template<typename T>
    bool operator<(const vector<T>& lhs, const vector<T>& rhs)
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(),
            rhs.begin(), rhs.end());
    }

    template<typename T>
    bool operator<=(const vector<T>& lhs, const vector<T>& rhs)
    {
        return !(rhs < lhs);
    }

    template<typename T>
    bool operator>(const vector<T>& lhs, const vector<T>& rhs)
    {
        return rhs < lhs;
    }

    template<typename T>
    bool operator>=(const vector<T>& lhs, const vector<T>& rhs)
    {
        return !(lhs < rhs);
    }


}

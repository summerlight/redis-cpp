#ifndef ARRAY_VIEW_H
#define ARRAY_VIEW_H

#include <iterator>
#include <exception>
#include <cassert>

template<typename T>
class array_view
{
public:
	typedef array_view<T> self_type;

	typedef T value_type;
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef T* pointer; 
	typedef const T* const_pointer;
	typedef T* iterator;
	typedef const T* const_iterator;
	typedef std::reverse_iterator<iterator> reverse_iterator;
	typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

	inline array_view() : begin_(nullptr), end_(nullptr) {}
	inline array_view(pointer ptr, size_t size) : begin_(ptr), end_(ptr+size) {}
	inline array_view(pointer begin, pointer end) : begin_(begin), end_(std::max(end, begin)) {}
	inline array_view(const self_type& other) : begin_(other.begin_), end_(other.end_) {}
	template<typename U>
	inline array_view(const array_view<U>& other) : begin_(other.data()), end_(other.data()+other.size()) {}

	inline ~array_view() {}

	inline const self_type& operator=(const self_type& lhs)
	{
		begin_ = lhs.begin_;
		end_ = lhs.end_;
		return *this;
	}

	// accessors
	inline reference at(size_t index)
	{
		return const_cast<reference>(const_cast<const self_type*>(this)->at(index));
	}

	inline const_reference at(size_t index) const
	{
		if (index >= size()) {
			throw std::out_of_range();
		}
		return *(begin_+index);
	}

	inline reference operator[](size_t index)
	{
		return const_cast<reference>(const_cast<const self_type*>(this)->operator[](index));
	}

	inline const_reference operator[](size_t index) const
	{
		assert(index < size());
		return *(begin_+index);
	}

	inline reference front()
	{
		return *begin_;
	}

	inline const_reference front() const
	{
		return *begin_;
	}

	inline reference back()
	{
		return *(end_-1);
	}

	inline const_reference back() const
	{
		return *(end_-1);
	}

	inline pointer data()
	{
		return begin_;
	}

	inline const_pointer data() const
	{
		return begin_;
	}

	// query functions
	inline bool empty() const
	{
		return end_ == begin_;
	}

	inline size_type size() const
	{
		return end_ - begin_;
	}

	inline bool valid() const
	{
		return begin_ != nullptr;
	}

	// utility functions
	inline self_type slice(difference_type i, difference_type j) const
	{
		return self_type(begin_+interpret(i), begin_+interpret(j));
	}

	inline std::pair<self_type, self_type> split(size_type i) const
	{
		size_type actual = std::min(i, size());
		return std::make_pair(self_type(begin_, actual), self_type(begin_ + actual, end_));
	}

	// iterators
	inline iterator begin()
	{
		return begin_;
	}

	inline const_iterator begin() const
	{
		return begin_;
	}

	inline const_iterator cbegin() const
	{
		return begin_;
	}

	inline iterator end()
	{
		return end_;
	}

	inline const_iterator end() const
	{
		return end_;
	}

	inline const_iterator cend() const
	{
		return end_;
	}

	// reverse iterators
	inline reverse_iterator rbegin()
	{
		return reverse_iterator(end());
	}

	inline const_reverse_iterator rbegin() const
	{
		return const_reverse_iterator(end());
	}

	inline const_reverse_iterator crbegin() const
	{
		return const_reverse_iterator(end());
	}

	inline reverse_iterator rend()
	{
		return reverse_iterator(begin());
	}

	inline const_reverse_iterator rend() const
	{
		return const_reverse_iterator(begin());
	}

	inline const_reverse_iterator crend() const
	{
		return const_reverse_iterator(begin());
	}

private:
	inline size_type interpret(difference_type idx) const
	{
		if (idx < 0) {
			return std::max<size_type>(size()+idx, 0);
		} else {
			return std::min<size_type>(idx, size());
		}
	}

	T* begin_;
	T* end_;
};

template<typename T>
inline auto begin(array_view<T>& v) -> decltype(v.begin())
{
	return v.begin();
}

template<typename T>
inline auto begin(const array_view<T>& v) -> decltype(v.begin())
{
	return v.begin();
}

template<typename T>
inline auto end(array_view<T>& v) -> decltype(v.end())
{
	return v.end();
}

template<typename T>
inline auto end(const array_view<T>& v) -> decltype(v.end())
{
	return v.end();
}

#endif // ARRAY_VIEW_H

/** \file
 * A queue implementation for vector as underlying container.
 * $Id: vector-queue.hpp,v 1.5 2008/01/04 14:30:00 mina86 Exp $
 */

#ifndef H_VECTOR_QUEUE_HPP
#define H_VECTOR_QUEUE_HPP

#include <assert.h>

#include <queue>
#include <vector>


namespace std {


template<typename T, typename Alloc>
struct queue<T, vector<T, Alloc> > {
	/** queue type. */
	typedef queue<T, vector<T, Alloc> >                  queue_type;
	/** Underlying vector type. */
	typedef vector<T, Alloc>                             vector_type;
	/** Templated type. */
	typedef typename vector_type::value_type             value_type;
	/** Pointer to templated type. */
	typedef typename vector_type::pointer                pointer;
	/** A const pointer to templated type. */
	typedef typename vector_type::const_pointer          const_pointer;
	/** A reference to templated type. */
	typedef typename vector_type::reference              reference;
	/** A const reference to templated type. */
	typedef typename vector_type::const_reference        const_reference;
	/** Type for specyfying vector size. */
	typedef typename vector_type::size_type              size_type;
	/** Type for specyfying distance between two elements. */
	typedef typename vector_type::difference_type        difference_type;
	/** Allocator type. */
	typedef typename vector_type::allocator_type         allocator_type;


	/**
	 * Default constructor creates no elements.
	 * \param v initial contents.
	 */
	explicit queue(const vector_type &v = vector_type())
		: c(v), count(c.size()), first(c.begin()), last(c.end()) {
		if (c.capacity() != c.size()) {
			c.resize(c.capacity());
		}
	}

	/**  Returns \c true if the %queue is empty. */
	bool empty() const { return !count; }

	/** Returns the number of elements in the %queue. */
	size_type size() const { return count; }

	/**
	 * Returns a read/write reference to the data at the first element
	 * of the %queue.
	 */
	reference front() { return *first; }

	/**
	 * Returns a read-only (constant) reference to the data at the
	 * first element of the %queue.
	 */
	const_reference front() const { return *first; }

	/**
	 * Returns a read/write reference to the data at the last
	 * element of the %queue.
	 */
	reference back() {
		typename vector_type::iterator it = (last==c.begin()?c.end():last);
		--it;
		return *it;
	}

	/**
	 * Returns a read-only (constant) reference to the data at the
	 * last element of the %queue.
	 */
	const_reference back() const {
		typename vector_type::const_iterator it =
			(last == c.begin() ? c.end() : last);
		--it;
		return *it;
	}

	/**
	 * Add data to the end of the %queue.  This is a typical %queue
	 * operation.  The function creates an element at the end of the
	 * %queue and assigns the given data to it.  The time complexity
	 * of the operation depends on the underlying sequence.
	 *
	 * \param x Data to be added.
	 */
	void push(const value_type &x) {
		if (c.capacity() == count) {
			moreSpace();
		}
		++count;
		*last = x;
		if (++last == c.end()) {
			last = c.begin();
		}
	}

	/**
	 * Removes first element.  This is a typical %queue operation.  It
	 * shrinks the %queue by one.  The time complexity of the
	 * operation depends on the underlying sequence.
	 *
	 * Note that no data is returned, and if the first element's data
	 * is needed, it should be retrieved before pop() is called.
	 */
	void pop() {
		if (!count) {
			assert(0);
		} else if (!--count) {
			if (c.capacity() > 2 * init_size()) c.swap(vector_type());
			first = last = c.begin();
		} else if (++first == c.end()) {
			first = c.begin();
		}
	}


protected:
	/** Underlying container. */
	vector_type c;


private:
	/** Number of elements. */
	size_type count;

	/** First element of the queue. */
	typename vector_type::iterator first;

	/** Last element of the queue. */
	typename vector_type::iterator last;


	/** Allocates more space. */
	void moreSpace() {
#if 1
		typename vector_type::size_type pos = first - c.begin();
		c.resize((count > init_size() ? count : init_size()) * 2);
		first = c.begin();
		last = first + count;
		while (pos) {
			*last = *first;
			++first;
			++last;
			--pos;
		}
#else
		if (first == c.begin()) {
			c.resize((count > init_size() ? count : init_size()) * 2);
		} else {
			vector_type v((count > init_size() ? count : init_size()) * 2);
			typename vector_type::iterator end = c.end(), x = v.begin();
			for (size_type i = count; i; --i) {
				*x = *first;
				++x;
				if (++first == end) first = c.begin();
			}
			c.swap(v);
		}
		first = c.begin();
		last = first + count;
#endif
	}


	/** Returns intial size of vector. */
	static typename vector_type::size_type init_size() {
		return 512 / sizeof T > 8 ? 512 / sizeof T : 8;
	}
};


}

#endif

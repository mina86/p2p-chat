/** \file
 * An unordered vector definition and implementation.
 * $Id: unordered-vector.hpp,v 1.2 2007/12/03 23:49:19 mina86 Exp $
 */
#ifndef H_UNORDERED_VECTOR_HPP
#define H_UNORDERED_VECTOR_HPP

#include <vector>

namespace ppc {


/**
 * A container which offers fixed time access to individual elements
 * in any order but which does not preserve elements order.  This is
 * optimisation choice -- when removing elements to keep order of
 * elements some elements have to be moved, this class does not
 * preserve order when removing elements and thus needs to move less
 * elements.
 *
 * All operations are implemented using a vector class.
 */
template<typename T, typename Alloc = std::allocator<T> >
struct unordered_vector {
	/** unordered_vector type. */
	typedef unordered_vector<T, Alloc>                   unordered_vector_type;
	/** Underlying vector type. */
	typedef std::vector<T, Alloc>                        vector_type;
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
	/** An iterator. */
	typedef typename vector_type::iterator               iterator;
	/** A const iterator. */
	typedef typename vector_type::const_iterator         const_iterator;
	/** A reverse iterator. */
	typedef typename vector_type::reverse_iterator       reverse_iterator;
	/** A const reverse iterator. */
	typedef typename vector_type::const_reverse_iterator const_reverse_iterator;
	/** Type for specyfying vector size. */
	typedef typename vector_type::size_type              size_type;
	/** Type for specyfying distance between two elements. */
	typedef typename vector_type::difference_type        difference_type;
	/** Allocator type. */
	typedef typename vector_type::allocator_type         allocator_type;


	/**
	 * Default constructor creates no elements.
	 * \param a allocator to use.
	 */
	explicit unordered_vector(const allocator_type &a = allocator_type())
		: storage(a) { }

	/**
	 * Create an %unordered_vector with copies of an exemplar element.
	 * This constructor fills the %unordered_vector with \a n copies
	 * of \a value.
	 *
	 * \param n     the number of elements to initially create.
	 * \param value an element to copy.
	 * \param a     allocator to use.
	 */
	explicit unordered_vector(size_type n,
	                          const value_type &value = value_type(),
	                          const allocator_type &a = allocator_type())
		: storage(n, value, a) { }

	/**
	 * %unordered_vector copy constructor.  The newly-created
	 * %unordered_vector uses a copy of the allocation object used by
	 * \a x.  All the elements of \a x are copied, but any extra
	 * memory in \a x (for fast expansion) will not be copied.
	 *
	 * \param x an %vector of identical element and allocator types.
	 */
	unordered_vector(const vector_type &x) : storage(x) { }

	/**
	 * Builds a %unordered_vector from a range. Create
	 * a %unordered_vector consisting of copies of the elements from
	 * [first,last).
	 *
	 * If the iterators are forward, bidirectional, or random-access,
	 * then this will call the elements' copy constructor N times
	 * (where N is distance(first,last)) and do no memory
	 * reallocation.  But if only input iterators are used, then this
	 * will do at most 2N calls to the copy constructor, and logN
	 * memory reallocations.
	 *
	 * \param first an input iterator.
	 * \param last  an input iterator.
	 * \param a     allocator to use.
	 */
	template<typename InputIterator>
	unordered_vector(InputIterator first, InputIterator last,
	                 const allocator_type &a = allocator_type())
		: storage(first, last, a) { }


	/**
	 * Returns a read/write iterator that points to the first element
	 * in the %unordered_vector.  Iteration is done in ordinary
	 * element order.
	 */
	iterator               begin   ()       { return storage.begin   (); }
	/**
	 * Returns a read-only (constant) iterator that points to the
	 * first element in the %unordered_vector.  Iteration is done in
	 * ordinary element order.
	 */
	const_iterator         begin   () const { return storage.begin   (); }
	/**
	 * Returns a read/write iterator that points one past the last
	 * element in the %unordered_vector.  Iteration is done in
	 * ordinary element order.
	 */
	iterator               end     ()       { return storage.end     (); }
	/**
	 * Returns a read-only (constant) iterator that points one past
	 * the last element in the %unordered_vector.  Iteration is done
	 * in ordinary element order.
	 */
	const_iterator         end     () const { return storage.end     (); }
	/**
	 * Returns a read/write reverse iterator that points to the last
	 * element in the %unordered_vector.  Iteration is done in reverse
	 * element order.
	 */
	reverse_iterator       rbegin  ()       { return storage.rbegin  (); }
	/**
	 * Returns a read-only (constant) reverse iterator that points to
	 * the last element in the %unordered_vector.  Iteration is done
	 * in reverse element order.
	 */
	const_reverse_iterator rbegin  () const { return storage.rbegin  (); }
	/**
	 * Returns a read/write reverse iterator that points to one
	 * before the first element in the %unordered_vector.  Iteration is done
	 * in reverse element order.
	 */
	reverse_iterator       rend    ()       { return storage.rend    (); }
	/**
	 * Returns a read-only (constant) reverse iterator that points
	 * to one before the first element in the %unordered_vector.  Iteration
	 * is done in reverse element order.
	 */
	const_reverse_iterator rend    () const { return storage.rend    (); }
	/** Returns the number of elements in the %unordered_vector.  */
	size_type              size    () const { return storage.size    (); }
	/** Returns the size() of the largest possible %unordered_vector.  */
	size_type              max_size() const { return storage.max_size(); }
	/**
	 * Returns the total number of elements that the %unordered_vector
	 * can hold before needing to allocate more memory.
	 */
	size_type              capacity() const { return storage.capacity(); }
	/**
	 * Returns true if the %unordered_vector is empty.  (Thus begin()
	 * would equal end().)
	 */
	bool                   empty   () const { return storage.empty   (); }
	/**
	 * Returns a read/write reference to the data at the first
	 * element of the %unordered_vector.
	 */
	reference              front   ()       { return storage.front   (); }
	/**
	 * Returns a read-only (constant) reference to the data at the
	 * first element of the %unordered_vector.
	 */
	const_reference        front   () const { return storage.front   (); }
	/**
	 * Returns a read/write reference to the data at the last element
	 * of the %unordered_vector.
	 */
	reference              back    ()       { return storage.back    (); }
	/**
	 * Returns a read-only (constant) reference to the data at the
	 * last element of the %unordered_vector.
	 */
	const_reference        back    () const { return storage.back    (); }
	/**
	 *  Returns a pointer such that [data(), data() + size()) is
	 *  a valid range.  For a non-empty %unordered_vector, data() ==
	 *  &front().
	 */
	pointer                data    ()       { return storage.data    (); }
	/**
	 *  Returns a pointer such that [data(), data() + size()) is
	 *  a valid range.  For a non-empty %unordered_vector, data() ==
	 *  &front().
	 */
	const_pointer          data    () const { return storage.data    (); }
	/** Returns underlying vector. */
	operator vector_type&          ()       { return storage           ; }
	/** Returns underlying vector. */
	operator const vector_type&    () const { return storage           ; }


	/**
	 * %unordered_ector assignment operator.  All the elements of \a
	 * x are copied, but any extra memory in \a x (for fast expansion)
	 * will not be copied.  Unlike the copy constructor, the allocator
	 * object is not copied.
	 *
	 * \param x an %unordered_vector of identical element and
	 *             allocator types.
	 */
	unordered_vector &operator=(const unordered_vector &x) {
		storage = x.storage;
		return *this;
	}

	/**
	 * Assigns a given value to an %unordered_vector.  This function
	 * fills a %unordered_vector with \a n copies of the given value.
	 *
	 * \note The assignment completely changes the %unordered_vector
	 * and that the resulting %unordered_vector's size is the same as
	 * the number of elements assigned.  Old data may be lost.
	 *
	 * \param n   number of elements to be assigned.
	 * \param val value to be assigned.
	 */
	void assign(size_type n, const value_type& val) {
		storage.assign(n, val);
	}

	/**
	 * Assigns a range to a %unordered_vector.  This function fills
	 * a %unordered_vector with copies of the elements in the range
	 * [first,last).
	 *
	 * \note The assignment completely changes the %unordered_vector
	 * and that the resulting %unordered_vector's size is the same as
	 * the number of elements assigned.  Old data may be lost.
	 *
	 * \param first an input iterator.
	 * \param last  an input iterator.
	 */
	template<typename InputIterator>
	void assign(InputIterator first, InputIterator last) {
		storage.assign(first, last);
	}

	/**
	 * Resizes the %unordered_vector to the specified number of
	 * elements.  This function will %resize the %unordered_vector to
	 * the specified number of elements.  If the number is smaller
	 * than the %unordered_vector's current size the %unordered_vector
	 * is truncated, otherwise the %unordered_vector is extended and
	 * new elements are populated with given data.
	 *
	 * \param new_size number of elements the %unordered_vector should
	 *                 contain.
	 * \param x        data with which new elements should be populated.
	 */
	void resize(size_type new_size, value_type x = value_type()) {
		storage.resize(new_size, x);
	}

	/**
	 * Attempt to preallocate enough memory for specified number of
	 * elements.  This function attempts to reserve enough memory for
	 * the %unordered_vector to hold the specified number of elements.
	 * If the number requested is more than max_size(), length_error
	 * is thrown.
	 *
	 * The advantage of this function is that if optimal code is a
	 * necessity and the user can determine the number of elements
	 * that will be required, the user can reserve the memory in
	 * %advance, and thus prevent a possible reallocation of memory
	 * and copying of %unordered_vector data.
	 *
	 * \param n number of elements required.
	 * \throw std::length_error if \a n exceeds \c max_size().
	 */
	void reserve(size_type n) {
		storage.reserve(n);
	}

	/**
	 * Subscript access to the data contained in the %unordered_vector.
	 * This operator allows for easy, array-style, data access.
	 *
	 * \note Data access with this operator is unchecked and
	 * out_of_range lookups are not defined. (For checked lookups see
	 * at().)
	 *
	 * \param n the index of the element for which data should be
	 *          accessed.
	 * \return read/write reference to data.
	 */
	reference       operator[](size_type n)       { return storage[n]; }
	/**
	 * Subscript access to the data contained in the %unordered_vector.
	 * This operator allows for easy, array-style, data access.
	 *
	 * \note Data access with this operator is unchecked and
	 * out_of_range lookups are not defined. (For checked lookups see
	 * at().)
	 *
	 * \param n the index of the element for which data should be
	 *          accessed.
	 * \return read-only (constant) reference to data.
	 */
	const_reference operator[](size_type n) const { return storage[n]; }
	/**
	 * Provides access to the data contained in the %unordered_vector.
	 * This function provides for safer data access.  The parameter is
	 * first checked that it is in the range of the vector.  The
	 * function throws out_of_range if the check fails.
	 *
	 * \param n the index of the element for which data should be
	 *          accessed.
	 * \return read/write reference to data.
	 * \throw std::out_of_range if \a n is an invalid index.
	 */
	reference       at        (size_type n)       { return storage.at(n); }
	/**
	 * Provides access to the data contained in the %unordered_vector.
	 * This function provides for safer data access.  The parameter is
	 * first checked that it is in the range of the vector.  The
	 * function throws out_of_range if the check fails.
	 *
	 * \param n the index of the element for which data should be
	 *          accessed.
	 * \return  read-only (constant) reference to data.
	 */
	const_reference at        (size_type n) const { return storage.at(n); }

	/**
	 * Add data to the end of the %unordered_vector.
	 *
	 * This is a typical stack operation.  The function creates an
	 * element at the end of the %unordered_vector and assigns the
	 * given data to it.  Due to the nature of a %unordered_vector
	 * this operation can be done in constant time if the
	 * %unordered_vector has preallocated space available.
	 *
	 * \param x data to be added.
	 */
	void  push_back(const value_type &x) {
		storage.push_back(x);
	}

	/**
	 * Removes last element. This is a typical stack operation.  It
	 * shrinks the %unordered_vector by one.
	 *
	 * \note No data is returned, and if the last element's data is
	 * needed, it should be retrieved before pop_back() is called.
	 */
	void pop_back() {
		storage.pop_back();
	}

	/**
	 * Finds element.
	 *
	 * \param x  element to search for.
	 * \param position position to start looking for.
	 * \return iterator pointing to element or end().
	 */
	iterator find(const value_type &x, iterator position) {
		const iterator last = end();
		while (position!=last && *position!=x) ++position;
		return position;
	}

	/**
	 * Finds element.
	 *
	 * \param x  element to search for.
	 * \return iterator pointing to element or end().
	 */
	iterator find(const value_type &x) {
		return find(x, begin());
	}

	/**
	 * Finds element.
	 *
	 * \param x  element to search for.
	 * \param position position to start looking for.
	 * \return iterator pointing to element or end().
	 */
	const_iterator find(const value_type &x, const_iterator position) const {
		return const_cast<unordered_vector_type*>(this)->find(x, position);
	}

	/**
	 * Finds element.
	 *
	 * \param x  element to search for.
	 * \return iterator pointing to element or end().
	 */
	const_iterator find(const value_type &x) const {
		return find(x, begin());
	}

	/**
	 * Inserts given value into %unordered_vector before specified
	 * iterator.  This function will insert a copy of the given value
	 * before the specified location.
	 *
	 * \note This kind of operation could be expensive for
	 * a %unordered_vector and if it is frequently used the user
	 * should consider using std::list.
	 *
	 * \param position an iterator into the %unordered_vector.
	 * \param x        data to be inserted.
	 * \return an iterator that points to the inserted data.
	 */
	iterator insert(iterator position, const value_type &x) {
		return data.insert(position, x);
	}

	/**
	 * Inserts a number of copies of given data into the
	 * %unordered_vector.  This function will insert a specified
	 * number of copies of the given data before the location
	 * specified by \a position.
	 *
	 * \note This kind of operation could be expensive for
	 * a %unordered_vector and if it is frequently used the user
	 * should consider using std::list.
	 *
	 * \param position  an iterator into the %unordered_vector.
	 * \param n         number of elements to be inserted.
	 * \param x         data to be inserted.
	 */
	void insert(iterator position, size_type n, const value_type &x) {
		data.storage(position, n, x);
	}

	/**
	 * Inserts a range into the %unordered_vector.  This function will
	 * insert copies of the data in the range [first,last) into the
	 * %unordered_vector before the location specified by \a pos.
	 *
	 * \note This kind of operation could be expensive for
	 * a %unordered_vector and if it is frequently used the user
	 * should consider using std::list.
	 *
	 * \param position an iterator into the %unordered_vector.
	 * \param first    an input iterator.
	 * \param last     an input iterator.
	 */
	template<typename InputIterator>
	void insert(iterator position, InputIterator first, InputIterator last) {
		storage.insert(position, first, last);
	}

	/**
	 * Remove element at given position.  This function will erase the
	 * element at the given position and thus shorten the
	 * %unordered_vector by one.
	 *
	 * \note This operation does not preserve elment's order.  In
	 * particular it moves element from the end of the
	 * %unordered_vector to \a position and then shortens
	 * %unordered_vector by one.
	 *
	 * \param position iterator pointing to element to be erased.
	 * \return         an iterator pointing to the next element (or end()).
	 */
	iterator erase(iterator position) {
		if (empty() || position==end()) return end();
		const difference_type idx = position - begin();
		if (position != end() - 1) {
			*position = back();
		}
		storage.pop_back();
		return begin() + idx;
	}

	/**
	 * Remove a range of elements.  This function will erase the
	 * elements in the range [first,last) and shorten the
	 * %unordered_vector accordingly.
	 *
	 * \note This operation does not preserve elment's order.  In
	 * particular it moves elements from the end of the
	 * %unordered_vector to erased space.
	 *
	 * \param first iterator pointing to the first element to be erased.
	 * \param last  iterator pointing to one past the last element to be
	 *              erased.
	 * \return an iterator pointing to the element pointed to by \a last
	 *         prior to erasing (or end()).
	 */
	iterator erase(iterator first, iterator last) {
		const difference_type count = last - first;
		if (end() - last > count) {
			const difference_type idx = first - begin();
			const iterator e = end();
			for (iterator it = e - count; it!=e; ++it, ++first) {
				*first = *it;
			}
			storage.erase(e - count, e);
			return begin() + idx;
		} else {
			return storage.erase(first, last);
		}
	}


	/**
	 * Swaps data with another %unordered_vector.  This exchanges the
	 * elements between two vectors in constant time.  Note that the
	 * global std::swap() function is specialized such that
	 * std::swap(v1,v2) will feed to this function.
	 *
	 * \param x a %unordered_vector of the same element and allocator types.
	 */
	void swap(unordered_vector &x) {
		storage.swap(x.storage);
	}

	/**
	 * Erases all the elements.  Note that this function only erases the
	 * elements, and that if the elements themselves are pointers, the
	 * pointed-to memory is not touched in any way.  Managing the pointer is
	 * the user's responsibilty.
	 */
	void clear() {
		storage.clear();
	}


private:
	/** Underlying vector. */
	vector_type storage;
};



/**
 * Unordered_vector equality comparison.  This is an equivalence
 * relation.  It is linear in the size of the vectors.  Vectors are
 * considered equivalent if their sizes are equal, and if
 * corresponding elements compare equal.
 *
 * \param x a %unordered_vector.
 * \param y a %unordered_vector of the same type as \a x.
 * \return \c true iff the size and elements of the vectors are equal.
 */
template<typename T, typename Alloc>
inline bool operator==(const unordered_vector<T, Alloc> &x,
                       const unordered_vector<T, Alloc> &y) {
	return x.vector() == y.vector();
}

/**
 * Unordered_vector ordering relation.  This is a total ordering
 * relation.  It is linear in the size of the vectors.  The elements
 * must be comparable with \c <.
 *
 * See std::lexicographical_compare() for how the determination is made.
 *
 * \param x a %unordered_vector.
 * \param y a %unordered_vector of the same type as \a x.
 * \return \c true iff \a x is lexicographically less than \a y.
 */
template<typename T, typename Alloc>
inline bool operator<(const unordered_vector<T, Alloc> &x,
                      const unordered_vector<T, Alloc> &y) {
	return x.vector() < y.vector();
}

/**
 * Unordered_vector equality comparison.  This is an equivalence
 * relation.  It is linear in the size of the vectors.  Vectors are
 * considered equivalent if their sizes are equal, and if
 * corresponding elements compare equal.
 *
 * \param x a %unordered_vector.
 * \param y a %unordered_vector of the same type as \a x.
 * \return \c false iff the size and elements of the vectors are equal.
 */
template<typename T, typename Alloc>
inline bool operator!=(const unordered_vector<T, Alloc> &x,
                       const unordered_vector<T, Alloc> &y) {
	return x.vector() != y.vector();
}

/**
 * Unordered_vector ordering relation.  This is a total ordering
 * relation.  It is linear in the size of the vectors.  The elements
 * must be comparable with \c <.
 *
 * See std::lexicographical_compare() for how the determination is made.
 *
 * \param x a %unordered_vector.
 * \param y a %unordered_vector of the same type as \a x.
 * \return \c true iff \a x is lexicographically greater than \a y.
 */
template<typename T, typename Alloc>
inline bool operator>(const unordered_vector<T, Alloc> &x,
                      const unordered_vector<T, Alloc> &y) {
	return x.vector() > y.vector();
}

/**
 * Unordered_vector ordering relation.  This is a total ordering
 * relation.  It is linear in the size of the vectors.  The elements
 * must be comparable with \c <.
 *
 * See std::lexicographical_compare() for how the determination is made.
 *
 * \param x a %unordered_vector.
 * \param y a %unordered_vector of the same type as \a x.
 * \return \c true iff \a x is lexicographically less than or equal to \a y.
 */
template<typename T, typename Alloc>
inline bool operator<=(const unordered_vector<T, Alloc> &x,
                       const unordered_vector<T, Alloc> &y) {
	return x.vector() <= y.vector();
}

/**
 * Unordered_vector ordering relation.  This is a total ordering
 * relation.  It is linear in the size of the vectors.  The elements
 * must be comparable with \c <.
 *
 * See std::lexicographical_compare() for how the determination is made.
 *
 * \param x a %unordered_vector.
 * \param y a %unordered_vector of the same type as \a x.
 * \return \c true iff \a x is lexicographically greater than or equal \a y.
 */
template<typename T, typename Alloc>
inline bool operator>=(const unordered_vector<T, Alloc> &x,
                       const unordered_vector<T, Alloc> &y) {
	return x.vector() >= y.vector();
}


}


/**
 * Swaps data within two %unordered_vector.  This exchanges the
 * elements between two vectors in constant time.
 *
 * \param x a %unordered_vector.
 * \param y a %unordered_vector of the same type as \a x.
 */
template<typename T, typename Alloc>
inline void std::swap(ppc::unordered_vector<T, Alloc> &x,
                      ppc::unordered_vector<T, Alloc> &y) {
	x.swap(y);
}


#endif

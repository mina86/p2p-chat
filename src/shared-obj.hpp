/** \file
 * Shared object definition.
 * $Id: shared-obj.hpp,v 1.1 2007/12/08 18:02:22 mina86 Exp $
 */

#ifndef H_SHARED_OBJ_HPP
#define H_SHARED_OBJ_HPP


namespace ppc {


/**
 * Template which gives typedef for template's argument with \c const
 * removed.
 */
template<class T>
struct non_const {
	/** A non-const type. */
	typedef T type;
};

/**
 * Template which gives typedef for template's argument with \c const
 * removed.
 */
template<class T>
struct non_const<const T> {
	/** A non-const type. */
	typedef T type;
};


/**
 * A shared pointer which maintains a reference counter and deletes
 * pointed object automatically if it reaches zero.  It's limitation
 * is that it can hold a pointer to shared_obj_base objects or objects
 * which have the same interface.
 */
template<class T = struct shared_obj_base>
struct shared_obj {
	/** A non-const pointer type. */
	typedef typename non_const<T>::type value_type;

	/**
	 * Sets pointer to given value.
	 * \param ptr pointed object.
	 */
	shared_obj(value_type *ptr = 0) : pointer(ptr) {
		if (pointer) pointer->increase_references();
	}

	/**
	 * Copy constructor.
	 * \param ptr shared pointer to copy.
	 */
	template<class T2>
	shared_obj(const shared_obj<T2> &ptr) : pointer(ptr.pointer) {
		pointer->increase_references();
	}

	/**
	 * Sets pointer to given value.  If currently pointed object has
	 * only one reference (after decrementation it reaches zero) it
	 * will be deleted.
	 *
	 * \param ptr pointed object.
	 * \return this object.
	 */
	shared_obj &operator=(value_type *ptr) {
		if (pointer != ptr) {
			if (pointer) pointer->decrease_references();
			if ((pointer = ptr)) pointer->increase_references();
		}
		return *this;
	}

	/**
	 * Copies pointer value.  If currently pointed object has only one
	 * reference (after decrementation it reaches zero) it will be
	 * deleted.
	 *
	 * \param ptr shared pointer to copy.
	 * \return this object.
	 */
	shared_obj &operator=(const shared_obj &ptr) {
		return *this = ptr.pointer;
	}

	/**
	 * Copies pointer value.  If currently pointed object has only one
	 * reference (after decrementation it reaches zero) it will be
	 * deleted.
	 *
	 * \param ptr shared pointer to copy.
	 * \return this object.
	 */
	template<class T2>
	shared_obj &operator=(const shared_obj<T2> &ptr) {
		return *this = ptr.pointer;
	}

	/** Returns object pointer. */
	T *get() const { return pointer; }
	/** Returns object pointer. */
	T *operator->() const { return pointer; }
	/** Returns pointed object. */
	T &operator*() const { return *pointer; }

private:
	/** Pointer to object. */
	value_type *pointer;
};



/**
 * A base class for objects pointed by shared_obj class.  shared_obj
 * class require that object it points to have the
 * a decrease_references() and increase_references() functions hence
 * they are here.
 */
struct shared_obj_base {
	/** A virtual destructor. */
	virtual ~shared_obj_base() { }

protected:
	/** Default constructor. */
	shared_obj_base() : references(0) { }

private:
	/** Copying not allowed. 
	 * \param ob ignored. */
	shared_obj_base(const shared_obj_base &ob) { (void)ob; }

	/** Decrements reference counter and delets \c this if it reaches 0. */
	void decrease_references() { if (!--references) { delete this; } }

	/** Increments reference counter. */
	void increase_references() { ++references; }

	/** Reference counter. */
	unsigned references;

	/* shared_obj must be friend so it can use private methods. */
	template<class T> friend struct shared_obj;
};


}


#endif

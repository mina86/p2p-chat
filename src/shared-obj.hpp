/** \file
 * Shared object definition.
 * $Id: shared-obj.hpp,v 1.3 2007/12/29 14:40:40 mina86 Exp $
 */

#ifndef H_SHARED_OBJ_HPP
#define H_SHARED_OBJ_HPP


namespace ppc {



/**
 * A shared pointer which maintains a reference counter and deletes
 * pointed object automatically if it reaches zero.  It's limitation
 * is that it can hold a pointer to shared_obj_base objects or objects
 * which have the same interface.
 */
template<class T = struct shared_obj_base>
struct shared_obj : public shared_obj<const T> {
	/**
	 * Sets pointer to given value.
	 * \param ptr pointed object.
	 */
	shared_obj(T *ptr = 0) : base(ptr) { }

	/**
	 * Copy constructor.  If you try to construct a shared_obj
	 * pointing to a modifable object with a shared_obj pointing to
	 * a const object compile error will stop you.
	 *
	 * \param ptr shared pointer to copy.
	 */
	template<class T2>
	shared_obj(const shared_obj<T2> &ptr) : base(ptr.get()) { }

	/**
	 * Sets pointer to given value.  If currently pointed object has
	 * only one reference (after decrementation it reaches zero) it
	 * will be deleted.
	 *
	 * \param ptr pointed object.
	 * \return this object.
	 */
	shared_obj &operator=(T *ptr) {
		base::operator=(ptr);
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
		base::operator=(ptr.get());
		return *this;
	}

	/**
	 * Copies pointer value.  If currently pointed object has only one
	 * reference (after decrementation it reaches zero) it will be
	 * deleted.  If you try to assing a shared_obj pointing to a const
	 * object compile error will stop you.
	 *
	 * \param ptr shared pointer to copy.
	 * \return this object.
	 */
	template<class T2>
	shared_obj &operator=(const shared_obj<T2> &ptr) {
		base::operator=(ptr.get());
		return *this;
	}

	/** Returns object pointer. */
	T *get() const { return const_cast<T*>(base::get()); }
	/** Returns object pointer. */
	T *operator->() const { return const_cast<T*>(base::get()); }
	/** Returns pointed object. */
	T &operator*() const { return *const_cast<T*>(base::get()); }


private:
	/** A base class */
	typedef struct shared_obj<const T> base;
};



template<class T>
struct shared_obj<const T> {
	/**
	 * Sets pointer to given value.
	 * \param ptr pointed object.
	 */
	shared_obj(T *ptr = 0) : pointer(ptr) {
		if (pointer) pointer->increase_references();
	}

	/**
	 * Copy constructor.
	 * \param ptr shared pointer to copy.
	 */
	template<class T2>
	shared_obj(const shared_obj<const T2> &ptr) : pointer(ptr.pointer) {
		if (pointer) pointer->increase_references();
	}

	/**
	 * Decreases pointer object reference counter.
	 */
	~shared_obj() {
		if (pointer) pointer->decrease_references();
	}

	/**
	 * Sets pointer to given value.  If currently pointed object has
	 * only one reference (after decrementation it reaches zero) it
	 * will be deleted.
	 *
	 * \param ptr pointed object.
	 * \return this object.
	 */
	shared_obj &operator=(T *ptr) {
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
	shared_obj &operator=(const shared_obj<const T2> &ptr) {
		return *this = ptr.pointer;
	}

	/** Returns object pointer. */
	const T *get() const { return pointer; }
	/** Returns object pointer. */
	const T *operator->() const { return pointer; }
	/** Returns pointed object. */
	const T &operator*() const { return *pointer; }


private:
	/** Pointer to object. */
	T *pointer;

	/* all shared_obj are friends */
	template<class T2> friend struct shared_obj;
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

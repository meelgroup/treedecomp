#ifndef ARRAY_ID_FUNC_H
#define ARRAY_ID_FUNC_H

#include "id_func.hpp"
#include <type_traits>
#include <algorithm>
#include <cassert>
// two levels up in the source tree and in the installed include
// tree alike; a bare "treedecomp_defs.hpp" only resolves in-tree,
// where -Isrc is on the command line, and breaks consumers
#include "../../treedecomp_defs.hpp"

template<class T>
class ArrayIDFunc{
public:
	ArrayIDFunc()noexcept:preimage_count_(0), data_(nullptr){}

	explicit ArrayIDFunc(int preimage_count)
		:preimage_count_(preimage_count){
		assert(preimage_count >= 0 && "ids may not be negative");
		if(preimage_count == 0)
			data_ = nullptr;
		else
			data_ = new T[preimage_count_];
	}

	template<class IDFunc>
	ArrayIDFunc(const IDFunc&o)
		:preimage_count_(o.preimage_count()){
		if(preimage_count_ == 0)
			data_ = nullptr;
		else{
			data_ = new T[preimage_count_];
			try{
				for(int id=0; id<preimage_count_; ++id)
					data_[id] = o(id);
			}catch(...){
				delete[]data_;
				throw;
			}
		}
	}

	ArrayIDFunc(const ArrayIDFunc&o)
		:preimage_count_(o.preimage_count_){
		if(preimage_count_ == 0)
			data_ = nullptr;
		else{
			data_ = new T[preimage_count_];
			try{
				std::copy(o.data_, o.data_ + o.preimage_count_, data_);
			}catch(...){
				delete[]data_;
				throw;
			}
		}
	}

	ArrayIDFunc(ArrayIDFunc&&o)noexcept
		:preimage_count_(o.preimage_count_), data_(o.data_){
		o.preimage_count_ = 0;
		o.data_ = nullptr;
	}

	~ArrayIDFunc(){
		delete[]data_;
	}

	void swap(ArrayIDFunc&o)noexcept{
		std::swap(preimage_count_, o.preimage_count_);
		std::swap(data_, o.data_);
	}

	template<class IDFunc>
	typename std::enable_if<
		is_id_func<IDFunc>::value &&
		std::is_convertible<typename id_func_image_type<IDFunc>::type, T>::value,
	ArrayIDFunc&>::type operator=(const IDFunc&o){
		ArrayIDFunc(o).swap(*this);
		return *this;
	}

	ArrayIDFunc&operator=(const ArrayIDFunc&o){
		ArrayIDFunc(o).swap(*this);
		return *this;
	}

	ArrayIDFunc&operator=(ArrayIDFunc&&o)noexcept{
		this->~ArrayIDFunc();
		data_ = nullptr;
		preimage_count_ = 0;
		swap(o);
		return *this;
	}

	// These accessors are the innermost flow-cutter loops; their bounds checks
	// alone cost ~25% of the runtime, hence SLOW_DEBUG rather than plain assert.
	// IDFunc
	int preimage_count() const{return preimage_count_;}

	const T&operator()(int id) const{
		SLOW_DEBUG_DO(assert(0 <= id && id < preimage_count_ && "id out of bounds"));
		return data_[id];
	}

	// Mutable IDFunc
	void set(int id, T t){
		SLOW_DEBUG_DO(assert(0 <= id && id < preimage_count_ && "id out of bounds"));
		data_[id] = std::move(t);
	}

	T move(int id){
		SLOW_DEBUG_DO(assert(0 <= id && id < preimage_count_ && "id out of bounds"));
		return std::move(data_[id]);
	}

	void fill(const T&t){
		std::fill(data_, data_+preimage_count_, t);
	}

	// Array only functionality
	T&operator[](int id){
		SLOW_DEBUG_DO(assert(0 <= id && id < preimage_count_ && "id out of bounds"));
		return data_[id];
	}

	const T&operator[](int id) const{
		SLOW_DEBUG_DO(assert(0 <= id && id < preimage_count_ && "id out of bounds"));
		return data_[id];
	}

	T*begin(){ return data_; }
	const T*begin() const{ return data_; }
	T*end(){ return data_ + preimage_count_; }
	const T*end()const{ return data_ + preimage_count_; }

	int preimage_count_;
	T*data_;
};

struct ArrayIDIDFunc : public ArrayIDFunc<int>{

	ArrayIDIDFunc()noexcept :image_count_(0){}

	ArrayIDIDFunc(int preimage_count, int image_count)
		:ArrayIDFunc<int>(preimage_count), image_count_(image_count){}

	ArrayIDIDFunc(const ArrayIDIDFunc&o) = default;
	ArrayIDIDFunc(ArrayIDIDFunc&&) = default;
	ArrayIDIDFunc&operator=(const ArrayIDIDFunc&) = default;
	ArrayIDIDFunc&operator=(ArrayIDIDFunc&&) = default;

	void swap(ArrayIDIDFunc&o){
		std::swap(image_count_, o.image_count_);
		ArrayIDFunc<int>::swap(static_cast<ArrayIDFunc<int>&>(o));
	}

	template<class IDFunc>
	ArrayIDIDFunc(const IDFunc&f, int _image_count_)
		: ArrayIDFunc<int>(f), image_count_(_image_count_){}


	template<class IDIDFunc>
	ArrayIDIDFunc(const IDIDFunc&f/*,
		typename std::enable_if<is_id_id_func<IDIDFunc>::value, void>::type*dummy=0*/)
		: ArrayIDFunc<int>(f), image_count_(f.image_count()){}

	template<class IDIDFunc>
	typename std::enable_if<
		is_id_id_func<IDIDFunc>::value,
		ArrayIDIDFunc&
	>::type operator=(const IDIDFunc&o){
		ArrayIDIDFunc(o).swap(*this);
		return *this;
	}

	int image_count()const { return image_count_; }

	int operator()(int x) const{
		SLOW_DEBUG_DO(assert(0 <= x && x < preimage_count_ && "preimage id out of bounds"));
		int y = data_[x];
		SLOW_DEBUG_DO(assert(0 <= y && y < image_count_ && "image id out of bounds"));
		return y;
	}

	void set_image_count(int new_image_count){
		image_count_ = new_image_count;
	}

	int image_count_;
};

#endif


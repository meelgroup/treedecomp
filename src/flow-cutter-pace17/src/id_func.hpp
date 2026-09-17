#ifndef ID_FUNC_H
#define ID_FUNC_H

#include <cassert>
// two levels up in the source tree and in the installed include
// tree alike; a bare "treedecomp_defs.hpp" only resolves in-tree,
// where -Isrc is on the command line, and breaks consumers
#include "../../treedecomp_defs.hpp"
#include <utility>
#include "id_func_traits.hpp"
#include <type_traits>

template<class Func>
struct LambdaIDFunc{
	int preimage_count()const{return preimage_count_;}

	typename id_func_image_type<Func>::type operator()(int id)const{
		SLOW_DEBUG_DO(assert(0 <= id && id <= preimage_count_ && "id out of bounds"));
		return func_(id);
	}

	int preimage_count_;
	Func func_;
};

template<class Func>
typename std::enable_if<
	has_int_call_operator<Func>::value,
	LambdaIDFunc<Func>
>::type id_func(int preimage_count, Func func){
	return {preimage_count, std::move(func)};
}

template<class IDFunc>
struct LambdaIDIDFunc{
	static_assert(std::is_same<int, typename id_func_image_type<IDFunc>::type>::value, "IDIDFunc must return int");

	int preimage_count()const{return id_func_.preimage_count();}
	int image_count()const{return image_count_;}

	int operator()(int preimage)const{
		SLOW_DEBUG_DO(assert(0 <= preimage && preimage <= preimage_count() && "preimage out of bounds"));
		int image = id_func_(preimage);
		SLOW_DEBUG_DO(assert(0 <= image && image <= image_count() && "image out of bounds"));
		return image;
	}

	int image_count_;
	IDFunc id_func_;

};

template<class IDFunc>
typename std::enable_if<
	is_id_func<IDFunc>::value,
	LambdaIDIDFunc<IDFunc>
>::type id_id_func(int image_count, IDFunc func){
	return {image_count, std::move(func)};
}

template<class Func>
typename std::enable_if<
	has_int_call_operator<Func>::value,
	LambdaIDIDFunc<LambdaIDFunc<Func>> 
>::type id_id_func(int preimage_count, int image_count, Func func){
	return {image_count, id_func(preimage_count, std::move(func))};
}

template<int value>
struct ConstIntIDFunc{
	explicit ConstIntIDFunc(int preimage_count):preimage_count_(preimage_count){}

	int preimage_count()const{
		return preimage_count_;
	}

	int operator()(int)const{
		return value;
	}

	// Every arc has the same capacity, so the back arc's is the same too.
	int back_capacity(int)const{
		return value;
	}

	int preimage_count_;
};

template<class IDIDFunc>
class ConstRefIDIDFunc{
public:
	ConstRefIDIDFunc():ptr(0){}
	ConstRefIDIDFunc(const IDIDFunc&x):ptr(&x){}

	int preimage_count()const{ return ptr->preimage_count(); }
	int image_count()const{return ptr->image_count(); }
	int operator()(int x)const{return (*ptr)(x);}

private:
	const IDIDFunc*ptr;
};

template<class IDIDFunc>
ConstRefIDIDFunc<IDIDFunc>make_const_ref_id_id_func(const IDIDFunc&f){
	return {f};
}

template<class IDFunc>
class ConstRefIDFunc{
public:
	ConstRefIDFunc():ptr(0){}
	ConstRefIDFunc(const IDFunc&x):ptr(&x){}

	int preimage_count()const{ return ptr->preimage_count(); }
	decltype(std::declval<const IDFunc>()(0)) operator()(int x)const{return (*ptr)(x);}

private:
	const IDFunc*ptr;
};

template<class IDFunc>
ConstRefIDFunc<IDFunc>make_const_ref_id_func(const IDFunc&f){
	return {f};
}


#endif


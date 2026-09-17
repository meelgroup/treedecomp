#ifndef COUNT_RANGE_H
#define COUNT_RANGE_H

#include "range.hpp"
#include <cassert>
#include <iterator>
// two levels up in the source tree and in the installed include
// tree alike; a bare "treedecomp_defs.hpp" only resolves in-tree,
// where -Isrc is on the command line, and breaks consumers
#include "../../treedecomp_defs.hpp"

struct CountIterator{
	typedef int value_type;
	typedef int difference_type;
	typedef const int* pointer;
	typedef const int& reference;
	typedef std::random_access_iterator_tag iterator_category;

	CountIterator&operator++(){ ++n_; return *this;}
	CountIterator operator++(int) {CountIterator tmp(*this); operator++(); return tmp;}
	CountIterator&operator--(){ --n_; return *this;}
	CountIterator operator--(int) {CountIterator tmp(*this); operator++(); return tmp;}
	int operator*() const {return n_;}

	const int*operator->() const {return &n_;}

	int operator[](int o)const{return n_ + o;}
	CountIterator&operator+=(CountIterator::difference_type o){n_ += o; return *this;}
	CountIterator&operator-=(CountIterator::difference_type o){n_ -= o; return *this;}

	int n_;
};

inline bool operator==(CountIterator l, CountIterator r){return l.n_ == r.n_;}
inline bool operator!=(CountIterator l, CountIterator r){return l.n_ != r.n_;}
inline bool operator< (CountIterator l, CountIterator r){return l.n_ <  r.n_;}
inline bool operator> (CountIterator l, CountIterator r){return l.n_ >  r.n_;}
inline bool operator<=(CountIterator l, CountIterator r){return l.n_ <= r.n_;}
inline bool operator>=(CountIterator l, CountIterator r){return l.n_ >= r.n_;}

inline CountIterator::difference_type operator-(CountIterator l, CountIterator r){return l.n_ - r.n_;}
inline CountIterator operator-(CountIterator l, CountIterator::difference_type r){return {l.n_ - r};}
inline CountIterator operator+(CountIterator l, CountIterator::difference_type r){return {l.n_ + r};}
inline CountIterator operator+(CountIterator::difference_type l, CountIterator r){return {l + r.n_};}

typedef Range<CountIterator> CountRange;

inline CountRange count_range(int n){SLOW_DEBUG_DO(assert(n >= 0)); return {CountIterator{0}, CountIterator{n}}; }
inline CountRange count_range(int begin, int end){SLOW_DEBUG_DO(assert(begin <= end));return {CountIterator{begin}, CountIterator{end}};}

#endif

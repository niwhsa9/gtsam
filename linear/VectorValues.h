/**
 * @file    VectorValues.h
 * @brief   Factor Graph Valuesuration
 * @author  Richard Roberts
 */

#pragma once

#include <gtsam/base/Testable.h>
#include <gtsam/base/Vector.h>
#include <gtsam/base/types.h>

#include <boost/foreach.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/shared_ptr.hpp>

#include <vector>

namespace gtsam {

class VectorValues : public Testable<VectorValues> {
protected:
  Vector values_;
  std::vector<size_t> varStarts_;

public:
  template<class C> class _impl_iterator;  // Forward declaration of iterator implementation
  typedef boost::shared_ptr<VectorValues> shared_ptr;
  typedef _impl_iterator<VectorValues> iterator;
  typedef _impl_iterator<const VectorValues> const_iterator;
  typedef boost::numeric::ublas::vector_range<Vector> value_reference_type;
  typedef boost::numeric::ublas::vector_range<const Vector> const_value_reference_type;
  typedef boost::numeric::ublas::vector_range<Vector> mapped_type;
  typedef boost::numeric::ublas::vector_range<const Vector> const_mapped_type;

//  /**
//   * Constructor requires an existing GaussianVariableIndex to get variable
//   * dimensions.
//   */
//  VectorValues(const GaussianVariableIndex& variableIndex);

  /**
   * Default constructor creates an empty VectorValues.  reserve(...) must be
   * called to allocate space before any values can be added.  This prevents
   * slow reallocation of space at runtime.
   */
  VectorValues() : varStarts_(1,0) {}
  VectorValues(const VectorValues &V) : values_(V.values_), varStarts_(V.varStarts_) {}

  /** Construct from a container of variable dimensions (in variable order). */
  template<class Container>
  VectorValues(const Container& dimensions);

  /** Construct to hold nVars vectors of varDim dimension each. */
  VectorValues(Index nVars, size_t varDim);

  /** Construct from a container of variable dimensions in variable order and
   * a combined Vector of all of the variables in order.
   */
  VectorValues(const std::vector<size_t>& dimensions, const Vector& values);

  /** Named constructor to create a VectorValues that matches the structure of
   * the specified VectorValues, but do not initialize the new values.
   */
  static VectorValues SameStructure(const VectorValues& otherValues);

  /** Element access */
  mapped_type operator[](Index variable);
  const_mapped_type operator[](Index variable) const;

  /** Number of elements */
  Index size() const { return varStarts_.size()-1; }

  /** Total dimensionality used (could be smaller than what has been allocated
   * with reserve(...) ).
   */
  size_t dim() const { return varStarts_.back(); }

  /* dot product */
  double dot(const VectorValues& V) const { return gtsam::dot(this->values_, V.values_) ; }


  /** Total dimensions capacity allocated */
  size_t dimCapacity() const { return values_.size(); }

  /** Iterator access */
  iterator begin() { return _impl_iterator<VectorValues>(*this, 0); }
  const_iterator begin() const { return _impl_iterator<const VectorValues>(*this, 0); }
  iterator end() { return _impl_iterator<VectorValues>(*this, varStarts_.size()-1); }
  const_iterator end() const { return _impl_iterator<const VectorValues>(*this, varStarts_.size()-1); }

  /** Reserve space for a total number of variables and dimensionality */
  void reserve(Index nVars, size_t totalDims) { values_.resize(std::max(totalDims, values_.size())); varStarts_.reserve(nVars+1); }

  /**
   * Append a variable using the next variable ID, and return that ID.  Space
   * must have been allocated ahead of time using reserve(...).
   */
  Index push_back_preallocated(const Vector& vector) {
    Index var = varStarts_.size()-1;
    varStarts_.push_back(varStarts_.back()+vector.size());
    this->operator[](var) = vector;  // This will assert that values_ has enough allocated space.
    return var;
  }

  /** Set all elements to zero */
  void makeZero() { boost::numeric::ublas::noalias(values_) = boost::numeric::ublas::zero_vector<double>(values_.size()); }

  /** print required by Testable for unit testing */
  void print(const std::string& str = "VectorValues: ") const {
    std::cout << str << ": " << varStarts_.size()-1 << " elements\n";
    for(Index var=0; var<size(); ++var) {
      std::cout << "  " << var << " " << operator[](var) << "\n";
    }
    std::cout.flush();
  }

  /** equals required by Testable for unit testing */
  bool equals(const VectorValues& expected, double tol=1e-9) const {
    if(size() != expected.size()) return false;
    // iterate over all elements
    for(Index var=0; var<size(); ++var)
      if(!equal_with_abs_tol(expected[var],operator[](var),tol))
        return false;
    return true;
  }

  /** + operator simply adds Vectors.  This checks for structural equivalence
   * when NDEBUG is not defined.
   */
  VectorValues operator+(const VectorValues& c) const {
    assert(varStarts_ == c.varStarts_);
    VectorValues result;
    result.varStarts_ = varStarts_;
    result.values_ = boost::numeric::ublas::project(values_, boost::numeric::ublas::range(0, varStarts_.back())) +
        boost::numeric::ublas::project(c.values_, boost::numeric::ublas::range(0, c.varStarts_.back()));
    return result;
  }

  /**
   * Iterator (handles both iterator and const_iterator depending on whether
   * the template type is const.
   */
  template<class C>
  class _impl_iterator {
  protected:
    C& config_;
    Index curVariable_;

    _impl_iterator(C& config, Index curVariable) : config_(config), curVariable_(curVariable) {}
    void checkCompat(const _impl_iterator<C>& r) { assert(&config_ == &r.config_); }
    friend class VectorValues;

  public:
    typedef typename const_selector<C, VectorValues, VectorValues::mapped_type, VectorValues::const_mapped_type>::type value_type;
    _impl_iterator<C>& operator++() { ++curVariable_; return *this; }
    _impl_iterator<C>& operator--() { --curVariable_; return *this; }
    _impl_iterator<C>& operator++(int) { throw std::runtime_error("Use prefix ++ operator"); }
    _impl_iterator<C>& operator--(int) { throw std::runtime_error("Use prefix -- operator"); }
    _impl_iterator<C>& operator+=(ptrdiff_t step) { curVariable_ += step; return *this; }
    _impl_iterator<C>& operator-=(ptrdiff_t step) { curVariable_ += step; return *this; }
    ptrdiff_t operator-(const _impl_iterator<C>& r) { checkCompat(r); return curVariable_ - r.curVariable_; }
    bool operator==(const _impl_iterator<C>& r) { checkCompat(r); return curVariable_ == r.curVariable_; }
    bool operator!=(const _impl_iterator<C>& r) { checkCompat(r); return curVariable_ != r.curVariable_; }
    value_type operator*() { return config_[curVariable_]; }
  };

protected:
  void checkVariable(Index variable) const { assert(variable < varStarts_.size()-1); }


public:
  friend double dot(const VectorValues& V1, const VectorValues& V2) { return gtsam::dot(V1.values_, V2.values_) ; }
  friend void scal(double alpha, VectorValues& x) {	gtsam::scal(alpha, x.values_) ; }
  friend void axpy(double alpha, const VectorValues& x, VectorValues& y) { gtsam::axpy(alpha, x.values_, y.values_) ; }

};


//inline VectorValues::VectorValues(const GaussianVariableIndex& variableIndex) : varStarts_(variableIndex.size()+1) {
//  size_t varStart = 0;
//  varStarts_[0] = 0;
//  for(Index var=0; var<variableIndex.size(); ++var) {
//    varStart += variableIndex.dim(var);
//    varStarts_[var+1] = varStart;
//  }
//  values_.resize(varStarts_.back(), false);
//}

template<class Container>
inline VectorValues::VectorValues(const Container& dimensions) : varStarts_(dimensions.size()+1) {
  varStarts_[0] = 0;
  size_t varStart = 0;
  Index var = 0;
  BOOST_FOREACH(size_t dim, dimensions) {
    varStarts_[++var] = (varStart += dim);
  }
  values_.resize(varStarts_.back(), false);
}

inline VectorValues::VectorValues(Index nVars, size_t varDim) : varStarts_(nVars+1) {
  varStarts_[0] = 0;
  size_t varStart = 0;
  for(Index j=1; j<=nVars; ++j)
    varStarts_[j] = (varStart += varDim);
  values_.resize(varStarts_.back(), false);
}

inline VectorValues::VectorValues(const std::vector<size_t>& dimensions, const Vector& values) :
    values_(values), varStarts_(dimensions.size()+1) {
  varStarts_[0] = 0;
  size_t varStart = 0;
  Index var = 0;
  BOOST_FOREACH(size_t dim, dimensions) {
    varStarts_[++var] = (varStart += dim);
  }
  assert(varStarts_.back() == values.size());
}

inline VectorValues VectorValues::SameStructure(const VectorValues& otherValues) {
  VectorValues ret;
  ret.varStarts_ = otherValues.varStarts_;
  ret.values_.resize(ret.varStarts_.back(), false);
  return ret;
}

inline VectorValues::mapped_type VectorValues::operator[](Index variable) {
  checkVariable(variable);
  return boost::numeric::ublas::project(values_,
      boost::numeric::ublas::range(varStarts_[variable], varStarts_[variable+1]));
}

inline VectorValues::const_mapped_type VectorValues::operator[](Index variable) const {
  checkVariable(variable);
  return boost::numeric::ublas::project(values_,
      boost::numeric::ublas::range(varStarts_[variable], varStarts_[variable+1]));
}


}

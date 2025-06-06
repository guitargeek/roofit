/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id: RooArgSet.h,v 1.45 2007/08/09 19:55:47 wouter Exp $
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/
#ifndef ROO_ARG_SET
#define ROO_ARG_SET

#include "RooAbsCollection.h"

class RooAbsArg ;
class RooArgList ;

class RooArgSet : public RooAbsCollection {
public:

  // Constructors, assignment etc.
  RooArgSet();

  /// Construct a (non-owning) RooArgSet from one or more
  /// RooFit objects. The set will not own its contents.
  /// \tparam Ts Parameter pack of objects that derive from RooAbsArg or RooFit collections; or a name.
  /// \param arg A RooFit object.
  ///            Note that you can also pass a `double` as first argument
  ///            when constructing a RooArgSet, and another templated
  ///            constructor will be used where a RooConstVar is implicitly
  ///            created from the `double` value.
  /// \param moreArgsOrName Arbitrary number of
  /// - Further RooFit objects that derive from RooAbsArg
  /// - RooFit collections of such objects
  /// - `double`s from which a RooConstVar is implicitly created via `RooFit::RooConst`.
  /// - A name for the set. Given multiple names, the last-given name prevails.
  template<typename... Args_t>
  RooArgSet(const RooAbsArg& arg, Args_t &&... moreArgsOrName)
  /*NB: Making this a delegating constructor led to linker errors with MSVC*/
  {
    // This constructor should cause a failed static_assert if any of the input
    // arguments is a temporary (r-value reference), which will be checked in
    // processArg. This works statically because of the universal reference
    // mechanism with templated functions.
    // Unfortunately, we can't check the first arg, because it's type can't be
    // a template parameter and hence a universal reference can't be used.
    // This problem is solved by introducing another templated constructor below,
    // which accepts a RooAbsArg && as the first argument which is forwarded to
    // be the second argument for this constructor.
    processArgs(arg, std::forward<Args_t>(moreArgsOrName)...);
  }

  /// This constructor will provoke a `static_assert`, because passing a
  /// RooAbsArg as r-value reference is not allowed.
  template<typename... Args_t>
  RooArgSet(RooAbsArg && arg, Args_t &&... moreArgsOrName)
    : RooArgSet{arg, std::move(arg), std::forward<Args_t>(moreArgsOrName)...} {}

  template<typename... Args_t>
  explicit RooArgSet(double arg, Args_t &&... moreArgsOrName) {
    processArgs(arg, std::forward<Args_t>(moreArgsOrName)...);
  }

  /// Construct a (non-owning) RooArgSet from iterators.
  /// \tparam Iterator_t An iterator pointing to RooFit objects or to pointers/references of those.
  /// \param beginIt Iterator to first element to add.
  /// \param endIt Iterator to end of range to be added.
  /// \param name Optional name of the collection.
  template<typename Iterator_t,
      typename value_type = typename std::remove_pointer<typename std::iterator_traits<Iterator_t>::value_type>::type,
      typename = std::enable_if<std::is_convertible<const value_type*, const RooAbsArg*>::value> >
  RooArgSet(Iterator_t beginIt, Iterator_t endIt, const char* name="") :
  RooArgSet(name) {
    for (auto it = beginIt; it != endIt; ++it) {
      processArg(*it);
    }
  }

  /// Construct a non-owning RooArgSet from a vector of RooAbsArg pointers.
  /// This constructor is mainly intended for pyROOT. With cppyy, a Python list
  /// or tuple can be implicitly converted to an std::vector, and by enabling
  /// implicit construction of a RooArgSet from a std::vector, we indirectly
  /// enable implicit conversion from a Python list/tuple to RooArgSets.
  /// \param vec A vector with pointers to the arguments or doubles for RooFit::RooConst().
  RooArgSet(std::vector<RooAbsArgPtrOrDouble> const& vec) {
    for(auto const& arg : vec) {
      if(arg.hasPtr) processArg(arg.ptr);
      else processArg(arg.val);
    }
  }

  RooArgSet(const RooArgSet& other, const char *name="");
  /// Move constructor.
  RooArgSet(RooArgSet && other) : RooAbsCollection(std::move(other)) {}

  RooArgSet(const RooArgSet& set1, const RooArgSet& set2,
            const char *name="");

  RooArgSet(const RooAbsCollection& coll) ;
  RooArgSet(const RooAbsCollection& collection, const RooAbsArg* var1);
  explicit RooArgSet(const TCollection& tcoll, const char* name="") ;
  explicit RooArgSet(const char *name);

  ~RooArgSet() override;
  TObject* clone(const char* newname=nullptr) const override { return new RooArgSet(*this,newname); }
  TObject* create(const char* newname) const override { return new RooArgSet(newname); }
  RooArgSet& operator=(const RooArgSet& other) { RooAbsCollection::operator=(other) ; return *this ;}

  using RooAbsCollection::operator[];
  RooAbsArg& operator[](const TString& str) const;


  /// Shortcut for readFromStream(std::istream&, bool, const char*, const char*, bool), setting
  /// `flagReadAtt` and `section` to 0.
  virtual bool readFromStream(std::istream& is, bool compact, bool verbose=false) {
    // I/O streaming interface (machine readable)
    return readFromStream(is, compact, nullptr, nullptr, verbose) ;
  }
  bool readFromStream(std::istream& is, bool compact, const char* flagReadAtt, const char* section, bool verbose=false) ;
  virtual void writeToStream(std::ostream& os, bool compact, const char* section=nullptr) const;
  void writeToFile(const char* fileName) const ;
  bool readFromFile(const char* fileName, const char* flagReadAtt=nullptr, const char* section=nullptr, bool verbose=false) ;


  /// Check if this exact instance is in this collection.
  bool containsInstance(const RooAbsArg& var) const override {
    return find(var) == &var;
  }

  static void cleanup() ;

  bool isInRange(const char* rangeSpec) ;

  using RooAbsCollection::selectCommon;
  using RooAbsCollection::snapshot;

  /// Use RooAbsCollection::selectByAttrib(), but return as RooArgSet.
  RooArgSet* selectByAttrib(const char* name, bool value) const {
    return static_cast<RooArgSet*>(RooAbsCollection::selectByAttrib(name, value));
  }

  /// Use RooAbsCollection::selectByName(), but return as RooArgSet.
  inline RooArgSet* selectByName(const char* nameList, bool verbose=false) const {
    return static_cast<RooArgSet*>(RooAbsCollection::selectByName(nameList, verbose));
  }

  /// Use RooAbsCollection::selecCommon(), but return as RooArgSet.
  inline RooArgSet* selectCommon(const RooAbsCollection& refColl) const {
    return static_cast<RooArgSet*>(RooAbsCollection::selectCommon(refColl));
  }

  /// Use RooAbsCollection::snapshot(), but return as RooArgSet.
  inline RooArgSet * snapshot(bool deepCopy = true) const {
    return static_cast<RooArgSet*>(RooAbsCollection::snapshot(deepCopy));
  }

protected:
  bool checkForDup(const RooAbsArg& arg, bool silent) const ;
  bool canBeAdded(const RooAbsArg& arg, bool silent) const override {
    return !checkForDup(arg, silent);
  }

private:

  template<typename... Args_t>
  void processArgs(Args_t &&... args) {
    // Expand parameter pack in C++ 11 way:
    int dummy[] = { 0, (processArg(std::forward<Args_t>(args)), 0) ... };
    (void)dummy;
  }
  void processArg(const RooAbsArg& arg) { add(arg); }
  void processArg(const RooAbsArg* arg) { add(*arg); }
  void processArg(RooAbsArg* var) { add(*var); }
  template<class Arg_t>
  void processArg(Arg_t && arg) {
    assert_is_no_temporary(std::forward<Arg_t>(arg));
    add(arg);
  }
  void processArg(const char* name) { _name = name; }
  void processArg(double value);
  void processArg(const RooAbsCollection& coll) { add(coll); if (_name.Length() == 0) _name = coll.GetName(); }
  // this overload with r-value references is needed so we don't trigger the
  // templated function with the failing static_assert for r-value references
  void processArg(RooAbsCollection && coll) { processArg(coll); }
  void processArg(const RooArgList& list);

  ClassDefOverride(RooArgSet,1) // Set of RooAbsArg objects
};


namespace RooFitShortHand {

template<class... Args_t>
RooArgSet S(Args_t&&... args) {
  return {std::forward<Args_t>(args)...};
}

} // namespace RooFitShortHand


#endif

//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// This module contains several datatypes that are to be used together with the MemoryContext.
// With current implementation, we focus on creating STL collection of pointers such as
//   MCVector<MCString *>
//   MCList<MCString *>
// Our SQL processes work on pointers exclusively. For example, the variable nodes that share the
// same name would all be point to the same MCString and also the same entry in our symbol table.
// Eventually, our symbol table should use unique pointer and treenode would contain the raw
// pointers of the unique pointers.
//
// Examples:
// - String type.
//     MCString mc_string(memctx.get(), "abc");
//     mc_string += "xyz";
//
// - STL types.
//     MCVector<int> mc_vec(memctx.get());
//     vec.reserve(77);
//
// - SQL user-defined object.
//     class MyClass : public MCBase {
//     };
//     MyClass *mc_obj = new(memctx.get()) MyClass();
//
// - All of the above instances - mc_string, mc_vec, mc_obj - contain memory context, which
//   can be use to allocate new classes.
//     MCList<int> mc_list(mc_string.memory_context());
//--------------------------------------------------------------------------------------------------
#ifndef YB_UTIL_MEMORY_MC_TYPES_H
#define YB_UTIL_MEMORY_MC_TYPES_H

#include <list>
#include <set>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/tti/has_type.hpp>

#include "yb/util/memory/arena.h"

namespace yb {

//--------------------------------------------------------------------------------------------------
// Buffer (char*) support.
char *MCStrdup(Arena *arena, const char *str);

// Class MCList.
template<class MCObject> using MCList = std::list<MCObject, ArenaAllocator<MCObject>>;

// Class MCVector.
template<class MCObject> using MCVector = std::vector<MCObject, ArenaAllocator<MCObject>>;

// Class MCSet.
template<class MCObject, class Compare = std::less<MCObject>>
using MCSet = std::set<MCObject, Compare, ArenaAllocator<MCObject>>;

// Class MCMap.
template<class MCKey, class MCObject, class Compare = std::less<MCKey>>
using MCMap = std::map<MCKey, MCObject, Compare, ArenaAllocator<MCObject>>;

//--------------------------------------------------------------------------------------------------
// String support.
// To use MCAllocator, strings should be declared as one of the following.
//   MCString s(memctx);
//   MCSharedPtr<MCString> s = MCMakeSharedString(memctx);

typedef std::basic_string<char, std::char_traits<char>, ArenaAllocator<char>> MCString;

typedef Arena MemoryContext;

//--------------------------------------------------------------------------------------------------
// Context-control shared_ptr and unique_ptr
template<class MCObject> using MCUniPtr = std::unique_ptr<MCObject, ArenaObjectDeleter>;
template<class MCObject> using MCSharedPtr = std::shared_ptr<MCObject>;

//--------------------------------------------------------------------------------------------------
// User-defined object support.
// All objects that use MCAllocator should be derived from MCBase. For example:
// class MCMyObject : public MCBase {
// };

BOOST_TTI_HAS_TYPE(allocator_type);

template<class MCObject, typename... TypeArgs>
typename std::enable_if<!has_type_allocator_type<MCObject>::value, MCSharedPtr<MCObject>>::type
MCAllocateSharedHelper(MCObject*, ArenaAllocator<MCObject> allocator, TypeArgs&&... args) {
  return std::allocate_shared<MCObject>(allocator,
                                        allocator.arena(),
                                        std::forward<TypeArgs>(args)...);
}

template<class MCObject, typename... TypeArgs>
typename std::enable_if<has_type_allocator_type<MCObject>::value, MCSharedPtr<MCObject>>::type
MCAllocateSharedHelper(MCObject*, ArenaAllocator<MCObject> allocator, TypeArgs&&... args) {
  return std::allocate_shared<MCObject>(allocator, std::forward<TypeArgs>(args)..., allocator);
}

template<class MCObject, typename... TypeArgs>
MCSharedPtr<MCObject> MCAllocateShared(ArenaAllocator<MCObject> allocator, TypeArgs&&... args) {
  return MCAllocateSharedHelper(static_cast<MCObject*>(nullptr),
                                allocator,
                                std::forward<TypeArgs>(args)...);
}

// Construct a shared_ptr to any MC object.
template<class MCObject, typename... TypeArgs>
MCSharedPtr<MCObject> MCMakeShared(Arena *arena, TypeArgs&&... args) {
  ArenaAllocator<MCObject> allocator(arena);
  return MCAllocateShared<MCObject>(allocator, std::forward<TypeArgs>(args)...);
}

// MC base class.
class MCBase {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<MCBase> SharedPtr;
  typedef MCSharedPtr<const MCBase> SharedPtrConst;

  // Constructors.
  explicit MCBase(Arena *arena = nullptr);
  virtual ~MCBase();

  template<typename... TypeArgs>
  inline static MCBase::SharedPtr MakeShared(Arena *arena, TypeArgs&&... args) {
    return MCMakeShared<MCBase>(arena, std::forward<TypeArgs>(args)...);
  }

  void operator delete(void* ptr) noexcept;
  void *operator new(size_t bytes, Arena* arena) noexcept;

 private:
  //------------------------------------------------------------------------------------------------
  // The following functions are deprecated and not supported for MC objects.

  // Default new/delete operators are disabled.
  void operator delete[](void* ptr) noexcept = delete;
  void *operator new(size_t bytes) noexcept = delete;
  void *operator new[](size_t bytes) noexcept = delete;
};

}  // namespace yb

#endif // YB_UTIL_MEMORY_MC_TYPES_H
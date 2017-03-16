//--------------------------------------------------------------------------------------------------
// Copyright (c) YugaByte, Inc.
//
// Tree node definitions for INSERT statement.
//--------------------------------------------------------------------------------------------------

#ifndef YB_SQL_PTREE_PT_INSERT_H_
#define YB_SQL_PTREE_PT_INSERT_H_

#include "yb/client/client.h"

#include "yb/sql/ptree/list_node.h"
#include "yb/sql/ptree/tree_node.h"
#include "yb/sql/ptree/pt_select.h"
#include "yb/sql/ptree/column_desc.h"
#include "yb/sql/ptree/pt_dml.h"

namespace yb {
namespace sql {

//--------------------------------------------------------------------------------------------------

class PTInsertStmt : public PTDmlStmt {
 public:
  //------------------------------------------------------------------------------------------------
  // Public types.
  typedef MCSharedPtr<PTInsertStmt> SharedPtr;
  typedef MCSharedPtr<const PTInsertStmt> SharedPtrConst;

  //------------------------------------------------------------------------------------------------
  // Constructor and destructor.
  PTInsertStmt(MemoryContext *memctx,
               YBLocation::SharedPtr loc,
               PTQualifiedName::SharedPtr relation,
               PTQualifiedNameListNode::SharedPtr columns,
               PTCollection::SharedPtr value_clause,
               PTExpr::SharedPtr if_clause = nullptr,
               PTConstInt::SharedPtr ttl_seconds = nullptr);
  virtual ~PTInsertStmt();

  template<typename... TypeArgs>
  inline static PTInsertStmt::SharedPtr MakeShared(MemoryContext *memctx,
                                                   TypeArgs&&... args) {
    return MCMakeShared<PTInsertStmt>(memctx, std::forward<TypeArgs>(args)...);
  }

  // Node semantics analysis.
  virtual CHECKED_STATUS Analyze(SemContext *sem_context) OVERRIDE;
  void PrintSemanticAnalysisResult(SemContext *sem_context);

  // Node type.
  virtual TreeNodeOpcode opcode() const OVERRIDE {
    return TreeNodeOpcode::kPTInsertStmt;
  }

  // Table name.
  client::YBTableName table_name() const OVERRIDE {
    return relation_->ToTableName();
  }

  // Returns location of table name.
  const YBLocation& table_loc() const OVERRIDE {
    return relation_->loc();
  }

  // IF clause.
  const PTExpr* if_clause() const {
    return if_clause_.get();
  }

  bool is_system() const OVERRIDE {
    return relation_->is_system();
  }

 private:
  // The parser will constructs the following tree nodes.
  PTQualifiedName::SharedPtr relation_;
  PTQualifiedNameListNode::SharedPtr columns_;
  PTCollection::SharedPtr value_clause_;
  PTExpr::SharedPtr if_clause_;
};

}  // namespace sql
}  // namespace yb

#endif  // YB_SQL_PTREE_PT_INSERT_H_

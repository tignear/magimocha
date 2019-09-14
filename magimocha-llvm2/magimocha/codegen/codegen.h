#include "llvm/IR/Value.h"
#include "magimocha/typing.h"
namespace tig::magimocha::codegen2 {
	struct llvm_values
	{
		virtual void set(std::shared_ptr<ast::typed_data>, llvm::Value*)=0;
		virtual llvm::Value* get(std::shared_ptr<ast::typed_data>)=0;
	};
	struct llvm_values_impl:llvm_values {
		std::unordered_map<std::shared_ptr<ast::typed_data>, llvm::Value*> map;
		llvm::Value* get(std::shared_ptr<ast::typed_data> k) override {
			auto itr=map.find(k);
			if (itr == cend(map)) {
				return nullptr;
			}
			return itr->second;
		}
		void set(std::shared_ptr<ast::typed_data> k, llvm::Value* v)override {
			map.insert_or_assign(k, v);
		}
	};
	struct context {
		std::shared_ptr<ast::make_scope> scope;
		std::shared_ptr<typing::variable_table> vars;
		std::shared_ptr<typing::type_table> types;
		std::shared_ptr<typing::type_schema_table> schemas;
		std::shared_ptr<typing::typed_data_2_variable_table_table> typed_data_2_variable_table_table;
		std::shared_ptr<llvm_values> values;
	};
}
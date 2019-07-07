#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/IRBuilder.h"
#include "magimocha/typing.h"
namespace tig::magimocha::codegen2 {
	struct visitor {
		using R=llvm::Value*;
		std::shared_ptr<typing::variable_table> vars;
		std::shared_ptr<typing::type_table> types;
		std::shared_ptr<typing::type_schema_table> schemas;

		llvm::LLVMContext& the_context;
		llvm::Module* the_module;
		R  operator()(std::shared_ptr<ast::signed_number_literal> l) {
			return llvm::ConstantInt::get(the_context, llvm::APInt(64,l->value(),true));
		}
		R  operator()(std::shared_ptr<ast::unsigned_number_literal> l) {
			return llvm::ConstantInt::get(the_context, llvm::APInt(64, l->value(), false));

		}
		R  operator()(std::shared_ptr<ast::floating_literal> l) {
			llvm::ConstantFP::get(the_context, llvm::APFloat(l->value()));
		}
		R  operator()(std::shared_ptr<ast::string_literal> l) {

		}
		R  operator()(std::shared_ptr<ast::call_name> cn) {

		}
		R  operator()(std::shared_ptr<ast::declaration_variable> dv) {
			//llvm::
		}
		R  operator()(std::shared_ptr<ast::declaration_function> df) {

		}
		R  operator()(std::shared_ptr<ast::apply_function> af) {
		}
		R  operator()(std::shared_ptr<ast::operation>) {
			throw "ILLEGAL STATE";
		}
		R  operator()(std::shared_ptr<ast::expression_block> exprs) {

		}
		R  operator()(std::shared_ptr<ast::named_function> nf) {

		}
	};
}
int main() {

}
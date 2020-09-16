/*
A small patch on top of Z3 to add a command which
explores the state graph (set of all derivatives)
of a regex.
*/

#include "cmd_context/cmd_context.h"
#include "ast/rewriter/th_rewriter.h"
#include "ast/shared_occs.h"
#include "ast/ast_smt_pp.h"
#include "ast/for_each_expr.h"
#include "cmd_context/parametric_cmd.h"
#include "util/scoped_timer.h"
#include "util/scoped_ctrl_c.h"
#include "util/cancel_eh.h"
#include <iomanip>

#include "smt/smt_context.h"
#include "ast/reg_decl_plugins.h"
#include "smt/theory_seq.h"
#include "smt/seq_regex.h"

class explore_derivatives_cmd : public parametric_cmd {
    expr *m_target;
public:
    explore_derivatives_cmd(
        char const * name = "explore_derivatives"
    ) : parametric_cmd(name) {}

    ~explore_derivatives_cmd() override {}

    char const * get_usage() const override { return "<term>"; }

    char const * get_main_descr() const override {
        return "explore all derivatives of the given term, which must be a regular expression.";
    }

    void prepare(cmd_context & ctx) override {
        parametric_cmd::prepare(ctx);
        m_target   = nullptr;
    }

    void init_pdescrs(cmd_context & ctx, param_descrs & p) override {
        th_rewriter::get_param_descrs(p);
        insert_timeout(p);
    }

    cmd_arg_kind next_arg_kind(cmd_context & ctx) const override {
        if (m_target == nullptr) return CPK_EXPR;
        return parametric_cmd::next_arg_kind(ctx);
    }

    void set_next_arg(cmd_context & ctx, expr * arg) override {
        m_target = arg;
    }

    void execute(cmd_context & ctx) override {
        if (m_target == nullptr)
            throw cmd_exception("invalid explore_derivatives command, argument expected");

        // 1. Initialize rewriter
        th_rewriter th_rw(ctx.m(), m_params);
        th_rw.set_solver(alloc(th_solver, ctx));

        // 2. Initialize SMT handle to seq_regex
        smt_params th_seq_params;
        smt::context th_seq_ctx(ctx.m(), th_seq_params);
        smt::theory_seq th_seq(th_seq_ctx);
        smt::seq_regex th_regex(th_seq);

        // 3. Rewrite the expression
        expr_ref r(ctx.m());
        proof_ref _pr(ctx.m());
        try {
            th_rw(m_target, r, _pr);
        }
        catch (z3_error & ex) {
            throw ex;
        }
        catch (z3_exception & ex) {
            ctx.regular_stream()
                << "(error \"simplifier failed: "
                << ex.msg() << "\")" << std::endl;
            r = m_target;
        }
        th_rw.cleanup();

        // For debugging: display rewritten expression
        // ctx.display(ctx.regular_stream(), r);
        // ctx.regular_stream() << std::endl;

        // 4. Explore all derivatives of r
        th_regex.public_explore_all_derivs(r);

    }
};

void install_explore_derivatives_cmd(cmd_context & ctx, char const * cmd_name) {
    ctx.insert(alloc(explore_derivatives_cmd, cmd_name));
}

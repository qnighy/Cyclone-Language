#ifndef _TCENV_H_
#define _TCENV_H_

#include "core.h"
#include "list.h"
#include "set.h"
#include "dict.h"
#include "absyn.h"
#include "position.h"

namespace Tcenv {

using Core;
using List;
using Set;
using Dict;
using Absyn;
using Position;

// Used to tell what an ordinary identifer refers to 
extern enum Resolved {
  VarRes(binding_t); // includes unresolved variant
  StructRes(structdecl);
  EnumRes(enumdecl,enumfield);
  XenumRes(xenumdecl,enumfield);
};
typedef enum Resolved resolved_t;

// Global environments -- what's declared in a global scope 
// Warning: ordinaries should really be abstract so we can ensure that any
// lookup sets the bool field to true!
// FIX: We should tree-shake the type declarations too!
extern struct Genv {
  Set<var>               namespaces;
  Dict<var,structdecl@>  structdecls;
  Dict<var,enumdecl@>    enumdecls;
  Dict<var,xenumdecl@>   xenumdecls;
  Dict<var,typedefdecl>  typedefs; // indirection unneeded b/c no redeclaration
  Dict::Dict<var,$(resolved_t,bool)@> ordinaries; // bool for tree-shaking
  list<list<var>>        availables; // "using" namespaces
};
typedef struct Genv @genv_t;

// Local function environments
extern struct Fenv;
typedef struct Fenv @fenv_t; 

extern enum Jumpee {
  NotLoop_j;
  CaseEnd_j;
  FnEnd_j;
  Stmt_j(stmt);
};
typedef enum Jumpee jumpee_t;

// Models the nesting of the RTCG constructs 
extern enum Frames<`a> {
  Outermost(`a);
  Frame(`a,enum Frames<`a>);
  Hidden(`a,enum Frames<`a>);
};
typedef enum Frames<`a> frames<`a>;

// Type environments 
extern struct Tenv {
  list<var>               ns; // current namespace
  Dict<list<var>,genv_t>  ae; // absolute environment
  Opt_t<frames<fenv_t>>   le; // local environment, == null except in functions
};
typedef struct Tenv @tenv;
typedef struct Tenv @tenv_t; // same as tenv but better highlighting

extern tenv_t tc_init();
extern genv_t empty_genv();
extern fenv_t new_fenv(fndecl);

extern tenv_t enter_ns(tenv_t, var);

extern list<var>         resolve_namespace(tenv_t,seg_t,list<var>);
extern resolved_t        lookup_ordinary(tenv_t,seg_t,qvar);
extern structdecl@       lookup_structdecl(tenv_t,seg_t,qvar);
extern enumdecl@         lookup_enumdecl(tenv_t,seg_t,qvar);
extern Opt_t<xenumdecl@> lookup_xenumdecl(tenv_t,seg_t,qvar);
extern typedefdecl       lookup_typedefdecl(tenv_t,seg_t,qvar);
extern structdecl@       lookup_structdecl_abs(tenv_t,seg_t,qvar);
extern enumdecl@         lookup_enumdecl_abs(tenv_t,seg_t,qvar);
extern Opt_t<xenumdecl@> lookup_xenumdecl_abs(tenv_t,seg_t,qvar);


extern typ  return_typ(tenv_t);

extern tenv_t add_local_var(seg_t,tenv_t,vardecl);
extern tenv_t add_pat_var  (seg_t,tenv_t,vardecl);

extern list<tvar> lookup_type_vars(tenv_t);
extern tenv_t     add_type_vars(seg_t,tenv_t,list<tvar>);

extern tenv_t set_in_loop(tenv_t te, stmt continue_dest);
extern tenv_t set_in_switch(tenv_t);
extern tenv_t set_fallthru(tenv_t te, 
			   $(list<tvar>,list<vardecl>) * pat_typ,
			   stmt body);
extern tenv_t clear_fallthru(tenv_t);
extern tenv_t set_next(tenv_t, jumpee_t);

// The next 4 all assign through their last arg
extern void process_continue(tenv_t,stmt,Opt_t<stmt>*);
extern void process_break   (tenv_t,stmt,Opt_t<stmt>*);
extern void process_goto(tenv_t,stmt,var,Opt_t<stmt>*);
extern $(stmt,list<tvar>,list<typ>)* process_fallthru(tenv_t,stmt,Opt_t<stmt>*);

extern stmt get_encloser(tenv_t);
extern tenv_t set_encloser(tenv_t,stmt);

extern tenv_t add_label(tenv_t, var, stmt);
extern bool all_labels_resolved(tenv_t);

extern tenv_t new_block(tenv_t);
extern int  curr_block(tenv_t);
extern typ  block_to_typ(tenv_t,int);
extern void check_rgn_accessible(tenv_t,seg_t,typ);

// what we synthesize when type-checking a statement or expression:
// This should be abstract, but I'm saving an allocation and a level of
// indirection -- do NOT use this representation information outside of Tcenv! 
extern struct Synth { typ type; };
typedef struct Synth synth; // no longer a pointer (only one field)
// the type of the expression (only meaningful for synth generated by exps)
extern typ synth_typ(synth);
// given a synth, imperatively set the type to t
extern synth synth_set_typ(synth s,typ t);
// synth for an exp that has this type
extern synth standard_synth(tenv_t, typ);
// synth we get on error in expressions (type is wild) or on throw
extern synth wild_synth(tenv_t);
}
#endif

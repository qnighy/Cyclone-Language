#include <setjmp.h>
/* This is a C header used by the output of the Cyclone to
   C translator.  Corresponding definitions are in file lib/runtime_*.c */
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

/* Need one of these per thread (see runtime_stack.c). The runtime maintains 
   a stack that contains either _handler_cons structs or _RegionHandle structs.
   The tag is 0 for a handler_cons and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; 
  struct _RuntimeStack *next;
  void (*cleanup)(struct _RuntimeStack *frame);
};

#ifndef offsetof
/* should be size_t but int is fine */
#define offsetof(t,n) ((int)(&(((t*)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Regions */
struct _RegionPage
{ 
#ifdef CYC_REGION_PROFILE
  unsigned total_bytes;
  unsigned free_bytes;
#endif
  struct _RegionPage *next;
  char data[1];
};

struct _pool;
struct bget_region_key;
struct _RegionAllocFunctions;

struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
#if(defined(__linux__) && defined(__KERNEL__))
  struct _RegionPage *vpage;
#endif 
  struct _RegionAllocFunctions *fcns;
  char               *offset;
  char               *last_plus_one;
  struct _pool *released_ptrs;
  struct bget_region_key *key;
#ifdef CYC_REGION_PROFILE
  const char *name;
#endif
  unsigned used_bytes;
  unsigned wasted_bytes;
};


// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

/* Alias qualifier stuff */
typedef unsigned int _AliasQualHandle_t; // must match aqualt_type() in toc.cyc

struct _RegionHandle _new_region(unsigned int, const char*);
void* _region_malloc(struct _RegionHandle*, _AliasQualHandle_t, unsigned);
void* _region_calloc(struct _RegionHandle*, _AliasQualHandle_t, unsigned t, unsigned n);
void* _region_vmalloc(struct _RegionHandle*, unsigned);
void * _aqual_malloc(_AliasQualHandle_t aq, unsigned int s);
void * _aqual_calloc(_AliasQualHandle_t aq, unsigned int n, unsigned int t);
void _free_region(struct _RegionHandle*);

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
void _push_handler(struct _handler_cons*);
void _push_region(struct _RegionHandle*);
void _npop_handler(int);
void _pop_handler();
void _pop_region();


#ifndef _throw
void* _throw_null_fn(const char*,unsigned);
void* _throw_arraybounds_fn(const char*,unsigned);
void* _throw_badalloc_fn(const char*,unsigned);
void* _throw_match_fn(const char*,unsigned);
void* _throw_assert_fn(const char *,unsigned);
void* _throw_fn(void*,const char*,unsigned);
void* _rethrow(void*);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw_assert() (_throw_assert_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

void* Cyc_Core_get_exn_thrown();
/* Built-in Exceptions */
struct Cyc_Null_Exception_exn_struct { char *tag; };
struct Cyc_Array_bounds_exn_struct { char *tag; };
struct Cyc_Match_Exception_exn_struct { char *tag; };
struct Cyc_Bad_alloc_exn_struct { char *tag; };
struct Cyc_Assert_exn_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];
extern char Cyc_Assert[];

/* Built-in Run-time Checks and company */
#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ typeof(ptr) _cks_null = (ptr); \
     if (!_cks_null) _throw_null(); \
     _cks_null; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index)\
   (((char*)ptr) + (elt_sz)*(index))
#ifdef NO_CYC_NULL_CHECKS
#define _check_known_subscript_null _check_known_subscript_notnull
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr);\
  int _index = (index);\
  if (!_cks_ptr) _throw_null(); \
  _cks_ptr + (elt_sz)*_index; })
#endif
#define _zero_arr_plus_char_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_other_fn(t_sz,orig_x,orig_sz,orig_i,f,l)((orig_x)+(orig_i))
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })

/* _zero_arr_plus_*_fn(x,sz,i,filename,lineno) adds i to zero-terminated ptr
   x that has at least sz elements */
char* _zero_arr_plus_char_fn(char*,unsigned,int,const char*,unsigned);
void* _zero_arr_plus_other_fn(unsigned,void*,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
unsigned _get_zero_arr_size_char(const char*,unsigned);
unsigned _get_zero_arr_size_other(unsigned,const void*,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
// note: must cast result in toc.cyc
void* _zero_arr_inplace_plus_other_fn(unsigned,void**,int,const char*,unsigned);
void* _zero_arr_inplace_plus_post_other_fn(unsigned,void**,int,const char*,unsigned);
#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char**)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char**)(x),(i),__FILE__,__LINE__)
#define _zero_arr_plus_other(t,x,s,i) \
  (_zero_arr_plus_other_fn(t,x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_other(t,x,i) \
  _zero_arr_inplace_plus_other_fn(t,(void**)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_other(t,x,i) \
  _zero_arr_inplace_plus_post_other_fn(t,(void**)(x),(i),__FILE__,__LINE__)

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#define _untag_fat_ptr_check_bound(arr,elt_sz,num_elts) ((arr).curr)
#define _check_fat_at_base(arr) (arr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#define _untag_fat_ptr_check_bound(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char*)0) \
    _throw_arraybounds(); \
  _curr; })
#define _check_fat_at_base(arr) ({ \
  struct _fat_ptr _arr = (arr); \
  if (_arr.base != _arr.curr) _throw_arraybounds(); \
  _arr; })
#endif

#define _tag_fat(tcurr,elt_sz,num_elts) ({ \
  struct _fat_ptr _ans; \
  unsigned _num_elts = (num_elts);\
  _ans.base = _ans.curr = (void*)(tcurr); \
  /* JGM: if we're tagging NULL, ignore num_elts */ \
  _ans.last_plus_one = _ans.base ? (_ans.base + (elt_sz) * _num_elts) : 0; \
  _ans; })

#define _get_fat_size(arr,elt_sz) \
  ({struct _fat_ptr _arr = (arr); \
    unsigned char *_arr_curr=_arr.curr; \
    unsigned char *_arr_last=_arr.last_plus_one; \
    (_arr_curr < _arr.base || _arr_curr >= _arr_last) ? 0 : \
    ((_arr_last - _arr_curr) / (elt_sz));})

#define _fat_ptr_plus(arr,elt_sz,change) ({ \
  struct _fat_ptr _ans = (arr); \
  int _change = (change);\
  _ans.curr += (elt_sz) * _change;\
  _ans; })
#define _fat_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += (elt_sz) * (change);\
  *_arr_ptr; })
#define _fat_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  struct _fat_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += (elt_sz) * (change);\
  _ans; })

//Not a macro since initialization order matters. Defined in runtime_zeroterm.c.
struct _fat_ptr _fat_ptr_decrease_size(struct _fat_ptr,unsigned sz,unsigned numelts);

#ifdef CYC_GC_PTHREAD_REDIRECTS
# define pthread_create GC_pthread_create
# define pthread_sigmask GC_pthread_sigmask
# define pthread_join GC_pthread_join
# define pthread_detach GC_pthread_detach
# define dlopen GC_dlopen
#endif
/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);

#if(defined(__linux__) && defined(__KERNEL__))
void *cyc_vmalloc(unsigned);
void cyc_vfree(void*);
#endif
// bound the allocation size to be < MAX_ALLOC_SIZE. See macros below for usage.
#define MAX_MALLOC_SIZE (1 << 28)
void* _bounded_GC_malloc(int,const char*,int);
void* _bounded_GC_malloc_atomic(int,const char*,int);
void* _bounded_GC_calloc(unsigned,unsigned,const char*,int);
void* _bounded_GC_calloc_atomic(unsigned,unsigned,const char*,int);
/* these macros are overridden below ifdef CYC_REGION_PROFILE */
#ifndef CYC_REGION_PROFILE
#define _cycalloc(n) _bounded_GC_malloc(n,__FILE__,__LINE__)
#define _cycalloc_atomic(n) _bounded_GC_malloc_atomic(n,__FILE__,__LINE__)
#define _cyccalloc(n,s) _bounded_GC_calloc(n,s,__FILE__,__LINE__)
#define _cyccalloc_atomic(n,s) _bounded_GC_calloc_atomic(n,s,__FILE__,__LINE__)
#endif

static inline unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#define _CYC_MAX_REGION_CONST 0
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static inline void*_fast_region_malloc(struct _RegionHandle*r, _AliasQualHandle_t aq, unsigned orig_s) {  
  if (r > (struct _RegionHandle*)_CYC_MAX_REGION_CONST && r->curr != 0) { 
#ifdef CYC_NOALIGN
    unsigned s =  orig_s;
#else
    unsigned s =  (orig_s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT -1)); 
#endif
    char *result; 
    result = r->offset; 
    if (s <= (r->last_plus_one - result)) {
      r->offset = result + s; 
#ifdef CYC_REGION_PROFILE
    r->curr->free_bytes = r->curr->free_bytes - s;
    rgn_total_bytes += s;
#endif
      return result;
    }
  } 
  return _region_malloc(r,aq,orig_s); 
}

//doesn't make sense to fast malloc with reaps
#ifndef DISABLE_REAPS
#define _fast_region_malloc _region_malloc
#endif

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,_AliasQualHandle_t,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,_AliasQualHandle_t,unsigned,unsigned,const char *,const char*,int);
void * _profile_aqual_malloc(_AliasQualHandle_t aq, unsigned int s,const char *file, const char *func, int lineno);
void * _profile_aqual_calloc(_AliasQualHandle_t aq, unsigned int t1,unsigned int t2,const char *file, const char *func, int lineno);
struct _RegionHandle _profile_new_region(unsigned int i, const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
#define _new_region(i,n) _profile_new_region(i,n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,aq,n) _profile_region_malloc(rh,aq,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,aq,n,t) _profile_region_calloc(rh,aq,n,t,__FILE__,__FUNCTION__,__LINE__)
#define _aqual_malloc(aq,n) _profile_aqual_malloc(aq,n,__FILE__,__FUNCTION__,__LINE__)
#define _aqual_calloc(aq,n,t) _profile_aqual_calloc(aq,n,t,__FILE__,__FUNCTION__,__LINE__)
#endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif //CYC_REGION_PROFILE
#endif //_CYC_INCLUDE_H
 struct Cyc_Core_Opt{void*v;};
# 173 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 72 "list.h"
extern struct Cyc_List_List*Cyc_List_rcopy(struct _RegionHandle*,struct Cyc_List_List*);
# 178
extern struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*);
# 184
extern struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_Iter_Iter{void*env;int(*next)(void*,void*);};
# 37 "iter.h"
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;
# 54 "set.h"
extern struct Cyc_Set_Set*Cyc_Set_rempty(struct _RegionHandle*,int(*)(void*,void*));
# 71
extern struct Cyc_Set_Set*Cyc_Set_rinsert(struct _RegionHandle*,struct Cyc_Set_Set*,void*);
# 143
extern struct Cyc_Iter_Iter Cyc_Set_make_iter(struct _RegionHandle*,struct Cyc_Set_Set*);struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};
# 68 "dict.h"
extern struct Cyc_Dict_Dict Cyc_Dict_rempty(struct _RegionHandle*,int(*)(void*,void*));
# 83
extern int Cyc_Dict_member(struct Cyc_Dict_Dict,void*);
# 87
extern struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict,void*,void*);
# 110
extern void*Cyc_Dict_lookup(struct Cyc_Dict_Dict,void*);
# 277 "dict.h"
extern struct Cyc_Dict_Dict Cyc_Dict_rdelete_same(struct Cyc_Dict_Dict,void*);
# 50 "string.h"
extern int Cyc_strptrcmp(struct _fat_ptr*,struct _fat_ptr*);struct Cyc_AssnDef_ExistAssnFn;struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f0;struct _fat_ptr*f1;};
# 140 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*checks_clause;struct Cyc_AssnDef_ExistAssnFn*checks_assn;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_AssnDef_ExistAssnFn*requires_assn;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_AssnDef_ExistAssnFn*ensures_assn;struct Cyc_Absyn_Exp*throws_clause;struct Cyc_AssnDef_ExistAssnFn*throws_assn;struct Cyc_Absyn_Vardecl*return_value;struct Cyc_List_List*arg_vardecls;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};
# 535 "absyn.h"
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U,Cyc_Absyn_Tagof =19U,Cyc_Absyn_UDiv =20U,Cyc_Absyn_UMod =21U,Cyc_Absyn_UGt =22U,Cyc_Absyn_ULt =23U,Cyc_Absyn_UGte =24U,Cyc_Absyn_ULte =25U};
# 542
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
# 560
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Subset_coercion =3U,Cyc_Absyn_Other_coercion =4U};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct _fat_ptr*f0;struct Cyc_Absyn_Tqual f1;void*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f0;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;struct Cyc_Absyn_Exp*rename;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;int escapes;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};
# 932 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
# 946
struct Cyc_Absyn_Tqual Cyc_Absyn_const_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 953
void*Cyc_Absyn_compress(void*);
# 988
extern void*Cyc_Absyn_void_type;
# 1009
extern struct _tuple0*Cyc_Absyn_exn_name;
# 1039
void*Cyc_Absyn_cstar_type(void*,struct Cyc_Absyn_Tqual);
# 1069
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
# 1075
struct Cyc_Absyn_Exp*Cyc_Absyn_true_exp(unsigned);
# 1079
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1085
struct Cyc_Absyn_Exp*Cyc_Absyn_var_exp(struct _tuple0*,unsigned);
# 1109
struct Cyc_Absyn_Exp*Cyc_Absyn_assign_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1111
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1114
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1116
struct Cyc_Absyn_Exp*Cyc_Absyn_fncall_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
# 1122
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1128
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1131
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1153
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
# 1155
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1166
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
# 1179
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
# 1251
struct _tuple0*Cyc_Absyn_binding2qvar(void*);
# 159 "tcutil.h"
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*);
# 37 "warn.h"
void Cyc_Warn_warn(unsigned,struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Stmt_Warn_Warg_struct{int tag;struct Cyc_Absyn_Stmt*f1;};
# 79
void*Cyc_Warn_impos2(struct _fat_ptr);
# 41 "toc.h"
struct _tuple0*Cyc_Toc_temp_var (void);
extern struct _fat_ptr Cyc_Toc_globals;
# 27 "tovc.h"
extern int Cyc_Tovc_elim_array_initializers;
# 73 "tovc.cyc"
int Cyc_Tovc_elim_array_initializers=1;struct Cyc_Tovc_BoxingEnv{struct _RegionHandle*rgn;struct Cyc_List_List*all_locals;struct Cyc_Dict_Dict varmap;struct Cyc_List_List*boundvars;struct Cyc_Set_Set**freevars;};struct Cyc_Tovc_ToExpEnv{struct _RegionHandle*rgn;struct Cyc_List_List*all_locals;struct Cyc_Dict_Dict varmap;struct Cyc_Absyn_Stmt*encloser;struct Cyc_List_List**gen_ds;};struct _tuple11{struct Cyc_List_List*f0;struct Cyc_Absyn_Exp*f1;};
# 101
static struct Cyc_Absyn_Exp*Cyc_Tovc_box_free_vars_exp(struct Cyc_Tovc_BoxingEnv env,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;void*_T3;struct _tuple0*_T4;union Cyc_Absyn_Nmspace _T5;struct _union_Nmspace_Loc_n _T6;unsigned _T7;int(*_T8)(struct Cyc_Dict_Dict,struct _fat_ptr*);int(*_T9)(struct Cyc_Dict_Dict,void*);struct Cyc_Tovc_BoxingEnv _TA;struct Cyc_Dict_Dict _TB;struct _fat_ptr*_TC;int _TD;struct Cyc_Absyn_Exp*_TE;struct Cyc_Absyn_Exp*(*_TF)(struct Cyc_Dict_Dict,struct _fat_ptr*);void*(*_T10)(struct Cyc_Dict_Dict,void*);struct Cyc_Tovc_BoxingEnv _T11;struct Cyc_Dict_Dict _T12;struct _fat_ptr*_T13;struct Cyc_Absyn_Exp*_T14;int _T15;unsigned long _T16;struct _fat_ptr _T17;unsigned long _T18;struct _fat_ptr*_T19;struct _fat_ptr _T1A;unsigned char*_T1B;struct _fat_ptr**_T1C;struct _fat_ptr**_T1D;int _T1E;struct _fat_ptr*_T1F;int _T20;struct Cyc_Tovc_BoxingEnv _T21;struct Cyc_List_List*_T22;struct _fat_ptr*_T23;struct Cyc_Tovc_BoxingEnv _T24;struct Cyc_List_List*_T25;void*_T26;struct Cyc_Absyn_Vardecl*_T27;struct _tuple0*_T28;struct _tuple0 _T29;struct _fat_ptr*_T2A;int _T2B;struct Cyc_Tovc_BoxingEnv _T2C;struct Cyc_List_List*_T2D;struct Cyc_Tovc_BoxingEnv _T2E;struct Cyc_List_List*_T2F;struct Cyc_Tovc_BoxingEnv _T30;struct Cyc_List_List*_T31;void*_T32;struct _fat_ptr*_T33;struct Cyc_Absyn_Vardecl*_T34;struct _tuple0*_T35;struct _tuple0 _T36;struct _fat_ptr*_T37;int _T38;struct Cyc_Tovc_BoxingEnv _T39;struct Cyc_Set_Set**_T3A;struct Cyc_Set_Set*(*_T3B)(struct _RegionHandle*,struct Cyc_Set_Set*,struct Cyc_Absyn_Vardecl*);struct Cyc_Set_Set*(*_T3C)(struct _RegionHandle*,struct Cyc_Set_Set*,void*);struct Cyc_Tovc_BoxingEnv _T3D;struct _RegionHandle*_T3E;struct Cyc_Tovc_BoxingEnv _T3F;struct Cyc_Set_Set**_T40;struct Cyc_Set_Set*_T41;struct Cyc_Absyn_Vardecl*_T42;struct Cyc_Absyn_Vardecl*_T43;int*_T44;int _T45;struct Cyc_Absyn_Vardecl*_T46;struct _tuple0*_T47;struct Cyc_Absyn_Exp*_T48;struct Cyc_Absyn_Exp*_T49;struct Cyc_Tovc_BoxingEnv _T4A;struct Cyc_List_List*_T4B;struct Cyc_Tovc_BoxingEnv _T4C;struct Cyc_List_List*_T4D;struct _fat_ptr*_T4E;struct Cyc_Tovc_BoxingEnv _T4F;struct Cyc_List_List*_T50;void*_T51;struct Cyc_Absyn_Vardecl*_T52;struct _tuple0*_T53;struct _tuple0 _T54;struct _fat_ptr*_T55;int _T56;struct Cyc_Warn_String_Warn_Warg_struct _T57;struct Cyc_Warn_String_Warn_Warg_struct _T58;struct _fat_ptr*_T59;struct Cyc_Warn_String_Warn_Warg_struct _T5A;int(*_T5B)(struct _fat_ptr);void*(*_T5C)(struct _fat_ptr);struct _fat_ptr _T5D;struct Cyc_Tovc_BoxingEnv _T5E;struct Cyc_List_List*_T5F;struct Cyc_Absyn_Exp*_T60;struct Cyc_Absyn_Exp*_T61;struct Cyc_Absyn_Exp*_T62;unsigned _T63;struct Cyc_Absyn_Exp*_T64;struct Cyc_Absyn_Exp*_T65;struct Cyc_Absyn_Exp*_T66;struct Cyc_Absyn_Exp*_T67;struct Cyc_Absyn_Exp*_T68;struct Cyc_Absyn_Exp*_T69;struct Cyc_Absyn_Exp*_T6A;struct Cyc_Absyn_Exp*_T6B;unsigned _T6C;struct Cyc_Absyn_Exp*_T6D;struct Cyc_Absyn_Exp*_T6E;struct Cyc_Absyn_Exp*_T6F;struct Cyc_Absyn_Exp*_T70;struct Cyc_Absyn_Exp*_T71;struct Cyc_Absyn_Exp*_T72;struct Cyc_Absyn_Exp*_T73;struct Cyc_Absyn_Exp*_T74;unsigned _T75;struct Cyc_Absyn_Exp*_T76;struct Cyc_Absyn_Exp*_T77;struct Cyc_Absyn_Exp*_T78;struct Cyc_Absyn_Exp*_T79;struct Cyc_Absyn_Exp*_T7A;struct Cyc_Absyn_Exp*_T7B;struct Cyc_Absyn_Exp*_T7C;unsigned _T7D;struct Cyc_Absyn_Exp*_T7E;struct Cyc_Absyn_Exp*_T7F;struct Cyc_Absyn_Exp*_T80;struct Cyc_Absyn_Exp*_T81;struct Cyc_Absyn_Exp*_T82;struct Cyc_Absyn_Exp*_T83;struct Cyc_Absyn_Exp*_T84;unsigned _T85;struct Cyc_Absyn_Exp*_T86;struct Cyc_Absyn_Exp*_T87;struct Cyc_Absyn_Exp*_T88;struct Cyc_Absyn_Exp*_T89;struct Cyc_Absyn_Exp*_T8A;struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T8B;void*_T8C;struct Cyc_Absyn_Exp*_T8D;unsigned _T8E;struct Cyc_Absyn_Exp*_T8F;struct Cyc_Absyn_Exp*_T90;struct Cyc_Absyn_Exp*_T91;struct Cyc_Absyn_Exp*_T92;struct Cyc_Absyn_Exp*_T93;struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T94;void*_T95;struct Cyc_Absyn_Exp*_T96;unsigned _T97;struct Cyc_Absyn_Exp*_T98;struct Cyc_Absyn_Exp*_T99;struct Cyc_Absyn_Exp*_T9A;struct Cyc_Absyn_Exp*_T9B;struct Cyc_Absyn_Exp*_T9C;struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T9D;struct Cyc_Absyn_Exp**_T9E;struct Cyc_Tovc_BoxingEnv _T9F;struct Cyc_Absyn_Exp**_TA0;struct Cyc_Absyn_Exp*_TA1;struct Cyc_Absyn_Exp**_TA2;struct Cyc_Tovc_BoxingEnv _TA3;struct Cyc_List_List*_TA4;void*_TA5;struct Cyc_Absyn_Exp*_TA6;struct Cyc_List_List*_TA7;struct Cyc_Absyn_Exp*_TA8;struct Cyc_List_List*_TA9;struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_TAA;struct Cyc_Absyn_Exp**_TAB;struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_TAC;struct Cyc_Absyn_Exp**_TAD;struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_TAE;struct Cyc_Absyn_Exp**_TAF;struct Cyc_Tovc_BoxingEnv _TB0;struct Cyc_Absyn_Exp**_TB1;struct Cyc_Absyn_Exp*_TB2;struct Cyc_Absyn_Exp**_TB3;struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_TB4;struct Cyc_Absyn_Exp**_TB5;struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_TB6;struct Cyc_Absyn_Exp**_TB7;struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_TB8;struct Cyc_Absyn_Exp**_TB9;struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_TBA;struct Cyc_Absyn_Exp**_TBB;struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_TBC;struct Cyc_Absyn_Exp**_TBD;struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_TBE;struct Cyc_Absyn_Exp**_TBF;struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_TC0;struct Cyc_Absyn_Exp**_TC1;struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_TC2;struct Cyc_Absyn_Exp**_TC3;struct Cyc_Tovc_BoxingEnv _TC4;struct Cyc_Absyn_Exp**_TC5;struct Cyc_Absyn_Exp*_TC6;struct Cyc_Absyn_Exp**_TC7;struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_TC8;struct Cyc_Absyn_Exp**_TC9;struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_TCA;struct Cyc_Absyn_Exp**_TCB;struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_TCC;struct Cyc_Absyn_Exp**_TCD;struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_TCE;struct Cyc_Absyn_Exp**_TCF;struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_TD0;struct Cyc_Absyn_Exp**_TD1;struct Cyc_Tovc_BoxingEnv _TD2;struct Cyc_Absyn_Exp**_TD3;struct Cyc_Absyn_Exp*_TD4;struct Cyc_Absyn_Exp**_TD5;struct Cyc_Tovc_BoxingEnv _TD6;struct Cyc_List_List*_TD7;void*_TD8;struct _tuple11*_TD9;struct _tuple11 _TDA;struct Cyc_Absyn_Exp*_TDB;struct Cyc_List_List*_TDC;void*_TDD;struct _tuple11*_TDE;struct Cyc_List_List*_TDF;struct Cyc_Warn_String_Warn_Warg_struct _TE0;int(*_TE1)(struct _fat_ptr);void*(*_TE2)(struct _fat_ptr);struct _fat_ptr _TE3;
_TL0: if(1)goto _TL1;else{goto _TL2;}
_TL1: _T0=e;{void*_TE4=_T0->r;struct Cyc_List_List*_TE5;void*_TE6;int _TE7;int _TE8;void*_TE9;void*_TEA;_T1=(int*)_TE4;_T2=*_T1;switch(_T2){case 1:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_TE4;_T3=_TEB->f1;_TEA=(void*)_T3;}{void*b=_TEA;
# 105
struct _tuple0*_TEB=Cyc_Absyn_binding2qvar(b);struct _fat_ptr*_TEC;_T4=(struct _tuple0*)_TEB;_T5=_T4->f0;_T6=_T5.Loc_n;_T7=_T6.tag;if(_T7!=4)goto _TL4;{struct _tuple0 _TED=*_TEB;_TEC=_TED.f1;}{struct _fat_ptr*v=_TEC;_T9=Cyc_Dict_member;{
# 109
int(*_TED)(struct Cyc_Dict_Dict,struct _fat_ptr*)=(int(*)(struct Cyc_Dict_Dict,struct _fat_ptr*))_T9;_T8=_TED;}_TA=env;_TB=_TA.varmap;_TC=v;_TD=_T8(_TB,_TC);if(!_TD)goto _TL6;_TE=e;_T10=Cyc_Dict_lookup;{
struct Cyc_Absyn_Exp*(*_TED)(struct Cyc_Dict_Dict,struct _fat_ptr*)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_Dict_Dict,struct _fat_ptr*))_T10;_TF=_TED;}_T11=env;_T12=_T11.varmap;_T13=v;_T14=_TF(_T12,_T13);_TE->r=_T14->r;goto _TL0;_TL6:{
# 114
int i=0;_TLB: _T15=i;_T16=(unsigned long)_T15;_T17=Cyc_Toc_globals;_T18=_get_fat_size(_T17,sizeof(struct _fat_ptr*));if(_T16 < _T18)goto _TL9;else{goto _TLA;}
_TL9: _T19=v;_T1A=Cyc_Toc_globals;_T1B=_T1A.curr;_T1C=(struct _fat_ptr**)_T1B;_T1D=_check_null(_T1C);_T1E=i;_T1F=_T1D[_T1E];_T20=Cyc_strptrcmp(_T19,_T1F);if(_T20!=0)goto _TLC;
return 0;_TLC:
# 114
 i=i + 1;goto _TLB;_TLA:;}
# 118
_TL11: _T21=env;_T22=_T21.boundvars;if(_T22!=0)goto _TLF;else{goto _TL10;}
_TLF: _T23=v;_T24=env;_T25=_T24.boundvars;_T26=_T25->hd;_T27=(struct Cyc_Absyn_Vardecl*)_T26;_T28=_T27->name;_T29=*_T28;_T2A=_T29.f1;_T2B=Cyc_strptrcmp(_T23,_T2A);if(_T2B!=0)goto _TL12;
return 0;_TL12: _T2C=env;_T2D=_T2C.boundvars;
# 118
env.boundvars=_T2D->tl;goto _TL11;_TL10:
# 122
 _TL17: _T2E=env;_T2F=_T2E.all_locals;if(_T2F!=0)goto _TL15;else{goto _TL16;}
_TL15: _T30=env;_T31=_T30.all_locals;_T32=_T31->hd;{struct Cyc_Absyn_Vardecl*vd=(struct Cyc_Absyn_Vardecl*)_T32;_T33=v;_T34=vd;_T35=_T34->name;_T36=*_T35;_T37=_T36.f1;_T38=
Cyc_strptrcmp(_T33,_T37);if(_T38!=0)goto _TL18;_T39=env;_T3A=_T39.freevars;_T3C=Cyc_Set_rinsert;{
struct Cyc_Set_Set*(*_TED)(struct _RegionHandle*,struct Cyc_Set_Set*,struct Cyc_Absyn_Vardecl*)=(struct Cyc_Set_Set*(*)(struct _RegionHandle*,struct Cyc_Set_Set*,struct Cyc_Absyn_Vardecl*))_T3C;_T3B=_TED;}_T3D=env;_T3E=_T3D.rgn;_T3F=env;_T40=_T3F.freevars;_T41=*_T40;_T42=vd;*_T3A=_T3B(_T3E,_T41,_T42);_T43=vd;{
# 127
void*_TED=_T43->type;_T44=(int*)_TED;_T45=*_T44;if(_T45!=5)goto _TL1A;
return 0;_TL1A: _T46=vd;_T47=_T46->name;_T48=
Cyc_Absyn_var_exp(_T47,0U);_T49=Cyc_Absyn_deref_exp(_T48,0U);return _T49;;}goto _TL19;_TL18: _TL19:;}_T4A=env;_T4B=_T4A.all_locals;
# 122
env.all_locals=_T4B->tl;goto _TL17;_TL16:
# 136
 _TL1F: _T4C=env;_T4D=_T4C.all_locals;if(_T4D!=0)goto _TL1D;else{goto _TL1E;}
_TL1D: _T4E=v;_T4F=env;_T50=_T4F.all_locals;_T51=_T50->hd;_T52=(struct Cyc_Absyn_Vardecl*)_T51;_T53=_T52->name;_T54=*_T53;_T55=_T54.f1;_T56=Cyc_strptrcmp(_T4E,_T55);if(_T56!=0)goto _TL20;{struct Cyc_Warn_String_Warn_Warg_struct _TED;_TED.tag=0;
_TED.f1=_tag_fat("unbound variable ",sizeof(char),18U);_T57=_TED;}{struct Cyc_Warn_String_Warn_Warg_struct _TED=_T57;{struct Cyc_Warn_String_Warn_Warg_struct _TEE;_TEE.tag=0;_T59=v;_TEE.f1=*_T59;_T58=_TEE;}{struct Cyc_Warn_String_Warn_Warg_struct _TEE=_T58;{struct Cyc_Warn_String_Warn_Warg_struct _TEF;_TEF.tag=0;_TEF.f1=_tag_fat(" found in box_free_vars_exp",sizeof(char),28U);_T5A=_TEF;}{struct Cyc_Warn_String_Warn_Warg_struct _TEF=_T5A;void*_TF0[3];_TF0[0]=& _TED;_TF0[1]=& _TEE;_TF0[2]=& _TEF;_T5C=Cyc_Warn_impos2;{int(*_TF1)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T5C;_T5B=_TF1;}_T5D=_tag_fat(_TF0,sizeof(void*),3);_T5B(_T5D);}}}goto _TL21;_TL20: _TL21: _T5E=env;_T5F=_T5E.all_locals;
# 136
env.all_locals=_T5F->tl;goto _TL1F;_TL1E:
# 142
 return 0;}_TL4:
 return 0;;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_TE4;_TEA=_TEB->f1;_TE9=_TEB->f2;}{struct Cyc_Absyn_Exp*e1=_TEA;struct Cyc_Absyn_Exp*e2=_TE9;
# 149
struct Cyc_Absyn_Exp*e1b=Cyc_Tovc_box_free_vars_exp(env,e1);
struct Cyc_Absyn_Exp*e2b=Cyc_Tovc_box_free_vars_exp(env,e2);
if(e1b==0)goto _TL22;
if(e2b==0)goto _TL24;_T60=e1b;_T61=e2b;_T62=e;_T63=_T62->loc;{
struct Cyc_Absyn_Exp*eb=Cyc_Absyn_subscript_exp(_T60,_T61,_T63);_T64=eb;_T65=e;
_T64->topt=_T65->topt;_T66=eb;_T67=e;
_T66->annot=_T67->annot;_T68=eb;
return _T68;}
# 159
_TL24: _T69=e1b;_T6A=e2;_T6B=e;_T6C=_T6B->loc;{struct Cyc_Absyn_Exp*eb=Cyc_Absyn_subscript_exp(_T69,_T6A,_T6C);_T6D=eb;_T6E=e;
_T6D->topt=_T6E->topt;_T6F=eb;_T70=e;
_T6F->annot=_T70->annot;_T71=eb;
return _T71;}
# 164
_TL22: if(e2b==0)goto _TL26;_T72=e1;_T73=e2b;_T74=e;_T75=_T74->loc;{
struct Cyc_Absyn_Exp*eb=Cyc_Absyn_subscript_exp(_T72,_T73,_T75);_T76=eb;_T77=e;
_T76->topt=_T77->topt;_T78=eb;_T79=e;
_T78->annot=_T79->annot;_T7A=eb;
return _T7A;}_TL26:
# 170
 return 0;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_TE4;_TEA=_TEB->f1;}{struct Cyc_Absyn_Exp*e1=_TEA;
# 173
struct Cyc_Absyn_Exp*e1b=Cyc_Tovc_box_free_vars_exp(env,e1);
if(e1b==0)goto _TL28;_T7B=e1b;_T7C=e;_T7D=_T7C->loc;{
struct Cyc_Absyn_Exp*eb=Cyc_Absyn_address_exp(_T7B,_T7D);_T7E=eb;_T7F=e;
_T7E->topt=_T7F->topt;_T80=eb;_T81=e;
_T80->annot=_T81->annot;_T82=eb;
return _T82;}_TL28:
# 180
 return 0;}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_TE4;_TEA=_TEB->f1;}{struct Cyc_Absyn_Exp*e1=_TEA;
# 183
struct Cyc_Absyn_Exp*e1b=Cyc_Tovc_box_free_vars_exp(env,e1);
if(e1b==0)goto _TL2A;_T83=e1b;_T84=e;_T85=_T84->loc;{
struct Cyc_Absyn_Exp*eb=Cyc_Absyn_deref_exp(_T83,_T85);_T86=eb;_T87=e;
_T86->topt=_T87->topt;_T88=eb;_T89=e;
_T88->annot=_T89->annot;_T8A=eb;
return _T8A;}_TL2A:
# 190
 return 0;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_TE4;_TEA=_TEB->f1;_TE9=_TEB->f2;_TE8=_TEB->f3;_TE7=_TEB->f4;}{struct Cyc_Absyn_Exp*e1=_TEA;struct _fat_ptr*fn=_TE9;int ist=_TE8;int isr=_TE7;
# 193
struct Cyc_Absyn_Exp*e1b=Cyc_Tovc_box_free_vars_exp(env,e1);
if(e1b==0)goto _TL2C;{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_TEB=_cycalloc(sizeof(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct));_TEB->tag=23;
_TEB->f1=e1b;_TEB->f2=fn;_TEB->f3=ist;_TEB->f4=isr;_T8B=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_TEB;}_T8C=(void*)_T8B;_T8D=e;_T8E=_T8D->loc;{struct Cyc_Absyn_Exp*eb=Cyc_Absyn_new_exp(_T8C,_T8E);_T8F=eb;_T90=e;
_T8F->topt=_T90->topt;_T91=eb;_T92=e;
_T91->annot=_T92->annot;_T93=eb;
return _T93;}_TL2C:
# 200
 return 0;}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_TE4;_TEA=_TEB->f1;_TE9=_TEB->f2;_TE8=_TEB->f3;_TE7=_TEB->f4;}{struct Cyc_Absyn_Exp*e1=_TEA;struct _fat_ptr*fn=_TE9;int ist=_TE8;int isr=_TE7;
# 203
struct Cyc_Absyn_Exp*e1b=Cyc_Tovc_box_free_vars_exp(env,e1);
if(e1b==0)goto _TL2E;{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_TEB=_cycalloc(sizeof(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct));_TEB->tag=24;
_TEB->f1=e1b;_TEB->f2=fn;_TEB->f3=ist;_TEB->f4=isr;_T94=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_TEB;}_T95=(void*)_T94;_T96=e;_T97=_T96->loc;{struct Cyc_Absyn_Exp*eb=Cyc_Absyn_new_exp(_T95,_T97);_T98=eb;_T99=e;
_T98->topt=_T99->topt;_T9A=eb;_T9B=e;
_T9A->annot=_T9B->annot;_T9C=eb;
return _T9C;}_TL2E:
# 210
 return 0;}case 0:
# 213
 return 0;case 10:{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_TE4;_T9D=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_TE4;_T9E=& _T9D->f1;_TEA=(struct Cyc_Absyn_Exp**)_T9E;_TE9=_TEB->f2;}{struct Cyc_Absyn_Exp**e1=_TEA;struct Cyc_List_List*es=_TE9;_T9F=env;_TA0=e1;_TA1=*_TA0;{
# 215
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T9F,_TA1);if(__ei==0)goto _TL30;_TA2=e1;*_TA2=__ei;goto _TL31;_TL30: _TL31:;}_TEA=es;goto _LL12;}case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_TE4;_TEA=_TEB->f2;}_LL12: {struct Cyc_List_List*es=_TEA;
# 218
_TL35: if(es!=0)goto _TL33;else{goto _TL34;}
_TL33: _TA3=env;_TA4=es;_TA5=_TA4->hd;_TA6=(struct Cyc_Absyn_Exp*)_TA5;{struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_TA3,_TA6);if(__ei==0)goto _TL36;_TA7=es;_TA8=__ei;_TA7->hd=(void*)_TA8;goto _TL37;_TL36: _TL37:;}_TA9=es;
# 218
es=_TA9->tl;goto _TL35;_TL34:
# 221
 return 0;}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_TE4;_TAA=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_TE4;_TAB=& _TAA->f1;_TEA=(struct Cyc_Absyn_Exp**)_TAB;_TAC=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_TE4;_TAD=& _TAC->f2;_TE9=(struct Cyc_Absyn_Exp**)_TAD;_TAE=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_TE4;_TAF=& _TAE->f3;_TE6=(struct Cyc_Absyn_Exp**)_TAF;}{struct Cyc_Absyn_Exp**e1=_TEA;struct Cyc_Absyn_Exp**e2=_TE9;struct Cyc_Absyn_Exp**e3=(struct Cyc_Absyn_Exp**)_TE6;_TB0=env;_TB1=e3;_TB2=*_TB1;{
# 224
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_TB0,_TB2);if(__ei==0)goto _TL38;_TB3=e3;*_TB3=__ei;goto _TL39;_TL38: _TL39:;}_TEA=e1;_TE9=e2;goto _LL16;}case 7:{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_TE4;_TB4=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_TE4;_TB5=& _TB4->f1;_TEA=(struct Cyc_Absyn_Exp**)_TB5;_TB6=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_TE4;_TB7=& _TB6->f2;_TE9=(struct Cyc_Absyn_Exp**)_TB7;}_LL16: {struct Cyc_Absyn_Exp**e1=_TEA;struct Cyc_Absyn_Exp**e2=_TE9;_TEA=e1;_TE9=e2;goto _LL18;}case 8:{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_TE4;_TB8=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_TE4;_TB9=& _TB8->f1;_TEA=(struct Cyc_Absyn_Exp**)_TB9;_TBA=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_TE4;_TBB=& _TBA->f2;_TE9=(struct Cyc_Absyn_Exp**)_TBB;}_LL18: {struct Cyc_Absyn_Exp**e1=_TEA;struct Cyc_Absyn_Exp**e2=_TE9;_TEA=e1;_TE9=e2;goto _LL1A;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_TE4;_TBC=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_TE4;_TBD=& _TBC->f1;_TEA=(struct Cyc_Absyn_Exp**)_TBD;_TBE=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_TE4;_TBF=& _TBE->f2;_TE9=(struct Cyc_Absyn_Exp**)_TBF;}_LL1A: {struct Cyc_Absyn_Exp**e1=_TEA;struct Cyc_Absyn_Exp**e2=_TE9;_TEA=e1;_TE9=e2;goto _LL1C;}case 4:{struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_TE4;_TC0=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_TE4;_TC1=& _TC0->f1;_TEA=(struct Cyc_Absyn_Exp**)_TC1;_TC2=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_TE4;_TC3=& _TC2->f3;_TE9=(struct Cyc_Absyn_Exp**)_TC3;}_LL1C: {struct Cyc_Absyn_Exp**e1=_TEA;struct Cyc_Absyn_Exp**e2=(struct Cyc_Absyn_Exp**)_TE9;_TC4=env;_TC5=e2;_TC6=*_TC5;{
# 230
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_TC4,_TC6);if(__ei==0)goto _TL3A;_TC7=e2;*_TC7=__ei;goto _TL3B;_TL3A: _TL3B:;}_TEA=e1;goto _LL1E;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_TE4;_TC8=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_TE4;_TC9=& _TC8->f1;_TEA=(struct Cyc_Absyn_Exp**)_TC9;}_LL1E: {struct Cyc_Absyn_Exp**e1=_TEA;_TEA=e1;goto _LL20;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_TE4;_TCA=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_TE4;_TCB=& _TCA->f1;_TEA=(struct Cyc_Absyn_Exp**)_TCB;}_LL20: {struct Cyc_Absyn_Exp**e1=_TEA;_TEA=e1;goto _LL22;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_TE4;_TCC=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_TE4;_TCD=& _TCC->f2;_TEA=(struct Cyc_Absyn_Exp**)_TCD;}_LL22: {struct Cyc_Absyn_Exp**e1=_TEA;_TEA=e1;goto _LL24;}case 18:{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_TE4;_TCE=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_TE4;_TCF=& _TCE->f1;_TEA=(struct Cyc_Absyn_Exp**)_TCF;}_LL24: {struct Cyc_Absyn_Exp**e1=_TEA;_TEA=e1;goto _LL26;}case 5:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_TE4;_TD0=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_TE4;_TD1=& _TD0->f1;_TEA=(struct Cyc_Absyn_Exp**)_TD1;}_LL26: {struct Cyc_Absyn_Exp**e1=(struct Cyc_Absyn_Exp**)_TEA;_TD2=env;_TD3=e1;_TD4=*_TD3;{
# 238
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_TD2,_TD4);if(__ei==0)goto _TL3C;_TD5=e1;*_TD5=__ei;goto _TL3D;_TL3C: _TL3D:;}
return 0;}case 17: goto _LL2A;case 21: _LL2A: goto _LL2C;case 33: _LL2C:
# 243
 return 0;case 37:{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_TEB=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_TE4;_TE5=_TEB->f2;}{struct Cyc_List_List*dles=_TE5;
# 246
_TL41: if(dles!=0)goto _TL3F;else{goto _TL40;}
_TL3F: _TD6=env;_TD7=dles;_TD8=_TD7->hd;_TD9=(struct _tuple11*)_TD8;_TDA=*_TD9;_TDB=_TDA.f1;{struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_TD6,_TDB);if(__ei==0)goto _TL42;_TDC=dles;_TDD=_TDC->hd;_TDE=(struct _tuple11*)_TDD;(*_TDE).f1=__ei;goto _TL43;_TL42: _TL43:;}_TDF=dles;
# 246
dles=_TDF->tl;goto _TL41;_TL40:
# 249
 return 0;}default:{struct Cyc_Warn_String_Warn_Warg_struct _TEB;_TEB.tag=0;
# 251
_TEB.f1=_tag_fat("bad exp form in Tocv::box_free_vars_exp",sizeof(char),40U);_TE0=_TEB;}{struct Cyc_Warn_String_Warn_Warg_struct _TEB=_TE0;void*_TEC[1];_TEC[0]=& _TEB;_TE2=Cyc_Warn_impos2;{int(*_TED)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_TE2;_TE1=_TED;}_TE3=_tag_fat(_TEC,sizeof(void*),1);_TE1(_TE3);}};}goto _TL0;_TL2:;}
# 255
static void Cyc_Tovc_box_free_vars_stmt(struct Cyc_Tovc_BoxingEnv env,struct Cyc_Absyn_Stmt*s){struct Cyc_Absyn_Stmt*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Stmt*_T3;unsigned _T4;struct _fat_ptr _T5;struct _fat_ptr _T6;struct Cyc_Absyn_Stmt*_T7;unsigned _T8;struct _fat_ptr _T9;struct _fat_ptr _TA;struct Cyc_Absyn_Stmt*_TB;unsigned _TC;struct _fat_ptr _TD;struct _fat_ptr _TE;struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_TF;struct Cyc_Absyn_Exp**_T10;struct Cyc_Absyn_Stmt*_T11;unsigned _T12;struct _fat_ptr _T13;struct _fat_ptr _T14;struct Cyc_Absyn_Exp**_T15;struct Cyc_Absyn_Exp*_T16;struct Cyc_Tovc_BoxingEnv _T17;struct Cyc_Absyn_Exp**_T18;struct Cyc_Absyn_Exp*_T19;struct Cyc_Absyn_Exp**_T1A;struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T1B;struct Cyc_Absyn_Exp**_T1C;struct Cyc_Tovc_BoxingEnv _T1D;struct Cyc_Absyn_Exp**_T1E;struct Cyc_Absyn_Exp*_T1F;struct Cyc_Absyn_Exp**_T20;struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_T21;struct Cyc_Absyn_Exp**_T22;struct Cyc_Tovc_BoxingEnv _T23;struct Cyc_Absyn_Exp**_T24;struct Cyc_Absyn_Exp*_T25;struct Cyc_Absyn_Exp**_T26;struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*_T27;struct Cyc_Absyn_Exp**_T28;struct Cyc_Tovc_BoxingEnv _T29;struct Cyc_Absyn_Exp**_T2A;struct Cyc_Absyn_Exp*_T2B;struct Cyc_Absyn_Exp**_T2C;struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T2D;struct Cyc_Absyn_Exp**_T2E;struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T2F;struct Cyc_Absyn_Exp**_T30;struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T31;struct Cyc_Absyn_Exp**_T32;struct Cyc_Tovc_BoxingEnv _T33;struct Cyc_Absyn_Exp**_T34;struct Cyc_Absyn_Exp*_T35;struct Cyc_Absyn_Exp**_T36;struct Cyc_Tovc_BoxingEnv _T37;struct Cyc_Absyn_Exp**_T38;struct Cyc_Absyn_Exp*_T39;struct Cyc_Absyn_Exp**_T3A;struct Cyc_Tovc_BoxingEnv _T3B;struct Cyc_Absyn_Exp**_T3C;struct Cyc_Absyn_Exp*_T3D;struct Cyc_Absyn_Exp**_T3E;struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*_T3F;struct Cyc_Absyn_Exp**_T40;struct Cyc_Tovc_BoxingEnv _T41;struct Cyc_Absyn_Exp**_T42;struct Cyc_Absyn_Exp*_T43;struct Cyc_Absyn_Exp**_T44;struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_T45;struct Cyc_Absyn_Exp**_T46;struct Cyc_Tovc_BoxingEnv _T47;struct Cyc_Absyn_Exp**_T48;struct Cyc_Absyn_Exp*_T49;struct Cyc_Absyn_Exp**_T4A;struct Cyc_Tovc_BoxingEnv _T4B;struct Cyc_List_List*_T4C;void*_T4D;struct Cyc_Absyn_Switch_clause*_T4E;struct Cyc_Absyn_Stmt*_T4F;struct Cyc_List_List*_T50;struct Cyc_Absyn_Decl*_T51;int*_T52;int _T53;struct Cyc_List_List*_T54;struct Cyc_Tovc_BoxingEnv _T55;struct _RegionHandle*_T56;struct Cyc_Tovc_BoxingEnv _T57;struct Cyc_Dict_Dict(*_T58)(struct Cyc_Dict_Dict,struct _fat_ptr*);struct Cyc_Dict_Dict(*_T59)(struct Cyc_Dict_Dict,void*);struct Cyc_Tovc_BoxingEnv _T5A;struct Cyc_Dict_Dict _T5B;struct Cyc_Absyn_Vardecl*_T5C;struct _tuple0*_T5D;struct _tuple0 _T5E;struct _fat_ptr*_T5F;struct Cyc_Absyn_Vardecl*_T60;struct Cyc_Absyn_Exp*_T61;struct Cyc_Tovc_BoxingEnv _T62;struct Cyc_Absyn_Vardecl*_T63;struct Cyc_Absyn_Exp*_T64;struct Cyc_Absyn_Vardecl*_T65;struct Cyc_Warn_String_Warn_Warg_struct _T66;int(*_T67)(struct _fat_ptr);void*(*_T68)(struct _fat_ptr);struct _fat_ptr _T69;
# 259
_TL44: if(1)goto _TL45;else{goto _TL46;}
_TL45: _T0=s;{void*_T6A=_T0->r;struct Cyc_Absyn_Decl*_T6B;struct Cyc_List_List*_T6C;struct Cyc_Absyn_Stmt*_T6D;void*_T6E;void*_T6F;void*_T70;_T1=(int*)_T6A;_T2=*_T1;switch(_T2){case 0:
 return;case 6: _T3=s;_T4=_T3->loc;_T5=
# 263
_tag_fat("break in eliminated statement expression",sizeof(char),41U);_T6=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_warn(_T4,_T5,_T6);
return;case 7: _T7=s;_T8=_T7->loc;_T9=
# 266
_tag_fat("continue in eliminated statement expression",sizeof(char),44U);_TA=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_warn(_T8,_T9,_TA);
# 269
return;case 8: _TB=s;_TC=_TB->loc;_TD=
# 271
_tag_fat("goto in eliminated statement expression",sizeof(char),40U);_TE=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_warn(_TC,_TD,_TE);
return;case 13:{struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_T6A;_T70=_T71->f2;}{struct Cyc_Absyn_Stmt*s2=_T70;
# 274
s=s2;goto _TL44;}case 3:{struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_T6A;_TF=(struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_T6A;_T10=& _TF->f1;_T70=(struct Cyc_Absyn_Exp**)_T10;}{struct Cyc_Absyn_Exp**eopt=_T70;_T11=s;_T12=_T11->loc;_T13=
# 277
_tag_fat("return in eliminated statement expression",sizeof(char),42U);_T14=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_warn(_T12,_T13,_T14);_T15=eopt;_T16=*_T15;
# 279
if(_T16==0)goto _TL48;_T17=env;_T18=eopt;_T19=*_T18;{
struct Cyc_Absyn_Exp*ei=Cyc_Tovc_box_free_vars_exp(_T17,_T19);
if(ei==0)goto _TL4A;_T1A=eopt;*_T1A=ei;goto _TL4B;_TL4A: _TL4B:;}goto _TL49;_TL48: _TL49:
# 283
 return;}case 1:{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T6A;_T1B=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T6A;_T1C=& _T1B->f1;_T70=(struct Cyc_Absyn_Exp**)_T1C;}{struct Cyc_Absyn_Exp**e=_T70;_T1D=env;_T1E=e;_T1F=*_T1E;{
# 285
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T1D,_T1F);if(__ei==0)goto _TL4C;_T20=e;*_T20=__ei;goto _TL4D;_TL4C: _TL4D:;}
return;}case 2:{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T6A;_T70=_T71->f1;_T6F=_T71->f2;}{struct Cyc_Absyn_Stmt*s1=_T70;struct Cyc_Absyn_Stmt*s2=_T6F;
# 288
Cyc_Tovc_box_free_vars_stmt(env,s1);
s=s2;goto _TL44;}case 4:{struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_T6A;_T21=(struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_T6A;_T22=& _T21->f1;_T70=(struct Cyc_Absyn_Exp**)_T22;_T6F=_T71->f2;_T6E=_T71->f3;}{struct Cyc_Absyn_Exp**e=_T70;struct Cyc_Absyn_Stmt*s1=_T6F;struct Cyc_Absyn_Stmt*s2=_T6E;_T23=env;_T24=e;_T25=*_T24;{
# 292
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T23,_T25);if(__ei==0)goto _TL4E;_T26=e;*_T26=__ei;goto _TL4F;_TL4E: _TL4F:;}
Cyc_Tovc_box_free_vars_stmt(env,s1);
s=s2;goto _TL44;}case 5:{struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_T6A;_T27=(struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_T6A;_T28=& _T27->f1.f0;_T70=(struct Cyc_Absyn_Exp**)_T28;_T6F=_T71->f2;}{struct Cyc_Absyn_Exp**e=_T70;struct Cyc_Absyn_Stmt*s2=_T6F;_T29=env;_T2A=e;_T2B=*_T2A;{
# 297
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T29,_T2B);if(__ei==0)goto _TL50;_T2C=e;*_T2C=__ei;goto _TL51;_TL50: _TL51:;}
s=s2;goto _TL44;}case 9:{struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T6A;_T2D=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T6A;_T2E=& _T2D->f1;_T70=(struct Cyc_Absyn_Exp**)_T2E;_T2F=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T6A;_T30=& _T2F->f2.f0;_T6F=(struct Cyc_Absyn_Exp**)_T30;_T31=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T6A;_T32=& _T31->f3.f0;_T6E=(struct Cyc_Absyn_Exp**)_T32;_T6D=_T71->f4;}{struct Cyc_Absyn_Exp**e1=_T70;struct Cyc_Absyn_Exp**e2=_T6F;struct Cyc_Absyn_Exp**e3=(struct Cyc_Absyn_Exp**)_T6E;struct Cyc_Absyn_Stmt*s2=_T6D;_T33=env;_T34=e1;_T35=*_T34;{
# 301
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T33,_T35);if(__ei==0)goto _TL52;_T36=e1;*_T36=__ei;goto _TL53;_TL52: _TL53:;}_T37=env;_T38=e2;_T39=*_T38;{
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T37,_T39);if(__ei==0)goto _TL54;_T3A=e2;*_T3A=__ei;goto _TL55;_TL54: _TL55:;}_T3B=env;_T3C=e3;_T3D=*_T3C;{
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T3B,_T3D);if(__ei==0)goto _TL56;_T3E=e3;*_T3E=__ei;goto _TL57;_TL56: _TL57:;}
s=s2;goto _TL44;}case 14:{struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_T6A;_T70=_T71->f1;_T3F=(struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_T6A;_T40=& _T3F->f2.f0;_T6F=(struct Cyc_Absyn_Exp**)_T40;}{struct Cyc_Absyn_Stmt*s2=_T70;struct Cyc_Absyn_Exp**e=(struct Cyc_Absyn_Exp**)_T6F;_T41=env;_T42=e;_T43=*_T42;{
# 307
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T41,_T43);if(__ei==0)goto _TL58;_T44=e;*_T44=__ei;goto _TL59;_TL58: _TL59:;}
s=s2;goto _TL44;}case 10:{struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_T6A;_T45=(struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_T6A;_T46=& _T45->f1;_T70=(struct Cyc_Absyn_Exp**)_T46;_T6C=_T71->f2;}{struct Cyc_Absyn_Exp**e=(struct Cyc_Absyn_Exp**)_T70;struct Cyc_List_List*scs=_T6C;_T47=env;_T48=e;_T49=*_T48;{
# 312
struct Cyc_Absyn_Exp*__ei=Cyc_Tovc_box_free_vars_exp(_T47,_T49);if(__ei==0)goto _TL5A;_T4A=e;*_T4A=__ei;goto _TL5B;_TL5A: _TL5B:;}
_TL5F: if(scs!=0)goto _TL5D;else{goto _TL5E;}
_TL5D: _T4B=env;_T4C=scs;_T4D=_T4C->hd;_T4E=(struct Cyc_Absyn_Switch_clause*)_T4D;_T4F=_T4E->body;Cyc_Tovc_box_free_vars_stmt(_T4B,_T4F);_T50=scs;
# 313
scs=_T50->tl;goto _TL5F;_TL5E:
# 315
 return;}case 12:{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T71=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T6A;_T6B=_T71->f1;_T6D=_T71->f2;}{struct Cyc_Absyn_Decl*d=_T6B;struct Cyc_Absyn_Stmt*s2=_T6D;_T51=d;{
# 317
void*_T71=_T51->r;struct Cyc_Absyn_Vardecl*_T72;_T52=(int*)_T71;_T53=*_T52;if(_T53!=0)goto _TL60;{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T73=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T71;_T72=_T73->f1;}{struct Cyc_Absyn_Vardecl*vd=_T72;_T55=env;_T56=_T55.rgn;{struct Cyc_List_List*_T73=_region_malloc(_T56,0U,sizeof(struct Cyc_List_List));
# 319
_T73->hd=vd;_T57=env;_T73->tl=_T57.boundvars;_T54=(struct Cyc_List_List*)_T73;}env.boundvars=_T54;_T59=Cyc_Dict_rdelete_same;{
struct Cyc_Dict_Dict(*_T73)(struct Cyc_Dict_Dict,struct _fat_ptr*)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _fat_ptr*))_T59;_T58=_T73;}_T5A=env;_T5B=_T5A.varmap;_T5C=vd;_T5D=_T5C->name;_T5E=*_T5D;_T5F=_T5E.f1;env.varmap=_T58(_T5B,_T5F);_T60=vd;_T61=_T60->initializer;
if(_T61==0)goto _TL62;_T62=env;_T63=vd;_T64=_T63->initializer;{
struct Cyc_Absyn_Exp*ei=Cyc_Tovc_box_free_vars_exp(_T62,_T64);
if(ei==0)goto _TL64;_T65=vd;_T65->initializer=ei;goto _TL65;_TL64: _TL65:;}goto _TL63;_TL62: _TL63:
# 325
 s=s2;goto _TL44;}_TL60: goto _LL1F;_LL1F:;}goto _LL1E;}default: _LL1E:{struct Cyc_Warn_String_Warn_Warg_struct _T71;_T71.tag=0;
# 330
_T71.f1=_tag_fat("bad stmt after xlation to C",sizeof(char),28U);_T66=_T71;}{struct Cyc_Warn_String_Warn_Warg_struct _T71=_T66;void*_T72[1];_T72[0]=& _T71;_T68=Cyc_Warn_impos2;{int(*_T73)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T68;_T67=_T73;}_T69=_tag_fat(_T72,sizeof(void*),1);_T67(_T69);}};}goto _TL44;_TL46:;}
# 340
static int Cyc_Tovc_stmt_to_fun_cmp(struct Cyc_Absyn_Vardecl*x,struct Cyc_Absyn_Vardecl*y){struct Cyc_Absyn_Vardecl*_T0;struct _tuple0*_T1;struct Cyc_Absyn_Vardecl*_T2;struct _tuple0*_T3;int _T4;_T0=x;_T1=_T0->name;_T2=y;_T3=_T2->name;_T4=
Cyc_Absyn_qvar_cmp(_T1,_T3);return _T4;}
# 344
static struct Cyc_Absyn_Exp*Cyc_Tovc_stmt_to_fun(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s,void*rettype){struct Cyc_Set_Set*(*_T0)(struct _RegionHandle*,int(*)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*));struct Cyc_Set_Set*(*_T1)(struct _RegionHandle*,int(*)(void*,void*));struct Cyc_Tovc_ToExpEnv _T2;struct _RegionHandle*_T3;struct Cyc_Tovc_BoxingEnv _T4;struct Cyc_Tovc_ToExpEnv _T5;struct Cyc_Tovc_ToExpEnv _T6;struct Cyc_Tovc_ToExpEnv _T7;struct Cyc_Absyn_Vardecl*_T8;struct Cyc_Absyn_Vardecl*_T9;struct Cyc_Absyn_Vardecl*_TA;struct Cyc_Absyn_Vardecl*_TB;struct Cyc_Absyn_Vardecl*_TC;struct Cyc_Absyn_Vardecl*_TD;struct Cyc_Absyn_Vardecl*_TE;struct Cyc_Absyn_Vardecl*_TF;struct Cyc_Absyn_Vardecl*_T10;struct Cyc_Absyn_Vardecl*_T11;struct Cyc_Absyn_Vardecl*_T12;int(*_T13)(struct Cyc_Iter_Iter,struct Cyc_Absyn_Vardecl**);int(*_T14)(struct Cyc_Iter_Iter,void*);struct Cyc_Iter_Iter _T15;struct Cyc_Absyn_Vardecl**_T16;int _T17;struct Cyc_Absyn_Vardecl*_T18;int*_T19;int _T1A;struct Cyc_Absyn_ArrayInfo _T1B;struct Cyc_List_List*_T1C;struct _tuple8*_T1D;struct Cyc_Absyn_Vardecl*_T1E;struct _tuple0*_T1F;struct _tuple0 _T20;struct Cyc_Absyn_Vardecl*_T21;void*_T22;struct Cyc_Absyn_Tqual _T23;struct Cyc_List_List*_T24;struct Cyc_Absyn_Vardecl*_T25;struct _tuple0*_T26;struct Cyc_List_List*_T27;struct _tuple8*_T28;struct Cyc_Absyn_Vardecl*_T29;struct _tuple0*_T2A;struct _tuple0 _T2B;struct Cyc_Absyn_Vardecl*_T2C;struct Cyc_Absyn_Vardecl*_T2D;void*_T2E;struct Cyc_Absyn_Tqual _T2F;struct Cyc_List_List*_T30;struct Cyc_Absyn_Vardecl*_T31;struct _tuple0*_T32;struct Cyc_Absyn_Exp*_T33;struct Cyc_Tovc_ToExpEnv _T34;struct Cyc_List_List**_T35;struct Cyc_List_List*_T36;struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_T37;struct Cyc_Absyn_Fndecl*_T38;void*_T39;struct Cyc_Tovc_ToExpEnv _T3A;struct Cyc_List_List**_T3B;struct Cyc_Absyn_Exp*_T3C;struct Cyc_List_List*_T3D;struct Cyc_Absyn_Exp*_T3E;_T1=Cyc_Set_rempty;{
# 347
struct Cyc_Set_Set*(*_T3F)(struct _RegionHandle*,int(*)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*))=(struct Cyc_Set_Set*(*)(struct _RegionHandle*,int(*)(struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Vardecl*)))_T1;_T0=_T3F;}_T2=env;_T3=_T2.rgn;{struct Cyc_Set_Set*freevars=_T0(_T3,Cyc_Tovc_stmt_to_fun_cmp);{struct Cyc_Tovc_BoxingEnv _T3F;_T5=env;
_T3F.rgn=_T5.rgn;_T6=env;_T3F.all_locals=_T6.all_locals;_T7=env;_T3F.varmap=_T7.varmap;_T3F.boundvars=0;_T3F.freevars=& freevars;_T4=_T3F;}{struct Cyc_Tovc_BoxingEnv boxenv=_T4;
Cyc_Tovc_box_free_vars_stmt(boxenv,s);{
struct Cyc_List_List*params=0;
struct Cyc_List_List*args=0;
# 353
struct Cyc_Iter_Iter iter=Cyc_Set_make_iter(Cyc_Core_heap_region,freevars);
# 355
struct Cyc_Absyn_Vardecl*vd;vd=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));_T8=vd;_T8->sc=2U;_T9=vd;_T9->name=Cyc_Absyn_exn_name;_TA=vd;_TA->varloc=0U;_TB=vd;_TB->tq=Cyc_Absyn_empty_tqual(0U);_TC=vd;_TC->type=Cyc_Absyn_void_type;_TD=vd;
_TD->initializer=0;_TE=vd;_TE->rgn=0;_TF=vd;_TF->attributes=0;_T10=vd;_T10->escapes=0;_T11=vd;_T11->is_proto=0;_T12=vd;_T12->rename=0;
# 358
_TL66: _T14=Cyc_Iter_next;{int(*_T3F)(struct Cyc_Iter_Iter,struct Cyc_Absyn_Vardecl**)=(int(*)(struct Cyc_Iter_Iter,struct Cyc_Absyn_Vardecl**))_T14;_T13=_T3F;}_T15=iter;_T16=& vd;_T17=_T13(_T15,_T16);if(_T17)goto _TL67;else{goto _TL68;}
_TL67: _T18=vd;{void*_T3F=_T18->type;void*_T40;_T19=(int*)_T3F;_T1A=*_T19;if(_T1A!=5)goto _TL69;{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T41=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T3F;_T1B=_T41->f1;_T40=_T1B.elt_type;}{void*elt_type=_T40;{struct Cyc_List_List*_T41=_cycalloc(sizeof(struct Cyc_List_List));{struct _tuple8*_T42=_cycalloc(sizeof(struct _tuple8));_T1E=vd;_T1F=_T1E->name;_T20=*_T1F;
# 361
_T42->f0=_T20.f1;_T21=vd;_T42->f1=_T21->tq;_T22=elt_type;_T23=
Cyc_Absyn_empty_tqual(0U);_T42->f2=Cyc_Absyn_cstar_type(_T22,_T23);_T1D=(struct _tuple8*)_T42;}
# 361
_T41->hd=_T1D;
# 363
_T41->tl=params;_T1C=(struct Cyc_List_List*)_T41;}
# 361
params=_T1C;{struct Cyc_List_List*_T41=_cycalloc(sizeof(struct Cyc_List_List));_T25=vd;_T26=_T25->name;
# 364
_T41->hd=Cyc_Absyn_var_exp(_T26,0U);_T41->tl=args;_T24=(struct Cyc_List_List*)_T41;}args=_T24;goto _LL0;}_TL69:{struct Cyc_List_List*_T41=_cycalloc(sizeof(struct Cyc_List_List));{struct _tuple8*_T42=_cycalloc(sizeof(struct _tuple8));_T29=vd;_T2A=_T29->name;_T2B=*_T2A;
# 367
_T42->f0=_T2B.f1;_T2C=vd;_T42->f1=_T2C->tq;_T2D=vd;_T2E=_T2D->type;_T2F=
Cyc_Absyn_empty_tqual(0U);_T42->f2=Cyc_Absyn_cstar_type(_T2E,_T2F);_T28=(struct _tuple8*)_T42;}
# 367
_T41->hd=_T28;
# 369
_T41->tl=params;_T27=(struct Cyc_List_List*)_T41;}
# 367
params=_T27;{struct Cyc_List_List*_T41=_cycalloc(sizeof(struct Cyc_List_List));_T31=vd;_T32=_T31->name;_T33=
# 370
Cyc_Absyn_var_exp(_T32,0U);_T41->hd=Cyc_Absyn_address_exp(_T33,0U);_T41->tl=args;_T30=(struct Cyc_List_List*)_T41;}args=_T30;goto _LL0;_LL0:;}goto _TL66;_TL68: {
# 373
struct _tuple0*funname=Cyc_Toc_temp_var();_T34=env;_T35=_T34.gen_ds;{struct Cyc_List_List*_T3F=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_T40=_cycalloc(sizeof(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct));_T40->tag=1;{struct Cyc_Absyn_Fndecl*_T41=_cycalloc(sizeof(struct Cyc_Absyn_Fndecl));
# 375
_T41->sc=0U;_T41->is_inline=0;_T41->name=funname;_T41->body=s;
_T41->i.tvars=0;_T41->i.effect=0;_T41->i.ret_tqual=Cyc_Absyn_empty_tqual(0U);
_T41->i.ret_type=rettype;_T41->i.args=params;_T41->i.c_varargs=0;
_T41->i.cyc_varargs=0;_T41->i.qual_bnd=0;
_T41->i.attributes=0;_T41->i.checks_clause=0;_T41->i.checks_assn=0;_T41->i.requires_clause=0;_T41->i.requires_assn=0;
_T41->i.ensures_clause=0;_T41->i.ensures_assn=0;_T41->i.throws_clause=0;_T41->i.throws_assn=0;_T41->i.return_value=0;_T41->i.arg_vardecls=0;_T41->i.effconstr=0;
_T41->cached_type=0;_T41->param_vardecls=0;_T41->fn_vardecl=0;_T41->orig_scope=0U;_T41->escapes=0;_T38=(struct Cyc_Absyn_Fndecl*)_T41;}
# 375
_T40->f1=_T38;_T37=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_T40;}_T39=(void*)_T37;_T3F->hd=Cyc_Absyn_new_decl(_T39,0U);_T3A=env;_T3B=_T3A.gen_ds;
# 382
_T3F->tl=*_T3B;_T36=(struct Cyc_List_List*)_T3F;}
# 374
*_T35=_T36;_T3C=
# 383
Cyc_Absyn_var_exp(funname,0U);_T3D=args;_T3E=Cyc_Absyn_fncall_exp(_T3C,_T3D,0U);return _T3E;}}}}}
# 386
static void Cyc_Tovc_apply_varmap(struct Cyc_Dict_Dict varmap,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;void*_T3;struct _tuple0*_T4;union Cyc_Absyn_Nmspace _T5;struct _union_Nmspace_Loc_n _T6;unsigned _T7;int(*_T8)(struct Cyc_Dict_Dict,struct _fat_ptr*);int(*_T9)(struct Cyc_Dict_Dict,void*);int _TA;struct Cyc_Absyn_Exp*_TB;struct Cyc_Absyn_Exp*(*_TC)(struct Cyc_Dict_Dict,struct _fat_ptr*);void*(*_TD)(struct Cyc_Dict_Dict,void*);struct Cyc_Absyn_Exp*_TE;struct Cyc_Dict_Dict _TF;struct Cyc_List_List*_T10;void*_T11;struct Cyc_Absyn_Exp*_T12;struct Cyc_List_List*_T13;struct Cyc_Dict_Dict _T14;struct Cyc_List_List*_T15;void*_T16;struct _tuple11*_T17;struct _tuple11 _T18;struct Cyc_Absyn_Exp*_T19;struct Cyc_List_List*_T1A;struct Cyc_Warn_String_Warn_Warg_struct _T1B;int(*_T1C)(struct _fat_ptr);void*(*_T1D)(struct _fat_ptr);struct _fat_ptr _T1E;_T0=e;{
void*_T1F=_T0->r;struct Cyc_Absyn_Exp*_T20;struct Cyc_Absyn_Exp*_T21;struct Cyc_List_List*_T22;struct Cyc_Absyn_Exp*_T23;void*_T24;_T1=(int*)_T1F;_T2=*_T1;switch(_T2){case 1:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T1F;_T3=_T25->f1;_T24=(void*)_T3;}{void*b=_T24;
# 389
struct _tuple0*_T25=Cyc_Absyn_binding2qvar(b);struct _fat_ptr*_T26;_T4=(struct _tuple0*)_T25;_T5=_T4->f0;_T6=_T5.Loc_n;_T7=_T6.tag;if(_T7!=4)goto _TL6C;{struct _tuple0 _T27=*_T25;_T26=_T27.f1;}{struct _fat_ptr*v=_T26;_T9=Cyc_Dict_member;{
# 391
int(*_T27)(struct Cyc_Dict_Dict,struct _fat_ptr*)=(int(*)(struct Cyc_Dict_Dict,struct _fat_ptr*))_T9;_T8=_T27;}_TA=_T8(varmap,v);if(!_TA)goto _TL6E;_TB=e;_TD=Cyc_Dict_lookup;{
struct Cyc_Absyn_Exp*(*_T27)(struct Cyc_Dict_Dict,struct _fat_ptr*)=(struct Cyc_Absyn_Exp*(*)(struct Cyc_Dict_Dict,struct _fat_ptr*))_TD;_TC=_T27;}_TE=_TC(varmap,v);_TB->r=_TE->r;goto _TL6F;_TL6E: _TL6F:
 return;}_TL6C:
 return;;}case 0:
# 398
 return;case 10:{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;_T22=_T25->f2;}{struct Cyc_Absyn_Exp*e1=_T23;struct Cyc_List_List*es=_T22;
# 400
Cyc_Tovc_apply_varmap(varmap,e1);_T22=es;goto _LL8;}case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_T1F;_T22=_T25->f2;}_LL8: {struct Cyc_List_List*es=_T22;
# 402
_TL73: if(es!=0)goto _TL71;else{goto _TL72;}
_TL71: _TF=varmap;_T10=es;_T11=_T10->hd;_T12=(struct Cyc_Absyn_Exp*)_T11;Cyc_Tovc_apply_varmap(_TF,_T12);_T13=es;
# 402
es=_T13->tl;goto _TL73;_TL72:
# 404
 return;}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;_T21=_T25->f2;_T20=_T25->f3;}{struct Cyc_Absyn_Exp*e1=_T23;struct Cyc_Absyn_Exp*e2=_T21;struct Cyc_Absyn_Exp*e3=_T20;
# 407
Cyc_Tovc_apply_varmap(varmap,e3);_T23=e1;_T21=e2;goto _LLC;}case 7:{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;_T21=_T25->f2;}_LLC: {struct Cyc_Absyn_Exp*e1=_T23;struct Cyc_Absyn_Exp*e2=_T21;_T23=e1;_T21=e2;goto _LLE;}case 8:{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;_T21=_T25->f2;}_LLE: {struct Cyc_Absyn_Exp*e1=_T23;struct Cyc_Absyn_Exp*e2=_T21;_T23=e1;_T21=e2;goto _LL10;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;_T21=_T25->f2;}_LL10: {struct Cyc_Absyn_Exp*e1=_T23;struct Cyc_Absyn_Exp*e2=_T21;_T23=e1;_T21=e2;goto _LL12;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;_T21=_T25->f2;}_LL12: {struct Cyc_Absyn_Exp*e1=_T23;struct Cyc_Absyn_Exp*e2=_T21;_T23=e1;_T21=e2;goto _LL14;}case 4:{struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;_T21=_T25->f3;}_LL14: {struct Cyc_Absyn_Exp*e1=_T23;struct Cyc_Absyn_Exp*e2=_T21;
# 413
Cyc_Tovc_apply_varmap(varmap,e2);_T23=e1;goto _LL16;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL16: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL18;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL18: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL1A;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f2;}_LL1A: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL1C;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL1C: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL1E;}case 18:{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL1E: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL20;}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL20: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL22;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL22: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL24;}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL24: {struct Cyc_Absyn_Exp*e1=_T23;_T23=e1;goto _LL26;}case 5:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T1F;_T23=_T25->f1;}_LL26: {struct Cyc_Absyn_Exp*e1=_T23;
# 424
Cyc_Tovc_apply_varmap(varmap,e1);
return;}case 17: goto _LL2A;case 21: _LL2A: goto _LL2C;case 33: _LL2C:
# 429
 return;case 37:{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_T25=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_T1F;_T22=_T25->f2;}{struct Cyc_List_List*dles=_T22;
# 432
_TL77: if(dles!=0)goto _TL75;else{goto _TL76;}
_TL75: _T14=varmap;_T15=dles;_T16=_T15->hd;_T17=(struct _tuple11*)_T16;_T18=*_T17;_T19=_T18.f1;Cyc_Tovc_apply_varmap(_T14,_T19);_T1A=dles;
# 432
dles=_T1A->tl;goto _TL77;_TL76:
# 434
 return;}default:{struct Cyc_Warn_String_Warn_Warg_struct _T25;_T25.tag=0;
# 436
_T25.f1=_tag_fat("bad exp form in Tocv::apply_varmap",sizeof(char),35U);_T1B=_T25;}{struct Cyc_Warn_String_Warn_Warg_struct _T25=_T1B;void*_T26[1];_T26[0]=& _T25;_T1D=Cyc_Warn_impos2;{int(*_T27)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T1D;_T1C=_T27;}_T1E=_tag_fat(_T26,sizeof(void*),1);_T1C(_T1E);}};}}
# 440
static struct Cyc_Absyn_Exp*Cyc_Tovc_stmt_to_exp(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s){struct Cyc_Absyn_Stmt*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Exp*_T3;struct Cyc_Tovc_ToExpEnv _T4;struct Cyc_Dict_Dict _T5;struct Cyc_Absyn_Exp*_T6;struct Cyc_Absyn_Exp*_T7;struct Cyc_Absyn_Exp*_T8;struct Cyc_Tovc_ToExpEnv _T9;struct Cyc_Dict_Dict _TA;struct Cyc_Absyn_Exp*_TB;struct Cyc_Absyn_Exp*_TC;struct Cyc_Absyn_Decl*_TD;int*_TE;int _TF;struct Cyc_Absyn_Vardecl*_T10;struct _tuple0*_T11;struct _tuple0 _T12;struct Cyc_Absyn_Vardecl*_T13;struct Cyc_Dict_Dict(*_T14)(struct Cyc_Dict_Dict,struct _fat_ptr*,struct Cyc_Absyn_Exp*);struct Cyc_Dict_Dict(*_T15)(struct Cyc_Dict_Dict,void*,void*);struct Cyc_Tovc_ToExpEnv _T16;struct Cyc_Dict_Dict _T17;struct _fat_ptr*_T18;struct Cyc_Absyn_Exp*_T19;struct Cyc_Absyn_Vardecl*_T1A;struct Cyc_Absyn_Exp*_T1B;struct Cyc_Tovc_ToExpEnv _T1C;struct Cyc_Dict_Dict _T1D;struct Cyc_Absyn_Vardecl*_T1E;struct Cyc_Absyn_Exp*_T1F;struct Cyc_Absyn_Vardecl*_T20;struct Cyc_Absyn_Exp*_T21;struct Cyc_Absyn_Exp*_T22;int*_T23;int _T24;struct Cyc_Absyn_Vardecl*_T25;struct _tuple0*_T26;struct Cyc_List_List*_T27;struct Cyc_Absyn_Exp*_T28;struct Cyc_Absyn_Exp*_T29;struct Cyc_Absyn_Exp*_T2A;struct Cyc_List_List*_T2B;void*_T2C;struct _tuple11*_T2D;struct _tuple11 _T2E;struct Cyc_Absyn_Exp*_T2F;struct Cyc_List_List*_T30;struct Cyc_Absyn_Vardecl*_T31;struct Cyc_List_List*_T32;struct Cyc_Tovc_ToExpEnv _T33;struct _RegionHandle*_T34;struct Cyc_Tovc_ToExpEnv _T35;struct Cyc_Tovc_ToExpEnv _T36;struct Cyc_Absyn_Stmt*_T37;struct Cyc_Absyn_Decl*_T38;struct Cyc_Tovc_ToExpEnv _T39;struct Cyc_Absyn_Stmt*_T3A;void*_T3B;struct Cyc_Absyn_Stmt*_T3C;struct Cyc_Absyn_Stmt*_T3D;struct Cyc_Absyn_Vardecl*_T3E;struct Cyc_Absyn_Exp*_T3F;struct Cyc_Absyn_Vardecl*_T40;struct _tuple0*_T41;struct Cyc_Absyn_Exp*_T42;struct Cyc_Absyn_Vardecl*_T43;struct Cyc_Absyn_Exp*_T44;struct Cyc_Absyn_Exp*_T45;struct Cyc_Absyn_Exp*_T46;struct Cyc_Absyn_Exp*_T47;struct Cyc_Absyn_Vardecl*_T48;struct Cyc_List_List*_T49;void*_T4A;struct Cyc_Absyn_Exp*_T4B;struct Cyc_Absyn_Exp*_T4C;struct Cyc_List_List*_T4D;struct Cyc_Absyn_Exp*_T4E;struct Cyc_Warn_String_Warn_Warg_struct _T4F;int(*_T50)(struct _fat_ptr);void*(*_T51)(struct _fat_ptr);struct _fat_ptr _T52;struct Cyc_Absyn_Exp*_T53;_T0=s;{
void*_T54=_T0->r;struct Cyc_Absyn_Decl*_T55;struct Cyc_Absyn_Stmt*_T56;struct Cyc_Absyn_Stmt*_T57;struct Cyc_Absyn_Exp*_T58;_T1=(int*)_T54;_T2=*_T1;switch(_T2){case 0: _T3=
Cyc_Absyn_true_exp(0U);return _T3;case 1:{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T54;_T58=_T59->f1;}{struct Cyc_Absyn_Exp*e=_T58;_T4=env;_T5=_T4.varmap;_T6=e;
# 444
Cyc_Tovc_apply_varmap(_T5,_T6);_T7=e;
return _T7;}case 2:{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T54;_T57=_T59->f1;_T56=_T59->f2;}{struct Cyc_Absyn_Stmt*s1=_T57;struct Cyc_Absyn_Stmt*s2=_T56;
# 448
struct Cyc_Absyn_Exp*s1_new=Cyc_Tovc_stmt_to_exp(env,s1);
struct Cyc_Absyn_Exp*s2_new=Cyc_Tovc_stmt_to_exp(env,s2);_T8=
Cyc_Absyn_seq_exp(s1_new,s2_new,0U);return _T8;}case 4:{struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_T54;_T58=_T59->f1;_T57=_T59->f2;_T56=_T59->f3;}{struct Cyc_Absyn_Exp*e=_T58;struct Cyc_Absyn_Stmt*s1=_T57;struct Cyc_Absyn_Stmt*s2=_T56;_T9=env;_TA=_T9.varmap;_TB=e;
# 452
Cyc_Tovc_apply_varmap(_TA,_TB);{
# 454
struct Cyc_Absyn_Exp*s1_new=Cyc_Tovc_stmt_to_exp(env,s1);
struct Cyc_Absyn_Exp*s2_new=Cyc_Tovc_stmt_to_exp(env,s2);_TC=
Cyc_Absyn_conditional_exp(e,s1_new,s2_new,0U);return _TC;}}case 12:{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T54;_T55=_T59->f1;_T57=_T59->f2;}{struct Cyc_Absyn_Decl*d=_T55;struct Cyc_Absyn_Stmt*s2=_T57;_TD=d;{
# 458
void*_T59=_TD->r;struct Cyc_Absyn_Vardecl*_T5A;_TE=(int*)_T59;_TF=*_TE;if(_TF!=0)goto _TL79;{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T5B=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T59;_T5A=_T5B->f1;}{struct Cyc_Absyn_Vardecl*vd=_T5A;_T10=vd;_T11=_T10->name;_T12=*_T11;{
# 461
struct _fat_ptr*oldname=_T12.f1;
struct _tuple0*newqvar=Cyc_Toc_temp_var();_T13=vd;
_T13->name=newqvar;_T15=Cyc_Dict_insert;{
struct Cyc_Dict_Dict(*_T5B)(struct Cyc_Dict_Dict,struct _fat_ptr*,struct Cyc_Absyn_Exp*)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict,struct _fat_ptr*,struct Cyc_Absyn_Exp*))_T15;_T14=_T5B;}_T16=env;_T17=_T16.varmap;_T18=oldname;_T19=Cyc_Absyn_var_exp(newqvar,0U);env.varmap=_T14(_T17,_T18,_T19);{
struct Cyc_List_List*unresolvedmem_inits=0;_T1A=vd;_T1B=_T1A->initializer;
if(_T1B==0)goto _TL7B;_T1C=env;_T1D=_T1C.varmap;_T1E=vd;_T1F=_T1E->initializer;
Cyc_Tovc_apply_varmap(_T1D,_T1F);_T20=vd;_T21=_T20->initializer;_T22=
_check_null(_T21);{void*_T5B=_T22->r;struct Cyc_List_List*_T5C;_T23=(int*)_T5B;_T24=*_T23;if(_T24!=37)goto _TL7D;{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_T5D=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_T5B;_T5C=_T5D->f2;}{struct Cyc_List_List*dles=_T5C;_T25=vd;_T26=_T25->name;{
# 480 "tovc.cyc"
struct Cyc_Absyn_Exp*var=Cyc_Absyn_var_exp(_T26,0U);{
unsigned i=0U;_TL82: if(dles!=0)goto _TL80;else{goto _TL81;}
_TL80:{struct Cyc_List_List*_T5D=_cycalloc(sizeof(struct Cyc_List_List));_T28=var;_T29=Cyc_Absyn_uint_exp(i,0U);_T2A=Cyc_Absyn_subscript_exp(_T28,_T29,0U);_T2B=dles;_T2C=_T2B->hd;_T2D=(struct _tuple11*)_T2C;_T2E=*_T2D;_T2F=_T2E.f1;_T5D->hd=Cyc_Absyn_assign_exp(_T2A,_T2F,0U);
# 485
_T5D->tl=unresolvedmem_inits;_T27=(struct Cyc_List_List*)_T5D;}
# 482
unresolvedmem_inits=_T27;_T30=dles;
# 481
dles=_T30->tl;i=i + 1;goto _TL82;_TL81:;}_T31=vd;
# 486
_T31->initializer=0;goto _LL12;}}_TL7D: goto _LL12;_LL12:;}goto _TL7C;_TL7B: _TL7C: _T33=env;_T34=_T33.rgn;{struct Cyc_List_List*_T5B=_region_malloc(_T34,0U,sizeof(struct Cyc_List_List));
# 491
_T5B->hd=vd;_T35=env;_T5B->tl=_T35.all_locals;_T32=(struct Cyc_List_List*)_T5B;}env.all_locals=_T32;_T36=env;_T37=_T36.encloser;_T38=d;_T39=env;_T3A=_T39.encloser;_T3B=_T3A->r;_T3C=
# 493
Cyc_Absyn_new_stmt(_T3B,0U);_T3D=Cyc_Absyn_decl_stmt(_T38,_T3C,0U);_T37->r=_T3D->r;{
struct Cyc_Absyn_Exp*e=Cyc_Tovc_stmt_to_exp(env,s2);_T3E=vd;_T3F=_T3E->initializer;
if(_T3F==0)goto _TL83;_T40=vd;_T41=_T40->name;_T42=
Cyc_Absyn_var_exp(_T41,0U);_T43=vd;_T44=_T43->initializer;_T45=_check_null(_T44);_T46=Cyc_Absyn_assign_exp(_T42,_T45,0U);_T47=e;e=Cyc_Absyn_seq_exp(_T46,_T47,0U);_T48=vd;
# 498
_T48->initializer=0;goto _TL84;_TL83: _TL84:
# 500
 _TL88: if(unresolvedmem_inits!=0)goto _TL86;else{goto _TL87;}
_TL86: _T49=unresolvedmem_inits;_T4A=_T49->hd;_T4B=(struct Cyc_Absyn_Exp*)_T4A;_T4C=e;e=Cyc_Absyn_seq_exp(_T4B,_T4C,0U);_T4D=unresolvedmem_inits;
# 500
unresolvedmem_inits=_T4D->tl;goto _TL88;_TL87: _T4E=e;
# 502
return _T4E;}}}}_TL79:{struct Cyc_Warn_String_Warn_Warg_struct _T5B;_T5B.tag=0;
_T5B.f1=_tag_fat("bad local decl in Tovc::stmt_to_exp",sizeof(char),36U);_T4F=_T5B;}{struct Cyc_Warn_String_Warn_Warg_struct _T5B=_T4F;void*_T5C[1];_T5C[0]=& _T5B;_T51=Cyc_Warn_impos2;{int(*_T5D)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T51;_T50=_T5D;}_T52=_tag_fat(_T5C,sizeof(void*),1);_T50(_T52);};}}default: _T53=
# 506
Cyc_Tovc_stmt_to_fun(env,s,Cyc_Absyn_void_type);return _T53;};}}
# 510
static void Cyc_Tovc_stmt_to_vc(struct Cyc_Tovc_ToExpEnv,struct Cyc_Absyn_Stmt*);
# 512
static void Cyc_Tovc_exp_to_vc(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;struct Cyc_Tovc_ToExpEnv _T3;struct Cyc_List_List*_T4;void*_T5;struct Cyc_Absyn_Exp*_T6;struct Cyc_List_List*_T7;struct Cyc_Tovc_ToExpEnv _T8;struct Cyc_List_List*_T9;void*_TA;struct _tuple11*_TB;struct _tuple11 _TC;struct Cyc_Absyn_Exp*_TD;struct Cyc_List_List*_TE;struct Cyc_Absyn_Exp*_TF;struct Cyc_Absyn_Exp*_T10;struct Cyc_Warn_String_Warn_Warg_struct _T11;struct Cyc_Warn_Exp_Warn_Warg_struct _T12;int(*_T13)(struct _fat_ptr);void*(*_T14)(struct _fat_ptr);struct _fat_ptr _T15;
struct _fat_ptr bad_form=_tag_fat("",sizeof(char),1U);_T0=e;{
void*_T16=_T0->r;struct Cyc_Absyn_Stmt*_T17;struct Cyc_Absyn_Exp*_T18;struct Cyc_Absyn_Exp*_T19;struct Cyc_List_List*_T1A;struct Cyc_Absyn_Exp*_T1B;_T1=(int*)_T16;_T2=*_T1;switch(_T2){case 0:
 return;case 1:
 return;case 10:{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;_T1A=_T1C->f2;}{struct Cyc_Absyn_Exp*e1=_T1B;struct Cyc_List_List*es=_T1A;
# 519
Cyc_Tovc_exp_to_vc(env,e1);_T1A=es;goto _LL8;}case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_T16;_T1A=_T1C->f2;}_LL8: {struct Cyc_List_List*es=_T1A;
# 521
_TL8D: if(es!=0)goto _TL8B;else{goto _TL8C;}
_TL8B: _T3=env;_T4=es;_T5=_T4->hd;_T6=(struct Cyc_Absyn_Exp*)_T5;Cyc_Tovc_exp_to_vc(_T3,_T6);_T7=es;
# 521
es=_T7->tl;goto _TL8D;_TL8C:
# 523
 return;}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;_T19=_T1C->f2;_T18=_T1C->f3;}{struct Cyc_Absyn_Exp*e1=_T1B;struct Cyc_Absyn_Exp*e2=_T19;struct Cyc_Absyn_Exp*e3=_T18;
# 526
Cyc_Tovc_exp_to_vc(env,e3);_T1B=e1;_T19=e2;goto _LLC;}case 7:{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;_T19=_T1C->f2;}_LLC: {struct Cyc_Absyn_Exp*e1=_T1B;struct Cyc_Absyn_Exp*e2=_T19;_T1B=e1;_T19=e2;goto _LLE;}case 8:{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;_T19=_T1C->f2;}_LLE: {struct Cyc_Absyn_Exp*e1=_T1B;struct Cyc_Absyn_Exp*e2=_T19;_T1B=e1;_T19=e2;goto _LL10;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;_T19=_T1C->f2;}_LL10: {struct Cyc_Absyn_Exp*e1=_T1B;struct Cyc_Absyn_Exp*e2=_T19;_T1B=e1;_T19=e2;goto _LL12;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;_T19=_T1C->f2;}_LL12: {struct Cyc_Absyn_Exp*e1=_T1B;struct Cyc_Absyn_Exp*e2=_T19;_T1B=e1;_T19=e2;goto _LL14;}case 4:{struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;_T19=_T1C->f3;}_LL14: {struct Cyc_Absyn_Exp*e1=_T1B;struct Cyc_Absyn_Exp*e2=_T19;
# 532
Cyc_Tovc_exp_to_vc(env,e2);_T1B=e1;goto _LL16;}case 42:{struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL16: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL18;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL18: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL1A;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL1A: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL1C;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f2;}_LL1C: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL1E;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL1E: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL20;}case 18:{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL20: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL22;}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL22: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL24;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL24: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL26;}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL26: {struct Cyc_Absyn_Exp*e1=_T1B;_T1B=e1;goto _LL28;}case 5:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T16;_T1B=_T1C->f1;}_LL28: {struct Cyc_Absyn_Exp*e1=_T1B;
# 544
Cyc_Tovc_exp_to_vc(env,e1);
return;}case 17: goto _LL2C;case 21: _LL2C: goto _LL2E;case 34: _LL2E: goto _LL30;case 33: _LL30:
# 550
 return;case 37:{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_T16;_T1A=_T1C->f2;}{struct Cyc_List_List*dles=_T1A;
# 556
_TL91: if(dles!=0)goto _TL8F;else{goto _TL90;}
_TL8F: _T8=env;_T9=dles;_TA=_T9->hd;_TB=(struct _tuple11*)_TA;_TC=*_TB;_TD=_TC.f1;Cyc_Tovc_exp_to_vc(_T8,_TD);_TE=dles;
# 556
dles=_TE->tl;goto _TL91;_TL90:
# 558
 return;}case 38:{struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_T1C=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_T16;_T17=_T1C->f1;}{struct Cyc_Absyn_Stmt*s=_T17;
# 564
Cyc_Tovc_stmt_to_vc(env,s);_TF=e;_T10=
# 566
Cyc_Tovc_stmt_to_exp(env,s);_TF->r=_T10->r;
return;}case 41:
# 569
 return;default:{struct Cyc_Warn_String_Warn_Warg_struct _T1C;_T1C.tag=0;
_T1C.f1=_tag_fat("bad expression after translation to C: ",sizeof(char),40U);_T11=_T1C;}{struct Cyc_Warn_String_Warn_Warg_struct _T1C=_T11;{struct Cyc_Warn_Exp_Warn_Warg_struct _T1D;_T1D.tag=4;_T1D.f1=e;_T12=_T1D;}{struct Cyc_Warn_Exp_Warn_Warg_struct _T1D=_T12;void*_T1E[2];_T1E[0]=& _T1C;_T1E[1]=& _T1D;_T14=Cyc_Warn_impos2;{int(*_T1F)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T14;_T13=_T1F;}_T15=_tag_fat(_T1E,sizeof(void*),2);_T13(_T15);}}};}}
# 574
static void Cyc_Tovc_stmt_to_vc(struct Cyc_Tovc_ToExpEnv env,struct Cyc_Absyn_Stmt*s){struct Cyc_Absyn_Stmt*_T0;int*_T1;unsigned _T2;struct _tuple9 _T3;struct _tuple9 _T4;struct _tuple9 _T5;struct _tuple9 _T6;struct Cyc_Tovc_ToExpEnv _T7;struct Cyc_List_List*_T8;void*_T9;struct Cyc_Absyn_Switch_clause*_TA;struct Cyc_Absyn_Stmt*_TB;struct Cyc_List_List*_TC;struct Cyc_Absyn_Decl*_TD;int*_TE;int _TF;int _T10;struct Cyc_Absyn_Vardecl*_T11;void*_T12;int*_T13;int _T14;struct Cyc_Absyn_Vardecl*_T15;struct Cyc_Absyn_Exp*_T16;struct Cyc_Absyn_Vardecl*_T17;struct Cyc_Absyn_Exp*_T18;int*_T19;int _T1A;struct Cyc_Absyn_Vardecl*_T1B;struct Cyc_Absyn_Vardecl*_T1C;struct _tuple0*_T1D;struct Cyc_Absyn_Stmt*_T1E;void*_T1F;struct Cyc_Absyn_Exp*_T20;struct Cyc_Absyn_Exp*_T21;struct Cyc_Absyn_Exp*_T22;struct Cyc_List_List*_T23;void*_T24;struct _tuple11*_T25;struct _tuple11 _T26;struct Cyc_Absyn_Exp*_T27;struct Cyc_Absyn_Exp*_T28;struct Cyc_Absyn_Stmt*_T29;struct Cyc_Absyn_Stmt*_T2A;struct Cyc_List_List*_T2B;struct Cyc_Absyn_Stmt*_T2C;struct Cyc_Absyn_Stmt*_T2D;struct Cyc_Warn_String_Warn_Warg_struct _T2E;struct Cyc_Warn_Stmt_Warn_Warg_struct _T2F;struct Cyc_Warn_String_Warn_Warg_struct _T30;int(*_T31)(struct _fat_ptr);void*(*_T32)(struct _fat_ptr);struct _fat_ptr _T33;struct Cyc_Absyn_Vardecl*_T34;struct Cyc_Absyn_Exp*_T35;struct Cyc_Tovc_ToExpEnv _T36;struct Cyc_Absyn_Vardecl*_T37;struct Cyc_Absyn_Exp*_T38;struct Cyc_List_List*_T39;struct Cyc_Tovc_ToExpEnv _T3A;struct _RegionHandle*_T3B;struct Cyc_Tovc_ToExpEnv _T3C;struct Cyc_Warn_String_Warn_Warg_struct _T3D;int(*_T3E)(struct _fat_ptr);void*(*_T3F)(struct _fat_ptr);struct _fat_ptr _T40;
# 576
_TL92: if(1)goto _TL93;else{goto _TL94;}
_TL93: env.encloser=s;_T0=s;{
void*_T41=_T0->r;struct Cyc_Absyn_Decl*_T42;struct Cyc_List_List*_T43;struct Cyc_Absyn_Exp*_T44;struct Cyc_Absyn_Exp*_T45;struct Cyc_Absyn_Stmt*_T46;struct Cyc_Absyn_Exp*_T47;struct Cyc_Absyn_Stmt*_T48;_T1=(int*)_T41;_T2=*_T1;switch(_T2){case 0: goto _LL4;case 6: _LL4: goto _LL6;case 7: _LL6: goto _LL8;case 8: _LL8:
# 582
 return;case 13:{struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_T41;_T48=_T49->f2;}{struct Cyc_Absyn_Stmt*s2=_T48;
# 584
s=s2;goto _TL92;}case 3:{struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_T41;_T47=_T49->f1;}{struct Cyc_Absyn_Exp*eopt=_T47;
# 587
if(eopt!=0)goto _TL96;
return;_TL96: _T47=eopt;goto _LLE;}case 1:{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T41;_T47=_T49->f1;}_LLE: {struct Cyc_Absyn_Exp*e=_T47;
# 591
Cyc_Tovc_exp_to_vc(env,e);
return;}case 2:{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T41;_T48=_T49->f1;_T46=_T49->f2;}{struct Cyc_Absyn_Stmt*s1=_T48;struct Cyc_Absyn_Stmt*s2=_T46;
# 594
Cyc_Tovc_stmt_to_vc(env,s1);
s=s2;goto _TL92;}case 4:{struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_T41;_T47=_T49->f1;_T48=_T49->f2;_T46=_T49->f3;}{struct Cyc_Absyn_Exp*e=_T47;struct Cyc_Absyn_Stmt*s1=_T48;struct Cyc_Absyn_Stmt*s2=_T46;
# 598
Cyc_Tovc_exp_to_vc(env,e);
Cyc_Tovc_stmt_to_vc(env,s1);
s=s2;goto _TL92;}case 5:{struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_T41;_T3=_T49->f1;_T47=_T3.f0;_T48=_T49->f2;}{struct Cyc_Absyn_Exp*e=_T47;struct Cyc_Absyn_Stmt*s2=_T48;
# 603
Cyc_Tovc_exp_to_vc(env,e);
s=s2;goto _TL92;}case 9:{struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T41;_T47=_T49->f1;_T4=_T49->f2;_T45=_T4.f0;_T5=_T49->f3;_T44=_T5.f0;_T48=_T49->f4;}{struct Cyc_Absyn_Exp*e1=_T47;struct Cyc_Absyn_Exp*e2=_T45;struct Cyc_Absyn_Exp*e3=_T44;struct Cyc_Absyn_Stmt*s2=_T48;
# 607
Cyc_Tovc_exp_to_vc(env,e1);
Cyc_Tovc_exp_to_vc(env,e2);
Cyc_Tovc_exp_to_vc(env,e3);
s=s2;goto _TL92;}case 14:{struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_T41;_T48=_T49->f1;_T6=_T49->f2;_T47=_T6.f0;}{struct Cyc_Absyn_Stmt*s2=_T48;struct Cyc_Absyn_Exp*e=_T47;
# 613
Cyc_Tovc_exp_to_vc(env,e);
s=s2;goto _TL92;}case 10:{struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_T41;_T47=_T49->f1;_T43=_T49->f2;}{struct Cyc_Absyn_Exp*e=_T47;struct Cyc_List_List*scs=_T43;
# 619
Cyc_Tovc_exp_to_vc(env,e);
_TL9B: if(scs!=0)goto _TL99;else{goto _TL9A;}
_TL99: _T7=env;_T8=scs;_T9=_T8->hd;_TA=(struct Cyc_Absyn_Switch_clause*)_T9;_TB=_TA->body;Cyc_Tovc_stmt_to_vc(_T7,_TB);_TC=scs;
# 620
scs=_TC->tl;goto _TL9B;_TL9A:
# 622
 return;}case 12:{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T49=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T41;_T42=_T49->f1;_T48=_T49->f2;}{struct Cyc_Absyn_Decl*d=_T42;struct Cyc_Absyn_Stmt*s2=_T48;_TD=d;{
# 624
void*_T49=_TD->r;struct Cyc_Absyn_Vardecl*_T4A;_TE=(int*)_T49;_TF=*_TE;if(_TF!=0)goto _TL9C;{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T4B=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T49;_T4A=_T4B->f1;}{struct Cyc_Absyn_Vardecl*vd=_T4A;_T10=Cyc_Tovc_elim_array_initializers;
# 626
if(!_T10)goto _TL9E;_T11=vd;_T12=_T11->type;{
# 631
void*_T4B=Cyc_Absyn_compress(_T12);_T13=(int*)_T4B;_T14=*_T13;if(_T14!=5)goto _TLA0;_T15=vd;_T16=_T15->initializer;
# 633
if(_T16==0)goto _TLA2;_T17=vd;_T18=_T17->initializer;{
void*_T4C=_T18->r;struct Cyc_List_List*_T4D;_T19=(int*)_T4C;_T1A=*_T19;if(_T1A!=37)goto _TLA4;{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_T4E=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_T4C;_T4D=_T4E->f2;}{struct Cyc_List_List*dles=_T4D;_T1B=vd;
# 636
_T1B->initializer=0;_T1C=vd;_T1D=_T1C->name;{
# 638
struct Cyc_Absyn_Exp*arr=Cyc_Absyn_var_exp(_T1D,0U);_T1E=s2;_T1F=_T1E->r;{
struct Cyc_Absyn_Stmt*s3=Cyc_Absyn_new_stmt(_T1F,0U);{
unsigned i=0U;_TLA9: if(dles!=0)goto _TLA7;else{goto _TLA8;}
_TLA7: _T20=arr;_T21=
Cyc_Absyn_uint_exp(i,0U);_T22=
# 641
Cyc_Absyn_subscript_exp(_T20,_T21,0U);_T23=dles;_T24=_T23->hd;_T25=(struct _tuple11*)_T24;_T26=*_T25;_T27=_T26.f1;_T28=Cyc_Absyn_assign_exp(_T22,_T27,0U);_T29=Cyc_Absyn_exp_stmt(_T28,0U);_T2A=s3;s3=Cyc_Absyn_seq_stmt(_T29,_T2A,0U);_T2B=dles;
# 640
dles=_T2B->tl;i=i + 1;goto _TLA9;_TLA8:;}_T2C=s2;_T2D=s3;
# 646
_T2C->r=_T2D->r;goto _LL29;}}}_TLA4:{struct Cyc_Warn_String_Warn_Warg_struct _T4E;_T4E.tag=0;
# 648
_T4E.f1=_tag_fat("|",sizeof(char),2U);_T2E=_T4E;}{struct Cyc_Warn_String_Warn_Warg_struct _T4E=_T2E;{struct Cyc_Warn_Stmt_Warn_Warg_struct _T4F;_T4F.tag=5;_T4F.f1=s;_T2F=_T4F;}{struct Cyc_Warn_Stmt_Warn_Warg_struct _T4F=_T2F;{struct Cyc_Warn_String_Warn_Warg_struct _T50;_T50.tag=0;_T50.f1=_tag_fat("|: array type bad initializer",sizeof(char),30U);_T30=_T50;}{struct Cyc_Warn_String_Warn_Warg_struct _T50=_T30;void*_T51[3];_T51[0]=& _T4E;_T51[1]=& _T4F;_T51[2]=& _T50;_T32=Cyc_Warn_impos2;{int(*_T52)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T32;_T31=_T52;}_T33=_tag_fat(_T51,sizeof(void*),3);_T31(_T33);}}}_LL29:;}goto _TLA3;_TLA2: _TLA3: goto _LL24;_TLA0: goto _LL24;_LL24:;}goto _TL9F;_TL9E: _TL9F: _T34=vd;_T35=_T34->initializer;
# 654
if(_T35==0)goto _TLAA;_T36=env;_T37=vd;_T38=_T37->initializer;
Cyc_Tovc_exp_to_vc(_T36,_T38);goto _TLAB;_TLAA: _TLAB: _T3A=env;_T3B=_T3A.rgn;{struct Cyc_List_List*_T4B=_region_malloc(_T3B,0U,sizeof(struct Cyc_List_List));
_T4B->hd=vd;_T3C=env;_T4B->tl=_T3C.all_locals;_T39=(struct Cyc_List_List*)_T4B;}env.all_locals=_T39;
s=s2;goto _TL92;}_TL9C: goto _LL1F;_LL1F:;}goto _LL1E;}default: _LL1E:{struct Cyc_Warn_String_Warn_Warg_struct _T49;_T49.tag=0;
# 662
_T49.f1=_tag_fat("bad stmt after xlation to C",sizeof(char),28U);_T3D=_T49;}{struct Cyc_Warn_String_Warn_Warg_struct _T49=_T3D;void*_T4A[1];_T4A[0]=& _T49;_T3F=Cyc_Warn_impos2;{int(*_T4B)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T3F;_T3E=_T4B;}_T40=_tag_fat(_T4A,sizeof(void*),1);_T3E(_T40);}};}goto _TL92;_TL94:;}
# 667
struct Cyc_List_List*Cyc_Tovc_tovc(struct Cyc_List_List*old_ds){struct Cyc_List_List*_T0;void*_T1;struct Cyc_Absyn_Decl*_T2;int*_T3;unsigned _T4;struct Cyc_Absyn_Fndecl*_T5;struct Cyc_Absyn_Fndecl*_T6;struct Cyc_Core_Opt*_T7;struct _RegionHandle*_T8;struct Cyc_List_List**_T9;struct Cyc_Tovc_ToExpEnv _TA;struct _RegionHandle*_TB;struct Cyc_Absyn_Fndecl*_TC;struct Cyc_Core_Opt*_TD;void*_TE;struct Cyc_List_List*_TF;struct Cyc_Dict_Dict(*_T10)(struct _RegionHandle*,int(*)(struct _fat_ptr*,struct _fat_ptr*));struct Cyc_Dict_Dict(*_T11)(struct _RegionHandle*,int(*)(void*,void*));struct _RegionHandle*_T12;int(*_T13)(struct _fat_ptr*,struct _fat_ptr*);struct Cyc_Absyn_Fndecl*_T14;struct Cyc_Absyn_Fndecl*_T15;struct Cyc_Absyn_Stmt*_T16;struct Cyc_List_List**_T17;struct Cyc_List_List*_T18;struct Cyc_Absyn_Vardecl*_T19;struct Cyc_Absyn_Fndecl*_T1A;struct Cyc_Absyn_Vardecl*_T1B;struct Cyc_Absyn_Fndecl*_T1C;struct Cyc_Absyn_Vardecl*_T1D;struct Cyc_Absyn_Vardecl*_T1E;struct Cyc_Absyn_Vardecl*_T1F;struct Cyc_Absyn_Vardecl*_T20;struct Cyc_Absyn_Vardecl*_T21;struct Cyc_Absyn_Vardecl*_T22;struct Cyc_Absyn_Fndecl*_T23;struct Cyc_Absyn_FnInfo _T24;struct Cyc_Absyn_Vardecl*_T25;struct Cyc_Absyn_Vardecl*_T26;struct Cyc_Absyn_Fndecl*_T27;struct Cyc_Absyn_Stmt*_T28;struct Cyc_Absyn_Vardecl*_T29;struct Cyc_List_List*_T2A;struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T2B;void*_T2C;struct Cyc_List_List**_T2D;struct Cyc_List_List*_T2E;struct Cyc_List_List*_T2F;struct Cyc_List_List*_T30;struct Cyc_Warn_String_Warn_Warg_struct _T31;int(*_T32)(struct _fat_ptr);void*(*_T33)(struct _fat_ptr);struct _fat_ptr _T34;struct Cyc_List_List*_T35;struct Cyc_List_List*_T36;
struct Cyc_List_List*new_ds=0;
_TLAF: if(old_ds!=0)goto _TLAD;else{goto _TLAE;}
_TLAD: _T0=old_ds;_T1=_T0->hd;{struct Cyc_Absyn_Decl*next_d=(struct Cyc_Absyn_Decl*)_T1;_T2=next_d;{
void*_T37=_T2->r;struct Cyc_Absyn_Fndecl*_T38;_T3=(int*)_T37;_T4=*_T3;switch(_T4){case 1:{struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_T39=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_T37;_T38=_T39->f1;}{struct Cyc_Absyn_Fndecl*fd=_T38;_T5=fd;
# 676
_T5->cached_type=0;_T6=fd;_T7=_T6->param_vardecls;
# 682
if(_T7==0)goto _TLB1;{struct _RegionHandle _T39=_new_region(0U,"rgn");struct _RegionHandle*rgn=& _T39;_push_region(rgn);{
# 684
struct Cyc_List_List**gen_ds;_T8=rgn;gen_ds=_region_malloc(_T8,0U,sizeof(struct Cyc_List_List*));_T9=gen_ds;*_T9=0;{struct Cyc_Tovc_ToExpEnv _T3A;
_T3A.rgn=rgn;_TB=rgn;_TC=fd;_TD=_TC->param_vardecls;_TE=_TD->v;_TF=(struct Cyc_List_List*)_TE;
_T3A.all_locals=Cyc_List_rcopy(_TB,_TF);_T11=Cyc_Dict_rempty;{
struct Cyc_Dict_Dict(*_T3B)(struct _RegionHandle*,int(*)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Dict_Dict(*)(struct _RegionHandle*,int(*)(struct _fat_ptr*,struct _fat_ptr*)))_T11;_T10=_T3B;}_T12=rgn;_T13=Cyc_strptrcmp;_T3A.varmap=_T10(_T12,_T13);_T14=fd;
_T3A.encloser=_T14->body;
_T3A.gen_ds=gen_ds;_TA=_T3A;}_T15=fd;_T16=_T15->body;
# 685
Cyc_Tovc_stmt_to_vc(_TA,_T16);_T17=gen_ds;_T18=*_T17;
# 691
if(_T18==0)goto _TLB3;{
struct Cyc_Absyn_Vardecl*vd;vd=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));_T19=vd;_T1A=fd;_T19->sc=_T1A->sc;_T1B=vd;_T1C=fd;_T1B->name=_T1C->name;_T1D=vd;_T1D->varloc=0U;_T1E=vd;_T1E->tq=Cyc_Absyn_const_tqual(0U);_T1F=vd;
_T1F->type=Cyc_Tcutil_fndecl2type(fd);_T20=vd;_T20->initializer=0;_T21=vd;_T21->rgn=0;_T22=vd;_T23=fd;_T24=_T23->i;
_T22->attributes=_T24.attributes;_T25=vd;_T25->escapes=0;_T26=vd;_T27=fd;_T28=_T27->body;_T26->is_proto=_T28!=0;_T29=vd;
_T29->rename=0;{struct Cyc_List_List*_T3A=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T3B=_cycalloc(sizeof(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct));_T3B->tag=0;
_T3B->f1=vd;_T2B=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T3B;}_T2C=(void*)_T2B;_T3A->hd=Cyc_Absyn_new_decl(_T2C,0U);_T3A->tl=new_ds;_T2A=(struct Cyc_List_List*)_T3A;}new_ds=_T2A;_T2D=gen_ds;_T2E=*_T2D;_T2F=new_ds;
new_ds=Cyc_List_append(_T2E,_T2F);}goto _TLB4;_TLB3: _TLB4:;}_pop_region();}goto _TLB2;_TLB1: _TLB2: goto _LL4;}case 0: _LL4: goto _LL6;case 2: _LL6: goto _LL8;case 3: _LL8: goto _LLA;case 4: _LLA: goto _LLC;case 5: _LLC: goto _LLE;case 6: _LLE: goto _LL10;case 7: _LL10: goto _LL12;case 8: _LL12:{struct Cyc_List_List*_T39=_cycalloc(sizeof(struct Cyc_List_List));
# 708
_T39->hd=next_d;_T39->tl=new_ds;_T30=(struct Cyc_List_List*)_T39;}new_ds=_T30;goto _LL0;case 9: goto _LL16;case 10: _LL16: goto _LL18;case 11: _LL18: goto _LL1A;case 12: _LL1A: goto _LL1C;case 13: _LL1C: goto _LL1E;case 14: _LL1E: goto _LL20;case 15: _LL20: goto _LL22;default: _LL22:{struct Cyc_Warn_String_Warn_Warg_struct _T39;_T39.tag=0;
# 717
_T39.f1=_tag_fat("nested translation unit after translation to C",sizeof(char),47U);_T31=_T39;}{struct Cyc_Warn_String_Warn_Warg_struct _T39=_T31;void*_T3A[1];_T3A[0]=& _T39;_T33=Cyc_Warn_impos2;{int(*_T3B)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T33;_T32=_T3B;}_T34=_tag_fat(_T3A,sizeof(void*),1);_T32(_T34);}}_LL0:;}}_T35=old_ds;
# 669
old_ds=_T35->tl;goto _TLAF;_TLAE: _T36=
# 720
Cyc_List_imp_rev(new_ds);return _T36;}

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
# 29 "assert.h"
extern void*Cyc___assert_fail(struct _fat_ptr,struct _fat_ptr,unsigned);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 76 "list.h"
extern struct Cyc_List_List*Cyc_List_map(void*(*)(void*),struct Cyc_List_List*);struct Cyc_AssnDef_ExistAssnFn;struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f0;struct _fat_ptr*f1;};
# 140 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 163
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 181 "absyn.h"
enum Cyc_Absyn_AliasHint{Cyc_Absyn_UniqueHint =0U,Cyc_Absyn_RefcntHint =1U,Cyc_Absyn_RestrictedHint =2U,Cyc_Absyn_NoHint =3U};
# 187
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_EffKind =3U,Cyc_Absyn_IntKind =4U,Cyc_Absyn_BoolKind =5U,Cyc_Absyn_PtrBndKind =6U,Cyc_Absyn_AqualKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasHint aliashint;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;void*aquals_bound;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*eff;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;void*autoreleased;void*aqual;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*checks_clause;struct Cyc_AssnDef_ExistAssnFn*checks_assn;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_AssnDef_ExistAssnFn*requires_assn;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_AssnDef_ExistAssnFn*ensures_assn;struct Cyc_Absyn_Exp*throws_clause;struct Cyc_AssnDef_ExistAssnFn*throws_assn;struct Cyc_Absyn_Vardecl*return_value;struct Cyc_List_List*arg_vardecls;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct{int tag;void*f1;};struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_Cvar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;void*f4;const char*f5;const char*f6;int f7;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_SubsetType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_AssnDef_ExistAssnFn*f3;};
# 535 "absyn.h"
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U,Cyc_Absyn_Tagof =19U,Cyc_Absyn_UDiv =20U,Cyc_Absyn_UMod =21U,Cyc_Absyn_UGt =22U,Cyc_Absyn_ULt =23U,Cyc_Absyn_UGte =24U,Cyc_Absyn_ULte =25U};
# 542
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
# 560
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Subset_coercion =3U,Cyc_Absyn_Other_coercion =4U};
# 575
enum Cyc_Absyn_MallocKind{Cyc_Absyn_Malloc =0U,Cyc_Absyn_Calloc =1U,Cyc_Absyn_Vmalloc =2U};struct Cyc_Absyn_MallocInfo{enum Cyc_Absyn_MallocKind mknd;struct Cyc_Absyn_Exp*rgn;struct Cyc_Absyn_Exp*aqual;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct _fat_ptr*f0;struct Cyc_Absyn_Tqual f1;void*f2;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple8*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;int f3;};struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f0;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;void*f1;int f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;int f5;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;struct Cyc_Absyn_Exp*rename;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;int escapes;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*fields;int tagged;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};
# 935 "absyn.h"
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 953
void*Cyc_Absyn_compress(void*);
# 988
extern void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);
# 29 "currgn.h"
extern struct _fat_ptr Cyc_CurRgn_curr_rgn_name;
# 39 "tcutil.h"
int Cyc_Tcutil_is_function_type(void*);
# 92
void*Cyc_Tcutil_copy_type(void*);
# 131 "tcutil.h"
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
void*Cyc_Tcutil_substitute_nofun(struct Cyc_List_List*,void*);
# 217
int Cyc_Tcutil_new_tvar_id (void);
# 32 "kinds.h"
extern struct Cyc_Absyn_Kind Cyc_Kinds_ek;
# 77 "kinds.h"
void*Cyc_Kinds_kind_to_bound(struct Cyc_Absyn_Kind*);
# 48 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Tvar_Warn_Warg_struct{int tag;struct Cyc_Absyn_Tvar*f1;};
# 75
void Cyc_Warn_err2(unsigned,struct _fat_ptr);
# 33 "currgn.cyc"
static struct Cyc_Absyn_Tvar*Cyc_CurRgn_curr_tvar=0;
static void*Cyc_CurRgn_curr_rgn_typ=0;static char _TmpG0[3U]="`C";
# 36
struct _fat_ptr Cyc_CurRgn_curr_rgn_name={(unsigned char*)_TmpG0,(unsigned char*)_TmpG0,(unsigned char*)_TmpG0 + 3U};
# 38
struct Cyc_Absyn_Tvar*Cyc_CurRgn_curr_rgn_tvar (void){struct Cyc_Absyn_Tvar*_T0;struct _fat_ptr*_T1;struct Cyc_Absyn_Kind*_T2;struct Cyc_Absyn_Kind*_T3;struct Cyc_Absyn_Tvar*_T4;
if(Cyc_CurRgn_curr_tvar!=0)goto _TL0;{struct Cyc_Absyn_Tvar*_T5=_cycalloc(sizeof(struct Cyc_Absyn_Tvar));{struct _fat_ptr*_T6=_cycalloc(sizeof(struct _fat_ptr));
*_T6=Cyc_CurRgn_curr_rgn_name;_T1=(struct _fat_ptr*)_T6;}_T5->name=_T1;
_T5->identity=Cyc_Tcutil_new_tvar_id();_T2=& Cyc_Kinds_ek;_T3=(struct Cyc_Absyn_Kind*)_T2;
_T5->kind=Cyc_Kinds_kind_to_bound(_T3);
_T5->aquals_bound=0;_T0=(struct Cyc_Absyn_Tvar*)_T5;}
# 40
Cyc_CurRgn_curr_tvar=_T0;
# 44
Cyc_CurRgn_curr_rgn_typ=Cyc_Absyn_var_type(Cyc_CurRgn_curr_tvar);goto _TL1;_TL0: _TL1: _T4=
# 46
_check_null(Cyc_CurRgn_curr_tvar);return _T4;}
# 49
void*Cyc_CurRgn_curr_rgn_type (void){struct Cyc_Absyn_Tvar*_T0;void*_T1;
Cyc_CurRgn_curr_rgn_tvar();_T0=
_check_null(Cyc_CurRgn_curr_tvar);Cyc_CurRgn_curr_rgn_typ=Cyc_Absyn_var_type(_T0);_T1=Cyc_CurRgn_curr_rgn_typ;
return _T1;}struct _tuple11{struct Cyc_Absyn_Tvar*f0;void*f1;};
# 55
void Cyc_CurRgn_subst_inst(struct Cyc_List_List*inst,struct Cyc_Absyn_Tvar*tv,void*t){struct Cyc_List_List*_T0;void*_T1;struct Cyc_Absyn_Tvar**_T2;void**_T3;struct Cyc_Absyn_Tvar**_T4;struct Cyc_Absyn_Tvar*_T5;struct Cyc_Absyn_Tvar*_T6;int _T7;void**_T8;struct Cyc_List_List*_T9;
_TL5: if(inst!=0)goto _TL3;else{goto _TL4;}
_TL3: _T0=inst;_T1=_T0->hd;{struct _tuple11*_TA=(struct _tuple11*)_T1;void*_TB;void*_TC;{struct _tuple11 _TD=*_TA;_T2=& _TA->f0;_TC=(struct Cyc_Absyn_Tvar**)_T2;_T3=& _TA->f1;_TB=(void**)_T3;}{struct Cyc_Absyn_Tvar**tvi=(struct Cyc_Absyn_Tvar**)_TC;void**ti=(void**)_TB;_T4=tvi;_T5=*_T4;_T6=tv;_T7=
Cyc_Absyn_tvar_cmp(_T5,_T6);if(_T7!=0)goto _TL6;_T8=ti;
*_T8=t;
return;_TL6:;}}_T9=inst;goto _TL5;_TL4:;}
# 68
static struct _tuple8*Cyc_CurRgn_argtype(struct _tuple8*arg){struct _tuple8*_T0;void*_T1;int*_T2;int _T3;void*_T4;struct Cyc_Absyn_PtrInfo _T5;int _T6;struct _tuple8*_T7;struct _tuple8*_T8;int*_T9;int _TA;struct Cyc_Absyn_PtrInfo _TB;int*_TC;int _TD;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TE;struct Cyc_List_List**_TF;struct Cyc_List_List**_T10;struct Cyc_List_List**_T11;struct Cyc_List_List*_T12;struct _tuple8*_T13;int(*_T14)(struct _fat_ptr,struct _fat_ptr);void*(*_T15)(struct _fat_ptr,struct _fat_ptr);struct _fat_ptr _T16;struct _fat_ptr _T17;int(*_T18)(struct _fat_ptr,struct _fat_ptr);void*(*_T19)(struct _fat_ptr,struct _fat_ptr);struct _fat_ptr _T1A;struct _fat_ptr _T1B;void*_T1C;struct Cyc_Absyn_Tqual _T1D;struct _fat_ptr*_T1E;_T0=arg;{struct _tuple8 _T1F=*_T0;_T1E=_T1F.f0;_T1D=_T1F.f1;_T1C=_T1F.f2;}{struct _fat_ptr*vopt=_T1E;struct Cyc_Absyn_Tqual tq=_T1D;void*t=_T1C;{void*_T1F;_T1=t;_T2=(int*)_T1;_T3=*_T2;if(_T3!=4)goto _TL8;_T4=t;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T20=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T4;_T5=_T20->f1;_T1F=_T5.elt_type;}{void*t1=_T1F;_T6=
# 72
Cyc_Tcutil_is_function_type(t1);if(_T6)goto _TLA;else{goto _TLC;}_TLC: _T7=arg;return _T7;_TLA: goto _LL3;}_TL8: _T8=arg;
# 74
return _T8;_LL3:;}{
# 76
void*t2=Cyc_Tcutil_copy_type(t);
void*_T1F=Cyc_Absyn_compress(t2);void*_T20;_T9=(int*)_T1F;_TA=*_T9;if(_TA!=4)goto _TLD;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T21=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T1F;_TB=_T21->f1;_T20=_TB.elt_type;}{void*t1=_T20;
# 79
void*_T21=Cyc_Absyn_compress(t1);void*_T22;_TC=(int*)_T21;_TD=*_TC;if(_TD!=6)goto _TLF;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T23=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T21;_TE=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T21;_TF=& _TE->f1.tvars;_T22=(struct Cyc_List_List**)_TF;}{struct Cyc_List_List**tvars=(struct Cyc_List_List**)_T22;_T10=tvars;{
# 83
struct Cyc_List_List*tvars2=*_T10;_T11=tvars;{struct Cyc_List_List*_T23=_cycalloc(sizeof(struct Cyc_List_List));
_T23->hd=_check_null(Cyc_CurRgn_curr_tvar);_T23->tl=tvars2;_T12=(struct Cyc_List_List*)_T23;}*_T11=_T12;{struct _tuple8*_T23=_cycalloc(sizeof(struct _tuple8));
_T23->f0=vopt;_T23->f1=tq;_T23->f2=t2;_T13=(struct _tuple8*)_T23;}return _T13;}}_TLF: _T15=Cyc_Warn_impos;{
int(*_T23)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))_T15;_T14=_T23;}_T16=_tag_fat("internal compiler error: not a function type",sizeof(char),45U);_T17=_tag_fat(0U,sizeof(void*),0);_T14(_T16,_T17);;}goto _TLE;_TLD: _T19=Cyc_Warn_impos;{
# 88
int(*_T21)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))_T19;_T18=_T21;}_T1A=_tag_fat("internal compiler error: not a pointer type",sizeof(char),44U);_T1B=_tag_fat(0U,sizeof(void*),0);_T18(_T1A,_T1B);_TLE:;}}}
# 93
void*Cyc_CurRgn_instantiate(void*fntype,void*currgn){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_FnInfo _T4;struct Cyc_Absyn_FnInfo _T5;struct Cyc_Absyn_FnInfo _T6;struct Cyc_Absyn_FnInfo _T7;void*_T8;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T9;struct Cyc_List_List**_TA;struct Cyc_List_List**_TB;struct Cyc_List_List**_TC;struct Cyc_List_List*(*_TD)(struct _tuple8*(*)(struct _tuple8*),struct Cyc_List_List*);struct Cyc_List_List*(*_TE)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List**_TF;struct Cyc_List_List*_T10;struct Cyc_List_List*_T11;struct _tuple11*_T12;struct Cyc_List_List*_T13;void*_T14;int*_T15;int _T16;void*_T17;struct Cyc_Absyn_FnInfo _T18;void*_T19;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T1A;struct Cyc_List_List**_T1B;struct Cyc_List_List**_T1C;struct Cyc_List_List**_T1D;void*_T1E;void*_T1F;void*_T20;void*_T21;struct Cyc_Absyn_Tqual _T22;void*_T23;struct Cyc_List_List*_T24;_T0=fntype;_T1=(int*)_T0;_T2=*_T1;if(_T2!=6)goto _TL11;_T3=fntype;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T25=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T3;_T4=_T25->f1;_T24=_T4.tvars;_T5=_T25->f1;_T23=_T5.effect;_T6=_T25->f1;_T22=_T6.ret_tqual;_T7=_T25->f1;_T21=_T7.ret_type;_T8=fntype;_T9=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T8;_TA=& _T9->f1.args;_T20=(struct Cyc_List_List**)_TA;}{struct Cyc_List_List*tvars=_T24;void*eff=_T23;struct Cyc_Absyn_Tqual res_tq=_T22;void*res_typ=_T21;struct Cyc_List_List**args_info=(struct Cyc_List_List**)_T20;_TB=args_info;{
# 99
struct Cyc_List_List*old_args_info=*_TB;_TC=args_info;_TE=Cyc_List_map;{
# 101
struct Cyc_List_List*(*_T25)(struct _tuple8*(*)(struct _tuple8*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple8*(*)(struct _tuple8*),struct Cyc_List_List*))_TE;_TD=_T25;}_TF=args_info;_T10=*_TF;*_TC=_TD(Cyc_CurRgn_argtype,_T10);{
# 103
struct Cyc_List_List*inst;inst=_cycalloc(sizeof(struct Cyc_List_List));_T11=inst;{struct _tuple11*_T25=_cycalloc(sizeof(struct _tuple11));_T25->f0=_check_null(Cyc_CurRgn_curr_tvar);_T25->f1=currgn;_T12=(struct _tuple11*)_T25;}_T11->hd=_T12;_T13=inst;_T13->tl=0;{
void*fntype2=Cyc_Tcutil_substitute(inst,fntype);{void*_T25;struct Cyc_List_List*_T26;_T14=fntype2;_T15=(int*)_T14;_T16=*_T15;if(_T16!=6)goto _TL13;_T17=fntype2;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T27=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T17;_T18=_T27->f1;_T26=_T18.tvars;_T19=fntype2;_T1A=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T19;_T1B=& _T1A->f1.args;_T25=(struct Cyc_List_List**)_T1B;}{struct Cyc_List_List*tvars=_T26;struct Cyc_List_List**args_info2=(struct Cyc_List_List**)_T25;_T1C=args_info2;
# 108
*_T1C=old_args_info;{
struct Cyc_List_List*tvars2=tvars;goto _LL5;}}_TL13: goto _LL5;_LL5:;}_T1D=args_info;
# 113
*_T1D=old_args_info;_T1E=fntype2;
return _T1E;}}}}_TL11: _T1F=fntype;
return _T1F;;}
# 120
int Cyc_CurRgn_is_not_curr_rgn(struct Cyc_Absyn_Tvar*v){struct Cyc_Absyn_Tvar*_T0;struct Cyc_Absyn_Tvar*_T1;int _T2;int _T3;_T0=v;_T1=
_check_null(Cyc_CurRgn_curr_tvar);_T2=Cyc_Absyn_tvar_cmp(_T0,_T1);_T3=_T2!=0;return _T3;}
# 124
int Cyc_CurRgn_is_curr_rgn(struct Cyc_Absyn_Tvar*v){struct Cyc_Absyn_Tvar*_T0;struct Cyc_Absyn_Tvar*_T1;int _T2;int _T3;_T0=v;_T1=
_check_null(Cyc_CurRgn_curr_tvar);_T2=Cyc_Absyn_tvar_cmp(_T0,_T1);_T3=_T2==0;return _T3;}struct Cyc_CurRgn_rgnsubenv{struct Cyc_List_List*inst;int in_externC;int toplevel;};
# 134
static void Cyc_CurRgn_rgnsubstmt(struct Cyc_CurRgn_rgnsubenv,struct Cyc_Absyn_Stmt*);
static void Cyc_CurRgn_rgnsubexp(struct Cyc_CurRgn_rgnsubenv,struct Cyc_Absyn_Exp*);
static void Cyc_CurRgn_rgnsubexpopt(struct Cyc_CurRgn_rgnsubenv,struct Cyc_Absyn_Exp*);
static void Cyc_CurRgn_rgnsubds(struct Cyc_CurRgn_rgnsubenv,struct Cyc_List_List*);
static void Cyc_CurRgn_rgnsubdtdecl(unsigned,struct Cyc_CurRgn_rgnsubenv,struct Cyc_Absyn_Datatypedecl*);
static void Cyc_CurRgn_rgnsubaggrdecl(unsigned,struct Cyc_CurRgn_rgnsubenv,struct Cyc_Absyn_Aggrdecl*);
# 142
static void Cyc_CurRgn_check_tvar(unsigned loc,struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;struct Cyc_Absyn_Tvar*_T1;int _T2;struct Cyc_Warn_String_Warn_Warg_struct _T3;struct Cyc_Warn_Tvar_Warn_Warg_struct _T4;unsigned _T5;struct _fat_ptr _T6;_T0=tv;_T1=
_check_null(Cyc_CurRgn_curr_tvar);_T2=Cyc_Absyn_tvar_cmp(_T0,_T1);if(_T2!=0)goto _TL15;{struct Cyc_Warn_String_Warn_Warg_struct _T7;_T7.tag=0;
_T7.f1=_tag_fat("Cannot abstract special type variable ",sizeof(char),39U);_T3=_T7;}{struct Cyc_Warn_String_Warn_Warg_struct _T7=_T3;{struct Cyc_Warn_Tvar_Warn_Warg_struct _T8;_T8.tag=7;_T8.f1=_check_null(Cyc_CurRgn_curr_tvar);_T4=_T8;}{struct Cyc_Warn_Tvar_Warn_Warg_struct _T8=_T4;void*_T9[2];_T9[0]=& _T7;_T9[1]=& _T8;_T5=loc;_T6=_tag_fat(_T9,sizeof(void*),2);Cyc_Warn_err2(_T5,_T6);}}goto _TL16;_TL15: _TL16:;}
# 146
static void Cyc_CurRgn_check_tvars(unsigned loc,struct Cyc_List_List*tvs){unsigned _T0;struct Cyc_List_List*_T1;void*_T2;struct Cyc_Absyn_Tvar*_T3;struct Cyc_List_List*_T4;
_TL1A: if(tvs!=0)goto _TL18;else{goto _TL19;}
_TL18: _T0=loc;_T1=tvs;_T2=_T1->hd;_T3=(struct Cyc_Absyn_Tvar*)_T2;Cyc_CurRgn_check_tvar(_T0,_T3);_T4=tvs;
# 147
tvs=_T4->tl;goto _TL1A;_TL19:;}
# 151
static void Cyc_CurRgn_rgnsubtypes(unsigned,struct Cyc_CurRgn_rgnsubenv,struct Cyc_List_List*);
# 154
static void Cyc_CurRgn_rgnsubtype(unsigned loc,struct Cyc_CurRgn_rgnsubenv env,void*type){int*_T0;unsigned _T1;struct Cyc_CurRgn_rgnsubenv _T2;int _T3;unsigned _T4;struct Cyc_CurRgn_rgnsubenv _T5;struct Cyc_Absyn_Vardecl*_T6;void*_T7;struct Cyc_Absyn_ArrayInfo _T8;struct Cyc_Absyn_ArrayInfo _T9;struct Cyc_Absyn_ArrayInfo _TA;struct Cyc_Absyn_PtrInfo _TB;struct Cyc_Absyn_PtrInfo _TC;struct Cyc_Absyn_PtrAtts _TD;struct Cyc_Absyn_PtrInfo _TE;struct Cyc_Absyn_PtrAtts _TF;struct Cyc_Absyn_PtrInfo _T10;struct Cyc_Absyn_PtrAtts _T11;struct Cyc_Absyn_PtrInfo _T12;struct Cyc_Absyn_PtrAtts _T13;unsigned _T14;struct Cyc_CurRgn_rgnsubenv _T15;struct Cyc_List_List*_T16;void*_T17;struct Cyc_Absyn_Aggrfield*_T18;void*_T19;struct Cyc_CurRgn_rgnsubenv _T1A;struct Cyc_List_List*_T1B;void*_T1C;struct Cyc_Absyn_Aggrfield*_T1D;struct Cyc_Absyn_Exp*_T1E;struct Cyc_List_List*_T1F;void*_T20;void*_T21;void*_T22;int(*_T23)(struct _fat_ptr,struct _fat_ptr,unsigned);void*(*_T24)(struct _fat_ptr,struct _fat_ptr,unsigned);struct _fat_ptr _T25;struct _fat_ptr _T26;struct Cyc_Absyn_TypeDecl*_T27;int*_T28;unsigned _T29;
void*_T2A=Cyc_Absyn_compress(type);void**_T2B;struct Cyc_Absyn_TypeDecl*_T2C;struct Cyc_Absyn_Exp*_T2D;struct Cyc_List_List*_T2E;void*_T2F;void*_T30;void*_T31;void*_T32;void*_T33;_T0=(int*)_T2A;_T1=*_T0;switch(_T1){case 2:{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T2A;_T33=_T34->f1;}{struct Cyc_Absyn_Tvar*v=_T33;_T2=env;_T3=_T2.toplevel;
# 157
if(!_T3)goto _TL1C;Cyc_CurRgn_check_tvar(loc,v);goto _TL1D;_TL1C: _TL1D: return;}case 8:{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T2A;_T33=_T34->f2;}{struct Cyc_List_List*ts=_T33;
Cyc_CurRgn_rgnsubtypes(loc,env,ts);return;}case 12:{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_T2A;_T33=_T34->f1;_T32=_T34->f2;}{struct Cyc_Absyn_Vardecl*vd=_T33;struct Cyc_Absyn_Exp*e=_T32;_T4=loc;_T5=env;_T6=vd;_T7=_T6->type;
# 160
Cyc_CurRgn_rgnsubtype(_T4,_T5,_T7);
Cyc_CurRgn_rgnsubexpopt(env,e);
return;}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T2A;_T8=_T34->f1;_T33=_T8.elt_type;_T9=_T34->f1;_T32=_T9.num_elts;_TA=_T34->f1;_T31=_TA.zero_term;}{void*t1=_T33;struct Cyc_Absyn_Exp*e=_T32;void*zt=_T31;
# 164
Cyc_CurRgn_rgnsubtype(loc,env,t1);
Cyc_CurRgn_rgnsubexpopt(env,e);
Cyc_CurRgn_rgnsubtype(loc,env,zt);
return;}case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T2A;_TB=_T34->f1;_T33=_TB.elt_type;_TC=_T34->f1;_TD=_TC.ptr_atts;_T32=_TD.eff;_TE=_T34->f1;_TF=_TE.ptr_atts;_T31=_TF.bounds;_T10=_T34->f1;_T11=_T10.ptr_atts;_T30=_T11.zero_term;_T12=_T34->f1;_T13=_T12.ptr_atts;_T2F=_T13.autoreleased;}{void*t1=_T33;void*r=_T32;void*b=_T31;void*zt=_T30;void*rel=_T2F;
# 169
Cyc_CurRgn_rgnsubtype(loc,env,t1);
Cyc_CurRgn_rgnsubtype(loc,env,r);
Cyc_CurRgn_rgnsubtype(loc,env,b);
Cyc_CurRgn_rgnsubtype(loc,env,zt);
Cyc_CurRgn_rgnsubtype(loc,env,rel);
return;}case 6:
# 176
 return;case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T2A;_T33=_T34->f3;}{struct Cyc_List_List*fs=_T33;
# 178
_TL21: if(fs!=0)goto _TL1F;else{goto _TL20;}
_TL1F: _T14=loc;_T15=env;_T16=fs;_T17=_T16->hd;_T18=(struct Cyc_Absyn_Aggrfield*)_T17;_T19=_T18->type;Cyc_CurRgn_rgnsubtype(_T14,_T15,_T19);_T1A=env;_T1B=fs;_T1C=_T1B->hd;_T1D=(struct Cyc_Absyn_Aggrfield*)_T1C;_T1E=_T1D->requires_clause;
Cyc_CurRgn_rgnsubexpopt(_T1A,_T1E);_T1F=fs;
# 178
fs=_T1F->tl;goto _TL21;_TL20:
# 182
 return;}case 3:{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_T34=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T2A;_T20=_T34->f2;_T33=(void*)_T20;}{void*r=_T33;_T33=r;goto _LL12;}case 1:{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T34=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T2A;_T21=_T34->f2;_T33=(void*)_T21;}_LL12: {void*r=_T33;
# 185
if(r==0)goto _TL22;Cyc_CurRgn_rgnsubtype(loc,env,r);goto _TL23;_TL22: _TL23:
 return;}case 0:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2A;_T22=_T34->f1;_T33=(void*)_T22;_T2E=_T34->f2;}{void*c=_T33;struct Cyc_List_List*ts=_T2E;
Cyc_CurRgn_rgnsubtypes(loc,env,ts);return;}case 9:{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_T2A;_T2D=_T34->f1;}{struct Cyc_Absyn_Exp*e=_T2D;
Cyc_CurRgn_rgnsubexp(env,e);return;}case 11:{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_T2A;_T2D=_T34->f1;}{struct Cyc_Absyn_Exp*e=_T2D;
Cyc_CurRgn_rgnsubexp(env,e);return;}default:{struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_T2A;_T2C=_T34->f1;_T2B=_T34->f2;}{struct Cyc_Absyn_TypeDecl*td=_T2C;void**topt=_T2B;
# 191
if(topt!=0)goto _TL24;goto _TL25;_TL24: _T24=Cyc___assert_fail;{int(*_T34)(struct _fat_ptr,struct _fat_ptr,unsigned)=(int(*)(struct _fat_ptr,struct _fat_ptr,unsigned))_T24;_T23=_T34;}_T25=_tag_fat("topt == NULL",sizeof(char),13U);_T26=_tag_fat("currgn.cyc",sizeof(char),11U);_T23(_T25,_T26,191U);_TL25: _T27=td;{
void*_T34=_T27->r;struct Cyc_Absyn_Datatypedecl*_T35;struct Cyc_Absyn_Aggrdecl*_T36;_T28=(int*)_T34;_T29=*_T28;switch(_T29){case 0:{struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*_T37=(struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)_T34;_T36=_T37->f1;}{struct Cyc_Absyn_Aggrdecl*ad=_T36;
Cyc_CurRgn_rgnsubaggrdecl(loc,env,ad);return;}case 2:{struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*_T37=(struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)_T34;_T35=_T37->f1;}{struct Cyc_Absyn_Datatypedecl*dtd=_T35;
Cyc_CurRgn_rgnsubdtdecl(loc,env,dtd);return;}default:
 return;};}
# 197
return;}};}
# 201
static void Cyc_CurRgn_rgnsubtypes(unsigned loc,struct Cyc_CurRgn_rgnsubenv env,struct Cyc_List_List*ts){unsigned _T0;struct Cyc_CurRgn_rgnsubenv _T1;struct Cyc_List_List*_T2;void*_T3;struct Cyc_List_List*_T4;
# 203
_TL2A: if(ts!=0)goto _TL28;else{goto _TL29;}_TL28: _T0=loc;_T1=env;_T2=ts;_T3=_T2->hd;Cyc_CurRgn_rgnsubtype(_T0,_T1,_T3);_T4=ts;ts=_T4->tl;goto _TL2A;_TL29:;}
# 208
static void Cyc_CurRgn_rgnsubexp(struct Cyc_CurRgn_rgnsubenv env,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_T3;void**_T4;struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*_T5;void**_T6;struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_T7;void**_T8;struct Cyc_Absyn_Exp*_T9;unsigned _TA;struct Cyc_CurRgn_rgnsubenv _TB;void**_TC;void*_TD;struct Cyc_CurRgn_rgnsubenv _TE;int _TF;void**_T10;struct Cyc_CurRgn_rgnsubenv _T11;struct Cyc_List_List*_T12;void**_T13;void*_T14;struct Cyc_CurRgn_rgnsubenv _T15;struct Cyc_List_List*_T16;void*_T17;struct Cyc_Absyn_Exp*_T18;struct Cyc_List_List*_T19;struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_T1A;void**_T1B;struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T1C;void**_T1D;struct Cyc_Absyn_Exp*_T1E;unsigned _T1F;struct Cyc_CurRgn_rgnsubenv _T20;void**_T21;void*_T22;struct Cyc_CurRgn_rgnsubenv _T23;int _T24;void**_T25;struct Cyc_CurRgn_rgnsubenv _T26;struct Cyc_List_List*_T27;void**_T28;void*_T29;struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_T2A;void**_T2B;struct Cyc_Absyn_Exp*_T2C;unsigned _T2D;struct Cyc_CurRgn_rgnsubenv _T2E;void**_T2F;void*_T30;struct Cyc_CurRgn_rgnsubenv _T31;int _T32;struct Cyc_CurRgn_rgnsubenv _T33;struct Cyc_List_List*_T34;void**_T35;void*_T36;void*_T37;void**_T38;void*_T39;int*_T3A;int _T3B;struct Cyc_Absyn_Exp*_T3C;struct Cyc_Absyn_Exp*_T3D;struct Cyc_Absyn_Exp*_T3E;struct Cyc_Absyn_Exp*_T3F;struct Cyc_Absyn_Exp*_T40;struct Cyc_Absyn_Exp*_T41;void**_T42;struct Cyc_CurRgn_rgnsubenv _T43;struct Cyc_List_List*_T44;void*_T45;struct Cyc_Absyn_Exp*_T46;struct Cyc_List_List*_T47;struct Cyc_Absyn_Exp*_T48;unsigned _T49;struct Cyc_CurRgn_rgnsubenv _T4A;struct Cyc_List_List*_T4B;void*_T4C;struct Cyc_CurRgn_rgnsubenv _T4D;int _T4E;struct Cyc_List_List*_T4F;struct Cyc_CurRgn_rgnsubenv _T50;struct Cyc_List_List*_T51;struct Cyc_List_List*_T52;void*_T53;void*_T54;struct Cyc_List_List*_T55;struct Cyc_Absyn_Exp*_T56;unsigned _T57;struct Cyc_CurRgn_rgnsubenv _T58;struct _tuple8*_T59;struct _tuple8 _T5A;void*_T5B;struct Cyc_CurRgn_rgnsubenv _T5C;int _T5D;struct _tuple8*_T5E;struct Cyc_CurRgn_rgnsubenv _T5F;struct Cyc_List_List*_T60;struct _tuple8*_T61;struct _tuple8 _T62;void*_T63;struct Cyc_Absyn_Exp*_T64;unsigned _T65;struct Cyc_CurRgn_rgnsubenv _T66;struct Cyc_List_List*_T67;void*_T68;struct Cyc_CurRgn_rgnsubenv _T69;int _T6A;struct Cyc_List_List*_T6B;struct Cyc_CurRgn_rgnsubenv _T6C;struct Cyc_List_List*_T6D;struct Cyc_List_List*_T6E;void*_T6F;void*_T70;struct Cyc_List_List*_T71;struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_T72;void**_T73;void**_T74;void*_T75;struct Cyc_Absyn_Exp*_T76;unsigned _T77;struct Cyc_CurRgn_rgnsubenv _T78;void**_T79;void*_T7A;struct Cyc_CurRgn_rgnsubenv _T7B;int _T7C;void**_T7D;struct Cyc_CurRgn_rgnsubenv _T7E;struct Cyc_List_List*_T7F;void**_T80;void*_T81;void*_T82;struct Cyc_Absyn_MallocInfo _T83;struct Cyc_Absyn_MallocInfo _T84;struct Cyc_Absyn_MallocInfo _T85;struct Cyc_Absyn_MallocInfo _T86;struct Cyc_CurRgn_rgnsubenv _T87;int _T88;int(*_T89)(struct _fat_ptr,struct _fat_ptr,unsigned);void*(*_T8A)(struct _fat_ptr,struct _fat_ptr,unsigned);struct _fat_ptr _T8B;struct _fat_ptr _T8C;struct Cyc_Absyn_Exp*_T8D;unsigned _T8E;struct Cyc_CurRgn_rgnsubenv _T8F;void**_T90;void*_T91;void**_T92;struct Cyc_CurRgn_rgnsubenv _T93;struct Cyc_List_List*_T94;void**_T95;void*_T96;struct Cyc_Absyn_Exp*_T97;void*_T98;struct Cyc_Absyn_Exp*_T99;unsigned _T9A;struct Cyc_CurRgn_rgnsubenv _T9B;struct Cyc_Absyn_Exp*_T9C;void*_T9D;struct Cyc_CurRgn_rgnsubenv _T9E;int _T9F;struct Cyc_Absyn_Exp*_TA0;struct Cyc_CurRgn_rgnsubenv _TA1;struct Cyc_List_List*_TA2;struct Cyc_Absyn_Exp*_TA3;void*_TA4;void*_TA5;_T0=e;{
void*_TA6=_T0->r;struct Cyc_Absyn_Stmt*_TA7;struct Cyc_List_List*_TA8;struct _tuple8*_TA9;struct Cyc_Absyn_VarargCallInfo*_TAA;struct Cyc_Absyn_Datatypefield*_TAB;struct Cyc_Absyn_Datatypedecl*_TAC;int _TAD;struct Cyc_Absyn_Vardecl*_TAE;struct Cyc_Absyn_Exp*_TAF;struct Cyc_Absyn_Exp*_TB0;struct Cyc_Absyn_Exp*_TB1;struct Cyc_List_List*_TB2;void**_TB3;_T1=(int*)_TA6;_T2=*_T1;switch(_T2){case 0: goto _LL4;case 33: _LL4: goto _LL6;case 34: _LL6: goto _LL8;case 2: _LL8: goto _LLA;case 41: _LLA: goto _LLC;case 1: _LLC: goto _LL0;case 21:{struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_TA6;_T3=(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_TA6;_T4=& _T3->f1;_TB3=(void**)_T4;_TB2=_TB4->f2;}{void**t=_TB3;struct Cyc_List_List*f=_TB2;_TB3=t;goto _LL10;}case 19:{struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*)_TA6;_T5=(struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*)_TA6;_T6=& _T5->f1;_TB3=(void**)_T6;}_LL10: {void**t=_TB3;_TB3=t;goto _LL12;}case 17:{struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_TA6;_T7=(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_TA6;_T8=& _T7->f1;_TB3=(void**)_T8;}_LL12: {void**t=_TB3;_T9=e;_TA=_T9->loc;_TB=env;_TC=t;_TD=*_TC;
# 220
Cyc_CurRgn_rgnsubtype(_TA,_TB,_TD);_TE=env;_TF=_TE.toplevel;
if(_TF)goto _TL2C;else{goto _TL2E;}
_TL2E: _T10=t;_T11=env;_T12=_T11.inst;_T13=t;_T14=*_T13;*_T10=Cyc_Tcutil_substitute_nofun(_T12,_T14);goto _TL2D;_TL2C: _TL2D: goto _LL0;}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f2;_TAF=_TB4->f3;}{struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;struct Cyc_Absyn_Exp*e3=_TAF;
# 226
Cyc_CurRgn_rgnsubexp(env,e1);Cyc_CurRgn_rgnsubexp(env,e2);Cyc_CurRgn_rgnsubexp(env,e3);goto _LL0;}case 28:{struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_TA6;_TAE=_TB4->f1;_TB1=_TB4->f2;_TB0=_TB4->f3;_TAD=_TB4->f4;}{struct Cyc_Absyn_Vardecl*vd=_TAE;struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;int res=_TAD;_TB1=e1;_TB0=e2;goto _LL18;}case 4:{struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f3;}_LL18: {struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;_TB1=e1;_TB0=e2;goto _LL1A;}case 36:{struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f2;}_LL1A: {struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;_TB1=e1;_TB0=e2;goto _LL1C;}case 7:{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f2;}_LL1C: {struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;_TB1=e1;_TB0=e2;goto _LL1E;}case 8:{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f2;}_LL1E: {struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;_TB1=e1;_TB0=e2;goto _LL20;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f2;}_LL20: {struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;_TB1=e1;_TB0=e2;goto _LL22;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f2;}_LL22: {struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_Absyn_Exp*e2=_TB0;
# 234
Cyc_CurRgn_rgnsubexp(env,e1);Cyc_CurRgn_rgnsubexp(env,e2);goto _LL0;}case 32:{struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_TA6;_TB2=_TB4->f1;_TAC=_TB4->f2;_TAB=_TB4->f3;}{struct Cyc_List_List*es=_TB2;struct Cyc_Absyn_Datatypedecl*dtd=_TAC;struct Cyc_Absyn_Datatypefield*dtfd=_TAB;_TB2=es;goto _LL26;}case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_TA6;_TB2=_TB4->f2;}_LL26: {struct Cyc_List_List*es=_TB2;
# 239
_TL32: if(es!=0)goto _TL30;else{goto _TL31;}
_TL30: _T15=env;_T16=es;_T17=_T16->hd;_T18=(struct Cyc_Absyn_Exp*)_T17;Cyc_CurRgn_rgnsubexp(_T15,_T18);_T19=es;
# 239
es=_T19->tl;goto _TL32;_TL31: goto _LL0;}case 29:{struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_T1A=(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_TA6;_T1B=& _T1A->f2;_TB3=(void**)_T1B;}{struct Cyc_Absyn_Exp*e1=_TB1;void**t=_TB3;_TB3=t;_TB1=e1;goto _LL2A;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_TA6;_T1C=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_TA6;_T1D=& _T1C->f1;_TB3=(void**)_T1D;_TB1=_TB4->f2;}_LL2A: {void**t=_TB3;struct Cyc_Absyn_Exp*e1=_TB1;_T1E=e;_T1F=_T1E->loc;_T20=env;_T21=t;_T22=*_T21;
# 244
Cyc_CurRgn_rgnsubtype(_T1F,_T20,_T22);_T23=env;_T24=_T23.toplevel;
if(_T24)goto _TL33;else{goto _TL35;}
_TL35: _T25=t;_T26=env;_T27=_T26.inst;_T28=t;_T29=*_T28;*_T25=Cyc_Tcutil_substitute_nofun(_T27,_T29);goto _TL34;_TL33: _TL34: _TB1=e1;goto _LL2C;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL2C: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL2E;}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL2E: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL30;}case 39:{struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL30: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL32;}case 5:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL32: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL34;}case 43:{struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL34: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL36;}case 44:{struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL36: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL38;}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL38: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL3A;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL3A: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL3C;}case 18:{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL3C: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL3E;}case 20:{struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL3E: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL40;}case 42:{struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL40: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL42;}case 11:{struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL42: {struct Cyc_Absyn_Exp*e1=_TB1;_TB1=e1;goto _LL44;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;}_LL44: {struct Cyc_Absyn_Exp*e1=_TB1;
# 261
Cyc_CurRgn_rgnsubexp(env,e1);goto _LL0;}case 40:{struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_TA6;_T2A=(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_TA6;_T2B=& _T2A->f1;_TB3=(void**)_T2B;}{void**t=_TB3;_T2C=e;_T2D=_T2C->loc;_T2E=env;_T2F=t;_T30=*_T2F;
# 263
Cyc_CurRgn_rgnsubtype(_T2D,_T2E,_T30);_T31=env;_T32=_T31.toplevel;
if(_T32)goto _TL36;else{goto _TL38;}
_TL38: _T33=env;_T34=_T33.inst;_T35=t;_T36=*_T35;{void*new_typ=Cyc_Tcutil_substitute_nofun(_T34,_T36);_T37=new_typ;_T38=t;_T39=*_T38;
if(_T37!=_T39)goto _TL39;goto _LL0;_TL39: {
# 268
void*_TB4=Cyc_Absyn_compress(new_typ);struct Cyc_Absyn_Exp*_TB5;_T3A=(int*)_TB4;_T3B=*_T3A;if(_T3B!=9)goto _TL3B;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TB6=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TB4;_TB5=_TB6->f1;}{struct Cyc_Absyn_Exp*e1=_TB5;_T3C=e;_T3D=e1;
# 270
_T3C->r=_T3D->r;_T3E=e;_T3F=e1;
_T3E->topt=_T3F->topt;_T40=e;_T41=e1;
_T40->annot=_T41->annot;goto _LL5B;}_TL3B: _T42=t;
# 274
*_T42=new_typ;goto _LL5B;_LL5B:;}}goto _TL37;_TL36: _TL37: goto _LL0;}case 10:{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB2=_TB4->f2;_TAA=_TB4->f3;_TAD=_TB4->f4;}{struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_List_List*es=_TB2;struct Cyc_Absyn_VarargCallInfo*vararg=_TAA;int resv=_TAD;
# 279
_TL3D: if(es!=0)goto _TL3E;else{goto _TL3F;}
_TL3E: _T43=env;_T44=es;_T45=_T44->hd;_T46=(struct Cyc_Absyn_Exp*)_T45;Cyc_CurRgn_rgnsubexp(_T43,_T46);_T47=es;
es=_T47->tl;goto _TL3D;_TL3F:
# 283
 Cyc_CurRgn_rgnsubexp(env,e1);goto _LL0;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB2=_TB4->f2;}{struct Cyc_Absyn_Exp*e1=_TB1;struct Cyc_List_List*ts=_TB2;
# 286
_TL40: if(ts!=0)goto _TL41;else{goto _TL42;}
_TL41: _T48=e;_T49=_T48->loc;_T4A=env;_T4B=ts;_T4C=_T4B->hd;Cyc_CurRgn_rgnsubtype(_T49,_T4A,_T4C);_T4D=env;_T4E=_T4D.toplevel;
if(_T4E)goto _TL43;else{goto _TL45;}
_TL45: _T4F=ts;_T50=env;_T51=_T50.inst;_T52=ts;_T53=_T52->hd;_T54=Cyc_Tcutil_substitute_nofun(_T51,_T53);_T4F->hd=(void*)_T54;goto _TL44;_TL43: _TL44: _T55=ts;
ts=_T55->tl;goto _TL40;_TL42:
# 292
 Cyc_CurRgn_rgnsubexp(env,e1);goto _LL0;}case 16:{struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_TA6;_TB1=_TB4->f1;_TB0=_TB4->f2;_TAF=_TB4->f3;}{struct Cyc_Absyn_Exp*eopt=_TB1;struct Cyc_Absyn_Exp*e1=_TB0;struct Cyc_Absyn_Exp*qopt=_TAF;
# 296
Cyc_CurRgn_rgnsubexpopt(env,eopt);Cyc_CurRgn_rgnsubexpopt(env,qopt);Cyc_CurRgn_rgnsubexp(env,e1);goto _LL0;}case 26:{struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_TA6;_TA9=_TB4->f1;_TB2=_TB4->f2;}{struct _tuple8*i=_TA9;struct Cyc_List_List*ds=_TB2;_T56=e;_T57=_T56->loc;_T58=env;_T59=i;_T5A=*_T59;_T5B=_T5A.f2;
# 299
Cyc_CurRgn_rgnsubtype(_T57,_T58,_T5B);_T5C=env;_T5D=_T5C.toplevel;
if(_T5D)goto _TL46;else{goto _TL48;}
_TL48: _T5E=i;_T5F=env;_T60=_T5F.inst;_T61=i;_T62=*_T61;_T63=_T62.f2;(*_T5E).f2=Cyc_Tcutil_substitute_nofun(_T60,_T63);goto _TL47;_TL46: _TL47:
 Cyc_CurRgn_rgnsubds(env,ds);goto _LL0;}case 37:{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_TA6;_TB2=_TB4->f2;}{struct Cyc_List_List*ds=_TB2;
# 305
Cyc_CurRgn_rgnsubds(env,ds);goto _LL0;}case 27:{struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_TA6;_TB2=_TB4->f1;}{struct Cyc_List_List*ds=_TB2;
Cyc_CurRgn_rgnsubds(env,ds);goto _LL0;}case 30:{struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_TA6;_TB2=_TB4->f2;_TA8=_TB4->f3;}{struct Cyc_List_List*ts=_TB2;struct Cyc_List_List*ds=_TA8;
# 309
_TL4C: if(ts!=0)goto _TL4A;else{goto _TL4B;}
_TL4A: _T64=e;_T65=_T64->loc;_T66=env;_T67=ts;_T68=_T67->hd;Cyc_CurRgn_rgnsubtype(_T65,_T66,_T68);_T69=env;_T6A=_T69.toplevel;
if(_T6A)goto _TL4D;else{goto _TL4F;}
_TL4F: _T6B=ts;_T6C=env;_T6D=_T6C.inst;_T6E=ts;_T6F=_T6E->hd;_T70=Cyc_Tcutil_substitute_nofun(_T6D,_T6F);_T6B->hd=(void*)_T70;goto _TL4E;_TL4D: _TL4E: _T71=ts;
# 309
ts=_T71->tl;goto _TL4C;_TL4B:
# 314
 Cyc_CurRgn_rgnsubds(env,ds);goto _LL0;}case 31:{struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_TA6;_T72=(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_TA6;_T73=& _T72->f1;_TB3=(void**)_T73;_TB2=_TB4->f3;}{void**t=_TB3;struct Cyc_List_List*ds=_TB2;_T74=t;_T75=*_T74;
# 318
if(_T75==0)goto _TL50;_T76=e;_T77=_T76->loc;_T78=env;_T79=t;_T7A=*_T79;
Cyc_CurRgn_rgnsubtype(_T77,_T78,_T7A);_T7B=env;_T7C=_T7B.toplevel;
if(_T7C)goto _TL52;else{goto _TL54;}
_TL54: _T7D=t;_T7E=env;_T7F=_T7E.inst;_T80=t;_T81=*_T80;_T82=_check_null(_T81);*_T7D=Cyc_Tcutil_substitute_nofun(_T7F,_T82);goto _TL53;_TL52: _TL53: goto _TL51;_TL50: _TL51:
# 323
 Cyc_CurRgn_rgnsubds(env,ds);goto _LL0;}case 35:{struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_TA6;_T83=_TB4->f1;_TB1=_T83.rgn;_T84=_TB4->f1;_TB0=_T84.aqual;_T85=_TB4->f1;_TB3=_T85.elt_type;_T86=_TB4->f1;_TAF=_T86.num_elts;}{struct Cyc_Absyn_Exp*rgnopt=_TB1;struct Cyc_Absyn_Exp*aqopt=_TB0;void**elt_t=_TB3;struct Cyc_Absyn_Exp*e1=_TAF;_T87=env;_T88=_T87.toplevel;
# 327
if(_T88)goto _TL55;else{goto _TL57;}_TL57: goto _TL56;_TL55: _T8A=Cyc___assert_fail;{int(*_TB4)(struct _fat_ptr,struct _fat_ptr,unsigned)=(int(*)(struct _fat_ptr,struct _fat_ptr,unsigned))_T8A;_T89=_TB4;}_T8B=_tag_fat("!env.toplevel",sizeof(char),14U);_T8C=_tag_fat("currgn.cyc",sizeof(char),11U);_T89(_T8B,_T8C,327U);_TL56:
 Cyc_CurRgn_rgnsubexpopt(env,rgnopt);
Cyc_CurRgn_rgnsubexpopt(env,aqopt);
if(elt_t==0)goto _TL58;_T8D=e;_T8E=_T8D->loc;_T8F=env;_T90=elt_t;_T91=*_T90;
Cyc_CurRgn_rgnsubtype(_T8E,_T8F,_T91);_T92=elt_t;_T93=env;_T94=_T93.inst;_T95=elt_t;_T96=*_T95;
*_T92=Cyc_Tcutil_substitute_nofun(_T94,_T96);goto _TL59;_TL58: _TL59:
# 334
 Cyc_CurRgn_rgnsubexp(env,e1);goto _LL0;}default:{struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_TB4=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_TA6;_TA7=_TB4->f1;}{struct Cyc_Absyn_Stmt*s=_TA7;
# 337
Cyc_CurRgn_rgnsubstmt(env,s);goto _LL0;}}_LL0:;}_T97=e;_T98=_T97->topt;
# 339
if(_T98==0)goto _TL5A;_T99=e;_T9A=_T99->loc;_T9B=env;_T9C=e;_T9D=_T9C->topt;
Cyc_CurRgn_rgnsubtype(_T9A,_T9B,_T9D);_T9E=env;_T9F=_T9E.toplevel;
if(_T9F)goto _TL5C;else{goto _TL5E;}
_TL5E: _TA0=e;_TA1=env;_TA2=_TA1.inst;_TA3=e;_TA4=_TA3->topt;_TA5=_check_null(_TA4);_TA0->topt=Cyc_Tcutil_substitute_nofun(_TA2,_TA5);goto _TL5D;_TL5C: _TL5D: goto _TL5B;_TL5A: _TL5B:;}
# 346
static void Cyc_CurRgn_rgnsubexpopt(struct Cyc_CurRgn_rgnsubenv env,struct Cyc_Absyn_Exp*e){
if(e!=0)goto _TL5F;return;
_TL5F: Cyc_CurRgn_rgnsubexp(env,e);;}struct _tuple12{struct Cyc_List_List*f0;struct Cyc_Absyn_Exp*f1;};
# 351
static void Cyc_CurRgn_rgnsubds(struct Cyc_CurRgn_rgnsubenv env,struct Cyc_List_List*ds){struct Cyc_CurRgn_rgnsubenv _T0;struct Cyc_List_List*_T1;void*_T2;struct _tuple12*_T3;struct _tuple12 _T4;struct Cyc_Absyn_Exp*_T5;struct Cyc_List_List*_T6;
# 353
_TL64: if(ds!=0)goto _TL62;else{goto _TL63;}
_TL62: _T0=env;_T1=ds;_T2=_T1->hd;_T3=(struct _tuple12*)_T2;_T4=*_T3;_T5=_T4.f1;Cyc_CurRgn_rgnsubexp(_T0,_T5);_T6=ds;
# 353
ds=_T6->tl;goto _TL64;_TL63:;}struct _tuple13{struct Cyc_List_List*f0;struct Cyc_Absyn_Pat*f1;};
# 357
static void Cyc_CurRgn_rgnsubpat(struct Cyc_CurRgn_rgnsubenv env,struct Cyc_Absyn_Pat*p){struct Cyc_CurRgn_rgnsubenv _T0;int _T1;int(*_T2)(struct _fat_ptr,struct _fat_ptr,unsigned);void*(*_T3)(struct _fat_ptr,struct _fat_ptr,unsigned);struct _fat_ptr _T4;struct _fat_ptr _T5;struct Cyc_Absyn_Pat*_T6;int*_T7;unsigned _T8;struct Cyc_Absyn_Vardecl*_T9;void*_TA;struct Cyc_Absyn_Pat*_TB;unsigned _TC;struct Cyc_CurRgn_rgnsubenv _TD;struct Cyc_Absyn_Vardecl*_TE;void*_TF;struct Cyc_Absyn_Vardecl*_T10;struct Cyc_CurRgn_rgnsubenv _T11;struct Cyc_List_List*_T12;struct Cyc_Absyn_Vardecl*_T13;void*_T14;struct Cyc_Absyn_Pat*_T15;unsigned _T16;struct Cyc_Absyn_Tvar*_T17;struct Cyc_Absyn_Vardecl*_T18;void*_T19;struct Cyc_Absyn_Pat*_T1A;unsigned _T1B;struct Cyc_CurRgn_rgnsubenv _T1C;struct Cyc_Absyn_Vardecl*_T1D;void*_T1E;struct Cyc_Absyn_Vardecl*_T1F;struct Cyc_CurRgn_rgnsubenv _T20;struct Cyc_List_List*_T21;struct Cyc_Absyn_Vardecl*_T22;void*_T23;struct Cyc_Absyn_Vardecl*_T24;void*_T25;struct Cyc_Absyn_Pat*_T26;unsigned _T27;struct Cyc_CurRgn_rgnsubenv _T28;struct Cyc_Absyn_Vardecl*_T29;void*_T2A;struct Cyc_Absyn_Vardecl*_T2B;struct Cyc_CurRgn_rgnsubenv _T2C;struct Cyc_List_List*_T2D;struct Cyc_Absyn_Vardecl*_T2E;void*_T2F;struct Cyc_Absyn_Pat*_T30;unsigned _T31;struct Cyc_Absyn_Tvar*_T32;struct Cyc_Absyn_Pat*_T33;unsigned _T34;struct Cyc_List_List*_T35;void*_T36;struct Cyc_Absyn_Tvar*_T37;struct Cyc_List_List*_T38;struct Cyc_CurRgn_rgnsubenv _T39;struct Cyc_List_List*_T3A;void*_T3B;struct _tuple13*_T3C;struct _tuple13 _T3D;struct Cyc_Absyn_Pat*_T3E;struct Cyc_List_List*_T3F;struct Cyc_CurRgn_rgnsubenv _T40;struct Cyc_List_List*_T41;void*_T42;struct Cyc_Absyn_Pat*_T43;struct Cyc_List_List*_T44;_T0=env;_T1=_T0.toplevel;
if(_T1)goto _TL65;else{goto _TL67;}_TL67: goto _TL66;_TL65: _T3=Cyc___assert_fail;{int(*_T45)(struct _fat_ptr,struct _fat_ptr,unsigned)=(int(*)(struct _fat_ptr,struct _fat_ptr,unsigned))_T3;_T2=_T45;}_T4=_tag_fat("!env.toplevel",sizeof(char),14U);_T5=_tag_fat("currgn.cyc",sizeof(char),11U);_T2(_T4,_T5,358U);_TL66: _T6=p;{
void*_T45=_T6->r;struct Cyc_Absyn_Exp*_T46;struct Cyc_List_List*_T47;struct Cyc_List_List*_T48;struct Cyc_Absyn_Tvar*_T49;struct Cyc_Absyn_Pat*_T4A;struct Cyc_Absyn_Vardecl*_T4B;_T7=(int*)_T45;_T8=*_T7;switch(_T8){case 0:
 return;case 1:{struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_T45;_T4B=_T4C->f1;_T4A=_T4C->f2;}{struct Cyc_Absyn_Vardecl*vd=_T4B;struct Cyc_Absyn_Pat*p1=_T4A;_T4B=vd;_T4A=p1;goto _LL6;}case 3:{struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_T45;_T4B=_T4C->f1;_T4A=_T4C->f2;}_LL6: {struct Cyc_Absyn_Vardecl*vd=_T4B;struct Cyc_Absyn_Pat*p1=_T4A;_T9=vd;_TA=_T9->type;
# 365
if(_TA==0)goto _TL69;_TB=p;_TC=_TB->loc;_TD=env;_TE=vd;_TF=_TE->type;
Cyc_CurRgn_rgnsubtype(_TC,_TD,_TF);_T10=vd;_T11=env;_T12=_T11.inst;_T13=vd;_T14=_T13->type;
_T10->type=Cyc_Tcutil_substitute_nofun(_T12,_T14);goto _TL6A;_TL69: _TL6A:
# 369
 Cyc_CurRgn_rgnsubpat(env,p1);
return;}case 2:{struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_T45;_T49=_T4C->f1;_T4B=_T4C->f2;}{struct Cyc_Absyn_Tvar*tv=_T49;struct Cyc_Absyn_Vardecl*vd=_T4B;_T15=p;_T16=_T15->loc;_T17=tv;
# 373
Cyc_CurRgn_check_tvar(_T16,_T17);_T18=vd;_T19=_T18->type;
if(_T19==0)goto _TL6B;_T1A=p;_T1B=_T1A->loc;_T1C=env;_T1D=vd;_T1E=_T1D->type;
Cyc_CurRgn_rgnsubtype(_T1B,_T1C,_T1E);_T1F=vd;_T20=env;_T21=_T20.inst;_T22=vd;_T23=_T22->type;
_T1F->type=Cyc_Tcutil_substitute_nofun(_T21,_T23);goto _TL6C;_TL6B: _TL6C:
# 378
 return;}case 4:{struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_T45;_T49=_T4C->f1;_T4B=_T4C->f2;}{struct Cyc_Absyn_Tvar*tv=_T49;struct Cyc_Absyn_Vardecl*vd=_T4B;_T24=vd;_T25=_T24->type;
# 382
if(_T25==0)goto _TL6D;_T26=p;_T27=_T26->loc;_T28=env;_T29=vd;_T2A=_T29->type;
Cyc_CurRgn_rgnsubtype(_T27,_T28,_T2A);_T2B=vd;_T2C=env;_T2D=_T2C.inst;_T2E=vd;_T2F=_T2E->type;
_T2B->type=Cyc_Tcutil_substitute_nofun(_T2D,_T2F);goto _TL6E;_TL6D: _TL6E: _T30=p;_T31=_T30->loc;_T32=tv;
# 386
Cyc_CurRgn_check_tvar(_T31,_T32);
return;}case 5:{struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_T45;_T4A=_T4C->f1;}{struct Cyc_Absyn_Pat*p1=_T4A;
# 389
Cyc_CurRgn_rgnsubpat(env,p1);return;}case 6:{struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_T45;_T48=_T4C->f3;_T47=_T4C->f4;}{struct Cyc_List_List*tvs=_T48;struct Cyc_List_List*ds=_T47;
# 392
_TL72: if(tvs!=0)goto _TL70;else{goto _TL71;}
_TL70: _T33=p;_T34=_T33->loc;_T35=tvs;_T36=_T35->hd;_T37=(struct Cyc_Absyn_Tvar*)_T36;Cyc_CurRgn_check_tvar(_T34,_T37);_T38=tvs;
# 392
tvs=_T38->tl;goto _TL72;_TL71:
# 394
 _TL76: if(ds!=0)goto _TL74;else{goto _TL75;}
_TL74: _T39=env;_T3A=ds;_T3B=_T3A->hd;_T3C=(struct _tuple13*)_T3B;_T3D=*_T3C;_T3E=_T3D.f1;Cyc_CurRgn_rgnsubpat(_T39,_T3E);_T3F=ds;
# 394
ds=_T3F->tl;goto _TL76;_TL75:
# 396
 return;}case 7:{struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_T45;_T48=_T4C->f3;}{struct Cyc_List_List*ps=_T48;_T48=ps;goto _LL12;}case 15:{struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_T45;_T48=_T4C->f2;}_LL12: {struct Cyc_List_List*ps=_T48;
# 401
_TL7A: if(ps!=0)goto _TL78;else{goto _TL79;}
_TL78: _T40=env;_T41=ps;_T42=_T41->hd;_T43=(struct Cyc_Absyn_Pat*)_T42;Cyc_CurRgn_rgnsubpat(_T40,_T43);_T44=ps;
# 401
ps=_T44->tl;goto _TL7A;_TL79:
# 403
 return;}case 8: goto _LL16;case 9: _LL16: goto _LL18;case 10: _LL18: goto _LL1A;case 11: _LL1A: goto _LL1C;case 12: _LL1C: goto _LL1E;case 13: _LL1E: goto _LL20;case 14: _LL20:
# 411
 return;default:{struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*_T4C=(struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_T45;_T46=_T4C->f1;}{struct Cyc_Absyn_Exp*e=_T46;
# 413
Cyc_CurRgn_rgnsubexp(env,e);return;}};}}
# 417
static void Cyc_CurRgn_rgnsubaggrdecl(unsigned loc,struct Cyc_CurRgn_rgnsubenv env,struct Cyc_Absyn_Aggrdecl*ad){unsigned _T0;struct Cyc_Absyn_Aggrdecl*_T1;struct Cyc_List_List*_T2;struct Cyc_Absyn_Aggrdecl*_T3;struct Cyc_Absyn_AggrdeclImpl*_T4;unsigned _T5;struct Cyc_Absyn_Aggrdecl*_T6;struct Cyc_Absyn_AggrdeclImpl*_T7;struct Cyc_List_List*_T8;struct Cyc_Absyn_Aggrdecl*_T9;struct Cyc_Absyn_AggrdeclImpl*_TA;struct Cyc_Absyn_AggrdeclImpl*_TB;struct Cyc_List_List*_TC;int*_TD;unsigned _TE;void*_TF;void*_T10;void*_T11;void*_T12;void*_T13;struct Cyc_List_List*_T14;struct Cyc_Absyn_Aggrdecl*_T15;struct Cyc_Absyn_AggrdeclImpl*_T16;unsigned _T17;struct Cyc_CurRgn_rgnsubenv _T18;struct Cyc_List_List*_T19;void*_T1A;struct Cyc_Absyn_Aggrfield*_T1B;void*_T1C;struct Cyc_CurRgn_rgnsubenv _T1D;struct Cyc_List_List*_T1E;void*_T1F;struct Cyc_Absyn_Aggrfield*_T20;struct Cyc_Absyn_Exp*_T21;struct Cyc_List_List*_T22;_T0=loc;_T1=ad;_T2=_T1->tvs;
Cyc_CurRgn_check_tvars(_T0,_T2);_T3=ad;_T4=_T3->impl;
if(_T4==0)goto _TL7B;_T5=loc;_T6=ad;_T7=_T6->impl;_T8=_T7->exist_vars;
Cyc_CurRgn_check_tvars(_T5,_T8);_T9=ad;_TA=_T9->impl;_TB=
_check_null(_TA);{struct Cyc_List_List*fc=_TB->effconstr;
_TL80: if(fc!=0)goto _TL7E;else{goto _TL7F;}
_TL7E: _TC=fc;{void*_T23=_TC->hd;void*_T24;void*_T25;_TD=(int*)_T23;_TE=*_TD;switch(_TE){case 0:{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T26=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T23;_TF=_T26->f1;_T25=(void*)_TF;}{void*e=_T25;
Cyc_CurRgn_rgnsubtype(loc,env,e);goto _LL0;}case 1:{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T26=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T23;_T10=_T26->f1;_T25=(void*)_T10;_T11=_T26->f2;_T24=(void*)_T11;}{void*e1=_T25;void*e2=_T24;_T25=e1;_T24=e2;goto _LL6;}default:{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T26=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T23;_T12=_T26->f1;_T25=(void*)_T12;_T13=_T26->f2;_T24=(void*)_T13;}_LL6: {void*e1=_T25;void*e2=_T24;
# 426
Cyc_CurRgn_rgnsubtype(loc,env,e1);Cyc_CurRgn_rgnsubtype(loc,env,e2);goto _LL0;}}_LL0:;}_T14=fc;
# 422
fc=_T14->tl;goto _TL80;_TL7F: _T15=ad;_T16=_T15->impl;{
# 429
struct Cyc_List_List*fs=_T16->fields;
_TL85: if(fs!=0)goto _TL83;else{goto _TL84;}
_TL83: _T17=loc;_T18=env;_T19=fs;_T1A=_T19->hd;_T1B=(struct Cyc_Absyn_Aggrfield*)_T1A;_T1C=_T1B->type;Cyc_CurRgn_rgnsubtype(_T17,_T18,_T1C);_T1D=env;_T1E=fs;_T1F=_T1E->hd;_T20=(struct Cyc_Absyn_Aggrfield*)_T1F;_T21=_T20->requires_clause;
Cyc_CurRgn_rgnsubexpopt(_T1D,_T21);_T22=fs;
# 430
fs=_T22->tl;goto _TL85;_TL84:;}}goto _TL7C;_TL7B: _TL7C:;}struct _tuple14{struct Cyc_Absyn_Tqual f0;void*f1;};
# 437
static void Cyc_CurRgn_rgnsubdtdecl(unsigned loc,struct Cyc_CurRgn_rgnsubenv env,struct Cyc_Absyn_Datatypedecl*tud){unsigned _T0;struct Cyc_Absyn_Datatypedecl*_T1;struct Cyc_List_List*_T2;struct Cyc_Absyn_Datatypedecl*_T3;struct Cyc_Core_Opt*_T4;struct Cyc_Absyn_Datatypedecl*_T5;struct Cyc_Core_Opt*_T6;void*_T7;struct Cyc_List_List*_T8;void*_T9;struct Cyc_Absyn_Datatypefield*_TA;struct Cyc_List_List*_TB;void*_TC;void**_TD;unsigned _TE;struct Cyc_CurRgn_rgnsubenv _TF;void**_T10;void*_T11;struct Cyc_List_List*_T12;struct Cyc_List_List*_T13;_T0=loc;_T1=tud;_T2=_T1->tvs;
Cyc_CurRgn_check_tvars(_T0,_T2);_T3=tud;_T4=_T3->fields;
if(_T4==0)goto _TL86;_T5=tud;_T6=_T5->fields;_T7=_T6->v;{
struct Cyc_List_List*fs=(struct Cyc_List_List*)_T7;_TL8B: if(fs!=0)goto _TL89;else{goto _TL8A;}
_TL89: _T8=fs;_T9=_T8->hd;_TA=(struct Cyc_Absyn_Datatypefield*)_T9;{struct Cyc_List_List*ts=_TA->typs;
_TL8C: if(ts!=0)goto _TL8D;else{goto _TL8E;}
_TL8D: _TB=ts;_TC=_TB->hd;{struct _tuple14*_T14=(struct _tuple14*)_TC;void*_T15;{struct _tuple14 _T16=*_T14;_TD=& _T14->f1;_T15=(void**)_TD;}{void**typ=(void**)_T15;_TE=loc;_TF=env;_T10=typ;_T11=*_T10;
Cyc_CurRgn_rgnsubtype(_TE,_TF,_T11);_T12=ts;
ts=_T12->tl;}}goto _TL8C;_TL8E:;}_T13=fs;
# 440
fs=_T13->tl;goto _TL8B;_TL8A:;}goto _TL87;_TL86: _TL87:;}
# 451
static void Cyc_CurRgn_rgnsubdecls(struct Cyc_CurRgn_rgnsubenv,struct Cyc_List_List*);
static void Cyc_CurRgn_rgnsubdecl(struct Cyc_CurRgn_rgnsubenv env,struct Cyc_Absyn_Decl*d){struct Cyc_Absyn_Decl*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Vardecl*_T3;struct Cyc_Absyn_Vardecl*_T4;void**_T5;struct Cyc_Absyn_Decl*_T6;unsigned _T7;struct Cyc_CurRgn_rgnsubenv _T8;void**_T9;void*_TA;struct Cyc_CurRgn_rgnsubenv _TB;int _TC;void**_TD;struct Cyc_CurRgn_rgnsubenv _TE;struct Cyc_List_List*_TF;void**_T10;void*_T11;struct Cyc_List_List*_T12;struct _tuple11*_T13;struct Cyc_CurRgn_rgnsubenv _T14;struct Cyc_Absyn_Fndecl*_T15;struct Cyc_Absyn_Stmt*_T16;struct Cyc_CurRgn_rgnsubenv _T17;int _T18;int(*_T19)(struct _fat_ptr,struct _fat_ptr,unsigned);void*(*_T1A)(struct _fat_ptr,struct _fat_ptr,unsigned);struct _fat_ptr _T1B;struct _fat_ptr _T1C;struct Cyc_Absyn_Decl*_T1D;unsigned _T1E;struct Cyc_Absyn_Typedefdecl*_T1F;struct Cyc_List_List*_T20;struct Cyc_Absyn_Typedefdecl*_T21;void*_T22;struct Cyc_Absyn_Decl*_T23;unsigned _T24;struct Cyc_CurRgn_rgnsubenv _T25;struct Cyc_Absyn_Typedefdecl*_T26;void*_T27;struct Cyc_CurRgn_rgnsubenv _T28;int _T29;int(*_T2A)(struct _fat_ptr,struct _fat_ptr,unsigned);void*(*_T2B)(struct _fat_ptr,struct _fat_ptr,unsigned);struct _fat_ptr _T2C;struct _fat_ptr _T2D;struct Cyc_Absyn_Decl*_T2E;unsigned _T2F;struct Cyc_CurRgn_rgnsubenv _T30;struct Cyc_Absyn_Aggrdecl*_T31;struct Cyc_CurRgn_rgnsubenv _T32;int _T33;int(*_T34)(struct _fat_ptr,struct _fat_ptr,unsigned);void*(*_T35)(struct _fat_ptr,struct _fat_ptr,unsigned);struct _fat_ptr _T36;struct _fat_ptr _T37;struct Cyc_Absyn_Decl*_T38;unsigned _T39;struct Cyc_CurRgn_rgnsubenv _T3A;struct Cyc_Absyn_Datatypedecl*_T3B;struct Cyc_CurRgn_rgnsubenv _T3C;int _T3D;int(*_T3E)(struct _fat_ptr,struct _fat_ptr,unsigned);void*(*_T3F)(struct _fat_ptr,struct _fat_ptr,unsigned);struct _fat_ptr _T40;struct _fat_ptr _T41;struct Cyc_Absyn_Decl*_T42;unsigned _T43;struct Cyc_CurRgn_rgnsubenv _T44;struct Cyc_List_List*_T45;void*_T46;struct Cyc_Absyn_Vardecl*_T47;void*_T48;struct Cyc_List_List*_T49;void*_T4A;struct Cyc_Absyn_Vardecl*_T4B;struct Cyc_CurRgn_rgnsubenv _T4C;struct Cyc_List_List*_T4D;struct Cyc_List_List*_T4E;void*_T4F;struct Cyc_Absyn_Vardecl*_T50;void*_T51;struct Cyc_List_List*_T52;int(*_T53)(struct _fat_ptr,struct _fat_ptr);void*(*_T54)(struct _fat_ptr,struct _fat_ptr);struct _fat_ptr _T55;struct _fat_ptr _T56;_T0=d;{
void*_T57=_T0->r;struct Cyc_Absyn_Exp*_T58;struct Cyc_Absyn_Pat*_T59;struct Cyc_List_List*_T5A;struct _fat_ptr*_T5B;struct Cyc_Absyn_Datatypedecl*_T5C;struct Cyc_Absyn_Aggrdecl*_T5D;struct Cyc_Absyn_Typedefdecl*_T5E;struct Cyc_Absyn_Fndecl*_T5F;struct Cyc_Absyn_Vardecl*_T60;_T1=(int*)_T57;_T2=*_T1;switch(_T2){case 0:{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T57;_T60=_T61->f1;}{struct Cyc_Absyn_Vardecl*vd=_T60;struct Cyc_Absyn_Exp*_T61;void*_T62;_T3=vd;{struct Cyc_Absyn_Vardecl _T63=*_T3;_T4=vd;_T5=& _T4->type;_T62=(void**)_T5;_T61=_T63.initializer;}{void**t=(void**)_T62;struct Cyc_Absyn_Exp*initializer=_T61;
# 456
Cyc_CurRgn_rgnsubexpopt(env,initializer);_T6=d;_T7=_T6->loc;_T8=env;_T9=t;_TA=*_T9;
Cyc_CurRgn_rgnsubtype(_T7,_T8,_TA);_TB=env;_TC=_TB.toplevel;
if(_TC)goto _TL90;else{goto _TL92;}
_TL92: _TD=t;_TE=env;_TF=_TE.inst;_T10=t;_T11=*_T10;*_TD=Cyc_Tcutil_substitute_nofun(_TF,_T11);goto _TL91;_TL90: _TL91:
 return;}}case 1:{struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_T57;_T5F=_T61->f1;}{struct Cyc_Absyn_Fndecl*fd=_T5F;
# 468
env.toplevel=0;{struct Cyc_List_List*_T61=_cycalloc(sizeof(struct Cyc_List_List));{struct _tuple11*_T62=_cycalloc(sizeof(struct _tuple11));
_T62->f0=_check_null(Cyc_CurRgn_curr_tvar);
_T62->f1=_check_null(Cyc_CurRgn_curr_rgn_typ);_T13=(struct _tuple11*)_T62;}
# 469
_T61->hd=_T13;
_T61->tl=0;_T12=(struct Cyc_List_List*)_T61;}
# 469
env.inst=_T12;_T14=env;_T15=fd;_T16=_T15->body;
# 471
Cyc_CurRgn_rgnsubstmt(_T14,_T16);
# 474
return;}case 8:{struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_T57;_T5E=_T61->f1;}{struct Cyc_Absyn_Typedefdecl*td=_T5E;_T17=env;_T18=_T17.toplevel;
# 477
if(!_T18)goto _TL93;goto _TL94;_TL93: _T1A=Cyc___assert_fail;{int(*_T61)(struct _fat_ptr,struct _fat_ptr,unsigned)=(int(*)(struct _fat_ptr,struct _fat_ptr,unsigned))_T1A;_T19=_T61;}_T1B=_tag_fat("env.toplevel",sizeof(char),13U);_T1C=_tag_fat("currgn.cyc",sizeof(char),11U);_T19(_T1B,_T1C,477U);_TL94: _T1D=d;_T1E=_T1D->loc;_T1F=td;_T20=_T1F->tvs;
Cyc_CurRgn_check_tvars(_T1E,_T20);_T21=td;_T22=_T21->defn;
if(_T22==0)goto _TL95;_T23=d;_T24=_T23->loc;_T25=env;_T26=td;_T27=_T26->defn;Cyc_CurRgn_rgnsubtype(_T24,_T25,_T27);goto _TL96;_TL95: _TL96:
 return;}case 5:{struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_T57;_T5D=_T61->f1;}{struct Cyc_Absyn_Aggrdecl*ad=_T5D;_T28=env;_T29=_T28.toplevel;
# 483
if(!_T29)goto _TL97;goto _TL98;_TL97: _T2B=Cyc___assert_fail;{int(*_T61)(struct _fat_ptr,struct _fat_ptr,unsigned)=(int(*)(struct _fat_ptr,struct _fat_ptr,unsigned))_T2B;_T2A=_T61;}_T2C=_tag_fat("env.toplevel",sizeof(char),13U);_T2D=_tag_fat("currgn.cyc",sizeof(char),11U);_T2A(_T2C,_T2D,483U);_TL98: _T2E=d;_T2F=_T2E->loc;_T30=env;_T31=ad;
Cyc_CurRgn_rgnsubaggrdecl(_T2F,_T30,_T31);
return;}case 6:{struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_T57;_T5C=_T61->f1;}{struct Cyc_Absyn_Datatypedecl*tud=_T5C;_T32=env;_T33=_T32.toplevel;
# 488
if(!_T33)goto _TL99;goto _TL9A;_TL99: _T35=Cyc___assert_fail;{int(*_T61)(struct _fat_ptr,struct _fat_ptr,unsigned)=(int(*)(struct _fat_ptr,struct _fat_ptr,unsigned))_T35;_T34=_T61;}_T36=_tag_fat("env.toplevel",sizeof(char),13U);_T37=_tag_fat("currgn.cyc",sizeof(char),11U);_T34(_T36,_T37,488U);_TL9A: _T38=d;_T39=_T38->loc;_T3A=env;_T3B=tud;
Cyc_CurRgn_rgnsubdtdecl(_T39,_T3A,_T3B);
return;}case 11: goto _LLE;case 12: _LLE: goto _LL10;case 7: _LL10: goto _LL12;case 13: _LL12: goto _LL14;case 14: _LL14: goto _LL16;case 15: _LL16: goto _LL18;case 16: _LL18:
# 500
 return;case 9:{struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_T57;_T5B=_T61->f1;_T5A=_T61->f2;}{struct _fat_ptr*v=_T5B;struct Cyc_List_List*ds2=_T5A;_T5A=ds2;goto _LL1C;}case 10:{struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_T57;_T5A=_T61->f2;}_LL1C: {struct Cyc_List_List*ds2=_T5A;
# 503
Cyc_CurRgn_rgnsubdecls(env,ds2);return;}case 2:{struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_T57;_T59=_T61->f1;_T58=_T61->f3;}{struct Cyc_Absyn_Pat*p=_T59;struct Cyc_Absyn_Exp*e=_T58;
# 506
Cyc_CurRgn_rgnsubpat(env,p);
Cyc_CurRgn_rgnsubexp(env,e);
return;}case 3:{struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*_T61=(struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_T57;_T5A=_T61->f1;}{struct Cyc_List_List*vds=_T5A;_T3C=env;_T3D=_T3C.toplevel;
# 511
if(_T3D)goto _TL9B;else{goto _TL9D;}_TL9D: goto _TL9C;_TL9B: _T3F=Cyc___assert_fail;{int(*_T61)(struct _fat_ptr,struct _fat_ptr,unsigned)=(int(*)(struct _fat_ptr,struct _fat_ptr,unsigned))_T3F;_T3E=_T61;}_T40=_tag_fat("!env.toplevel",sizeof(char),14U);_T41=_tag_fat("currgn.cyc",sizeof(char),11U);_T3E(_T40,_T41,511U);_TL9C:
 _TLA1: if(vds!=0)goto _TL9F;else{goto _TLA0;}
_TL9F: _T42=d;_T43=_T42->loc;_T44=env;_T45=vds;_T46=_T45->hd;_T47=(struct Cyc_Absyn_Vardecl*)_T46;_T48=_T47->type;Cyc_CurRgn_rgnsubtype(_T43,_T44,_T48);_T49=vds;_T4A=_T49->hd;_T4B=(struct Cyc_Absyn_Vardecl*)_T4A;_T4C=env;_T4D=_T4C.inst;_T4E=vds;_T4F=_T4E->hd;_T50=(struct Cyc_Absyn_Vardecl*)_T4F;_T51=_T50->type;
_T4B->type=Cyc_Tcutil_substitute_nofun(_T4D,_T51);_T52=vds;
# 512
vds=_T52->tl;goto _TLA1;_TLA0:
# 516
 return;}default: _T54=Cyc_Warn_impos;{
# 518
int(*_T61)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))_T54;_T53=_T61;}_T55=_tag_fat("internal compiler error: region or pool at toplevel",sizeof(char),52U);_T56=_tag_fat(0U,sizeof(void*),0);_T53(_T55,_T56);};}}
# 522
static void Cyc_CurRgn_rgnsubstmt(struct Cyc_CurRgn_rgnsubenv env,struct Cyc_Absyn_Stmt*s){struct Cyc_Absyn_Stmt*_T0;int*_T1;unsigned _T2;struct _tuple9 _T3;struct _tuple9 _T4;struct _tuple9 _T5;struct _tuple9 _T6;struct _tuple9 _T7;struct _tuple9 _T8;struct Cyc_CurRgn_rgnsubenv _T9;struct Cyc_List_List*_TA;void*_TB;struct Cyc_Absyn_Exp*_TC;struct Cyc_List_List*_TD;struct Cyc_List_List*_TE;void*_TF;struct Cyc_List_List*_T10;struct Cyc_List_List*_T11;void*_T12;struct Cyc_List_List*_T13;struct Cyc_Absyn_Decl*_T14;int*_T15;int _T16;struct Cyc_Absyn_Decl*_T17;unsigned _T18;struct Cyc_Absyn_Tvar*_T19;struct Cyc_CurRgn_rgnsubenv _T1A;struct Cyc_List_List*_T1B;struct Cyc_Absyn_Tvar*_T1C;void*_T1D;_T0=s;{
void*_T1E=_T0->r;struct Cyc_Absyn_Decl*_T1F;struct Cyc_List_List*_T20;struct _fat_ptr*_T21;struct Cyc_Absyn_Stmt*_T22;struct Cyc_Absyn_Exp*_T23;struct Cyc_Absyn_Exp*_T24;struct Cyc_Absyn_Stmt*_T25;struct Cyc_Absyn_Stmt*_T26;struct Cyc_Absyn_Exp*_T27;_T1=(int*)_T1E;_T2=*_T1;switch(_T2){case 0: goto _LL4;case 6: _LL4: goto _LL6;case 7: _LL6: goto _LL8;case 8: _LL8:
# 527
 return;case 1:{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T1E;_T27=_T28->f1;}{struct Cyc_Absyn_Exp*e=_T27;
# 529
Cyc_CurRgn_rgnsubexp(env,e);return;}case 2:{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T1E;_T26=_T28->f1;_T25=_T28->f2;}{struct Cyc_Absyn_Stmt*s1=_T26;struct Cyc_Absyn_Stmt*s2=_T25;
Cyc_CurRgn_rgnsubstmt(env,s1);Cyc_CurRgn_rgnsubstmt(env,s2);return;}case 3:{struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_T1E;_T27=_T28->f1;}{struct Cyc_Absyn_Exp*eopt=_T27;
Cyc_CurRgn_rgnsubexpopt(env,eopt);return;}case 4:{struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_T1E;_T27=_T28->f1;_T26=_T28->f2;_T25=_T28->f3;}{struct Cyc_Absyn_Exp*e=_T27;struct Cyc_Absyn_Stmt*s1=_T26;struct Cyc_Absyn_Stmt*s2=_T25;
# 534
Cyc_CurRgn_rgnsubexp(env,e);Cyc_CurRgn_rgnsubstmt(env,s1);Cyc_CurRgn_rgnsubstmt(env,s2);return;}case 14:{struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_T1E;_T26=_T28->f1;_T3=_T28->f2;_T27=_T3.f0;}{struct Cyc_Absyn_Stmt*s=_T26;struct Cyc_Absyn_Exp*e=_T27;_T27=e;_T26=s;goto _LL14;}case 5:{struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_T1E;_T4=_T28->f1;_T27=_T4.f0;_T26=_T28->f2;}_LL14: {struct Cyc_Absyn_Exp*e=_T27;struct Cyc_Absyn_Stmt*s1=_T26;
# 537
Cyc_CurRgn_rgnsubexp(env,e);Cyc_CurRgn_rgnsubstmt(env,s1);return;}case 9:{struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T1E;_T27=_T28->f1;_T5=_T28->f2;_T24=_T5.f0;_T6=_T28->f2;_T26=_T6.f1;_T7=_T28->f3;_T23=_T7.f0;_T8=_T28->f3;_T25=_T8.f1;_T22=_T28->f4;}{struct Cyc_Absyn_Exp*e1=_T27;struct Cyc_Absyn_Exp*e2=_T24;struct Cyc_Absyn_Stmt*guard_s=_T26;struct Cyc_Absyn_Exp*e3=_T23;struct Cyc_Absyn_Stmt*cont_s=_T25;struct Cyc_Absyn_Stmt*s1=_T22;
# 540
Cyc_CurRgn_rgnsubexp(env,e1);
Cyc_CurRgn_rgnsubexp(env,e2);
Cyc_CurRgn_rgnsubexp(env,e3);
Cyc_CurRgn_rgnsubstmt(env,s1);
return;}case 13:{struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_T1E;_T21=_T28->f1;_T26=_T28->f2;}{struct _fat_ptr*l=_T21;struct Cyc_Absyn_Stmt*s1=_T26;
# 546
Cyc_CurRgn_rgnsubstmt(env,s1);return;}case 11:{struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_T1E;_T20=_T28->f1;}{struct Cyc_List_List*es=_T20;
# 549
_TLA3: if(es!=0)goto _TLA4;else{goto _TLA5;}
_TLA4: _T9=env;_TA=es;_TB=_TA->hd;_TC=(struct Cyc_Absyn_Exp*)_TB;Cyc_CurRgn_rgnsubexp(_T9,_TC);_TD=es;
es=_TD->tl;goto _TLA3;_TLA5:
# 553
 return;}case 10:{struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_T1E;_T27=_T28->f1;_T20=_T28->f2;}{struct Cyc_Absyn_Exp*e=_T27;struct Cyc_List_List*scs=_T20;
# 556
Cyc_CurRgn_rgnsubexp(env,e);
_TLA6: if(scs!=0)goto _TLA7;else{goto _TLA8;}
_TLA7: _TE=scs;_TF=_TE->hd;{struct Cyc_Absyn_Switch_clause*_T28=(struct Cyc_Absyn_Switch_clause*)_TF;struct Cyc_Absyn_Stmt*_T29;struct Cyc_Absyn_Exp*_T2A;struct Cyc_Absyn_Pat*_T2B;{struct Cyc_Absyn_Switch_clause _T2C=*_T28;_T2B=_T2C.pattern;_T2A=_T2C.where_clause;_T29=_T2C.body;}{struct Cyc_Absyn_Pat*p=_T2B;struct Cyc_Absyn_Exp*eopt=_T2A;struct Cyc_Absyn_Stmt*s1=_T29;
Cyc_CurRgn_rgnsubpat(env,p);
Cyc_CurRgn_rgnsubexpopt(env,eopt);
Cyc_CurRgn_rgnsubstmt(env,s1);_T10=scs;
scs=_T10->tl;}}goto _TLA6;_TLA8:
# 564
 return;}case 15:{struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_T1E;_T26=_T28->f1;_T20=_T28->f2;}{struct Cyc_Absyn_Stmt*s1=_T26;struct Cyc_List_List*scs=_T20;
# 567
_TLA9: if(scs!=0)goto _TLAA;else{goto _TLAB;}
_TLAA: _T11=scs;_T12=_T11->hd;{struct Cyc_Absyn_Switch_clause*_T28=(struct Cyc_Absyn_Switch_clause*)_T12;struct Cyc_Absyn_Stmt*_T29;struct Cyc_Absyn_Exp*_T2A;struct Cyc_Absyn_Pat*_T2B;{struct Cyc_Absyn_Switch_clause _T2C=*_T28;_T2B=_T2C.pattern;_T2A=_T2C.where_clause;_T29=_T2C.body;}{struct Cyc_Absyn_Pat*p=_T2B;struct Cyc_Absyn_Exp*eopt=_T2A;struct Cyc_Absyn_Stmt*s1=_T29;
Cyc_CurRgn_rgnsubpat(env,p);
Cyc_CurRgn_rgnsubexpopt(env,eopt);
Cyc_CurRgn_rgnsubstmt(env,s1);_T13=scs;
scs=_T13->tl;}}goto _TLA9;_TLAB:
# 574
 return;}default:{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T28=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T1E;_T1F=_T28->f1;_T26=_T28->f2;}{struct Cyc_Absyn_Decl*d=_T1F;struct Cyc_Absyn_Stmt*s1=_T26;_T14=d;{
# 577
void*_T28=_T14->r;struct Cyc_Absyn_Exp*_T29;struct Cyc_Absyn_Vardecl*_T2A;struct Cyc_Absyn_Tvar*_T2B;_T15=(int*)_T28;_T16=*_T15;if(_T16!=4)goto _TLAC;{struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*_T2C=(struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_T28;_T2B=_T2C->f1;_T2A=_T2C->f2;_T29=_T2C->f3;}{struct Cyc_Absyn_Tvar*tv=_T2B;struct Cyc_Absyn_Vardecl*vd=_T2A;struct Cyc_Absyn_Exp*open_exp_opt=_T29;_T17=d;_T18=_T17->loc;_T19=tv;
# 579
Cyc_CurRgn_check_tvar(_T18,_T19);
Cyc_CurRgn_rgnsubexpopt(env,open_exp_opt);
# 584
if(open_exp_opt!=0)goto _TLAE;_T1A=env;_T1B=_T1A.inst;_T1C=
_check_null(Cyc_CurRgn_curr_tvar);_T1D=Cyc_Absyn_var_type(tv);Cyc_CurRgn_subst_inst(_T1B,_T1C,_T1D);
Cyc_CurRgn_rgnsubstmt(env,s1);goto _TLAF;
# 589
_TLAE: Cyc_CurRgn_rgnsubstmt(env,s1);_TLAF:
 return;}_TLAC:
# 593
 Cyc_CurRgn_rgnsubdecl(env,d);
Cyc_CurRgn_rgnsubstmt(env,s1);
return;;}}};}}
# 600
static void Cyc_CurRgn_rgnsubdecls(struct Cyc_CurRgn_rgnsubenv env,struct Cyc_List_List*ds){struct Cyc_CurRgn_rgnsubenv _T0;struct Cyc_List_List*_T1;void*_T2;struct Cyc_Absyn_Decl*_T3;struct Cyc_List_List*_T4;
_TLB3: if(ds!=0)goto _TLB1;else{goto _TLB2;}
_TLB1: _T0=env;_T1=ds;_T2=_T1->hd;_T3=(struct Cyc_Absyn_Decl*)_T2;Cyc_CurRgn_rgnsubdecl(_T0,_T3);_T4=ds;
# 601
ds=_T4->tl;goto _TLB3;_TLB2:;}
# 605
void Cyc_CurRgn_adjust_all(struct Cyc_List_List*tds){struct Cyc_CurRgn_rgnsubenv _T0;
Cyc_CurRgn_curr_rgn_type();{struct Cyc_CurRgn_rgnsubenv _T1;
_T1.inst=0;_T1.in_externC=0;_T1.toplevel=1;_T0=_T1;}{struct Cyc_CurRgn_rgnsubenv env=_T0;
Cyc_CurRgn_rgnsubdecls(env,tds);}}

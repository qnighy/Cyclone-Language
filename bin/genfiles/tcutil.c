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
# 121 "core.h"
int Cyc_Core_intcmp(int,int);extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};
# 173
extern struct _RegionHandle*Cyc_Core_heap_region;struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
extern struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
extern int Cyc_List_length(struct Cyc_List_List*);
# 76
extern struct Cyc_List_List*Cyc_List_map(void*(*)(void*),struct Cyc_List_List*);
# 83
extern struct Cyc_List_List*Cyc_List_map_c(void*(*)(void*,void*),void*,struct Cyc_List_List*);
# 86
extern struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*)(void*,void*),void*,struct Cyc_List_List*);
# 94
extern struct Cyc_List_List*Cyc_List_map2(void*(*)(void*,void*),struct Cyc_List_List*,struct Cyc_List_List*);
# 133
extern void Cyc_List_iter(void(*)(void*),struct Cyc_List_List*);
# 161
extern struct Cyc_List_List*Cyc_List_revappend(struct Cyc_List_List*,struct Cyc_List_List*);
# 178
extern struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*);
# 220
extern struct Cyc_List_List*Cyc_List_rimp_merge_sort(int(*)(void*,void*),struct Cyc_List_List*);
# 254
extern int Cyc_List_forall_c(int(*)(void*,void*),void*,struct Cyc_List_List*);
# 258
extern int Cyc_List_exists(int(*)(void*),struct Cyc_List_List*);
# 261
extern int Cyc_List_exists_c(int(*)(void*,void*),void*,struct Cyc_List_List*);
# 270
extern struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*,struct Cyc_List_List*);
# 276
extern struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*,struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);struct _tuple0{struct Cyc_List_List*f0;struct Cyc_List_List*f1;};
# 303
extern struct _tuple0 Cyc_List_rsplit(struct _RegionHandle*,struct _RegionHandle*,struct Cyc_List_List*);
# 322
extern int Cyc_List_mem(int(*)(void*,void*),struct Cyc_List_List*,void*);
# 336
extern void*Cyc_List_assoc_cmp(int(*)(void*,void*),struct Cyc_List_List*,void*);
# 383
extern int Cyc_List_list_cmp(int(*)(void*,void*),struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};
# 73
extern struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
extern int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr);
# 49 "string.h"
extern int Cyc_strcmp(struct _fat_ptr,struct _fat_ptr);
extern int Cyc_strptrcmp(struct _fat_ptr*,struct _fat_ptr*);struct Cyc_AssnDef_ExistAssnFn;struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f0;struct _fat_ptr*f1;};
# 140 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 161
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 165
enum Cyc_Absyn_AliasQualVal{Cyc_Absyn_Aliasable_qual =0U,Cyc_Absyn_Unique_qual =1U,Cyc_Absyn_Refcnt_qual =2U,Cyc_Absyn_Restricted_qual =3U};
# 181 "absyn.h"
enum Cyc_Absyn_AliasHint{Cyc_Absyn_UniqueHint =0U,Cyc_Absyn_RefcntHint =1U,Cyc_Absyn_RestrictedHint =2U,Cyc_Absyn_NoHint =3U};
# 187
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_EffKind =3U,Cyc_Absyn_IntKind =4U,Cyc_Absyn_BoolKind =5U,Cyc_Absyn_PtrBndKind =6U,Cyc_Absyn_AqualKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasHint aliashint;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;void*aquals_bound;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*eff;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;void*autoreleased;void*aqual;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*checks_clause;struct Cyc_AssnDef_ExistAssnFn*checks_assn;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_AssnDef_ExistAssnFn*requires_assn;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_AssnDef_ExistAssnFn*ensures_assn;struct Cyc_Absyn_Exp*throws_clause;struct Cyc_AssnDef_ExistAssnFn*throws_assn;struct Cyc_Absyn_Vardecl*return_value;struct Cyc_List_List*arg_vardecls;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 312
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f0;struct Cyc_Absyn_Datatypefield*f1;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple3{enum Cyc_Absyn_AggrKind f0;struct _tuple1*f1;struct Cyc_Core_Opt*f2;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 332
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple1*,struct Cyc_Core_Opt*);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_AliasQualVal f1;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct{int tag;void*f1;};struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct{int tag;void*f1;void*f2;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_Cvar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;void*f4;const char*f5;const char*f6;int f7;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_SubsetType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_AssnDef_ExistAssnFn*f3;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f0;char f1;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f0;short f1;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f0;int f1;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f0;long long f1;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f0;int f1;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 535 "absyn.h"
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U,Cyc_Absyn_Tagof =19U,Cyc_Absyn_UDiv =20U,Cyc_Absyn_UMod =21U,Cyc_Absyn_UGt =22U,Cyc_Absyn_ULt =23U,Cyc_Absyn_UGte =24U,Cyc_Absyn_ULte =25U};
# 542
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};
# 560
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Subset_coercion =3U,Cyc_Absyn_Other_coercion =4U};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};
# 575
enum Cyc_Absyn_MallocKind{Cyc_Absyn_Malloc =0U,Cyc_Absyn_Calloc =1U,Cyc_Absyn_Vmalloc =2U};struct Cyc_Absyn_MallocInfo{enum Cyc_Absyn_MallocKind mknd;struct Cyc_Absyn_Exp*rgn;struct Cyc_Absyn_Exp*aqual;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple9{struct _fat_ptr*f0;struct Cyc_Absyn_Tqual f1;void*f2;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;int f2;struct Cyc_List_List*f3;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;int f3;};struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f0;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;struct Cyc_Absyn_Exp*rename;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;int escapes;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*qual_bnd;struct Cyc_List_List*fields;int tagged;struct Cyc_List_List*effconstr;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};
# 932 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple1*,struct _tuple1*);
# 935
int Cyc_Absyn_tvar_cmp(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);
# 940
union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n (void);
# 947
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 953
void*Cyc_Absyn_compress(void*);
# 957
int Cyc_Absyn_type2bool(int,void*);
# 966
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*,struct Cyc_Core_Opt*);
# 971
extern void*Cyc_Absyn_uint_type;extern void*Cyc_Absyn_ulong_type;extern void*Cyc_Absyn_ulonglong_type;
# 973
extern void*Cyc_Absyn_sint_type;extern void*Cyc_Absyn_slong_type;extern void*Cyc_Absyn_slonglong_type;
# 980
extern void*Cyc_Absyn_heap_rgn_type;
# 982
extern void*Cyc_Absyn_al_qual_type;
# 984
extern void*Cyc_Absyn_empty_effect;
# 986
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 988
extern void*Cyc_Absyn_void_type;extern void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);extern void*Cyc_Absyn_join_eff(struct Cyc_List_List*);extern void*Cyc_Absyn_regionsof_eff(void*);extern void*Cyc_Absyn_enum_type(struct _tuple1*,struct Cyc_Absyn_Enumdecl*);
# 1019
extern void*Cyc_Absyn_fat_bound_type;
# 1021
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
# 1023
void*Cyc_Absyn_bounds_one (void);
void*Cyc_Absyn_cvar_type(struct Cyc_Core_Opt*);
# 1026
void*Cyc_Absyn_fatconst (void);
void*Cyc_Absyn_thinconst (void);
# 1029
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo);
# 1033
void*Cyc_Absyn_atb_type(void*,void*,void*,struct Cyc_Absyn_Tqual,void*,void*,void*);
# 1052
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*);
# 1054
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*);
# 1056
void*Cyc_Absyn_aqualsof_type(void*);
# 1069
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(unsigned);
# 1079
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
# 1086
struct Cyc_Absyn_Exp*Cyc_Absyn_varb_exp(void*,unsigned);
# 1089
struct Cyc_Absyn_Exp*Cyc_Absyn_pragma_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned);
# 1093
struct Cyc_Absyn_Exp*Cyc_Absyn_swap_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1108
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned);
# 1110
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1117
struct Cyc_Absyn_Exp*Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_rethrow_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int,enum Cyc_Absyn_Coercion,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_alignoftype_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_alignofexp_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1131
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1133
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1135
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_asm_exp(int,struct _fat_ptr,struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_List_List*,unsigned);
# 1139
struct Cyc_Absyn_Exp*Cyc_Absyn_extension_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assert_exp(struct Cyc_Absyn_Exp*,int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assert_false_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1154
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1158
struct Cyc_Absyn_Stmt*Cyc_Absyn_return_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_ifthenelse_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_while_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_break_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_continue_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_for_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_switch_stmt(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_fallthru_stmt(struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
# 1168
struct Cyc_Absyn_Stmt*Cyc_Absyn_label_stmt(struct _fat_ptr*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_do_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_goto_stmt(struct _fat_ptr*,unsigned);
# 1172
struct Cyc_Absyn_Stmt*Cyc_Absyn_trycatch_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_List_List*,unsigned);
# 1183
struct Cyc_Absyn_Decl*Cyc_Absyn_alias_decl(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned,struct _tuple1*,void*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);
# 1231
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_field(struct Cyc_List_List*,struct _fat_ptr*);
# 1233
struct Cyc_Absyn_Aggrfield*Cyc_Absyn_lookup_decl_field(struct Cyc_Absyn_Aggrdecl*,struct _fat_ptr*);
# 1245
struct Cyc_Absyn_Aggrdecl*Cyc_Absyn_get_known_aggrdecl(union Cyc_Absyn_AggrInfo);
# 1258
void Cyc_Absyn_visit_exp(int(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Exp*);
# 63 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_kindbound2string(void*);
# 77
struct _fat_ptr Cyc_Absynpp_tvar2string(struct Cyc_Absyn_Tvar*);
# 29 "kinds.h"
extern struct Cyc_Absyn_Kind Cyc_Kinds_ak;
extern struct Cyc_Absyn_Kind Cyc_Kinds_bk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_mk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ek;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ik;
extern struct Cyc_Absyn_Kind Cyc_Kinds_boolk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ptrbk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_aqk;
# 57 "kinds.h"
extern struct Cyc_Core_Opt Cyc_Kinds_eko;
extern struct Cyc_Core_Opt Cyc_Kinds_boolko;
extern struct Cyc_Core_Opt Cyc_Kinds_ptrbko;
# 76 "kinds.h"
struct Cyc_Core_Opt*Cyc_Kinds_kind_to_opt(struct Cyc_Absyn_Kind*);
# 78
void*Cyc_Kinds_copy_kindbound(void*);
# 83
struct Cyc_Absyn_Kind*Cyc_Kinds_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*);
struct Cyc_Core_Opt Cyc_Kinds_tvar_kind_opt(struct Cyc_Absyn_Tvar*);
# 48 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Int_Warn_Warg_struct{int tag;int f1;};
# 75
void Cyc_Warn_err2(unsigned,struct _fat_ptr);
# 77
void Cyc_Warn_warn2(unsigned,struct _fat_ptr);
# 79
void*Cyc_Warn_impos2(struct _fat_ptr);
# 59 "flags.h"
extern int Cyc_Flags_warn_alias_coerce;struct _tuple12{unsigned f0;int f1;};
# 28 "evexp.h"
extern struct _tuple12 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*);
# 50 "evexp.h"
extern int Cyc_Evexp_uint_exp_cmp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);
# 35
void Cyc_Unify_occurs(void*,struct _RegionHandle*,struct Cyc_List_List*,void*);
# 70 "tcutil.h"
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp_constrain(void*,void*,int);
# 82
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*,void*);
# 92
void*Cyc_Tcutil_copy_type(void*);
# 95
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int,struct Cyc_Absyn_Exp*);
# 99
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void Cyc_Tcutil_unchecked_cast(struct Cyc_Absyn_Exp*,void*,enum Cyc_Absyn_Coercion);
# 102
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Absyn_Exp*);
# 106
int Cyc_Tcutil_coerce_assign(struct Cyc_List_List*,struct Cyc_Absyn_Exp*,void*);
# 128 "tcutil.h"
int Cyc_Tcutil_typecmp(void*,void*);
# 131
void*Cyc_Tcutil_substitute(struct Cyc_List_List*,void*);
# 134
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);
# 195
int Cyc_Tcutil_is_noalias_pointer(struct Cyc_List_List*,void*,int);
# 200
int Cyc_Tcutil_is_noalias_path(struct Cyc_List_List*,struct Cyc_Absyn_Exp*);struct _tuple13{int f0;void*f1;};
# 212
void*Cyc_Tcutil_normalize_effect(void*);
# 215
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 226
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 246
void*Cyc_Tcutil_promote_array(void*,void*,void*,int);
# 253
int Cyc_Tcutil_force_type2bool(int,void*);
# 287
void Cyc_Tcutil_substitute_vars(struct Cyc_List_List*,struct Cyc_Absyn_Exp*);
# 55 "attributes.h"
int Cyc_Atts_same_atts(struct Cyc_List_List*,struct Cyc_List_List*);
# 69
int Cyc_Atts_fntype_att(void*);
# 80
int Cyc_Atts_attribute_cmp(void*,void*);struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};struct Cyc_Set_Set;
# 151 "assndef.h"
extern struct Cyc_Absyn_Vardecl*Cyc_AssnDef_memory;
extern struct Cyc_Absyn_Vardecl*Cyc_AssnDef_pre_memory;struct Cyc_AssnDef_AssnFn{struct Cyc_List_List*actuals;void*assn;};struct Cyc_AssnDef_ExistAssnFn{struct Cyc_AssnDef_AssnFn*af;struct Cyc_Set_Set*existvars;};struct Cyc_AssnDef_AssnMap{void*assn;struct Cyc_Dict_Dict map;};
# 277 "assndef.h"
extern struct Cyc_AssnDef_ExistAssnFn*Cyc_AssnDef_assnmap2existassnfn(struct Cyc_AssnDef_AssnMap,struct Cyc_List_List*);
# 339 "assndef.h"
extern struct Cyc_AssnDef_AssnMap*Cyc_AssnDef_subst_vardecls(struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_AssnDef_AssnMap*);
# 45 "vcgen.h"
extern struct Cyc_AssnDef_AssnMap Cyc_Vcgen_clause2assn(struct Cyc_Absyn_Exp*);
# 17 "bansheeif.h"
void*Cyc_BansheeIf_equality_constraint(void*,void*);
# 28
int Cyc_BansheeIf_add_constraint(unsigned,void*);
# 10 "subtype.h"
enum Cyc_Absyn_Coercion Cyc_Subtype_coercible_warn(unsigned,void*,void*);
# 44 "tcutil.cyc"
int Cyc_Tcutil_is_void_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;int _T5;
void*_T6=Cyc_Absyn_compress(t);_T0=(int*)_T6;_T1=*_T0;if(_T1!=0)goto _TL0;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;if(_T5!=0)goto _TL2;
return 1;_TL2: goto _LL3;_TL0: _LL3:
 return 0;;}
# 50
int Cyc_Tcutil_is_array_type(void*t){int*_T0;int _T1;
void*_T2=Cyc_Absyn_compress(t);_T0=(int*)_T2;_T1=*_T0;if(_T1!=5)goto _TL4;
return 1;_TL4:
 return 0;;}
# 56
int Cyc_Tcutil_is_heap_rgn_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;int _T5;
void*_T6=Cyc_Absyn_compress(t);_T0=(int*)_T6;_T1=*_T0;if(_T1!=0)goto _TL6;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;if(_T5!=6)goto _TL8;
return 1;_TL8: goto _LL3;_TL6: _LL3:
 return 0;;}
# 62
int Cyc_Tcutil_is_pointer_type(void*t){int*_T0;int _T1;
void*_T2=Cyc_Absyn_compress(t);_T0=(int*)_T2;_T1=*_T0;if(_T1!=4)goto _TLA;
return 1;_TLA:
 return 0;;}
# 69
int Cyc_Tcutil_is_char_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;int _T5;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T6;void*_T7;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T8;enum Cyc_Absyn_Size_of _T9;
void*_TA=Cyc_Absyn_compress(t);_T0=(int*)_TA;_T1=*_T0;if(_T1!=0)goto _TLC;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;if(_T5!=1)goto _TLE;_T6=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_T7=_T6->f1;_T8=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T7;_T9=_T8->f2;if(_T9!=Cyc_Absyn_Char_sz)goto _TL10;
return 1;_TL10: goto _LL3;_TLE: goto _LL3;_TLC: _LL3:
 return 0;;}
# 76
int Cyc_Tcutil_is_any_int_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;int _T5;
void*_T6=Cyc_Absyn_compress(t);_T0=(int*)_T6;_T1=*_T0;if(_T1!=0)goto _TL12;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;if(_T5!=1)goto _TL14;
return 1;_TL14: goto _LL3;_TL12: _LL3:
 return 0;;}
# 83
int Cyc_Tcutil_is_any_float_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;int _T5;
void*_T6=Cyc_Absyn_compress(t);_T0=(int*)_T6;_T1=*_T0;if(_T1!=0)goto _TL16;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;if(_T5!=2)goto _TL18;
return 1;_TL18: goto _LL3;_TL16: _LL3:
 return 0;;}
# 90
int Cyc_Tcutil_is_integral_type(void*t){int*_T0;int _T1;void*_T2;void*_T3;int*_T4;unsigned _T5;
void*_T6=Cyc_Absyn_compress(t);void*_T7;_T0=(int*)_T6;_T1=*_T0;if(_T1!=0)goto _TL1A;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T8=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6;_T2=_T8->f1;_T7=(void*)_T2;}{void*c=_T7;_T3=c;_T4=(int*)_T3;_T5=*_T4;switch(_T5){case 1: goto _LL9;case 5: _LL9: goto _LLB;case 19: _LLB: goto _LLD;case 20: _LLD:
# 97
 return 1;default:
 return 0;};}goto _TL1B;_TL1A:
# 100
 return 0;_TL1B:;}
# 103
int Cyc_Tcutil_is_signed_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;unsigned _T5;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T6;void*_T7;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T8;enum Cyc_Absyn_Sign _T9;
void*_TA=Cyc_Absyn_compress(t);_T0=(int*)_TA;_T1=*_T0;if(_T1!=0)goto _TL1D;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;switch(_T5){case 1: _T6=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_T7=_T6->f1;_T8=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T7;_T9=_T8->f1;if(_T9!=Cyc_Absyn_Signed)goto _TL20;
return 1;_TL20: goto _LL5;case 2:
 return 1;default: goto _LL5;}goto _TL1E;_TL1D: _LL5:
 return 0;_TL1E:;}
# 110
int Cyc_Tcutil_is_arithmetic_type(void*t){int _T0;int _T1;_T1=
Cyc_Tcutil_is_integral_type(t);if(!_T1)goto _TL22;_T0=1;goto _TL23;_TL22: _T0=Cyc_Tcutil_is_any_float_type(t);_TL23: return _T0;}
# 113
int Cyc_Tcutil_is_strict_arithmetic_type(void*t){int _T0;int _T1;_T1=
Cyc_Tcutil_is_any_int_type(t);if(!_T1)goto _TL24;_T0=1;goto _TL25;_TL24: _T0=Cyc_Tcutil_is_any_float_type(t);_TL25: return _T0;}
# 116
int Cyc_Tcutil_is_tag_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;int _T5;
void*_T6=Cyc_Absyn_compress(t);_T0=(int*)_T6;_T1=*_T0;if(_T1!=0)goto _TL26;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;if(_T5!=5)goto _TL28;
return 1;_TL28: goto _LL3;_TL26: _LL3:
 return 0;;}
# 122
int Cyc_Tcutil_is_function_type(void*t){int*_T0;int _T1;
void*_T2=Cyc_Absyn_compress(t);_T0=(int*)_T2;_T1=*_T0;if(_T1!=6)goto _TL2A;
return 1;_TL2A:
 return 0;;}
# 128
int Cyc_Tcutil_is_typedef_type(void*t){int*_T0;int _T1;
void*_T2=Cyc_Absyn_compress(t);_T0=(int*)_T2;_T1=*_T0;if(_T1!=8)goto _TL2C;
return 1;_TL2C:
 return 0;;}
# 134
int Cyc_Tcutil_is_boxed(void*t){struct Cyc_Absyn_Kind*_T0;enum Cyc_Absyn_KindQual _T1;int _T2;int _T3;_T0=
Cyc_Tcutil_type_kind(t);_T1=_T0->kind;_T2=(int)_T1;_T3=_T2==2;return _T3;}
# 142
int Cyc_Tcutil_is_integral(struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;void*_T1;void*_T2;int*_T3;int _T4;struct Cyc_Absyn_Exp*_T5;void*_T6;void*_T7;void*_T8;int _T9;int _TA;_T0=e;_T1=_T0->topt;_T2=
_check_null(_T1);{void*_TB=Cyc_Absyn_compress(_T2);void*_TC;_T3=(int*)_TB;_T4=*_T3;if(_T4!=1)goto _TL2E;_T5=e;_T6=_T5->topt;_T7=
_check_null(_T6);_T8=Cyc_Absyn_sint_type;_T9=Cyc_Unify_unify(_T7,_T8);return _T9;_TL2E: _TC=_TB;{void*t=_TC;_TA=
Cyc_Tcutil_is_integral_type(t);return _TA;};}}
# 150
int Cyc_Tcutil_is_numeric(struct Cyc_Absyn_Exp*e){int _T0;struct Cyc_Absyn_Exp*_T1;void*_T2;void*_T3;int*_T4;int _T5;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T6;void*_T7;int*_T8;int _T9;_T0=
Cyc_Tcutil_is_integral(e);if(!_T0)goto _TL30;
return 1;_TL30: _T1=e;_T2=_T1->topt;_T3=
_check_null(_T2);{void*_TA=Cyc_Absyn_compress(_T3);_T4=(int*)_TA;_T5=*_T4;if(_T5!=0)goto _TL32;_T6=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_T7=_T6->f1;_T8=(int*)_T7;_T9=*_T8;if(_T9!=2)goto _TL34;
return 1;_TL34: goto _LL3;_TL32: _LL3:
 return 0;;}}
# 160
int Cyc_Tcutil_is_zeroterm_pointer_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrAtts _T3;int _T4;
void*_T5=Cyc_Absyn_compress(t);void*_T6;_T0=(int*)_T5;_T1=*_T0;if(_T1!=4)goto _TL36;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T7=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T5;_T2=_T7->f1;_T3=_T2.ptr_atts;_T6=_T3.zero_term;}{void*ztl=_T6;_T4=
# 163
Cyc_Tcutil_force_type2bool(0,ztl);return _T4;}_TL36:
 return 0;;}
# 169
int Cyc_Tcutil_is_nullable_pointer_type(void*t,int def){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrAtts _T3;int _T4;
void*_T5=Cyc_Absyn_compress(t);void*_T6;_T0=(int*)_T5;_T1=*_T0;if(_T1!=4)goto _TL38;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T7=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T5;_T2=_T7->f1;_T3=_T2.ptr_atts;_T6=_T3.nullable;}{void*nbl=_T6;_T4=
# 172
Cyc_Tcutil_force_type2bool(def,nbl);return _T4;}_TL38:
 return 0;;}
# 177
int Cyc_Tcutil_is_cvar_type(void*cv){int*_T0;int _T1;
void*_T2=Cyc_Absyn_compress(cv);_T0=(int*)_T2;_T1=*_T0;if(_T1!=3)goto _TL3A;
return 1;_TL3A:
 return 0;;}
# 184
void*Cyc_Tcutil_ptrbnd_cvar_equivalent(void*t){void*_T0;int*_T1;unsigned _T2;void*_T3;void*_T4;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T5;void*_T6;int*_T7;int _T8;void*_T9;void*_TA;void*_TB;void*_TC;
t=Cyc_Absyn_compress(t);_T0=t;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 3: _T3=t;
# 187
return _T3;case 0: _T4=t;_T5=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T4;_T6=_T5->f1;_T7=(int*)_T6;_T8=*_T7;if(_T8!=14)goto _TL3D;_T9=
Cyc_Absyn_fatconst();return _T9;_TL3D: goto _LL5;default: _LL5: _TB=t;_TC=
Cyc_Absyn_bounds_one();if(_TB!=_TC)goto _TL3F;_TA=Cyc_Absyn_thinconst();goto _TL40;_TL3F: _TA=0;_TL40: return _TA;};}
# 194
int Cyc_Tcutil_is_fat_ptr(void*t){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrAtts _T3;int _T4;
void*_T5=Cyc_Absyn_compress(t);void*_T6;_T0=(int*)_T5;_T1=*_T0;if(_T1!=4)goto _TL41;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T7=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T5;_T2=_T7->f1;_T3=_T2.ptr_atts;_T6=_T3.bounds;}{void*b=_T6;_T4=
# 197
Cyc_Unify_unify(Cyc_Absyn_fat_bound_type,b);return _T4;}_TL41:
 return 0;;}
# 204
int Cyc_Tcutil_is_fat_pointer_type_elt(void*t,void**elt_type_dest){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrInfo _T3;struct Cyc_Absyn_PtrAtts _T4;int _T5;void**_T6;
void*_T7=Cyc_Absyn_compress(t);void*_T8;void*_T9;_T0=(int*)_T7;_T1=*_T0;if(_T1!=4)goto _TL43;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TA=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T7;_T2=_TA->f1;_T9=_T2.elt_type;_T3=_TA->f1;_T4=_T3.ptr_atts;_T8=_T4.bounds;}{void*elt_type=_T9;void*b=_T8;_T5=
# 207
Cyc_Unify_unify(b,Cyc_Absyn_fat_bound_type);if(_T5)goto _TL45;else{goto _TL47;}
_TL47: return 0;_TL45: _T6=elt_type_dest;
*_T6=elt_type;
return 1;}_TL43:
 return 0;;}
# 217
int Cyc_Tcutil_is_zero_pointer_type_elt(void*t,void**elt_type_dest){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrInfo _T3;struct Cyc_Absyn_PtrAtts _T4;void**_T5;int _T6;
void*_T7=Cyc_Absyn_compress(t);void*_T8;void*_T9;_T0=(int*)_T7;_T1=*_T0;if(_T1!=4)goto _TL48;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TA=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T7;_T2=_TA->f1;_T9=_T2.elt_type;_T3=_TA->f1;_T4=_T3.ptr_atts;_T8=_T4.zero_term;}{void*elt_type=_T9;void*zt=_T8;_T5=elt_type_dest;
# 220
*_T5=elt_type;_T6=
Cyc_Absyn_type2bool(0,zt);return _T6;}_TL48:
 return 0;;}
# 229
int Cyc_Tcutil_is_zero_ptr_type(void*t,void**ptr_type,int*is_fat,void**elt_type){int*_T0;unsigned _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrInfo _T3;struct Cyc_Absyn_PtrAtts _T4;struct Cyc_Absyn_PtrInfo _T5;struct Cyc_Absyn_PtrAtts _T6;int _T7;void**_T8;void**_T9;int*_TA;int _TB;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TC;void*_TD;int*_TE;int _TF;int*_T10;int*_T11;struct Cyc_Absyn_ArrayInfo _T12;struct Cyc_Absyn_ArrayInfo _T13;struct Cyc_Absyn_ArrayInfo _T14;struct Cyc_Absyn_ArrayInfo _T15;int _T16;void**_T17;int*_T18;void**_T19;
# 231
void*_T1A=Cyc_Absyn_compress(t);struct Cyc_Absyn_Exp*_T1B;struct Cyc_Absyn_Tqual _T1C;void*_T1D;void*_T1E;void*_T1F;_T0=(int*)_T1A;_T1=*_T0;switch(_T1){case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T20=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T1A;_T2=_T20->f1;_T1F=_T2.elt_type;_T3=_T20->f1;_T4=_T3.ptr_atts;_T1E=_T4.bounds;_T5=_T20->f1;_T6=_T5.ptr_atts;_T1D=_T6.zero_term;}{void*elt=_T1F;void*bnds=_T1E;void*zt=_T1D;_T7=
# 233
Cyc_Absyn_type2bool(0,zt);if(_T7)goto _TL4B;else{goto _TL4D;}
_TL4D: return 0;_TL4B: _T8=ptr_type;
*_T8=t;_T9=elt_type;
*_T9=elt;{
void*_T20=Cyc_Absyn_compress(bnds);_TA=(int*)_T20;_TB=*_TA;if(_TB!=0)goto _TL4E;_TC=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T20;_TD=_TC->f1;_TE=(int*)_TD;_TF=*_TE;if(_TF!=14)goto _TL50;_T10=is_fat;
*_T10=1;goto _LL7;_TL50: goto _LLA;_TL4E: _LLA: _T11=is_fat;
*_T11=0;goto _LL7;_LL7:;}
# 241
return 1;}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T20=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T1A;_T12=_T20->f1;_T1F=_T12.elt_type;_T13=_T20->f1;_T1C=_T13.tq;_T14=_T20->f1;_T1B=_T14.num_elts;_T15=_T20->f1;_T1E=_T15.zero_term;}{void*elt=_T1F;struct Cyc_Absyn_Tqual tq=_T1C;struct Cyc_Absyn_Exp*n=_T1B;void*zt=_T1E;_T16=
# 243
Cyc_Absyn_type2bool(0,zt);if(_T16)goto _TL52;else{goto _TL54;}
_TL54: return 0;_TL52: _T17=elt_type;
*_T17=elt;_T18=is_fat;
*_T18=0;_T19=ptr_type;
*_T19=Cyc_Tcutil_promote_array(t,Cyc_Absyn_heap_rgn_type,Cyc_Absyn_al_qual_type,0);
return 1;}default:
 return 0;};}
# 256
int Cyc_Tcutil_is_fat_pointer_type(void*t){void*_T0;void**_T1;int _T2;
void*ignore=Cyc_Absyn_void_type;_T0=t;_T1=& ignore;_T2=
Cyc_Tcutil_is_fat_pointer_type_elt(_T0,_T1);return _T2;}
# 261
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp_constrain(void*,void*,int);
# 263
int Cyc_Tcutil_is_bound_one(void*b,int constrain){int _T0;int _T1;void*_T2;void*_T3;int _T4;int _T5;int _T6;_T0=
Cyc_Tcutil_is_cvar_type(b);if(!_T0)goto _TL55;_T1=constrain;if(_T1)goto _TL55;else{goto _TL57;}
_TL57: return 0;_TL55: _T2=
Cyc_Absyn_bounds_one();_T3=b;_T4=constrain;{struct Cyc_Absyn_Exp*eopt=Cyc_Tcutil_get_bounds_exp_constrain(_T2,_T3,_T4);
if(eopt!=0)goto _TL58;return 0;_TL58: {
struct _tuple12 _T7=Cyc_Evexp_eval_const_uint_exp(eopt);int _T8;unsigned _T9;_T9=_T7.f0;_T8=_T7.f1;{unsigned i=_T9;int known=_T8;_T6=known;
if(!_T6)goto _TL5A;_T5=i==1U;goto _TL5B;_TL5A: _T5=0;_TL5B: return _T5;}}}}
# 273
int Cyc_Tcutil_is_bits_only_type(void*t){int*_T0;unsigned _T1;void*_T2;void*_T3;int*_T4;unsigned _T5;void*_T6;struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T7;union Cyc_Absyn_AggrInfo _T8;struct _union_AggrInfo_UnknownAggr _T9;unsigned _TA;void*_TB;union Cyc_Absyn_AggrInfo _TC;struct _union_AggrInfo_KnownAggr _TD;struct Cyc_Absyn_Aggrdecl**_TE;struct Cyc_Absyn_Aggrdecl*_TF;struct Cyc_Absyn_AggrdeclImpl*_T10;struct _RegionHandle*_T11;struct _RegionHandle*_T12;struct Cyc_Absyn_Aggrdecl*_T13;struct Cyc_List_List*_T14;struct Cyc_List_List*_T15;struct Cyc_Absyn_Aggrdecl*_T16;struct Cyc_Absyn_AggrdeclImpl*_T17;struct Cyc_Absyn_AggrdeclImpl*_T18;struct _RegionHandle*_T19;struct Cyc_List_List*_T1A;struct Cyc_List_List*_T1B;void*_T1C;struct Cyc_Absyn_Aggrfield*_T1D;void*_T1E;void*_T1F;int _T20;struct Cyc_List_List*_T21;struct Cyc_Absyn_ArrayInfo _T22;struct Cyc_Absyn_ArrayInfo _T23;int _T24;int _T25;struct Cyc_List_List*_T26;void*_T27;struct Cyc_Absyn_Aggrfield*_T28;void*_T29;int _T2A;struct Cyc_List_List*_T2B;
void*_T2C=Cyc_Absyn_compress(t);struct Cyc_List_List*_T2D;void*_T2E;void*_T2F;_T0=(int*)_T2C;_T1=*_T0;switch(_T1){case 0:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T30=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2C;_T2=_T30->f1;_T2F=(void*)_T2;_T2E=_T30->f2;}{void*c=_T2F;struct Cyc_List_List*ts=_T2E;struct Cyc_Absyn_Aggrdecl*_T30;_T3=c;_T4=(int*)_T3;_T5=*_T4;switch(_T5){case 0: goto _LLD;case 1: _LLD: goto _LLF;case 2: _LLF: goto _LL11;case 3: _LL11: goto _LL13;case 20: _LL13: goto _LL15;case 19: _LL15:
# 282
 return 1;case 24: _T6=c;_T7=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T6;_T8=_T7->f1;_T9=_T8.UnknownAggr;_TA=_T9.tag;if(_TA!=1)goto _TL5E;
# 284
return 0;_TL5E: _TB=c;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T31=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_TB;_TC=_T31->f1;_TD=_TC.KnownAggr;_TE=_TD.val;{struct Cyc_Absyn_Aggrdecl*_T32=*_TE;_T30=_T32;}}{struct Cyc_Absyn_Aggrdecl*ad=_T30;_TF=ad;_T10=_TF->impl;
# 286
if(_T10!=0)goto _TL60;
return 0;_TL60: {struct _RegionHandle _T31=_new_region(0U,"rgn");struct _RegionHandle*rgn=& _T31;_push_region(rgn);_T11=rgn;_T12=rgn;_T13=ad;_T14=_T13->tvs;_T15=ts;{
# 289
struct Cyc_List_List*inst=Cyc_List_rzip(_T11,_T12,_T14,_T15);_T16=ad;_T17=_T16->impl;_T18=
_check_null(_T17);{struct Cyc_List_List*fs=_T18->fields;_TL65: if(fs!=0)goto _TL63;else{goto _TL64;}
_TL63: _T19=rgn;_T1A=inst;_T1B=fs;_T1C=_T1B->hd;_T1D=(struct Cyc_Absyn_Aggrfield*)_T1C;_T1E=_T1D->type;_T1F=Cyc_Tcutil_rsubstitute(_T19,_T1A,_T1E);_T20=Cyc_Tcutil_is_bits_only_type(_T1F);if(_T20)goto _TL66;else{goto _TL68;}
_TL68:{int _T32=0;_npop_handler(0);return _T32;}_TL66: _T21=fs;
# 290
fs=_T21->tl;goto _TL65;_TL64:;}{int _T32=1;_npop_handler(0);return _T32;}}_pop_region();}};default:
# 294
 return 0;};}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T30=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T2C;_T22=_T30->f1;_T2F=_T22.elt_type;_T23=_T30->f1;_T2E=_T23.zero_term;}{void*t=_T2F;void*zero_term=_T2E;_T25=
# 299
Cyc_Absyn_type2bool(0,zero_term);if(_T25)goto _TL69;else{goto _TL6B;}_TL6B: _T24=Cyc_Tcutil_is_bits_only_type(t);goto _TL6A;_TL69: _T24=0;_TL6A: return _T24;}case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T30=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T2C;_T2D=_T30->f3;}{struct Cyc_List_List*fs=_T2D;
# 301
_TL6F: if(fs!=0)goto _TL6D;else{goto _TL6E;}
_TL6D: _T26=fs;_T27=_T26->hd;_T28=(struct Cyc_Absyn_Aggrfield*)_T27;_T29=_T28->type;_T2A=Cyc_Tcutil_is_bits_only_type(_T29);if(_T2A)goto _TL70;else{goto _TL72;}
_TL72: return 0;_TL70: _T2B=fs;
# 301
fs=_T2B->tl;goto _TL6F;_TL6E:
# 304
 return 1;}default:
 return 0;};}
# 310
int Cyc_Tcutil_fast_tvar_cmp(struct Cyc_Absyn_Tvar*tv1,struct Cyc_Absyn_Tvar*tv2){struct Cyc_Absyn_Tvar*_T0;int _T1;struct Cyc_Absyn_Tvar*_T2;int _T3;int _T4;_T0=tv1;_T1=_T0->identity;_T2=tv2;_T3=_T2->identity;_T4=_T1 - _T3;
return _T4;}
# 314
static struct Cyc_Absyn_Tvar*Cyc_Tcutil_copy_tvar(struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;struct Cyc_Absyn_Tvar*_T1;struct Cyc_Absyn_Tvar*_T2;void*_T3;struct Cyc_Absyn_Tvar*_T4;{struct Cyc_Absyn_Tvar*_T5=_cycalloc(sizeof(struct Cyc_Absyn_Tvar));_T1=tv;
_T5->name=_T1->name;_T5->identity=- 1;_T2=tv;_T3=_T2->kind;_T5->kind=Cyc_Kinds_copy_kindbound(_T3);_T4=tv;_T5->aquals_bound=_T4->aquals_bound;_T0=(struct Cyc_Absyn_Tvar*)_T5;}return _T0;}
# 317
static struct _tuple9*Cyc_Tcutil_copy_arg(struct _tuple9*arg){struct _tuple9*_T0;struct _tuple9*_T1;void*_T2;struct Cyc_Absyn_Tqual _T3;struct _fat_ptr*_T4;_T0=arg;{struct _tuple9 _T5=*_T0;_T4=_T5.f0;_T3=_T5.f1;_T2=_T5.f2;}{struct _fat_ptr*x=_T4;struct Cyc_Absyn_Tqual y=_T3;void*t=_T2;{struct _tuple9*_T5=_cycalloc(sizeof(struct _tuple9));
# 319
_T5->f0=x;_T5->f1=y;_T5->f2=Cyc_Tcutil_copy_type(t);_T1=(struct _tuple9*)_T5;}return _T1;}}struct _tuple14{struct Cyc_Absyn_Tqual f0;void*f1;};
# 321
static struct _tuple14*Cyc_Tcutil_copy_tqt(struct _tuple14*arg){struct _tuple14*_T0;struct _tuple14*_T1;struct _tuple14 _T2;struct _tuple14*_T3;struct _tuple14 _T4;void*_T5;{struct _tuple14*_T6=_cycalloc(sizeof(struct _tuple14));_T1=arg;_T2=*_T1;
_T6->f0=_T2.f0;_T3=arg;_T4=*_T3;_T5=_T4.f1;_T6->f1=Cyc_Tcutil_copy_type(_T5);_T0=(struct _tuple14*)_T6;}return _T0;}
# 324
static struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp_opt(int preserve_types,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;
if(e!=0)goto _TL73;_T0=0;goto _TL74;_TL73: _T0=Cyc_Tcutil_deep_copy_exp(preserve_types,e);_TL74: return _T0;}
# 327
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_copy_field(struct Cyc_Absyn_Aggrfield*f){struct Cyc_Absyn_Aggrfield*_T0;struct Cyc_Absyn_Aggrfield*_T1;struct Cyc_Absyn_Aggrfield*_T2;struct Cyc_Absyn_Aggrfield*_T3;void*_T4;struct Cyc_Absyn_Aggrfield*_T5;struct Cyc_Absyn_Aggrfield*_T6;struct Cyc_Absyn_Aggrfield*_T7;struct Cyc_Absyn_Exp*_T8;{struct Cyc_Absyn_Aggrfield*_T9=_cycalloc(sizeof(struct Cyc_Absyn_Aggrfield));_T1=f;
_T9->name=_T1->name;_T2=f;_T9->tq=_T2->tq;_T3=f;_T4=_T3->type;_T9->type=Cyc_Tcutil_copy_type(_T4);_T5=f;_T9->width=_T5->width;_T6=f;_T9->attributes=_T6->attributes;_T7=f;_T8=_T7->requires_clause;
_T9->requires_clause=Cyc_Tcutil_deep_copy_exp_opt(1,_T8);_T0=(struct Cyc_Absyn_Aggrfield*)_T9;}
# 328
return _T0;}struct _tuple15{void*f0;void*f1;};
# 331
static struct _tuple15*Cyc_Tcutil_copy_type_pair(struct _tuple15*x){struct _tuple15*_T0;struct _tuple15*_T1;void*_T2;void*_T3;_T0=x;{struct _tuple15 _T4=*_T0;_T3=_T4.f0;_T2=_T4.f1;}{void*r1=_T3;void*r2=_T2;{struct _tuple15*_T4=_cycalloc(sizeof(struct _tuple15));
# 333
_T4->f0=Cyc_Tcutil_copy_type(r1);_T4->f1=Cyc_Tcutil_copy_type(r2);_T1=(struct _tuple15*)_T4;}return _T1;}}
# 335
static void*Cyc_Tcutil_copy_effconstr(void*fc){void*_T0;int*_T1;unsigned _T2;void*_T3;void*_T4;struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T5;void*_T6;void*_T7;void*_T8;void*_T9;struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_TA;void*_TB;void*_TC;void*_TD;void*_TE;struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_TF;void*_T10;void*_T11;void*_T12;_T0=fc;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 0: _T3=fc;{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T13=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T3;_T4=_T13->f1;_T12=(void*)_T4;}{void*e=_T12;{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T13=_cycalloc(sizeof(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct));_T13->tag=0;
# 338
_T13->f1=Cyc_Tcutil_copy_type(e);_T5=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T13;}_T6=(void*)_T5;return _T6;}case 1: _T7=fc;{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T13=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T7;_T8=_T13->f1;_T12=(void*)_T8;_T9=_T13->f2;_T11=(void*)_T9;}{void*e1=_T12;void*e2=_T11;{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T13=_cycalloc(sizeof(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct));_T13->tag=1;
# 340
_T13->f1=Cyc_Tcutil_copy_type(e1);_T13->f2=Cyc_Tcutil_copy_type(e2);_TA=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T13;}_TB=(void*)_TA;return _TB;}default: _TC=fc;{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T13=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_TC;_TD=_T13->f1;_T12=(void*)_TD;_TE=_T13->f2;_T11=(void*)_TE;}{void*e1=_T12;void*e2=_T11;{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T13=_cycalloc(sizeof(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct));_T13->tag=2;
# 342
_T13->f1=Cyc_Tcutil_copy_type(e1);_T13->f2=Cyc_Tcutil_copy_type(e2);_TF=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T13;}_T10=(void*)_TF;return _T10;}};}
# 345
static void*Cyc_Tcutil_tvar2type(struct Cyc_Absyn_Tvar*t){struct Cyc_Absyn_Tvar*_T0;void*_T1;_T0=
Cyc_Tcutil_copy_tvar(t);_T1=Cyc_Absyn_var_type(_T0);return _T1;}
# 348
static struct Cyc_Absyn_Vardecl*Cyc_Tcutil_copy_vardecl_opt(struct Cyc_Absyn_Vardecl*vdopt){struct Cyc_Absyn_Vardecl*_T0;struct Cyc_Absyn_Vardecl*_T1;struct Cyc_Absyn_Vardecl*_T2;struct Cyc_Absyn_Vardecl*_T3;void*_T4;struct Cyc_Absyn_Vardecl*_T5;struct Cyc_Absyn_Vardecl*_T6;struct Cyc_Absyn_Exp*_T7;struct Cyc_Absyn_Vardecl*_T8;void*_T9;struct Cyc_Absyn_Vardecl*_TA;struct Cyc_Absyn_Vardecl*_TB;void*_TC;struct Cyc_Absyn_Vardecl*_TD;
if(vdopt!=0)goto _TL76;return 0;_TL76: {
struct Cyc_Absyn_Vardecl*vd;vd=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));_T0=vd;_T1=vdopt;*_T0=*_T1;_T2=vd;_T3=vd;_T4=_T3->type;
_T2->type=Cyc_Tcutil_copy_type(_T4);_T5=vd;_T6=vd;_T7=_T6->initializer;
_T5->initializer=Cyc_Tcutil_deep_copy_exp_opt(1,_T7);_T8=vd;_T9=_T8->rgn;
if(_T9==0)goto _TL78;_TA=vd;_TB=vd;_TC=_TB->rgn;
_TA->rgn=Cyc_Tcutil_copy_type(_TC);goto _TL79;_TL78: _TL79: _TD=vd;
return _TD;}}struct _tuple16{struct Cyc_Absyn_Vardecl*f0;struct Cyc_Absyn_Exp*f1;};
# 358
void*Cyc_Tcutil_copy_type(void*t){int*_T0;unsigned _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;struct Cyc_List_List*_T3;void*_T4;void*_T5;void*_T6;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T7;struct Cyc_List_List*_T8;void*_T9;void*_TA;void*_TB;struct Cyc_Absyn_Tvar*_TC;void*_TD;struct Cyc_Absyn_PtrInfo _TE;struct Cyc_Absyn_PtrInfo _TF;struct Cyc_Absyn_PtrInfo _T10;struct Cyc_Absyn_PtrAtts _T11;struct Cyc_Absyn_PtrInfo _T12;struct Cyc_Absyn_PtrAtts _T13;struct Cyc_Absyn_PtrInfo _T14;struct Cyc_Absyn_PtrAtts _T15;struct Cyc_Absyn_PtrInfo _T16;struct Cyc_Absyn_PtrAtts _T17;struct Cyc_Absyn_PtrInfo _T18;struct Cyc_Absyn_PtrAtts _T19;struct Cyc_Absyn_PtrInfo _T1A;struct Cyc_Absyn_PtrAtts _T1B;struct Cyc_Absyn_PtrInfo _T1C;struct Cyc_Absyn_PtrAtts _T1D;struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T1E;void*_T1F;struct Cyc_Absyn_ArrayInfo _T20;struct Cyc_Absyn_ArrayInfo _T21;struct Cyc_Absyn_ArrayInfo _T22;struct Cyc_Absyn_ArrayInfo _T23;struct Cyc_Absyn_ArrayInfo _T24;struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T25;void*_T26;struct Cyc_Absyn_FnInfo _T27;struct Cyc_Absyn_FnInfo _T28;struct Cyc_Absyn_FnInfo _T29;struct Cyc_Absyn_FnInfo _T2A;struct Cyc_Absyn_FnInfo _T2B;struct Cyc_Absyn_FnInfo _T2C;struct Cyc_Absyn_FnInfo _T2D;struct Cyc_Absyn_FnInfo _T2E;struct Cyc_Absyn_FnInfo _T2F;struct Cyc_Absyn_FnInfo _T30;struct Cyc_Absyn_FnInfo _T31;struct Cyc_Absyn_FnInfo _T32;struct Cyc_Absyn_FnInfo _T33;struct Cyc_Absyn_FnInfo _T34;struct Cyc_Absyn_FnInfo _T35;struct Cyc_Absyn_FnInfo _T36;struct Cyc_Absyn_FnInfo _T37;struct Cyc_Absyn_FnInfo _T38;struct Cyc_Absyn_FnInfo _T39;struct Cyc_Absyn_FnInfo _T3A;struct Cyc_List_List*(*_T3B)(struct Cyc_Absyn_Tvar*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*);struct Cyc_List_List*(*_T3C)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T3D;void*_T3E;struct Cyc_List_List*(*_T3F)(struct _tuple9*(*)(struct _tuple9*),struct Cyc_List_List*);struct Cyc_List_List*(*_T40)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T41;struct Cyc_Absyn_VarargInfo*_T42;struct Cyc_Absyn_VarargInfo*_T43;struct Cyc_Absyn_VarargInfo*_T44;struct Cyc_Absyn_VarargInfo*_T45;struct Cyc_Absyn_VarargInfo*_T46;void*_T47;struct Cyc_List_List*_T48;struct Cyc_List_List*(*_T49)(struct _tuple15*(*)(struct _tuple15*),struct Cyc_List_List*);struct Cyc_List_List*(*_T4A)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T4B;struct Cyc_List_List*(*_T4C)(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*);struct Cyc_List_List*(*_T4D)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T4E;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T4F;void*_T50;struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T51;struct Cyc_List_List*(*_T52)(struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*);struct Cyc_List_List*(*_T53)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T54;void*_T55;struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T56;void*_T57;struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_T58;void*_T59;struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T5A;struct Cyc_List_List*_T5B;void*_T5C;struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_T5D;struct Cyc_Absyn_TypeDecl*_T5E;struct Cyc_Absyn_TypeDecl*_T5F;void*_T60;int*_T61;unsigned _T62;struct Cyc_Absyn_TypeDecl*_T63;void*_T64;struct Cyc_List_List*(*_T65)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*);struct Cyc_List_List*(*_T66)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_Absyn_Aggrdecl*_T67;struct Cyc_List_List*_T68;struct Cyc_Absyn_Aggrdecl*_T69;enum Cyc_Absyn_AggrKind _T6A;struct Cyc_Absyn_Aggrdecl*_T6B;struct _tuple1*_T6C;union Cyc_Absyn_AggrInfo _T6D;struct Cyc_List_List*_T6E;void*_T6F;struct Cyc_Absyn_TypeDecl*_T70;void*_T71;struct Cyc_Absyn_Enumdecl*_T72;struct _tuple1*_T73;void*_T74;struct Cyc_Absyn_TypeDecl*_T75;void*_T76;struct Cyc_List_List*(*_T77)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*);struct Cyc_List_List*(*_T78)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_Absyn_Datatypedecl*_T79;struct Cyc_List_List*_T7A;struct Cyc_Absyn_UnknownDatatypeInfo _T7B;struct Cyc_Absyn_Datatypedecl*_T7C;union Cyc_Absyn_DatatypeInfo _T7D;struct Cyc_List_List*_T7E;void*_T7F;struct _tuple16 _T80;struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T81;void*_T82;struct Cyc_List_List _T83;struct Cyc_List_List*_T84;struct Cyc_List_List*_T85;struct Cyc_Absyn_Exp*_T86;struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_T87;void*_T88;
void*_T89=Cyc_Absyn_compress(t);struct Cyc_Absyn_Datatypedecl*_T8A;struct Cyc_Absyn_Enumdecl*_T8B;struct Cyc_Absyn_Aggrdecl*_T8C;struct Cyc_Absyn_Typedefdecl*_T8D;struct _tuple1*_T8E;enum Cyc_Absyn_AggrKind _T8F;struct Cyc_List_List*_T90;struct Cyc_List_List*_T91;struct Cyc_Absyn_Vardecl*_T92;struct Cyc_AssnDef_ExistAssnFn*_T93;struct Cyc_Absyn_Exp*_T94;struct Cyc_AssnDef_ExistAssnFn*_T95;struct Cyc_Absyn_Exp*_T96;struct Cyc_AssnDef_ExistAssnFn*_T97;struct Cyc_Absyn_Exp*_T98;struct Cyc_AssnDef_ExistAssnFn*_T99;struct Cyc_Absyn_Exp*_T9A;struct Cyc_List_List*_T9B;struct Cyc_List_List*_T9C;struct Cyc_Absyn_VarargInfo*_T9D;int _T9E;struct Cyc_List_List*_T9F;struct Cyc_List_List*_TA0;unsigned _TA1;void*_TA2;void*_TA3;struct Cyc_Absyn_PtrLoc*_TA4;void*_TA5;void*_TA6;void*_TA7;struct Cyc_Absyn_Tqual _TA8;void*_TA9;void*_TAA;_T0=(int*)_T89;_T1=*_T0;switch(_T1){case 0: _T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T89;_T3=_T2->f2;if(_T3!=0)goto _TL7B;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T89;_T4=_TAB->f1;_TAA=(void*)_T4;}{void*c=_TAA;_T5=t;
return _T5;}_TL7B:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T89;_T6=_TAB->f1;_TAA=(void*)_T6;_TA9=_TAB->f2;}{void*c=_TAA;struct Cyc_List_List*ts=_TA9;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_AppType_Absyn_Type_struct));_TAB->tag=0;
_TAB->f1=c;_T8=ts;_TAB->f2=Cyc_List_map(Cyc_Tcutil_copy_type,_T8);_T7=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TAB;}_T9=(void*)_T7;return _T9;}case 3: _TA=t;
return _TA;case 1: _TB=t;
return _TB;case 2:{struct Cyc_Absyn_VarType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T89;_TAA=_TAB->f1;}{struct Cyc_Absyn_Tvar*tv=_TAA;_TC=
Cyc_Tcutil_copy_tvar(tv);_TD=Cyc_Absyn_var_type(_TC);return _TD;}case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T89;_TE=_TAB->f1;_TAA=_TE.elt_type;_TF=_TAB->f1;_TA8=_TF.elt_tq;_T10=_TAB->f1;_T11=_T10.ptr_atts;_TA9=_T11.eff;_T12=_TAB->f1;_T13=_T12.ptr_atts;_TA7=_T13.nullable;_T14=_TAB->f1;_T15=_T14.ptr_atts;_TA6=_T15.bounds;_T16=_TAB->f1;_T17=_T16.ptr_atts;_TA5=_T17.zero_term;_T18=_TAB->f1;_T19=_T18.ptr_atts;_TA4=_T19.ptrloc;_T1A=_TAB->f1;_T1B=_T1A.ptr_atts;_TA3=_T1B.autoreleased;_T1C=_TAB->f1;_T1D=_T1C.ptr_atts;_TA2=_T1D.aqual;}{void*elt=_TAA;struct Cyc_Absyn_Tqual tq=_TA8;void*rgn=_TA9;void*nbl=_TA7;void*bs=_TA6;void*zt=_TA5;struct Cyc_Absyn_PtrLoc*loc=_TA4;void*rel=_TA3;void*aq=_TA2;
# 366
void*elt2=Cyc_Tcutil_copy_type(elt);
void*rgn2=Cyc_Tcutil_copy_type(rgn);
void*nbl2=Cyc_Tcutil_copy_type(nbl);
struct Cyc_Absyn_Tqual tq2=tq;
# 371
void*bs2=Cyc_Tcutil_copy_type(bs);
void*zt2=Cyc_Tcutil_copy_type(zt);
void*rel2=Cyc_Tcutil_copy_type(rel);{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_PointerType_Absyn_Type_struct));_TAB->tag=4;
_TAB->f1.elt_type=elt2;_TAB->f1.elt_tq=tq2;_TAB->f1.ptr_atts.eff=rgn2;_TAB->f1.ptr_atts.nullable=nbl2;_TAB->f1.ptr_atts.bounds=bs2;_TAB->f1.ptr_atts.zero_term=zt2;_TAB->f1.ptr_atts.ptrloc=loc;_TAB->f1.ptr_atts.autoreleased=rel2;_TAB->f1.ptr_atts.aqual=aq;_T1E=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TAB;}_T1F=(void*)_T1E;return _T1F;}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T89;_T20=_TAB->f1;_TAA=_T20.elt_type;_T21=_TAB->f1;_TA8=_T21.tq;_T22=_TAB->f1;_TA9=_T22.num_elts;_T23=_TAB->f1;_TA7=_T23.zero_term;_T24=_TAB->f1;_TA1=_T24.zt_loc;}{void*et=_TAA;struct Cyc_Absyn_Tqual tq=_TA8;struct Cyc_Absyn_Exp*eo=_TA9;void*zt=_TA7;unsigned ztl=_TA1;{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_ArrayType_Absyn_Type_struct));_TAB->tag=5;
# 376
_TAB->f1.elt_type=Cyc_Tcutil_copy_type(et);_TAB->f1.tq=tq;_TAB->f1.num_elts=Cyc_Tcutil_deep_copy_exp_opt(1,eo);
_TAB->f1.zero_term=Cyc_Tcutil_copy_type(zt);_TAB->f1.zt_loc=ztl;_T25=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_TAB;}_T26=(void*)_T25;
# 376
return _T26;}case 6:{struct Cyc_Absyn_FnType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T89;_T27=_TAB->f1;_TA0=_T27.tvars;_T28=_TAB->f1;_TAA=_T28.effect;_T29=_TAB->f1;_TA8=_T29.ret_tqual;_T2A=_TAB->f1;_TA9=_T2A.ret_type;_T2B=_TAB->f1;_T9F=_T2B.args;_T2C=_TAB->f1;_T9E=_T2C.c_varargs;_T2D=_TAB->f1;_T9D=_T2D.cyc_varargs;_T2E=_TAB->f1;_T9C=_T2E.qual_bnd;_T2F=_TAB->f1;_T9B=_T2F.attributes;_T30=_TAB->f1;_T9A=_T30.checks_clause;_T31=_TAB->f1;_T99=_T31.checks_assn;_T32=_TAB->f1;_T98=_T32.requires_clause;_T33=_TAB->f1;_T97=_T33.requires_assn;_T34=_TAB->f1;_T96=_T34.ensures_clause;_T35=_TAB->f1;_T95=_T35.ensures_assn;_T36=_TAB->f1;_T94=_T36.throws_clause;_T37=_TAB->f1;_T93=_T37.throws_assn;_T38=_TAB->f1;_T92=_T38.return_value;_T39=_TAB->f1;_T91=_T39.arg_vardecls;_T3A=_TAB->f1;_T90=_T3A.effconstr;}{struct Cyc_List_List*tvs=_TA0;void*effopt=_TAA;struct Cyc_Absyn_Tqual rt_tq=_TA8;void*rt=_TA9;struct Cyc_List_List*args=_T9F;int c_varargs=_T9E;struct Cyc_Absyn_VarargInfo*cyc_varargs=_T9D;struct Cyc_List_List*qb=_T9C;struct Cyc_List_List*atts=_T9B;struct Cyc_Absyn_Exp*chk=_T9A;struct Cyc_AssnDef_ExistAssnFn*chk_rlns=_T99;struct Cyc_Absyn_Exp*req=_T98;struct Cyc_AssnDef_ExistAssnFn*req_rlns=_T97;struct Cyc_Absyn_Exp*ens=_T96;struct Cyc_AssnDef_ExistAssnFn*ens_rlns=_T95;struct Cyc_Absyn_Exp*thrws=_T94;struct Cyc_AssnDef_ExistAssnFn*thrws_rlns=_T93;struct Cyc_Absyn_Vardecl*ret_val=_T92;struct Cyc_List_List*argvds=_T91;struct Cyc_List_List*effc=_T90;_T3C=Cyc_List_map;{
# 380
struct Cyc_List_List*(*_TAB)(struct Cyc_Absyn_Tvar*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))_T3C;_T3B=_TAB;}_T3D=tvs;{struct Cyc_List_List*tvs2=_T3B(Cyc_Tcutil_copy_tvar,_T3D);
if(effopt!=0)goto _TL7D;_T3E=0;goto _TL7E;_TL7D: _T3E=Cyc_Tcutil_copy_type(effopt);_TL7E: {void*effopt2=_T3E;
void*rt2=Cyc_Tcutil_copy_type(rt);_T40=Cyc_List_map;{
struct Cyc_List_List*(*_TAB)(struct _tuple9*(*)(struct _tuple9*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple9*(*)(struct _tuple9*),struct Cyc_List_List*))_T40;_T3F=_TAB;}_T41=args;{struct Cyc_List_List*args2=_T3F(Cyc_Tcutil_copy_arg,_T41);
int c_varargs2=c_varargs;
if(cyc_varargs!=0)goto _TL7F;_T42=0;goto _TL80;_TL7F:{struct Cyc_Absyn_VarargInfo*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_VarargInfo));_T44=cyc_varargs;*_TAB=*_T44;_T43=(struct Cyc_Absyn_VarargInfo*)_TAB;}_T42=_T43;_TL80: {struct Cyc_Absyn_VarargInfo*cyc_varargs2=_T42;
if(cyc_varargs==0)goto _TL81;_T45=cyc_varargs2;_T46=cyc_varargs;_T47=_T46->type;
_T45->type=Cyc_Tcutil_copy_type(_T47);goto _TL82;_TL81: _TL82: _T48=effc;{
struct Cyc_List_List*effc2=Cyc_List_map(Cyc_Tcutil_copy_effconstr,_T48);_T4A=Cyc_List_map;{
struct Cyc_List_List*(*_TAB)(struct _tuple15*(*)(struct _tuple15*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple15*(*)(struct _tuple15*),struct Cyc_List_List*))_T4A;_T49=_TAB;}_T4B=qb;{struct Cyc_List_List*qb2=_T49(Cyc_Tcutil_copy_type_pair,_T4B);
struct Cyc_List_List*atts2=atts;
struct Cyc_Absyn_Exp*chk2=Cyc_Tcutil_deep_copy_exp_opt(1,chk);
struct Cyc_AssnDef_ExistAssnFn*chk_rlns2=chk_rlns;
struct Cyc_Absyn_Exp*req2=Cyc_Tcutil_deep_copy_exp_opt(1,req);
struct Cyc_AssnDef_ExistAssnFn*req_rlns2=req_rlns;
struct Cyc_Absyn_Exp*ens2=Cyc_Tcutil_deep_copy_exp_opt(1,ens);
struct Cyc_AssnDef_ExistAssnFn*ens_rlns2=ens_rlns;
struct Cyc_Absyn_Exp*thrws2=Cyc_Tcutil_deep_copy_exp_opt(1,thrws);
struct Cyc_AssnDef_ExistAssnFn*thrws_rlns2=thrws_rlns;_T4D=Cyc_List_map;{
# 403
struct Cyc_List_List*(*_TAB)(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Vardecl*(*)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*))_T4D;_T4C=_TAB;}_T4E=argvds;{struct Cyc_List_List*argvds2=_T4C(Cyc_Tcutil_copy_vardecl_opt,_T4E);
struct Cyc_Absyn_Vardecl*ret_val2=Cyc_Tcutil_copy_vardecl_opt(ret_val);{struct Cyc_Absyn_FnType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_TAB->tag=6;
_TAB->f1.tvars=tvs2;_TAB->f1.effect=effopt2;_TAB->f1.ret_tqual=rt_tq;_TAB->f1.ret_type=rt2;_TAB->f1.args=args2;_TAB->f1.c_varargs=c_varargs2;
_TAB->f1.cyc_varargs=cyc_varargs2;_TAB->f1.qual_bnd=qb2;_TAB->f1.attributes=atts2;_TAB->f1.checks_clause=chk2;_TAB->f1.checks_assn=chk_rlns2;
_TAB->f1.requires_clause=req2;_TAB->f1.requires_assn=req_rlns2;
_TAB->f1.ensures_clause=ens2;_TAB->f1.ensures_assn=ens_rlns2;_TAB->f1.throws_clause=thrws2;_TAB->f1.throws_assn=thrws_rlns2;_TAB->f1.return_value=ret_val2;
_TAB->f1.arg_vardecls=argvds2;_TAB->f1.effconstr=effc2;_T4F=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_TAB;}_T50=(void*)_T4F;
# 405
return _T50;}}}}}}}}case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T89;_T8F=_TAB->f1;_T9E=_TAB->f2;_TA0=_TAB->f3;}{enum Cyc_Absyn_AggrKind k=_T8F;int b=_T9E;struct Cyc_List_List*fs=_TA0;{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct));_TAB->tag=7;
# 410
_TAB->f1=k;_TAB->f2=b;_T53=Cyc_List_map;{struct Cyc_List_List*(*_TAC)(struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*))_T53;_T52=_TAC;}_T54=fs;_TAB->f3=_T52(Cyc_Tcutil_copy_field,_T54);_T51=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_TAB;}_T55=(void*)_T51;return _T55;}case 9:{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_T89;_T9A=_TAB->f1;}{struct Cyc_Absyn_Exp*e=_T9A;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_ValueofType_Absyn_Type_struct));_TAB->tag=9;
_TAB->f1=e;_T56=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TAB;}_T57=(void*)_T56;return _T57;}case 11:{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_T89;_T9A=_TAB->f1;}{struct Cyc_Absyn_Exp*e=_T9A;{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_TypeofType_Absyn_Type_struct));_TAB->tag=11;
_TAB->f1=e;_T58=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_TAB;}_T59=(void*)_T58;return _T59;}case 8:{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T89;_T8E=_TAB->f1;_TA0=_TAB->f2;_T8D=_TAB->f3;}{struct _tuple1*tdn=_T8E;struct Cyc_List_List*ts=_TA0;struct Cyc_Absyn_Typedefdecl*td=_T8D;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_TypedefType_Absyn_Type_struct));_TAB->tag=8;
# 414
_TAB->f1=tdn;_T5B=ts;_TAB->f2=Cyc_List_map(Cyc_Tcutil_copy_type,_T5B);_TAB->f3=td;_TAB->f4=0;_T5A=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_TAB;}_T5C=(void*)_T5A;return _T5C;}case 10: _T5D=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_T89;_T5E=_T5D->f1;_T5F=(struct Cyc_Absyn_TypeDecl*)_T5E;_T60=_T5F->r;_T61=(int*)_T60;_T62=*_T61;switch(_T62){case 0:{struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_T89;_T63=_TAB->f1;{struct Cyc_Absyn_TypeDecl _TAC=*_T63;_T64=_TAC.r;{struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*_TAD=(struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)_T64;_T8C=_TAD->f1;}}}{struct Cyc_Absyn_Aggrdecl*ad=_T8C;_T66=Cyc_List_map;{
# 417
struct Cyc_List_List*(*_TAB)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))_T66;_T65=_TAB;}_T67=ad;_T68=_T67->tvs;{struct Cyc_List_List*targs=_T65(Cyc_Tcutil_tvar2type,_T68);_T69=ad;_T6A=_T69->kind;_T6B=ad;_T6C=_T6B->name;_T6D=
Cyc_Absyn_UnknownAggr(_T6A,_T6C,0);_T6E=targs;_T6F=Cyc_Absyn_aggr_type(_T6D,_T6E);return _T6F;}}case 1:{struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_T89;_T70=_TAB->f1;{struct Cyc_Absyn_TypeDecl _TAC=*_T70;_T71=_TAC.r;{struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*_TAD=(struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)_T71;_T8B=_TAD->f1;}}}{struct Cyc_Absyn_Enumdecl*ed=_T8B;_T72=ed;_T73=_T72->name;_T74=
# 420
Cyc_Absyn_enum_type(_T73,0);return _T74;}default:{struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_T89;_T75=_TAB->f1;{struct Cyc_Absyn_TypeDecl _TAC=*_T75;_T76=_TAC.r;{struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*_TAD=(struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)_T76;_T8A=_TAD->f1;}}}{struct Cyc_Absyn_Datatypedecl*dd=_T8A;_T78=Cyc_List_map;{
# 422
struct Cyc_List_List*(*_TAB)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))_T78;_T77=_TAB;}_T79=dd;_T7A=_T79->tvs;{struct Cyc_List_List*targs=_T77(Cyc_Tcutil_tvar2type,_T7A);{struct Cyc_Absyn_UnknownDatatypeInfo _TAB;_T7C=dd;
_TAB.name=_T7C->name;_TAB.is_extensible=0;_T7B=_TAB;}_T7D=Cyc_Absyn_UnknownDatatype(_T7B);_T7E=targs;_T7F=Cyc_Absyn_datatype_type(_T7D,_T7E);return _T7F;}}};default:{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TAB=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_T89;_T92=_TAB->f1;_T9A=_TAB->f2;_T99=_TAB->f3;}{struct Cyc_Absyn_Vardecl*vd=_T92;struct Cyc_Absyn_Exp*w=_T9A;struct Cyc_AssnDef_ExistAssnFn*a=_T99;
# 425
struct Cyc_Absyn_Vardecl*vd2=Cyc_Tcutil_copy_vardecl_opt(vd);
struct Cyc_Absyn_Exp*w2=Cyc_Tcutil_deep_copy_exp(1,w);{struct _tuple16 _TAB;
_TAB.f0=vd;{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_TAC=_cycalloc(sizeof(struct Cyc_Absyn_Local_b_Absyn_Binding_struct));_TAC->tag=4;_TAC->f1=vd2;_T81=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_TAC;}_T82=(void*)_T81;_TAB.f1=Cyc_Absyn_varb_exp(_T82,0U);_T80=_TAB;}{struct _tuple16 p=_T80;{struct Cyc_List_List _TAB;
_TAB.hd=& p;_TAB.tl=0;_T83=_TAB;}{struct Cyc_List_List subst=_T83;_T84=& subst;_T85=(struct Cyc_List_List*)_T84;_T86=w2;
Cyc_Tcutil_substitute_vars(_T85,_T86);{
struct Cyc_AssnDef_ExistAssnFn*a2=a;{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TAB=_cycalloc(sizeof(struct Cyc_Absyn_SubsetType_Absyn_Type_struct));_TAB->tag=12;
_TAB->f1=vd2;_TAB->f2=w2;_TAB->f3=a2;_T87=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_TAB;}_T88=(void*)_T87;return _T88;}}}}};}
# 436
static void*Cyc_Tcutil_copy_designator(int preserve_types,void*d){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_T4;void*_T5;void*_T6;struct Cyc_Absyn_Exp*_T7;_T0=d;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TL84;_T3=d;{struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_T8=(struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_T3;_T7=_T8->f1;}{struct Cyc_Absyn_Exp*e=_T7;{struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_T8=_cycalloc(sizeof(struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct));_T8->tag=0;
# 438
_T8->f1=Cyc_Tcutil_deep_copy_exp(preserve_types,e);_T4=(struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_T8;}_T5=(void*)_T4;return _T5;}_TL84: _T6=d;
return _T6;;}struct _tuple17{struct Cyc_List_List*f0;struct Cyc_Absyn_Exp*f1;};
# 442
static struct _tuple17*Cyc_Tcutil_copy_eds(int preserve_types,struct _tuple17*e){struct _tuple17*_T0;struct Cyc_List_List*(*_T1)(void*(*)(int,void*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T2)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _T3;struct _tuple17*_T4;struct _tuple17 _T5;struct Cyc_List_List*_T6;int _T7;struct _tuple17*_T8;struct _tuple17 _T9;struct Cyc_Absyn_Exp*_TA;{struct _tuple17*_TB=_cycalloc(sizeof(struct _tuple17));_T2=Cyc_List_map_c;{
# 444
struct Cyc_List_List*(*_TC)(void*(*)(int,void*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(int,void*),int,struct Cyc_List_List*))_T2;_T1=_TC;}_T3=preserve_types;_T4=e;_T5=*_T4;_T6=_T5.f0;_TB->f0=_T1(Cyc_Tcutil_copy_designator,_T3,_T6);_T7=preserve_types;_T8=e;_T9=*_T8;_TA=_T9.f1;
_TB->f1=Cyc_Tcutil_deep_copy_exp(_T7,_TA);_T0=(struct _tuple17*)_TB;}
# 444
return _T0;}
# 448
struct Cyc_Absyn_Stmt*Cyc_Tcutil_deep_copy_stmt(int,struct Cyc_Absyn_Stmt*);
struct Cyc_Absyn_Exp*Cyc_Tcutil_deep_copy_exp(int preserve_types,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;union Cyc_Absyn_Cnst _T3;struct Cyc_Absyn_Exp*_T4;unsigned _T5;void*_T6;void*_T7;struct Cyc_Absyn_Exp*_T8;unsigned _T9;struct _fat_ptr _TA;struct Cyc_Absyn_Exp*_TB;unsigned _TC;enum Cyc_Absyn_Primop _TD;struct Cyc_List_List*(*_TE)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_TF)(void*(*)(void*,void*),void*,struct Cyc_List_List*);struct Cyc_Absyn_Exp*(*_T10)(int,struct Cyc_Absyn_Exp*);int _T11;struct Cyc_List_List*_T12;struct Cyc_List_List*_T13;struct Cyc_Absyn_Exp*_T14;unsigned _T15;struct Cyc_Absyn_Exp*_T16;enum Cyc_Absyn_Incrementor _T17;struct Cyc_Absyn_Exp*_T18;unsigned _T19;struct Cyc_Absyn_Exp*_T1A;struct Cyc_Absyn_Exp*_T1B;struct Cyc_Absyn_Exp*_T1C;unsigned _T1D;struct Cyc_Absyn_Exp*_T1E;struct Cyc_Absyn_Exp*_T1F;struct Cyc_Absyn_Exp*_T20;unsigned _T21;struct Cyc_Absyn_Exp*_T22;struct Cyc_Absyn_Exp*_T23;struct Cyc_Absyn_Exp*_T24;unsigned _T25;struct Cyc_Absyn_Exp*_T26;struct Cyc_Absyn_Exp*_T27;struct Cyc_Absyn_Exp*_T28;struct Cyc_Absyn_Exp*_T29;unsigned _T2A;struct Cyc_Absyn_Exp*_T2B;struct Cyc_Core_Opt*_T2C;struct Cyc_Core_Opt*_T2D;unsigned long _T2E;struct Cyc_Core_Opt*_T2F;struct Cyc_Core_Opt*_T30;void*_T31;struct Cyc_Absyn_Exp*_T32;struct Cyc_Absyn_Exp*_T33;unsigned _T34;struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T35;struct Cyc_Absyn_VarargCallInfo*_T36;struct Cyc_Absyn_VarargCallInfo*_T37;struct Cyc_Absyn_VarargInfo*_T38;struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T39;struct Cyc_List_List*(*_T3A)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T3B)(void*(*)(void*,void*),void*,struct Cyc_List_List*);struct Cyc_Absyn_Exp*(*_T3C)(int,struct Cyc_Absyn_Exp*);int _T3D;struct Cyc_List_List*_T3E;struct Cyc_Absyn_VarargCallInfo*_T3F;struct Cyc_Absyn_VarargInfo*_T40;void*_T41;struct Cyc_Absyn_Exp*_T42;unsigned _T43;struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T44;struct Cyc_List_List*(*_T45)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T46)(void*(*)(void*,void*),void*,struct Cyc_List_List*);struct Cyc_Absyn_Exp*(*_T47)(int,struct Cyc_Absyn_Exp*);int _T48;struct Cyc_List_List*_T49;void*_T4A;struct Cyc_Absyn_Exp*_T4B;unsigned _T4C;struct Cyc_Absyn_Exp*_T4D;int _T4E;struct Cyc_Absyn_Exp*_T4F;struct Cyc_Absyn_Exp*_T50;unsigned _T51;struct Cyc_Absyn_Exp*_T52;struct Cyc_Absyn_Exp*_T53;unsigned _T54;struct Cyc_Absyn_Exp*_T55;struct Cyc_Absyn_Exp*_T56;unsigned _T57;struct Cyc_Absyn_Exp*_T58;struct Cyc_List_List*_T59;struct Cyc_List_List*_T5A;struct Cyc_Absyn_Exp*_T5B;unsigned _T5C;void*_T5D;void*_T5E;struct Cyc_Absyn_Exp*_T5F;int _T60;enum Cyc_Absyn_Coercion _T61;struct Cyc_Absyn_Exp*_T62;unsigned _T63;struct Cyc_Absyn_Exp*_T64;struct Cyc_Absyn_Exp*_T65;unsigned long _T66;struct Cyc_Absyn_Exp*_T67;struct Cyc_Absyn_Exp*_T68;struct Cyc_Absyn_Exp*_T69;unsigned long _T6A;struct Cyc_Absyn_Exp*_T6B;unsigned _T6C;struct Cyc_Absyn_Exp*_T6D;struct Cyc_Absyn_Exp*_T6E;unsigned _T6F;void*_T70;void*_T71;struct Cyc_Absyn_Exp*_T72;unsigned _T73;struct Cyc_Absyn_Exp*_T74;struct Cyc_Absyn_Exp*_T75;unsigned _T76;void*_T77;void*_T78;struct Cyc_Absyn_Exp*_T79;unsigned _T7A;struct Cyc_Absyn_Exp*_T7B;struct Cyc_Absyn_Exp*_T7C;unsigned _T7D;void*_T7E;void*_T7F;struct Cyc_List_List*_T80;struct Cyc_Absyn_Exp*_T81;unsigned _T82;struct Cyc_Absyn_Exp*_T83;struct Cyc_Absyn_Exp*_T84;unsigned _T85;struct Cyc_Absyn_Exp*_T86;struct Cyc_Absyn_Exp*_T87;unsigned _T88;struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T89;void*_T8A;struct Cyc_Absyn_Exp*_T8B;unsigned _T8C;struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T8D;void*_T8E;struct Cyc_Absyn_Exp*_T8F;unsigned _T90;struct Cyc_Absyn_Exp*_T91;struct Cyc_Absyn_Exp*_T92;struct Cyc_Absyn_Exp*_T93;unsigned _T94;struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_T95;struct Cyc_List_List*(*_T96)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T97)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _T98;struct Cyc_List_List*_T99;void*_T9A;struct Cyc_Absyn_Exp*_T9B;unsigned _T9C;struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_T9D;struct Cyc_List_List*(*_T9E)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T9F)(void*(*)(void*,void*),void*,struct Cyc_List_List*);struct Cyc_Absyn_Exp*(*_TA0)(int,struct Cyc_Absyn_Exp*);int _TA1;struct Cyc_List_List*_TA2;void*_TA3;struct Cyc_Absyn_Exp*_TA4;unsigned _TA5;struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_TA6;void*_TA7;struct Cyc_Absyn_Exp*_TA8;unsigned _TA9;void*_TAA;struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_TAB;void*_TAC;struct Cyc_Absyn_Exp*_TAD;unsigned _TAE;struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_TAF;struct Cyc_List_List*_TB0;struct Cyc_List_List*(*_TB1)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_TB2)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _TB3;struct Cyc_List_List*_TB4;void*_TB5;struct Cyc_Absyn_Exp*_TB6;unsigned _TB7;void*_TB8;struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_TB9;struct Cyc_List_List*(*_TBA)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_TBB)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _TBC;struct Cyc_List_List*_TBD;void*_TBE;struct Cyc_Absyn_Exp*_TBF;unsigned _TC0;struct _tuple9*_TC1;struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_TC2;struct _tuple9*_TC3;struct Cyc_List_List*(*_TC4)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_TC5)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _TC6;struct Cyc_List_List*_TC7;void*_TC8;struct Cyc_Absyn_Exp*_TC9;unsigned _TCA;void*_TCB;struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_TCC;void*_TCD;struct Cyc_Absyn_Exp*_TCE;unsigned _TCF;struct Cyc_Absyn_MallocInfo _TD0;struct Cyc_Absyn_MallocInfo _TD1;struct Cyc_Absyn_MallocInfo _TD2;struct Cyc_Absyn_MallocInfo _TD3;struct Cyc_Absyn_MallocInfo _TD4;struct Cyc_Absyn_MallocInfo _TD5;struct Cyc_Absyn_MallocInfo _TD6;void**_TD7;void**_TD8;void*_TD9;struct Cyc_Absyn_Exp*_TDA;struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_TDB;struct Cyc_Absyn_Exp*_TDC;struct Cyc_Absyn_Exp*_TDD;struct Cyc_Absyn_Exp*_TDE;unsigned _TDF;struct Cyc_Core_Opt*_TE0;struct Cyc_Core_Opt*_TE1;void*_TE2;struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_TE3;struct Cyc_List_List*(*_TE4)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_TE5)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _TE6;struct Cyc_List_List*_TE7;void*_TE8;struct Cyc_Absyn_Exp*_TE9;unsigned _TEA;struct Cyc_Absyn_Stmt*_TEB;struct Cyc_Absyn_Exp*_TEC;unsigned _TED;struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_TEE;void*_TEF;struct Cyc_Absyn_Exp*_TF0;unsigned _TF1;void*_TF2;void*_TF3;struct Cyc_Absyn_Exp*_TF4;unsigned _TF5;int _TF6;struct _fat_ptr _TF7;struct Cyc_List_List*_TF8;struct Cyc_List_List*_TF9;struct Cyc_List_List*_TFA;struct Cyc_Absyn_Exp*_TFB;unsigned _TFC;struct Cyc_Absyn_Exp*_TFD;int _TFE;struct Cyc_Absyn_Exp*_TFF;unsigned _T100;struct Cyc_Absyn_Exp*_T101;struct Cyc_Absyn_Exp*_T102;unsigned _T103;int _T104;struct Cyc_Absyn_Exp*_T105;struct Cyc_Absyn_Exp*_T106;struct Cyc_Absyn_Exp*_T107;struct Cyc_Absyn_Exp*_T108;struct Cyc_Absyn_Exp*_T109;
struct Cyc_Absyn_Exp*(*DC)(int,struct Cyc_Absyn_Exp*)=Cyc_Tcutil_deep_copy_exp;
struct Cyc_Absyn_Exp*new_e;
int pt=preserve_types;_T0=e;{
void*_T10A=_T0->r;struct Cyc_List_List*_T10B;struct Cyc_List_List*_T10C;struct Cyc_Absyn_Exp*_T10D;void**_T10E;enum Cyc_Absyn_MallocKind _T10F;struct Cyc_Absyn_Enumfield*_T110;struct Cyc_Absyn_Aggrdecl*_T111;struct Cyc_Absyn_Datatypefield*_T112;struct Cyc_Absyn_Exp*_T113;enum Cyc_Absyn_Coercion _T114;struct Cyc_Absyn_VarargCallInfo*_T115;int _T116;int _T117;struct Cyc_Absyn_Tqual _T118;struct _fat_ptr*_T119;struct Cyc_List_List*_T11A;int _T11B;void*_T11C;void*_T11D;enum Cyc_Absyn_Incrementor _T11E;enum Cyc_Absyn_Primop _T11F;struct _fat_ptr _T120;void*_T121;union Cyc_Absyn_Cnst _T122;_T1=(int*)_T10A;_T2=*_T1;switch(_T2){case 0:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T10A;_T122=_T123->f1;}{union Cyc_Absyn_Cnst c=_T122;_T3=c;_T4=e;_T5=_T4->loc;
new_e=Cyc_Absyn_const_exp(_T3,_T5);goto _LL0;}case 1:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T10A;_T6=_T123->f1;_T121=(void*)_T6;}{void*b=_T121;_T7=b;_T8=e;_T9=_T8->loc;
new_e=Cyc_Absyn_varb_exp(_T7,_T9);goto _LL0;}case 2:{struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_T10A;_T120=_T123->f1;}{struct _fat_ptr p=_T120;_TA=p;_TB=e;_TC=_TB->loc;
new_e=Cyc_Absyn_pragma_exp(_TA,_TC);goto _LL0;}case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_T10A;_T11F=_T123->f1;_T121=_T123->f2;}{enum Cyc_Absyn_Primop p=_T11F;struct Cyc_List_List*es=_T121;_TD=p;_TF=Cyc_List_map_c;{
struct Cyc_List_List*(*_T123)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))_TF;_TE=_T123;}_T10=DC;_T11=pt;_T12=es;_T13=_TE(_T10,_T11,_T12);_T14=e;_T15=_T14->loc;new_e=Cyc_Absyn_primop_exp(_TD,_T13,_T15);goto _LL0;}case 5:{struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11E=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;enum Cyc_Absyn_Incrementor i=_T11E;_T16=
DC(pt,e1);_T17=i;_T18=e;_T19=_T18->loc;new_e=Cyc_Absyn_increment_exp(_T16,_T17,_T19);goto _LL0;}case 7:{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_Absyn_Exp*e2=_T11D;_T1A=
DC(pt,e1);_T1B=DC(pt,e2);_T1C=e;_T1D=_T1C->loc;new_e=Cyc_Absyn_and_exp(_T1A,_T1B,_T1D);goto _LL0;}case 8:{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_Absyn_Exp*e2=_T11D;_T1E=
DC(pt,e1);_T1F=DC(pt,e2);_T20=e;_T21=_T20->loc;new_e=Cyc_Absyn_or_exp(_T1E,_T1F,_T21);goto _LL0;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_Absyn_Exp*e2=_T11D;_T22=
DC(pt,e1);_T23=DC(pt,e2);_T24=e;_T25=_T24->loc;new_e=Cyc_Absyn_seq_exp(_T22,_T23,_T25);goto _LL0;}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T11C=_T123->f3;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_Absyn_Exp*e2=_T11D;struct Cyc_Absyn_Exp*e3=_T11C;_T26=
# 463
DC(pt,e1);_T27=DC(pt,e2);_T28=DC(pt,e3);_T29=e;_T2A=_T29->loc;new_e=Cyc_Absyn_conditional_exp(_T26,_T27,_T28,_T2A);goto _LL0;}case 4:{struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T11C=_T123->f3;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_Core_Opt*po=_T11D;struct Cyc_Absyn_Exp*e2=_T11C;_T2B=
# 465
DC(pt,e1);_T2D=po;_T2E=(unsigned long)_T2D;if(!_T2E)goto _TL87;{struct Cyc_Core_Opt*_T123=_cycalloc(sizeof(struct Cyc_Core_Opt));_T30=po;_T31=_T30->v;_T123->v=(void*)_T31;_T2F=(struct Cyc_Core_Opt*)_T123;}_T2C=_T2F;goto _TL88;_TL87: _T2C=0;_TL88: _T32=DC(pt,e2);_T33=e;_T34=_T33->loc;new_e=Cyc_Absyn_assignop_exp(_T2B,_T2C,_T32,_T34);goto _LL0;}case 10: _T35=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T10A;_T36=_T35->f3;if(_T36==0)goto _TL89;{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T37=_T123->f3;{struct Cyc_Absyn_VarargCallInfo _T124=*_T37;_T11B=_T124.num_varargs;_T11A=_T124.injectors;_T38=_T124.vai;{struct Cyc_Absyn_VarargInfo _T125=*_T38;_T119=_T125.name;_T118=_T125.tq;_T11C=_T125.type;_T117=_T125.inject;}}_T116=_T123->f4;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_List_List*es=_T11D;int n=_T11B;struct Cyc_List_List*is=_T11A;struct _fat_ptr*nm=_T119;struct Cyc_Absyn_Tqual tq=_T118;void*t=_T11C;int i=_T117;int resolved=_T116;{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct));_T123->tag=10;
# 469
_T123->f1=DC(pt,e1);_T3B=Cyc_List_map_c;{struct Cyc_List_List*(*_T124)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))_T3B;_T3A=_T124;}_T3C=DC;_T3D=pt;_T3E=es;_T123->f2=_T3A(_T3C,_T3D,_T3E);{struct Cyc_Absyn_VarargCallInfo*_T124=_cycalloc(sizeof(struct Cyc_Absyn_VarargCallInfo));
# 471
_T124->num_varargs=n;_T124->injectors=is;{struct Cyc_Absyn_VarargInfo*_T125=_cycalloc(sizeof(struct Cyc_Absyn_VarargInfo));_T125->name=nm;_T125->tq=tq;_T125->type=Cyc_Tcutil_copy_type(t);_T125->inject=i;_T40=(struct Cyc_Absyn_VarargInfo*)_T125;}_T124->vai=_T40;_T3F=(struct Cyc_Absyn_VarargCallInfo*)_T124;}
# 470
_T123->f3=_T3F;
_T123->f4=resolved;_T39=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T123;}_T41=(void*)_T39;_T42=e;_T43=_T42->loc;
# 468
new_e=Cyc_Absyn_new_exp(_T41,_T43);goto _LL0;}_TL89:{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T115=_T123->f3;_T11B=_T123->f4;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_List_List*es=_T11D;struct Cyc_Absyn_VarargCallInfo*vci=_T115;int resolved=_T11B;{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct));_T123->tag=10;
# 475
_T123->f1=DC(pt,e1);_T46=Cyc_List_map_c;{struct Cyc_List_List*(*_T124)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))_T46;_T45=_T124;}_T47=DC;_T48=pt;_T49=es;_T123->f2=_T45(_T47,_T48,_T49);_T123->f3=vci;_T123->f4=resolved;_T44=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T123;}_T4A=(void*)_T44;_T4B=e;_T4C=_T4B->loc;new_e=Cyc_Absyn_new_exp(_T4A,_T4C);goto _LL0;}case 11:{struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11B=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;int b=_T11B;_T4E=b;
# 478
if(!_T4E)goto _TL8B;_T4F=DC(pt,e1);_T50=e;_T51=_T50->loc;_T4D=Cyc_Absyn_rethrow_exp(_T4F,_T51);goto _TL8C;_TL8B: _T52=DC(pt,e1);_T53=e;_T54=_T53->loc;_T4D=Cyc_Absyn_throw_exp(_T52,_T54);_TL8C: new_e=_T4D;goto _LL0;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_Absyn_Exp*e1=_T121;_T55=
# 481
DC(pt,e1);_T56=e;_T57=_T56->loc;new_e=Cyc_Absyn_noinstantiate_exp(_T55,_T57);goto _LL0;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_List_List*ts=_T11D;_T58=
# 483
DC(pt,e1);_T59=ts;_T5A=Cyc_List_map(Cyc_Tcutil_copy_type,_T59);_T5B=e;_T5C=_T5B->loc;new_e=Cyc_Absyn_instantiate_exp(_T58,_T5A,_T5C);goto _LL0;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T10A;_T5D=_T123->f1;_T121=(void*)_T5D;_T11D=_T123->f2;_T11B=_T123->f3;_T114=_T123->f4;}{void*t=_T121;struct Cyc_Absyn_Exp*e1=_T11D;int b=_T11B;enum Cyc_Absyn_Coercion c=_T114;_T5E=
# 485
Cyc_Tcutil_copy_type(t);_T5F=DC(pt,e1);_T60=b;_T61=c;_T62=e;_T63=_T62->loc;new_e=Cyc_Absyn_cast_exp(_T5E,_T5F,_T60,_T61,_T63);goto _LL0;}case 16:{struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T113=_T123->f3;}{struct Cyc_Absyn_Exp*eo=_T121;struct Cyc_Absyn_Exp*e1=_T11D;struct Cyc_Absyn_Exp*qo=_T113;_T65=eo;_T66=(unsigned long)_T65;
# 487
if(!_T66)goto _TL8D;_T64=DC(pt,eo);goto _TL8E;_TL8D: _T64=0;_TL8E: _T67=DC(pt,e1);_T69=qo;_T6A=(unsigned long)_T69;if(!_T6A)goto _TL8F;_T68=DC(pt,qo);goto _TL90;_TL8F: _T68=0;_TL90: _T6B=e;_T6C=_T6B->loc;new_e=Cyc_Absyn_New_exp(_T64,_T67,_T68,_T6C);goto _LL0;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_Absyn_Exp*e1=_T121;_T6D=
DC(pt,e1);_T6E=e;_T6F=_T6E->loc;new_e=Cyc_Absyn_address_exp(_T6D,_T6F);goto _LL0;}case 17:{struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_T10A;_T70=_T123->f1;_T121=(void*)_T70;}{void*t=_T121;_T71=
Cyc_Tcutil_copy_type(t);_T72=e;_T73=_T72->loc;new_e=Cyc_Absyn_sizeoftype_exp(_T71,_T73);goto _LL0;}case 18:{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_Absyn_Exp*e1=_T121;_T74=
DC(pt,e1);_T75=e;_T76=_T75->loc;new_e=Cyc_Absyn_sizeofexp_exp(_T74,_T76);goto _LL0;}case 19:{struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*)_T10A;_T77=_T123->f1;_T121=(void*)_T77;}{void*t=_T121;_T78=
Cyc_Tcutil_copy_type(t);_T79=e;_T7A=_T79->loc;new_e=Cyc_Absyn_alignoftype_exp(_T78,_T7A);goto _LL0;}case 20:{struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_Absyn_Exp*e1=_T121;_T7B=
DC(pt,e1);_T7C=e;_T7D=_T7C->loc;new_e=Cyc_Absyn_alignofexp_exp(_T7B,_T7D);goto _LL0;}case 21:{struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_T10A;_T7E=_T123->f1;_T121=(void*)_T7E;_T11D=_T123->f2;}{void*t=_T121;struct Cyc_List_List*ofs=_T11D;_T7F=
Cyc_Tcutil_copy_type(t);_T80=ofs;_T81=e;_T82=_T81->loc;new_e=Cyc_Absyn_offsetof_exp(_T7F,_T80,_T82);goto _LL0;}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_Absyn_Exp*e1=_T121;_T83=
DC(pt,e1);_T84=e;_T85=_T84->loc;new_e=Cyc_Absyn_deref_exp(_T83,_T85);goto _LL0;}case 42:{struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_Absyn_Exp*e1=_T121;_T86=
DC(pt,e1);_T87=e;_T88=_T87->loc;new_e=Cyc_Absyn_extension_exp(_T86,_T88);goto _LL0;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T11B=_T123->f3;_T117=_T123->f4;}{struct Cyc_Absyn_Exp*e1=_T121;struct _fat_ptr*n=_T11D;int f1=_T11B;int f2=_T117;{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct));_T123->tag=23;
# 497
_T123->f1=DC(pt,e1);_T123->f2=n;_T123->f3=f1;_T123->f4=f2;_T89=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T123;}_T8A=(void*)_T89;_T8B=e;_T8C=_T8B->loc;new_e=Cyc_Absyn_new_exp(_T8A,_T8C);goto _LL0;}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T11B=_T123->f3;_T117=_T123->f4;}{struct Cyc_Absyn_Exp*e1=_T121;struct _fat_ptr*n=_T11D;int f1=_T11B;int f2=_T117;{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct));_T123->tag=24;
# 499
_T123->f1=DC(pt,e1);_T123->f2=n;_T123->f3=f1;_T123->f4=f2;_T8D=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T123;}_T8E=(void*)_T8D;_T8F=e;_T90=_T8F->loc;new_e=Cyc_Absyn_new_exp(_T8E,_T90);goto _LL0;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_Absyn_Exp*e2=_T11D;_T91=
# 501
DC(pt,e1);_T92=DC(pt,e2);_T93=e;_T94=_T93->loc;new_e=Cyc_Absyn_subscript_exp(_T91,_T92,_T94);goto _LL0;}case 27:{struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_List_List*eds=_T121;{struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct));_T123->tag=27;_T97=Cyc_List_map_c;{
# 503
struct Cyc_List_List*(*_T124)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*))_T97;_T96=_T124;}_T98=pt;_T99=eds;_T123->f1=_T96(Cyc_Tcutil_copy_eds,_T98,_T99);_T95=(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_T123;}_T9A=(void*)_T95;_T9B=e;_T9C=_T9B->loc;new_e=Cyc_Absyn_new_exp(_T9A,_T9C);goto _LL0;}case 32:{struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T112=_T123->f3;}{struct Cyc_List_List*es=_T121;struct Cyc_Absyn_Datatypedecl*dtd=_T11D;struct Cyc_Absyn_Datatypefield*dtf=_T112;{struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct));_T123->tag=32;_T9F=Cyc_List_map_c;{
# 505
struct Cyc_List_List*(*_T124)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))_T9F;_T9E=_T124;}_TA0=DC;_TA1=pt;_TA2=es;_T123->f1=_T9E(_TA0,_TA1,_TA2);_T123->f2=dtd;_T123->f3=dtf;_T9D=(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_T123;}_TA3=(void*)_T9D;_TA4=e;_TA5=_TA4->loc;new_e=Cyc_Absyn_new_exp(_TA3,_TA5);goto _LL0;}case 28:{struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T113=_T123->f3;_T11B=_T123->f4;}{struct Cyc_Absyn_Vardecl*vd=_T121;struct Cyc_Absyn_Exp*e1=_T11D;struct Cyc_Absyn_Exp*e2=_T113;int b=_T11B;{struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct));_T123->tag=28;
# 507
_T123->f1=vd;_T123->f2=DC(pt,e1);_T123->f3=DC(pt,e2);_T123->f4=b;_TA6=(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_T123;}_TA7=(void*)_TA6;_TA8=e;_TA9=_TA8->loc;new_e=Cyc_Absyn_new_exp(_TA7,_TA9);goto _LL0;}case 29:{struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_TAA=_T123->f2;_T11D=(void*)_TAA;_T11B=_T123->f3;}{struct Cyc_Absyn_Exp*e=_T121;void*t=_T11D;int b=_T11B;{struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct));_T123->tag=29;
# 509
_T123->f1=DC(pt,e);_T123->f2=Cyc_Tcutil_copy_type(t);_T123->f3=b;_TAB=(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_T123;}_TAC=(void*)_TAB;_TAD=e;_TAE=_TAD->loc;new_e=Cyc_Absyn_new_exp(_TAC,_TAE);goto _LL0;}case 30:{struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11D=_T123->f2;_T11A=_T123->f3;_T111=_T123->f4;}{struct _tuple1*n=_T121;struct Cyc_List_List*ts=_T11D;struct Cyc_List_List*eds=_T11A;struct Cyc_Absyn_Aggrdecl*agr=_T111;{struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct));_T123->tag=30;
# 512
_T123->f1=n;_TB0=ts;_T123->f2=Cyc_List_map(Cyc_Tcutil_copy_type,_TB0);_TB2=Cyc_List_map_c;{struct Cyc_List_List*(*_T124)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*))_TB2;_TB1=_T124;}_TB3=pt;_TB4=eds;_T123->f3=_TB1(Cyc_Tcutil_copy_eds,_TB3,_TB4);_T123->f4=agr;_TAF=(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_T123;}_TB5=(void*)_TAF;_TB6=e;_TB7=_TB6->loc;new_e=Cyc_Absyn_new_exp(_TB5,_TB7);goto _LL0;}case 31:{struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_T10A;_TB8=_T123->f1;_T121=(void*)_TB8;_T11B=_T123->f2;_T11D=_T123->f3;}{void*topt=_T121;int b=_T11B;struct Cyc_List_List*eds=_T11D;
# 516
void*new_topt=0;
if(topt==0)goto _TL91;
new_topt=Cyc_Tcutil_copy_type(topt);goto _TL92;_TL91: _TL92:{struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct));_T123->tag=31;
# 520
_T123->f1=new_topt;_T123->f2=b;_TBB=Cyc_List_map_c;{struct Cyc_List_List*(*_T124)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*))_TBB;_TBA=_T124;}_TBC=pt;_TBD=eds;_T123->f3=_TBA(Cyc_Tcutil_copy_eds,_TBC,_TBD);_TB9=(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_T123;}_TBE=(void*)_TB9;_TBF=e;_TC0=_TBF->loc;new_e=Cyc_Absyn_new_exp(_TBE,_TC0);goto _LL0;}case 26:{struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_T10A;_TC1=_T123->f1;{struct _tuple9 _T124=*_TC1;_T121=_T124.f0;_T118=_T124.f1;_T11D=_T124.f2;}_T11A=_T123->f2;}{struct _fat_ptr*vopt=_T121;struct Cyc_Absyn_Tqual tq=_T118;void*t=_T11D;struct Cyc_List_List*eds=_T11A;{struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct));_T123->tag=26;{struct _tuple9*_T124=_cycalloc(sizeof(struct _tuple9));
# 523
_T124->f0=vopt;_T124->f1=tq;_T124->f2=Cyc_Tcutil_copy_type(t);_TC3=(struct _tuple9*)_T124;}_T123->f1=_TC3;_TC5=Cyc_List_map_c;{
struct Cyc_List_List*(*_T124)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*))_TC5;_TC4=_T124;}_TC6=pt;_TC7=eds;_T123->f2=_TC4(Cyc_Tcutil_copy_eds,_TC6,_TC7);_TC2=(struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_T123;}_TC8=(void*)_TC2;_TC9=e;_TCA=_TC9->loc;
# 523
new_e=Cyc_Absyn_new_exp(_TC8,_TCA);goto _LL0;}case 33:{struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T110=_T123->f2;}{struct Cyc_Absyn_Enumdecl*ed=_T121;struct Cyc_Absyn_Enumfield*ef=_T110;
# 526
new_e=e;goto _LL0;}case 34:{struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_T10A;_TCB=_T123->f1;_T121=(void*)_TCB;_T110=_T123->f2;}{void*t=_T121;struct Cyc_Absyn_Enumfield*ef=_T110;{struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct));_T123->tag=34;
# 528
_T123->f1=Cyc_Tcutil_copy_type(t);_T123->f2=ef;_TCC=(struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_T123;}_TCD=(void*)_TCC;_TCE=e;_TCF=_TCE->loc;new_e=Cyc_Absyn_new_exp(_TCD,_TCF);goto _LL0;}case 35:{struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_T10A;_TD0=_T123->f1;_T10F=_TD0.mknd;_TD1=_T123->f1;_T121=_TD1.rgn;_TD2=_T123->f1;_T113=_TD2.aqual;_TD3=_T123->f1;_T10E=_TD3.elt_type;_TD4=_T123->f1;_T10D=_TD4.num_elts;_TD5=_T123->f1;_T11B=_TD5.fat_result;_TD6=_T123->f1;_T117=_TD6.inline_call;}{enum Cyc_Absyn_MallocKind mk=_T10F;struct Cyc_Absyn_Exp*r=_T121;struct Cyc_Absyn_Exp*q=_T113;void**t=_T10E;struct Cyc_Absyn_Exp*n=_T10D;int res=_T11B;int inlc=_T117;
# 530
struct Cyc_Absyn_Exp*e2=Cyc_Absyn_copy_exp(e);
struct Cyc_Absyn_Exp*r1=r;if(r==0)goto _TL93;r1=DC(pt,r);goto _TL94;_TL93: _TL94: {
struct Cyc_Absyn_Exp*q1=q;if(q==0)goto _TL95;q1=DC(pt,q);goto _TL96;_TL95: _TL96: {
void**t1=t;if(t==0)goto _TL97;{void**_T123=_cycalloc(sizeof(void*));_TD8=t;_TD9=*_TD8;*_T123=Cyc_Tcutil_copy_type(_TD9);_TD7=(void**)_T123;}t1=_TD7;goto _TL98;_TL97: _TL98: _TDA=e2;{struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct));_T123->tag=35;
_T123->f1.mknd=mk;_T123->f1.rgn=r1;_T123->f1.aqual=q1;_T123->f1.elt_type=t1;_T123->f1.num_elts=n;_T123->f1.fat_result=res;_T123->f1.inline_call=inlc;_TDB=(struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_T123;}_TDA->r=(void*)_TDB;
new_e=e2;goto _LL0;}}}case 36:{struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T113=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;struct Cyc_Absyn_Exp*e2=_T113;_TDC=
# 537
DC(pt,e1);_TDD=DC(pt,e2);_TDE=e;_TDF=_TDE->loc;new_e=Cyc_Absyn_swap_exp(_TDC,_TDD,_TDF);goto _LL0;}case 37:{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T11A=_T123->f2;}{struct Cyc_Core_Opt*nopt=_T121;struct Cyc_List_List*eds=_T11A;
# 539
struct Cyc_Core_Opt*nopt1=nopt;
if(nopt==0)goto _TL99;{struct Cyc_Core_Opt*_T123=_cycalloc(sizeof(struct Cyc_Core_Opt));_TE1=nopt;_TE2=_TE1->v;_T123->v=(struct _tuple1*)_TE2;_TE0=(struct Cyc_Core_Opt*)_T123;}nopt1=_TE0;goto _TL9A;_TL99: _TL9A:{struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct));_T123->tag=37;
_T123->f1=nopt1;_TE5=Cyc_List_map_c;{struct Cyc_List_List*(*_T124)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple17*(*)(int,struct _tuple17*),int,struct Cyc_List_List*))_TE5;_TE4=_T124;}_TE6=pt;_TE7=eds;_T123->f2=_TE4(Cyc_Tcutil_copy_eds,_TE6,_TE7);_TE3=(struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_T123;}_TE8=(void*)_TE3;_TE9=e;_TEA=_TE9->loc;new_e=Cyc_Absyn_new_exp(_TE8,_TEA);goto _LL0;}case 38:{struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;}{struct Cyc_Absyn_Stmt*s=_T121;_TEB=
# 544
Cyc_Tcutil_deep_copy_stmt(pt,s);_TEC=e;_TED=_TEC->loc;new_e=Cyc_Absyn_stmt_exp(_TEB,_TED);goto _LL0;}case 39:{struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_T10A;_T121=_T123->f1;_T119=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T121;struct _fat_ptr*fn=_T119;{struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_T123=_cycalloc(sizeof(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct));_T123->tag=39;
# 547
_T123->f1=DC(pt,e1);_T123->f2=fn;_TEE=(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_T123;}_TEF=(void*)_TEE;_TF0=e;_TF1=_TF0->loc;new_e=Cyc_Absyn_new_exp(_TEF,_TF1);goto _LL0;}case 40:{struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_T10A;_TF2=_T123->f1;_T121=(void*)_TF2;}{void*t=_T121;_TF3=
Cyc_Tcutil_copy_type(t);_TF4=e;_TF5=_TF4->loc;new_e=Cyc_Absyn_valueof_exp(_TF3,_TF5);goto _LL0;}case 41:{struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_T10A;_T11B=_T123->f1;_T120=_T123->f2;_T11A=_T123->f3;_T10C=_T123->f4;_T10B=_T123->f5;}{int v=_T11B;struct _fat_ptr t=_T120;struct Cyc_List_List*o=_T11A;struct Cyc_List_List*i=_T10C;struct Cyc_List_List*c=_T10B;_TF6=v;_TF7=t;_TF8=o;_TF9=i;_TFA=c;_TFB=e;_TFC=_TFB->loc;
new_e=Cyc_Absyn_asm_exp(_TF6,_TF7,_TF8,_TF9,_TFA,_TFC);goto _LL0;}case 43:{struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_T10A;_T113=_T123->f1;_T11B=_T123->f2;}{struct Cyc_Absyn_Exp*e1=_T113;int s=_T11B;_TFD=e1;_TFE=s;_TFF=e;_T100=_TFF->loc;
new_e=Cyc_Absyn_assert_exp(_TFD,_TFE,_T100);goto _LL0;}default:{struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*_T123=(struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*)_T10A;_T113=_T123->f1;}{struct Cyc_Absyn_Exp*e1=_T113;_T101=e1;_T102=e;_T103=_T102->loc;
new_e=Cyc_Absyn_assert_false_exp(_T101,_T103);goto _LL0;}}_LL0:;}_T104=preserve_types;
# 554
if(!_T104)goto _TL9B;_T105=new_e;_T106=e;
_T105->topt=_T106->topt;_T107=new_e;_T108=e;
_T107->annot=_T108->annot;goto _TL9C;_TL9B: _TL9C: _T109=new_e;
# 558
return _T109;}
# 561
static struct Cyc_Absyn_Pat*Cyc_Tcutil_deep_copy_pattern(int preserve_types,struct Cyc_Absyn_Pat*p){struct Cyc_Absyn_Pat*_T0;struct Cyc_Absyn_Pat*_T1;struct Cyc_Absyn_Pat*_T2;
struct Cyc_Absyn_Pat*new_p;new_p=_cycalloc(sizeof(struct Cyc_Absyn_Pat));_T0=new_p;_T1=p;*_T0=*_T1;_T2=new_p;
# 564
return _T2;}
# 567
static struct Cyc_Absyn_Switch_clause*Cyc_Tcutil_deep_copy_switch_clause(int preserve_types,struct Cyc_Absyn_Switch_clause*sc){struct Cyc_Absyn_Switch_clause*_T0;struct Cyc_Absyn_Switch_clause*_T1;struct Cyc_Absyn_Switch_clause*_T2;int _T3;struct Cyc_Absyn_Switch_clause*_T4;struct Cyc_Absyn_Pat*_T5;struct Cyc_Absyn_Switch_clause*_T6;struct Cyc_Absyn_Exp*_T7;struct Cyc_Absyn_Switch_clause*_T8;int _T9;struct Cyc_Absyn_Switch_clause*_TA;struct Cyc_Absyn_Exp*_TB;struct Cyc_Absyn_Switch_clause*_TC;int _TD;struct Cyc_Absyn_Switch_clause*_TE;struct Cyc_Absyn_Stmt*_TF;struct Cyc_Absyn_Switch_clause*_T10;
# 569
struct Cyc_Absyn_Switch_clause*new_sc;new_sc=_cycalloc(sizeof(struct Cyc_Absyn_Switch_clause));_T0=new_sc;_T1=sc;*_T0=*_T1;_T2=new_sc;_T3=preserve_types;_T4=new_sc;_T5=_T4->pattern;
_T2->pattern=Cyc_Tcutil_deep_copy_pattern(_T3,_T5);_T6=new_sc;_T7=_T6->where_clause;
if(_T7==0)goto _TL9D;_T8=new_sc;_T9=preserve_types;_TA=new_sc;_TB=_TA->where_clause;
_T8->where_clause=Cyc_Tcutil_deep_copy_exp(_T9,_TB);goto _TL9E;_TL9D: _TL9E: _TC=new_sc;_TD=preserve_types;_TE=new_sc;_TF=_TE->body;
# 575
_TC->body=Cyc_Tcutil_deep_copy_stmt(_TD,_TF);_T10=new_sc;
return _T10;}
# 579
static struct Cyc_Absyn_Decl*Cyc_Tcutil_deep_copy_decl(int preserve_types,struct Cyc_Absyn_Decl*d){struct Cyc_Absyn_Decl*_T0;struct Cyc_Absyn_Decl*_T1;struct Cyc_Absyn_Decl*_T2;
# 581
struct Cyc_Absyn_Decl*new_d;new_d=_cycalloc(sizeof(struct Cyc_Absyn_Decl));_T0=new_d;_T1=d;*_T0=*_T1;_T2=new_d;
return _T2;}
# 585
struct Cyc_Absyn_Stmt*Cyc_Tcutil_deep_copy_stmt(int preserve_types,struct Cyc_Absyn_Stmt*s){struct Cyc_Absyn_Stmt*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Stmt*_T3;unsigned _T4;struct Cyc_Absyn_Exp*_T5;struct Cyc_Absyn_Stmt*_T6;unsigned _T7;struct Cyc_Absyn_Stmt*_T8;struct Cyc_Absyn_Stmt*_T9;struct Cyc_Absyn_Stmt*_TA;unsigned _TB;struct Cyc_Absyn_Stmt*_TC;unsigned _TD;struct Cyc_Absyn_Exp*_TE;struct Cyc_Absyn_Stmt*_TF;unsigned _T10;struct Cyc_Absyn_Exp*_T11;struct Cyc_Absyn_Stmt*_T12;struct Cyc_Absyn_Stmt*_T13;struct Cyc_Absyn_Stmt*_T14;unsigned _T15;struct _tuple10 _T16;struct Cyc_Absyn_Exp*_T17;struct Cyc_Absyn_Stmt*_T18;struct Cyc_Absyn_Stmt*_T19;unsigned _T1A;struct Cyc_Absyn_Stmt*_T1B;unsigned _T1C;struct Cyc_Absyn_Stmt*_T1D;unsigned _T1E;struct _fat_ptr*_T1F;struct Cyc_Absyn_Stmt*_T20;unsigned _T21;struct _tuple10 _T22;struct _tuple10 _T23;struct Cyc_Absyn_Exp*_T24;struct Cyc_Absyn_Exp*_T25;struct Cyc_Absyn_Exp*_T26;struct Cyc_Absyn_Stmt*_T27;struct Cyc_Absyn_Stmt*_T28;unsigned _T29;void*_T2A;struct Cyc_Absyn_Exp*_T2B;struct Cyc_List_List*(*_T2C)(struct Cyc_Absyn_Switch_clause*(*)(int,struct Cyc_Absyn_Switch_clause*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T2D)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _T2E;struct Cyc_List_List*_T2F;struct Cyc_List_List*_T30;struct Cyc_Absyn_Stmt*_T31;unsigned _T32;struct Cyc_List_List*(*_T33)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T34)(void*(*)(void*,void*),void*,struct Cyc_List_List*);struct Cyc_Absyn_Exp*(*_T35)(int,struct Cyc_Absyn_Exp*);int _T36;struct Cyc_List_List*_T37;struct Cyc_List_List*_T38;struct Cyc_Absyn_Stmt*_T39;unsigned _T3A;struct Cyc_Absyn_Decl*_T3B;struct Cyc_Absyn_Stmt*_T3C;struct Cyc_Absyn_Stmt*_T3D;unsigned _T3E;struct _fat_ptr*_T3F;struct Cyc_Absyn_Stmt*_T40;struct Cyc_Absyn_Stmt*_T41;unsigned _T42;struct _tuple10 _T43;struct Cyc_Absyn_Stmt*_T44;struct Cyc_Absyn_Exp*_T45;struct Cyc_Absyn_Stmt*_T46;unsigned _T47;void*_T48;struct Cyc_Absyn_Stmt*_T49;struct Cyc_List_List*(*_T4A)(struct Cyc_Absyn_Switch_clause*(*)(int,struct Cyc_Absyn_Switch_clause*),int,struct Cyc_List_List*);struct Cyc_List_List*(*_T4B)(void*(*)(void*,void*),void*,struct Cyc_List_List*);int _T4C;struct Cyc_List_List*_T4D;struct Cyc_List_List*_T4E;struct Cyc_Absyn_Stmt*_T4F;unsigned _T50;struct Cyc_Absyn_Stmt*_T51;
struct Cyc_Absyn_Stmt*(*DS)(int,struct Cyc_Absyn_Stmt*)=Cyc_Tcutil_deep_copy_stmt;
struct Cyc_Absyn_Exp*(*DE)(int,struct Cyc_Absyn_Exp*)=Cyc_Tcutil_deep_copy_exp;
int pt=preserve_types;
struct Cyc_Absyn_Stmt*new_s;_T0=s;{
void*_T52=_T0->r;struct Cyc_Absyn_Switch_clause**_T53;struct Cyc_List_List*_T54;struct Cyc_Absyn_Exp*_T55;struct Cyc_Absyn_Stmt*_T56;void*_T57;void*_T58;_T1=(int*)_T52;_T2=*_T1;switch(_T2){case 0: _T3=s;_T4=_T3->loc;
new_s=Cyc_Absyn_skip_stmt(_T4);goto _LL0;case 1:{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;}{struct Cyc_Absyn_Exp*e=_T58;_T5=
DE(pt,e);_T6=s;_T7=_T6->loc;new_s=Cyc_Absyn_exp_stmt(_T5,_T7);goto _LL0;}case 2:{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;_T57=_T59->f2;}{struct Cyc_Absyn_Stmt*s1=_T58;struct Cyc_Absyn_Stmt*s2=_T57;_T8=
DS(pt,s1);_T9=DS(pt,s2);_TA=s;_TB=_TA->loc;new_s=Cyc_Absyn_seq_stmt(_T8,_T9,_TB);goto _LL0;}case 3:{struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;}{struct Cyc_Absyn_Exp*eopt=_T58;
# 595
if(eopt!=0)goto _TLA0;_TC=s;_TD=_TC->loc;
new_s=Cyc_Absyn_return_stmt(0,_TD);goto _TLA1;
# 598
_TLA0: _TE=DE(pt,eopt);_TF=s;_T10=_TF->loc;new_s=Cyc_Absyn_return_stmt(_TE,_T10);_TLA1: goto _LL0;}case 4:{struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;_T57=_T59->f2;_T56=_T59->f3;}{struct Cyc_Absyn_Exp*e=_T58;struct Cyc_Absyn_Stmt*s1=_T57;struct Cyc_Absyn_Stmt*s2=_T56;_T11=
# 601
DE(pt,e);_T12=DS(pt,s1);_T13=DS(pt,s2);_T14=s;_T15=_T14->loc;new_s=Cyc_Absyn_ifthenelse_stmt(_T11,_T12,_T13,_T15);goto _LL0;}case 5:{struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_T52;_T16=_T59->f1;_T58=_T16.f0;_T57=_T59->f2;}{struct Cyc_Absyn_Exp*e1=_T58;struct Cyc_Absyn_Stmt*s2=_T57;_T17=
# 604
DE(pt,e1);_T18=DS(pt,s2);_T19=s;_T1A=_T19->loc;new_s=Cyc_Absyn_while_stmt(_T17,_T18,_T1A);goto _LL0;}case 6: _T1B=s;_T1C=_T1B->loc;
new_s=Cyc_Absyn_break_stmt(_T1C);goto _LL0;case 7: _T1D=s;_T1E=_T1D->loc;
new_s=Cyc_Absyn_continue_stmt(_T1E);goto _LL0;case 8:{struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;}{struct _fat_ptr*x=_T58;_T1F=x;_T20=s;_T21=_T20->loc;
new_s=Cyc_Absyn_goto_stmt(_T1F,_T21);goto _LL0;}case 9:{struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;_T22=_T59->f2;_T57=_T22.f0;_T23=_T59->f3;_T55=_T23.f0;_T56=_T59->f4;}{struct Cyc_Absyn_Exp*e1=_T58;struct Cyc_Absyn_Exp*e2=_T57;struct Cyc_Absyn_Exp*e3=_T55;struct Cyc_Absyn_Stmt*s4=_T56;_T24=
# 609
DE(pt,e1);_T25=DE(pt,e2);_T26=DE(pt,e3);_T27=DS(pt,s4);_T28=s;_T29=_T28->loc;new_s=Cyc_Absyn_for_stmt(_T24,_T25,_T26,_T27,_T29);goto _LL0;}case 10:{struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;_T54=_T59->f2;_T2A=_T59->f3;_T57=(void*)_T2A;}{struct Cyc_Absyn_Exp*e=_T58;struct Cyc_List_List*scs=_T54;void*tdopt=_T57;_T2B=
# 612
DE(pt,e);_T2D=Cyc_List_map_c;{struct Cyc_List_List*(*_T59)(struct Cyc_Absyn_Switch_clause*(*)(int,struct Cyc_Absyn_Switch_clause*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Switch_clause*(*)(int,struct Cyc_Absyn_Switch_clause*),int,struct Cyc_List_List*))_T2D;_T2C=_T59;}_T2E=pt;_T2F=scs;_T30=_T2C(Cyc_Tcutil_deep_copy_switch_clause,_T2E,_T2F);_T31=s;_T32=_T31->loc;new_s=Cyc_Absyn_switch_stmt(_T2B,_T30,_T32);goto _LL0;}case 11:{struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;_T53=_T59->f2;}{struct Cyc_List_List*es=_T58;struct Cyc_Absyn_Switch_clause**scp=_T53;_T34=Cyc_List_map_c;{
# 615
struct Cyc_List_List*(*_T59)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*)(int,struct Cyc_Absyn_Exp*),int,struct Cyc_List_List*))_T34;_T33=_T59;}_T35=DE;_T36=pt;_T37=es;_T38=_T33(_T35,_T36,_T37);_T39=s;_T3A=_T39->loc;new_s=Cyc_Absyn_fallthru_stmt(_T38,_T3A);goto _LL0;}case 12:{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;_T56=_T59->f2;}{struct Cyc_Absyn_Decl*d=_T58;struct Cyc_Absyn_Stmt*s1=_T56;_T3B=
# 618
Cyc_Tcutil_deep_copy_decl(pt,d);_T3C=DS(pt,s1);_T3D=s;_T3E=_T3D->loc;new_s=Cyc_Absyn_decl_stmt(_T3B,_T3C,_T3E);goto _LL0;}case 13:{struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_T52;_T58=_T59->f1;_T56=_T59->f2;}{struct _fat_ptr*x=_T58;struct Cyc_Absyn_Stmt*s1=_T56;_T3F=x;_T40=
# 621
DS(pt,s1);_T41=s;_T42=_T41->loc;new_s=Cyc_Absyn_label_stmt(_T3F,_T40,_T42);goto _LL0;}case 14:{struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_T52;_T56=_T59->f1;_T43=_T59->f2;_T58=_T43.f0;}{struct Cyc_Absyn_Stmt*s1=_T56;struct Cyc_Absyn_Exp*e=_T58;_T44=
# 624
DS(pt,s1);_T45=DE(pt,e);_T46=s;_T47=_T46->loc;new_s=Cyc_Absyn_do_stmt(_T44,_T45,_T47);goto _LL0;}default:{struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*_T59=(struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_T52;_T56=_T59->f1;_T54=_T59->f2;_T48=_T59->f3;_T58=(void*)_T48;}{struct Cyc_Absyn_Stmt*s1=_T56;struct Cyc_List_List*scs=_T54;void*tdopt=_T58;_T49=
# 627
DS(pt,s1);_T4B=Cyc_List_map_c;{struct Cyc_List_List*(*_T59)(struct Cyc_Absyn_Switch_clause*(*)(int,struct Cyc_Absyn_Switch_clause*),int,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Switch_clause*(*)(int,struct Cyc_Absyn_Switch_clause*),int,struct Cyc_List_List*))_T4B;_T4A=_T59;}_T4C=pt;_T4D=scs;_T4E=_T4A(Cyc_Tcutil_deep_copy_switch_clause,_T4C,_T4D);_T4F=s;_T50=_T4F->loc;new_s=Cyc_Absyn_trycatch_stmt(_T49,_T4E,_T50);goto _LL0;}}_LL0:;}_T51=new_s;
# 631
return _T51;}struct _tuple18{struct Cyc_Absyn_Tvar*f0;void*f1;};
# 636
static struct Cyc_Absyn_Kind*Cyc_Tcutil_field_kind(void*field_type,struct Cyc_List_List*ts,struct Cyc_List_List*tvs){struct Cyc_Absyn_Kind*_T0;struct Cyc_Absyn_Kind*_T1;struct Cyc_Absyn_Kind*_T2;struct Cyc_List_List*_T3;void*_T4;struct Cyc_List_List*_T5;struct Cyc_Absyn_Tvar*_T6;struct Cyc_Absyn_Kind*_T7;struct Cyc_Absyn_Kind*_T8;struct Cyc_Absyn_Kind*_T9;int _TA;struct Cyc_List_List*_TB;struct _RegionHandle*_TC;struct _tuple18*_TD;struct _RegionHandle*_TE;struct Cyc_List_List*_TF;struct Cyc_List_List*_T10;struct _RegionHandle*_T11;struct Cyc_List_List*_T12;void*_T13;struct Cyc_Absyn_Kind*_T14;
# 638
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_type_kind(field_type);
if(ts==0)goto _TLA2;_T0=k;_T1=& Cyc_Kinds_ak;_T2=(struct Cyc_Absyn_Kind*)_T1;if(_T0!=_T2)goto _TLA2;{struct _RegionHandle _T15=_new_region(0U,"r");struct _RegionHandle*r=& _T15;_push_region(r);{
# 643
struct Cyc_List_List*inst=0;
# 645
_TLA7: if(tvs!=0)goto _TLA5;else{goto _TLA6;}
_TLA5: _T3=tvs;_T4=_T3->hd;{struct Cyc_Absyn_Tvar*tv=(struct Cyc_Absyn_Tvar*)_T4;_T5=ts;{
void*t=_T5->hd;_T6=tv;_T7=& Cyc_Kinds_bk;_T8=(struct Cyc_Absyn_Kind*)_T7;_T9=
Cyc_Kinds_tvar_kind(_T6,_T8);{enum Cyc_Absyn_KindQual _T16=_T9->kind;_TA=(int)_T16;switch(_TA){case Cyc_Absyn_IntKind: goto _LL4;case Cyc_Absyn_AnyKind: _LL4: _TC=r;{struct Cyc_List_List*_T17=_region_malloc(_TC,0U,sizeof(struct Cyc_List_List));_TE=r;{struct _tuple18*_T18=_region_malloc(_TE,0U,sizeof(struct _tuple18));
# 650
_T18->f0=tv;_T18->f1=t;_TD=(struct _tuple18*)_T18;}_T17->hd=_TD;_T17->tl=inst;_TB=(struct Cyc_List_List*)_T17;}inst=_TB;goto _LL0;default: goto _LL0;}_LL0:;}}}_TF=tvs;
# 645
tvs=_TF->tl;_T10=ts;ts=_T10->tl;goto _TLA7;_TLA6:
# 654
 if(inst==0)goto _TLA9;_T11=r;_T12=
Cyc_List_imp_rev(inst);_T13=field_type;field_type=Cyc_Tcutil_rsubstitute(_T11,_T12,_T13);
k=Cyc_Tcutil_type_kind(field_type);goto _TLAA;_TLA9: _TLAA:;}_pop_region();}goto _TLA3;_TLA2: _TLA3: _T14=k;
# 659
return _T14;}
# 666
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*t){int*_T0;unsigned _T1;struct Cyc_Core_Opt*_T2;void*_T3;struct Cyc_Absyn_Kind*_T4;struct Cyc_Absyn_Tvar*_T5;struct Cyc_Absyn_Kind*_T6;struct Cyc_Absyn_Kind*_T7;struct Cyc_Absyn_Kind*_T8;struct Cyc_Absyn_Vardecl*_T9;void*_TA;struct Cyc_Absyn_Kind*_TB;void*_TC;void*_TD;int*_TE;unsigned _TF;struct Cyc_Absyn_Kind*_T10;struct Cyc_Absyn_Kind*_T11;void*_T12;struct Cyc_Absyn_Kind*_T13;enum Cyc_Absyn_Size_of _T14;int _T15;enum Cyc_Absyn_Size_of _T16;int _T17;struct Cyc_Absyn_Kind*_T18;struct Cyc_Absyn_Kind*_T19;struct Cyc_Absyn_Kind*_T1A;struct Cyc_Absyn_Kind*_T1B;struct Cyc_Absyn_Kind*_T1C;struct Cyc_Absyn_Kind*_T1D;struct Cyc_Absyn_Kind*_T1E;struct Cyc_Absyn_Kind*_T1F;struct Cyc_Absyn_Kind*_T20;struct Cyc_Absyn_Kind*_T21;void*_T22;struct Cyc_Absyn_Kind*_T23;struct Cyc_Absyn_Kind*_T24;struct Cyc_Absyn_Kind*_T25;struct Cyc_Absyn_Kind*_T26;struct Cyc_Absyn_Kind*_T27;struct Cyc_Absyn_Kind*_T28;struct Cyc_Absyn_Kind*_T29;struct Cyc_Absyn_Kind*_T2A;struct Cyc_Absyn_Kind*_T2B;struct Cyc_Absyn_Kind*_T2C;struct Cyc_Absyn_Kind*_T2D;struct Cyc_Absyn_Kind*_T2E;struct Cyc_Absyn_Kind*_T2F;void*_T30;struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*_T31;union Cyc_Absyn_DatatypeFieldInfo _T32;struct _union_DatatypeFieldInfo_KnownDatatypefield _T33;unsigned _T34;struct Cyc_Absyn_Kind*_T35;struct Cyc_Absyn_Kind*_T36;struct Cyc_Warn_String_Warn_Warg_struct _T37;int(*_T38)(struct _fat_ptr);void*(*_T39)(struct _fat_ptr);struct _fat_ptr _T3A;void*_T3B;struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T3C;union Cyc_Absyn_AggrInfo _T3D;struct _union_AggrInfo_UnknownAggr _T3E;unsigned _T3F;struct Cyc_Absyn_Kind*_T40;struct Cyc_Absyn_Kind*_T41;void*_T42;union Cyc_Absyn_AggrInfo _T43;struct _union_AggrInfo_KnownAggr _T44;struct Cyc_Absyn_Aggrdecl**_T45;struct Cyc_Absyn_Kind*_T46;int _T47;struct Cyc_Absyn_Kind*_T48;struct Cyc_Absyn_Kind*_T49;struct Cyc_Absyn_AggrdeclImpl*_T4A;struct Cyc_Absyn_Kind*_T4B;struct Cyc_Absyn_Kind*_T4C;enum Cyc_Absyn_AggrKind _T4D;int _T4E;struct Cyc_List_List*_T4F;struct Cyc_List_List*_T50;struct Cyc_List_List*_T51;struct Cyc_List_List*_T52;void*_T53;struct Cyc_Absyn_Aggrfield*_T54;struct Cyc_Absyn_Kind*_T55;struct Cyc_Absyn_Kind*_T56;struct Cyc_Absyn_Kind*_T57;struct Cyc_Absyn_Kind*_T58;struct Cyc_List_List*_T59;void*_T5A;struct Cyc_Absyn_Aggrfield*_T5B;struct Cyc_Absyn_Kind*_T5C;struct Cyc_Absyn_Kind*_T5D;struct Cyc_Absyn_Kind*_T5E;struct Cyc_Absyn_Kind*_T5F;struct Cyc_List_List*_T60;struct Cyc_Absyn_Kind*_T61;struct Cyc_Absyn_Kind*_T62;struct Cyc_Absyn_Kind*_T63;struct Cyc_Absyn_Kind*_T64;struct Cyc_Absyn_Kind*_T65;struct Cyc_Absyn_Kind*_T66;struct Cyc_Absyn_PtrInfo _T67;struct Cyc_Absyn_PtrAtts _T68;void*_T69;int*_T6A;int _T6B;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T6C;void*_T6D;int*_T6E;unsigned _T6F;struct Cyc_Absyn_Kind*_T70;struct Cyc_Absyn_Kind*_T71;struct Cyc_Absyn_Kind*_T72;struct Cyc_Absyn_Kind*_T73;struct Cyc_Absyn_Kind*_T74;struct Cyc_Absyn_Kind*_T75;struct Cyc_Absyn_Kind*_T76;struct Cyc_Absyn_Kind*_T77;struct Cyc_Absyn_ArrayInfo _T78;int _T79;struct Cyc_Absyn_Kind*_T7A;struct Cyc_Absyn_Kind*_T7B;struct Cyc_Absyn_Kind*_T7C;struct Cyc_Absyn_Kind*_T7D;struct Cyc_Absyn_Typedefdecl*_T7E;struct Cyc_Core_Opt*_T7F;struct Cyc_Warn_String_Warn_Warg_struct _T80;struct Cyc_Warn_Typ_Warn_Warg_struct _T81;int(*_T82)(struct _fat_ptr);void*(*_T83)(struct _fat_ptr);struct _fat_ptr _T84;struct Cyc_Absyn_Typedefdecl*_T85;struct Cyc_Core_Opt*_T86;void*_T87;struct Cyc_Absyn_Kind*_T88;struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_T89;struct Cyc_Absyn_TypeDecl*_T8A;struct Cyc_Absyn_TypeDecl*_T8B;void*_T8C;int*_T8D;unsigned _T8E;struct Cyc_Absyn_Kind*_T8F;struct Cyc_Absyn_Kind*_T90;struct Cyc_Absyn_Kind*_T91;struct Cyc_Absyn_Kind*_T92;
# 668
void*_T93=Cyc_Absyn_compress(t);struct Cyc_Absyn_Typedefdecl*_T94;struct Cyc_Absyn_Exp*_T95;struct Cyc_Absyn_PtrInfo _T96;struct Cyc_List_List*_T97;void*_T98;_T0=(int*)_T93;_T1=*_T0;switch(_T1){case 3:{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_T99=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T93;_T98=_T99->f1;}{struct Cyc_Core_Opt*k=_T98;_T98=k;goto _LL4;}case 1:{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T99=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T93;_T98=_T99->f1;}_LL4: {struct Cyc_Core_Opt*k=_T98;_T2=
# 670
_check_null(k);_T3=_T2->v;_T4=(struct Cyc_Absyn_Kind*)_T3;return _T4;}case 2:{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T99=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T93;_T98=_T99->f1;}{struct Cyc_Absyn_Tvar*tv=_T98;_T5=tv;_T6=& Cyc_Kinds_bk;_T7=(struct Cyc_Absyn_Kind*)_T6;_T8=
Cyc_Kinds_tvar_kind(_T5,_T7);return _T8;}case 12:{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_T99=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_T93;_T98=_T99->f1;}{struct Cyc_Absyn_Vardecl*vd=_T98;_T9=vd;_TA=_T9->type;_TB=
Cyc_Tcutil_type_kind(_TA);return _TB;}case 0:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T99=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T93;_TC=_T99->f1;_T98=(void*)_TC;_T97=_T99->f2;}{void*c=_T98;struct Cyc_List_List*ts=_T97;int _T99;struct Cyc_Absyn_AggrdeclImpl*_T9A;struct Cyc_List_List*_T9B;enum Cyc_Absyn_AggrKind _T9C;struct Cyc_Absyn_Kind*_T9D;enum Cyc_Absyn_Size_of _T9E;_TD=c;_TE=(int*)_TD;_TF=*_TE;switch(_TF){case 0: _T10=& Cyc_Kinds_mk;_T11=(struct Cyc_Absyn_Kind*)_T10;
# 675
return _T11;case 1: _T12=c;{struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T9F=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T12;_T9E=_T9F->f2;}{enum Cyc_Absyn_Size_of sz=_T9E;_T14=sz;_T15=(int)_T14;
# 677
if(_T15==2)goto _TLAF;else{goto _TLB0;}_TLB0: _T16=sz;_T17=(int)_T16;if(_T17==3)goto _TLAF;else{goto _TLAD;}_TLAF: _T18=& Cyc_Kinds_bk;_T13=(struct Cyc_Absyn_Kind*)_T18;goto _TLAE;_TLAD: _T19=& Cyc_Kinds_mk;_T13=(struct Cyc_Absyn_Kind*)_T19;_TLAE: return _T13;}case 3: goto _LL27;case 2: _LL27: _T1A=& Cyc_Kinds_mk;_T1B=(struct Cyc_Absyn_Kind*)_T1A;
# 679
return _T1B;case 19: goto _LL2B;case 20: _LL2B: goto _LL2D;case 4: _LL2D: _T1C=& Cyc_Kinds_bk;_T1D=(struct Cyc_Absyn_Kind*)_T1C;
# 682
return _T1D;case 18: _T1E=& Cyc_Kinds_bk;_T1F=(struct Cyc_Absyn_Kind*)_T1E;
return _T1F;case 7: goto _LL33;case 8: _LL33: goto _LL35;case 6: _LL35: _T20=& Cyc_Kinds_ek;_T21=(struct Cyc_Absyn_Kind*)_T20;
# 686
return _T21;case 21: _T22=c;{struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*_T9F=(struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_T22;_T9D=_T9F->f2;}{struct Cyc_Absyn_Kind*k=_T9D;_T23=k;
return _T23;}case 5: _T24=& Cyc_Kinds_bk;_T25=(struct Cyc_Absyn_Kind*)_T24;
return _T25;case 9: goto _LL3D;case 10: _LL3D: _T26=& Cyc_Kinds_ek;_T27=(struct Cyc_Absyn_Kind*)_T26;
# 690
return _T27;case 12: goto _LL41;case 11: _LL41: _T28=& Cyc_Kinds_boolk;_T29=(struct Cyc_Absyn_Kind*)_T28;
# 692
return _T29;case 13: goto _LL45;case 14: _LL45: _T2A=& Cyc_Kinds_ptrbk;_T2B=(struct Cyc_Absyn_Kind*)_T2A;
# 694
return _T2B;case 17: goto _LL49;case 16: _LL49: goto _LL4B;case 15: _LL4B: _T2C=& Cyc_Kinds_aqk;_T2D=(struct Cyc_Absyn_Kind*)_T2C;
# 697
return _T2D;case 22: _T2E=& Cyc_Kinds_ak;_T2F=(struct Cyc_Absyn_Kind*)_T2E;
return _T2F;case 23: _T30=c;_T31=(struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_T30;_T32=_T31->f1;_T33=_T32.KnownDatatypefield;_T34=_T33.tag;if(_T34!=2)goto _TLB1;_T35=& Cyc_Kinds_mk;_T36=(struct Cyc_Absyn_Kind*)_T35;
return _T36;_TLB1:{struct Cyc_Warn_String_Warn_Warg_struct _T9F;_T9F.tag=0;
# 701
_T9F.f1=_tag_fat("type_kind: Unresolved DatatypeFieldType",sizeof(char),40U);_T37=_T9F;}{struct Cyc_Warn_String_Warn_Warg_struct _T9F=_T37;void*_TA0[1];_TA0[0]=& _T9F;_T39=Cyc_Warn_impos2;{int(*_TA1)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T39;_T38=_TA1;}_T3A=_tag_fat(_TA0,sizeof(void*),1);_T38(_T3A);};default: _T3B=c;_T3C=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T3B;_T3D=_T3C->f1;_T3E=_T3D.UnknownAggr;_T3F=_T3E.tag;if(_T3F!=1)goto _TLB3;_T40=& Cyc_Kinds_ak;_T41=(struct Cyc_Absyn_Kind*)_T40;
# 704
return _T41;_TLB3: _T42=c;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T9F=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T42;_T43=_T9F->f1;_T44=_T43.KnownAggr;_T45=_T44.val;{struct Cyc_Absyn_Aggrdecl*_TA0=*_T45;struct Cyc_Absyn_Aggrdecl _TA1=*_TA0;_T9C=_TA1.kind;_T9B=_TA1.tvs;_T9A=_TA1.impl;_T99=_TA1.expected_mem_kind;}}{enum Cyc_Absyn_AggrKind strOrU=_T9C;struct Cyc_List_List*tvs=_T9B;struct Cyc_Absyn_AggrdeclImpl*i=_T9A;int expected_mem_kind=_T99;
# 706
if(i!=0)goto _TLB5;_T47=expected_mem_kind;
if(!_T47)goto _TLB7;_T48=& Cyc_Kinds_mk;_T46=(struct Cyc_Absyn_Kind*)_T48;goto _TLB8;_TLB7: _T49=& Cyc_Kinds_ak;_T46=(struct Cyc_Absyn_Kind*)_T49;_TLB8: return _T46;_TLB5: _T4A=i;{
struct Cyc_List_List*fields=_T4A->fields;
if(fields!=0)goto _TLB9;_T4B=& Cyc_Kinds_mk;_T4C=(struct Cyc_Absyn_Kind*)_T4B;return _T4C;_TLB9: _T4D=strOrU;_T4E=(int)_T4D;
# 711
if(_T4E!=0)goto _TLBB;
_TLC0: _T4F=_check_null(fields);_T50=_T4F->tl;if(_T50!=0)goto _TLBE;else{goto _TLBF;}
_TLBE: _T51=fields;
# 712
fields=_T51->tl;goto _TLC0;_TLBF: _T52=fields;_T53=_T52->hd;_T54=(struct Cyc_Absyn_Aggrfield*)_T53;{
# 714
void*last_type=_T54->type;
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_field_kind(last_type,ts,tvs);_T55=k;_T56=& Cyc_Kinds_ak;_T57=(struct Cyc_Absyn_Kind*)_T56;
if(_T55!=_T57)goto _TLC1;_T58=k;return _T58;_TLC1:;}goto _TLBC;
# 720
_TLBB: _TLC6: if(fields!=0)goto _TLC4;else{goto _TLC5;}
_TLC4: _T59=fields;_T5A=_T59->hd;_T5B=(struct Cyc_Absyn_Aggrfield*)_T5A;{void*type=_T5B->type;
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_field_kind(type,ts,tvs);_T5C=k;_T5D=& Cyc_Kinds_ak;_T5E=(struct Cyc_Absyn_Kind*)_T5D;
if(_T5C!=_T5E)goto _TLC7;_T5F=k;return _T5F;_TLC7:;}_T60=fields;
# 720
fields=_T60->tl;goto _TLC6;_TLC5: _TLBC: _T61=& Cyc_Kinds_mk;_T62=(struct Cyc_Absyn_Kind*)_T61;
# 725
return _T62;}}};}case 6: _T63=& Cyc_Kinds_ak;_T64=(struct Cyc_Absyn_Kind*)_T63;
# 727
return _T64;case 7: _T65=& Cyc_Kinds_mk;_T66=(struct Cyc_Absyn_Kind*)_T65;
return _T66;case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T99=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T93;_T96=_T99->f1;}{struct Cyc_Absyn_PtrInfo pinfo=_T96;_T67=pinfo;_T68=_T67.ptr_atts;_T69=_T68.bounds;{
# 730
void*_T99=Cyc_Absyn_compress(_T69);_T6A=(int*)_T99;_T6B=*_T6A;if(_T6B!=0)goto _TLC9;_T6C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T99;_T6D=_T6C->f1;_T6E=(int*)_T6D;_T6F=*_T6E;switch(_T6F){case 13: _T70=& Cyc_Kinds_bk;_T71=(struct Cyc_Absyn_Kind*)_T70;
# 732
return _T71;case 14: goto _LL5C;default: goto _LL5B;}goto _TLCA;_TLC9: _LL5B: _LL5C: _T72=& Cyc_Kinds_mk;_T73=(struct Cyc_Absyn_Kind*)_T72;
# 736
return _T73;_TLCA:;}}case 9: _T74=& Cyc_Kinds_ik;_T75=(struct Cyc_Absyn_Kind*)_T74;
# 738
return _T75;case 11: _T76=& Cyc_Kinds_ak;_T77=(struct Cyc_Absyn_Kind*)_T76;
# 742
return _T77;case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T99=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T93;_T78=_T99->f1;_T95=_T78.num_elts;}{struct Cyc_Absyn_Exp*num_elts=_T95;
# 744
if(num_elts==0)goto _TLCE;else{goto _TLCF;}_TLCF: _T79=Cyc_Tcutil_is_const_exp(num_elts);if(_T79)goto _TLCE;else{goto _TLCC;}_TLCE: _T7A=& Cyc_Kinds_mk;_T7B=(struct Cyc_Absyn_Kind*)_T7A;return _T7B;_TLCC: _T7C=& Cyc_Kinds_ak;_T7D=(struct Cyc_Absyn_Kind*)_T7C;
return _T7D;}case 8:{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T99=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T93;_T94=_T99->f3;}{struct Cyc_Absyn_Typedefdecl*td=_T94;
# 747
if(td==0)goto _TLD2;else{goto _TLD3;}_TLD3: _T7E=td;_T7F=_T7E->kind;if(_T7F==0)goto _TLD2;else{goto _TLD0;}
_TLD2:{struct Cyc_Warn_String_Warn_Warg_struct _T99;_T99.tag=0;_T99.f1=_tag_fat("type_kind: typedef found: ",sizeof(char),27U);_T80=_T99;}{struct Cyc_Warn_String_Warn_Warg_struct _T99=_T80;{struct Cyc_Warn_Typ_Warn_Warg_struct _T9A;_T9A.tag=2;_T9A.f1=t;_T81=_T9A;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T9A=_T81;void*_T9B[2];_T9B[0]=& _T99;_T9B[1]=& _T9A;_T83=Cyc_Warn_impos2;{int(*_T9C)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T83;_T82=_T9C;}_T84=_tag_fat(_T9B,sizeof(void*),2);_T82(_T84);}}goto _TLD1;_TLD0: _TLD1: _T85=td;_T86=_T85->kind;_T87=_T86->v;_T88=(struct Cyc_Absyn_Kind*)_T87;
return _T88;}default: _T89=(struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_T93;_T8A=_T89->f1;_T8B=(struct Cyc_Absyn_TypeDecl*)_T8A;_T8C=_T8B->r;_T8D=(int*)_T8C;_T8E=*_T8D;switch(_T8E){case 1: _T8F=& Cyc_Kinds_bk;_T90=(struct Cyc_Absyn_Kind*)_T8F;
return _T90;case 0: goto _LL1E;default: _LL1E: _T91=& Cyc_Kinds_ak;_T92=(struct Cyc_Absyn_Kind*)_T91;
# 752
return _T92;};};}
# 756
static void*Cyc_Tcutil_rgns_of(void*);
# 758
static void*Cyc_Tcutil_rgns_of_field(struct Cyc_Absyn_Aggrfield*af){struct Cyc_Absyn_Aggrfield*_T0;void*_T1;void*_T2;_T0=af;_T1=_T0->type;_T2=
Cyc_Tcutil_rgns_of(_T1);return _T2;}
# 762
static struct _tuple18*Cyc_Tcutil_region_free_subst(struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;struct Cyc_Absyn_Kind*_T1;struct Cyc_Absyn_Kind*_T2;struct Cyc_Absyn_Kind*_T3;enum Cyc_Absyn_KindQual _T4;int _T5;struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T6;struct _tuple18*_T7;
void*t;_T0=tv;_T1=& Cyc_Kinds_bk;_T2=(struct Cyc_Absyn_Kind*)_T1;{
struct Cyc_Absyn_Kind*_T8=Cyc_Kinds_tvar_kind(_T0,_T2);_T3=(struct Cyc_Absyn_Kind*)_T8;_T4=_T3->kind;_T5=(int)_T4;switch(_T5){case Cyc_Absyn_EffKind:
# 766
 t=Cyc_Absyn_heap_rgn_type;goto _LL0;case Cyc_Absyn_IntKind:{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T9=_cycalloc(sizeof(struct Cyc_Absyn_ValueofType_Absyn_Type_struct));_T9->tag=9;
_T9->f1=Cyc_Absyn_uint_exp(0U,0U);_T6=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_T9;}t=(void*)_T6;goto _LL0;case Cyc_Absyn_BoolKind:
 t=Cyc_Absyn_true_type;goto _LL0;case Cyc_Absyn_PtrBndKind:
 t=Cyc_Absyn_fat_bound_type;goto _LL0;default:
 t=Cyc_Absyn_sint_type;goto _LL0;}_LL0:;}{struct _tuple18*_T8=_cycalloc(sizeof(struct _tuple18));
# 772
_T8->f0=tv;_T8->f1=t;_T7=(struct _tuple18*)_T8;}return _T7;}
# 779
static void*Cyc_Tcutil_rgns_of(void*t){int*_T0;unsigned _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;struct Cyc_List_List*_T3;void*_T4;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T5;void*_T6;int*_T7;int _T8;void*_T9;struct Cyc_List_List*_TA;struct Cyc_List_List*_TB;void*_TC;void*_TD;struct Cyc_Absyn_Vardecl*_TE;void*_TF;void*_T10;struct Cyc_Absyn_Kind*_T11;enum Cyc_Absyn_KindQual _T12;int _T13;void*_T14;void*_T15;void*_T16;struct Cyc_Absyn_PtrInfo _T17;struct Cyc_Absyn_PtrInfo _T18;struct Cyc_Absyn_PtrAtts _T19;struct Cyc_List_List*_T1A;struct _fat_ptr _T1B;void*_T1C;void*_T1D;struct Cyc_Absyn_ArrayInfo _T1E;void*_T1F;void*_T20;struct Cyc_List_List*(*_T21)(void*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*);struct Cyc_List_List*(*_T22)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T23;struct Cyc_List_List*_T24;void*_T25;void*_T26;struct Cyc_Absyn_FnInfo _T27;struct Cyc_Absyn_FnInfo _T28;struct Cyc_Absyn_FnInfo _T29;struct Cyc_Absyn_FnInfo _T2A;struct Cyc_Absyn_FnInfo _T2B;struct Cyc_Absyn_FnInfo _T2C;struct Cyc_Absyn_FnInfo _T2D;struct Cyc_List_List*(*_T2E)(struct _tuple18*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*);struct Cyc_List_List*(*_T2F)(void*(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T30;struct Cyc_List_List*_T31;void*_T32;void*_T33;struct Cyc_List_List*_T34;struct Cyc_List_List*_T35;void*_T36;void*_T37;struct Cyc_Warn_String_Warn_Warg_struct _T38;int(*_T39)(struct _fat_ptr);void*(*_T3A)(struct _fat_ptr);struct _fat_ptr _T3B;void*_T3C;
void*_T3D=Cyc_Absyn_compress(t);struct Cyc_List_List*_T3E;struct Cyc_Absyn_VarargInfo*_T3F;struct Cyc_List_List*_T40;struct Cyc_Absyn_Tqual _T41;struct Cyc_List_List*_T42;void*_T43;void*_T44;_T0=(int*)_T3D;_T1=*_T0;switch(_T1){case 0: _T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3D;_T3=_T2->f2;if(_T3!=0)goto _TLD7;_T4=Cyc_Absyn_empty_effect;
return _T4;_TLD7: _T5=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3D;_T6=_T5->f1;_T7=(int*)_T6;_T8=*_T7;if(_T8!=9)goto _TLD9;_T9=t;
return _T9;_TLD9:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T45=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3D;_T44=_T45->f2;}{struct Cyc_List_List*ts=_T44;_TA=ts;_TB=
Cyc_List_map(Cyc_Tcutil_rgns_of,_TA);_TC=Cyc_Absyn_join_eff(_TB);_TD=Cyc_Tcutil_normalize_effect(_TC);return _TD;}case 12:{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_T45=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_T3D;_T44=_T45->f1;}{struct Cyc_Absyn_Vardecl*vd=_T44;_TE=vd;_TF=_TE->type;_T10=
Cyc_Tcutil_rgns_of(_TF);return _T10;}case 3: goto _LLC;case 1: _LLC: goto _LLE;case 2: _LLE: {
# 788
struct Cyc_Absyn_Kind*_T45=Cyc_Tcutil_type_kind(t);_T11=(struct Cyc_Absyn_Kind*)_T45;_T12=_T11->kind;_T13=(int)_T12;switch(_T13){case Cyc_Absyn_EffKind: _T14=t;
# 790
return _T14;case Cyc_Absyn_IntKind: _T15=Cyc_Absyn_empty_effect;
return _T15;default: _T16=
Cyc_Absyn_regionsof_eff(t);return _T16;};}case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T45=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T3D;_T17=_T45->f1;_T44=_T17.elt_type;_T18=_T45->f1;_T19=_T18.ptr_atts;_T43=_T19.eff;}{void*et=_T44;void*r=_T43;{void*_T45[2];
# 796
_T45[0]=r;_T45[1]=Cyc_Tcutil_rgns_of(et);_T1B=_tag_fat(_T45,sizeof(void*),2);_T1A=Cyc_List_list(_T1B);}_T1C=Cyc_Absyn_join_eff(_T1A);_T1D=Cyc_Tcutil_normalize_effect(_T1C);return _T1D;}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T45=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T3D;_T1E=_T45->f1;_T44=_T1E.elt_type;}{void*et=_T44;_T1F=
# 798
Cyc_Tcutil_rgns_of(et);_T20=Cyc_Tcutil_normalize_effect(_T1F);return _T20;}case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T45=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T3D;_T42=_T45->f3;}{struct Cyc_List_List*fs=_T42;_T22=Cyc_List_map;{
# 800
struct Cyc_List_List*(*_T45)(void*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(void*(*)(struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*))_T22;_T21=_T45;}_T23=fs;_T24=_T21(Cyc_Tcutil_rgns_of_field,_T23);_T25=Cyc_Absyn_join_eff(_T24);_T26=Cyc_Tcutil_normalize_effect(_T25);return _T26;}case 6:{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T45=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T3D;_T27=_T45->f1;_T42=_T27.tvars;_T28=_T45->f1;_T44=_T28.effect;_T29=_T45->f1;_T41=_T29.ret_tqual;_T2A=_T45->f1;_T43=_T2A.ret_type;_T2B=_T45->f1;_T40=_T2B.args;_T2C=_T45->f1;_T3F=_T2C.cyc_varargs;_T2D=_T45->f1;_T3E=_T2D.qual_bnd;}{struct Cyc_List_List*tvs=_T42;void*eff=_T44;struct Cyc_Absyn_Tqual rt_tq=_T41;void*rt=_T43;struct Cyc_List_List*args=_T40;struct Cyc_Absyn_VarargInfo*cyc_varargs=_T3F;struct Cyc_List_List*rpo=_T3E;_T2F=Cyc_List_map;{
# 809
struct Cyc_List_List*(*_T45)(struct _tuple18*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple18*(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))_T2F;_T2E=_T45;}_T30=tvs;_T31=_T2E(Cyc_Tcutil_region_free_subst,_T30);_T32=_check_null(eff);{void*e=Cyc_Tcutil_substitute(_T31,_T32);_T33=
Cyc_Tcutil_normalize_effect(e);return _T33;}}case 8:{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T45=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T3D;_T42=_T45->f2;}{struct Cyc_List_List*ts=_T42;_T34=ts;_T35=
# 812
Cyc_List_map(Cyc_Tcutil_rgns_of,_T34);_T36=Cyc_Absyn_join_eff(_T35);_T37=Cyc_Tcutil_normalize_effect(_T36);return _T37;}case 10:{struct Cyc_Warn_String_Warn_Warg_struct _T45;_T45.tag=0;
_T45.f1=_tag_fat("typedecl in rgns_of",sizeof(char),20U);_T38=_T45;}{struct Cyc_Warn_String_Warn_Warg_struct _T45=_T38;void*_T46[1];_T46[0]=& _T45;_T3A=Cyc_Warn_impos2;{int(*_T47)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T3A;_T39=_T47;}_T3B=_tag_fat(_T46,sizeof(void*),1);_T39(_T3B);}case 9: goto _LL1E;default: _LL1E: _T3C=Cyc_Absyn_empty_effect;
# 815
return _T3C;};}
# 822
static void*Cyc_Tcutil_normalize_effect_dups(void*e){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;unsigned _T7;void*_T8;void*_T9;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TA;struct Cyc_List_List**_TB;struct Cyc_List_List**_TC;struct Cyc_List_List*_TD;struct Cyc_List_List*_TE;void*_TF;void*_T10;struct Cyc_List_List*_T11;int*_T12;int _T13;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T14;void*_T15;int*_T16;int _T17;struct Cyc_List_List*_T18;int _T19;void*_T1A;struct Cyc_List_List**_T1B;struct Cyc_List_List*_T1C;void*_T1D;int*_T1E;int _T1F;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T20;void*_T21;int*_T22;int _T23;struct Cyc_List_List*_T24;struct Cyc_List_List*_T25;struct Cyc_List_List**_T26;void*_T27;void*_T28;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T29;struct Cyc_List_List*_T2A;void*_T2B;struct Cyc_List_List*_T2C;void*_T2D;int*_T2E;unsigned _T2F;void*_T30;void*_T31;void*_T32;
e=Cyc_Absyn_compress(e);{void*_T33;_T0=e;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TLDC;_T3=e;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;switch(_T7){case 9: _T8=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_T9=e;_TA=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_TB=& _TA->f2;_T33=(struct Cyc_List_List**)_TB;}{struct Cyc_List_List**es=_T33;
# 826
int redo_join=0;_TC=es;{
struct Cyc_List_List*effs=*_TC;_TLE2: if(effs!=0)goto _TLE0;else{goto _TLE1;}
_TLE0: _TD=effs;{void*eff=_TD->hd;_TE=effs;_TF=
Cyc_Tcutil_normalize_effect(eff);_T10=Cyc_Absyn_compress(_TF);_TE->hd=(void*)_T10;_T11=effs;{
void*_T34=_T11->hd;_T12=(int*)_T34;_T13=*_T12;if(_T13!=0)goto _TLE3;_T14=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T34;_T15=_T14->f1;_T16=(int*)_T15;_T17=*_T16;if(_T17!=9)goto _TLE5;
# 835
redo_join=1;goto _LL7;_TLE5: goto _LLA;_TLE3: _LLA: goto _LL7;_LL7:;}}_T18=effs;
# 827
effs=_T18->tl;goto _TLE2;_TLE1:;}_T19=redo_join;
# 839
if(_T19)goto _TLE7;else{goto _TLE9;}_TLE9: _T1A=e;return _T1A;_TLE7: {
struct Cyc_List_List*effects=0;_T1B=es;{
struct Cyc_List_List*effs=*_T1B;_TLED: if(effs!=0)goto _TLEB;else{goto _TLEC;}
_TLEB: _T1C=effs;_T1D=_T1C->hd;{void*_T34=Cyc_Absyn_compress(_T1D);void*_T35;_T1E=(int*)_T34;_T1F=*_T1E;if(_T1F!=0)goto _TLEE;_T20=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T34;_T21=_T20->f1;_T22=(int*)_T21;_T23=*_T22;if(_T23!=9)goto _TLF0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T36=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T34;_T35=_T36->f2;}{struct Cyc_List_List*nested_effs=_T35;
# 844
effects=Cyc_List_revappend(nested_effs,effects);goto _LLC;}_TLF0: goto _LLF;_TLEE: _LLF: _T35=_T34;{void*e=_T35;{struct Cyc_List_List*_T36=_cycalloc(sizeof(struct Cyc_List_List));
# 849
_T36->hd=e;_T36->tl=effects;_T24=(struct Cyc_List_List*)_T36;}effects=_T24;goto _LLC;}_LLC:;}_T25=effs;
# 841
effs=_T25->tl;goto _TLED;_TLEC:;}_T26=es;
# 851
*_T26=Cyc_List_imp_rev(effects);_T27=e;
return _T27;}}case 10: _T28=e;_T29=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T28;_T2A=_T29->f2;if(_T2A==0)goto _TLF2;_T2B=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2B;_T2C=_T34->f2;{struct Cyc_List_List _T35=*_T2C;_T2D=_T35.hd;_T33=(void*)_T2D;}}{void*t=_T33;
# 854
void*_T34=Cyc_Absyn_compress(t);_T2E=(int*)_T34;_T2F=*_T2E;switch(_T2F){case 1: goto _LL15;case 2: _LL15: _T30=e;
# 856
return _T30;default: _T31=
Cyc_Tcutil_rgns_of(t);return _T31;};}goto _TLF3;_TLF2: goto _LL5;_TLF3:;default: goto _LL5;}goto _TLDD;_TLDC: _LL5: _T32=e;
# 859
return _T32;_TLDD:;}}
# 862
static void Cyc_Tcutil_imp_remove_dups(int(*)(void*,void*),struct Cyc_List_List*);
void*Cyc_Tcutil_normalize_effect(void*e){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;int _T7;void*_T8;int(*_T9)(void*,void*);struct Cyc_List_List*_TA;void*_TB;
e=Cyc_Tcutil_normalize_effect_dups(e);{struct Cyc_List_List*_TC;_T0=e;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TLF5;_T3=e;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;if(_T7!=9)goto _TLF7;_T8=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TD=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_TC=_TD->f2;}{struct Cyc_List_List*l=_TC;_T9=Cyc_Tcutil_typecmp;_TA=l;
# 867
Cyc_Tcutil_imp_remove_dups(_T9,_TA);goto _LL4;}_TLF7: goto _LL3;_TLF5: _LL3: _LL4: _TB=e;
# 870
return _TB;;}}
# 874
static int Cyc_Tcutil_type_equals(void*t1,void*t2){int _T0;int _T1;_T0=
Cyc_Tcutil_typecmp(t1,t2);_T1=_T0==0;return _T1;}
# 879
static void*Cyc_Tcutil_dummy_fntype(void*eff){struct Cyc_Absyn_FnType_Absyn_Type_struct*_T0;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T1;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T2;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T3;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T4;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T5;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T6;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T7;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T8;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T9;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TA;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TB;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TC;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TD;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TE;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TF;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T10;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T11;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T12;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T13;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T14;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T15;void*_T16;void*_T17;void*_T18;struct Cyc_Absyn_Tqual _T19;void*_T1A;void*_T1B;void*_T1C;void*_T1D;
struct Cyc_Absyn_FnType_Absyn_Type_struct*fntype;fntype=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_T0=fntype;_T0->tag=6;_T1=fntype;_T1->f1.tvars=0;_T2=fntype;_T2->f1.effect=eff;_T3=fntype;
_T3->f1.ret_tqual=Cyc_Absyn_empty_tqual(0U);_T4=fntype;
_T4->f1.ret_type=Cyc_Absyn_void_type;_T5=fntype;
_T5->f1.args=0;_T6=fntype;_T6->f1.c_varargs=0;_T7=fntype;
_T7->f1.cyc_varargs=0;_T8=fntype;_T8->f1.qual_bnd=0;_T9=fntype;
_T9->f1.attributes=0;_TA=fntype;
_TA->f1.checks_clause=0;_TB=fntype;
_TB->f1.checks_assn=0;_TC=fntype;
_TC->f1.requires_clause=0;_TD=fntype;
_TD->f1.requires_assn=0;_TE=fntype;
_TE->f1.ensures_clause=0;_TF=fntype;
_TF->f1.ensures_assn=0;_T10=fntype;
_T10->f1.throws_clause=0;_T11=fntype;
_T11->f1.throws_assn=0;_T12=fntype;
_T12->f1.return_value=0;_T13=fntype;
_T13->f1.arg_vardecls=0;_T14=fntype;
_T14->f1.effconstr=0;_T15=fntype;_T16=(void*)_T15;_T17=Cyc_Absyn_heap_rgn_type;_T18=Cyc_Absyn_al_qual_type;_T19=
Cyc_Absyn_empty_tqual(0U);_T1A=Cyc_Absyn_bounds_one();_T1B=Cyc_Absyn_false_type;_T1C=Cyc_Absyn_false_type;_T1D=Cyc_Absyn_atb_type(_T16,_T17,_T18,_T19,_T1A,_T1B,_T1C);return _T1D;}
# 903
int Cyc_Tcutil_type_in_effect(int may_constrain_evars,void*t,void*e){void*_T0;void*_T1;int*_T2;unsigned _T3;void*_T4;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T5;void*_T6;int*_T7;unsigned _T8;int _T9;int _TA;int _TB;int _TC;void*_TD;int _TE;void*_TF;struct Cyc_List_List*_T10;void*_T11;int _T12;struct Cyc_List_List*_T13;void*_T14;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T15;struct Cyc_List_List*_T16;void*_T17;struct Cyc_List_List*_T18;void*_T19;int _T1A;int _T1B;int*_T1C;int _T1D;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1E;void*_T1F;int*_T20;int _T21;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T22;struct Cyc_List_List*_T23;struct Cyc_List_List*_T24;void*_T25;struct _tuple15 _T26;void*_T27;int*_T28;int _T29;void*_T2A;int*_T2B;int _T2C;void*_T2D;void*_T2E;int _T2F;int _T30;int _T31;void*_T32;void*_T33;struct Cyc_Absyn_Evar_Absyn_Type_struct*_T34;void**_T35;struct Cyc_Core_Opt*_T36;void*_T37;struct Cyc_Absyn_Kind*_T38;enum Cyc_Absyn_KindQual _T39;int _T3A;struct Cyc_Warn_String_Warn_Warg_struct _T3B;int(*_T3C)(struct _fat_ptr);void*(*_T3D)(struct _fat_ptr);struct _fat_ptr _T3E;int _T3F;struct Cyc_Core_Opt*_T40;struct Cyc_Core_Opt*_T41;struct Cyc_Core_Opt*_T42;void*_T43;struct _RegionHandle*_T44;struct Cyc_Core_Opt*_T45;void*_T46;struct Cyc_List_List*_T47;void*_T48;struct Cyc_List_List*_T49;struct _fat_ptr _T4A;void**_T4B;int _T4C;int _T4D;
t=Cyc_Absyn_compress(t);_T0=
Cyc_Absyn_compress(e);e=Cyc_Tcutil_normalize_effect(_T0);{struct Cyc_Core_Opt*_T4E;struct Cyc_Core_Opt*_T4F;void*_T50;_T1=e;_T2=(int*)_T1;_T3=*_T2;switch(_T3){case 0: _T4=e;_T5=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T4;_T6=_T5->f1;_T7=(int*)_T6;_T8=*_T7;switch(_T8){case 6: _T9=may_constrain_evars;
# 909
if(!_T9)goto _TLFB;_TA=Cyc_Unify_unify(t,Cyc_Absyn_heap_rgn_type);return _TA;_TLFB: _TB=
Cyc_Tcutil_typecmp(t,Cyc_Absyn_heap_rgn_type);_TC=_TB==0;return _TC;case 9: _TD=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T51=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TD;_T50=_T51->f2;}{struct Cyc_List_List*es=_T50;
# 912
_TL100: if(es!=0)goto _TLFE;else{goto _TLFF;}
_TLFE: _TE=may_constrain_evars;_TF=t;_T10=es;_T11=_T10->hd;_T12=Cyc_Tcutil_type_in_effect(_TE,_TF,_T11);if(!_T12)goto _TL101;
return 1;_TL101: _T13=es;
# 912
es=_T13->tl;goto _TL100;_TLFF:
# 915
 return 0;}case 10: _T14=e;_T15=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T14;_T16=_T15->f2;if(_T16==0)goto _TL103;_T17=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T51=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T17;_T18=_T51->f2;{struct Cyc_List_List _T52=*_T18;_T19=_T52.hd;_T50=(void*)_T19;}}{void*t2=_T50;
# 917
t2=Cyc_Absyn_compress(t2);
if(t!=t2)goto _TL105;return 1;_TL105: _T1A=may_constrain_evars;
if(!_T1A)goto _TL107;_T1B=Cyc_Unify_unify(t,t2);return _T1B;_TL107: {
void*_T51=Cyc_Tcutil_rgns_of(t);void*_T52;_T1C=(int*)_T51;_T1D=*_T1C;if(_T1D!=0)goto _TL109;_T1E=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T51;_T1F=_T1E->f1;_T20=(int*)_T1F;_T21=*_T20;if(_T21!=10)goto _TL10B;_T22=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T51;_T23=_T22->f2;if(_T23==0)goto _TL10D;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T53=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T51;_T24=_T53->f2;{struct Cyc_List_List _T54=*_T24;_T25=_T54.hd;_T52=(void*)_T25;}}{void*t3=_T52;{struct _tuple15 _T53;
# 922
_T53.f0=Cyc_Absyn_compress(t3);_T53.f1=t2;_T26=_T53;}{struct _tuple15 _T53=_T26;struct Cyc_Absyn_Tvar*_T54;struct Cyc_Absyn_Tvar*_T55;_T27=_T53.f0;_T28=(int*)_T27;_T29=*_T28;if(_T29!=2)goto _TL10F;_T2A=_T53.f1;_T2B=(int*)_T2A;_T2C=*_T2B;if(_T2C!=2)goto _TL111;_T2D=_T53.f0;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T56=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T2D;_T55=_T56->f1;}_T2E=_T53.f1;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T56=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T2E;_T54=_T56->f1;}{struct Cyc_Absyn_Tvar*tv1=_T55;struct Cyc_Absyn_Tvar*tv2=_T54;_T2F=
Cyc_Unify_unify(t,t2);return _T2F;}_TL111: goto _LL13;_TL10F: _LL13: _T30=t3==t2;
return _T30;;}}goto _TL10E;_TL10D: goto _LLE;_TL10E: goto _TL10C;_TL10B: goto _LLE;_TL10C: goto _TL10A;_TL109: _LLE: _T52=_T51;{void*e2=_T52;_T31=
# 926
Cyc_Tcutil_type_in_effect(may_constrain_evars,t,e2);return _T31;}_TL10A:;}}goto _TL104;_TL103: goto _LL9;_TL104:;default: goto _LL9;};case 1: _T32=e;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T51=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T32;_T4F=_T51->f1;_T33=e;_T34=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T33;_T35=& _T34->f2;_T50=(void**)_T35;_T4E=_T51->f4;}{struct Cyc_Core_Opt*k=_T4F;void**p=(void**)_T50;struct Cyc_Core_Opt*s=_T4E;
# 929
if(k==0)goto _TL115;else{goto _TL116;}_TL116: _T36=k;_T37=_T36->v;_T38=(struct Cyc_Absyn_Kind*)_T37;_T39=_T38->kind;_T3A=(int)_T39;if(_T3A!=3)goto _TL115;else{goto _TL113;}
_TL115:{struct Cyc_Warn_String_Warn_Warg_struct _T51;_T51.tag=0;_T51.f1=_tag_fat("effect evar has wrong kind",sizeof(char),27U);_T3B=_T51;}{struct Cyc_Warn_String_Warn_Warg_struct _T51=_T3B;void*_T52[1];_T52[0]=& _T51;_T3D=Cyc_Warn_impos2;{int(*_T53)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T3D;_T3C=_T53;}_T3E=_tag_fat(_T52,sizeof(void*),1);_T3C(_T3E);}goto _TL114;_TL113: _TL114: _T3F=may_constrain_evars;
if(_T3F)goto _TL117;else{goto _TL119;}_TL119: return 0;_TL117: _T40=& Cyc_Kinds_eko;_T41=(struct Cyc_Core_Opt*)_T40;_T42=s;{
# 934
void*ev=Cyc_Absyn_new_evar(_T41,_T42);_T43=ev;_T44=Cyc_Core_heap_region;_T45=
# 937
_check_null(s);_T46=_T45->v;_T47=(struct Cyc_List_List*)_T46;_T48=t;Cyc_Unify_occurs(_T43,_T44,_T47,_T48);{void*_T51[2];
_T51[0]=ev;_T51[1]=Cyc_Absyn_regionsof_eff(t);_T4A=_tag_fat(_T51,sizeof(void*),2);_T49=Cyc_List_list(_T4A);}{void*new_typ=Cyc_Absyn_join_eff(_T49);_T4B=p;
*_T4B=new_typ;
return 1;}}}default: _LL9: _T4C=
Cyc_Tcutil_typecmp(t,e);_T4D=_T4C==0;return _T4D;};}}
# 947
static int Cyc_Tcutil_variable_in_effect(int may_constrain_evars,struct Cyc_Absyn_Tvar*v,void*e){void*_T0;int*_T1;unsigned _T2;void*_T3;int _T4;int _T5;void*_T6;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T7;void*_T8;int*_T9;unsigned _TA;void*_TB;int _TC;struct Cyc_Absyn_Tvar*_TD;struct Cyc_List_List*_TE;void*_TF;int _T10;struct Cyc_List_List*_T11;void*_T12;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T13;struct Cyc_List_List*_T14;void*_T15;struct Cyc_List_List*_T16;void*_T17;int*_T18;int _T19;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1A;void*_T1B;int*_T1C;int _T1D;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1E;struct Cyc_List_List*_T1F;struct Cyc_List_List*_T20;void*_T21;int _T22;int*_T23;int _T24;struct Cyc_Absyn_Evar_Absyn_Type_struct*_T25;void**_T26;struct Cyc_Core_Opt*_T27;struct Cyc_Core_Opt*_T28;struct Cyc_Core_Opt*_T29;int(*_T2A)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*);int(*_T2B)(int(*)(void*,void*),struct Cyc_List_List*,void*);struct Cyc_Core_Opt*_T2C;void*_T2D;struct Cyc_List_List*_T2E;struct Cyc_Absyn_Tvar*_T2F;int _T30;void**_T31;struct Cyc_List_List*_T32;struct _fat_ptr _T33;void*_T34;int _T35;void*_T36;void*_T37;struct Cyc_Absyn_Evar_Absyn_Type_struct*_T38;void**_T39;struct Cyc_Core_Opt*_T3A;void*_T3B;struct Cyc_Absyn_Kind*_T3C;enum Cyc_Absyn_KindQual _T3D;int _T3E;struct Cyc_Warn_String_Warn_Warg_struct _T3F;int(*_T40)(struct _fat_ptr);void*(*_T41)(struct _fat_ptr);struct _fat_ptr _T42;int _T43;struct Cyc_Core_Opt*_T44;struct Cyc_Core_Opt*_T45;struct Cyc_Core_Opt*_T46;int(*_T47)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*);int(*_T48)(int(*)(void*,void*),struct Cyc_List_List*,void*);struct Cyc_Core_Opt*_T49;void*_T4A;struct Cyc_List_List*_T4B;struct Cyc_Absyn_Tvar*_T4C;int _T4D;struct Cyc_List_List*_T4E;struct _fat_ptr _T4F;void**_T50;
e=Cyc_Absyn_compress(e);{struct Cyc_Core_Opt*_T51;struct Cyc_Core_Opt*_T52;void*_T53;_T0=e;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 2: _T3=e;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T54=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T3;_T53=_T54->f1;}{struct Cyc_Absyn_Tvar*v2=_T53;_T4=
# 950
Cyc_Absyn_tvar_cmp(v,v2);_T5=_T4==0;return _T5;}case 0: _T6=e;_T7=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6;_T8=_T7->f1;_T9=(int*)_T8;_TA=*_T9;switch(_TA){case 9: _TB=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T54=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TB;_T53=_T54->f2;}{struct Cyc_List_List*es=_T53;
# 952
_TL11F: if(es!=0)goto _TL11D;else{goto _TL11E;}
_TL11D: _TC=may_constrain_evars;_TD=v;_TE=es;_TF=_TE->hd;_T10=Cyc_Tcutil_variable_in_effect(_TC,_TD,_TF);if(!_T10)goto _TL120;
return 1;_TL120: _T11=es;
# 952
es=_T11->tl;goto _TL11F;_TL11E:
# 955
 return 0;}case 10: _T12=e;_T13=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T12;_T14=_T13->f2;if(_T14==0)goto _TL122;_T15=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T54=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T15;_T16=_T54->f2;{struct Cyc_List_List _T55=*_T16;_T17=_T55.hd;_T53=(void*)_T17;}}{void*t=_T53;
# 957
void*_T54=Cyc_Tcutil_rgns_of(t);void*_T55;_T18=(int*)_T54;_T19=*_T18;if(_T19!=0)goto _TL124;_T1A=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T54;_T1B=_T1A->f1;_T1C=(int*)_T1B;_T1D=*_T1C;if(_T1D!=10)goto _TL126;_T1E=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T54;_T1F=_T1E->f2;if(_T1F==0)goto _TL128;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T56=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T54;_T20=_T56->f2;{struct Cyc_List_List _T57=*_T20;_T21=_T57.hd;_T55=(void*)_T21;}}{void*t2=_T55;_T22=may_constrain_evars;
# 959
if(_T22)goto _TL12A;else{goto _TL12C;}_TL12C: return 0;_TL12A: {
void*_T56=Cyc_Absyn_compress(t2);struct Cyc_Core_Opt*_T57;void*_T58;struct Cyc_Core_Opt*_T59;_T23=(int*)_T56;_T24=*_T23;if(_T24!=1)goto _TL12D;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T5A=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T56;_T59=_T5A->f1;_T25=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T56;_T26=& _T25->f2;_T58=(void**)_T26;_T57=_T5A->f4;}{struct Cyc_Core_Opt*k=_T59;void**p=(void**)_T58;struct Cyc_Core_Opt*s=_T57;_T27=& Cyc_Kinds_eko;_T28=(struct Cyc_Core_Opt*)_T27;_T29=s;{
# 965
void*ev=Cyc_Absyn_new_evar(_T28,_T29);_T2B=Cyc_List_mem;{
# 967
int(*_T5A)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*)=(int(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*))_T2B;_T2A=_T5A;}_T2C=_check_null(s);_T2D=_T2C->v;_T2E=(struct Cyc_List_List*)_T2D;_T2F=v;_T30=_T2A(Cyc_Tcutil_fast_tvar_cmp,_T2E,_T2F);if(_T30)goto _TL12F;else{goto _TL131;}_TL131: return 0;_TL12F: _T31=p;{void*_T5A[2];
_T5A[0]=ev;_T5A[1]=Cyc_Absyn_var_type(v);_T33=_tag_fat(_T5A,sizeof(void*),2);_T32=Cyc_List_list(_T33);}_T34=Cyc_Absyn_join_eff(_T32);*_T31=Cyc_Tcutil_dummy_fntype(_T34);
return 1;}}_TL12D:
 return 0;;}}goto _TL129;_TL128: goto _LLE;_TL129: goto _TL127;_TL126: goto _LLE;_TL127: goto _TL125;_TL124: _LLE: _T55=_T54;{void*e2=_T55;_T35=
# 972
Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,e2);return _T35;}_TL125:;}goto _TL123;_TL122: goto _LL9;_TL123:;default: goto _LL9;};case 1: _T36=e;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T54=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T36;_T52=_T54->f1;_T37=e;_T38=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T37;_T39=& _T38->f2;_T53=(void**)_T39;_T51=_T54->f4;}{struct Cyc_Core_Opt*k=_T52;void**p=(void**)_T53;struct Cyc_Core_Opt*s=_T51;
# 975
if(k==0)goto _TL134;else{goto _TL135;}_TL135: _T3A=k;_T3B=_T3A->v;_T3C=(struct Cyc_Absyn_Kind*)_T3B;_T3D=_T3C->kind;_T3E=(int)_T3D;if(_T3E!=3)goto _TL134;else{goto _TL132;}
_TL134:{struct Cyc_Warn_String_Warn_Warg_struct _T54;_T54.tag=0;_T54.f1=_tag_fat("effect evar has wrong kind",sizeof(char),27U);_T3F=_T54;}{struct Cyc_Warn_String_Warn_Warg_struct _T54=_T3F;void*_T55[1];_T55[0]=& _T54;_T41=Cyc_Warn_impos2;{int(*_T56)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T41;_T40=_T56;}_T42=_tag_fat(_T55,sizeof(void*),1);_T40(_T42);}goto _TL133;_TL132: _TL133: _T43=may_constrain_evars;
if(_T43)goto _TL136;else{goto _TL138;}
_TL138: return 0;_TL136: _T44=& Cyc_Kinds_eko;_T45=(struct Cyc_Core_Opt*)_T44;_T46=s;{
# 981
void*ev=Cyc_Absyn_new_evar(_T45,_T46);_T48=Cyc_List_mem;{
# 983
int(*_T54)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*)=(int(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*))_T48;_T47=_T54;}_T49=_check_null(s);_T4A=_T49->v;_T4B=(struct Cyc_List_List*)_T4A;_T4C=v;_T4D=_T47(Cyc_Tcutil_fast_tvar_cmp,_T4B,_T4C);if(_T4D)goto _TL139;else{goto _TL13B;}
_TL13B: return 0;_TL139:{void*_T54[2];
_T54[0]=ev;_T54[1]=Cyc_Absyn_var_type(v);_T4F=_tag_fat(_T54,sizeof(void*),2);_T4E=Cyc_List_list(_T4F);}{void*new_typ=Cyc_Absyn_join_eff(_T4E);_T50=p;
*_T50=new_typ;
return 1;}}}default: _LL9:
 return 0;};}}
# 993
static int Cyc_Tcutil_evar_in_effect(void*evar,void*e){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;unsigned _T7;void*_T8;void*_T9;struct Cyc_List_List*_TA;void*_TB;int _TC;struct Cyc_List_List*_TD;void*_TE;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TF;struct Cyc_List_List*_T10;void*_T11;struct Cyc_List_List*_T12;void*_T13;int*_T14;int _T15;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T16;void*_T17;int*_T18;int _T19;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1A;struct Cyc_List_List*_T1B;struct Cyc_List_List*_T1C;void*_T1D;int _T1E;int _T1F;
e=Cyc_Absyn_compress(e);{void*_T20;_T0=e;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 0: _T3=e;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;switch(_T7){case 9: _T8=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T21=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_T20=_T21->f2;}{struct Cyc_List_List*es=_T20;
# 997
_TL141: if(es!=0)goto _TL13F;else{goto _TL140;}
_TL13F: _T9=evar;_TA=es;_TB=_TA->hd;_TC=Cyc_Tcutil_evar_in_effect(_T9,_TB);if(!_TC)goto _TL142;
return 1;_TL142: _TD=es;
# 997
es=_TD->tl;goto _TL141;_TL140:
# 1000
 return 0;}case 10: _TE=e;_TF=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TE;_T10=_TF->f2;if(_T10==0)goto _TL144;_T11=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T21=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T11;_T12=_T21->f2;{struct Cyc_List_List _T22=*_T12;_T13=_T22.hd;_T20=(void*)_T13;}}{void*t=_T20;
# 1002
void*_T21=Cyc_Tcutil_rgns_of(t);void*_T22;_T14=(int*)_T21;_T15=*_T14;if(_T15!=0)goto _TL146;_T16=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T17=_T16->f1;_T18=(int*)_T17;_T19=*_T18;if(_T19!=10)goto _TL148;_T1A=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T1B=_T1A->f2;if(_T1B==0)goto _TL14A;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T23=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T1C=_T23->f2;{struct Cyc_List_List _T24=*_T1C;_T1D=_T24.hd;_T22=(void*)_T1D;}}{void*t2=_T22;
return 0;}_TL14A: goto _LLC;_TL148: goto _LLC;_TL146: _LLC: _T22=_T21;{void*e2=_T22;_T1E=
Cyc_Tcutil_evar_in_effect(evar,e2);return _T1E;};}goto _TL145;_TL144: goto _LL7;_TL145:;default: goto _LL7;};case 1: _T1F=evar==e;
# 1006
return _T1F;default: _LL7:
 return 0;};}}
# 1012
static int Cyc_Tcutil_unify_effect_component(void*ev,void*eff){void*_T0;int*_T1;int _T2;void*_T3;void*_T4;struct Cyc_Absyn_Evar_Absyn_Type_struct*_T5;void**_T6;void*_T7;int*_T8;int _T9;void*_TA;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TB;void*_TC;int*_TD;int _TE;void*_TF;struct _handler_cons*_T10;int _T11;void*_T12;struct _RegionHandle*_T13;struct Cyc_Core_Opt*_T14;void*_T15;struct Cyc_List_List*_T16;struct Cyc_List_List*_T17;void*_T18;void**_T19;struct Cyc_List_List*_T1A;void*_T1B;struct Cyc_List_List*_T1C;int(*_T1D)(struct _fat_ptr,struct _fat_ptr);void*(*_T1E)(struct _fat_ptr,struct _fat_ptr);struct _fat_ptr _T1F;struct _fat_ptr _T20;{struct Cyc_Core_Opt*_T21;void*_T22;_T0=ev;_T1=(int*)_T0;_T2=*_T1;if(_T2!=1)goto _TL14C;_T3=ev;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T23=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T3;_T4=ev;_T5=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T4;_T6=& _T5->f2;_T22=(void**)_T6;_T21=_T23->f4;}{void**p=(void**)_T22;struct Cyc_Core_Opt*s=_T21;{struct Cyc_List_List*_T23;_T7=eff;_T8=(int*)_T7;_T9=*_T8;if(_T9!=0)goto _TL14E;_TA=eff;_TB=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_TC=_TB->f1;_TD=(int*)_TC;_TE=*_TD;if(_TE!=9)goto _TL150;_TF=eff;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T24=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TF;_T23=_T24->f2;}{struct Cyc_List_List*l=_T23;
# 1017
_TL155: if(l!=0)goto _TL153;else{goto _TL154;}
_TL153:{struct _handler_cons _T24;_T10=& _T24;_push_handler(_T10);{int _T25=0;_T11=setjmp(_T24.handler);if(!_T11)goto _TL156;_T25=1;goto _TL157;_TL156: _TL157: if(_T25)goto _TL158;else{goto _TL15A;}_TL15A: _T12=ev;_T13=Cyc_Core_heap_region;_T14=
_check_null(s);_T15=_T14->v;_T16=(struct Cyc_List_List*)_T15;_T17=l;_T18=_T17->hd;Cyc_Unify_occurs(_T12,_T13,_T16,_T18);_T19=p;_T1A=l;
*_T19=_T1A->hd;{int _T26=1;_npop_handler(0);return _T26;}_pop_handler();goto _TL159;_TL158: _T1B=Cyc_Core_get_exn_thrown();{void*_T26=(void*)_T1B;goto _LLA;_LLA:;}_TL159:;}}_T1C=l;
# 1017
l=_T1C->tl;goto _TL155;_TL154: goto _LL5;}_TL150: goto _LL8;_TL14E: _LL8:
# 1031
 return 0;_LL5:;}goto _LL0;}_TL14C: _T1E=Cyc_Warn_impos;{
# 1035
int(*_T23)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))_T1E;_T1D=_T23;}_T1F=_tag_fat("unify_effect_component expects an Evar",sizeof(char),39U);_T20=_tag_fat(0U,sizeof(void*),0);_T1D(_T1F,_T20);_LL0:;}
# 1037
return 0;}
# 1049 "tcutil.cyc"
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;unsigned _T7;int _T8;void*_T9;int _TA;struct Cyc_List_List*_TB;void*_TC;void*_TD;int _TE;struct Cyc_List_List*_TF;void*_T10;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T11;struct Cyc_List_List*_T12;void*_T13;struct Cyc_List_List*_T14;void*_T15;int*_T16;int _T17;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T18;void*_T19;int*_T1A;int _T1B;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1C;struct Cyc_List_List*_T1D;struct Cyc_List_List*_T1E;void*_T1F;int _T20;int _T21;int _T22;int _T23;int _T24;void*_T25;int _T26;void*_T27;void*_T28;struct Cyc_Absyn_Evar_Absyn_Type_struct*_T29;void**_T2A;int _T2B;int _T2C;int _T2D;int _T2E;struct _handler_cons*_T2F;int _T30;void*_T31;struct _RegionHandle*_T32;struct Cyc_Core_Opt*_T33;void*_T34;struct Cyc_List_List*_T35;void*_T36;void**_T37;void*_T38;int _T39;void**_T3A;int _T3B;int _T3C;
# 1052
if(e1!=e2)goto _TL15B;return 1;_TL15B:
 e1=Cyc_Absyn_compress(e1);{struct Cyc_Core_Opt*_T3D;void*_T3E;_T0=e1;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 0: _T3=e1;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;switch(_T7){case 6: _T8=
# 1056
Cyc_Tcutil_type_in_effect(may_constrain_evars,e1,e2);return _T8;case 9: _T9=e1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3F=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_T3E=_T3F->f2;}{struct Cyc_List_List*es=_T3E;
# 1058
_TL162: if(es!=0)goto _TL160;else{goto _TL161;}
_TL160: _TA=may_constrain_evars;_TB=es;_TC=_TB->hd;_TD=e2;_TE=Cyc_Tcutil_subset_effect(_TA,_TC,_TD);if(_TE)goto _TL163;else{goto _TL165;}
_TL165: return 0;_TL163: _TF=es;
# 1058
es=_TF->tl;goto _TL162;_TL161:
# 1061
 return 1;}case 10: _T10=e1;_T11=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T10;_T12=_T11->f2;if(_T12==0)goto _TL166;_T13=e1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3F=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T13;_T14=_T3F->f2;{struct Cyc_List_List _T40=*_T14;_T15=_T40.hd;_T3E=(void*)_T15;}}{void*t=_T3E;
# 1064
void*_T3F=Cyc_Tcutil_rgns_of(t);void*_T40;_T16=(int*)_T3F;_T17=*_T16;if(_T17!=0)goto _TL168;_T18=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3F;_T19=_T18->f1;_T1A=(int*)_T19;_T1B=*_T1A;if(_T1B!=10)goto _TL16A;_T1C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3F;_T1D=_T1C->f2;if(_T1D==0)goto _TL16C;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T41=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3F;_T1E=_T41->f2;{struct Cyc_List_List _T42=*_T1E;_T1F=_T42.hd;_T40=(void*)_T1F;}}{void*t2=_T40;_T21=
# 1069
Cyc_Tcutil_type_in_effect(may_constrain_evars,t2,e2);if(!_T21)goto _TL16E;_T20=1;goto _TL16F;_TL16E: _T23=may_constrain_evars;
if(!_T23)goto _TL170;_T22=Cyc_Unify_unify(t2,Cyc_Absyn_sint_type);goto _TL171;_TL170: _T22=0;_TL171: _T20=_T22;_TL16F:
# 1069
 return _T20;}_TL16C: goto _LL10;_TL16A: goto _LL10;_TL168: _LL10: _T40=_T3F;{void*e=_T40;_T24=
# 1071
Cyc_Tcutil_subset_effect(may_constrain_evars,e,e2);return _T24;};}goto _TL167;_TL166: goto _LLB;_TL167:;default: goto _LLB;};case 2: _T25=e1;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T3F=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T25;_T3E=_T3F->f1;}{struct Cyc_Absyn_Tvar*v=_T3E;_T26=
# 1062
Cyc_Tcutil_variable_in_effect(may_constrain_evars,v,e2);return _T26;}case 1: _T27=e1;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T3F=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T27;_T28=e1;_T29=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T28;_T2A=& _T29->f2;_T3E=(void**)_T2A;_T3D=_T3F->f4;}{void**p=(void**)_T3E;struct Cyc_Core_Opt*s=_T3D;
# 1074
int evar_occurs=Cyc_Tcutil_evar_in_effect(e1,e2);_T2B=may_constrain_evars;
# 1076
if(_T2B)goto _TL172;else{goto _TL174;}_TL174: _T2C=evar_occurs;if(!_T2C)goto _TL172;
return 1;_TL172: _T2D=may_constrain_evars;
# 1080
if(!_T2D)goto _TL175;_T2E=evar_occurs;if(_T2E)goto _TL175;else{goto _TL177;}
_TL177: e2=Cyc_Tcutil_normalize_effect(e2);{struct _handler_cons _T3F;_T2F=& _T3F;_push_handler(_T2F);{int _T40=0;_T30=setjmp(_T3F.handler);if(!_T30)goto _TL178;_T40=1;goto _TL179;_TL178: _TL179: if(_T40)goto _TL17A;else{goto _TL17C;}_TL17C: _T31=e1;_T32=Cyc_Core_heap_region;_T33=
# 1083
_check_null(s);_T34=_T33->v;_T35=(struct Cyc_List_List*)_T34;_T36=e2;Cyc_Unify_occurs(_T31,_T32,_T35,_T36);_T37=p;
*_T37=e2;{int _T41=1;_npop_handler(0);return _T41;}_pop_handler();goto _TL17B;_TL17A: _T38=Cyc_Core_get_exn_thrown();{void*_T41=(void*)_T38;_T39=
# 1088
Cyc_Tcutil_type_in_effect(0,Cyc_Absyn_heap_rgn_type,e2);if(!_T39)goto _TL17D;_T3A=p;
*_T3A=Cyc_Absyn_heap_rgn_type;
return 1;
# 1093
_TL17D: _T3B=Cyc_Tcutil_unify_effect_component(e1,e2);return _T3B;;}_TL17B:;}}goto _TL176;_TL175: _TL176:
# 1097
 return 0;}default: _LLB: _T3C=
Cyc_Tcutil_type_in_effect(may_constrain_evars,e1,e2);return _T3C;};}}
# 1103
static int Cyc_Tcutil_cmp_atomic_effect(void*t1,void*t2){struct _tuple15 _T0;void*_T1;int*_T2;unsigned _T3;void*_T4;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T5;void*_T6;int*_T7;unsigned _T8;int _T9;void*_TA;int*_TB;unsigned _TC;int _TD;void*_TE;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TF;void*_T10;int*_T11;int _T12;void*_T13;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T14;struct Cyc_List_List*_T15;void*_T16;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T17;void*_T18;int*_T19;int _T1A;void*_T1B;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1C;struct Cyc_List_List*_T1D;void*_T1E;struct Cyc_List_List*_T1F;void*_T20;void*_T21;struct Cyc_List_List*_T22;void*_T23;struct _tuple15 _T24;struct _tuple15 _T25;void*_T26;int*_T27;int _T28;void*_T29;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2A;void*_T2B;int*_T2C;int _T2D;void*_T2E;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2F;struct Cyc_List_List*_T30;void*_T31;int*_T32;int _T33;void*_T34;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T35;void*_T36;int*_T37;int _T38;void*_T39;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3A;struct Cyc_List_List*_T3B;void*_T3C;struct Cyc_List_List*_T3D;void*_T3E;void*_T3F;struct Cyc_List_List*_T40;void*_T41;int _T42;int _T43;void*_T44;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T45;struct Cyc_List_List*_T46;void*_T47;int*_T48;int _T49;void*_T4A;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4B;void*_T4C;int*_T4D;int _T4E;void*_T4F;int*_T50;unsigned _T51;void*_T52;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T53;void*_T54;int*_T55;unsigned _T56;int _T57;int _T58;void*_T59;void*_T5A;struct Cyc_Absyn_Tvar*_T5B;int _T5C;struct Cyc_Absyn_Tvar*_T5D;int _T5E;int _T5F;void*_T60;int*_T61;int _T62;void*_T63;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T64;void*_T65;int*_T66;int _T67;void*_T68;int*_T69;int _T6A;void*_T6B;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T6C;void*_T6D;int*_T6E;int _T6F;void*_T70;int*_T71;int _T72;void*_T73;int*_T74;unsigned _T75;void*_T76;void*_T77;int _T78;struct Cyc_Warn_String_Warn_Warg_struct _T79;struct Cyc_Warn_Typ_Warn_Warg_struct _T7A;struct Cyc_Warn_Typ_Warn_Warg_struct _T7B;int(*_T7C)(struct _fat_ptr);void*(*_T7D)(struct _fat_ptr);struct _fat_ptr _T7E;{struct _tuple15 _T7F;
_T7F.f0=Cyc_Absyn_compress(t1);_T7F.f1=Cyc_Absyn_compress(t2);_T0=_T7F;}{struct _tuple15 _T7F=_T0;int _T80;int _T81;struct Cyc_Absyn_Tvar*_T82;struct Cyc_Absyn_Tvar*_T83;void*_T84;void*_T85;_T1=_T7F.f0;_T2=(int*)_T1;_T3=*_T2;switch(_T3){case 0: _T4=_T7F.f0;_T5=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T4;_T6=_T5->f1;_T7=(int*)_T6;_T8=*_T7;switch(_T8){case 6: _T9=- 1;
# 1106
return _T9;case 10: _TA=_T7F.f1;_TB=(int*)_TA;_TC=*_TB;switch(_TC){case 1: _TD=- 1;
# 1112
return _TD;case 0: _TE=_T7F.f1;_TF=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TE;_T10=_TF->f1;_T11=(int*)_T10;_T12=*_T11;if(_T12!=6)goto _TL182;goto _LL9;_TL182: _T13=_T7F.f0;_T14=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T13;_T15=_T14->f2;if(_T15==0)goto _TL184;_T16=_T7F.f1;_T17=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T16;_T18=_T17->f1;_T19=(int*)_T18;_T1A=*_T19;if(_T1A!=10)goto _TL186;_T1B=_T7F.f1;_T1C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T1B;_T1D=_T1C->f2;if(_T1D==0)goto _TL188;_T1E=_T7F.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T86=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T1E;_T1F=_T86->f2;{struct Cyc_List_List _T87=*_T1F;_T20=_T87.hd;_T85=(void*)_T20;}}_T21=_T7F.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T86=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T22=_T86->f2;{struct Cyc_List_List _T87=*_T22;_T23=_T87.hd;_T84=(void*)_T23;}}{void*tt1=_T85;void*tt2=_T84;{struct _tuple15 _T86;
# 1124
_T86.f0=Cyc_Tcutil_rgns_of(tt1);_T86.f1=Cyc_Tcutil_rgns_of(tt2);_T24=_T86;}{struct _tuple15 _T86=_T24;void*_T87;void*_T88;_T88=_T86.f0;_T87=_T86.f1;{void*rtt1=_T88;void*rtt2=_T87;{struct _tuple15 _T89;
_T89.f0=rtt1;_T89.f1=rtt2;_T25=_T89;}{struct _tuple15 _T89=_T25;void*_T8A;void*_T8B;_T26=_T89.f0;_T27=(int*)_T26;_T28=*_T27;if(_T28!=0)goto _TL18A;_T29=_T89.f0;_T2A=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T29;_T2B=_T2A->f1;_T2C=(int*)_T2B;_T2D=*_T2C;if(_T2D!=10)goto _TL18C;_T2E=_T89.f0;_T2F=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2E;_T30=_T2F->f2;if(_T30==0)goto _TL18E;_T31=_T89.f1;_T32=(int*)_T31;_T33=*_T32;if(_T33!=0)goto _TL190;_T34=_T89.f1;_T35=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T34;_T36=_T35->f1;_T37=(int*)_T36;_T38=*_T37;if(_T38!=10)goto _TL192;_T39=_T89.f1;_T3A=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T39;_T3B=_T3A->f2;if(_T3B==0)goto _TL194;_T3C=_T89.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T8C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3C;_T3D=_T8C->f2;{struct Cyc_List_List _T8D=*_T3D;_T3E=_T8D.hd;_T8B=(void*)_T3E;}}_T3F=_T89.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T8C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3F;_T40=_T8C->f2;{struct Cyc_List_List _T8D=*_T40;_T41=_T8D.hd;_T8A=(void*)_T41;}}{void*ev1=_T8B;void*ev2=_T8A;_T42=
# 1127
Cyc_Tcutil_cmp_atomic_effect(ev1,ev2);return _T42;}_TL194: goto _LL1F;_TL192: goto _LL1F;_TL190: goto _LL1F;_TL18E: goto _LL1F;_TL18C: goto _LL1F;_TL18A: _LL1F: _T43=
# 1129
Cyc_Tcutil_cmp_atomic_effect(rtt1,rtt2);return _T43;;}}}}goto _TL189;_TL188: goto _LL17;_TL189: goto _TL187;_TL186: goto _LL17;_TL187: goto _TL185;_TL184: goto _LL17;_TL185:;case 2:
# 1116
 return 1;default: _T44=_T7F.f0;_T45=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T44;_T46=_T45->f2;if(_T46==0)goto _TL196;goto _LL17;_TL196: goto _LL17;};default: _T47=_T7F.f1;_T48=(int*)_T47;_T49=*_T48;if(_T49!=0)goto _TL198;_T4A=_T7F.f1;_T4B=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T4A;_T4C=_T4B->f1;_T4D=(int*)_T4C;_T4E=*_T4D;if(_T4E!=6)goto _TL19A;goto _LL9;_TL19A: goto _LL17;_TL198: goto _LL17;};case 2: _T4F=_T7F.f1;_T50=(int*)_T4F;_T51=*_T50;switch(_T51){case 0: _T52=_T7F.f1;_T53=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T52;_T54=_T53->f1;_T55=(int*)_T54;_T56=*_T55;switch(_T56){case 10: _T57=- 1;
# 1108
return _T57;case 6: goto _LL9;default: goto _LL17;};case 1: _T58=- 1;
# 1110
return _T58;case 2: _T59=_T7F.f0;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T86=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T59;_T83=_T86->f1;}_T5A=_T7F.f1;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T86=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T5A;_T82=_T86->f1;}{struct Cyc_Absyn_Tvar*tv1=_T83;struct Cyc_Absyn_Tvar*tv2=_T82;_T5B=tv1;_T5C=_T5B->identity;_T5D=tv2;_T5E=_T5D->identity;_T5F=_T5C - _T5E;
# 1122
return _T5F;}default: goto _LL17;};default: _T60=_T7F.f1;_T61=(int*)_T60;_T62=*_T61;if(_T62!=0)goto _TL19E;_T63=_T7F.f1;_T64=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T63;_T65=_T64->f1;_T66=(int*)_T65;_T67=*_T66;if(_T67!=6)goto _TL1A0;_LL9:
# 1114
 return 1;_TL1A0: _T68=_T7F.f0;_T69=(int*)_T68;_T6A=*_T69;if(_T6A!=1)goto _TL1A2;_T6B=_T7F.f1;_T6C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T6B;_T6D=_T6C->f1;_T6E=(int*)_T6D;_T6F=*_T6E;if(_T6F!=10)goto _TL1A4;
# 1120
return 1;_TL1A4: goto _LL17;_TL1A2: goto _LL17;_TL19E: _T70=_T7F.f0;_T71=(int*)_T70;_T72=*_T71;if(_T72!=1)goto _TL1A6;_T73=_T7F.f1;_T74=(int*)_T73;_T75=*_T74;switch(_T75){case 2:
# 1118
 return 1;case 1: _T76=_T7F.f0;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T86=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T76;_T81=_T86->f3;}_T77=_T7F.f1;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T86=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T77;_T80=_T86->f3;}{int ev1=_T81;int ev2=_T80;_T78=ev1 - ev2;
# 1132
return _T78;}default: goto _LL17;}goto _TL1A7;_TL1A6: _LL17:{struct Cyc_Warn_String_Warn_Warg_struct _T86;_T86.tag=0;
# 1134
_T86.f1=_tag_fat("Cannot canonicalize effects that contain typedefs",sizeof(char),50U);_T79=_T86;}{struct Cyc_Warn_String_Warn_Warg_struct _T86=_T79;{struct Cyc_Warn_Typ_Warn_Warg_struct _T87;_T87.tag=2;_T87.f1=t1;_T7A=_T87;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T87=_T7A;{struct Cyc_Warn_Typ_Warn_Warg_struct _T88;_T88.tag=2;_T88.f1=t2;_T7B=_T88;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T88=_T7B;void*_T89[3];_T89[0]=& _T86;_T89[1]=& _T87;_T89[2]=& _T88;_T7D=Cyc_Warn_impos2;{int(*_T8A)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T7D;_T7C=_T8A;}_T7E=_tag_fat(_T89,sizeof(void*),3);_T7C(_T7E);}}}_TL1A7:;};}}
# 1139
static int Cyc_Tcutil_uncanonical_effect(void*a){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;unsigned _T7;void*_T8;struct Cyc_List_List*_T9;int _TA;struct Cyc_List_List*_TB;_T0=a;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 0: _T3=a;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;switch(_T7){case 6: goto _LL4;case 10: _T8=a;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_TB=_TC->f2;}{struct Cyc_List_List*l=_TB;_T9=l;_TA=
# 1145
Cyc_List_exists(Cyc_Tcutil_uncanonical_effect,_T9);return _TA;}default: goto _LL9;};case 2: _LL4: goto _LL6;case 1: _LL6:
# 1143
 return 0;default: _LL9:
# 1147
 return 1;};}
# 1153
static int Cyc_Tcutil_canonical_join_effect(void*je){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;int _T7;void*_T8;void*_T9;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TA;struct Cyc_List_List**_TB;struct Cyc_List_List**_TC;struct Cyc_List_List*_TD;int _TE;struct Cyc_List_List**_TF;struct Cyc_List_List**_T10;struct Cyc_List_List*_T11;{void*_T12;_T0=je;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TL1AB;_T3=je;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;if(_T7!=9)goto _TL1AD;_T8=je;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T13=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_T9=je;_TA=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_TB=& _TA->f2;_T12=(struct Cyc_List_List**)_TB;}{struct Cyc_List_List**l=(struct Cyc_List_List**)_T12;_TC=l;_TD=*_TC;_TE=
# 1156
Cyc_List_exists(Cyc_Tcutil_uncanonical_effect,_TD);if(_TE)goto _TL1AF;else{goto _TL1B1;}
_TL1B1: _TF=l;_T10=l;_T11=*_T10;*_TF=Cyc_List_rimp_merge_sort(Cyc_Tcutil_cmp_atomic_effect,_T11);
return 1;_TL1AF: goto _LL0;}_TL1AD: goto _LL3;_TL1AB: _LL3: goto _LL0;_LL0:;}
# 1164
return 0;}
# 1168
static void*Cyc_Tcutil_close_rgnof_effect(void*,struct Cyc_List_List*);
static void*Cyc_Tcutil_close_canonical_join(void*,struct Cyc_List_List*);
static void*Cyc_Tcutil_close_effect(void*eff,struct Cyc_List_List*effconstr){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;unsigned _T7;void*_T8;int _T9;void*_TA;struct Cyc_List_List*_TB;struct Cyc_List_List*_TC;void*_TD;struct Cyc_List_List*_TE;struct Cyc_List_List*_TF;void*_T10;void*_T11;void*_T12;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T13;struct Cyc_List_List*_T14;void*_T15;struct Cyc_List_List*_T16;void*_T17;void*_T18;int*_T19;int _T1A;void*_T1B;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1C;void*_T1D;int*_T1E;int _T1F;void*_T20;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T21;struct Cyc_List_List*_T22;void*_T23;struct Cyc_List_List*_T24;void*_T25;void*_T26;void*_T27;struct Cyc_List_List*_T28;int*_T29;int _T2A;void*_T2B;void*_T2C;void*_T2D;void*_T2E;int _T2F;void*_T30;struct Cyc_List_List*_T31;void*_T32;
eff=Cyc_Tcutil_normalize_effect(eff);{void*_T33;_T0=eff;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TL1B2;_T3=eff;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;switch(_T7){case 9: _T8=eff;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_T33=_T34->f2;}{struct Cyc_List_List*e_lst=_T33;
# 1175
int can=Cyc_Tcutil_canonical_join_effect(eff);_T9=can;
if(!_T9)goto _TL1B5;{
void*t=Cyc_Tcutil_close_canonical_join(eff,effconstr);
if(t==eff)goto _TL1B7;_TA=
_check_null(t);return _TA;_TL1B7:;}goto _TL1B6;_TL1B5: _TL1B6: {
# 1182
struct Cyc_List_List*tmplist=0;{
struct Cyc_List_List*i=e_lst;_TL1BC: if(i!=0)goto _TL1BA;else{goto _TL1BB;}
_TL1BA:{struct Cyc_List_List*_T34=_cycalloc(sizeof(struct Cyc_List_List));_TC=i;_TD=_TC->hd;_TE=effconstr;_T34->hd=Cyc_Tcutil_close_effect(_TD,_TE);
_T34->tl=tmplist;_TB=(struct Cyc_List_List*)_T34;}
# 1184
tmplist=_TB;_TF=i;
# 1183
i=_TF->tl;goto _TL1BC;_TL1BB:;}_T10=
# 1187
Cyc_Absyn_join_eff(tmplist);_T11=Cyc_Tcutil_normalize_effect(_T10);return _T11;}}case 10: _T12=eff;_T13=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T12;_T14=_T13->f2;if(_T14==0)goto _TL1BD;_T15=eff;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T15;_T16=_T34->f2;{struct Cyc_List_List _T35=*_T16;_T17=_T35.hd;_T33=(void*)_T17;}}{void*t2=_T33;
# 1189
void*tt=Cyc_Tcutil_rgns_of(t2);void*_T34;_T18=tt;_T19=(int*)_T18;_T1A=*_T19;if(_T1A!=0)goto _TL1BF;_T1B=tt;_T1C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T1B;_T1D=_T1C->f1;_T1E=(int*)_T1D;_T1F=*_T1E;if(_T1F!=10)goto _TL1C1;_T20=tt;_T21=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T20;_T22=_T21->f2;if(_T22==0)goto _TL1C3;_T23=tt;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T35=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T23;_T24=_T35->f2;{struct Cyc_List_List _T36=*_T24;_T25=_T36.hd;_T34=(void*)_T25;}}{void*t22=_T34;
# 1192
void*to=Cyc_Tcutil_close_rgnof_effect(t2,effconstr);
if(to==0)goto _TL1C5;_T26=to;goto _TL1C6;_TL1C5: _T26=eff;_TL1C6: return _T26;}_TL1C3: goto _LLA;_TL1C1: goto _LLA;_TL1BF: _LLA: _T27=tt;
# 1195
return _T27;;}goto _TL1BE;_TL1BD: goto _LL5;_TL1BE:;default: goto _LL5;}goto _TL1B3;_TL1B2: _LL5:{
# 1198
struct Cyc_List_List*effc=effconstr;_TL1CA: if(effc!=0)goto _TL1C8;else{goto _TL1C9;}
_TL1C8: _T28=effc;{void*_T34=_T28->hd;void*_T35;void*_T36;_T29=(int*)_T34;_T2A=*_T29;if(_T2A!=2)goto _TL1CB;{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T37=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T34;_T2B=_T37->f1;_T36=(void*)_T2B;_T2C=_T37->f2;_T35=(void*)_T2C;}{void*lhs=_T36;void*rhs=_T35;_T2D=
# 1201
Cyc_Tcutil_normalize_effect(lhs);_T2E=eff;_T2F=Cyc_Tcutil_typecmp(_T2D,_T2E);if(_T2F!=0)goto _TL1CD;_T30=
Cyc_Tcutil_close_effect(rhs,effconstr);return _T30;_TL1CD: goto _LLC;}_TL1CB: goto _LLC;_LLC:;}_T31=effc;
# 1198
effc=_T31->tl;goto _TL1CA;_TL1C9:;}_T32=eff;
# 1209
return _T32;_TL1B3:;}}
# 1213
static void*Cyc_Tcutil_close_canonical_join(void*je,struct Cyc_List_List*effconstr){struct Cyc_List_List*_T0;int*_T1;int _T2;void*_T3;void*_T4;void*_T5;int*_T6;int _T7;void*_T8;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T9;void*_TA;int*_TB;int _TC;int _TD;int _TE;void*_TF;struct Cyc_List_List*_T10;void*_T11;{
# 1215
struct Cyc_List_List*effc=effconstr;_TL1D2: if(effc!=0)goto _TL1D0;else{goto _TL1D1;}
_TL1D0: _T0=effc;{void*_T12=_T0->hd;void*_T13;void*_T14;_T1=(int*)_T12;_T2=*_T1;if(_T2!=2)goto _TL1D3;{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T15=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T12;_T3=_T15->f1;_T14=(void*)_T3;_T4=_T15->f2;_T13=(void*)_T4;}{void*lhs=_T14;void*rhs=_T13;
# 1218
lhs=Cyc_Tcutil_normalize_effect(lhs);_T5=lhs;_T6=(int*)_T5;_T7=*_T6;if(_T7!=0)goto _TL1D5;_T8=lhs;_T9=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_TA=_T9->f1;_TB=(int*)_TA;_TC=*_TB;if(_TC!=9)goto _TL1D7;{
# 1221
int can=Cyc_Tcutil_canonical_join_effect(lhs);_TD=can;
if(!_TD)goto _TL1D9;_TE=Cyc_Tcutil_typecmp(lhs,je);if(_TE!=0)goto _TL1D9;_TF=
Cyc_Tcutil_close_effect(rhs,effconstr);return _TF;_TL1D9: goto _LL5;}_TL1D7: goto _LL8;_TL1D5: _LL8: goto _LL5;_LL5: goto _LL0;}_TL1D3: goto _LL0;_LL0:;}_T10=effc;
# 1215
effc=_T10->tl;goto _TL1D2;_TL1D1:;}_T11=je;
# 1234
return _T11;}
# 1237
static void*Cyc_Tcutil_close_rgnof_effect(void*rgnof,struct Cyc_List_List*effconstr){struct Cyc_List_List*_T0;int*_T1;int _T2;void*_T3;void*_T4;int*_T5;int _T6;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T7;void*_T8;int*_T9;int _TA;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TB;struct Cyc_List_List*_TC;struct Cyc_List_List*_TD;void*_TE;int _TF;void*_T10;struct Cyc_List_List*_T11;void*_T12;struct Cyc_List_List*_T13;
_TL1DE: if(effconstr!=0)goto _TL1DC;else{goto _TL1DD;}
_TL1DC: _T0=effconstr;{void*_T14=_T0->hd;void*_T15;void*_T16;_T1=(int*)_T14;_T2=*_T1;if(_T2!=2)goto _TL1DF;{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T17=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T14;_T3=_T17->f1;_T16=(void*)_T3;_T4=_T17->f2;_T15=(void*)_T4;}{void*lhs=_T16;void*rhs=_T15;{
# 1241
void*_T17=Cyc_Absyn_compress(lhs);void*_T18;_T5=(int*)_T17;_T6=*_T5;if(_T6!=0)goto _TL1E1;_T7=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T17;_T8=_T7->f1;_T9=(int*)_T8;_TA=*_T9;if(_TA!=10)goto _TL1E3;_TB=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T17;_TC=_TB->f2;if(_TC==0)goto _TL1E5;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T19=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T17;_TD=_T19->f2;{struct Cyc_List_List _T1A=*_TD;_TE=_T1A.hd;_T18=(void*)_TE;}}{void*t1=_T18;_TF=
# 1243
Cyc_Tcutil_typecmp(t1,rgnof);if(_TF)goto _TL1E7;else{goto _TL1E9;}
_TL1E9: _T10=Cyc_Absyn_compress(rhs);_T11=effconstr;_T12=Cyc_Tcutil_close_effect(_T10,_T11);return _T12;_TL1E7: goto _LL5;}_TL1E5: goto _LL8;_TL1E3: goto _LL8;_TL1E1: _LL8: goto _LL5;_LL5:;}goto _LL0;}_TL1DF: goto _LL0;_LL0:;}_T13=effconstr;
# 1238
effconstr=_T13->tl;goto _TL1DE;_TL1DD:
# 1255
 return 0;}
# 1258
int Cyc_Tcutil_check_eff_in_cap(unsigned loc,int may_constrain_evars,void*eff,struct Cyc_List_List*effconstr,void*cap){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;unsigned _T7;void*_T8;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T9;struct Cyc_List_List*_TA;void*_TB;struct Cyc_List_List*_TC;void*_TD;int*_TE;int _TF;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T10;void*_T11;int*_T12;int _T13;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T14;struct Cyc_List_List*_T15;struct Cyc_List_List*_T16;void*_T17;void*_T18;struct Cyc_List_List*_T19;int _T1A;int _T1B;void*_T1C;unsigned _T1D;int _T1E;struct Cyc_List_List*_T1F;void*_T20;struct Cyc_List_List*_T21;void*_T22;int _T23;struct Cyc_List_List*_T24;int _T25;int _T26;int _T27;void*_T28;void*_T29;
# 1261
eff=Cyc_Absyn_compress(eff);{struct Cyc_List_List*_T2A;void*_T2B;_T0=eff;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TL1EA;_T3=eff;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;switch(_T7){case 10: _T8=eff;_T9=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_TA=_T9->f2;if(_TA==0)goto _TL1ED;_TB=eff;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TB;_TC=_T2C->f2;{struct Cyc_List_List _T2D=*_TC;_TD=_T2D.hd;_T2B=(void*)_TD;}}{void*t1=_T2B;
# 1264
void*_T2C=Cyc_Tcutil_rgns_of(t1);void*_T2D;_TE=(int*)_T2C;_TF=*_TE;if(_TF!=0)goto _TL1EF;_T10=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2C;_T11=_T10->f1;_T12=(int*)_T11;_T13=*_T12;if(_T13!=10)goto _TL1F1;_T14=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2C;_T15=_T14->f2;if(_T15==0)goto _TL1F3;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2E=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2C;_T16=_T2E->f2;{struct Cyc_List_List _T2F=*_T16;_T17=_T2F.hd;_T2D=(void*)_T17;}}{void*t2=_T2D;_T18=
# 1267
Cyc_Absyn_regionsof_eff(t2);_T19=effconstr;{void*e=Cyc_Tcutil_close_effect(_T18,_T19);_T1A=
# 1269
Cyc_Tcutil_subset_effect(may_constrain_evars,e,cap);return _T1A;}}_TL1F3: goto _LLA;_TL1F1: goto _LLA;_TL1EF: _LLA: _T2D=_T2C;{void*e=_T2D;_T1B=
# 1271
Cyc_Tcutil_subset_effect(may_constrain_evars,e,cap);return _T1B;};}goto _TL1EE;_TL1ED: goto _LL5;_TL1EE:;case 9: _T1C=eff;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T1C;_T2A=_T2C->f2;}{struct Cyc_List_List*l=_T2A;
# 1274
_TL1F8: if(l!=0)goto _TL1F6;else{goto _TL1F7;}
# 1276
_TL1F6: _T1D=loc;_T1E=may_constrain_evars;_T1F=l;_T20=_T1F->hd;_T21=effconstr;_T22=cap;_T23=Cyc_Tcutil_check_eff_in_cap(_T1D,_T1E,_T20,_T21,_T22);if(_T23)goto _TL1F9;else{goto _TL1FB;}
_TL1FB: return 0;_TL1F9: _T24=l;
# 1274
l=_T24->tl;goto _TL1F8;_TL1F7:
# 1279
 return 1;}default: goto _LL5;}goto _TL1EB;_TL1EA: _LL5: _T26=
# 1281
Cyc_Tcutil_subset_effect(may_constrain_evars,eff,cap);if(!_T26)goto _TL1FC;_T25=1;goto _TL1FD;_TL1FC: _T27=may_constrain_evars;_T28=
Cyc_Tcutil_close_effect(eff,effconstr);_T29=cap;_T25=Cyc_Tcutil_subset_effect(_T27,_T28,_T29);_TL1FD:
# 1281
 return _T25;_TL1EB:;}}
# 1286
static int Cyc_Tcutil_check_single_constraint(void*e,struct Cyc_List_List*env_ec){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;int _T7;void*_T8;int _T9;struct Cyc_List_List*_TA;struct Cyc_List_List*_TB;int*_TC;int _TD;void*_TE;int _TF;struct Cyc_List_List*_T10;{struct Cyc_List_List*_T11;_T0=e;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TL1FE;_T3=e;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;if(_T7!=9)goto _TL200;_T8=e;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T12=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_T11=_T12->f2;}{struct Cyc_List_List*l=_T11;_T9=
# 1289
Cyc_List_length(l);if(_T9!=1)goto _TL202;_TA=
_check_null(l);e=_TA->hd;goto _TL203;
# 1292
_TL202: return 0;_TL203: goto _LL0;}_TL200: goto _LL3;_TL1FE: _LL3: goto _LL0;_LL0:;}
# 1297
_TL207: if(env_ec!=0)goto _TL205;else{goto _TL206;}
_TL205: _TB=env_ec;{void*_T11=_TB->hd;void*_T12;_TC=(int*)_T11;_TD=*_TC;if(_TD!=0)goto _TL208;{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T13=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T11;_TE=_T13->f1;_T12=(void*)_TE;}{void*e1=_T12;_TF=
# 1300
Cyc_Tcutil_typecmp(e1,e);if(_TF!=0)goto _TL20A;
return 1;_TL20A: goto _LL5;}_TL208: goto _LL5;_LL5:;}_T10=env_ec;
# 1297
env_ec=_T10->tl;goto _TL207;_TL206:
# 1307
 return 0;}struct _tuple19{void*f0;struct Cyc_List_List*f1;};
# 1311
static int Cyc_Tcutil_disjoint_effects(struct _tuple19*e,void*e2){struct _tuple19*_T0;struct Cyc_List_List*_T1;int*_T2;int _T3;void*_T4;void*_T5;struct _tuple15 _T6;void*_T7;int*_T8;int _T9;void*_TA;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TB;void*_TC;int*_TD;int _TE;void*_TF;int*_T10;int _T11;void*_T12;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T13;void*_T14;int*_T15;int _T16;void*_T17;void*_T18;void*_T19;struct Cyc_List_List*_T1A;int _T1B;void*_T1C;struct Cyc_List_List*_T1D;int _T1E;void*_T1F;struct Cyc_List_List*_T20;int _T21;void*_T22;struct Cyc_List_List*_T23;int _T24;void*_T25;void*_T26;int*_T27;int _T28;void*_T29;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2A;void*_T2B;int*_T2C;int _T2D;void*_T2E;int*_T2F;int _T30;void*_T31;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T32;void*_T33;int*_T34;int _T35;void*_T36;int _T37;void*_T38;struct Cyc_List_List*_T39;int _T3A;int _T3B;void*_T3C;struct Cyc_List_List*_T3D;int _T3E;int _T3F;int _T40;int _T41;int _T42;struct Cyc_List_List*_T43;struct Cyc_List_List*_T44;void*_T45;_T0=e;{struct _tuple19 _T46=*_T0;_T45=_T46.f0;_T44=_T46.f1;}{void*e1=_T45;struct Cyc_List_List*env_ec=_T44;
# 1313
_TL20F: if(env_ec!=0)goto _TL20D;else{goto _TL20E;}
_TL20D: _T1=env_ec;{void*_T46=_T1->hd;void*_T47;void*_T48;_T2=(int*)_T46;_T3=*_T2;if(_T3!=1)goto _TL210;{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T49=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T46;_T4=_T49->f1;_T48=(void*)_T4;_T5=_T49->f2;_T47=(void*)_T5;}{void*f1=_T48;void*f2=_T47;{struct _tuple15 _T49;
# 1316
_T49.f0=f1;_T49.f1=f2;_T6=_T49;}{struct _tuple15 _T49=_T6;struct Cyc_List_List*_T4A;void*_T4B;void*_T4C;_T7=_T49.f0;_T8=(int*)_T7;_T9=*_T8;if(_T9!=0)goto _TL212;_TA=_T49.f0;_TB=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_TC=_TB->f1;_TD=(int*)_TC;_TE=*_TD;if(_TE!=9)goto _TL214;_TF=_T49.f1;_T10=(int*)_TF;_T11=*_T10;if(_T11!=0)goto _TL216;_T12=_T49.f1;_T13=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T12;_T14=_T13->f1;_T15=(int*)_T14;_T16=*_T15;if(_T16!=9)goto _TL218;_T17=_T49.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4D=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T17;_T4C=_T4D->f2;}_T18=_T49.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4D=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T18;_T4B=_T4D->f2;}{struct Cyc_List_List*l1=_T4C;struct Cyc_List_List*l2=_T4B;_T19=e1;_T1A=l1;_T1B=
# 1318
Cyc_List_exists_c(Cyc_Tcutil_type_equals,_T19,_T1A);if(_T1B)goto _TL21E;else{goto _TL21D;}_TL21E: _T1C=e2;_T1D=l2;_T1E=Cyc_List_exists_c(Cyc_Tcutil_type_equals,_T1C,_T1D);if(_T1E)goto _TL21C;else{goto _TL21D;}_TL21D: _T1F=e2;_T20=l2;_T21=
Cyc_List_exists_c(Cyc_Tcutil_type_equals,_T1F,_T20);
# 1318
if(_T21)goto _TL21F;else{goto _TL21A;}_TL21F: _T22=e1;_T23=l1;_T24=
Cyc_List_exists_c(Cyc_Tcutil_type_equals,_T22,_T23);
# 1318
if(_T24)goto _TL21C;else{goto _TL21A;}
# 1320
_TL21C: return 1;_TL21A: goto _LL8;}_TL218: goto _LLB;_TL216: _LLB: _T25=_T49.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4D=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T25;_T4C=_T4D->f2;}_T4B=_T49.f1;{struct Cyc_List_List*l1=_T4C;void*ff=_T4B;_T4C=ff;_T4A=l1;goto _LLE;}_TL214: _T26=_T49.f1;_T27=(int*)_T26;_T28=*_T27;if(_T28!=0)goto _TL220;_T29=_T49.f1;_T2A=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T29;_T2B=_T2A->f1;_T2C=(int*)_T2B;_T2D=*_T2C;if(_T2D!=9)goto _TL222;goto _LLD;_TL222: goto _LLF;_TL220: goto _LLF;_TL212: _T2E=_T49.f1;_T2F=(int*)_T2E;_T30=*_T2F;if(_T30!=0)goto _TL224;_T31=_T49.f1;_T32=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T31;_T33=_T32->f1;_T34=(int*)_T33;_T35=*_T34;if(_T35!=9)goto _TL226;_LLD: _T4C=_T49.f0;_T36=_T49.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4D=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T36;_T4A=_T4D->f2;}_LLE:{void*ff=_T4C;struct Cyc_List_List*l1=_T4A;_T37=
# 1324
Cyc_Tcutil_typecmp(e1,ff);if(_T37==0)goto _TL22C;else{goto _TL22B;}_TL22C: _T38=e2;_T39=l1;_T3A=Cyc_List_exists_c(Cyc_Tcutil_type_equals,_T38,_T39);if(_T3A)goto _TL22A;else{goto _TL22B;}_TL22B: _T3B=
Cyc_Tcutil_typecmp(e2,ff);
# 1324
if(_T3B==0)goto _TL22D;else{goto _TL228;}_TL22D: _T3C=e1;_T3D=l1;_T3E=
Cyc_List_exists_c(Cyc_Tcutil_type_equals,_T3C,_T3D);
# 1324
if(_T3E)goto _TL22A;else{goto _TL228;}
# 1326
_TL22A: return 1;_TL228: goto _LL8;}_TL226: goto _LLF;_TL224: _LLF: _T3F=
# 1329
Cyc_Tcutil_typecmp(e1,f1);if(_T3F==0)goto _TL232;else{goto _TL231;}_TL232: _T40=Cyc_Tcutil_typecmp(e2,f2);if(_T40==0)goto _TL230;else{goto _TL231;}_TL231: _T41=
Cyc_Tcutil_typecmp(e1,f2);
# 1329
if(_T41==0)goto _TL233;else{goto _TL22E;}_TL233: _T42=
Cyc_Tcutil_typecmp(e2,f1);
# 1329
if(_T42==0)goto _TL230;else{goto _TL22E;}
# 1331
_TL230: return 1;_TL22E: goto _LL8;_LL8:;}goto _LL3;}_TL210: goto _LL3;_LL3:;}_T43=env_ec;
# 1313
env_ec=_T43->tl;goto _TL20F;_TL20E:
# 1339
 return 0;}}
# 1342
static int Cyc_Tcutil_check_disjoint_constraint(void*e1,void*e2,struct Cyc_List_List*env_ec){struct _tuple15 _T0;void*_T1;int*_T2;int _T3;void*_T4;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T5;void*_T6;int*_T7;int _T8;void*_T9;int*_TA;int _TB;void*_TC;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TD;void*_TE;int*_TF;int _T10;void*_T11;void*_T12;struct _tuple19 _T13;struct Cyc_List_List*_T14;int(*_T15)(int(*)(struct _tuple19*,void*),struct _tuple19*,struct Cyc_List_List*);int(*_T16)(int(*)(void*,void*),void*,struct Cyc_List_List*);struct _tuple19*_T17;struct Cyc_List_List*_T18;int _T19;struct Cyc_List_List*_T1A;void*_T1B;int _T1C;void*_T1D;int*_T1E;int _T1F;void*_T20;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T21;void*_T22;int*_T23;int _T24;void*_T25;int*_T26;int _T27;void*_T28;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T29;void*_T2A;int*_T2B;int _T2C;void*_T2D;struct _tuple19 _T2E;int(*_T2F)(int(*)(struct _tuple19*,void*),struct _tuple19*,struct Cyc_List_List*);int(*_T30)(int(*)(void*,void*),void*,struct Cyc_List_List*);struct _tuple19*_T31;struct Cyc_List_List*_T32;int _T33;struct _tuple19 _T34;struct _tuple19*_T35;void*_T36;int _T37;{struct _tuple15 _T38;
_T38.f0=e1;_T38.f1=e2;_T0=_T38;}{struct _tuple15 _T38=_T0;struct Cyc_List_List*_T39;struct Cyc_List_List*_T3A;_T1=_T38.f0;_T2=(int*)_T1;_T3=*_T2;if(_T3!=0)goto _TL234;_T4=_T38.f0;_T5=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T4;_T6=_T5->f1;_T7=(int*)_T6;_T8=*_T7;if(_T8!=9)goto _TL236;_T9=_T38.f1;_TA=(int*)_T9;_TB=*_TA;if(_TB!=0)goto _TL238;_TC=_T38.f1;_TD=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TC;_TE=_TD->f1;_TF=(int*)_TE;_T10=*_TF;if(_T10!=9)goto _TL23A;_T11=_T38.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3B=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T11;_T3A=_T3B->f2;}_T12=_T38.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3B=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T12;_T39=_T3B->f2;}{struct Cyc_List_List*l1=_T3A;struct Cyc_List_List*l2=_T39;
# 1345
_TL23F: if(l1!=0)goto _TL23D;else{goto _TL23E;}
_TL23D:{struct _tuple19 _T3B;_T14=l1;_T3B.f0=_T14->hd;_T3B.f1=env_ec;_T13=_T3B;}{struct _tuple19 env=_T13;_T16=Cyc_List_forall_c;{
int(*_T3B)(int(*)(struct _tuple19*,void*),struct _tuple19*,struct Cyc_List_List*)=(int(*)(int(*)(struct _tuple19*,void*),struct _tuple19*,struct Cyc_List_List*))_T16;_T15=_T3B;}_T17=& env;_T18=l2;_T19=_T15(Cyc_Tcutil_disjoint_effects,_T17,_T18);if(_T19)goto _TL240;else{goto _TL242;}
_TL242: return 0;_TL240:;}_T1A=l1;
# 1345
l1=_T1A->tl;goto _TL23F;_TL23E:
# 1350
 return 1;}_TL23A: goto _LL3;_TL238: _LL3: _T1B=_T38.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3B=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T1B;_T3A=_T3B->f2;}{struct Cyc_List_List*l2=_T3A;_T1C=
# 1352
Cyc_Tcutil_check_disjoint_constraint(e2,e1,env_ec);return _T1C;}_TL236: _T1D=_T38.f1;_T1E=(int*)_T1D;_T1F=*_T1E;if(_T1F!=0)goto _TL243;_T20=_T38.f1;_T21=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T20;_T22=_T21->f1;_T23=(int*)_T22;_T24=*_T23;if(_T24!=9)goto _TL245;goto _LL5;_TL245: goto _LL7;_TL243: goto _LL7;_TL234: _T25=_T38.f1;_T26=(int*)_T25;_T27=*_T26;if(_T27!=0)goto _TL247;_T28=_T38.f1;_T29=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T28;_T2A=_T29->f1;_T2B=(int*)_T2A;_T2C=*_T2B;if(_T2C!=9)goto _TL249;_LL5: _T2D=_T38.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3B=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T2D;_T3A=_T3B->f2;}{struct Cyc_List_List*l2=_T3A;{struct _tuple19 _T3B;
# 1354
_T3B.f0=e1;_T3B.f1=env_ec;_T2E=_T3B;}{struct _tuple19 env=_T2E;_T30=Cyc_List_forall_c;{
int(*_T3B)(int(*)(struct _tuple19*,void*),struct _tuple19*,struct Cyc_List_List*)=(int(*)(int(*)(struct _tuple19*,void*),struct _tuple19*,struct Cyc_List_List*))_T30;_T2F=_T3B;}_T31=& env;_T32=l2;_T33=_T2F(Cyc_Tcutil_disjoint_effects,_T31,_T32);return _T33;}}_TL249: goto _LL7;_TL247: _LL7:{struct _tuple19 _T3B;
# 1357
_T3B.f0=e1;_T3B.f1=env_ec;_T34=_T3B;}{struct _tuple19 env=_T34;_T35=& env;_T36=e2;_T37=
Cyc_Tcutil_disjoint_effects(_T35,_T36);return _T37;};}}
# 1362
int Cyc_Tcutil_satisfies_effect_constraints(unsigned loc,struct Cyc_List_List*ec,struct Cyc_List_List*env_ec){struct Cyc_List_List*_T0;int*_T1;unsigned _T2;void*_T3;int _T4;void*_T5;struct Cyc_List_List*_T6;int _T7;void*_T8;void*_T9;void*_TA;void*_TB;struct Cyc_List_List*_TC;int _TD;void*_TE;void*_TF;void*_T10;struct Cyc_List_List*_T11;void*_T12;void*_T13;int _T14;struct Cyc_List_List*_T15;
_TL24E: if(ec!=0)goto _TL24C;else{goto _TL24D;}
_TL24C: _T0=ec;{void*_T16=_T0->hd;void*_T17;void*_T18;_T1=(int*)_T16;_T2=*_T1;switch(_T2){case 0:{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T19=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T16;_T3=_T19->f1;_T18=(void*)_T3;}{void*e=_T18;
# 1366
e=Cyc_Absyn_compress(e);_T4=
Cyc_Tcutil_check_single_constraint(e,env_ec);if(_T4)goto _TL250;else{goto _TL252;}_TL252: _T5=
Cyc_Tcutil_close_effect(e,env_ec);_T6=env_ec;_T7=Cyc_Tcutil_check_single_constraint(_T5,_T6);
# 1367
if(_T7)goto _TL250;else{goto _TL253;}
# 1369
_TL253: return 0;_TL250: goto _LL0;}case 1:{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T19=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T16;_T8=_T19->f1;_T18=(void*)_T8;_T9=_T19->f2;_T17=(void*)_T9;}{void*e1=_T18;void*e2=_T17;_TA=
# 1372
Cyc_Tcutil_normalize_effect(e1);_TB=Cyc_Tcutil_normalize_effect(e2);_TC=env_ec;_TD=Cyc_Tcutil_check_disjoint_constraint(_TA,_TB,_TC);if(_TD)goto _TL254;else{goto _TL256;}
_TL256: return 0;_TL254: goto _LL0;}default:{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T19=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T16;_TE=_T19->f1;_T18=(void*)_TE;_TF=_T19->f2;_T17=(void*)_TF;}{void*e1=_T18;void*e2=_T17;_T10=
# 1376
Cyc_Absyn_compress(e1);_T11=env_ec;_T12=Cyc_Tcutil_close_effect(_T10,_T11);_T13=Cyc_Absyn_compress(e2);_T14=Cyc_Tcutil_subset_effect(0,_T12,_T13);if(_T14)goto _TL257;else{goto _TL259;}
_TL259: return 0;_TL257: goto _LL0;}}_LL0:;}_T15=ec;
# 1363
ec=_T15->tl;goto _TL24E;_TL24D:
# 1381
 return 1;}
# 1384
void*Cyc_Tcutil_evaluate_effect(struct Cyc_List_List*ec,void*eff){void*_T0;_T0=
Cyc_Tcutil_close_effect(eff,ec);return _T0;}
# 1388
int Cyc_Tcutil_cmp_effect_constraints(struct Cyc_List_List*ec0,struct Cyc_List_List*ec1){struct Cyc_List_List*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_List_List*_T4;int*_T5;int _T6;void*_T7;int _T8;void*_T9;void*_TA;struct Cyc_List_List*_TB;int*_TC;int _TD;void*_TE;void*_TF;int _T10;int _T11;void*_T12;void*_T13;struct Cyc_List_List*_T14;int*_T15;int _T16;void*_T17;void*_T18;int _T19;int _T1A;struct Cyc_List_List*_T1B;struct Cyc_List_List*_T1C;
# 1390
_TL25D: if(ec0!=0)goto _TL25E;else{goto _TL25C;}_TL25E: if(ec1!=0)goto _TL25B;else{goto _TL25C;}
_TL25B: _T0=ec0;{void*_T1D=_T0->hd;void*_T1E;void*_T1F;_T1=(int*)_T1D;_T2=*_T1;switch(_T2){case 0:{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T20=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T1D;_T3=_T20->f1;_T1F=(void*)_T3;}{void*e01=_T1F;_T4=ec1;{
# 1393
void*_T20=_T4->hd;void*_T21;_T5=(int*)_T20;_T6=*_T5;if(_T6!=0)goto _TL260;{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T22=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T20;_T7=_T22->f1;_T21=(void*)_T7;}{void*e11=_T21;_T8=
# 1395
Cyc_Unify_unify(e01,e11);if(_T8)goto _TL262;else{goto _TL264;}
_TL264: return 1;_TL262: goto _LL7;}_TL260:
# 1399
 return 1;_LL7:;}goto _LL0;}case 1:{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T20=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T1D;_T9=_T20->f1;_T1F=(void*)_T9;_TA=_T20->f2;_T1E=(void*)_TA;}{void*e01=_T1F;void*e02=_T1E;_TB=ec1;{
# 1403
void*_T20=_TB->hd;void*_T21;void*_T22;_TC=(int*)_T20;_TD=*_TC;if(_TD!=1)goto _TL265;{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T23=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T20;_TE=_T23->f1;_T22=(void*)_TE;_TF=_T23->f2;_T21=(void*)_TF;}{void*e11=_T22;void*e12=_T21;_T10=
# 1405
Cyc_Unify_unify(e01,e11);if(_T10)goto _TL26A;else{goto _TL269;}_TL26A: _T11=Cyc_Unify_unify(e02,e12);if(_T11)goto _TL267;else{goto _TL269;}
_TL269: return 1;_TL267: goto _LLC;}_TL265:
# 1409
 return 1;_LLC:;}goto _LL0;}default:{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T20=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T1D;_T12=_T20->f1;_T1F=(void*)_T12;_T13=_T20->f2;_T1E=(void*)_T13;}{void*e01=_T1F;void*e02=_T1E;_T14=ec1;{
# 1413
void*_T20=_T14->hd;void*_T21;void*_T22;_T15=(int*)_T20;_T16=*_T15;if(_T16!=2)goto _TL26B;{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T23=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T20;_T17=_T23->f1;_T22=(void*)_T17;_T18=_T23->f2;_T21=(void*)_T18;}{void*e11=_T22;void*e12=_T21;_T19=
# 1415
Cyc_Unify_unify(e01,e11);if(_T19)goto _TL270;else{goto _TL26F;}_TL270: _T1A=Cyc_Unify_unify(e02,e12);if(_T1A)goto _TL26D;else{goto _TL26F;}
_TL26F: return 1;_TL26D: goto _LL11;}_TL26B:
# 1419
 return 1;_LL11:;}goto _LL0;}}_LL0:;}_T1B=ec0;
# 1390
ec0=_T1B->tl;_T1C=ec1;ec1=_T1C->tl;goto _TL25D;_TL25C:
# 1424
 return 0;}
# 1428
static int Cyc_Tcutil_tycon2int(void*t){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T4;enum Cyc_Absyn_Sign _T5;int _T6;void*_T7;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T8;enum Cyc_Absyn_Size_of _T9;int _TA;void*_TB;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_TC;enum Cyc_Absyn_Size_of _TD;int _TE;void*_TF;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T10;enum Cyc_Absyn_Size_of _T11;int _T12;void*_T13;struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_T14;int _T15;int _T16;void*_T17;const unsigned*_T18;unsigned _T19;int _T1A;_T0=t;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 1: _T3=t;_T4=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T3;_T5=_T4->f1;_T6=(int)_T5;switch(_T6){case Cyc_Absyn_Unsigned: _T7=t;_T8=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T7;_T9=_T8->f2;_TA=(int)_T9;switch(_TA){case Cyc_Absyn_Char_sz:
# 1430
 return 501;case Cyc_Absyn_Short_sz:
# 1433
 return 504;case Cyc_Absyn_Int_sz:
# 1436
 return 507;case Cyc_Absyn_Long_sz:
# 1439
 return 507;case Cyc_Absyn_LongLong_sz:
# 1442
 return 513;default: goto _LL27;};case Cyc_Absyn_Signed: _TB=t;_TC=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_TB;_TD=_TC->f2;_TE=(int)_TD;switch(_TE){case Cyc_Absyn_Char_sz:
# 1431
 return 502;case Cyc_Absyn_Short_sz:
# 1434
 return 505;case Cyc_Absyn_Int_sz:
# 1437
 return 508;case Cyc_Absyn_Long_sz:
# 1440
 return 508;case Cyc_Absyn_LongLong_sz:
# 1443
 return 514;default: goto _LL27;};case Cyc_Absyn_None: _TF=t;_T10=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_TF;_T11=_T10->f2;_T12=(int)_T11;switch(_T12){case Cyc_Absyn_Char_sz:
# 1432
 return 503;case Cyc_Absyn_Short_sz:
# 1435
 return 506;case Cyc_Absyn_Int_sz:
# 1438
 return 509;case Cyc_Absyn_Long_sz:
# 1441
 return 509;case Cyc_Absyn_LongLong_sz:
# 1444
 return 515;default: goto _LL27;};default: goto _LL27;};case 2: _T13=t;_T14=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T13;_T15=_T14->f1;_T16=(int)_T15;switch(_T16){case 0:
 return 516;case 1:
 return 517;case 2:
 return 518;default:
 return 519;};default: _LL27: _T17=t;_T18=(const unsigned*)_T17;_T19=*_T18;_T1A=(int)_T19;
return _T1A;};}
# 1452
static int Cyc_Tcutil_type_case_number(void*t){void*_T0;int*_T1;int _T2;void*_T3;void*_T4;int _T5;int _T6;void*_T7;const unsigned*_T8;unsigned _T9;int _TA;void*_TB;_T0=t;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TL277;_T3=t;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T3;_T4=_TC->f1;_TB=(void*)_T4;}{void*c=_TB;_T5=
# 1454
Cyc_Tcutil_tycon2int(c);_T6=1000 + _T5;return _T6;}_TL277: _T7=t;_T8=(const unsigned*)_T7;_T9=*_T8;_TA=(int)_T9;
return _TA;;}
# 1458
static int Cyc_Tcutil_star_cmp(int(*cmp)(void*,void*),void*a1,void*a2){int _T0;int _T1;
if(a1!=a2)goto _TL279;return 0;_TL279:
 if(a1!=0)goto _TL27B;if(a2==0)goto _TL27B;_T0=- 1;return _T0;_TL27B:
 if(a1==0)goto _TL27D;if(a2!=0)goto _TL27D;return 1;_TL27D: _T1=
cmp(a1,a2);return _T1;}
# 1464
static int Cyc_Tcutil_enumfield_cmp(struct Cyc_Absyn_Enumfield*e1,struct Cyc_Absyn_Enumfield*e2){struct Cyc_Absyn_Enumfield*_T0;struct _tuple1*_T1;struct Cyc_Absyn_Enumfield*_T2;struct _tuple1*_T3;int _T4;int(*_T5)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);int(*_T6)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);struct Cyc_Absyn_Enumfield*_T7;struct Cyc_Absyn_Exp*_T8;struct Cyc_Absyn_Enumfield*_T9;struct Cyc_Absyn_Exp*_TA;int _TB;_T0=e1;_T1=_T0->name;_T2=e2;_T3=_T2->name;{
int qc=Cyc_Absyn_qvar_cmp(_T1,_T3);
if(qc==0)goto _TL27F;_T4=qc;return _T4;_TL27F:{
int(*_TC)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_T5=_TC;}_T6=Cyc_Evexp_uint_exp_cmp;_T7=e1;_T8=_T7->tag;_T9=e2;_TA=_T9->tag;_TB=_T5(_T6,_T8,_TA);return _TB;}}
# 1469
static struct _tuple1*Cyc_Tcutil_get_datatype_qvar(union Cyc_Absyn_DatatypeInfo i){union Cyc_Absyn_DatatypeInfo _T0;struct _union_DatatypeInfo_KnownDatatype _T1;unsigned _T2;union Cyc_Absyn_DatatypeInfo _T3;struct _union_DatatypeInfo_KnownDatatype _T4;struct Cyc_Absyn_Datatypedecl**_T5;struct Cyc_Absyn_Datatypedecl*_T6;struct _tuple1*_T7;union Cyc_Absyn_DatatypeInfo _T8;struct _union_DatatypeInfo_UnknownDatatype _T9;struct Cyc_Absyn_UnknownDatatypeInfo _TA;struct _tuple1*_TB;struct _tuple1*_TC;struct Cyc_Absyn_Datatypedecl*_TD;_T0=i;_T1=_T0.KnownDatatype;_T2=_T1.tag;if(_T2!=2)goto _TL281;_T3=i;_T4=_T3.KnownDatatype;_T5=_T4.val;{struct Cyc_Absyn_Datatypedecl*_TE=*_T5;_TD=_TE;}{struct Cyc_Absyn_Datatypedecl*dd=_TD;_T6=dd;_T7=_T6->name;
# 1471
return _T7;}_TL281: _T8=i;_T9=_T8.UnknownDatatype;_TA=_T9.val;_TC=_TA.name;{struct _tuple1*n=_TC;_TB=n;
return _TB;};}struct _tuple20{struct _tuple1*f0;struct _tuple1*f1;};
# 1475
static struct _tuple20 Cyc_Tcutil_get_datatype_field_qvars(union Cyc_Absyn_DatatypeFieldInfo i){union Cyc_Absyn_DatatypeFieldInfo _T0;struct _union_DatatypeFieldInfo_KnownDatatypefield _T1;unsigned _T2;union Cyc_Absyn_DatatypeFieldInfo _T3;struct _union_DatatypeFieldInfo_KnownDatatypefield _T4;struct _tuple2 _T5;union Cyc_Absyn_DatatypeFieldInfo _T6;struct _union_DatatypeFieldInfo_KnownDatatypefield _T7;struct _tuple2 _T8;struct _tuple20 _T9;struct Cyc_Absyn_Datatypedecl*_TA;struct Cyc_Absyn_Datatypefield*_TB;union Cyc_Absyn_DatatypeFieldInfo _TC;struct _union_DatatypeFieldInfo_UnknownDatatypefield _TD;struct Cyc_Absyn_UnknownDatatypeFieldInfo _TE;union Cyc_Absyn_DatatypeFieldInfo _TF;struct _union_DatatypeFieldInfo_UnknownDatatypefield _T10;struct Cyc_Absyn_UnknownDatatypeFieldInfo _T11;struct _tuple20 _T12;struct _tuple1*_T13;struct _tuple1*_T14;struct Cyc_Absyn_Datatypefield*_T15;struct Cyc_Absyn_Datatypedecl*_T16;_T0=i;_T1=_T0.KnownDatatypefield;_T2=_T1.tag;if(_T2!=2)goto _TL283;_T3=i;_T4=_T3.KnownDatatypefield;_T5=_T4.val;_T16=_T5.f0;_T6=i;_T7=_T6.KnownDatatypefield;_T8=_T7.val;_T15=_T8.f1;{struct Cyc_Absyn_Datatypedecl*dd=_T16;struct Cyc_Absyn_Datatypefield*df=_T15;{struct _tuple20 _T17;_TA=dd;
# 1478
_T17.f0=_TA->name;_TB=df;_T17.f1=_TB->name;_T9=_T17;}return _T9;}_TL283: _TC=i;_TD=_TC.UnknownDatatypefield;_TE=_TD.val;_T14=_TE.datatype_name;_TF=i;_T10=_TF.UnknownDatatypefield;_T11=_T10.val;_T13=_T11.field_name;{struct _tuple1*d=_T14;struct _tuple1*f=_T13;{struct _tuple20 _T17;
# 1480
_T17.f0=d;_T17.f1=f;_T12=_T17;}return _T12;};}struct _tuple21{enum Cyc_Absyn_AggrKind f0;struct _tuple1*f1;};
# 1483
static struct _tuple21 Cyc_Tcutil_get_aggr_kind_and_qvar(union Cyc_Absyn_AggrInfo i){union Cyc_Absyn_AggrInfo _T0;struct _union_AggrInfo_UnknownAggr _T1;unsigned _T2;union Cyc_Absyn_AggrInfo _T3;struct _union_AggrInfo_UnknownAggr _T4;struct _tuple3 _T5;union Cyc_Absyn_AggrInfo _T6;struct _union_AggrInfo_UnknownAggr _T7;struct _tuple3 _T8;struct _tuple21 _T9;union Cyc_Absyn_AggrInfo _TA;struct _union_AggrInfo_KnownAggr _TB;struct Cyc_Absyn_Aggrdecl**_TC;struct _tuple21 _TD;struct Cyc_Absyn_Aggrdecl*_TE;struct Cyc_Absyn_Aggrdecl*_TF;struct Cyc_Absyn_Aggrdecl*_T10;struct _tuple1*_T11;enum Cyc_Absyn_AggrKind _T12;_T0=i;_T1=_T0.UnknownAggr;_T2=_T1.tag;if(_T2!=1)goto _TL285;_T3=i;_T4=_T3.UnknownAggr;_T5=_T4.val;_T12=_T5.f0;_T6=i;_T7=_T6.UnknownAggr;_T8=_T7.val;_T11=_T8.f1;{enum Cyc_Absyn_AggrKind k=_T12;struct _tuple1*n=_T11;{struct _tuple21 _T13;
# 1485
_T13.f0=k;_T13.f1=n;_T9=_T13;}return _T9;}_TL285: _TA=i;_TB=_TA.KnownAggr;_TC=_TB.val;{struct Cyc_Absyn_Aggrdecl*_T13=*_TC;_T10=_T13;}{struct Cyc_Absyn_Aggrdecl*ad=_T10;{struct _tuple21 _T13;_TE=ad;
_T13.f0=_TE->kind;_TF=ad;_T13.f1=_TF->name;_TD=_T13;}return _TD;};}
# 1489
int Cyc_Tcutil_tycon_cmp(void*t1,void*t2){int _T0;struct _tuple15 _T1;void*_T2;int*_T3;unsigned _T4;void*_T5;int*_T6;int _T7;void*_T8;void*_T9;int _TA;void*_TB;int*_TC;int _TD;void*_TE;void*_TF;int _T10;void*_T11;int*_T12;int _T13;void*_T14;void*_T15;int(*_T16)(int(*)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct Cyc_List_List*);int(*_T17)(int(*)(void*,void*),struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_List_List*_T18;struct Cyc_List_List*_T19;int _T1A;void*_T1B;int*_T1C;int _T1D;void*_T1E;void*_T1F;struct _tuple1*_T20;struct _tuple1*_T21;int _T22;void*_T23;int*_T24;int _T25;void*_T26;void*_T27;int _T28;int _T29;void*_T2A;int*_T2B;int _T2C;void*_T2D;void*_T2E;int _T2F;enum Cyc_Absyn_AggrKind _T30;int _T31;enum Cyc_Absyn_AggrKind _T32;int _T33;int _T34;
if(t1!=t2)goto _TL287;return 0;_TL287: {
int i1=Cyc_Tcutil_tycon2int(t1);
int i2=Cyc_Tcutil_tycon2int(t2);
if(i1==i2)goto _TL289;_T0=i1 - i2;return _T0;_TL289:{struct _tuple15 _T35;
# 1495
_T35.f0=t1;_T35.f1=t2;_T1=_T35;}{struct _tuple15 _T35=_T1;union Cyc_Absyn_AggrInfo _T36;union Cyc_Absyn_AggrInfo _T37;union Cyc_Absyn_DatatypeFieldInfo _T38;union Cyc_Absyn_DatatypeFieldInfo _T39;union Cyc_Absyn_DatatypeInfo _T3A;union Cyc_Absyn_DatatypeInfo _T3B;struct Cyc_List_List*_T3C;struct Cyc_List_List*_T3D;struct _fat_ptr _T3E;struct _fat_ptr _T3F;struct _tuple1*_T40;struct _tuple1*_T41;_T2=_T35.f0;_T3=(int*)_T2;_T4=*_T3;switch(_T4){case 19: _T5=_T35.f1;_T6=(int*)_T5;_T7=*_T6;if(_T7!=19)goto _TL28C;_T8=_T35.f0;{struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_T8;_T41=_T42->f1;}_T9=_T35.f1;{struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_T9;_T40=_T42->f1;}{struct _tuple1*n1=_T41;struct _tuple1*n2=_T40;_TA=
Cyc_Absyn_qvar_cmp(n1,n2);return _TA;}_TL28C: goto _LLD;case 21: _TB=_T35.f1;_TC=(int*)_TB;_TD=*_TC;if(_TD!=21)goto _TL28E;_TE=_T35.f0;{struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_TE;_T3F=_T42->f1;}_TF=_T35.f1;{struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_TF;_T3E=_T42->f1;}{struct _fat_ptr s1=_T3F;struct _fat_ptr s2=_T3E;_T10=
Cyc_strcmp(s1,s2);return _T10;}_TL28E: goto _LLD;case 20: _T11=_T35.f1;_T12=(int*)_T11;_T13=*_T12;if(_T13!=20)goto _TL290;_T14=_T35.f0;{struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_T14;_T3D=_T42->f1;}_T15=_T35.f1;{struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_T15;_T3C=_T42->f1;}{struct Cyc_List_List*fs1=_T3D;struct Cyc_List_List*fs2=_T3C;_T17=Cyc_List_list_cmp;{
# 1499
int(*_T42)(int(*)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct Cyc_List_List*)=(int(*)(int(*)(struct Cyc_Absyn_Enumfield*,struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*,struct Cyc_List_List*))_T17;_T16=_T42;}_T18=fs1;_T19=fs2;_T1A=_T16(Cyc_Tcutil_enumfield_cmp,_T18,_T19);return _T1A;}_TL290: goto _LLD;case 22: _T1B=_T35.f1;_T1C=(int*)_T1B;_T1D=*_T1C;if(_T1D!=22)goto _TL292;_T1E=_T35.f0;{struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_T1E;_T3B=_T42->f1;}_T1F=_T35.f1;{struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_T1F;_T3A=_T42->f1;}{union Cyc_Absyn_DatatypeInfo info1=_T3B;union Cyc_Absyn_DatatypeInfo info2=_T3A;_T20=
# 1501
Cyc_Tcutil_get_datatype_qvar(info1);_T21=Cyc_Tcutil_get_datatype_qvar(info2);_T22=Cyc_Absyn_qvar_cmp(_T20,_T21);return _T22;}_TL292: goto _LLD;case 23: _T23=_T35.f1;_T24=(int*)_T23;_T25=*_T24;if(_T25!=23)goto _TL294;_T26=_T35.f0;{struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_T26;_T39=_T42->f1;}_T27=_T35.f1;{struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_T27;_T38=_T42->f1;}{union Cyc_Absyn_DatatypeFieldInfo info1=_T39;union Cyc_Absyn_DatatypeFieldInfo info2=_T38;
# 1503
struct _tuple20 _T42=Cyc_Tcutil_get_datatype_field_qvars(info1);struct _tuple1*_T43;struct _tuple1*_T44;_T44=_T42.f0;_T43=_T42.f1;{struct _tuple1*d1=_T44;struct _tuple1*f1=_T43;
struct _tuple20 _T45=Cyc_Tcutil_get_datatype_field_qvars(info2);struct _tuple1*_T46;struct _tuple1*_T47;_T47=_T45.f0;_T46=_T45.f1;{struct _tuple1*d2=_T47;struct _tuple1*f2=_T46;
int c=Cyc_Absyn_qvar_cmp(d1,d2);
if(c==0)goto _TL296;_T28=c;return _T28;_TL296: _T29=
Cyc_Absyn_qvar_cmp(f1,f2);return _T29;}}}_TL294: goto _LLD;case 24: _T2A=_T35.f1;_T2B=(int*)_T2A;_T2C=*_T2B;if(_T2C!=24)goto _TL298;_T2D=_T35.f0;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T2D;_T37=_T42->f1;}_T2E=_T35.f1;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T42=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T2E;_T36=_T42->f1;}{union Cyc_Absyn_AggrInfo info1=_T37;union Cyc_Absyn_AggrInfo info2=_T36;
# 1509
struct _tuple21 _T42=Cyc_Tcutil_get_aggr_kind_and_qvar(info1);struct _tuple1*_T43;enum Cyc_Absyn_AggrKind _T44;_T44=_T42.f0;_T43=_T42.f1;{enum Cyc_Absyn_AggrKind k1=_T44;struct _tuple1*q1=_T43;
struct _tuple21 _T45=Cyc_Tcutil_get_aggr_kind_and_qvar(info2);struct _tuple1*_T46;enum Cyc_Absyn_AggrKind _T47;_T47=_T45.f0;_T46=_T45.f1;{enum Cyc_Absyn_AggrKind k2=_T47;struct _tuple1*q2=_T46;
int c=Cyc_Absyn_qvar_cmp(q1,q2);
if(c==0)goto _TL29A;_T2F=c;return _T2F;_TL29A: _T30=k1;_T31=(int)_T30;_T32=k2;_T33=(int)_T32;_T34=_T31 - _T33;
return _T34;}}}_TL298: goto _LLD;default: _LLD:
 return 0;};}}}
# 1518
int Cyc_Tcutil_tqual_cmp(struct Cyc_Absyn_Tqual tq1,struct Cyc_Absyn_Tqual tq2){struct Cyc_Absyn_Tqual _T0;int _T1;struct Cyc_Absyn_Tqual _T2;int _T3;int _T4;int _T5;struct Cyc_Absyn_Tqual _T6;int _T7;int _T8;struct Cyc_Absyn_Tqual _T9;int _TA;struct Cyc_Absyn_Tqual _TB;int _TC;int _TD;int _TE;struct Cyc_Absyn_Tqual _TF;int _T10;int _T11;int _T12;_T0=tq1;_T1=_T0.real_const;_T2=tq1;_T3=_T2.q_volatile;_T4=_T3 << 1;_T5=_T1 + _T4;_T6=tq1;_T7=_T6.q_restrict;_T8=_T7 << 2;{
int i1=_T5 + _T8;_T9=tq2;_TA=_T9.real_const;_TB=tq2;_TC=_TB.q_volatile;_TD=_TC << 1;_TE=_TA + _TD;_TF=tq2;_T10=_TF.q_restrict;_T11=_T10 << 2;{
int i2=_TE + _T11;_T12=
Cyc_Core_intcmp(i1,i2);return _T12;}}}
# 1523
static int Cyc_Tcutil_tqual_type_cmp(struct _tuple14*tqt1,struct _tuple14*tqt2){struct _tuple14*_T0;struct _tuple14*_T1;int _T2;int _T3;void*_T4;struct Cyc_Absyn_Tqual _T5;_T0=tqt1;{struct _tuple14 _T6=*_T0;_T5=_T6.f0;_T4=_T6.f1;}{struct Cyc_Absyn_Tqual tq1=_T5;void*t1=_T4;void*_T6;struct Cyc_Absyn_Tqual _T7;_T1=tqt2;{struct _tuple14 _T8=*_T1;_T7=_T8.f0;_T6=_T8.f1;}{struct Cyc_Absyn_Tqual tq2=_T7;void*t2=_T6;
# 1526
int tqc=Cyc_Tcutil_tqual_cmp(tq1,tq2);
if(tqc==0)goto _TL29C;_T2=tqc;return _T2;_TL29C: _T3=
Cyc_Tcutil_typecmp(t1,t2);return _T3;}}}
# 1531
int Cyc_Tcutil_aggrfield_cmp(struct Cyc_Absyn_Aggrfield*f1,struct Cyc_Absyn_Aggrfield*f2){struct Cyc_Absyn_Aggrfield*_T0;struct _fat_ptr*_T1;struct Cyc_Absyn_Aggrfield*_T2;struct _fat_ptr*_T3;int _T4;struct Cyc_Absyn_Aggrfield*_T5;struct Cyc_Absyn_Tqual _T6;struct Cyc_Absyn_Aggrfield*_T7;struct Cyc_Absyn_Tqual _T8;int _T9;struct Cyc_Absyn_Aggrfield*_TA;void*_TB;struct Cyc_Absyn_Aggrfield*_TC;void*_TD;int _TE;int(*_TF)(void*,void*);struct Cyc_Absyn_Aggrfield*_T10;struct Cyc_List_List*_T11;struct Cyc_Absyn_Aggrfield*_T12;struct Cyc_List_List*_T13;int _T14;int(*_T15)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);int(*_T16)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);struct Cyc_Absyn_Aggrfield*_T17;struct Cyc_Absyn_Exp*_T18;struct Cyc_Absyn_Aggrfield*_T19;struct Cyc_Absyn_Exp*_T1A;int _T1B;int(*_T1C)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);int(*_T1D)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);struct Cyc_Absyn_Aggrfield*_T1E;struct Cyc_Absyn_Exp*_T1F;struct Cyc_Absyn_Aggrfield*_T20;struct Cyc_Absyn_Exp*_T21;int _T22;_T0=f1;_T1=_T0->name;_T2=f2;_T3=_T2->name;{
int zsc=Cyc_strptrcmp(_T1,_T3);
if(zsc==0)goto _TL29E;_T4=zsc;return _T4;_TL29E: _T5=f1;_T6=_T5->tq;_T7=f2;_T8=_T7->tq;{
int tqc=Cyc_Tcutil_tqual_cmp(_T6,_T8);
if(tqc==0)goto _TL2A0;_T9=tqc;return _T9;_TL2A0: _TA=f1;_TB=_TA->type;_TC=f2;_TD=_TC->type;{
int tc=Cyc_Tcutil_typecmp(_TB,_TD);
if(tc==0)goto _TL2A2;_TE=tc;return _TE;_TL2A2: _TF=Cyc_Atts_attribute_cmp;_T10=f1;_T11=_T10->attributes;_T12=f2;_T13=_T12->attributes;{
int ac=Cyc_List_list_cmp(_TF,_T11,_T13);
if(ac==0)goto _TL2A4;_T14=ac;return _T14;_TL2A4:{
int(*_T23)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_T15=_T23;}_T16=Cyc_Evexp_uint_exp_cmp;_T17=f1;_T18=_T17->width;_T19=f2;_T1A=_T19->width;ac=_T15(_T16,_T18,_T1A);
if(ac==0)goto _TL2A6;_T1B=ac;return _T1B;_TL2A6:{
int(*_T23)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_T1C=_T23;}_T1D=Cyc_Evexp_uint_exp_cmp;_T1E=f1;_T1F=_T1E->requires_clause;_T20=f2;_T21=_T20->requires_clause;_T22=_T1C(_T1D,_T1F,_T21);return _T22;}}}}}
# 1548
int Cyc_Tcutil_typecmp(void*t1,void*t2){int _T0;int _T1;int _T2;struct _tuple15 _T3;void*_T4;int*_T5;unsigned _T6;void*_T7;int*_T8;int _T9;void*_TA;void*_TB;void*_TC;void*_TD;int _TE;struct Cyc_List_List*_TF;struct Cyc_List_List*_T10;int _T11;void*_T12;int*_T13;int _T14;void*_T15;int _T16;void*_T17;int _T18;int _T19;void*_T1A;int*_T1B;int _T1C;void*_T1D;void*_T1E;struct Cyc_Absyn_Tvar*_T1F;int _T20;struct Cyc_Absyn_Tvar*_T21;int _T22;int _T23;void*_T24;int*_T25;int _T26;void*_T27;struct Cyc_Absyn_PtrInfo _T28;struct Cyc_Absyn_PtrInfo _T29;struct Cyc_Absyn_PtrInfo _T2A;struct Cyc_Absyn_PtrAtts _T2B;struct Cyc_Absyn_PtrInfo _T2C;struct Cyc_Absyn_PtrAtts _T2D;struct Cyc_Absyn_PtrInfo _T2E;struct Cyc_Absyn_PtrAtts _T2F;struct Cyc_Absyn_PtrInfo _T30;struct Cyc_Absyn_PtrAtts _T31;struct Cyc_Absyn_PtrInfo _T32;struct Cyc_Absyn_PtrAtts _T33;struct Cyc_Absyn_PtrInfo _T34;struct Cyc_Absyn_PtrAtts _T35;void*_T36;struct Cyc_Absyn_PtrInfo _T37;struct Cyc_Absyn_PtrInfo _T38;struct Cyc_Absyn_PtrInfo _T39;struct Cyc_Absyn_PtrAtts _T3A;struct Cyc_Absyn_PtrInfo _T3B;struct Cyc_Absyn_PtrAtts _T3C;struct Cyc_Absyn_PtrInfo _T3D;struct Cyc_Absyn_PtrAtts _T3E;struct Cyc_Absyn_PtrInfo _T3F;struct Cyc_Absyn_PtrAtts _T40;struct Cyc_Absyn_PtrInfo _T41;struct Cyc_Absyn_PtrAtts _T42;struct Cyc_Absyn_PtrInfo _T43;struct Cyc_Absyn_PtrAtts _T44;int _T45;int _T46;int _T47;int _T48;int _T49;int _T4A;int _T4B;int _T4C;void*_T4D;int*_T4E;int _T4F;void*_T50;struct Cyc_Absyn_ArrayInfo _T51;struct Cyc_Absyn_ArrayInfo _T52;struct Cyc_Absyn_ArrayInfo _T53;struct Cyc_Absyn_ArrayInfo _T54;void*_T55;struct Cyc_Absyn_ArrayInfo _T56;struct Cyc_Absyn_ArrayInfo _T57;struct Cyc_Absyn_ArrayInfo _T58;struct Cyc_Absyn_ArrayInfo _T59;int _T5A;int _T5B;int _T5C;struct Cyc_Warn_String_Warn_Warg_struct _T5D;int(*_T5E)(struct _fat_ptr);void*(*_T5F)(struct _fat_ptr);struct _fat_ptr _T60;int(*_T61)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);int(*_T62)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);struct Cyc_Absyn_Exp*_T63;struct Cyc_Absyn_Exp*_T64;int _T65;void*_T66;int*_T67;int _T68;void*_T69;void*_T6A;int _T6B;struct Cyc_Absyn_FnInfo _T6C;void*_T6D;struct Cyc_Absyn_FnInfo _T6E;void*_T6F;int _T70;struct Cyc_Absyn_FnInfo _T71;struct Cyc_Absyn_Tqual _T72;struct Cyc_Absyn_FnInfo _T73;struct Cyc_Absyn_Tqual _T74;int _T75;struct Cyc_Absyn_FnInfo _T76;struct Cyc_Absyn_FnInfo _T77;struct Cyc_List_List*_T78;void*_T79;struct _tuple9*_T7A;struct Cyc_List_List*_T7B;void*_T7C;struct _tuple9*_T7D;int _T7E;int _T7F;struct Cyc_List_List*_T80;struct Cyc_List_List*_T81;int _T82;struct Cyc_Absyn_FnInfo _T83;int _T84;struct Cyc_Absyn_FnInfo _T85;int _T86;struct Cyc_Absyn_FnInfo _T87;int _T88;struct Cyc_Absyn_FnInfo _T89;int _T8A;int _T8B;struct Cyc_Absyn_FnInfo _T8C;struct Cyc_Absyn_VarargInfo*_T8D;struct Cyc_Absyn_FnInfo _T8E;struct Cyc_Absyn_VarargInfo*_T8F;struct Cyc_Absyn_FnInfo _T90;struct Cyc_Absyn_VarargInfo*_T91;struct Cyc_Absyn_FnInfo _T92;struct Cyc_Absyn_VarargInfo*_T93;int _T94;struct Cyc_Absyn_FnInfo _T95;struct Cyc_Absyn_VarargInfo*_T96;struct Cyc_Absyn_FnInfo _T97;struct Cyc_Absyn_VarargInfo*_T98;struct Cyc_Absyn_FnInfo _T99;struct Cyc_Absyn_VarargInfo*_T9A;struct Cyc_Absyn_Tqual _T9B;struct Cyc_Absyn_FnInfo _T9C;struct Cyc_Absyn_VarargInfo*_T9D;struct Cyc_Absyn_Tqual _T9E;int _T9F;struct Cyc_Absyn_FnInfo _TA0;struct Cyc_Absyn_VarargInfo*_TA1;void*_TA2;struct Cyc_Absyn_FnInfo _TA3;struct Cyc_Absyn_VarargInfo*_TA4;void*_TA5;int _TA6;struct Cyc_Absyn_FnInfo _TA7;struct Cyc_Absyn_VarargInfo*_TA8;int _TA9;struct Cyc_Absyn_FnInfo _TAA;struct Cyc_Absyn_VarargInfo*_TAB;int _TAC;struct Cyc_Absyn_FnInfo _TAD;struct Cyc_Absyn_VarargInfo*_TAE;int _TAF;struct Cyc_Absyn_FnInfo _TB0;struct Cyc_Absyn_VarargInfo*_TB1;int _TB2;int _TB3;struct Cyc_Absyn_FnInfo _TB4;void*_TB5;struct Cyc_Absyn_FnInfo _TB6;void*_TB7;int _TB8;struct Cyc_Absyn_FnInfo _TB9;struct Cyc_List_List*_TBA;struct Cyc_Absyn_FnInfo _TBB;struct Cyc_List_List*_TBC;int _TBD;int(*_TBE)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);int(*_TBF)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);struct Cyc_Absyn_FnInfo _TC0;struct Cyc_Absyn_Exp*_TC1;struct Cyc_Absyn_FnInfo _TC2;struct Cyc_Absyn_Exp*_TC3;int _TC4;int(*_TC5)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);int(*_TC6)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);struct Cyc_Absyn_FnInfo _TC7;struct Cyc_Absyn_Exp*_TC8;struct Cyc_Absyn_FnInfo _TC9;struct Cyc_Absyn_Exp*_TCA;int _TCB;int(*_TCC)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);int(*_TCD)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);struct Cyc_Absyn_FnInfo _TCE;struct Cyc_Absyn_Exp*_TCF;struct Cyc_Absyn_FnInfo _TD0;struct Cyc_Absyn_Exp*_TD1;int _TD2;struct Cyc_Absyn_FnInfo _TD3;struct Cyc_List_List*_TD4;struct Cyc_Absyn_FnInfo _TD5;struct Cyc_List_List*_TD6;int _TD7;struct Cyc_Warn_String_Warn_Warg_struct _TD8;int(*_TD9)(struct _fat_ptr);void*(*_TDA)(struct _fat_ptr);struct _fat_ptr _TDB;void*_TDC;int*_TDD;int _TDE;void*_TDF;void*_TE0;enum Cyc_Absyn_AggrKind _TE1;int _TE2;enum Cyc_Absyn_AggrKind _TE3;int _TE4;int _TE5;enum Cyc_Absyn_AggrKind _TE6;int _TE7;int(*_TE8)(int(*)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*,struct Cyc_List_List*);int(*_TE9)(int(*)(void*,void*),struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_List_List*_TEA;struct Cyc_List_List*_TEB;int _TEC;void*_TED;int*_TEE;int _TEF;void*_TF0;void*_TF1;void*_TF2;int*_TF3;int _TF4;void*_TF5;void*_TF6;int _TF7;void*_TF8;int*_TF9;int _TFA;void*_TFB;void*_TFC;void*_TFD;void*_TFE;struct Cyc_Warn_String_Warn_Warg_struct _TFF;int(*_T100)(struct _fat_ptr);void*(*_T101)(struct _fat_ptr);struct _fat_ptr _T102;int _T103;struct Cyc_List_List*_T104;struct Cyc_List_List*_T105;int _T106;void*_T107;int*_T108;int _T109;void*_T10A;void*_T10B;struct _fat_ptr _T10C;void*_T10D;void*_T10E;unsigned long _T10F;struct _fat_ptr _T110;void*_T111;void*_T112;unsigned long _T113;int _T114;struct Cyc_Warn_String_Warn_Warg_struct _T115;int(*_T116)(struct _fat_ptr);void*(*_T117)(struct _fat_ptr);struct _fat_ptr _T118;
t1=Cyc_Absyn_compress(t1);
t2=Cyc_Absyn_compress(t2);
if(t1!=t2)goto _TL2A8;return 0;_TL2A8: _T0=
Cyc_Tcutil_type_case_number(t1);_T1=Cyc_Tcutil_type_case_number(t2);{int shallowcmp=Cyc_Core_intcmp(_T0,_T1);
if(shallowcmp==0)goto _TL2AA;_T2=shallowcmp;
return _T2;_TL2AA:{struct _tuple15 _T119;
# 1557
_T119.f0=t2;_T119.f1=t1;_T3=_T119;}{struct _tuple15 _T119=_T3;const char*_T11A;int _T11B;const char*_T11C;int _T11D;struct Cyc_List_List*_T11E;struct _tuple1*_T11F;struct Cyc_List_List*_T120;struct _tuple1*_T121;enum Cyc_Absyn_AggrKind _T122;enum Cyc_Absyn_AggrKind _T123;struct Cyc_Absyn_FnInfo _T124;struct Cyc_Absyn_FnInfo _T125;struct Cyc_Absyn_Exp*_T126;void*_T127;void*_T128;void*_T129;void*_T12A;void*_T12B;void*_T12C;struct Cyc_Absyn_Tqual _T12D;void*_T12E;void*_T12F;void*_T130;void*_T131;struct Cyc_Absyn_Tqual _T132;void*_T133;void*_T134;void*_T135;void*_T136;_T4=_T119.f0;_T5=(int*)_T4;_T6=*_T5;switch(_T6){case 0: _T7=_T119.f1;_T8=(int*)_T7;_T9=*_T8;if(_T9!=0)goto _TL2AD;_TA=_T119.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA;_TB=_T137->f1;_T136=(void*)_TB;_T135=_T137->f2;}_TC=_T119.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TC;_TD=_T137->f1;_T134=(void*)_TD;_T133=_T137->f2;}{void*c1=_T136;struct Cyc_List_List*ts1=_T135;void*c2=_T134;struct Cyc_List_List*ts2=_T133;
# 1559
int c=Cyc_Tcutil_tycon_cmp(c1,c2);
if(c==0)goto _TL2AF;_TE=c;return _TE;_TL2AF: _TF=ts1;_T10=ts2;_T11=
Cyc_List_list_cmp(Cyc_Tcutil_typecmp,_TF,_T10);return _T11;}_TL2AD: goto _LL17;case 1: _T12=_T119.f1;_T13=(int*)_T12;_T14=*_T13;if(_T14!=1)goto _TL2B1;_T15=t1;_T16=(int)_T15;_T17=t2;_T18=(int)_T17;_T19=_T16 - _T18;
# 1563
return _T19;_TL2B1: goto _LL17;case 2: _T1A=_T119.f1;_T1B=(int*)_T1A;_T1C=*_T1B;if(_T1C!=2)goto _TL2B3;_T1D=_T119.f0;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T1D;_T136=_T137->f1;}_T1E=_T119.f1;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T1E;_T135=_T137->f1;}{struct Cyc_Absyn_Tvar*tv2=_T136;struct Cyc_Absyn_Tvar*tv1=_T135;_T1F=tv1;_T20=_T1F->identity;_T21=tv2;_T22=_T21->identity;_T23=
# 1568
Cyc_Core_intcmp(_T20,_T22);return _T23;}_TL2B3: goto _LL17;case 4: _T24=_T119.f1;_T25=(int*)_T24;_T26=*_T25;if(_T26!=4)goto _TL2B5;_T27=_T119.f0;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T27;_T28=_T137->f1;_T136=_T28.elt_type;_T29=_T137->f1;_T132=_T29.elt_tq;_T2A=_T137->f1;_T2B=_T2A.ptr_atts;_T135=_T2B.eff;_T2C=_T137->f1;_T2D=_T2C.ptr_atts;_T134=_T2D.nullable;_T2E=_T137->f1;_T2F=_T2E.ptr_atts;_T133=_T2F.bounds;_T30=_T137->f1;_T31=_T30.ptr_atts;_T131=_T31.zero_term;_T32=_T137->f1;_T33=_T32.ptr_atts;_T130=_T33.autoreleased;_T34=_T137->f1;_T35=_T34.ptr_atts;_T12F=_T35.aqual;}_T36=_T119.f1;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T36;_T37=_T137->f1;_T12E=_T37.elt_type;_T38=_T137->f1;_T12D=_T38.elt_tq;_T39=_T137->f1;_T3A=_T39.ptr_atts;_T12C=_T3A.eff;_T3B=_T137->f1;_T3C=_T3B.ptr_atts;_T12B=_T3C.nullable;_T3D=_T137->f1;_T3E=_T3D.ptr_atts;_T12A=_T3E.bounds;_T3F=_T137->f1;_T40=_T3F.ptr_atts;_T129=_T40.zero_term;_T41=_T137->f1;_T42=_T41.ptr_atts;_T128=_T42.autoreleased;_T43=_T137->f1;_T44=_T43.ptr_atts;_T127=_T44.aqual;}{void*t2a=_T136;struct Cyc_Absyn_Tqual tqual2a=_T132;void*rgn2=_T135;void*null2a=_T134;void*b2=_T133;void*zt2=_T131;void*rel2=_T130;void*aq2=_T12F;void*t1a=_T12E;struct Cyc_Absyn_Tqual tqual1a=_T12D;void*rgn1=_T12C;void*null1a=_T12B;void*b1=_T12A;void*zt1=_T129;void*rel1=_T128;void*aq1=_T127;
# 1572
int etc=Cyc_Tcutil_typecmp(t1a,t2a);
if(etc==0)goto _TL2B7;_T45=etc;return _T45;_TL2B7: {
int rc=Cyc_Tcutil_typecmp(rgn1,rgn2);
if(rc==0)goto _TL2B9;_T46=rc;return _T46;_TL2B9: {
int tqc=Cyc_Tcutil_tqual_cmp(tqual1a,tqual2a);
if(tqc==0)goto _TL2BB;_T47=tqc;return _T47;_TL2BB: {
int cc=Cyc_Tcutil_typecmp(b1,b2);
if(cc==0)goto _TL2BD;_T48=cc;return _T48;_TL2BD: {
int zc=Cyc_Tcutil_typecmp(zt1,zt2);
if(zc==0)goto _TL2BF;_T49=zc;return _T49;_TL2BF: {
int relc=Cyc_Tcutil_typecmp(rel1,rel2);
if(relc==0)goto _TL2C1;_T4A=relc;return _T4A;_TL2C1: {
int bc=Cyc_Tcutil_typecmp(b1,b2);
if(bc==0)goto _TL2C3;_T4B=bc;return _T4B;_TL2C3:
 if(aq1==aq2)goto _TL2C5;return 1;_TL2C5: _T4C=
Cyc_Tcutil_typecmp(null1a,null2a);return _T4C;}}}}}}}_TL2B5: goto _LL17;case 5: _T4D=_T119.f1;_T4E=(int*)_T4D;_T4F=*_T4E;if(_T4F!=5)goto _TL2C7;_T50=_T119.f0;{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T50;_T51=_T137->f1;_T136=_T51.elt_type;_T52=_T137->f1;_T132=_T52.tq;_T53=_T137->f1;_T135=_T53.num_elts;_T54=_T137->f1;_T134=_T54.zero_term;}_T55=_T119.f1;{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T55;_T56=_T137->f1;_T133=_T56.elt_type;_T57=_T137->f1;_T12D=_T57.tq;_T58=_T137->f1;_T126=_T58.num_elts;_T59=_T137->f1;_T131=_T59.zero_term;}{void*t2a=_T136;struct Cyc_Absyn_Tqual tq2a=_T132;struct Cyc_Absyn_Exp*e1=_T135;void*zt1=_T134;void*t1a=_T133;struct Cyc_Absyn_Tqual tq1a=_T12D;struct Cyc_Absyn_Exp*e2=_T126;void*zt2=_T131;
# 1591
int tqc=Cyc_Tcutil_tqual_cmp(tq1a,tq2a);
if(tqc==0)goto _TL2C9;_T5A=tqc;return _T5A;_TL2C9: {
int tc=Cyc_Tcutil_typecmp(t1a,t2a);
if(tc==0)goto _TL2CB;_T5B=tc;return _T5B;_TL2CB: {
int ztc=Cyc_Tcutil_typecmp(zt1,zt2);
if(ztc==0)goto _TL2CD;_T5C=ztc;return _T5C;_TL2CD:
 if(e1!=e2)goto _TL2CF;return 0;_TL2CF:
 if(e1==0)goto _TL2D3;else{goto _TL2D4;}_TL2D4: if(e2==0)goto _TL2D3;else{goto _TL2D1;}
_TL2D3:{struct Cyc_Warn_String_Warn_Warg_struct _T137;_T137.tag=0;_T137.f1=_tag_fat("missing expression in array index",sizeof(char),34U);_T5D=_T137;}{struct Cyc_Warn_String_Warn_Warg_struct _T137=_T5D;void*_T138[1];_T138[0]=& _T137;_T5F=Cyc_Warn_impos2;{int(*_T139)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T5F;_T5E=_T139;}_T60=_tag_fat(_T138,sizeof(void*),1);_T5E(_T60);}goto _TL2D2;_TL2D1: _TL2D2:{
int(*_T137)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_T61=_T137;}_T62=Cyc_Evexp_uint_exp_cmp;_T63=e1;_T64=e2;_T65=_T61(_T62,_T63,_T64);return _T65;}}}_TL2C7: goto _LL17;case 6: _T66=_T119.f1;_T67=(int*)_T66;_T68=*_T67;if(_T68!=6)goto _TL2D5;_T69=_T119.f0;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T69;_T125=_T137->f1;}_T6A=_T119.f1;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T6A;_T124=_T137->f1;}{struct Cyc_Absyn_FnInfo f1=_T125;struct Cyc_Absyn_FnInfo f2=_T124;_T6B=
# 1603
Cyc_Unify_unify(t1,t2);if(!_T6B)goto _TL2D7;return 0;_TL2D7: _T6C=f1;_T6D=_T6C.ret_type;_T6E=f2;_T6F=_T6E.ret_type;{
int r=Cyc_Tcutil_typecmp(_T6D,_T6F);
if(r==0)goto _TL2D9;_T70=r;return _T70;_TL2D9: _T71=f1;_T72=_T71.ret_tqual;_T73=f2;_T74=_T73.ret_tqual;
r=Cyc_Tcutil_tqual_cmp(_T72,_T74);
if(r==0)goto _TL2DB;_T75=r;return _T75;_TL2DB: _T76=f1;{
struct Cyc_List_List*args1=_T76.args;_T77=f2;{
struct Cyc_List_List*args2=_T77.args;
_TL2E0: if(args1!=0)goto _TL2E1;else{goto _TL2DF;}_TL2E1: if(args2!=0)goto _TL2DE;else{goto _TL2DF;}
_TL2DE: _T78=args1;_T79=_T78->hd;_T7A=(struct _tuple9*)_T79;{struct _tuple9 _T137=*_T7A;void*_T138;struct Cyc_Absyn_Tqual _T139;_T139=_T137.f1;_T138=_T137.f2;{struct Cyc_Absyn_Tqual tq1=_T139;void*t1=_T138;_T7B=args2;_T7C=_T7B->hd;_T7D=(struct _tuple9*)_T7C;{
struct _tuple9 _T13A=*_T7D;void*_T13B;struct Cyc_Absyn_Tqual _T13C;_T13C=_T13A.f1;_T13B=_T13A.f2;{struct Cyc_Absyn_Tqual tq2=_T13C;void*t2=_T13B;
r=Cyc_Tcutil_tqual_cmp(tq1,tq2);
if(r==0)goto _TL2E2;_T7E=r;return _T7E;_TL2E2:
 r=Cyc_Tcutil_typecmp(t1,t2);
if(r==0)goto _TL2E4;_T7F=r;return _T7F;_TL2E4:;}}}}_T80=args1;
# 1610
args1=_T80->tl;_T81=args2;args2=_T81->tl;goto _TL2E0;_TL2DF:
# 1618
 if(args1==0)goto _TL2E6;return 1;_TL2E6:
 if(args2==0)goto _TL2E8;_T82=- 1;return _T82;_TL2E8: _T83=f1;_T84=_T83.c_varargs;
if(!_T84)goto _TL2EA;_T85=f2;_T86=_T85.c_varargs;if(_T86)goto _TL2EA;else{goto _TL2EC;}_TL2EC: return 1;_TL2EA: _T87=f1;_T88=_T87.c_varargs;
if(_T88)goto _TL2ED;else{goto _TL2EF;}_TL2EF: _T89=f2;_T8A=_T89.c_varargs;if(!_T8A)goto _TL2ED;_T8B=- 1;return _T8B;_TL2ED: _T8C=f1;_T8D=_T8C.cyc_varargs;
if(_T8D==0)goto _TL2F0;_T8E=f2;_T8F=_T8E.cyc_varargs;if(_T8F!=0)goto _TL2F0;return 1;_TL2F0: _T90=f1;_T91=_T90.cyc_varargs;
if(_T91!=0)goto _TL2F2;_T92=f2;_T93=_T92.cyc_varargs;if(_T93==0)goto _TL2F2;_T94=- 1;return _T94;_TL2F2: _T95=f1;_T96=_T95.cyc_varargs;
if(_T96==0)goto _TL2F4;_T97=f2;_T98=_T97.cyc_varargs;if(_T98==0)goto _TL2F4;_T99=f1;_T9A=_T99.cyc_varargs;_T9B=_T9A->tq;_T9C=f2;_T9D=_T9C.cyc_varargs;_T9E=_T9D->tq;
r=Cyc_Tcutil_tqual_cmp(_T9B,_T9E);
if(r==0)goto _TL2F6;_T9F=r;return _T9F;_TL2F6: _TA0=f1;_TA1=_TA0.cyc_varargs;_TA2=_TA1->type;_TA3=f2;_TA4=_TA3.cyc_varargs;_TA5=_TA4->type;
r=Cyc_Tcutil_typecmp(_TA2,_TA5);
if(r==0)goto _TL2F8;_TA6=r;return _TA6;_TL2F8: _TA7=f1;_TA8=_TA7.cyc_varargs;_TA9=_TA8->inject;
if(!_TA9)goto _TL2FA;_TAA=f2;_TAB=_TAA.cyc_varargs;_TAC=_TAB->inject;if(_TAC)goto _TL2FA;else{goto _TL2FC;}_TL2FC: return 1;_TL2FA: _TAD=f1;_TAE=_TAD.cyc_varargs;_TAF=_TAE->inject;
if(_TAF)goto _TL2FD;else{goto _TL2FF;}_TL2FF: _TB0=f2;_TB1=_TB0.cyc_varargs;_TB2=_TB1->inject;if(!_TB2)goto _TL2FD;_TB3=- 1;return _TB3;_TL2FD: goto _TL2F5;_TL2F4: _TL2F5: _TB4=f1;_TB5=_TB4.effect;_TB6=f2;_TB7=_TB6.effect;
# 1632
r=Cyc_Tcutil_star_cmp(Cyc_Tcutil_typecmp,_TB5,_TB7);
if(r==0)goto _TL300;_TB8=r;return _TB8;_TL300: _TB9=f1;_TBA=_TB9.effconstr;_TBB=f2;_TBC=_TBB.effconstr;_TBD=
# 1644 "tcutil.cyc"
Cyc_Tcutil_cmp_effect_constraints(_TBA,_TBC);if(!_TBD)goto _TL302;
return 1;_TL302:{
int(*_T137)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_TBE=_T137;}_TBF=Cyc_Evexp_uint_exp_cmp;_TC0=f1;_TC1=_TC0.checks_clause;_TC2=f2;_TC3=_TC2.checks_clause;r=_TBE(_TBF,_TC1,_TC3);
if(r==0)goto _TL304;_TC4=r;return _TC4;_TL304:{
int(*_T137)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_TC5=_T137;}_TC6=Cyc_Evexp_uint_exp_cmp;_TC7=f1;_TC8=_TC7.requires_clause;_TC9=f2;_TCA=_TC9.requires_clause;r=_TC5(_TC6,_TC8,_TCA);
if(r==0)goto _TL306;_TCB=r;return _TCB;_TL306:{
int(*_T137)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*)=(int(*)(int(*)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*),struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*))Cyc_Tcutil_star_cmp;_TCC=_T137;}_TCD=Cyc_Evexp_uint_exp_cmp;_TCE=f1;_TCF=_TCE.ensures_clause;_TD0=f2;_TD1=_TD0.ensures_clause;r=_TCC(_TCD,_TCF,_TD1);
if(r==0)goto _TL308;_TD2=r;return _TD2;_TL308: _TD3=f1;_TD4=_TD3.attributes;_TD5=f2;_TD6=_TD5.attributes;_TD7=
# 1653
Cyc_Atts_same_atts(_TD4,_TD6);if(!_TD7)goto _TL30A;{struct Cyc_Warn_String_Warn_Warg_struct _T137;_T137.tag=0;
# 1656
_T137.f1=_tag_fat("typecmp: function type comparison should never get here!",sizeof(char),57U);_TD8=_T137;}{struct Cyc_Warn_String_Warn_Warg_struct _T137=_TD8;void*_T138[1];_T138[0]=& _T137;_TDA=Cyc_Warn_impos2;{int(*_T139)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_TDA;_TD9=_T139;}_TDB=_tag_fat(_T138,sizeof(void*),1);_TD9(_TDB);}goto _TL30B;_TL30A: _TL30B:
# 1658
 return 1;}}}}_TL2D5: goto _LL17;case 7: _TDC=_T119.f1;_TDD=(int*)_TDC;_TDE=*_TDD;if(_TDE!=7)goto _TL30C;_TDF=_T119.f0;{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_TDF;_T123=_T137->f1;_T136=_T137->f3;}_TE0=_T119.f1;{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_TE0;_T122=_T137->f1;_T135=_T137->f3;}{enum Cyc_Absyn_AggrKind k2=_T123;struct Cyc_List_List*fs2=_T136;enum Cyc_Absyn_AggrKind k1=_T122;struct Cyc_List_List*fs1=_T135;_TE1=k1;_TE2=(int)_TE1;_TE3=k2;_TE4=(int)_TE3;
# 1660
if(_TE2==_TE4)goto _TL30E;_TE6=k1;_TE7=(int)_TE6;
if(_TE7!=0)goto _TL310;_TE5=- 1;goto _TL311;_TL310: _TE5=1;_TL311: return _TE5;_TL30E: _TE9=Cyc_List_list_cmp;{
int(*_T137)(int(*)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*,struct Cyc_List_List*)=(int(*)(int(*)(struct Cyc_Absyn_Aggrfield*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*,struct Cyc_List_List*))_TE9;_TE8=_T137;}_TEA=fs1;_TEB=fs2;_TEC=_TE8(Cyc_Tcutil_aggrfield_cmp,_TEA,_TEB);return _TEC;}_TL30C: goto _LL17;case 9: _TED=_T119.f1;_TEE=(int*)_TED;_TEF=*_TEE;if(_TEF!=9)goto _TL312;_TF0=_T119.f0;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TF0;_T136=_T137->f1;}_TF1=_T119.f1;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TF1;_T135=_T137->f1;}{struct Cyc_Absyn_Exp*e1=_T136;struct Cyc_Absyn_Exp*e2=_T135;_T136=e1;_T135=e2;goto _LL12;}_TL312: goto _LL17;case 11: _TF2=_T119.f1;_TF3=(int*)_TF2;_TF4=*_TF3;if(_TF4!=11)goto _TL314;_TF5=_T119.f0;{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_TF5;_T136=_T137->f1;}_TF6=_T119.f1;{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_TF6;_T135=_T137->f1;}_LL12:{struct Cyc_Absyn_Exp*e1=_T136;struct Cyc_Absyn_Exp*e2=_T135;_TF7=
# 1665
Cyc_Evexp_uint_exp_cmp(e1,e2);return _TF7;}_TL314: goto _LL17;case 8: _TF8=_T119.f1;_TF9=(int*)_TF8;_TFA=*_TF9;if(_TFA!=8)goto _TL316;_TFB=_T119.f0;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_TFB;_T121=_T137->f1;_T120=_T137->f2;_TFC=_T137->f4;_T136=(void*)_TFC;}_TFD=_T119.f1;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T137=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_TFD;_T11F=_T137->f1;_T11E=_T137->f2;_TFE=_T137->f4;_T135=(void*)_TFE;}{struct _tuple1*n1=_T121;struct Cyc_List_List*ts1=_T120;void*def1=_T136;struct _tuple1*n2=_T11F;struct Cyc_List_List*ts2=_T11E;void*def2=_T135;
# 1668
if(def1==0)goto _TL318;if(def2==0)goto _TL318;{struct Cyc_Warn_String_Warn_Warg_struct _T137;_T137.tag=0;
_T137.f1=_tag_fat("typecmp: compress did not remove typedefs",sizeof(char),42U);_TFF=_T137;}{struct Cyc_Warn_String_Warn_Warg_struct _T137=_TFF;void*_T138[1];_T138[0]=& _T137;_T101=Cyc_Warn_impos2;{int(*_T139)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T101;_T100=_T139;}_T102=_tag_fat(_T138,sizeof(void*),1);_T100(_T102);}goto _TL319;_TL318: _TL319: {
int i=Cyc_Absyn_qvar_cmp(n1,n2);if(i==0)goto _TL31A;_T103=i;return _T103;_TL31A: _T104=ts1;_T105=ts2;_T106=
Cyc_List_list_cmp(Cyc_Tcutil_typecmp,_T104,_T105);return _T106;}}_TL316: goto _LL17;case 3: _T107=_T119.f1;_T108=(int*)_T107;_T109=*_T108;if(_T109!=3)goto _TL31C;_T10A=_T119.f0;{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_T137=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T10A;_T11D=_T137->f3;_T11C=_T137->f5;}_T10B=_T119.f1;{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_T137=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T10B;_T11B=_T137->f3;_T11A=_T137->f5;}{int id1=_T11D;const char*n1=_T11C;int id2=_T11B;const char*n2=_T11A;{const char*_T137=n1;_T10D=(void*)_T137;_T10E=(void*)_T137;_T10F=_get_zero_arr_size_char(_T10E,1U);_T10C=_tag_fat(_T10D,sizeof(char),_T10F);}{const char*_T137=n2;_T111=(void*)_T137;_T112=(void*)_T137;_T113=_get_zero_arr_size_char(_T112,1U);_T110=_tag_fat(_T111,sizeof(char),_T113);}_T114=
# 1673
Cyc_strcmp(_T10C,_T110);return _T114;}_TL31C: goto _LL17;default: _LL17:{struct Cyc_Warn_String_Warn_Warg_struct _T137;_T137.tag=0;
_T137.f1=_tag_fat("Unmatched case in typecmp",sizeof(char),26U);_T115=_T137;}{struct Cyc_Warn_String_Warn_Warg_struct _T137=_T115;void*_T138[1];_T138[0]=& _T137;_T117=Cyc_Warn_impos2;{int(*_T139)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T117;_T116=_T139;}_T118=_tag_fat(_T138,sizeof(void*),1);_T116(_T118);}};}}}
# 1682
int Cyc_Tcutil_will_lose_precision(void*t1,void*t2){struct _tuple15 _T0;void*_T1;int*_T2;int _T3;void*_T4;int*_T5;int _T6;void*_T7;void*_T8;void*_T9;void*_TA;struct _tuple15 _TB;void*_TC;int*_TD;unsigned _TE;void*_TF;int*_T10;unsigned _T11;void*_T12;void*_T13;int _T14;void*_T15;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T16;enum Cyc_Absyn_Size_of _T17;int _T18;void*_T19;int*_T1A;int _T1B;void*_T1C;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T1D;enum Cyc_Absyn_Size_of _T1E;void*_T1F;int*_T20;unsigned _T21;void*_T22;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T23;enum Cyc_Absyn_Size_of _T24;int _T25;void*_T26;struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_T27;int _T28;void*_T29;int*_T2A;unsigned _T2B;void*_T2C;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T2D;enum Cyc_Absyn_Size_of _T2E;int _T2F;void*_T30;struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_T31;int _T32;void*_T33;int*_T34;int _T35;void*_T36;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T37;enum Cyc_Absyn_Size_of _T38;void*_T39;int*_T3A;int _T3B;void*_T3C;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T3D;enum Cyc_Absyn_Size_of _T3E;int _T3F;{struct _tuple15 _T40;
_T40.f0=Cyc_Absyn_compress(t1);_T40.f1=Cyc_Absyn_compress(t2);_T0=_T40;}{struct _tuple15 _T40=_T0;void*_T41;void*_T42;_T1=_T40.f0;_T2=(int*)_T1;_T3=*_T2;if(_T3!=0)goto _TL31E;_T4=_T40.f1;_T5=(int*)_T4;_T6=*_T5;if(_T6!=0)goto _TL320;_T7=_T40.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T43=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T7;_T8=_T43->f1;_T42=(void*)_T8;}_T9=_T40.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T43=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_TA=_T43->f1;_T41=(void*)_TA;}{void*c1=_T42;void*c2=_T41;{struct _tuple15 _T43;
# 1685
_T43.f0=c1;_T43.f1=c2;_TB=_T43;}{struct _tuple15 _T43=_TB;int _T44;int _T45;_TC=_T43.f0;_TD=(int*)_TC;_TE=*_TD;switch(_TE){case 2: _TF=_T43.f1;_T10=(int*)_TF;_T11=*_T10;switch(_T11){case 2: _T12=_T43.f0;{struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_T46=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T12;_T45=_T46->f1;}_T13=_T43.f1;{struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_T46=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T13;_T44=_T46->f1;}{int i1=_T45;int i2=_T44;_T14=i2 < i1;
return _T14;}case 1: goto _LLB;case 5: _LLB:
# 1688
 return 1;default: goto _LL26;};case 1: _T15=_T43.f0;_T16=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T15;_T17=_T16->f2;_T18=(int)_T17;switch(_T18){case Cyc_Absyn_LongLong_sz: _T19=_T43.f1;_T1A=(int*)_T19;_T1B=*_T1A;if(_T1B!=1)goto _TL325;_T1C=_T43.f1;_T1D=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T1C;_T1E=_T1D->f2;if(_T1E!=Cyc_Absyn_LongLong_sz)goto _TL327;
return 0;_TL327: goto _LLE;_TL325: _LLE:
 return 1;case Cyc_Absyn_Long_sz: _T1F=_T43.f1;_T20=(int*)_T1F;_T21=*_T20;switch(_T21){case 1: _T22=_T43.f1;_T23=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T22;_T24=_T23->f2;_T25=(int)_T24;switch(_T25){case Cyc_Absyn_Int_sz: goto _LL13;case Cyc_Absyn_Short_sz: _LL19: goto _LL1B;case Cyc_Absyn_Char_sz: _LL1F: goto _LL21;default: goto _LL26;};case 2: _T26=_T43.f1;_T27=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T26;_T28=_T27->f1;if(_T28!=0)goto _TL32B;goto _LL17;_TL32B: goto _LL26;default: goto _LL26;};case Cyc_Absyn_Int_sz: _T29=_T43.f1;_T2A=(int*)_T29;_T2B=*_T2A;switch(_T2B){case 1: _T2C=_T43.f1;_T2D=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T2C;_T2E=_T2D->f2;_T2F=(int)_T2E;switch(_T2F){case Cyc_Absyn_Long_sz: _LL13:
# 1694
 return 0;case Cyc_Absyn_Short_sz: _LL1B: goto _LL1D;case Cyc_Absyn_Char_sz: _LL21: goto _LL23;default: goto _LL26;};case 2: _T30=_T43.f1;_T31=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T30;_T32=_T31->f1;if(_T32!=0)goto _TL32F;_LL17: goto _LL19;_TL32F: goto _LL26;default: goto _LL26;};case Cyc_Absyn_Short_sz: _T33=_T43.f1;_T34=(int*)_T33;_T35=*_T34;if(_T35!=1)goto _TL331;_T36=_T43.f1;_T37=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T36;_T38=_T37->f2;if(_T38!=Cyc_Absyn_Char_sz)goto _TL333;_LL23: goto _LL25;_TL333: goto _LL26;_TL331: goto _LL26;default: goto _LL26;};case 5: _T39=_T43.f1;_T3A=(int*)_T39;_T3B=*_T3A;if(_T3B!=1)goto _TL335;_T3C=_T43.f1;_T3D=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T3C;_T3E=_T3D->f2;_T3F=(int)_T3E;switch(_T3F){case Cyc_Absyn_Short_sz: _LL1D: goto _LL1F;case Cyc_Absyn_Char_sz: _LL25:
# 1704
 return 1;default: goto _LL26;}goto _TL336;_TL335: goto _LL26;_TL336:;default: _LL26:
# 1706
 return 0;};}}goto _TL321;_TL320: goto _LL3;_TL321: goto _TL31F;_TL31E: _LL3:
# 1708
 return 0;_TL31F:;}}
# 1712
void*Cyc_Tcutil_max_arithmetic_type(void*t1,void*t2){struct _tuple15 _T0;void*_T1;int*_T2;int _T3;void*_T4;int*_T5;int _T6;void*_T7;void*_T8;void*_T9;void*_TA;struct _tuple15 _TB;void*_TC;int*_TD;int _TE;void*_TF;int*_T10;int _T11;void*_T12;void*_T13;void*_T14;void*_T15;void*_T16;void*_T17;int*_T18;int _T19;void*_T1A;void*_T1B;int*_T1C;int _T1D;void*_T1E;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T1F;enum Cyc_Absyn_Sign _T20;void*_T21;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T22;enum Cyc_Absyn_Size_of _T23;void*_T24;int*_T25;int _T26;void*_T27;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T28;enum Cyc_Absyn_Sign _T29;void*_T2A;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T2B;enum Cyc_Absyn_Size_of _T2C;void*_T2D;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T2E;enum Cyc_Absyn_Size_of _T2F;void*_T30;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T31;enum Cyc_Absyn_Size_of _T32;void*_T33;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T34;enum Cyc_Absyn_Size_of _T35;void*_T36;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T37;enum Cyc_Absyn_Size_of _T38;void*_T39;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T3A;enum Cyc_Absyn_Size_of _T3B;void*_T3C;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T3D;enum Cyc_Absyn_Size_of _T3E;int _T3F;void*_T40;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T41;enum Cyc_Absyn_Size_of _T42;void*_T43;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T44;enum Cyc_Absyn_Size_of _T45;void*_T46;int*_T47;int _T48;void*_T49;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T4A;enum Cyc_Absyn_Size_of _T4B;void*_T4C;int*_T4D;int _T4E;void*_T4F;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T50;enum Cyc_Absyn_Sign _T51;void*_T52;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T53;enum Cyc_Absyn_Size_of _T54;void*_T55;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T56;enum Cyc_Absyn_Size_of _T57;void*_T58;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T59;enum Cyc_Absyn_Size_of _T5A;int _T5B;void*_T5C;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T5D;enum Cyc_Absyn_Size_of _T5E;void*_T5F;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T60;enum Cyc_Absyn_Size_of _T61;void*_T62;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T63;enum Cyc_Absyn_Size_of _T64;void*_T65;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T66;enum Cyc_Absyn_Size_of _T67;void*_T68;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T69;enum Cyc_Absyn_Size_of _T6A;void*_T6B;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T6C;enum Cyc_Absyn_Size_of _T6D;void*_T6E;int*_T6F;int _T70;void*_T71;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T72;enum Cyc_Absyn_Size_of _T73;void*_T74;int*_T75;int _T76;void*_T77;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T78;enum Cyc_Absyn_Sign _T79;void*_T7A;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T7B;enum Cyc_Absyn_Size_of _T7C;int _T7D;void*_T7E;void*_T7F;void*_T80;int*_T81;int _T82;void*_T83;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T84;enum Cyc_Absyn_Size_of _T85;void*_T86;void*_T87;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T88;enum Cyc_Absyn_Size_of _T89;void*_T8A;void*_T8B;int*_T8C;int _T8D;void*_T8E;struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T8F;enum Cyc_Absyn_Size_of _T90;void*_T91;void*_T92;int*_T93;int _T94;void*_T95;int*_T96;int _T97;void*_T98;{struct _tuple15 _T99;
_T99.f0=Cyc_Absyn_compress(t1);_T99.f1=Cyc_Absyn_compress(t2);_T0=_T99;}{struct _tuple15 _T99=_T0;void*_T9A;void*_T9B;_T1=_T99.f0;_T2=(int*)_T1;_T3=*_T2;if(_T3!=0)goto _TL338;_T4=_T99.f1;_T5=(int*)_T4;_T6=*_T5;if(_T6!=0)goto _TL33A;_T7=_T99.f0;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T9C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T7;_T8=_T9C->f1;_T9B=(void*)_T8;}_T9=_T99.f1;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T9C=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_TA=_T9C->f1;_T9A=(void*)_TA;}{void*c1=_T9B;void*c2=_T9A;{struct _tuple15 _T9C;
# 1715
_T9C.f0=c1;_T9C.f1=c2;_TB=_T9C;}{struct _tuple15 _T9C=_TB;int _T9D;int _T9E;_TC=_T9C.f0;_TD=(int*)_TC;_TE=*_TD;if(_TE!=2)goto _TL33C;_TF=_T9C.f1;_T10=(int*)_TF;_T11=*_T10;if(_T11!=2)goto _TL33E;_T12=_T9C.f0;{struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_T9F=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T12;_T9E=_T9F->f1;}_T13=_T9C.f1;{struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*_T9F=(struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_T13;_T9D=_T9F->f1;}{int i1=_T9E;int i2=_T9D;
# 1717
if(i1 < i2)goto _TL340;_T14=t1;return _T14;_TL340: _T15=t2;
return _T15;}_TL33E: _T16=t1;
return _T16;_TL33C: _T17=_T9C.f1;_T18=(int*)_T17;_T19=*_T18;if(_T19!=2)goto _TL342;_T1A=t2;
return _T1A;_TL342: _T1B=_T9C.f0;_T1C=(int*)_T1B;_T1D=*_T1C;if(_T1D!=1)goto _TL344;_T1E=_T9C.f0;_T1F=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T1E;_T20=_T1F->f1;if(_T20!=Cyc_Absyn_Unsigned)goto _TL346;_T21=_T9C.f0;_T22=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T21;_T23=_T22->f2;if(_T23!=Cyc_Absyn_LongLong_sz)goto _TL348;goto _LLF;_TL348: _T24=_T9C.f1;_T25=(int*)_T24;_T26=*_T25;if(_T26!=1)goto _TL34A;_T27=_T9C.f1;_T28=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T27;_T29=_T28->f1;if(_T29!=Cyc_Absyn_Unsigned)goto _TL34C;_T2A=_T9C.f1;_T2B=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T2A;_T2C=_T2B->f2;if(_T2C!=Cyc_Absyn_LongLong_sz)goto _TL34E;goto _LLE;_TL34E: _T2D=_T9C.f0;_T2E=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T2D;_T2F=_T2E->f2;if(_T2F!=Cyc_Absyn_Long_sz)goto _TL350;goto _LL14;_TL350: _T30=_T9C.f1;_T31=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T30;_T32=_T31->f2;if(_T32!=Cyc_Absyn_Long_sz)goto _TL352;goto _LL16;_TL352: _T33=_T9C.f0;_T34=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T33;_T35=_T34->f2;if(_T35!=Cyc_Absyn_Int_sz)goto _TL354;goto _LL1C;_TL354: _T36=_T9C.f1;_T37=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T36;_T38=_T37->f2;if(_T38!=Cyc_Absyn_Int_sz)goto _TL356;goto _LL1E;_TL356: goto _LL24;_TL34C: _T39=_T9C.f1;_T3A=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T39;_T3B=_T3A->f2;if(_T3B!=Cyc_Absyn_LongLong_sz)goto _TL358;goto _LL12;_TL358: _T3C=_T9C.f0;_T3D=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T3C;_T3E=_T3D->f2;_T3F=(int)_T3E;switch(_T3F){case Cyc_Absyn_Long_sz: goto _LL14;case Cyc_Absyn_Int_sz: goto _LL1C;default: _T40=_T9C.f1;_T41=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T40;_T42=_T41->f2;if(_T42!=Cyc_Absyn_Long_sz)goto _TL35B;goto _LL22;_TL35B: goto _LL24;}goto _TL34B;_TL34A: _T43=_T9C.f0;_T44=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T43;_T45=_T44->f2;if(_T45!=Cyc_Absyn_Long_sz)goto _TL35D;_LL14: goto _LL17;_TL35D: _T46=_T9C.f1;_T47=(int*)_T46;_T48=*_T47;if(_T48!=5)goto _TL35F;goto _LL1A;_TL35F: _T49=_T9C.f0;_T4A=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T49;_T4B=_T4A->f2;if(_T4B!=Cyc_Absyn_Int_sz)goto _TL361;_LL1C: _LL1D: goto _LL1F;_TL361: goto _LL24;_TL34B: goto _TL347;_TL346: _T4C=_T9C.f1;_T4D=(int*)_T4C;_T4E=*_T4D;if(_T4E!=1)goto _TL363;_T4F=_T9C.f1;_T50=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T4F;_T51=_T50->f1;if(_T51!=Cyc_Absyn_Unsigned)goto _TL365;_T52=_T9C.f1;_T53=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T52;_T54=_T53->f2;if(_T54!=Cyc_Absyn_LongLong_sz)goto _TL367;goto _LLE;_TL367: _T55=_T9C.f0;_T56=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T55;_T57=_T56->f2;if(_T57!=Cyc_Absyn_LongLong_sz)goto _TL369;goto _LL10;_TL369: _T58=_T9C.f1;_T59=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T58;_T5A=_T59->f2;_T5B=(int)_T5A;switch(_T5B){case Cyc_Absyn_Long_sz: goto _LL16;case Cyc_Absyn_Int_sz: goto _LL1E;default: _T5C=_T9C.f0;_T5D=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T5C;_T5E=_T5D->f2;if(_T5E!=Cyc_Absyn_Long_sz)goto _TL36C;goto _LL20;_TL36C: goto _LL24;}goto _TL366;_TL365: _T5F=_T9C.f0;_T60=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T5F;_T61=_T60->f2;if(_T61!=Cyc_Absyn_LongLong_sz)goto _TL36E;goto _LL10;_TL36E: _T62=_T9C.f1;_T63=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T62;_T64=_T63->f2;if(_T64!=Cyc_Absyn_LongLong_sz)goto _TL370;goto _LL12;_TL370: _T65=_T9C.f0;_T66=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T65;_T67=_T66->f2;if(_T67!=Cyc_Absyn_Long_sz)goto _TL372;goto _LL20;_TL372: _T68=_T9C.f1;_T69=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T68;_T6A=_T69->f2;if(_T6A!=Cyc_Absyn_Long_sz)goto _TL374;goto _LL22;_TL374: goto _LL24;_TL366: goto _TL364;_TL363: _T6B=_T9C.f0;_T6C=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T6B;_T6D=_T6C->f2;if(_T6D!=Cyc_Absyn_LongLong_sz)goto _TL376;_LL10: goto _LL13;_TL376: _T6E=_T9C.f1;_T6F=(int*)_T6E;_T70=*_T6F;if(_T70!=5)goto _TL378;goto _LL1A;_TL378: _T71=_T9C.f0;_T72=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T71;_T73=_T72->f2;if(_T73!=Cyc_Absyn_Long_sz)goto _TL37A;_LL20: goto _LL23;_TL37A: goto _LL24;_TL364: _TL347: goto _TL345;_TL344: _T74=_T9C.f1;_T75=(int*)_T74;_T76=*_T75;if(_T76!=1)goto _TL37C;_T77=_T9C.f1;_T78=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T77;_T79=_T78->f1;if(_T79!=Cyc_Absyn_Unsigned)goto _TL37E;_T7A=_T9C.f1;_T7B=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T7A;_T7C=_T7B->f2;_T7D=(int)_T7C;switch(_T7D){case Cyc_Absyn_LongLong_sz: _LLE: _LLF: _T7E=Cyc_Absyn_ulonglong_type;
# 1722
return _T7E;case Cyc_Absyn_Long_sz: _LL16: _LL17: _T7F=Cyc_Absyn_ulong_type;
# 1726
return _T7F;default: _T80=_T9C.f0;_T81=(int*)_T80;_T82=*_T81;if(_T82!=5)goto _TL381;goto _LL18;_TL381: _T83=_T9C.f1;_T84=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T83;_T85=_T84->f2;if(_T85!=Cyc_Absyn_Int_sz)goto _TL383;_LL1E: _LL1F: _T86=Cyc_Absyn_uint_type;
# 1731
return _T86;_TL383: goto _LL24;}goto _TL37F;_TL37E: _T87=_T9C.f1;_T88=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T87;_T89=_T88->f2;if(_T89!=Cyc_Absyn_LongLong_sz)goto _TL385;_LL12: _LL13: _T8A=Cyc_Absyn_slonglong_type;
# 1724
return _T8A;_TL385: _T8B=_T9C.f0;_T8C=(int*)_T8B;_T8D=*_T8C;if(_T8D!=5)goto _TL387;goto _LL18;_TL387: _T8E=_T9C.f1;_T8F=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T8E;_T90=_T8F->f2;if(_T90!=Cyc_Absyn_Long_sz)goto _TL389;_LL22: _LL23: _T91=Cyc_Absyn_slong_type;
# 1733
return _T91;_TL389: goto _LL24;_TL37F: goto _TL37D;_TL37C: _T92=_T9C.f0;_T93=(int*)_T92;_T94=*_T93;if(_T94!=5)goto _TL38B;_LL18: goto _LL1B;_TL38B: _T95=_T9C.f1;_T96=(int*)_T95;_T97=*_T96;if(_T97!=5)goto _TL38D;_LL1A: _LL1B: goto _LL1D;_TL38D: _LL24: goto _LL5;_TL37D: _TL345: _LL5:;}goto _LL0;}_TL33A: goto _LL3;_TL338: _LL3: goto _LL0;_LL0:;}_T98=Cyc_Absyn_sint_type;
# 1739
return _T98;}
# 1744
int Cyc_Tcutil_coerce_list(struct Cyc_List_List*aqb,void*t,struct Cyc_List_List*es){struct Cyc_List_List*_T0;void*_T1;struct Cyc_Absyn_Exp*_T2;void*_T3;void*_T4;int _T5;void*_T6;struct Cyc_Core_Opt*_T7;void*_T8;int _T9;struct Cyc_Core_Opt*_TA;struct Cyc_List_List*_TB;void*_TC;struct Cyc_Core_Opt*_TD;void*_TE;int _TF;struct Cyc_List_List*_T10;struct Cyc_List_List*_T11;void*_T12;struct Cyc_Absyn_Exp*_T13;void*_T14;int _T15;struct Cyc_Warn_String_Warn_Warg_struct _T16;struct Cyc_Warn_Typ_Warn_Warg_struct _T17;struct Cyc_Warn_String_Warn_Warg_struct _T18;struct Cyc_Warn_Typ_Warn_Warg_struct _T19;struct Cyc_List_List*_T1A;void*_T1B;struct Cyc_Absyn_Exp*_T1C;void*_T1D;struct Cyc_List_List*_T1E;void*_T1F;struct Cyc_Absyn_Exp*_T20;unsigned _T21;struct _fat_ptr _T22;struct Cyc_List_List*_T23;
# 1747
struct Cyc_Core_Opt*max_arith_type=0;{
struct Cyc_List_List*el=es;_TL392: if(el!=0)goto _TL390;else{goto _TL391;}
_TL390: _T0=el;_T1=_T0->hd;_T2=(struct Cyc_Absyn_Exp*)_T1;_T3=_T2->topt;_T4=_check_null(_T3);{void*t1=Cyc_Absyn_compress(_T4);_T5=
Cyc_Tcutil_is_arithmetic_type(t1);if(!_T5)goto _TL393;
if(max_arith_type==0)goto _TL397;else{goto _TL398;}_TL398: _T6=t1;_T7=max_arith_type;_T8=_T7->v;_T9=Cyc_Tcutil_will_lose_precision(_T6,_T8);if(_T9)goto _TL397;else{goto _TL395;}
_TL397:{struct Cyc_Core_Opt*_T24=_cycalloc(sizeof(struct Cyc_Core_Opt));_T24->v=t1;_TA=(struct Cyc_Core_Opt*)_T24;}max_arith_type=_TA;goto _TL396;_TL395: _TL396: goto _TL394;_TL393: _TL394:;}_TB=el;
# 1748
el=_TB->tl;goto _TL392;_TL391:;}
# 1755
if(max_arith_type==0)goto _TL399;_TC=t;_TD=max_arith_type;_TE=_TD->v;_TF=
Cyc_Unify_unify(_TC,_TE);if(_TF)goto _TL39B;else{goto _TL39D;}
_TL39D: return 0;_TL39B: goto _TL39A;_TL399: _TL39A:{
# 1759
struct Cyc_List_List*el=es;_TL3A1: if(el!=0)goto _TL39F;else{goto _TL3A0;}
_TL39F: _T10=aqb;_T11=el;_T12=_T11->hd;_T13=(struct Cyc_Absyn_Exp*)_T12;_T14=t;_T15=Cyc_Tcutil_coerce_assign(_T10,_T13,_T14);if(_T15)goto _TL3A2;else{goto _TL3A4;}
_TL3A4:{struct Cyc_Warn_String_Warn_Warg_struct _T24;_T24.tag=0;_T24.f1=_tag_fat("type mismatch: expecting ",sizeof(char),26U);_T16=_T24;}{struct Cyc_Warn_String_Warn_Warg_struct _T24=_T16;{struct Cyc_Warn_Typ_Warn_Warg_struct _T25;_T25.tag=2;_T25.f1=t;_T17=_T25;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T25=_T17;{struct Cyc_Warn_String_Warn_Warg_struct _T26;_T26.tag=0;_T26.f1=_tag_fat(" but found ",sizeof(char),12U);_T18=_T26;}{struct Cyc_Warn_String_Warn_Warg_struct _T26=_T18;{struct Cyc_Warn_Typ_Warn_Warg_struct _T27;_T27.tag=2;_T1A=el;_T1B=_T1A->hd;_T1C=(struct Cyc_Absyn_Exp*)_T1B;_T1D=_T1C->topt;
_T27.f1=_check_null(_T1D);_T19=_T27;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T27=_T19;void*_T28[4];_T28[0]=& _T24;_T28[1]=& _T25;_T28[2]=& _T26;_T28[3]=& _T27;_T1E=el;_T1F=_T1E->hd;_T20=(struct Cyc_Absyn_Exp*)_T1F;_T21=_T20->loc;_T22=_tag_fat(_T28,sizeof(void*),4);Cyc_Warn_err2(_T21,_T22);}}}}
return 0;_TL3A2: _T23=el;
# 1759
el=_T23->tl;goto _TL3A1;_TL3A0:;}
# 1765
return 1;}
# 1770
int Cyc_Tcutil_coerce_to_bool(struct Cyc_Absyn_Exp*e){int _T0;struct Cyc_Absyn_Exp*_T1;void*_T2;void*_T3;int*_T4;int _T5;_T0=
Cyc_Tcutil_coerce_sint_type(e);if(_T0)goto _TL3A5;else{goto _TL3A7;}
_TL3A7: _T1=e;_T2=_T1->topt;_T3=_check_null(_T2);{void*_T6=Cyc_Absyn_compress(_T3);_T4=(int*)_T6;_T5=*_T4;if(_T5!=4)goto _TL3A8;
Cyc_Tcutil_unchecked_cast(e,Cyc_Absyn_ulong_type,4U);goto _LL0;_TL3A8:
 return 0;_LL0:;}goto _TL3A6;_TL3A5: _TL3A6:
# 1776
 return 1;}
# 1779
static int Cyc_Tcutil_coerce_Xint_type(void*Xint_type,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;void*_T1;void*_T2;void*_T3;int _T4;struct Cyc_Absyn_Exp*_T5;void*_T6;void*_T7;int _T8;struct Cyc_Absyn_Exp*_T9;void*_TA;void*_TB;void*_TC;int _TD;struct Cyc_Warn_String_Warn_Warg_struct _TE;struct Cyc_Absyn_Exp*_TF;unsigned _T10;struct _fat_ptr _T11;_T0=e;_T1=_T0->topt;_T2=
_check_null(_T1);_T3=Xint_type;_T4=Cyc_Unify_unify(_T2,_T3);if(!_T4)goto _TL3AA;
return 1;_TL3AA: _T5=e;_T6=_T5->topt;_T7=
# 1783
_check_null(_T6);_T8=Cyc_Tcutil_is_integral_type(_T7);if(!_T8)goto _TL3AC;_T9=e;_TA=_T9->topt;_TB=
_check_null(_TA);_TC=Xint_type;_TD=Cyc_Tcutil_will_lose_precision(_TB,_TC);if(!_TD)goto _TL3AE;{struct Cyc_Warn_String_Warn_Warg_struct _T12;_T12.tag=0;
_T12.f1=_tag_fat("integral size mismatch; conversion supplied",sizeof(char),44U);_TE=_T12;}{struct Cyc_Warn_String_Warn_Warg_struct _T12=_TE;void*_T13[1];_T13[0]=& _T12;_TF=e;_T10=_TF->loc;_T11=_tag_fat(_T13,sizeof(void*),1);Cyc_Warn_warn2(_T10,_T11);}goto _TL3AF;_TL3AE: _TL3AF:
 Cyc_Tcutil_unchecked_cast(e,Xint_type,1U);
return 1;_TL3AC:
# 1789
 return 0;}
# 1792
int Cyc_Tcutil_coerce_ulong_type(struct Cyc_Absyn_Exp*e){int _T0;_T0=
Cyc_Tcutil_coerce_Xint_type(Cyc_Absyn_ulong_type,e);return _T0;}
# 1796
int Cyc_Tcutil_coerce_sint_type(struct Cyc_Absyn_Exp*e){int _T0;_T0=
Cyc_Tcutil_coerce_Xint_type(Cyc_Absyn_sint_type,e);return _T0;}
# 1802
int Cyc_Tcutil_force_type2bool(int desired,void*t){void*_T0;int _T1;void*_T2;int _T3;_T1=desired;
if(!_T1)goto _TL3B0;_T0=Cyc_Absyn_true_type;goto _TL3B1;_TL3B0: _T0=Cyc_Absyn_false_type;_TL3B1: _T2=t;Cyc_Unify_unify(_T0,_T2);_T3=
Cyc_Absyn_type2bool(desired,t);return _T3;}
# 1808
void*Cyc_Tcutil_force_bounds_one(void*t){void*_T0;void*_T1;void*_T2;_T0=t;_T1=
Cyc_Absyn_bounds_one();Cyc_Unify_unify(_T0,_T1);_T2=
Cyc_Absyn_compress(t);return _T2;}
# 1813
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_thin_bound(struct Cyc_List_List*ts){struct Cyc_List_List*_T0;void*_T1;void*_T2;int*_T3;int _T4;void*_T5;struct Cyc_Absyn_Exp*_T6;struct Cyc_Absyn_Exp*_T7;struct Cyc_Absyn_Exp*_T8;_T0=
_check_null(ts);_T1=_T0->hd;{void*t=Cyc_Absyn_compress(_T1);struct Cyc_Absyn_Exp*_T9;_T2=t;_T3=(int*)_T2;_T4=*_T3;if(_T4!=9)goto _TL3B2;_T5=t;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TA=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_T5;_T9=_TA->f1;}{struct Cyc_Absyn_Exp*e=_T9;_T6=e;
# 1816
return _T6;}_TL3B2:{
# 1818
struct Cyc_Absyn_Exp*v=Cyc_Absyn_valueof_exp(t,0U);_T7=v;
_T7->topt=Cyc_Absyn_ulong_type;_T8=v;
return _T8;};}}
# 1824
void*Cyc_Tcutil_get_pointer_bounds(void*ptrtype){void*_T0;int*_T1;int _T2;void*_T3;void*_T4;struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T5;struct Cyc_Absyn_PtrAtts*_T6;struct Cyc_Absyn_PtrAtts*_T7;void*_T8;void*_T9;_T0=ptrtype;_T1=(int*)_T0;_T2=*_T1;if(_T2!=4)goto _TL3B4;_T3=ptrtype;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TA=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T3;_T4=ptrtype;_T5=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T4;_T6=& _T5->f1.ptr_atts;_T9=(struct Cyc_Absyn_PtrAtts*)_T6;}{struct Cyc_Absyn_PtrAtts*pa=(struct Cyc_Absyn_PtrAtts*)_T9;_T7=pa;_T8=_T7->bounds;
# 1827
return _T8;}_TL3B4:
# 1829
 return 0;;}
# 1836
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp_constrain(void*def,void*b,int c){int _T0;int _T1;void*_T2;void*_T3;void*_T4;void*_T5;void*_T6;int _T7;struct Cyc_Warn_String_Warn_Warg_struct _T8;struct Cyc_Warn_Typ_Warn_Warg_struct _T9;int(*_TA)(struct _fat_ptr);void*(*_TB)(struct _fat_ptr);struct _fat_ptr _TC;int*_TD;int _TE;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TF;void*_T10;int*_T11;unsigned _T12;struct Cyc_Absyn_Exp*_T13;struct Cyc_Warn_String_Warn_Warg_struct _T14;struct Cyc_Warn_Typ_Warn_Warg_struct _T15;int(*_T16)(struct _fat_ptr);void*(*_T17)(struct _fat_ptr);struct _fat_ptr _T18;_T0=
Cyc_Tcutil_is_cvar_type(b);if(!_T0)goto _TL3B6;_T1=c;if(!_T1)goto _TL3B6;_T2=
Cyc_Tcutil_ptrbnd_cvar_equivalent(b);_T3=_check_null(_T2);_T4=
Cyc_Tcutil_ptrbnd_cvar_equivalent(def);_T5=_check_null(_T4);_T6=
# 1838
Cyc_BansheeIf_equality_constraint(_T3,_T5);_T7=Cyc_BansheeIf_add_constraint(0U,_T6);if(_T7)goto _TL3B8;else{goto _TL3BA;}
# 1840
_TL3BA:{struct Cyc_Warn_String_Warn_Warg_struct _T19;_T19.tag=0;_T19.f1=_tag_fat("get_bounds_exp -- unable to constrain cvar ",sizeof(char),44U);_T8=_T19;}{struct Cyc_Warn_String_Warn_Warg_struct _T19=_T8;{struct Cyc_Warn_Typ_Warn_Warg_struct _T1A;_T1A.tag=2;_T1A.f1=b;_T9=_T1A;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T1A=_T9;void*_T1B[2];_T1B[0]=& _T19;_T1B[1]=& _T1A;_TB=Cyc_Warn_impos2;{int(*_T1C)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_TB;_TA=_T1C;}_TC=_tag_fat(_T1B,sizeof(void*),2);_TA(_TC);}}goto _TL3B9;_TL3B8: _TL3B9: goto _TL3B7;_TL3B6: _TL3B7:
# 1843
 Cyc_Unify_unify(def,b);{
void*_T19=Cyc_Absyn_compress(b);struct Cyc_List_List*_T1A;_TD=(int*)_T19;_TE=*_TD;if(_TE!=0)goto _TL3BB;_TF=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T19;_T10=_TF->f1;_T11=(int*)_T10;_T12=*_T11;switch(_T12){case 14:
 return 0;case 13:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1B=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T19;_T1A=_T1B->f2;}{struct Cyc_List_List*ts=_T1A;_T13=
Cyc_Tcutil_get_thin_bound(ts);return _T13;}default: goto _LL5;}goto _TL3BC;_TL3BB: _LL5:{struct Cyc_Warn_String_Warn_Warg_struct _T1B;_T1B.tag=0;
_T1B.f1=_tag_fat("get_bounds_exp: ",sizeof(char),17U);_T14=_T1B;}{struct Cyc_Warn_String_Warn_Warg_struct _T1B=_T14;{struct Cyc_Warn_Typ_Warn_Warg_struct _T1C;_T1C.tag=2;_T1C.f1=b;_T15=_T1C;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T1C=_T15;void*_T1D[2];_T1D[0]=& _T1B;_T1D[1]=& _T1C;_T17=Cyc_Warn_impos2;{int(*_T1E)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T17;_T16=_T1E;}_T18=_tag_fat(_T1D,sizeof(void*),2);_T16(_T18);}}_TL3BC:;}}
# 1851
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b){struct Cyc_Absyn_Exp*_T0;_T0=
Cyc_Tcutil_get_bounds_exp_constrain(def,b,0);return _T0;}
# 1855
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_ptr_bounds_exp(void*def,void*t){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrAtts _T3;struct Cyc_Absyn_Exp*_T4;struct Cyc_Warn_String_Warn_Warg_struct _T5;struct Cyc_Warn_Typ_Warn_Warg_struct _T6;int(*_T7)(struct _fat_ptr);void*(*_T8)(struct _fat_ptr);struct _fat_ptr _T9;
void*_TA=Cyc_Absyn_compress(t);void*_TB;_T0=(int*)_TA;_T1=*_T0;if(_T1!=4)goto _TL3BE;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TA;_T2=_TC->f1;_T3=_T2.ptr_atts;_TB=_T3.bounds;}{void*b=_TB;_T4=
# 1858
Cyc_Tcutil_get_bounds_exp(def,b);return _T4;}_TL3BE:{struct Cyc_Warn_String_Warn_Warg_struct _TC;_TC.tag=0;
_TC.f1=_tag_fat("get_ptr_bounds_exp not pointer: ",sizeof(char),33U);_T5=_TC;}{struct Cyc_Warn_String_Warn_Warg_struct _TC=_T5;{struct Cyc_Warn_Typ_Warn_Warg_struct _TD;_TD.tag=2;_TD.f1=t;_T6=_TD;}{struct Cyc_Warn_Typ_Warn_Warg_struct _TD=_T6;void*_TE[2];_TE[0]=& _TC;_TE[1]=& _TD;_T8=Cyc_Warn_impos2;{int(*_TF)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T8;_T7=_TF;}_T9=_tag_fat(_TE,sizeof(void*),2);_T7(_T9);}};}
# 1864
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*tvs){struct Cyc_Core_Opt*_T0;struct Cyc_Core_Opt*_T1;struct Cyc_Core_Opt*_T2;void*_T3;_T0=& Cyc_Kinds_boolko;_T1=(struct Cyc_Core_Opt*)_T0;{struct Cyc_Core_Opt*_T4=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T4->v=tvs;_T2=(struct Cyc_Core_Opt*)_T4;}_T3=Cyc_Absyn_new_evar(_T1,_T2);return _T3;}
# 1868
void*Cyc_Tcutil_any_bounds(struct Cyc_List_List*tvs){struct Cyc_Core_Opt*_T0;struct Cyc_Core_Opt*_T1;struct Cyc_Core_Opt*_T2;void*_T3;_T0=& Cyc_Kinds_ptrbko;_T1=(struct Cyc_Core_Opt*)_T0;{struct Cyc_Core_Opt*_T4=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T4->v=tvs;_T2=(struct Cyc_Core_Opt*)_T4;}_T3=Cyc_Absyn_new_evar(_T1,_T2);return _T3;}
# 1872
void*Cyc_Tcutil_pointer_elt_type(void*t){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;void*_T3;struct Cyc_Warn_String_Warn_Warg_struct _T4;int(*_T5)(struct _fat_ptr);void*(*_T6)(struct _fat_ptr);struct _fat_ptr _T7;
void*_T8=Cyc_Absyn_compress(t);void*_T9;_T0=(int*)_T8;_T1=*_T0;if(_T1!=4)goto _TL3C0;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TA=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T8;_T2=_TA->f1;_T9=_T2.elt_type;}{void*e=_T9;_T3=e;
return _T3;}_TL3C0:{struct Cyc_Warn_String_Warn_Warg_struct _TA;_TA.tag=0;
_TA.f1=_tag_fat("pointer_elt_type",sizeof(char),17U);_T4=_TA;}{struct Cyc_Warn_String_Warn_Warg_struct _TA=_T4;void*_TB[1];_TB[0]=& _TA;_T6=Cyc_Warn_impos2;{int(*_TC)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T6;_T5=_TC;}_T7=_tag_fat(_TB,sizeof(void*),1);_T5(_T7);};}
# 1878
void*Cyc_Tcutil_pointer_region(void*t){int*_T0;int _T1;struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T2;struct Cyc_Absyn_PtrAtts*_T3;struct Cyc_Absyn_PtrAtts*_T4;void*_T5;struct Cyc_Warn_String_Warn_Warg_struct _T6;int(*_T7)(struct _fat_ptr);void*(*_T8)(struct _fat_ptr);struct _fat_ptr _T9;
void*_TA=Cyc_Absyn_compress(t);void*_TB;_T0=(int*)_TA;_T1=*_T0;if(_T1!=4)goto _TL3C2;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TA;_T2=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TA;_T3=& _T2->f1.ptr_atts;_TB=(struct Cyc_Absyn_PtrAtts*)_T3;}{struct Cyc_Absyn_PtrAtts*p=(struct Cyc_Absyn_PtrAtts*)_TB;_T4=p;_T5=_T4->eff;
return _T5;}_TL3C2:{struct Cyc_Warn_String_Warn_Warg_struct _TC;_TC.tag=0;
_TC.f1=_tag_fat("pointer_elt_type",sizeof(char),17U);_T6=_TC;}{struct Cyc_Warn_String_Warn_Warg_struct _TC=_T6;void*_TD[1];_TD[0]=& _TC;_T8=Cyc_Warn_impos2;{int(*_TE)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T8;_T7=_TE;}_T9=_tag_fat(_TD,sizeof(void*),1);_T7(_T9);};}
# 1885
int Cyc_Tcutil_rgn_of_pointer(void*t,void**rgn){int*_T0;int _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrAtts _T3;void**_T4;
void*_T5=Cyc_Absyn_compress(t);void*_T6;_T0=(int*)_T5;_T1=*_T0;if(_T1!=4)goto _TL3C4;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T7=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T5;_T2=_T7->f1;_T3=_T2.ptr_atts;_T6=_T3.eff;}{void*r=_T6;_T4=rgn;
*_T4=r;return 1;}_TL3C4:
 return 0;;}
# 1892
void*Cyc_Tcutil_pointer_aqual(void*t){int*_T0;int _T1;struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T2;struct Cyc_Absyn_PtrAtts*_T3;struct Cyc_Absyn_PtrAtts*_T4;void*_T5;struct Cyc_Warn_String_Warn_Warg_struct _T6;int(*_T7)(struct _fat_ptr);void*(*_T8)(struct _fat_ptr);struct _fat_ptr _T9;
void*_TA=Cyc_Absyn_compress(t);void*_TB;_T0=(int*)_TA;_T1=*_T0;if(_T1!=4)goto _TL3C6;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TA;_T2=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TA;_T3=& _T2->f1.ptr_atts;_TB=(struct Cyc_Absyn_PtrAtts*)_T3;}{struct Cyc_Absyn_PtrAtts*p=(struct Cyc_Absyn_PtrAtts*)_TB;_T4=p;_T5=_T4->aqual;
return _T5;}_TL3C6:{struct Cyc_Warn_String_Warn_Warg_struct _TC;_TC.tag=0;
_TC.f1=_tag_fat("pointer_elt_type",sizeof(char),17U);_T6=_TC;}{struct Cyc_Warn_String_Warn_Warg_struct _TC=_T6;void*_TD[1];_TD[0]=& _TC;_T8=Cyc_Warn_impos2;{int(*_TE)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T8;_T7=_TE;}_T9=_tag_fat(_TD,sizeof(void*),1);_T7(_T9);};}
# 1901
int Cyc_Tcutil_admits_zero(void*t){int*_T0;unsigned _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;unsigned _T5;struct Cyc_Absyn_PtrInfo _T6;struct Cyc_Absyn_PtrAtts _T7;struct Cyc_Absyn_PtrInfo _T8;struct Cyc_Absyn_PtrAtts _T9;int _TA;int _TB;
void*_TC=Cyc_Absyn_compress(t);void*_TD;void*_TE;_T0=(int*)_TC;_T1=*_T0;switch(_T1){case 0: _T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TC;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;switch(_T5){case 1: goto _LL4;case 2: _LL4:
# 1904
 return 1;default: goto _LL7;};case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TF=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TC;_T6=_TF->f1;_T7=_T6.ptr_atts;_TE=_T7.nullable;_T8=_TF->f1;_T9=_T8.ptr_atts;_TD=_T9.bounds;}{void*nullable=_TE;void*bounds=_TD;_TB=
# 1909
Cyc_Unify_unify(Cyc_Absyn_fat_bound_type,bounds);if(_TB)goto _TL3CA;else{goto _TL3CC;}_TL3CC: _TA=Cyc_Tcutil_force_type2bool(0,nullable);goto _TL3CB;_TL3CA: _TA=0;_TL3CB: return _TA;}default: _LL7:
 return 0;};}
# 1915
int Cyc_Tcutil_is_zero(struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T3;union Cyc_Absyn_Cnst _T4;struct _union_Cnst_Wchar_c _T5;unsigned _T6;struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T7;union Cyc_Absyn_Cnst _T8;struct _union_Cnst_Int_c _T9;struct _tuple6 _TA;int _TB;struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_TC;union Cyc_Absyn_Cnst _TD;struct _union_Cnst_Char_c _TE;struct _tuple4 _TF;char _T10;struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T11;union Cyc_Absyn_Cnst _T12;struct _union_Cnst_Short_c _T13;struct _tuple5 _T14;short _T15;struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T16;union Cyc_Absyn_Cnst _T17;struct _union_Cnst_LongLong_c _T18;struct _tuple7 _T19;long long _T1A;union Cyc_Absyn_Cnst _T1B;struct _union_Cnst_Wchar_c _T1C;struct _fat_ptr _T1D;unsigned char*_T1E;const char*_T1F;const char*_T20;char _T21;int _T22;struct _fat_ptr _T23;unsigned char*_T24;const char*_T25;char _T26;int _T27;struct _fat_ptr _T28;unsigned char*_T29;const char*_T2A;char _T2B;int _T2C;struct _fat_ptr _T2D;unsigned char*_T2E;const char*_T2F;char _T30;int _T31;int _T32;unsigned long _T33;unsigned long _T34;struct _fat_ptr _T35;unsigned char*_T36;const char*_T37;int _T38;char _T39;int _T3A;void*_T3B;int _T3C;int _T3D;_T0=e;{
void*_T3E=_T0->r;struct Cyc_Absyn_Exp*_T3F;void*_T40;struct _fat_ptr _T41;_T1=(int*)_T3E;_T2=*_T1;switch(_T2){case 0: _T3=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T3E;_T4=_T3->f1;_T5=_T4.Wchar_c;_T6=_T5.tag;switch(_T6){case 5: _T7=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T3E;_T8=_T7->f1;_T9=_T8.Int_c;_TA=_T9.val;_TB=_TA.f1;if(_TB!=0)goto _TL3CF;goto _LL4;_TL3CF: goto _LLF;case 2: _TC=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T3E;_TD=_TC->f1;_TE=_TD.Char_c;_TF=_TE.val;_T10=_TF.f1;if(_T10!=0)goto _TL3D1;_LL4: goto _LL6;_TL3D1: goto _LLF;case 4: _T11=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T3E;_T12=_T11->f1;_T13=_T12.Short_c;_T14=_T13.val;_T15=_T14.f1;if(_T15!=0)goto _TL3D3;_LL6: goto _LL8;_TL3D3: goto _LLF;case 6: _T16=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T3E;_T17=_T16->f1;_T18=_T17.LongLong_c;_T19=_T18.val;_T1A=_T19.f1;if(_T1A!=0)goto _TL3D5;_LL8: goto _LLA;_TL3D5: goto _LLF;case 3:{struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_T42=(struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_T3E;_T1B=_T42->f1;_T1C=_T1B.Wchar_c;_T41=_T1C.val;}{struct _fat_ptr s=_T41;
# 1923
unsigned long l=Cyc_strlen(s);
int i=0;
if(l < 2U)goto _TL3D7;_T1D=s;_T1E=_T1D.curr;_T1F=(const char*)_T1E;_T20=_check_null(_T1F);_T21=_T20[0];_T22=(int)_T21;if(_T22!=92)goto _TL3D7;_T23=s;_T24=_T23.curr;_T25=(const char*)_T24;_T26=_T25[1];_T27=(int)_T26;
if(_T27!=48)goto _TL3D9;i=2;goto _TL3DA;
_TL3D9: _T28=s;_T29=_T28.curr;_T2A=(const char*)_T29;_T2B=_T2A[1];_T2C=(int)_T2B;if(_T2C!=120)goto _TL3DB;if(l < 3U)goto _TL3DB;_T2D=s;_T2E=_T2D.curr;_T2F=(const char*)_T2E;_T30=_T2F[2];_T31=(int)_T30;if(_T31!=48)goto _TL3DB;i=3;goto _TL3DC;
_TL3DB: return 0;_TL3DC: _TL3DA:
 _TL3E0: _T32=i;_T33=(unsigned long)_T32;_T34=l;if(_T33 < _T34)goto _TL3DE;else{goto _TL3DF;}
_TL3DE: _T35=s;_T36=_T35.curr;_T37=(const char*)_T36;_T38=i;_T39=_T37[_T38];_T3A=(int)_T39;if(_T3A==48)goto _TL3E1;return 0;_TL3E1:
# 1929
 i=i + 1;goto _TL3E0;_TL3DF:
# 1931
 return 1;_TL3D7:
# 1933
 return 0;}default: goto _LLF;};case 2: _LLA:
# 1921
 return 1;case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T42=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T3E;_T3B=_T42->f1;_T40=(void*)_T3B;_T3F=_T42->f2;}{void*t=_T40;struct Cyc_Absyn_Exp*e2=_T3F;_T3D=
# 1934
Cyc_Tcutil_is_zero(e2);if(!_T3D)goto _TL3E3;_T3C=Cyc_Tcutil_admits_zero(t);goto _TL3E4;_TL3E3: _T3C=0;_TL3E4: return _T3C;}default: _LLF:
 return 0;};}}
# 1941
int Cyc_Tcutil_zero_to_null(void*t2,struct Cyc_Absyn_Exp*e1){int _T0;int*_T1;int _T2;struct Cyc_Absyn_PtrInfo _T3;struct Cyc_Absyn_PtrAtts _T4;int _T5;struct Cyc_Absyn_Exp*_T6;struct Cyc_Absyn_Exp*_T7;struct Cyc_Absyn_Exp*_T8;_T0=
Cyc_Tcutil_is_zero(e1);if(_T0)goto _TL3E5;else{goto _TL3E7;}
_TL3E7: return 0;_TL3E5: {
void*_T9=Cyc_Absyn_compress(t2);void*_TA;_T1=(int*)_T9;_T2=*_T1;if(_T2!=4)goto _TL3E8;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TB=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T9;_T3=_TB->f1;_T4=_T3.ptr_atts;_TA=_T4.nullable;}{void*nbl=_TA;_T5=
# 1946
Cyc_Tcutil_force_type2bool(1,nbl);if(_T5)goto _TL3EA;else{goto _TL3EC;}
_TL3EC: return 0;_TL3EA: _T6=e1;_T7=
Cyc_Absyn_null_exp(0U);_T6->r=_T7->r;_T8=e1;
_T8->topt=t2;
return 1;}_TL3E8:
 return 0;;}}
# 1955
int Cyc_Tcutil_is_aliasable_qual(void*aq){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;unsigned _T5;void*_T6;enum Cyc_Absyn_AliasQualVal _T7;int _T8;int _T9;struct Cyc_List_List*_TA;struct Cyc_List_List*_TB;struct Cyc_List_List*_TC;void*_TD;int _TE;
void*_TF=Cyc_Absyn_compress(aq);struct Cyc_List_List*_T10;enum Cyc_Absyn_AliasQualVal _T11;_T0=(int*)_TF;_T1=*_T0;if(_T1!=0)goto _TL3ED;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TF;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;switch(_T5){case 16:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T12=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TF;_T6=_T12->f1;{struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*_T13=(struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)_T6;_T11=_T13->f1;}}{enum Cyc_Absyn_AliasQualVal aqv=_T11;_T7=aqv;_T8=(int)_T7;_T9=_T8==0;
# 1958
return _T9;}case 17:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T12=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TF;_T10=_T12->f2;}{struct Cyc_List_List*tv_bnd=_T10;_TA=
# 1960
_check_null(tv_bnd);_TB=_TA->tl;_TC=_check_null(_TB);_TD=_TC->hd;_TE=Cyc_Tcutil_is_aliasable_qual(_TD);return _TE;}default: goto _LL5;}goto _TL3EE;_TL3ED: _LL5:
# 1962
 return 0;_TL3EE:;}struct _tuple22{struct Cyc_Absyn_Decl*f0;struct Cyc_Absyn_Exp*f1;int f2;};
# 1971
struct _tuple22 Cyc_Tcutil_insert_alias(struct Cyc_Absyn_Exp*e,void*e_type){struct _tuple1*_T0;struct _tuple1*_T1;struct _fat_ptr*_T2;struct _fat_ptr _T3;struct Cyc_Int_pa_PrintArg_struct _T4;int _T5;int _T6;struct _fat_ptr _T7;struct _fat_ptr _T8;struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T9;void*_TA;struct Cyc_Absyn_Exp*_TB;unsigned _TC;struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_TD;struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_TE;void*_TF;int*_T10;int _T11;struct Cyc_Absyn_PtrInfo _T12;struct Cyc_Absyn_PtrInfo _T13;struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T14;void**_T15;struct Cyc_Absyn_PtrInfo _T16;struct Cyc_Absyn_PtrAtts _T17;void**_T18;void*_T19;int*_T1A;int _T1B;struct Cyc_Absyn_Evar_Absyn_Type_struct*_T1C;void**_T1D;void**_T1E;void**_T1F;int _T20;struct Cyc_Warn_String_Warn_Warg_struct _T21;struct Cyc_Absyn_Exp*_T22;unsigned _T23;struct _fat_ptr _T24;struct Cyc_Absyn_Exp*_T25;struct Cyc_Absyn_Vardecl*_T26;struct Cyc_Absyn_Tvar*_T27;struct Cyc_Absyn_Vardecl*_T28;struct Cyc_Absyn_Exp*_T29;struct Cyc_Absyn_Exp*_T2A;unsigned _T2B;struct _tuple22 _T2C;
static struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct eff_kb={0,(struct Cyc_Absyn_Kind*)& Cyc_Kinds_ek};
int success=1;
# 1975
static int counter=0;
struct _tuple1*v;v=_cycalloc(sizeof(struct _tuple1));_T0=v;_T0->f0=Cyc_Absyn_Loc_n();_T1=v;{struct _fat_ptr*_T2D=_cycalloc(sizeof(struct _fat_ptr));{struct Cyc_Int_pa_PrintArg_struct _T2E;_T2E.tag=1;_T5=counter;counter=_T5 + 1;_T6=_T5;_T2E.f1=(unsigned long)_T6;_T4=_T2E;}{struct Cyc_Int_pa_PrintArg_struct _T2E=_T4;void*_T2F[1];_T2F[0]=& _T2E;_T7=_tag_fat("__aliasvar%d",sizeof(char),13U);_T8=_tag_fat(_T2F,sizeof(void*),1);_T3=Cyc_aprintf(_T7,_T8);}*_T2D=_T3;_T2=(struct _fat_ptr*)_T2D;}_T1->f1=_T2;{
struct Cyc_Absyn_Vardecl*vd=Cyc_Absyn_new_vardecl(0U,v,e_type,e,0);{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T2D=_cycalloc(sizeof(struct Cyc_Absyn_Local_b_Absyn_Binding_struct));_T2D->tag=4;
_T2D->f1=vd;_T9=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_T2D;}_TA=(void*)_T9;_TB=e;_TC=_TB->loc;{struct Cyc_Absyn_Exp*ve=Cyc_Absyn_varb_exp(_TA,_TC);_TD=& eff_kb;_TE=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_TD;_TF=(void*)_TE;{
# 1984
struct Cyc_Absyn_Tvar*tv=Cyc_Tcutil_new_tvar(_TF);{
void*_T2D=Cyc_Absyn_compress(e_type);void*_T2E;void*_T2F;struct Cyc_Absyn_Tqual _T30;void*_T31;_T10=(int*)_T2D;_T11=*_T10;if(_T11!=4)goto _TL3F0;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T32=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T2D;_T12=_T32->f1;_T31=_T12.elt_type;_T13=_T32->f1;_T30=_T13.elt_tq;_T14=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T2D;_T15=& _T14->f1.ptr_atts.eff;_T2F=(void**)_T15;_T16=_T32->f1;_T17=_T16.ptr_atts;_T2E=_T17.aqual;}{void*et=_T31;struct Cyc_Absyn_Tqual tq=_T30;void**old_r=(void**)_T2F;void*aq=_T2E;_T18=old_r;_T19=*_T18;{
# 1987
void*_T32=Cyc_Absyn_compress(_T19);struct Cyc_Core_Opt*_T33;void*_T34;_T1A=(int*)_T32;_T1B=*_T1A;if(_T1B!=1)goto _TL3F2;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_T35=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T32;_T1C=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T32;_T1D=& _T1C->f2;_T34=(void**)_T1D;_T33=_T35->f4;}{void**topt=(void**)_T34;struct Cyc_Core_Opt*ts=_T33;
# 1989
void*new_r=Cyc_Absyn_var_type(tv);_T1E=topt;
*_T1E=new_r;goto _LL5;}_TL3F2: _T1F=old_r;
# 2007 "tcutil.cyc"
*_T1F=Cyc_Absyn_var_type(tv);goto _LL5;_LL5:;}_T20=
# 2012
Cyc_Tcutil_is_aliasable_qual(aq);if(_T20)goto _TL3F4;else{goto _TL3F6;}
_TL3F6:{struct Cyc_Warn_String_Warn_Warg_struct _T32;_T32.tag=0;_T32.f1=_tag_fat("Automatic alias coercion of restricted pointers not supported",sizeof(char),62U);_T21=_T32;}{struct Cyc_Warn_String_Warn_Warg_struct _T32=_T21;void*_T33[1];_T33[0]=& _T32;_T22=e;_T23=_T22->loc;_T24=_tag_fat(_T33,sizeof(void*),1);Cyc_Warn_err2(_T23,_T24);}
success=0;goto _TL3F5;_TL3F4: _TL3F5: goto _LL0;}_TL3F0: goto _LL0;_LL0:;}_T25=e;
# 2019
_T25->topt=0;_T26=vd;
_T26->initializer=0;_T27=tv;_T28=vd;_T29=e;_T2A=e;_T2B=_T2A->loc;{
# 2023
struct Cyc_Absyn_Decl*d=Cyc_Absyn_alias_decl(_T27,_T28,_T29,_T2B);{struct _tuple22 _T2D;
# 2025
_T2D.f0=d;_T2D.f1=ve;_T2D.f2=success;_T2C=_T2D;}return _T2C;}}}}}
# 2030
static int Cyc_Tcutil_can_insert_alias(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e,void*e_type,void*wants_type,unsigned loc){int _T0;int _T1;int _T2;int*_T3;int _T4;struct Cyc_Absyn_PtrInfo _T5;struct Cyc_Absyn_PtrAtts _T6;struct Cyc_Absyn_PtrInfo _T7;struct Cyc_Absyn_PtrAtts _T8;int _T9;int _TA;struct Cyc_Absyn_Kind*_TB;enum Cyc_Absyn_KindQual _TC;int _TD;_T0=
# 2033
Cyc_Tcutil_is_noalias_path(aqb,e);if(!_T0)goto _TL3F7;_T1=
Cyc_Tcutil_is_noalias_pointer(aqb,e_type,0);
# 2033
if(!_T1)goto _TL3F7;_T2=
# 2035
Cyc_Tcutil_is_pointer_type(e_type);
# 2033
if(!_T2)goto _TL3F7;{
# 2038
void*_TE=Cyc_Absyn_compress(wants_type);void*_TF;void*_T10;_T3=(int*)_TE;_T4=*_T3;if(_T4!=4)goto _TL3F9;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T11=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TE;_T5=_T11->f1;_T6=_T5.ptr_atts;_T10=_T6.eff;_T7=_T11->f1;_T8=_T7.ptr_atts;_TF=_T8.aqual;}{void*r2=_T10;void*aq=_TF;_T9=
# 2040
Cyc_Tcutil_is_heap_rgn_type(r2);if(!_T9)goto _TL3FB;return 0;_TL3FB: {
struct Cyc_Absyn_Kind*k=Cyc_Tcutil_type_kind(r2);_TB=k;_TC=_TB->kind;_TD=(int)_TC;
if(_TD!=3)goto _TL3FD;_TA=
Cyc_Tcutil_is_aliasable_qual(aq);goto _TL3FE;_TL3FD: _TA=0;_TL3FE:
# 2042
 return _TA;}}_TL3F9:
# 2044
 return 0;;}goto _TL3F8;_TL3F7: _TL3F8:
# 2047
 return 0;}
# 2051
int Cyc_Tcutil_coerce_arg(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e,void*t2,int*alias_coercion){struct Cyc_Absyn_Exp*_T0;void*_T1;void*_T2;int _T3;int _T4;int _T5;int _T6;struct Cyc_Warn_String_Warn_Warg_struct _T7;struct Cyc_Warn_Typ_Warn_Warg_struct _T8;struct Cyc_Warn_String_Warn_Warg_struct _T9;struct Cyc_Warn_Typ_Warn_Warg_struct _TA;struct Cyc_Warn_String_Warn_Warg_struct _TB;struct Cyc_Absyn_Exp*_TC;unsigned _TD;struct _fat_ptr _TE;int _TF;int*_T10;unsigned long _T11;struct Cyc_List_List*_T12;struct Cyc_Absyn_Exp*_T13;void*_T14;void*_T15;struct Cyc_Absyn_Exp*_T16;unsigned _T17;struct Cyc_Absyn_Exp*_T18;unsigned _T19;void*_T1A;void*_T1B;enum Cyc_Absyn_Coercion _T1C;int _T1D;enum Cyc_Absyn_Coercion _T1E;int _T1F;int*_T20;unsigned long _T21;int*_T22;int _T23;int*_T24;unsigned long _T25;int*_T26;int _T27;struct Cyc_Warn_String_Warn_Warg_struct _T28;struct Cyc_Warn_Exp_Warn_Warg_struct _T29;struct Cyc_Warn_String_Warn_Warg_struct _T2A;struct Cyc_Warn_Typ_Warn_Warg_struct _T2B;struct Cyc_Warn_String_Warn_Warg_struct _T2C;struct Cyc_Warn_Typ_Warn_Warg_struct _T2D;struct Cyc_Absyn_Exp*_T2E;unsigned _T2F;struct _fat_ptr _T30;_T0=e;_T1=_T0->topt;_T2=
# 2054
_check_null(_T1);{void*t1=Cyc_Absyn_compress(_T2);_T3=
# 2057
Cyc_Tcutil_is_arithmetic_type(t2);if(!_T3)goto _TL3FF;_T4=Cyc_Tcutil_is_arithmetic_type(t1);if(!_T4)goto _TL3FF;_T5=Cyc_Tcutil_is_tag_type(t2);if(_T5)goto _TL3FF;else{goto _TL401;}
# 2059
_TL401: _T6=Cyc_Tcutil_will_lose_precision(t1,t2);if(!_T6)goto _TL402;{struct Cyc_Warn_String_Warn_Warg_struct _T31;_T31.tag=0;
_T31.f1=_tag_fat("integral size mismatch; ",sizeof(char),25U);_T7=_T31;}{struct Cyc_Warn_String_Warn_Warg_struct _T31=_T7;{struct Cyc_Warn_Typ_Warn_Warg_struct _T32;_T32.tag=2;
_T32.f1=t1;_T8=_T32;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T32=_T8;{struct Cyc_Warn_String_Warn_Warg_struct _T33;_T33.tag=0;_T33.f1=_tag_fat(" -> ",sizeof(char),5U);_T9=_T33;}{struct Cyc_Warn_String_Warn_Warg_struct _T33=_T9;{struct Cyc_Warn_Typ_Warn_Warg_struct _T34;_T34.tag=2;_T34.f1=t2;_TA=_T34;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T34=_TA;{struct Cyc_Warn_String_Warn_Warg_struct _T35;_T35.tag=0;_T35.f1=_tag_fat(" conversion supplied",sizeof(char),21U);_TB=_T35;}{struct Cyc_Warn_String_Warn_Warg_struct _T35=_TB;void*_T36[5];_T36[0]=& _T31;_T36[1]=& _T32;_T36[2]=& _T33;_T36[3]=& _T34;_T36[4]=& _T35;_TC=e;_TD=_TC->loc;_TE=_tag_fat(_T36,sizeof(void*),5);Cyc_Warn_warn2(_TD,_TE);}}}}}goto _TL403;_TL402: _TL403:
 Cyc_Tcutil_unchecked_cast(e,t2,1U);
return 1;_TL3FF: _TF=
# 2066
Cyc_Tcutil_zero_to_null(t2,e);if(!_TF)goto _TL404;
return 1;_TL404: {
# 2069
int do_alias_coercion=0;_T10=alias_coercion;_T11=(unsigned long)_T10;
# 2071
if(!_T11)goto _TL406;_T12=aqb;_T13=e;_T14=t1;_T15=t2;_T16=e;_T17=_T16->loc;
# 2073
do_alias_coercion=Cyc_Tcutil_can_insert_alias(_T12,_T13,_T14,_T15,_T17);goto _TL407;_TL406: _TL407: _T18=e;_T19=_T18->loc;_T1A=t1;_T1B=t2;{
# 2076
enum Cyc_Absyn_Coercion c=Cyc_Subtype_coercible_warn(_T19,_T1A,_T1B);_T1C=c;_T1D=(int)_T1C;
# 2078
if(_T1D==0)goto _TL408;_T1E=c;_T1F=(int)_T1E;
if(_T1F==1)goto _TL40A;_T20=alias_coercion;_T21=(unsigned long)_T20;
if(!_T21)goto _TL40C;_T22=alias_coercion;
*_T22=do_alias_coercion;goto _TL40D;_TL40C: _TL40D:
 Cyc_Tcutil_unchecked_cast(e,t2,c);goto _TL40B;_TL40A: _TL40B: _T23=Cyc_Flags_warn_alias_coerce;
# 2085
if(!_T23)goto _TL40E;_T24=alias_coercion;_T25=(unsigned long)_T24;if(!_T25)goto _TL40E;_T26=alias_coercion;_T27=*_T26;if(!_T27)goto _TL40E;{struct Cyc_Warn_String_Warn_Warg_struct _T31;_T31.tag=0;
_T31.f1=_tag_fat("implicit alias coercion for ",sizeof(char),29U);_T28=_T31;}{struct Cyc_Warn_String_Warn_Warg_struct _T31=_T28;{struct Cyc_Warn_Exp_Warn_Warg_struct _T32;_T32.tag=4;_T32.f1=e;_T29=_T32;}{struct Cyc_Warn_Exp_Warn_Warg_struct _T32=_T29;{struct Cyc_Warn_String_Warn_Warg_struct _T33;_T33.tag=0;_T33.f1=_tag_fat(":",sizeof(char),2U);_T2A=_T33;}{struct Cyc_Warn_String_Warn_Warg_struct _T33=_T2A;{struct Cyc_Warn_Typ_Warn_Warg_struct _T34;_T34.tag=2;_T34.f1=t1;_T2B=_T34;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T34=_T2B;{struct Cyc_Warn_String_Warn_Warg_struct _T35;_T35.tag=0;_T35.f1=_tag_fat(" to ",sizeof(char),5U);_T2C=_T35;}{struct Cyc_Warn_String_Warn_Warg_struct _T35=_T2C;{struct Cyc_Warn_Typ_Warn_Warg_struct _T36;_T36.tag=2;_T36.f1=t2;_T2D=_T36;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T36=_T2D;void*_T37[6];_T37[0]=& _T31;_T37[1]=& _T32;_T37[2]=& _T33;_T37[3]=& _T34;_T37[4]=& _T35;_T37[5]=& _T36;_T2E=e;_T2F=_T2E->loc;_T30=_tag_fat(_T37,sizeof(void*),6);Cyc_Warn_warn2(_T2F,_T30);}}}}}}goto _TL40F;_TL40E: _TL40F:
 return 1;_TL408:
# 2089
 return 0;}}}}
# 2095
int Cyc_Tcutil_coerce_assign(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e,void*t){struct Cyc_List_List*_T0;struct Cyc_Absyn_Exp*_T1;void*_T2;int*_T3;int _T4;
# 2098
int bogus=0;_T0=aqb;_T1=e;_T2=t;_T3=& bogus;_T4=
Cyc_Tcutil_coerce_arg(_T0,_T1,_T2,_T3);return _T4;}
# 2103
void*Cyc_Tcutil_lookup_aquals_opt(struct Cyc_List_List*aquals_map,void*t){void*_T0;int*_T1;int _T2;void*_T3;void*_T4;struct Cyc_Absyn_Kind*_T5;unsigned long _T6;struct Cyc_Absyn_Kind*_T7;enum Cyc_Absyn_KindQual _T8;int _T9;struct _handler_cons*_TA;int _TB;struct Cyc_List_List*_TC;void*_TD;void*_TE;struct Cyc_Core_Not_found_exn_struct*_TF;char*_T10;char*_T11;
void*aqt=t;{struct Cyc_Absyn_Tvar*_T12;_T0=t;_T1=(int*)_T0;_T2=*_T1;if(_T2!=2)goto _TL410;_T3=t;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_T13=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T3;_T12=_T13->f1;}{struct Cyc_Absyn_Tvar*tv=_T12;
# 2107
struct Cyc_Core_Opt _T13=Cyc_Kinds_tvar_kind_opt(tv);struct Cyc_Absyn_Kind*_T14;_T4=_T13.v;_T14=(struct Cyc_Absyn_Kind*)_T4;{struct Cyc_Absyn_Kind*k=_T14;_T5=k;_T6=(unsigned long)_T5;
if(!_T6)goto _TL412;_T7=k;_T8=_T7->kind;_T9=(int)_T8;if(_T9==7)goto _TL412;
aqt=Cyc_Absyn_aqualsof_type(t);goto _TL413;_TL412: _TL413: goto _LL0;}}_TL410: goto _LL0;_LL0:;}{struct _handler_cons _T12;_TA=& _T12;_push_handler(_TA);{int _T13=0;_TB=setjmp(_T12.handler);if(!_TB)goto _TL414;_T13=1;goto _TL415;_TL414: _TL415: if(_T13)goto _TL416;else{goto _TL418;}_TL418: _TC=aquals_map;_TD=aqt;{
# 2116
void*ret=Cyc_List_assoc_cmp(Cyc_Tcutil_typecmp,_TC,_TD);void*_T14=ret;_npop_handler(0);return _T14;}_pop_handler();goto _TL417;_TL416: _TE=Cyc_Core_get_exn_thrown();{void*_T14=(void*)_TE;void*_T15;_TF=(struct Cyc_Core_Not_found_exn_struct*)_T14;_T10=_TF->tag;_T11=Cyc_Core_Not_found;if(_T10!=_T11)goto _TL419;
# 2120
return 0;_TL419: _T15=_T14;{void*exn=_T15;_rethrow(exn);};}_TL417:;}}}
# 2124
void*Cyc_Tcutil_lookup_aquals(struct Cyc_List_List*aquals_map,void*t){void*_T0;
void*aq=Cyc_Tcutil_lookup_aquals_opt(aquals_map,t);
if(aq!=0)goto _TL41B;_T0=Cyc_Absyn_al_qual_type;goto _TL41C;_TL41B: _T0=aq;_TL41C: return _T0;}
# 2129
static int Cyc_Tcutil_substitute_vars_exp(struct Cyc_List_List*subst,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;int _T2;struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T3;void*_T4;int*_T5;unsigned _T6;void*_T7;void*_T8;void*_T9;struct Cyc_List_List*_TA;void*_TB;struct _tuple16*_TC;struct Cyc_Absyn_Vardecl*_TD;struct _tuple1*_TE;struct Cyc_Absyn_Vardecl*_TF;struct _tuple1*_T10;int _T11;struct Cyc_Absyn_Exp*_T12;struct Cyc_Absyn_Exp*_T13;struct Cyc_List_List*_T14;_T0=e;{
void*_T15=_T0->r;struct Cyc_Absyn_Vardecl*_T16;_T1=(int*)_T15;_T2=*_T1;if(_T2!=1)goto _TL41D;_T3=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T15;_T4=_T3->f1;_T5=(int*)_T4;_T6=*_T5;switch(_T6){case 3:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T17=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T15;_T7=_T17->f1;{struct Cyc_Absyn_Param_b_Absyn_Binding_struct*_T18=(struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_T7;_T16=_T18->f1;}}{struct Cyc_Absyn_Vardecl*vd=_T16;_T16=vd;goto _LL4;}case 4:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T17=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T15;_T8=_T17->f1;{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T18=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_T8;_T16=_T18->f1;}}_LL4: {struct Cyc_Absyn_Vardecl*vd=_T16;_T16=vd;goto _LL6;}case 5:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T17=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T15;_T9=_T17->f1;{struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*_T18=(struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_T9;_T16=_T18->f1;}}_LL6: {struct Cyc_Absyn_Vardecl*vd=_T16;{
# 2134
struct Cyc_List_List*s=subst;_TL423: if(s!=0)goto _TL421;else{goto _TL422;}
_TL421: _TA=s;_TB=_TA->hd;_TC=(struct _tuple16*)_TB;{struct _tuple16 _T17=*_TC;struct Cyc_Absyn_Exp*_T18;struct Cyc_Absyn_Vardecl*_T19;_T19=_T17.f0;_T18=_T17.f1;{struct Cyc_Absyn_Vardecl*vdopt=_T19;struct Cyc_Absyn_Exp*e2=_T18;
if(vdopt!=0)goto _TL424;goto _TL420;_TL424: _TD=vdopt;_TE=_TD->name;_TF=vd;_T10=_TF->name;_T11=
Cyc_Absyn_qvar_cmp(_TE,_T10);if(_T11!=0)goto _TL426;_T12=e;_T13=e2;
_T12->r=_T13->r;
return 0;_TL426:;}}_TL420: _T14=s;
# 2134
s=_T14->tl;goto _TL423;_TL422:;}goto _LL0;}default: goto _LL7;}goto _TL41E;_TL41D: _LL7: goto _LL0;_TL41E: _LL0:;}
# 2145
return 1;}
# 2148
static int Cyc_Tcutil_substitute_vars_stmt(struct Cyc_List_List*subst,struct Cyc_Absyn_Stmt*e){
return 1;}
# 2152
void Cyc_Tcutil_substitute_vars(struct Cyc_List_List*subst,struct Cyc_Absyn_Exp*e){void(*_T0)(int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*,struct Cyc_Absyn_Exp*);void(*_T1)(int(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Exp*);struct Cyc_List_List*_T2;struct Cyc_Absyn_Exp*_T3;_T1=Cyc_Absyn_visit_exp;{
void(*_T4)(int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*,struct Cyc_Absyn_Exp*)=(void(*)(int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*,struct Cyc_Absyn_Exp*))_T1;_T0=_T4;}_T2=subst;_T3=e;_T0(Cyc_Tcutil_substitute_vars_exp,Cyc_Tcutil_substitute_vars_stmt,_T2,_T3);}
# 2157
void Cyc_Tcutil_unchecked_cast(struct Cyc_Absyn_Exp*e,void*t,enum Cyc_Absyn_Coercion c){struct Cyc_Absyn_Exp*_T0;void*_T1;void*_T2;void*_T3;int _T4;enum Cyc_Absyn_Coercion _T5;int _T6;int*_T7;int _T8;struct _fat_ptr*_T9;struct _fat_ptr _TA;struct Cyc_Int_pa_PrintArg_struct _TB;int _TC;int _TD;struct _fat_ptr _TE;struct _fat_ptr _TF;struct _tuple1*_T10;struct _tuple1*_T11;struct Cyc_Absyn_Exp*_T12;unsigned _T13;struct _tuple1*_T14;struct Cyc_Absyn_Vardecl*_T15;void*_T16;struct Cyc_Absyn_Exp*_T17;struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T18;void*_T19;struct Cyc_Absyn_Exp*_T1A;unsigned _T1B;struct Cyc_Absyn_Exp*_T1C;struct Cyc_Absyn_Vardecl*_T1D;struct _tuple16 _T1E;struct Cyc_List_List _T1F;void(*_T20)(int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*,struct Cyc_Absyn_Exp*);void(*_T21)(int(*)(void*,struct Cyc_Absyn_Exp*),int(*)(void*,struct Cyc_Absyn_Stmt*),void*,struct Cyc_Absyn_Exp*);struct Cyc_List_List*_T22;struct Cyc_List_List*_T23;struct Cyc_Absyn_Exp*_T24;struct Cyc_Absyn_Exp*_T25;struct Cyc_Absyn_Exp*_T26;unsigned _T27;struct Cyc_Absyn_Exp*_T28;struct Cyc_Absyn_Decl*_T29;struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T2A;struct Cyc_Absyn_Exp*_T2B;struct Cyc_Absyn_Exp*_T2C;struct Cyc_Absyn_Exp*_T2D;unsigned _T2E;struct Cyc_Absyn_Stmt*_T2F;struct Cyc_Absyn_Exp*_T30;struct Cyc_Absyn_Exp*_T31;unsigned _T32;struct Cyc_Absyn_Stmt*_T33;struct Cyc_Absyn_Exp*_T34;unsigned _T35;struct Cyc_Absyn_Stmt*_T36;struct Cyc_Absyn_Exp*_T37;unsigned _T38;struct Cyc_Absyn_Exp*_T39;struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_T3A;struct Cyc_Absyn_Exp*_T3B;struct Cyc_Warn_String_Warn_Warg_struct _T3C;struct Cyc_Warn_Typ_Warn_Warg_struct _T3D;struct Cyc_Warn_String_Warn_Warg_struct _T3E;struct Cyc_Warn_Exp_Warn_Warg_struct _T3F;struct Cyc_Warn_String_Warn_Warg_struct _T40;int(*_T41)(struct _fat_ptr);void*(*_T42)(struct _fat_ptr);struct _fat_ptr _T43;struct Cyc_Absyn_Exp*_T44;struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T45;struct Cyc_Absyn_Exp*_T46;
static int ucast_ctr=0;_T0=e;_T1=_T0->topt;_T2=
_check_null(_T1);_T3=t;_T4=Cyc_Unify_unify(_T2,_T3);if(!_T4)goto _TL428;
return;_TL428: {
struct Cyc_Absyn_Exp*inner=Cyc_Absyn_copy_exp(e);_T5=c;_T6=(int)_T5;
if(_T6!=3)goto _TL42A;{
void*_T47=Cyc_Absyn_compress(t);struct Cyc_AssnDef_ExistAssnFn*_T48;struct Cyc_Absyn_Exp*_T49;struct Cyc_Absyn_Vardecl*_T4A;_T7=(int*)_T47;_T8=*_T7;if(_T8!=12)goto _TL42C;{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_T4B=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_T47;_T4A=_T4B->f1;_T49=_T4B->f2;_T48=_T4B->f3;}{struct Cyc_Absyn_Vardecl*vd=_T4A;struct Cyc_Absyn_Exp*assn_exp=_T49;struct Cyc_AssnDef_ExistAssnFn*af=_T48;
# 2167
struct _fat_ptr*ux;ux=_cycalloc(sizeof(struct _fat_ptr));_T9=ux;{struct Cyc_Int_pa_PrintArg_struct _T4B;_T4B.tag=1;_TC=ucast_ctr;ucast_ctr=_TC + 1;_TD=_TC;_T4B.f1=(unsigned)_TD;_TB=_T4B;}{struct Cyc_Int_pa_PrintArg_struct _T4B=_TB;void*_T4C[1];_T4C[0]=& _T4B;_TE=_tag_fat("_utmpS%X",sizeof(char),9U);_TF=_tag_fat(_T4C,sizeof(void*),1);_TA=Cyc_aprintf(_TE,_TF);}*_T9=_TA;{
struct _tuple1*x;x=_cycalloc(sizeof(struct _tuple1));_T10=x;_T10->f0=Cyc_Absyn_Loc_n();_T11=x;_T11->f1=ux;_T12=e;_T13=_T12->loc;_T14=x;_T15=vd;_T16=_T15->type;_T17=inner;{
struct Cyc_Absyn_Vardecl*newvd=Cyc_Absyn_new_vardecl(_T13,_T14,_T16,_T17,0);{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T4B=_cycalloc(sizeof(struct Cyc_Absyn_Local_b_Absyn_Binding_struct));_T4B->tag=4;
_T4B->f1=newvd;_T18=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_T4B;}_T19=(void*)_T18;_T1A=e;_T1B=_T1A->loc;{struct Cyc_Absyn_Exp*new_arg=Cyc_Absyn_varb_exp(_T19,_T1B);_T1C=new_arg;_T1D=vd;
_T1C->topt=_T1D->type;{
struct Cyc_Absyn_Exp*chk=Cyc_Tcutil_deep_copy_exp(1,assn_exp);{struct _tuple16 _T4B;
_T4B.f0=vd;_T4B.f1=new_arg;_T1E=_T4B;}{struct _tuple16 p=_T1E;{struct Cyc_List_List _T4B;
_T4B.hd=& p;_T4B.tl=0;_T1F=_T4B;}{struct Cyc_List_List subst=_T1F;_T21=Cyc_Absyn_visit_exp;{
void(*_T4B)(int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*,struct Cyc_Absyn_Exp*)=(void(*)(int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Exp*),int(*)(struct Cyc_List_List*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*,struct Cyc_Absyn_Exp*))_T21;_T20=_T4B;}_T22=& subst;_T23=(struct Cyc_List_List*)_T22;_T24=chk;_T20(Cyc_Tcutil_substitute_vars_exp,Cyc_Tcutil_substitute_vars_stmt,_T23,_T24);_T25=chk;_T26=e;_T27=_T26->loc;{
struct Cyc_Absyn_Exp*asrt=Cyc_Absyn_assert_exp(_T25,0,_T27);_T28=asrt;
_T28->topt=Cyc_Absyn_sint_type;{struct Cyc_Absyn_Decl*_T4B=_cycalloc(sizeof(struct Cyc_Absyn_Decl));{struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_T4C=_cycalloc(sizeof(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct));_T4C->tag=0;
_T4C->f1=newvd;_T2A=(struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_T4C;}_T4B->r=(void*)_T2A;_T2B=e;_T4B->loc=_T2B->loc;_T29=(struct Cyc_Absyn_Decl*)_T4B;}_T2C=asrt;_T2D=e;_T2E=_T2D->loc;_T2F=
Cyc_Absyn_exp_stmt(_T2C,_T2E);_T30=
Cyc_Absyn_copy_exp(new_arg);_T31=e;_T32=_T31->loc;_T33=Cyc_Absyn_exp_stmt(_T30,_T32);_T34=e;_T35=_T34->loc;_T36=
# 2179
Cyc_Absyn_seq_stmt(_T2F,_T33,_T35);_T37=e;_T38=_T37->loc;{
# 2178
struct Cyc_Absyn_Stmt*s=Cyc_Absyn_decl_stmt(_T29,_T36,_T38);_T39=e;{struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_T4B=_cycalloc(sizeof(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct));_T4B->tag=38;
# 2182
_T4B->f1=s;_T3A=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_T4B;}_T39->r=(void*)_T3A;_T3B=e;
_T3B->topt=t;goto _LL0;}}}}}}}}}_TL42C:{struct Cyc_Warn_String_Warn_Warg_struct _T4B;_T4B.tag=0;
# 2185
_T4B.f1=_tag_fat("unchecked_cast (",sizeof(char),17U);_T3C=_T4B;}{struct Cyc_Warn_String_Warn_Warg_struct _T4B=_T3C;{struct Cyc_Warn_Typ_Warn_Warg_struct _T4C;_T4C.tag=2;_T4C.f1=t;_T3D=_T4C;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T4C=_T3D;{struct Cyc_Warn_String_Warn_Warg_struct _T4D;_T4D.tag=0;_T4D.f1=_tag_fat(")",sizeof(char),2U);_T3E=_T4D;}{struct Cyc_Warn_String_Warn_Warg_struct _T4D=_T3E;{struct Cyc_Warn_Exp_Warn_Warg_struct _T4E;_T4E.tag=4;_T4E.f1=e;_T3F=_T4E;}{struct Cyc_Warn_Exp_Warn_Warg_struct _T4E=_T3F;{struct Cyc_Warn_String_Warn_Warg_struct _T4F;_T4F.tag=0;_T4F.f1=_tag_fat(" with Subset_coercion",sizeof(char),22U);_T40=_T4F;}{struct Cyc_Warn_String_Warn_Warg_struct _T4F=_T40;void*_T50[5];_T50[0]=& _T4B;_T50[1]=& _T4C;_T50[2]=& _T4D;_T50[3]=& _T4E;_T50[4]=& _T4F;_T42=Cyc_Warn_impos2;{int(*_T51)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T42;_T41=_T51;}_T43=_tag_fat(_T50,sizeof(void*),5);_T41(_T43);}}}}}_LL0:;}goto _TL42B;
# 2188
_TL42A: _T44=e;{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T47=_cycalloc(sizeof(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct));_T47->tag=14;_T47->f1=t;_T47->f2=inner;_T47->f3=0;_T47->f4=c;_T45=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T47;}_T44->r=(void*)_T45;_T46=e;
_T46->topt=t;_TL42B:;}}
# 2194
static int Cyc_Tcutil_tvar_id_counter=0;
int Cyc_Tcutil_new_tvar_id (void){int _T0;int _T1;_T0=Cyc_Tcutil_tvar_id_counter;
Cyc_Tcutil_tvar_id_counter=_T0 + 1;_T1=_T0;return _T1;}
# 2199
static int Cyc_Tcutil_tvar_counter=0;
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*k){struct Cyc_Absyn_Tvar*_T0;struct _fat_ptr*_T1;struct _fat_ptr _T2;struct Cyc_Int_pa_PrintArg_struct _T3;int _T4;int _T5;struct _fat_ptr _T6;struct _fat_ptr _T7;{struct Cyc_Absyn_Tvar*_T8=_cycalloc(sizeof(struct Cyc_Absyn_Tvar));{struct _fat_ptr*_T9=_cycalloc(sizeof(struct _fat_ptr));{struct Cyc_Int_pa_PrintArg_struct _TA;_TA.tag=1;_T4=Cyc_Tcutil_tvar_counter;
Cyc_Tcutil_tvar_counter=_T4 + 1;_T5=_T4;_TA.f1=(unsigned long)_T5;_T3=_TA;}{struct Cyc_Int_pa_PrintArg_struct _TA=_T3;void*_TB[1];_TB[0]=& _TA;_T6=_tag_fat("#%d",sizeof(char),4U);_T7=_tag_fat(_TB,sizeof(void*),1);_T2=Cyc_aprintf(_T6,_T7);}*_T9=_T2;_T1=(struct _fat_ptr*)_T9;}_T8->name=_T1;_T8->identity=- 1;_T8->kind=k;_T8->aquals_bound=0;_T0=(struct Cyc_Absyn_Tvar*)_T8;}return _T0;}
# 2203
int Cyc_Tcutil_is_temp_tvar(struct Cyc_Absyn_Tvar*t){struct Cyc_Absyn_Tvar*_T0;struct _fat_ptr*_T1;struct _fat_ptr _T2;unsigned char*_T3;const char*_T4;char _T5;int _T6;int _T7;_T0=t;_T1=_T0->name;_T2=_T1[0];_T3=_check_fat_subscript(_T2,sizeof(char),0U);_T4=(const char*)_T3;_T5=*_T4;_T6=(int)_T5;_T7=_T6==35;
return _T7;}
# 2208
void*Cyc_Tcutil_fndecl2type(struct Cyc_Absyn_Fndecl*fd){struct Cyc_Absyn_Fndecl*_T0;void*_T1;struct Cyc_Absyn_Fndecl*_T2;struct Cyc_Absyn_FnInfo _T3;struct Cyc_List_List*_T4;void*_T5;int _T6;struct Cyc_List_List*_T7;struct Cyc_List_List*_T8;struct Cyc_List_List*_T9;struct Cyc_Absyn_Fndecl*_TA;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TB;void*_TC;struct Cyc_Absyn_Fndecl*_TD;void*_TE;_T0=fd;_T1=_T0->cached_type;
if(_T1!=0)goto _TL42E;{
# 2215
struct Cyc_List_List*fn_type_atts=0;_T2=fd;_T3=_T2->i;{
struct Cyc_List_List*atts=_T3.attributes;_TL433: if(atts!=0)goto _TL431;else{goto _TL432;}
_TL431: _T4=atts;_T5=_T4->hd;_T6=Cyc_Atts_fntype_att(_T5);if(!_T6)goto _TL434;{struct Cyc_List_List*_TF=_cycalloc(sizeof(struct Cyc_List_List));_T8=atts;
_TF->hd=_T8->hd;_TF->tl=fn_type_atts;_T7=(struct Cyc_List_List*)_TF;}fn_type_atts=_T7;goto _TL435;_TL434: _TL435: _T9=atts;
# 2216
atts=_T9->tl;goto _TL433;_TL432:;}_TA=fd;{
# 2219
struct Cyc_Absyn_FnInfo type_info=_TA->i;
type_info.attributes=fn_type_atts;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_TF=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_TF->tag=6;
_TF->f1=type_info;_TB=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_TF;}_TC=(void*)_TB;return _TC;}}_TL42E: _TD=fd;_TE=_TD->cached_type;
# 2223
return _TE;}
# 2227
static struct _tuple14*Cyc_Tcutil_map2_tq(struct _tuple14*pr,void*t){struct _tuple14*_T0;struct _tuple14*_T1;struct _tuple14*_T2;void*_T3;struct Cyc_Absyn_Tqual _T4;_T0=pr;{struct _tuple14 _T5=*_T0;_T4=_T5.f0;_T3=_T5.f1;}{struct Cyc_Absyn_Tqual tq=_T4;void*t2=_T3;
# 2229
if(t2!=t)goto _TL436;_T1=pr;goto _TL437;_TL436:{struct _tuple14*_T5=_cycalloc(sizeof(struct _tuple14));_T5->f0=tq;_T5->f1=t;_T2=(struct _tuple14*)_T5;}_T1=_T2;_TL437: return _T1;}}struct _tuple23{struct _fat_ptr*f0;struct Cyc_Absyn_Tqual f1;};struct _tuple24{struct _tuple23*f0;void*f1;};
# 2231
static struct _tuple24*Cyc_Tcutil_substitute_f1(struct _RegionHandle*rgn,struct _tuple9*y){struct _tuple24*_T0;struct _RegionHandle*_T1;struct _tuple23*_T2;struct _RegionHandle*_T3;struct _tuple9*_T4;struct _tuple9 _T5;struct _tuple9*_T6;struct _tuple9 _T7;struct _tuple9*_T8;struct _tuple9 _T9;_T1=rgn;{struct _tuple24*_TA=_region_malloc(_T1,0U,sizeof(struct _tuple24));_T3=rgn;{struct _tuple23*_TB=_region_malloc(_T3,0U,sizeof(struct _tuple23));_T4=y;_T5=*_T4;
# 2233
_TB->f0=_T5.f0;_T6=y;_T7=*_T6;_TB->f1=_T7.f1;_T2=(struct _tuple23*)_TB;}_TA->f0=_T2;_T8=y;_T9=*_T8;_TA->f1=_T9.f2;_T0=(struct _tuple24*)_TA;}return _T0;}
# 2235
static struct _tuple9*Cyc_Tcutil_substitute_f2(struct _tuple9*orig_arg,void*t){struct _tuple9*_T0;struct _tuple9*_T1;struct _tuple9*_T2;_T0=orig_arg;{
# 2237
struct _tuple9 _T3=*_T0;void*_T4;struct Cyc_Absyn_Tqual _T5;struct _fat_ptr*_T6;_T6=_T3.f0;_T5=_T3.f1;_T4=_T3.f2;{struct _fat_ptr*vopt_orig=_T6;struct Cyc_Absyn_Tqual tq_orig=_T5;void*t_orig=_T4;
if(t!=t_orig)goto _TL438;_T1=orig_arg;return _T1;_TL438:{struct _tuple9*_T7=_cycalloc(sizeof(struct _tuple9));
_T7->f0=vopt_orig;_T7->f1=tq_orig;_T7->f2=t;_T2=(struct _tuple9*)_T7;}return _T2;}}}
# 2246
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*,struct Cyc_List_List*,struct _RegionHandle*,struct Cyc_List_List*);
# 2248
static struct Cyc_List_List*Cyc_Tcutil_substs_exps(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_List_List*);
# 2254
static struct Cyc_Absyn_Exp*Cyc_Tcutil_copye(struct Cyc_Absyn_Exp*old,void*r){struct Cyc_Absyn_Exp*_T0;struct Cyc_Absyn_Exp*_T1;struct Cyc_Absyn_Exp*_T2;struct Cyc_Absyn_Exp*_T3;{struct Cyc_Absyn_Exp*_T4=_cycalloc(sizeof(struct Cyc_Absyn_Exp));_T1=old;
# 2256
_T4->topt=_T1->topt;_T4->r=r;_T2=old;_T4->loc=_T2->loc;_T3=old;_T4->annot=_T3->annot;_T0=(struct Cyc_Absyn_Exp*)_T4;}return _T0;}
# 2261
struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubsexp(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Exp*_T3;int _T4;struct Cyc_List_List*_T5;void*_T6;struct Cyc_Absyn_Exp*_T7;struct Cyc_Absyn_Exp*_T8;struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_T9;struct Cyc_List_List*_TA;struct _fat_ptr _TB;void*_TC;int _TD;struct Cyc_List_List*_TE;void*_TF;struct Cyc_List_List*_T10;struct Cyc_List_List*_T11;struct Cyc_List_List*_T12;void*_T13;struct Cyc_Absyn_Exp*_T14;struct Cyc_Absyn_Exp*_T15;struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_T16;struct Cyc_List_List*_T17;struct _fat_ptr _T18;void*_T19;struct Cyc_Absyn_Exp*_T1A;struct Cyc_Absyn_Exp*_T1B;struct Cyc_Warn_String_Warn_Warg_struct _T1C;struct Cyc_Absyn_Exp*(*_T1D)(struct _fat_ptr);void*(*_T1E)(struct _fat_ptr);struct _fat_ptr _T1F;struct Cyc_Absyn_Exp*_T20;struct Cyc_Absyn_Exp*_T21;struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_T22;void*_T23;struct Cyc_Absyn_Exp*_T24;struct Cyc_Absyn_Exp*_T25;struct Cyc_Absyn_Exp*_T26;struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_T27;void*_T28;struct Cyc_Absyn_Exp*_T29;struct Cyc_Absyn_Exp*_T2A;struct Cyc_Absyn_Exp*_T2B;struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_T2C;void*_T2D;struct Cyc_Absyn_Exp*_T2E;struct Cyc_Absyn_Exp*_T2F;struct Cyc_Absyn_Exp*_T30;struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_T31;void*_T32;struct Cyc_Absyn_Exp*_T33;struct Cyc_Absyn_Exp*_T34;struct Cyc_Absyn_Exp*_T35;struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_T36;void*_T37;struct Cyc_Absyn_Exp*_T38;struct Cyc_Absyn_Exp*_T39;struct Cyc_Absyn_Exp*_T3A;struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_T3B;void*_T3C;struct Cyc_Absyn_Exp*_T3D;struct Cyc_Absyn_Exp*_T3E;struct Cyc_Absyn_Exp*_T3F;struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_T40;void*_T41;struct Cyc_Absyn_Exp*_T42;void*_T43;struct Cyc_Absyn_Exp*_T44;struct Cyc_Absyn_Exp*_T45;struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T46;void*_T47;struct Cyc_Absyn_Exp*_T48;struct Cyc_Absyn_Exp*_T49;struct Cyc_Absyn_Exp*_T4A;struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_T4B;void*_T4C;struct Cyc_Absyn_Exp*_T4D;struct Cyc_Absyn_Exp*_T4E;struct Cyc_Absyn_Exp*_T4F;struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T50;void*_T51;struct Cyc_Absyn_Exp*_T52;struct Cyc_Absyn_Exp*_T53;struct Cyc_Absyn_Exp*_T54;struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T55;void*_T56;struct Cyc_Absyn_Exp*_T57;struct Cyc_Absyn_Exp*_T58;struct Cyc_Absyn_Exp*_T59;struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T5A;void*_T5B;struct Cyc_Absyn_Exp*_T5C;struct Cyc_Absyn_Exp*_T5D;struct Cyc_Absyn_Exp*_T5E;struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T5F;void*_T60;struct Cyc_Absyn_Exp*_T61;struct Cyc_Absyn_Exp*_T62;struct Cyc_Absyn_Exp*_T63;struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_T64;void*_T65;struct Cyc_Absyn_Exp*_T66;void*_T67;struct Cyc_Absyn_Exp*_T68;struct Cyc_Absyn_Exp*_T69;struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_T6A;void*_T6B;struct Cyc_Absyn_Exp*_T6C;struct Cyc_Absyn_Exp*_T6D;struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_T6E;void*_T6F;void*_T70;struct Cyc_Absyn_Exp*_T71;struct Cyc_Absyn_Exp*_T72;struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*_T73;void*_T74;struct Cyc_Absyn_Exp*_T75;struct Cyc_Absyn_Exp*_T76;struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*_T77;void*_T78;struct Cyc_Absyn_Exp*_T79;struct Cyc_Absyn_Exp*_T7A;struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_T7B;void*_T7C;void*_T7D;struct Cyc_Absyn_Exp*_T7E;struct Cyc_Absyn_Exp*_T7F;struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_T80;void*_T81;void*_T82;struct Cyc_Absyn_Exp*_T83;int*_T84;int _T85;struct Cyc_Absyn_Exp*_T86;struct Cyc_Absyn_Exp*_T87;struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_T88;void*_T89;struct Cyc_Absyn_Exp*_T8A;struct Cyc_Absyn_Exp*_T8B;struct Cyc_Absyn_Exp*_T8C;struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*_T8D;void*_T8E;struct Cyc_Absyn_Exp*_T8F;struct Cyc_Absyn_Exp*_T90;struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*_T91;void*_T92;struct Cyc_Absyn_Exp*_T93;struct Cyc_Warn_String_Warn_Warg_struct _T94;struct Cyc_Absyn_Exp*(*_T95)(struct _fat_ptr);void*(*_T96)(struct _fat_ptr);struct _fat_ptr _T97;_T0=e;{
void*_T98=_T0->r;int _T99;struct _fat_ptr*_T9A;enum Cyc_Absyn_Coercion _T9B;int _T9C;struct Cyc_Absyn_VarargCallInfo*_T9D;struct Cyc_List_List*_T9E;struct Cyc_Absyn_Exp*_T9F;struct Cyc_Absyn_Exp*_TA0;void*_TA1;enum Cyc_Absyn_Primop _TA2;_T1=(int*)_T98;_T2=*_T1;switch(_T2){case 0: goto _LL4;case 33: _LL4: goto _LL6;case 34: _LL6: goto _LL8;case 2: _LL8: goto _LLA;case 1: _LLA: _T3=e;
# 2267
return _T3;case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_T98;_TA2=_TA3->f1;_TA1=_TA3->f2;}{enum Cyc_Absyn_Primop p=_TA2;struct Cyc_List_List*es=_TA1;_T4=
# 2270
Cyc_List_length(es);if(_T4!=1)goto _TL43B;_T5=
_check_null(es);_T6=_T5->hd;{struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)_T6;
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL43D;_T7=e;goto _TL43E;_TL43D: _T8=e;{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct));_TA3->tag=3;_TA3->f1=p;{struct Cyc_Absyn_Exp*_TA4[1];_TA4[0]=new_e1;_TB=_tag_fat(_TA4,sizeof(struct Cyc_Absyn_Exp*),1);_TA=Cyc_List_list(_TB);}_TA3->f2=_TA;_T9=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_TA3;}_TC=(void*)_T9;_T7=Cyc_Tcutil_copye(_T8,_TC);_TL43E: return _T7;}
_TL43B: _TD=Cyc_List_length(es);if(_TD!=2)goto _TL43F;_TE=
_check_null(es);_TF=_TE->hd;{struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)_TF;_T10=es;_T11=_T10->tl;_T12=
_check_null(_T11);_T13=_T12->hd;{struct Cyc_Absyn_Exp*e2=(struct Cyc_Absyn_Exp*)_T13;
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1!=e1)goto _TL441;if(new_e2!=e2)goto _TL441;_T14=e;return _T14;_TL441: _T15=e;{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct));_TA3->tag=3;
_TA3->f1=p;{struct Cyc_Absyn_Exp*_TA4[2];_TA4[0]=new_e1;_TA4[1]=new_e2;_T18=_tag_fat(_TA4,sizeof(struct Cyc_Absyn_Exp*),2);_T17=Cyc_List_list(_T18);}_TA3->f2=_T17;_T16=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_TA3;}_T19=(void*)_T16;_T1A=Cyc_Tcutil_copye(_T15,_T19);return _T1A;}}_TL43F:{struct Cyc_Warn_String_Warn_Warg_struct _TA3;_TA3.tag=0;
# 2282
_TA3.f1=_tag_fat("primop does not have 1 or 2 args!",sizeof(char),34U);_T1C=_TA3;}{struct Cyc_Warn_String_Warn_Warg_struct _TA3=_T1C;void*_TA4[1];_TA4[0]=& _TA3;_T1E=Cyc_Warn_impos2;{struct Cyc_Absyn_Exp*(*_TA5)(struct _fat_ptr)=(struct Cyc_Absyn_Exp*(*)(struct _fat_ptr))_T1E;_T1D=_TA5;}_T1F=_tag_fat(_TA4,sizeof(void*),1);_T1B=_T1D(_T1F);}return _T1B;}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_TA0=_TA3->f2;_T9F=_TA3->f3;}{struct Cyc_Absyn_Exp*e1=_TA1;struct Cyc_Absyn_Exp*e2=_TA0;struct Cyc_Absyn_Exp*e3=_T9F;
# 2284
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
struct Cyc_Absyn_Exp*new_e3=Cyc_Tcutil_rsubsexp(r,inst,e3);
if(new_e1!=e1)goto _TL443;if(new_e2!=e2)goto _TL443;if(new_e3!=e3)goto _TL443;_T20=e;return _T20;_TL443: _T21=e;{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct));_TA3->tag=6;
_TA3->f1=new_e1;_TA3->f2=new_e2;_TA3->f3=new_e3;_T22=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_TA3;}_T23=(void*)_T22;_T24=Cyc_Tcutil_copye(_T21,_T23);return _T24;}case 7:{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_TA0=_TA3->f2;}{struct Cyc_Absyn_Exp*e1=_TA1;struct Cyc_Absyn_Exp*e2=_TA0;
# 2290
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1!=e1)goto _TL445;if(new_e2!=e2)goto _TL445;_T25=e;return _T25;_TL445: _T26=e;{struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct));_TA3->tag=7;
_TA3->f1=new_e1;_TA3->f2=new_e2;_T27=(struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_TA3;}_T28=(void*)_T27;_T29=Cyc_Tcutil_copye(_T26,_T28);return _T29;}case 8:{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_TA0=_TA3->f2;}{struct Cyc_Absyn_Exp*e1=_TA1;struct Cyc_Absyn_Exp*e2=_TA0;
# 2295
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1!=e1)goto _TL447;if(new_e2!=e2)goto _TL447;_T2A=e;return _T2A;_TL447: _T2B=e;{struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct));_TA3->tag=8;
_TA3->f1=new_e1;_TA3->f2=new_e2;_T2C=(struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_TA3;}_T2D=(void*)_T2C;_T2E=Cyc_Tcutil_copye(_T2B,_T2D);return _T2E;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_TA0=_TA3->f2;}{struct Cyc_Absyn_Exp*e1=_TA1;struct Cyc_Absyn_Exp*e2=_TA0;
# 2300
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1!=e1)goto _TL449;if(new_e2!=e2)goto _TL449;_T2F=e;return _T2F;_TL449: _T30=e;{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct));_TA3->tag=9;
_TA3->f1=new_e1;_TA3->f2=new_e2;_T31=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_TA3;}_T32=(void*)_T31;_T33=Cyc_Tcutil_copye(_T30,_T32);return _T33;}case 10:{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_T9E=_TA3->f2;_T9D=_TA3->f3;_T9C=_TA3->f4;}{struct Cyc_Absyn_Exp*e1=_TA1;struct Cyc_List_List*args=_T9E;struct Cyc_Absyn_VarargCallInfo*vai=_T9D;int resolved=_T9C;
# 2305
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_List_List*new_args=Cyc_Tcutil_substs_exps(r,inst,args);
# 2308
if(new_e1!=e1)goto _TL44B;if(new_args!=args)goto _TL44B;_T34=e;return _T34;_TL44B: _T35=e;{struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct));_TA3->tag=10;
_TA3->f1=new_e1;_TA3->f2=new_args;_TA3->f3=vai;_TA3->f4=resolved;_T36=(struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_TA3;}_T37=(void*)_T36;_T38=Cyc_Tcutil_copye(_T35,_T37);return _T38;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;}{struct Cyc_Absyn_Exp*e1=_TA1;
# 2311
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL44D;_T39=e;return _T39;_TL44D: _T3A=e;{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct));_TA3->tag=12;
_TA3->f1=new_e1;_T3B=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_TA3;}_T3C=(void*)_T3B;_T3D=Cyc_Tcutil_copye(_T3A,_T3C);return _T3D;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_T9E=_TA3->f2;}{struct Cyc_Absyn_Exp*e1=_TA1;struct Cyc_List_List*ts=_T9E;
# 2315
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_List_List*new_ts=Cyc_Tcutil_substs(r,inst,Cyc_Core_heap_region,ts);
if(new_e1!=e1)goto _TL44F;if(new_ts!=ts)goto _TL44F;_T3E=e;return _T3E;_TL44F: _T3F=e;{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct));_TA3->tag=13;
_TA3->f1=e1;_TA3->f2=ts;_T40=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_TA3;}_T41=(void*)_T40;_T42=Cyc_Tcutil_copye(_T3F,_T41);return _T42;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T98;_T43=_TA3->f1;_TA1=(void*)_T43;_TA0=_TA3->f2;_T9C=_TA3->f3;_T9B=_TA3->f4;}{void*t=_TA1;struct Cyc_Absyn_Exp*e1=_TA0;int b=_T9C;enum Cyc_Absyn_Coercion c=_T9B;
# 2320
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_e1!=e1)goto _TL451;if(new_typ!=t)goto _TL451;_T44=e;return _T44;_TL451: _T45=e;{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct));_TA3->tag=14;
_TA3->f1=new_typ;_TA3->f2=new_e1;_TA3->f3=b;_TA3->f4=c;_T46=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_TA3;}_T47=(void*)_T46;_T48=Cyc_Tcutil_copye(_T45,_T47);return _T48;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;}{struct Cyc_Absyn_Exp*e1=_TA1;
# 2325
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL453;_T49=e;return _T49;_TL453: _T4A=e;{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct));_TA3->tag=15;
_TA3->f1=new_e1;_T4B=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_TA3;}_T4C=(void*)_T4B;_T4D=Cyc_Tcutil_copye(_T4A,_T4C);return _T4D;}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;}{struct Cyc_Absyn_Exp*e1=_TA1;
# 2329
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL455;_T4E=e;return _T4E;_TL455: _T4F=e;{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct));_TA3->tag=22;
_TA3->f1=new_e1;_T50=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_TA3;}_T51=(void*)_T50;_T52=Cyc_Tcutil_copye(_T4F,_T51);return _T52;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_T9A=_TA3->f2;_T9C=_TA3->f3;_T99=_TA3->f4;}{struct Cyc_Absyn_Exp*e1=_TA1;struct _fat_ptr*f=_T9A;int it=_T9C;int ir=_T99;
# 2333
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL457;_T53=e;return _T53;_TL457: _T54=e;{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct));_TA3->tag=23;
_TA3->f1=new_e1;_TA3->f2=f;_TA3->f3=it;_TA3->f4=ir;_T55=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_TA3;}_T56=(void*)_T55;_T57=Cyc_Tcutil_copye(_T54,_T56);return _T57;}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_T9A=_TA3->f2;_T9C=_TA3->f3;_T99=_TA3->f4;}{struct Cyc_Absyn_Exp*e1=_TA1;struct _fat_ptr*f=_T9A;int it=_T9C;int ir=_T99;
# 2337
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL459;_T58=e;return _T58;_TL459: _T59=e;{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct));_TA3->tag=24;
_TA3->f1=new_e1;_TA3->f2=f;_TA3->f3=it;_TA3->f4=ir;_T5A=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_TA3;}_T5B=(void*)_T5A;_T5C=Cyc_Tcutil_copye(_T59,_T5B);return _T5C;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_TA0=_TA3->f2;}{struct Cyc_Absyn_Exp*e1=_TA1;struct Cyc_Absyn_Exp*e2=_TA0;
# 2341
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
struct Cyc_Absyn_Exp*new_e2=Cyc_Tcutil_rsubsexp(r,inst,e2);
if(new_e1!=e1)goto _TL45B;if(new_e2!=e2)goto _TL45B;_T5D=e;return _T5D;_TL45B: _T5E=e;{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct));_TA3->tag=25;
_TA3->f1=new_e1;_TA3->f2=new_e2;_T5F=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_TA3;}_T60=(void*)_T5F;_T61=Cyc_Tcutil_copye(_T5E,_T60);return _T61;}case 39:{struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;_T9A=_TA3->f2;}{struct Cyc_Absyn_Exp*e1=_TA1;struct _fat_ptr*f=_T9A;
# 2346
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL45D;_T62=e;return _T62;_TL45D: _T63=e;{struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct));_TA3->tag=39;
_TA3->f1=e1;_TA3->f2=f;_T64=(struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_TA3;}_T65=(void*)_T64;_T66=Cyc_Tcutil_copye(_T63,_T65);return _T66;}case 17:{struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_T98;_T67=_TA3->f1;_TA1=(void*)_T67;}{void*t=_TA1;
# 2350
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_typ!=t)goto _TL45F;_T68=e;goto _TL460;_TL45F: _T69=e;{struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct));_TA3->tag=17;_TA3->f1=new_typ;_T6A=(struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_TA3;}_T6B=(void*)_T6A;_T68=Cyc_Tcutil_copye(_T69,_T6B);_TL460: return _T68;}case 18:{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;}{struct Cyc_Absyn_Exp*e1=_TA1;
# 2353
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL461;_T6C=e;goto _TL462;_TL461: _T6D=e;{struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct));_TA3->tag=18;_TA3->f1=new_e1;_T6E=(struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_TA3;}_T6F=(void*)_T6E;_T6C=Cyc_Tcutil_copye(_T6D,_T6F);_TL462: return _T6C;}case 19:{struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*)_T98;_T70=_TA3->f1;_TA1=(void*)_T70;}{void*t=_TA1;
# 2356
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_typ!=t)goto _TL463;_T71=e;goto _TL464;_TL463: _T72=e;{struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct));_TA3->tag=19;_TA3->f1=new_typ;_T73=(struct Cyc_Absyn_Alignoftype_e_Absyn_Raw_exp_struct*)_TA3;}_T74=(void*)_T73;_T71=Cyc_Tcutil_copye(_T72,_T74);_TL464: return _T71;}case 20:{struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;}{struct Cyc_Absyn_Exp*e1=_TA1;
# 2359
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL465;_T75=e;goto _TL466;_TL465: _T76=e;{struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct));_TA3->tag=20;_TA3->f1=new_e1;_T77=(struct Cyc_Absyn_Alignofexp_e_Absyn_Raw_exp_struct*)_TA3;}_T78=(void*)_T77;_T75=Cyc_Tcutil_copye(_T76,_T78);_TL466: return _T75;}case 42:{struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_T98;_TA1=_TA3->f1;}{struct Cyc_Absyn_Exp*e1=_TA1;
# 2362
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL467;_T79=e;goto _TL468;_TL467: _T7A=e;{struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct));_TA3->tag=42;_TA3->f1=new_e1;_T7B=(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_TA3;}_T7C=(void*)_T7B;_T79=Cyc_Tcutil_copye(_T7A,_T7C);_TL468: return _T79;}case 21:{struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_T98;_T7D=_TA3->f1;_TA1=(void*)_T7D;_T9E=_TA3->f2;}{void*t=_TA1;struct Cyc_List_List*f=_T9E;
# 2365
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_typ!=t)goto _TL469;_T7E=e;goto _TL46A;_TL469: _T7F=e;{struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct));_TA3->tag=21;_TA3->f1=new_typ;_TA3->f2=f;_T80=(struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_TA3;}_T81=(void*)_T80;_T7E=Cyc_Tcutil_copye(_T7F,_T81);_TL46A: return _T7E;}case 40:{struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_T98;_T82=_TA3->f1;_TA1=(void*)_T82;}{void*t=_TA1;
# 2368
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
if(new_typ!=t)goto _TL46B;_T83=e;return _T83;_TL46B: {
# 2371
void*_TA3=Cyc_Absyn_compress(new_typ);struct Cyc_Absyn_Exp*_TA4;_T84=(int*)_TA3;_T85=*_T84;if(_T85!=9)goto _TL46D;{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TA5=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TA3;_TA4=_TA5->f1;}{struct Cyc_Absyn_Exp*e=_TA4;_T86=e;
return _T86;}_TL46D: _T87=e;{struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_TA5=_cycalloc(sizeof(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct));_TA5->tag=40;
_TA5->f1=new_typ;_T88=(struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_TA5;}_T89=(void*)_T88;_T8A=Cyc_Tcutil_copye(_T87,_T89);return _T8A;;}}case 43:{struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_T98;_TA0=_TA3->f1;_T9C=_TA3->f2;_T99=_TA3->f3;}{struct Cyc_Absyn_Exp*e1=_TA0;int s=_T9C;int dc=_T99;
# 2376
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL46F;_T8B=e;goto _TL470;_TL46F: _T8C=e;{struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct));_TA3->tag=43;_TA3->f1=new_e1;_TA3->f2=s;_TA3->f3=dc;_T8D=(struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_TA3;}_T8E=(void*)_T8D;_T8B=Cyc_Tcutil_copye(_T8C,_T8E);_TL470: return _T8B;}case 44:{struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*_TA3=(struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*)_T98;_TA0=_TA3->f1;}{struct Cyc_Absyn_Exp*e1=_TA0;
# 2379
struct Cyc_Absyn_Exp*new_e1=Cyc_Tcutil_rsubsexp(r,inst,e1);
if(new_e1!=e1)goto _TL471;_T8F=e;goto _TL472;_TL471: _T90=e;{struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*_TA3=_cycalloc(sizeof(struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct));_TA3->tag=44;_TA3->f1=new_e1;_T91=(struct Cyc_Absyn_Assert_false_e_Absyn_Raw_exp_struct*)_TA3;}_T92=(void*)_T91;_T8F=Cyc_Tcutil_copye(_T90,_T92);_TL472: return _T8F;}case 27: goto _LL3E;case 28: _LL3E: goto _LL40;case 30: _LL40: goto _LL42;case 31: _LL42: goto _LL44;case 32: _LL44: goto _LL46;case 29: _LL46: goto _LL48;case 26: _LL48: goto _LL4A;case 38: _LL4A: goto _LL4C;case 35: _LL4C: goto _LL4E;case 16: _LL4E: goto _LL50;case 36: _LL50: goto _LL52;case 4: _LL52: goto _LL54;case 5: _LL54: goto _LL56;case 37: _LL56: goto _LL58;case 41: _LL58: goto _LL5A;default: _LL5A:{struct Cyc_Warn_String_Warn_Warg_struct _TA3;_TA3.tag=0;
# 2397
_TA3.f1=_tag_fat("non-type-level-expression in Tcutil::rsubsexp",sizeof(char),46U);_T94=_TA3;}{struct Cyc_Warn_String_Warn_Warg_struct _TA3=_T94;void*_TA4[1];_TA4[0]=& _TA3;_T96=Cyc_Warn_impos2;{struct Cyc_Absyn_Exp*(*_TA5)(struct _fat_ptr)=(struct Cyc_Absyn_Exp*(*)(struct _fat_ptr))_T96;_T95=_TA5;}_T97=_tag_fat(_TA4,sizeof(void*),1);_T93=_T95(_T97);}return _T93;};}}
# 2401
static struct Cyc_List_List*Cyc_Tcutil_substs_exps(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*es){struct Cyc_List_List*_T0;void*_T1;struct Cyc_List_List*_T2;struct Cyc_List_List*_T3;struct Cyc_List_List*_T4;
# 2404
if(es!=0)goto _TL473;
return 0;_TL473: _T0=es;_T1=_T0->hd;{
struct Cyc_Absyn_Exp*old_hd=(struct Cyc_Absyn_Exp*)_T1;_T2=es;{
struct Cyc_List_List*old_tl=_T2->tl;
struct Cyc_Absyn_Exp*new_hd=Cyc_Tcutil_rsubsexp(rgn,inst,old_hd);
struct Cyc_List_List*new_tl=Cyc_Tcutil_substs_exps(rgn,inst,old_tl);
if(old_hd!=new_hd)goto _TL475;if(old_tl!=new_tl)goto _TL475;_T3=es;
return _T3;_TL475:{struct Cyc_List_List*_T5=_cycalloc(sizeof(struct Cyc_List_List));
_T5->hd=new_hd;_T5->tl=new_tl;_T4=(struct Cyc_List_List*)_T5;}return _T4;}}}
# 2416
static struct Cyc_Absyn_Exp*Cyc_Tcutil_rsubs_exp_opt(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;
# 2419
if(e!=0)goto _TL477;_T0=0;goto _TL478;_TL477: _T0=Cyc_Tcutil_rsubsexp(r,inst,e);_TL478: return _T0;}
# 2422
static struct Cyc_Absyn_Aggrfield*Cyc_Tcutil_subst_aggrfield(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_Absyn_Aggrfield*f){struct Cyc_Absyn_Aggrfield*_T0;struct Cyc_Absyn_Aggrfield*_T1;struct Cyc_Absyn_Aggrfield*_T2;struct Cyc_Absyn_Aggrfield*_T3;struct Cyc_Absyn_Aggrfield*_T4;struct Cyc_Absyn_Aggrfield*_T5;struct Cyc_Absyn_Aggrfield*_T6;struct Cyc_Absyn_Aggrfield*_T7;_T0=f;{
# 2425
void*t=_T0->type;_T1=f;{
struct Cyc_Absyn_Exp*req=_T1->requires_clause;
void*new_typ=Cyc_Tcutil_rsubstitute(r,inst,t);
struct Cyc_Absyn_Exp*new_req=Cyc_Tcutil_rsubs_exp_opt(r,inst,req);
if(t!=new_typ)goto _TL479;if(req!=new_req)goto _TL479;_T2=f;
return _T2;_TL479: {
struct Cyc_Absyn_Aggrfield*ans;ans=_cycalloc(sizeof(struct Cyc_Absyn_Aggrfield));_T3=ans;_T4=f;*_T3=*_T4;_T5=ans;
_T5->type=new_typ;_T6=ans;
_T6->requires_clause=new_req;_T7=ans;
return _T7;}}}}
# 2437
static struct Cyc_List_List*Cyc_Tcutil_subst_aggrfields(struct _RegionHandle*r,struct Cyc_List_List*inst,struct Cyc_List_List*fs){struct Cyc_List_List*_T0;void*_T1;struct Cyc_List_List*_T2;struct Cyc_List_List*_T3;struct Cyc_List_List*_T4;
# 2440
if(fs!=0)goto _TL47B;return 0;_TL47B: _T0=fs;_T1=_T0->hd;{
struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)_T1;_T2=fs;{
struct Cyc_List_List*t=_T2->tl;
struct Cyc_Absyn_Aggrfield*new_f=Cyc_Tcutil_subst_aggrfield(r,inst,f);
struct Cyc_List_List*new_typ=Cyc_Tcutil_subst_aggrfields(r,inst,t);
if(new_f!=f)goto _TL47D;if(new_typ!=t)goto _TL47D;_T3=fs;goto _TL47E;_TL47D:{struct Cyc_List_List*_T5=_cycalloc(sizeof(struct Cyc_List_List));_T5->hd=new_f;_T5->tl=new_typ;_T4=(struct Cyc_List_List*)_T5;}_T3=_T4;_TL47E: return _T3;}}}
# 2448
struct Cyc_List_List*Cyc_Tcutil_rsubst_type_assoc(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*rgn_po){struct Cyc_List_List*_T0;
# 2451
struct _tuple0 _T1=Cyc_List_rsplit(rgn,rgn,rgn_po);struct Cyc_List_List*_T2;struct Cyc_List_List*_T3;_T3=_T1.f0;_T2=_T1.f1;{struct Cyc_List_List*rpo1a=_T3;struct Cyc_List_List*rpo1b=_T2;
struct Cyc_List_List*rpo2a=Cyc_Tcutil_substs(rgn,inst,rgn,rpo1a);
struct Cyc_List_List*rpo2b=Cyc_Tcutil_substs(rgn,inst,rgn,rpo1b);
if(rpo2a!=rpo1a)goto _TL47F;if(rpo2b!=rpo1b)goto _TL47F;_T0=rgn_po;goto _TL480;_TL47F: _T0=Cyc_List_zip(rpo2a,rpo2b);_TL480: return _T0;}}
# 2457
struct Cyc_List_List*Cyc_Tcutil_rsubst_effconstr(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct Cyc_List_List*ec){struct Cyc_List_List*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_List_List*_T4;struct Cyc_List_List*_T5;struct Cyc_List_List*_T6;struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T7;void*_T8;void*_T9;struct Cyc_List_List*_TA;struct Cyc_List_List*_TB;struct Cyc_List_List*_TC;struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_TD;void*_TE;void*_TF;struct Cyc_List_List*_T10;struct Cyc_List_List*_T11;struct Cyc_List_List*_T12;struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T13;struct Cyc_List_List*_T14;struct Cyc_List_List*_T15;
# 2460
struct Cyc_List_List*slist=0;
_TL484: if(ec!=0)goto _TL482;else{goto _TL483;}
_TL482: _T0=ec;{void*_T16=_T0->hd;void*_T17;void*_T18;_T1=(int*)_T16;_T2=*_T1;switch(_T2){case 0:{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T19=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T16;_T3=_T19->f1;_T18=(void*)_T3;}{void*t=_T18;
# 2464
void*nt=Cyc_Tcutil_rsubstitute(rgn,inst,t);
if(nt!=t)goto _TL486;{struct Cyc_List_List*_T19=_cycalloc(sizeof(struct Cyc_List_List));_T5=ec;
_T19->hd=_T5->hd;_T19->tl=slist;_T4=(struct Cyc_List_List*)_T19;}slist=_T4;goto _TL487;
# 2468
_TL486:{struct Cyc_List_List*_T19=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*_T1A=_cycalloc(sizeof(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct));_T1A->tag=0;_T1A->f1=nt;_T7=(struct Cyc_Absyn_SingleConstraint_Absyn_EffConstraint_struct*)_T1A;}_T19->hd=(void*)_T7;_T19->tl=slist;_T6=(struct Cyc_List_List*)_T19;}slist=_T6;_TL487: goto _LL0;}case 1:{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T19=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T16;_T8=_T19->f1;_T18=(void*)_T8;_T9=_T19->f2;_T17=(void*)_T9;}{void*t1=_T18;void*t2=_T17;
# 2471
void*nt1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
void*nt2=Cyc_Tcutil_rsubstitute(rgn,inst,t2);
if(nt1!=t1)goto _TL488;if(nt2!=t2)goto _TL488;{struct Cyc_List_List*_T19=_cycalloc(sizeof(struct Cyc_List_List));_TB=ec;
_T19->hd=_TB->hd;_T19->tl=slist;_TA=(struct Cyc_List_List*)_T19;}slist=_TA;goto _TL489;
# 2476
_TL488:{struct Cyc_List_List*_T19=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*_T1A=_cycalloc(sizeof(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct));_T1A->tag=1;_T1A->f1=nt1;_T1A->f2=nt2;_TD=(struct Cyc_Absyn_DisjointConstraint_Absyn_EffConstraint_struct*)_T1A;}_T19->hd=(void*)_TD;_T19->tl=slist;_TC=(struct Cyc_List_List*)_T19;}slist=_TC;_TL489: goto _LL0;}default:{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T19=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T16;_TE=_T19->f1;_T18=(void*)_TE;_TF=_T19->f2;_T17=(void*)_TF;}{void*t1=_T18;void*t2=_T17;
# 2479
void*nt1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
void*nt2=Cyc_Tcutil_rsubstitute(rgn,inst,t2);
if(nt1!=t1)goto _TL48A;if(nt2!=t2)goto _TL48A;{struct Cyc_List_List*_T19=_cycalloc(sizeof(struct Cyc_List_List));_T11=ec;
_T19->hd=_T11->hd;_T19->tl=slist;_T10=(struct Cyc_List_List*)_T19;}slist=_T10;goto _TL48B;
# 2484
_TL48A:{struct Cyc_List_List*_T19=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*_T1A=_cycalloc(sizeof(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct));_T1A->tag=2;_T1A->f1=nt1;_T1A->f2=nt2;_T13=(struct Cyc_Absyn_SubsetConstraint_Absyn_EffConstraint_struct*)_T1A;}_T19->hd=(void*)_T13;_T19->tl=slist;_T12=(struct Cyc_List_List*)_T19;}slist=_T12;_TL48B: goto _LL0;}}_LL0:;}_T14=ec;
# 2461
ec=_T14->tl;goto _TL484;_TL483: _T15=slist;
# 2488
return _T15;}
# 2493
static int Cyc_Tcutil_skip_fntypes=0;
# 2495
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*rgn,struct Cyc_List_List*inst,void*t){int*_T0;unsigned _T1;struct _handler_cons*_T2;int _T3;void*(*_T4)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*);void*(*_T5)(int(*)(void*,void*),struct Cyc_List_List*,void*);int(*_T6)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);struct Cyc_List_List*_T7;struct Cyc_Absyn_Tvar*_T8;void*_T9;struct Cyc_Core_Not_found_exn_struct*_TA;char*_TB;char*_TC;void*_TD;void*_TE;void*_TF;struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T10;struct Cyc_Absyn_ArrayInfo _T11;struct Cyc_Absyn_ArrayInfo _T12;struct Cyc_Absyn_ArrayInfo _T13;struct Cyc_Absyn_ArrayInfo _T14;struct Cyc_Absyn_ArrayInfo _T15;void*_T16;struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T17;struct Cyc_Absyn_PtrInfo _T18;struct Cyc_Absyn_PtrInfo _T19;struct Cyc_Absyn_PtrInfo _T1A;struct Cyc_Absyn_PtrAtts _T1B;struct Cyc_Absyn_PtrInfo _T1C;struct Cyc_Absyn_PtrAtts _T1D;struct Cyc_Absyn_PtrInfo _T1E;struct Cyc_Absyn_PtrAtts _T1F;struct Cyc_Absyn_PtrInfo _T20;struct Cyc_Absyn_PtrAtts _T21;struct Cyc_Absyn_PtrInfo _T22;struct Cyc_Absyn_PtrAtts _T23;struct Cyc_Absyn_PtrInfo _T24;struct Cyc_Absyn_PtrAtts _T25;void*_T26;struct Cyc_Absyn_PtrInfo _T27;void*_T28;struct Cyc_Absyn_FnInfo _T29;struct Cyc_Absyn_FnInfo _T2A;struct Cyc_Absyn_FnInfo _T2B;struct Cyc_Absyn_FnInfo _T2C;struct Cyc_Absyn_FnInfo _T2D;struct Cyc_Absyn_FnInfo _T2E;struct Cyc_Absyn_FnInfo _T2F;struct Cyc_Absyn_FnInfo _T30;struct Cyc_Absyn_FnInfo _T31;struct Cyc_Absyn_FnInfo _T32;struct Cyc_Absyn_FnInfo _T33;struct Cyc_Absyn_FnInfo _T34;struct Cyc_Absyn_FnInfo _T35;struct Cyc_Absyn_FnInfo _T36;struct Cyc_Absyn_FnInfo _T37;struct Cyc_Absyn_FnInfo _T38;int _T39;void*_T3A;struct Cyc_List_List*_T3B;struct _RegionHandle*_T3C;struct _tuple18*_T3D;struct _RegionHandle*_T3E;struct Cyc_List_List*_T3F;void*_T40;struct Cyc_List_List*_T41;void*_T42;struct Cyc_Absyn_Tvar*_T43;struct Cyc_List_List*_T44;struct _RegionHandle*_T45;struct _RegionHandle*_T46;struct Cyc_List_List*(*_T47)(struct _RegionHandle*,struct _tuple24*(*)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*,struct Cyc_List_List*);struct Cyc_List_List*(*_T48)(struct _RegionHandle*,void*(*)(void*,void*),void*,struct Cyc_List_List*);struct _RegionHandle*_T49;struct _RegionHandle*_T4A;struct Cyc_List_List*_T4B;struct Cyc_List_List*_T4C;struct Cyc_List_List*(*_T4D)(struct _tuple9*(*)(struct _tuple9*,void*),struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_List_List*(*_T4E)(void*(*)(void*,void*),struct Cyc_List_List*,struct Cyc_List_List*);struct Cyc_List_List*_T4F;struct Cyc_List_List*_T50;void*_T51;struct Cyc_Absyn_VarargInfo*_T52;struct Cyc_Absyn_VarargInfo*_T53;struct Cyc_Absyn_VarargInfo*_T54;struct Cyc_List_List*_T55;void*_T56;struct Cyc_Absyn_Vardecl*_T57;struct Cyc_Absyn_Vardecl*_T58;struct Cyc_Absyn_Vardecl*_T59;struct _RegionHandle*_T5A;struct Cyc_List_List*_T5B;struct Cyc_Absyn_Vardecl*_T5C;void*_T5D;struct Cyc_List_List*_T5E;struct Cyc_List_List*_T5F;struct Cyc_Absyn_Vardecl*_T60;struct Cyc_Absyn_Vardecl*_T61;struct Cyc_Absyn_Vardecl*_T62;struct _RegionHandle*_T63;struct Cyc_List_List*_T64;struct Cyc_Absyn_Vardecl*_T65;void*_T66;struct Cyc_List_List*_T67;struct Cyc_List_List*_T68;struct Cyc_List_List*_T69;struct Cyc_List_List*_T6A;struct Cyc_List_List*_T6B;struct Cyc_List_List*_T6C;struct Cyc_AssnDef_AssnMap*_T6D;struct Cyc_AssnDef_AssnMap*_T6E;struct Cyc_AssnDef_AssnMap _T6F;struct Cyc_List_List*_T70;struct Cyc_AssnDef_AssnMap*_T71;struct Cyc_AssnDef_AssnMap*_T72;struct Cyc_AssnDef_AssnMap _T73;struct Cyc_List_List*_T74;struct Cyc_List_List*_T75;struct Cyc_List_List*_T76;struct Cyc_List_List*_T77;struct Cyc_AssnDef_AssnMap*_T78;struct Cyc_AssnDef_AssnMap*_T79;struct Cyc_AssnDef_AssnMap _T7A;struct Cyc_List_List*_T7B;struct Cyc_AssnDef_AssnMap*_T7C;struct Cyc_AssnDef_AssnMap*_T7D;struct Cyc_AssnDef_AssnMap _T7E;struct Cyc_List_List*_T7F;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T80;void*_T81;void*_T82;struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T83;void*_T84;void*_T85;void*_T86;void*_T87;void*_T88;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T89;void*_T8A;struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_T8B;void*_T8C;struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_T8D;int _T8E;void*_T8F;struct Cyc_Warn_String_Warn_Warg_struct _T90;int(*_T91)(struct _fat_ptr);void*(*_T92)(struct _fat_ptr);struct _fat_ptr _T93;struct Cyc_Absyn_Vardecl*_T94;struct Cyc_Absyn_Vardecl*_T95;struct Cyc_Absyn_Vardecl*_T96;struct _RegionHandle*_T97;struct Cyc_List_List*_T98;struct Cyc_Absyn_Vardecl*_T99;void*_T9A;struct _tuple16 _T9B;struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T9C;void*_T9D;struct Cyc_List_List _T9E;struct Cyc_List_List*_T9F;struct Cyc_List_List*_TA0;struct Cyc_Absyn_Exp*_TA1;struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TA2;void*_TA3;
# 2498
void*_TA4=Cyc_Absyn_compress(t);struct Cyc_AssnDef_ExistAssnFn*_TA5;enum Cyc_Absyn_AggrKind _TA6;struct Cyc_List_List*_TA7;struct Cyc_List_List*_TA8;struct Cyc_Absyn_Vardecl*_TA9;struct Cyc_Absyn_Exp*_TAA;struct Cyc_Absyn_Exp*_TAB;struct Cyc_Absyn_Exp*_TAC;struct Cyc_Absyn_Exp*_TAD;struct Cyc_List_List*_TAE;struct Cyc_List_List*_TAF;struct Cyc_Absyn_VarargInfo*_TB0;int _TB1;struct Cyc_List_List*_TB2;void*_TB3;void*_TB4;void*_TB5;unsigned _TB6;struct Cyc_Absyn_Tqual _TB7;void*_TB8;void*_TB9;void*_TBA;void*_TBB;_T0=(int*)_TA4;_T1=*_T0;switch(_T1){case 2:{struct Cyc_Absyn_VarType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_TA4;_TBB=_TBC->f1;}{struct Cyc_Absyn_Tvar*v=_TBB;struct _handler_cons _TBC;_T2=& _TBC;_push_handler(_T2);{int _TBD=0;_T3=setjmp(_TBC.handler);if(!_T3)goto _TL48D;_TBD=1;goto _TL48E;_TL48D: _TL48E: if(_TBD)goto _TL48F;else{goto _TL491;}_TL491: _T5=Cyc_List_assoc_cmp;{
# 2501
void*(*_TBE)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*)=(void*(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,struct Cyc_Absyn_Tvar*))_T5;_T4=_TBE;}_T6=Cyc_Absyn_tvar_cmp;_T7=inst;_T8=v;{void*_TBE=_T4(_T6,_T7,_T8);_npop_handler(0);return _TBE;}_pop_handler();goto _TL490;_TL48F: _T9=Cyc_Core_get_exn_thrown();{void*_TBE=(void*)_T9;void*_TBF;_TA=(struct Cyc_Core_Not_found_exn_struct*)_TBE;_TB=_TA->tag;_TC=Cyc_Core_Not_found;if(_TB!=_TC)goto _TL492;_TD=t;
return _TD;_TL492: _TBF=_TBE;{void*exn=_TBF;_rethrow(exn);};}_TL490:;}}case 8:{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_TA4;_TBB=_TBC->f1;_TBA=_TBC->f2;_TB9=_TBC->f3;_TE=_TBC->f4;_TB8=(void*)_TE;}{struct _tuple1*n=_TBB;struct Cyc_List_List*ts=_TBA;struct Cyc_Absyn_Typedefdecl*td=_TB9;void*topt=_TB8;
# 2504
struct Cyc_List_List*new_ts=Cyc_Tcutil_substs(rgn,inst,Cyc_Core_heap_region,ts);
if(new_ts!=ts)goto _TL494;_TF=t;goto _TL495;_TL494:{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_TBC=_cycalloc(sizeof(struct Cyc_Absyn_TypedefType_Absyn_Type_struct));_TBC->tag=8;_TBC->f1=n;_TBC->f2=new_ts;_TBC->f3=td;_TBC->f4=topt;_T10=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_TBC;}_TF=(void*)_T10;_TL495: return _TF;}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_TA4;_T11=_TBC->f1;_TBB=_T11.elt_type;_T12=_TBC->f1;_TB7=_T12.tq;_T13=_TBC->f1;_TBA=_T13.num_elts;_T14=_TBC->f1;_TB9=_T14.zero_term;_T15=_TBC->f1;_TB6=_T15.zt_loc;}{void*t1=_TBB;struct Cyc_Absyn_Tqual tq=_TB7;struct Cyc_Absyn_Exp*e=_TBA;void*zt=_TB9;unsigned ztl=_TB6;
# 2507
void*new_t1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
struct Cyc_Absyn_Exp*new_e=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,e);
void*new_zt=Cyc_Tcutil_rsubstitute(rgn,inst,zt);
if(new_t1!=t1)goto _TL496;if(new_e!=e)goto _TL496;if(new_zt!=zt)goto _TL496;_T16=t;goto _TL497;_TL496:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_TBC=_cycalloc(sizeof(struct Cyc_Absyn_ArrayType_Absyn_Type_struct));_TBC->tag=5;
_TBC->f1.elt_type=new_t1;_TBC->f1.tq=tq;_TBC->f1.num_elts=new_e;_TBC->f1.zero_term=new_zt;_TBC->f1.zt_loc=ztl;_T17=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_TBC;}_T16=(void*)_T17;_TL497:
# 2510
 return _T16;}case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_TA4;_T18=_TBC->f1;_TBB=_T18.elt_type;_T19=_TBC->f1;_TB7=_T19.elt_tq;_T1A=_TBC->f1;_T1B=_T1A.ptr_atts;_TBA=_T1B.eff;_T1C=_TBC->f1;_T1D=_T1C.ptr_atts;_TB9=_T1D.nullable;_T1E=_TBC->f1;_T1F=_T1E.ptr_atts;_TB8=_T1F.bounds;_T20=_TBC->f1;_T21=_T20.ptr_atts;_TB5=_T21.zero_term;_T22=_TBC->f1;_T23=_T22.ptr_atts;_TB4=_T23.autoreleased;_T24=_TBC->f1;_T25=_T24.ptr_atts;_TB3=_T25.aqual;}{void*t1=_TBB;struct Cyc_Absyn_Tqual tq=_TB7;void*r=_TBA;void*n=_TB9;void*b=_TB8;void*zt=_TB5;void*rel=_TB4;void*aq=_TB3;
# 2513
void*new_t1=Cyc_Tcutil_rsubstitute(rgn,inst,t1);
void*new_r=Cyc_Tcutil_rsubstitute(rgn,inst,r);
void*new_b=Cyc_Tcutil_rsubstitute(rgn,inst,b);
void*new_zt=Cyc_Tcutil_rsubstitute(rgn,inst,zt);
void*new_rel=Cyc_Tcutil_rsubstitute(rgn,inst,rel);
void*new_aq=Cyc_Tcutil_rsubstitute(rgn,inst,aq);
if(new_t1!=t1)goto _TL498;if(new_r!=r)goto _TL498;if(new_b!=b)goto _TL498;if(new_zt!=zt)goto _TL498;if(new_rel!=rel)goto _TL498;if(new_aq!=aq)goto _TL498;_T26=t;
return _T26;_TL498:{struct Cyc_Absyn_PtrInfo _TBC;
_TBC.elt_type=new_t1;_TBC.elt_tq=tq;_TBC.ptr_atts.eff=new_r;_TBC.ptr_atts.nullable=n;_TBC.ptr_atts.bounds=new_b;_TBC.ptr_atts.zero_term=new_zt;_TBC.ptr_atts.ptrloc=0;_TBC.ptr_atts.autoreleased=new_rel;_TBC.ptr_atts.aqual=new_aq;_T27=_TBC;}_T28=Cyc_Absyn_pointer_type(_T27);return _T28;}case 6:{struct Cyc_Absyn_FnType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_TA4;_T29=_TBC->f1;_TBB=_T29.tvars;_T2A=_TBC->f1;_TBA=_T2A.effect;_T2B=_TBC->f1;_TB7=_T2B.ret_tqual;_T2C=_TBC->f1;_TB9=_T2C.ret_type;_T2D=_TBC->f1;_TB2=_T2D.args;_T2E=_TBC->f1;_TB1=_T2E.c_varargs;_T2F=_TBC->f1;_TB0=_T2F.cyc_varargs;_T30=_TBC->f1;_TAF=_T30.qual_bnd;_T31=_TBC->f1;_TAE=_T31.attributes;_T32=_TBC->f1;_TAD=_T32.checks_clause;_T33=_TBC->f1;_TAC=_T33.requires_clause;_T34=_TBC->f1;_TAB=_T34.ensures_clause;_T35=_TBC->f1;_TAA=_T35.throws_clause;_T36=_TBC->f1;_TA9=_T36.return_value;_T37=_TBC->f1;_TA8=_T37.arg_vardecls;_T38=_TBC->f1;_TA7=_T38.effconstr;}{struct Cyc_List_List*vs=_TBB;void*eff=_TBA;struct Cyc_Absyn_Tqual rtq=_TB7;void*rtyp=_TB9;struct Cyc_List_List*args=_TB2;int c_varargs=_TB1;struct Cyc_Absyn_VarargInfo*cyc_varargs=_TB0;struct Cyc_List_List*qb=_TAF;struct Cyc_List_List*atts=_TAE;struct Cyc_Absyn_Exp*chk=_TAD;struct Cyc_Absyn_Exp*req=_TAC;struct Cyc_Absyn_Exp*ens=_TAB;struct Cyc_Absyn_Exp*thrws=_TAA;struct Cyc_Absyn_Vardecl*ret_value=_TA9;struct Cyc_List_List*argvds=_TA8;struct Cyc_List_List*effconstr=_TA7;_T39=Cyc_Tcutil_skip_fntypes;
# 2524
if(!_T39)goto _TL49A;_T3A=t;return _T3A;_TL49A:{
# 2526
struct Cyc_List_List*p=vs;_TL49F: if(p!=0)goto _TL49D;else{goto _TL49E;}
_TL49D: _T3C=rgn;{struct Cyc_List_List*_TBC=_region_malloc(_T3C,0U,sizeof(struct Cyc_List_List));_T3E=rgn;{struct _tuple18*_TBD=_region_malloc(_T3E,0U,sizeof(struct _tuple18));_T3F=p;_T40=_T3F->hd;_TBD->f0=(struct Cyc_Absyn_Tvar*)_T40;_T41=p;_T42=_T41->hd;_T43=(struct Cyc_Absyn_Tvar*)_T42;_TBD->f1=Cyc_Absyn_var_type(_T43);_T3D=(struct _tuple18*)_TBD;}_TBC->hd=_T3D;_TBC->tl=inst;_T3B=(struct Cyc_List_List*)_TBC;}inst=_T3B;_T44=p;
# 2526
p=_T44->tl;goto _TL49F;_TL49E:;}_T45=rgn;_T46=rgn;_T48=Cyc_List_rmap_c;{
# 2529
struct Cyc_List_List*(*_TBC)(struct _RegionHandle*,struct _tuple24*(*)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct _tuple24*(*)(struct _RegionHandle*,struct _tuple9*),struct _RegionHandle*,struct Cyc_List_List*))_T48;_T47=_TBC;}_T49=rgn;_T4A=rgn;_T4B=args;_T4C=_T47(_T49,Cyc_Tcutil_substitute_f1,_T4A,_T4B);{
# 2528
struct _tuple0 _TBC=Cyc_List_rsplit(_T45,_T46,_T4C);struct Cyc_List_List*_TBD;struct Cyc_List_List*_TBE;_TBE=_TBC.f0;_TBD=_TBC.f1;{struct Cyc_List_List*qs=_TBE;struct Cyc_List_List*ts=_TBD;
# 2530
struct Cyc_List_List*args2=args;
struct Cyc_List_List*ts2=Cyc_Tcutil_substs(rgn,inst,rgn,ts);
if(ts2==ts)goto _TL4A0;_T4E=Cyc_List_map2;{
struct Cyc_List_List*(*_TBF)(struct _tuple9*(*)(struct _tuple9*,void*),struct Cyc_List_List*,struct Cyc_List_List*)=(struct Cyc_List_List*(*)(struct _tuple9*(*)(struct _tuple9*,void*),struct Cyc_List_List*,struct Cyc_List_List*))_T4E;_T4D=_TBF;}_T4F=args;_T50=ts2;args2=_T4D(Cyc_Tcutil_substitute_f2,_T4F,_T50);goto _TL4A1;_TL4A0: _TL4A1:
 if(eff!=0)goto _TL4A2;_T51=0;goto _TL4A3;_TL4A2: _T51=Cyc_Tcutil_rsubstitute(rgn,inst,eff);_TL4A3: {void*eff2=_T51;
# 2536
struct Cyc_Absyn_VarargInfo*cyc_varargs2;
if(cyc_varargs!=0)goto _TL4A4;
cyc_varargs2=0;goto _TL4A5;
# 2540
_TL4A4: _T52=cyc_varargs;{struct Cyc_Absyn_VarargInfo _TBF=*_T52;int _TC0;void*_TC1;struct Cyc_Absyn_Tqual _TC2;struct _fat_ptr*_TC3;_TC3=_TBF.name;_TC2=_TBF.tq;_TC1=_TBF.type;_TC0=_TBF.inject;{struct _fat_ptr*n=_TC3;struct Cyc_Absyn_Tqual tq=_TC2;void*t=_TC1;int i=_TC0;
void*t2=Cyc_Tcutil_rsubstitute(rgn,inst,t);
if(t2!=t)goto _TL4A6;_T53=cyc_varargs;goto _TL4A7;_TL4A6:{struct Cyc_Absyn_VarargInfo*_TC4=_cycalloc(sizeof(struct Cyc_Absyn_VarargInfo));_TC4->name=n;_TC4->tq=tq;_TC4->type=t2;_TC4->inject=i;_T54=(struct Cyc_Absyn_VarargInfo*)_TC4;}_T53=_T54;_TL4A7: cyc_varargs2=_T53;}}_TL4A5: {
# 2544
struct Cyc_List_List*effconstr2=Cyc_Tcutil_rsubst_effconstr(rgn,inst,effconstr);
# 2546
struct Cyc_List_List*qb2=Cyc_Tcutil_rsubst_type_assoc(rgn,inst,qb);
struct Cyc_Absyn_Exp*chk2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,chk);
struct Cyc_Absyn_Exp*req2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,req);
struct Cyc_Absyn_Exp*ens2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,ens);
struct Cyc_Absyn_Exp*thrws2=Cyc_Tcutil_rsubs_exp_opt(rgn,inst,thrws);
# 2556
struct Cyc_List_List*argvds2=0;{
struct Cyc_List_List*avs=argvds;_TL4AB: if(avs!=0)goto _TL4A9;else{goto _TL4AA;}
_TL4A9: _T55=avs;_T56=_T55->hd;{struct Cyc_Absyn_Vardecl*av=(struct Cyc_Absyn_Vardecl*)_T56;
struct Cyc_Absyn_Vardecl*av2=0;
if(av==0)goto _TL4AC;{struct Cyc_Absyn_Vardecl*_TBF=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));_T58=av;
*_TBF=*_T58;_T57=(struct Cyc_Absyn_Vardecl*)_TBF;}av2=_T57;_T59=av2;_T5A=rgn;_T5B=inst;_T5C=av;_T5D=_T5C->type;
_T59->type=Cyc_Tcutil_rsubstitute(_T5A,_T5B,_T5D);goto _TL4AD;_TL4AC: _TL4AD:{struct Cyc_List_List*_TBF=_cycalloc(sizeof(struct Cyc_List_List));
# 2564
_TBF->hd=av2;_TBF->tl=argvds2;_T5E=(struct Cyc_List_List*)_TBF;}argvds2=_T5E;}_T5F=avs;
# 2557
avs=_T5F->tl;goto _TL4AB;_TL4AA:;}
# 2566
argvds2=Cyc_List_imp_rev(argvds2);{
struct Cyc_Absyn_Vardecl*ret_value2=0;
if(ret_value==0)goto _TL4AE;{struct Cyc_Absyn_Vardecl*_TBF=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));_T61=ret_value;
*_TBF=*_T61;_T60=(struct Cyc_Absyn_Vardecl*)_TBF;}ret_value2=_T60;_T62=ret_value2;_T63=rgn;_T64=inst;_T65=ret_value2;_T66=_T65->type;
_T62->type=Cyc_Tcutil_rsubstitute(_T63,_T64,_T66);goto _TL4AF;_TL4AE: _TL4AF: {
# 2573
struct Cyc_AssnDef_ExistAssnFn*chk_assn2=0;
struct Cyc_AssnDef_ExistAssnFn*req_assn2=0;
struct Cyc_AssnDef_ExistAssnFn*ens_assn2=0;
struct Cyc_AssnDef_ExistAssnFn*thrws_assn2=0;
# 2578
struct Cyc_List_List*vds1;vds1=_cycalloc(sizeof(struct Cyc_List_List));_T67=vds1;_T67->hd=ret_value;_T68=vds1;_T68->tl=argvds;{
struct Cyc_List_List*vds2;vds2=_cycalloc(sizeof(struct Cyc_List_List));_T69=vds2;_T69->hd=ret_value2;_T6A=vds2;_T6A->tl=argvds2;{
struct Cyc_List_List*boundvds;boundvds=_cycalloc(sizeof(struct Cyc_List_List));_T6B=boundvds;_T6B->hd=Cyc_AssnDef_memory;_T6C=boundvds;_T6C->tl=argvds2;
# 2582
if(chk2==0)goto _TL4B0;{
struct Cyc_AssnDef_AssnMap*chk_am2;chk_am2=_cycalloc(sizeof(struct Cyc_AssnDef_AssnMap));_T6D=chk_am2;*_T6D=Cyc_Vcgen_clause2assn(chk2);
chk_am2=Cyc_AssnDef_subst_vardecls(vds1,vds2,chk_am2);_T6E=
_check_null(chk_am2);_T6F=*_T6E;_T70=boundvds;chk_assn2=Cyc_AssnDef_assnmap2existassnfn(_T6F,_T70);}goto _TL4B1;_TL4B0: _TL4B1:
# 2587
 if(req2==0)goto _TL4B2;{
struct Cyc_AssnDef_AssnMap*req_am2;req_am2=_cycalloc(sizeof(struct Cyc_AssnDef_AssnMap));_T71=req_am2;*_T71=Cyc_Vcgen_clause2assn(req2);
req_am2=Cyc_AssnDef_subst_vardecls(vds1,vds2,req_am2);_T72=
_check_null(req_am2);_T73=*_T72;_T74=boundvds;req_assn2=Cyc_AssnDef_assnmap2existassnfn(_T73,_T74);}goto _TL4B3;_TL4B2: _TL4B3: {
# 2592
struct Cyc_List_List*boundvds;boundvds=_cycalloc(sizeof(struct Cyc_List_List));_T75=boundvds;_T75->hd=Cyc_AssnDef_memory;_T76=boundvds;{struct Cyc_List_List*_TBF=_cycalloc(sizeof(struct Cyc_List_List));
_TBF->hd=Cyc_AssnDef_pre_memory;_TBF->tl=argvds2;_T77=(struct Cyc_List_List*)_TBF;}_T76->tl=_T77;
if(ens2==0)goto _TL4B4;{
struct Cyc_AssnDef_AssnMap*ens_am2;ens_am2=_cycalloc(sizeof(struct Cyc_AssnDef_AssnMap));_T78=ens_am2;*_T78=Cyc_Vcgen_clause2assn(ens2);
ens_am2=Cyc_AssnDef_subst_vardecls(vds1,vds2,ens_am2);_T79=
_check_null(ens_am2);_T7A=*_T79;{struct Cyc_List_List*_TBF=_cycalloc(sizeof(struct Cyc_List_List));_TBF->hd=ret_value2;_TBF->tl=boundvds;_T7B=(struct Cyc_List_List*)_TBF;}ens_assn2=Cyc_AssnDef_assnmap2existassnfn(_T7A,_T7B);}goto _TL4B5;_TL4B4: _TL4B5:
# 2599
 if(thrws2==0)goto _TL4B6;{
struct Cyc_AssnDef_AssnMap*thrws_am2;thrws_am2=_cycalloc(sizeof(struct Cyc_AssnDef_AssnMap));_T7C=thrws_am2;*_T7C=Cyc_Vcgen_clause2assn(thrws2);
thrws_am2=Cyc_AssnDef_subst_vardecls(vds1,vds2,thrws_am2);_T7D=
_check_null(thrws_am2);_T7E=*_T7D;_T7F=boundvds;thrws_assn2=Cyc_AssnDef_assnmap2existassnfn(_T7E,_T7F);}goto _TL4B7;_TL4B6: _TL4B7:{struct Cyc_Absyn_FnType_Absyn_Type_struct*_TBF=_cycalloc(sizeof(struct Cyc_Absyn_FnType_Absyn_Type_struct));_TBF->tag=6;
# 2604
_TBF->f1.tvars=vs;_TBF->f1.effect=eff2;_TBF->f1.ret_tqual=rtq;_TBF->f1.ret_type=Cyc_Tcutil_rsubstitute(rgn,inst,rtyp);_TBF->f1.args=args2;
_TBF->f1.c_varargs=c_varargs;_TBF->f1.cyc_varargs=cyc_varargs2;_TBF->f1.qual_bnd=qb2;_TBF->f1.attributes=atts;
_TBF->f1.checks_clause=chk2;_TBF->f1.checks_assn=chk_assn2;
_TBF->f1.requires_clause=req2;_TBF->f1.requires_assn=req_assn2;_TBF->f1.ensures_clause=ens2;_TBF->f1.ensures_assn=ens_assn2;
_TBF->f1.throws_clause=thrws2;_TBF->f1.throws_assn=thrws_assn2;_TBF->f1.return_value=ret_value2;_TBF->f1.arg_vardecls=argvds2;_TBF->f1.effconstr=effconstr2;_T80=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_TBF;}_T81=(void*)_T80;
# 2604
return _T81;}}}}}}}}}}case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_TA4;_TA6=_TBC->f1;_TB1=_TBC->f2;_TBB=_TBC->f3;}{enum Cyc_Absyn_AggrKind k=_TA6;int b=_TB1;struct Cyc_List_List*fs=_TBB;
# 2610
struct Cyc_List_List*new_fs=Cyc_Tcutil_subst_aggrfields(rgn,inst,fs);
if(fs!=new_fs)goto _TL4B8;_T82=t;goto _TL4B9;_TL4B8:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_TBC=_cycalloc(sizeof(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct));_TBC->tag=7;_TBC->f1=k;_TBC->f2=b;_TBC->f3=new_fs;_T83=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_TBC;}_T82=(void*)_T83;_TL4B9: return _T82;}case 3:{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_TA4;_T84=_TBC->f2;_TBB=(void*)_T84;}{void*r=_TBB;_TBB=r;goto _LL10;}case 1:{struct Cyc_Absyn_Evar_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_TA4;_T85=_TBC->f2;_TBB=(void*)_T85;}_LL10: {void*r=_TBB;
# 2614
if(r!=0)goto _TL4BA;_T86=t;goto _TL4BB;_TL4BA: _T86=Cyc_Tcutil_rsubstitute(rgn,inst,r);_TL4BB: return _T86;}case 0:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA4;_T87=_TBC->f1;_TBB=(void*)_T87;_TB2=_TBC->f2;}{void*c=_TBB;struct Cyc_List_List*ts=_TB2;
# 2619
struct Cyc_List_List*new_ts=Cyc_Tcutil_substs(rgn,inst,Cyc_Core_heap_region,ts);
if(ts!=new_ts)goto _TL4BC;_T88=t;goto _TL4BD;_TL4BC:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TBC=_cycalloc(sizeof(struct Cyc_Absyn_AppType_Absyn_Type_struct));_TBC->tag=0;_TBC->f1=c;_TBC->f2=new_ts;_T89=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TBC;}_T88=(void*)_T89;_TL4BD: return _T88;}case 9:{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TA4;_TAD=_TBC->f1;}{struct Cyc_Absyn_Exp*e=_TAD;
# 2622
struct Cyc_Absyn_Exp*new_e=Cyc_Tcutil_rsubsexp(rgn,inst,e);
if(new_e!=e)goto _TL4BE;_T8A=t;goto _TL4BF;_TL4BE:{struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_TBC=_cycalloc(sizeof(struct Cyc_Absyn_ValueofType_Absyn_Type_struct));_TBC->tag=9;_TBC->f1=new_e;_T8B=(struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_TBC;}_T8A=(void*)_T8B;_TL4BF: return _T8A;}case 11:{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_TA4;_TAD=_TBC->f1;}{struct Cyc_Absyn_Exp*e=_TAD;
# 2625
struct Cyc_Absyn_Exp*new_e=Cyc_Tcutil_rsubsexp(rgn,inst,e);
if(new_e!=e)goto _TL4C0;_T8C=t;goto _TL4C1;_TL4C0:{struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_TBC=_cycalloc(sizeof(struct Cyc_Absyn_TypeofType_Absyn_Type_struct));_TBC->tag=11;_TBC->f1=new_e;_T8D=(struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_TBC;}_T8C=(void*)_T8D;_TL4C1: return _T8C;}case 10: _T8E=Cyc_Tcutil_skip_fntypes;
# 2628
if(!_T8E)goto _TL4C2;_T8F=t;return _T8F;_TL4C2:{struct Cyc_Warn_String_Warn_Warg_struct _TBC;_TBC.tag=0;
_TBC.f1=_tag_fat("found typedecltype in rsubs",sizeof(char),28U);_T90=_TBC;}{struct Cyc_Warn_String_Warn_Warg_struct _TBC=_T90;void*_TBD[1];_TBD[0]=& _TBC;_T92=Cyc_Warn_impos2;{int(*_TBE)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T92;_T91=_TBE;}_T93=_tag_fat(_TBD,sizeof(void*),1);_T91(_T93);}default:{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TBC=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_TA4;_TA9=_TBC->f1;_TAD=_TBC->f2;_TA5=_TBC->f3;}{struct Cyc_Absyn_Vardecl*vd=_TA9;struct Cyc_Absyn_Exp*e=_TAD;struct Cyc_AssnDef_ExistAssnFn*a=_TA5;
# 2631
struct Cyc_Absyn_Vardecl*vd2;vd2=_cycalloc(sizeof(struct Cyc_Absyn_Vardecl));_T94=vd2;_T95=vd;*_T94=*_T95;_T96=vd2;_T97=rgn;_T98=inst;_T99=vd;_T9A=_T99->type;
_T96->type=Cyc_Tcutil_rsubstitute(_T97,_T98,_T9A);{
struct Cyc_Absyn_Exp*e2=Cyc_Tcutil_rsubsexp(rgn,inst,e);{struct _tuple16 _TBC;
_TBC.f0=vd;{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_TBD=_cycalloc(sizeof(struct Cyc_Absyn_Local_b_Absyn_Binding_struct));_TBD->tag=4;_TBD->f1=vd2;_T9C=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_TBD;}_T9D=(void*)_T9C;_TBC.f1=Cyc_Absyn_varb_exp(_T9D,0U);_T9B=_TBC;}{struct _tuple16 p=_T9B;{struct Cyc_List_List _TBC;
_TBC.hd=& p;_TBC.tl=0;_T9E=_TBC;}{struct Cyc_List_List subst=_T9E;_T9F=& subst;_TA0=(struct Cyc_List_List*)_T9F;_TA1=e2;
Cyc_Tcutil_substitute_vars(_TA0,_TA1);{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TBC=_cycalloc(sizeof(struct Cyc_Absyn_SubsetType_Absyn_Type_struct));_TBC->tag=12;
_TBC->f1=vd;_TBC->f2=e2;_TBC->f3=a;_TA2=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_TBC;}_TA3=(void*)_TA2;return _TA3;}}}}};}
# 2641
static struct Cyc_List_List*Cyc_Tcutil_substs(struct _RegionHandle*rgn,struct Cyc_List_List*inst,struct _RegionHandle*rgn2,struct Cyc_List_List*ts){struct Cyc_List_List*_T0;struct Cyc_List_List*_T1;struct Cyc_List_List*_T2;struct Cyc_List_List*_T3;struct _RegionHandle*_T4;
# 2645
if(ts!=0)goto _TL4C4;
return 0;_TL4C4: _T0=ts;{
void*old_hd=_T0->hd;_T1=ts;{
struct Cyc_List_List*old_tl=_T1->tl;
void*new_hd=Cyc_Tcutil_rsubstitute(rgn,inst,old_hd);
struct Cyc_List_List*new_tl=Cyc_Tcutil_substs(rgn,inst,rgn2,old_tl);
if(old_hd!=new_hd)goto _TL4C6;if(old_tl!=new_tl)goto _TL4C6;_T2=ts;
return _T2;_TL4C6: _T4=rgn2;{struct Cyc_List_List*_T5=_region_malloc(_T4,0U,sizeof(struct Cyc_List_List));
_T5->hd=new_hd;_T5->tl=new_tl;_T3=(struct Cyc_List_List*)_T5;}return _T3;}}}
# 2656
void*Cyc_Tcutil_substitute(struct Cyc_List_List*inst,void*t){void*_T0;
if(inst!=0)goto _TL4C8;_T0=t;goto _TL4C9;_TL4C8: _T0=Cyc_Tcutil_rsubstitute(Cyc_Core_heap_region,inst,t);_TL4C9: return _T0;}
# 2660
void*Cyc_Tcutil_substitute_nofun(struct Cyc_List_List*inst,void*t){void*_T0;
Cyc_Tcutil_skip_fntypes=1;{
void*new_typ=Cyc_Tcutil_substitute(inst,t);
Cyc_Tcutil_skip_fntypes=0;_T0=new_typ;
return _T0;}}
# 2668
struct _tuple18*Cyc_Tcutil_make_inst_var(struct Cyc_List_List*s,struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;struct Cyc_Absyn_Kind*_T1;struct Cyc_Absyn_Kind*_T2;struct Cyc_Absyn_Kind*_T3;struct _tuple18*_T4;struct Cyc_Core_Opt*_T5;struct Cyc_Core_Opt*_T6;_T0=tv;_T1=& Cyc_Kinds_bk;_T2=(struct Cyc_Absyn_Kind*)_T1;_T3=
Cyc_Kinds_tvar_kind(_T0,_T2);{struct Cyc_Core_Opt*k=Cyc_Kinds_kind_to_opt(_T3);{struct _tuple18*_T7=_cycalloc(sizeof(struct _tuple18));
_T7->f0=tv;_T5=k;{struct Cyc_Core_Opt*_T8=_cycalloc(sizeof(struct Cyc_Core_Opt));_T8->v=s;_T6=(struct Cyc_Core_Opt*)_T8;}_T7->f1=Cyc_Absyn_new_evar(_T5,_T6);_T4=(struct _tuple18*)_T7;}return _T4;}}struct _tuple25{struct Cyc_List_List*f0;struct _RegionHandle*f1;};
# 2673
struct _tuple18*Cyc_Tcutil_r_make_inst_var(struct _tuple25*env,struct Cyc_Absyn_Tvar*tv){struct _tuple25*_T0;struct Cyc_Absyn_Tvar*_T1;struct Cyc_Absyn_Kind*_T2;struct Cyc_Absyn_Kind*_T3;struct Cyc_Absyn_Kind*_T4;struct _tuple18*_T5;struct _RegionHandle*_T6;struct Cyc_Core_Opt*_T7;struct Cyc_Core_Opt*_T8;struct _RegionHandle*_T9;struct Cyc_List_List*_TA;_T0=env;{struct _tuple25 _TB=*_T0;_TA=_TB.f0;_T9=_TB.f1;}{struct Cyc_List_List*s=_TA;struct _RegionHandle*rgn=_T9;_T1=tv;_T2=& Cyc_Kinds_bk;_T3=(struct Cyc_Absyn_Kind*)_T2;_T4=
# 2676
Cyc_Kinds_tvar_kind(_T1,_T3);{struct Cyc_Core_Opt*k=Cyc_Kinds_kind_to_opt(_T4);_T6=rgn;{struct _tuple18*_TB=_region_malloc(_T6,0U,sizeof(struct _tuple18));
_TB->f0=tv;_T7=k;{struct Cyc_Core_Opt*_TC=_cycalloc(sizeof(struct Cyc_Core_Opt));_TC->v=s;_T8=(struct Cyc_Core_Opt*)_TC;}_TB->f1=Cyc_Absyn_new_evar(_T7,_T8);_T5=(struct _tuple18*)_TB;}return _T5;}}}
# 2680
void Cyc_Tcutil_check_bitfield(unsigned loc,void*field_type,struct Cyc_Absyn_Exp*width,struct _fat_ptr*fn){int _T0;struct Cyc_Warn_String_Warn_Warg_struct _T1;struct Cyc_Warn_String_Warn_Warg_struct _T2;struct _fat_ptr*_T3;struct Cyc_Warn_String_Warn_Warg_struct _T4;unsigned _T5;struct _fat_ptr _T6;int _T7;struct Cyc_Warn_String_Warn_Warg_struct _T8;unsigned _T9;struct _fat_ptr _TA;unsigned _TB;int _TC;struct Cyc_Warn_String_Warn_Warg_struct _TD;unsigned _TE;struct _fat_ptr _TF;int*_T10;int _T11;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T12;void*_T13;int*_T14;int _T15;void*_T16;enum Cyc_Absyn_Size_of _T17;int _T18;int _T19;struct Cyc_Warn_String_Warn_Warg_struct _T1A;unsigned _T1B;struct _fat_ptr _T1C;struct Cyc_Warn_String_Warn_Warg_struct _T1D;struct Cyc_Warn_String_Warn_Warg_struct _T1E;struct _fat_ptr*_T1F;struct Cyc_Warn_String_Warn_Warg_struct _T20;struct Cyc_Warn_Typ_Warn_Warg_struct _T21;unsigned _T22;struct _fat_ptr _T23;
# 2682
if(width==0)goto _TL4CA;{
unsigned w=0U;_T0=
Cyc_Tcutil_is_const_exp(width);if(_T0)goto _TL4CC;else{goto _TL4CE;}
_TL4CE:{struct Cyc_Warn_String_Warn_Warg_struct _T24;_T24.tag=0;_T24.f1=_tag_fat("bitfield ",sizeof(char),10U);_T1=_T24;}{struct Cyc_Warn_String_Warn_Warg_struct _T24=_T1;{struct Cyc_Warn_String_Warn_Warg_struct _T25;_T25.tag=0;_T3=fn;_T25.f1=*_T3;_T2=_T25;}{struct Cyc_Warn_String_Warn_Warg_struct _T25=_T2;{struct Cyc_Warn_String_Warn_Warg_struct _T26;_T26.tag=0;_T26.f1=_tag_fat(" does not have constant width",sizeof(char),30U);_T4=_T26;}{struct Cyc_Warn_String_Warn_Warg_struct _T26=_T4;void*_T27[3];_T27[0]=& _T24;_T27[1]=& _T25;_T27[2]=& _T26;_T5=loc;_T6=_tag_fat(_T27,sizeof(void*),3);Cyc_Warn_err2(_T5,_T6);}}}goto _TL4CD;
# 2687
_TL4CC:{struct _tuple12 _T24=Cyc_Evexp_eval_const_uint_exp(width);int _T25;unsigned _T26;_T26=_T24.f0;_T25=_T24.f1;{unsigned i=_T26;int known=_T25;_T7=known;
if(_T7)goto _TL4CF;else{goto _TL4D1;}
_TL4D1:{struct Cyc_Warn_String_Warn_Warg_struct _T27;_T27.tag=0;_T27.f1=_tag_fat("cannot evaluate bitfield width at compile time",sizeof(char),47U);_T8=_T27;}{struct Cyc_Warn_String_Warn_Warg_struct _T27=_T8;void*_T28[1];_T28[0]=& _T27;_T9=loc;_TA=_tag_fat(_T28,sizeof(void*),1);Cyc_Warn_warn2(_T9,_TA);}goto _TL4D0;_TL4CF: _TL4D0: _TB=i;_TC=(int)_TB;
if(_TC >= 0)goto _TL4D2;{struct Cyc_Warn_String_Warn_Warg_struct _T27;_T27.tag=0;
_T27.f1=_tag_fat("bitfield has negative width",sizeof(char),28U);_TD=_T27;}{struct Cyc_Warn_String_Warn_Warg_struct _T27=_TD;void*_T28[1];_T28[0]=& _T27;_TE=loc;_TF=_tag_fat(_T28,sizeof(void*),1);Cyc_Warn_err2(_TE,_TF);}goto _TL4D3;_TL4D2: _TL4D3:
 w=i;}}_TL4CD: {
# 2694
void*_T24=Cyc_Absyn_compress(field_type);enum Cyc_Absyn_Size_of _T25;_T10=(int*)_T24;_T11=*_T10;if(_T11!=0)goto _TL4D4;_T12=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T24;_T13=_T12->f1;_T14=(int*)_T13;_T15=*_T14;if(_T15!=1)goto _TL4D6;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T26=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T24;_T16=_T26->f1;{struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*_T27=(struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_T16;_T25=_T27->f2;}}{enum Cyc_Absyn_Size_of b=_T25;
# 2697
int bad;_T17=b;_T18=(int)_T17;switch(_T18){case Cyc_Absyn_Char_sz:
# 2699
 bad=w > 8U;goto _LL8;case Cyc_Absyn_Short_sz:
 bad=w > 16U;goto _LL8;case Cyc_Absyn_Long_sz: goto _LL10;case Cyc_Absyn_Int_sz: _LL10:
# 2702
 bad=w > 32U;goto _LL8;case Cyc_Absyn_LongLong_sz: goto _LL14;default: _LL14:
# 2704
 bad=w > 64U;goto _LL8;}_LL8: _T19=bad;
# 2706
if(!_T19)goto _TL4D9;{struct Cyc_Warn_String_Warn_Warg_struct _T26;_T26.tag=0;
_T26.f1=_tag_fat("bitfield larger than type",sizeof(char),26U);_T1A=_T26;}{struct Cyc_Warn_String_Warn_Warg_struct _T26=_T1A;void*_T27[1];_T27[0]=& _T26;_T1B=loc;_T1C=_tag_fat(_T27,sizeof(void*),1);Cyc_Warn_warn2(_T1B,_T1C);}goto _TL4DA;_TL4D9: _TL4DA: goto _LL3;}_TL4D6: goto _LL6;_TL4D4: _LL6:{struct Cyc_Warn_String_Warn_Warg_struct _T26;_T26.tag=0;
# 2710
_T26.f1=_tag_fat("bitfield ",sizeof(char),10U);_T1D=_T26;}{struct Cyc_Warn_String_Warn_Warg_struct _T26=_T1D;{struct Cyc_Warn_String_Warn_Warg_struct _T27;_T27.tag=0;_T1F=fn;_T27.f1=*_T1F;_T1E=_T27;}{struct Cyc_Warn_String_Warn_Warg_struct _T27=_T1E;{struct Cyc_Warn_String_Warn_Warg_struct _T28;_T28.tag=0;_T28.f1=_tag_fat(" must have integral type but has type ",sizeof(char),39U);_T20=_T28;}{struct Cyc_Warn_String_Warn_Warg_struct _T28=_T20;{struct Cyc_Warn_Typ_Warn_Warg_struct _T29;_T29.tag=2;
_T29.f1=field_type;_T21=_T29;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T29=_T21;void*_T2A[4];_T2A[0]=& _T26;_T2A[1]=& _T27;_T2A[2]=& _T28;_T2A[3]=& _T29;_T22=loc;_T23=_tag_fat(_T2A,sizeof(void*),4);Cyc_Warn_err2(_T22,_T23);}}}}goto _LL3;_LL3:;}}goto _TL4CB;_TL4CA: _TL4CB:;}
# 2719
int Cyc_Tcutil_extract_const_from_typedef(unsigned loc,int declared_const,void*t){void*_T0;int*_T1;int _T2;void*_T3;void*_T4;struct Cyc_Absyn_Typedefdecl*_T5;struct Cyc_Absyn_Tqual _T6;int _T7;struct Cyc_Absyn_Typedefdecl*_T8;struct Cyc_Absyn_Tqual _T9;int _TA;int _TB;struct Cyc_Warn_String_Warn_Warg_struct _TC;unsigned _TD;struct _fat_ptr _TE;void*_TF;unsigned long _T10;int _T11;int _T12;int _T13;void*_T14;struct Cyc_Absyn_Typedefdecl*_T15;_T0=t;_T1=(int*)_T0;_T2=*_T1;if(_T2!=8)goto _TL4DB;_T3=t;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T16=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T3;_T15=_T16->f3;_T4=_T16->f4;_T14=(void*)_T4;}{struct Cyc_Absyn_Typedefdecl*td=_T15;void*tdopt=_T14;_T5=
# 2722
_check_null(td);_T6=_T5->tq;_T7=_T6.real_const;if(_T7)goto _TL4DF;else{goto _TL4E0;}_TL4E0: _T8=td;_T9=_T8->tq;_TA=_T9.print_const;if(_TA)goto _TL4DF;else{goto _TL4DD;}
_TL4DF: _TB=declared_const;if(!_TB)goto _TL4E1;{struct Cyc_Warn_String_Warn_Warg_struct _T16;_T16.tag=0;_T16.f1=_tag_fat("extra const",sizeof(char),12U);_TC=_T16;}{struct Cyc_Warn_String_Warn_Warg_struct _T16=_TC;void*_T17[1];_T17[0]=& _T16;_TD=loc;_TE=_tag_fat(_T17,sizeof(void*),1);Cyc_Warn_warn2(_TD,_TE);}goto _TL4E2;_TL4E1: _TL4E2:
 return 1;_TL4DD: _TF=tdopt;_T10=(unsigned long)_TF;
# 2727
if(!_T10)goto _TL4E3;_T11=
Cyc_Tcutil_extract_const_from_typedef(loc,declared_const,tdopt);return _T11;_TL4E3: _T12=declared_const;
return _T12;}_TL4DB: _T13=declared_const;
return _T13;;}
# 2734
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;int _T1;int _T2;struct Cyc_Absyn_Tvar*_T3;_T0=tv;_T1=_T0->identity;_T2=- 1;
if(_T1!=_T2)goto _TL4E5;_T3=tv;
_T3->identity=Cyc_Tcutil_new_tvar_id();goto _TL4E6;_TL4E5: _TL4E6:;}
# 2738
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*tvs){void(*_T0)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*);void(*_T1)(void(*)(void*),struct Cyc_List_List*);struct Cyc_List_List*_T2;_T1=Cyc_List_iter;{
void(*_T3)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*)=(void(*)(void(*)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*))_T1;_T0=_T3;}_T2=tvs;_T0(Cyc_Tcutil_add_tvar_identity,_T2);}
# 2744
static void Cyc_Tcutil_check_unique_unsorted(int(*cmp)(void*,void*),struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr(*a2string)(void*),struct _fat_ptr msg){struct Cyc_List_List*_T0;struct Cyc_List_List*_T1;void*_T2;struct Cyc_List_List*_T3;void*_T4;int _T5;struct Cyc_Warn_String_Warn_Warg_struct _T6;struct Cyc_Warn_String_Warn_Warg_struct _T7;struct Cyc_Warn_String_Warn_Warg_struct _T8;struct Cyc_List_List*_T9;void*_TA;unsigned _TB;struct _fat_ptr _TC;struct Cyc_List_List*_TD;struct Cyc_List_List*_TE;
# 2747
_TL4EA: if(vs!=0)goto _TL4E8;else{goto _TL4E9;}
_TL4E8: _T0=vs;{struct Cyc_List_List*vs2=_T0->tl;_TL4EE: if(vs2!=0)goto _TL4EC;else{goto _TL4ED;}
_TL4EC: _T1=vs;_T2=_T1->hd;_T3=vs2;_T4=_T3->hd;_T5=cmp(_T2,_T4);if(_T5!=0)goto _TL4EF;{struct Cyc_Warn_String_Warn_Warg_struct _TF;_TF.tag=0;
_TF.f1=msg;_T6=_TF;}{struct Cyc_Warn_String_Warn_Warg_struct _TF=_T6;{struct Cyc_Warn_String_Warn_Warg_struct _T10;_T10.tag=0;_T10.f1=_tag_fat(": ",sizeof(char),3U);_T7=_T10;}{struct Cyc_Warn_String_Warn_Warg_struct _T10=_T7;{struct Cyc_Warn_String_Warn_Warg_struct _T11;_T11.tag=0;_T9=vs;_TA=_T9->hd;_T11.f1=a2string(_TA);_T8=_T11;}{struct Cyc_Warn_String_Warn_Warg_struct _T11=_T8;void*_T12[3];_T12[0]=& _TF;_T12[1]=& _T10;_T12[2]=& _T11;_TB=loc;_TC=_tag_fat(_T12,sizeof(void*),3);Cyc_Warn_err2(_TB,_TC);}}}goto _TL4F0;_TL4EF: _TL4F0: _TD=vs2;
# 2748
vs2=_TD->tl;goto _TL4EE;_TL4ED:;}_TE=vs;
# 2747
vs=_TE->tl;goto _TL4EA;_TL4E9:;}
# 2752
static struct _fat_ptr Cyc_Tcutil_strptr2string(struct _fat_ptr*s){struct _fat_ptr*_T0;struct _fat_ptr _T1;_T0=s;_T1=*_T0;
return _T1;}
# 2755
void Cyc_Tcutil_check_unique_vars(struct Cyc_List_List*vs,unsigned loc,struct _fat_ptr msg){void(*_T0)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct _fat_ptr*),struct _fat_ptr);int(*_T1)(struct _fat_ptr*,struct _fat_ptr*);struct Cyc_List_List*_T2;unsigned _T3;struct _fat_ptr _T4;{
void(*_T5)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct _fat_ptr*),struct _fat_ptr)=(void(*)(int(*)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct _fat_ptr*),struct _fat_ptr))Cyc_Tcutil_check_unique_unsorted;_T0=_T5;}_T1=Cyc_strptrcmp;_T2=vs;_T3=loc;_T4=msg;_T0(_T1,_T2,_T3,Cyc_Tcutil_strptr2string,_T4);}
# 2758
void Cyc_Tcutil_check_unique_tvars(unsigned loc,struct Cyc_List_List*tvs){void(*_T0)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct Cyc_Absyn_Tvar*),struct _fat_ptr);int(*_T1)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*);struct Cyc_List_List*_T2;unsigned _T3;struct _fat_ptr(*_T4)(struct Cyc_Absyn_Tvar*);struct _fat_ptr _T5;{
void(*_T6)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct Cyc_Absyn_Tvar*),struct _fat_ptr)=(void(*)(int(*)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Tvar*),struct Cyc_List_List*,unsigned,struct _fat_ptr(*)(struct Cyc_Absyn_Tvar*),struct _fat_ptr))Cyc_Tcutil_check_unique_unsorted;_T0=_T6;}_T1=Cyc_Absyn_tvar_cmp;_T2=tvs;_T3=loc;_T4=Cyc_Absynpp_tvar2string;_T5=
_tag_fat("duplicate type variable",sizeof(char),24U);
# 2759
_T0(_T1,_T2,_T3,_T4,_T5);}
# 2762
static void Cyc_Tcutil_imp_remove_dups(int(*cmp)(void*,void*),struct Cyc_List_List*l){struct Cyc_List_List*_T0;struct Cyc_List_List*_T1;void*_T2;struct Cyc_List_List*_T3;void*_T4;int _T5;struct Cyc_List_List*_T6;struct Cyc_List_List*_T7;struct Cyc_List_List*_T8;struct Cyc_List_List*_T9;struct Cyc_List_List*_TA;struct Cyc_List_List*_TB;struct Cyc_List_List*_TC;
struct Cyc_List_List*l1=l;_TL4F4: if(l1!=0)goto _TL4F2;else{goto _TL4F3;}
_TL4F2:{struct Cyc_List_List*prev=l1;_T0=l1;{
struct Cyc_List_List*l2=_T0->tl;
_TL4F5: if(l2!=0)goto _TL4F6;else{goto _TL4F7;}
_TL4F6: _T1=l1;_T2=_T1->hd;_T3=l2;_T4=_T3->hd;_T5=cmp(_T2,_T4);if(_T5)goto _TL4F8;else{goto _TL4FA;}
_TL4FA: _T6=_check_null(prev);_T7=l2;_T6->tl=_T7->tl;_T8=l2;
_T8->tl=0;_T9=prev;
l2=_T9->tl;goto _TL4F9;
# 2773
_TL4F8: _TA=_check_null(prev);prev=_TA->tl;_TB=l2;
l2=_TB->tl;_TL4F9: goto _TL4F5;_TL4F7:;}}_TC=l1;
# 2763
l1=_TC->tl;goto _TL4F4;_TL4F3:;}struct _tuple26{struct Cyc_Absyn_Aggrfield*f0;int f1;};struct _tuple27{struct Cyc_List_List*f0;void*f1;};struct _tuple28{struct Cyc_Absyn_Aggrfield*f0;void*f1;};
# 2789 "tcutil.cyc"
struct Cyc_List_List*Cyc_Tcutil_resolve_aggregate_designators(struct _RegionHandle*rgn,unsigned loc,struct Cyc_List_List*des,enum Cyc_Absyn_AggrKind aggr_kind,struct Cyc_List_List*sdfields){struct Cyc_List_List*_T0;void*_T1;struct Cyc_Absyn_Aggrfield*_T2;struct _fat_ptr*_T3;struct _fat_ptr _T4;struct _fat_ptr _T5;int _T6;struct Cyc_List_List*_T7;struct _RegionHandle*_T8;struct _tuple26*_T9;struct _RegionHandle*_TA;struct Cyc_List_List*_TB;void*_TC;struct Cyc_List_List*_TD;struct _fat_ptr _TE;enum Cyc_Absyn_AggrKind _TF;int _T10;struct Cyc_List_List*_T11;void*_T12;void*_T13;struct Cyc_List_List*_T14;void*_T15;struct _tuple26*_T16;struct _tuple26 _T17;int _T18;struct Cyc_List_List*_T19;void*_T1A;struct _tuple26*_T1B;struct Cyc_List_List*_T1C;void*_T1D;struct _tuple27*_T1E;struct Cyc_List_List*_T1F;struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_T20;struct Cyc_List_List*_T21;void*_T22;struct _tuple26*_T23;struct _tuple26 _T24;struct Cyc_Absyn_Aggrfield*_T25;struct Cyc_List_List*_T26;struct _RegionHandle*_T27;struct _tuple28*_T28;struct _RegionHandle*_T29;struct Cyc_List_List*_T2A;void*_T2B;struct _tuple26*_T2C;struct _tuple26 _T2D;struct Cyc_List_List*_T2E;struct Cyc_Warn_String_Warn_Warg_struct _T2F;struct Cyc_Warn_String_Warn_Warg_struct _T30;unsigned _T31;struct _fat_ptr _T32;struct Cyc_List_List*_T33;struct Cyc_List_List*_T34;struct Cyc_Warn_String_Warn_Warg_struct _T35;unsigned _T36;struct _fat_ptr _T37;struct Cyc_List_List*_T38;int*_T39;int _T3A;struct Cyc_Warn_String_Warn_Warg_struct _T3B;struct Cyc_Warn_String_Warn_Warg_struct _T3C;unsigned _T3D;struct _fat_ptr _T3E;struct _fat_ptr*_T3F;struct Cyc_List_List*_T40;void*_T41;struct _tuple26*_T42;struct _tuple26 _T43;struct Cyc_Absyn_Aggrfield*_T44;struct _fat_ptr*_T45;int _T46;struct Cyc_List_List*_T47;void*_T48;struct _tuple26*_T49;struct _tuple26 _T4A;int _T4B;struct Cyc_Warn_String_Warn_Warg_struct _T4C;struct Cyc_Warn_String_Warn_Warg_struct _T4D;struct _fat_ptr*_T4E;struct Cyc_Warn_String_Warn_Warg_struct _T4F;unsigned _T50;struct _fat_ptr _T51;struct Cyc_List_List*_T52;void*_T53;struct _tuple26*_T54;struct Cyc_List_List*_T55;struct _RegionHandle*_T56;struct _tuple28*_T57;struct _RegionHandle*_T58;struct Cyc_List_List*_T59;void*_T5A;struct _tuple26*_T5B;struct _tuple26 _T5C;struct Cyc_List_List*_T5D;struct Cyc_Warn_String_Warn_Warg_struct _T5E;struct Cyc_Warn_String_Warn_Warg_struct _T5F;struct _fat_ptr*_T60;unsigned _T61;struct _fat_ptr _T62;struct Cyc_List_List*_T63;enum Cyc_Absyn_AggrKind _T64;int _T65;int _T66;int _T67;struct Cyc_Warn_String_Warn_Warg_struct _T68;unsigned _T69;struct _fat_ptr _T6A;int _T6B;struct Cyc_Warn_String_Warn_Warg_struct _T6C;unsigned _T6D;struct _fat_ptr _T6E;struct _RegionHandle _T6F=_new_region(0U,"temp");struct _RegionHandle*temp=& _T6F;_push_region(temp);{
# 2798
struct Cyc_List_List*fields=0;{
struct Cyc_List_List*sd_fields=sdfields;_TL4FE: if(sd_fields!=0)goto _TL4FC;else{goto _TL4FD;}
_TL4FC: _T0=sd_fields;_T1=_T0->hd;_T2=(struct Cyc_Absyn_Aggrfield*)_T1;_T3=_T2->name;_T4=*_T3;_T5=_tag_fat("",sizeof(char),1U);_T6=Cyc_strcmp(_T4,_T5);if(_T6==0)goto _TL4FF;_T8=temp;{struct Cyc_List_List*_T70=_region_malloc(_T8,0U,sizeof(struct Cyc_List_List));_TA=temp;{struct _tuple26*_T71=_region_malloc(_TA,0U,sizeof(struct _tuple26));_TB=sd_fields;_TC=_TB->hd;
_T71->f0=(struct Cyc_Absyn_Aggrfield*)_TC;_T71->f1=0;_T9=(struct _tuple26*)_T71;}_T70->hd=_T9;_T70->tl=fields;_T7=(struct Cyc_List_List*)_T70;}fields=_T7;goto _TL500;_TL4FF: _TL500: _TD=sd_fields;
# 2799
sd_fields=_TD->tl;goto _TL4FE;_TL4FD:;}
# 2802
fields=Cyc_List_imp_rev(fields);_TF=aggr_kind;_T10=(int)_TF;
# 2804
if(_T10!=0)goto _TL501;_TE=_tag_fat("struct",sizeof(char),7U);goto _TL502;_TL501: _TE=_tag_fat("union",sizeof(char),6U);_TL502: {struct _fat_ptr aggr_str=_TE;
# 2807
struct Cyc_List_List*ans=0;
_TL506: if(des!=0)goto _TL504;else{goto _TL505;}
_TL504: _T11=des;_T12=_T11->hd;{struct _tuple27*_T70=(struct _tuple27*)_T12;void*_T71;struct Cyc_List_List*_T72;{struct _tuple27 _T73=*_T70;_T72=_T73.f0;_T13=_T73.f1;_T71=(void*)_T13;}{struct Cyc_List_List*dl=_T72;void*a=_T71;
if(dl!=0)goto _TL507;{
# 2812
struct Cyc_List_List*fields2=fields;
_TL50C: if(fields2!=0)goto _TL50A;else{goto _TL50B;}
_TL50A: _T14=fields2;_T15=_T14->hd;_T16=(struct _tuple26*)_T15;_T17=*_T16;_T18=_T17.f1;if(_T18)goto _TL50D;else{goto _TL50F;}
_TL50F: _T19=fields2;_T1A=_T19->hd;_T1B=(struct _tuple26*)_T1A;(*_T1B).f1=1;_T1C=des;_T1D=_T1C->hd;_T1E=(struct _tuple27*)_T1D;{struct Cyc_List_List*_T73=_cycalloc(sizeof(struct Cyc_List_List));{struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_T74=_cycalloc(sizeof(struct Cyc_Absyn_FieldName_Absyn_Designator_struct));_T74->tag=1;_T21=fields2;_T22=_T21->hd;_T23=(struct _tuple26*)_T22;_T24=*_T23;_T25=_T24.f0;
_T74->f1=_T25->name;_T20=(struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_T74;}_T73->hd=(void*)_T20;_T73->tl=0;_T1F=(struct Cyc_List_List*)_T73;}(*_T1E).f0=_T1F;_T27=rgn;{struct Cyc_List_List*_T73=_region_malloc(_T27,0U,sizeof(struct Cyc_List_List));_T29=rgn;{struct _tuple28*_T74=_region_malloc(_T29,0U,sizeof(struct _tuple28));_T2A=fields2;_T2B=_T2A->hd;_T2C=(struct _tuple26*)_T2B;_T2D=*_T2C;
_T74->f0=_T2D.f0;_T74->f1=a;_T28=(struct _tuple28*)_T74;}_T73->hd=_T28;_T73->tl=ans;_T26=(struct Cyc_List_List*)_T73;}ans=_T26;goto _TL50B;_TL50D: _T2E=fields2;
# 2813
fields2=_T2E->tl;goto _TL50C;_TL50B:
# 2820
 if(fields2!=0)goto _TL510;{struct Cyc_Warn_String_Warn_Warg_struct _T73;_T73.tag=0;
_T73.f1=_tag_fat("too many arguments to ",sizeof(char),23U);_T2F=_T73;}{struct Cyc_Warn_String_Warn_Warg_struct _T73=_T2F;{struct Cyc_Warn_String_Warn_Warg_struct _T74;_T74.tag=0;_T74.f1=aggr_str;_T30=_T74;}{struct Cyc_Warn_String_Warn_Warg_struct _T74=_T30;void*_T75[2];_T75[0]=& _T73;_T75[1]=& _T74;_T31=loc;_T32=_tag_fat(_T75,sizeof(void*),2);Cyc_Warn_err2(_T31,_T32);}}goto _TL511;_TL510: _TL511:;}goto _TL508;
_TL507: _T33=dl;_T34=_T33->tl;if(_T34==0)goto _TL512;{struct Cyc_Warn_String_Warn_Warg_struct _T73;_T73.tag=0;
# 2824
_T73.f1=_tag_fat("multiple designators are not yet supported",sizeof(char),43U);_T35=_T73;}{struct Cyc_Warn_String_Warn_Warg_struct _T73=_T35;void*_T74[1];_T74[0]=& _T73;_T36=loc;_T37=_tag_fat(_T74,sizeof(void*),1);Cyc_Warn_err2(_T36,_T37);}goto _TL513;
# 2827
_TL512: _T38=dl;{void*_T73=_T38->hd;struct _fat_ptr*_T74;_T39=(int*)_T73;_T3A=*_T39;if(_T3A!=0)goto _TL514;{struct Cyc_Warn_String_Warn_Warg_struct _T75;_T75.tag=0;
# 2829
_T75.f1=_tag_fat("array designator used in argument to ",sizeof(char),38U);_T3B=_T75;}{struct Cyc_Warn_String_Warn_Warg_struct _T75=_T3B;{struct Cyc_Warn_String_Warn_Warg_struct _T76;_T76.tag=0;_T76.f1=aggr_str;_T3C=_T76;}{struct Cyc_Warn_String_Warn_Warg_struct _T76=_T3C;void*_T77[2];_T77[0]=& _T75;_T77[1]=& _T76;_T3D=loc;_T3E=_tag_fat(_T77,sizeof(void*),2);Cyc_Warn_err2(_T3D,_T3E);}}goto _LL3;_TL514:{struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_T75=(struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_T73;_T74=_T75->f1;}{struct _fat_ptr*v=_T74;
# 2832
struct Cyc_List_List*fields2=fields;
_TL519: if(fields2!=0)goto _TL517;else{goto _TL518;}
_TL517: _T3F=v;_T40=fields2;_T41=_T40->hd;_T42=(struct _tuple26*)_T41;_T43=*_T42;_T44=_T43.f0;_T45=_T44->name;_T46=Cyc_strptrcmp(_T3F,_T45);if(_T46!=0)goto _TL51A;_T47=fields2;_T48=_T47->hd;_T49=(struct _tuple26*)_T48;_T4A=*_T49;_T4B=_T4A.f1;
if(!_T4B)goto _TL51C;{struct Cyc_Warn_String_Warn_Warg_struct _T75;_T75.tag=0;
_T75.f1=_tag_fat("member ",sizeof(char),8U);_T4C=_T75;}{struct Cyc_Warn_String_Warn_Warg_struct _T75=_T4C;{struct Cyc_Warn_String_Warn_Warg_struct _T76;_T76.tag=0;_T4E=v;_T76.f1=*_T4E;_T4D=_T76;}{struct Cyc_Warn_String_Warn_Warg_struct _T76=_T4D;{struct Cyc_Warn_String_Warn_Warg_struct _T77;_T77.tag=0;_T77.f1=_tag_fat(" has already been used as an argument",sizeof(char),38U);_T4F=_T77;}{struct Cyc_Warn_String_Warn_Warg_struct _T77=_T4F;void*_T78[3];_T78[0]=& _T75;_T78[1]=& _T76;_T78[2]=& _T77;_T50=loc;_T51=_tag_fat(_T78,sizeof(void*),3);Cyc_Warn_err2(_T50,_T51);}}}goto _TL51D;_TL51C: _TL51D: _T52=fields2;_T53=_T52->hd;_T54=(struct _tuple26*)_T53;
(*_T54).f1=1;_T56=rgn;{struct Cyc_List_List*_T75=_region_malloc(_T56,0U,sizeof(struct Cyc_List_List));_T58=rgn;{struct _tuple28*_T76=_region_malloc(_T58,0U,sizeof(struct _tuple28));_T59=fields2;_T5A=_T59->hd;_T5B=(struct _tuple26*)_T5A;_T5C=*_T5B;
_T76->f0=_T5C.f0;_T76->f1=a;_T57=(struct _tuple28*)_T76;}_T75->hd=_T57;_T75->tl=ans;_T55=(struct Cyc_List_List*)_T75;}ans=_T55;goto _TL518;_TL51A: _T5D=fields2;
# 2833
fields2=_T5D->tl;goto _TL519;_TL518:
# 2841
 if(fields2!=0)goto _TL51E;{struct Cyc_Warn_String_Warn_Warg_struct _T75;_T75.tag=0;
_T75.f1=_tag_fat("bad field designator ",sizeof(char),22U);_T5E=_T75;}{struct Cyc_Warn_String_Warn_Warg_struct _T75=_T5E;{struct Cyc_Warn_String_Warn_Warg_struct _T76;_T76.tag=0;_T60=v;_T76.f1=*_T60;_T5F=_T76;}{struct Cyc_Warn_String_Warn_Warg_struct _T76=_T5F;void*_T77[2];_T77[0]=& _T75;_T77[1]=& _T76;_T61=loc;_T62=_tag_fat(_T77,sizeof(void*),2);Cyc_Warn_err2(_T61,_T62);}}goto _TL51F;_TL51E: _TL51F: goto _LL3;}_LL3:;}_TL513: _TL508:;}}_T63=des;
# 2808
des=_T63->tl;goto _TL506;_TL505: _T64=aggr_kind;_T65=(int)_T64;
# 2848
if(_T65!=0)goto _TL520;_T66=
Cyc_List_length(ans);_T67=Cyc_List_length(fields);if(_T66 >= _T67)goto _TL522;{struct Cyc_Warn_String_Warn_Warg_struct _T70;_T70.tag=0;
_T70.f1=_tag_fat("too few arguments to struct",sizeof(char),28U);_T68=_T70;}{struct Cyc_Warn_String_Warn_Warg_struct _T70=_T68;void*_T71[1];_T71[0]=& _T70;_T69=loc;_T6A=_tag_fat(_T71,sizeof(void*),1);Cyc_Warn_err2(_T69,_T6A);}goto _TL523;_TL522: _TL523: goto _TL521;
_TL520: _T6B=Cyc_List_length(ans);if(_T6B==1)goto _TL524;{struct Cyc_Warn_String_Warn_Warg_struct _T70;_T70.tag=0;
_T70.f1=_tag_fat("union expression should have one member",sizeof(char),40U);_T6C=_T70;}{struct Cyc_Warn_String_Warn_Warg_struct _T70=_T6C;void*_T71[1];_T71[0]=& _T70;_T6D=loc;_T6E=_tag_fat(_T71,sizeof(void*),1);Cyc_Warn_err2(_T6D,_T6E);}goto _TL525;_TL524: _TL525: _TL521: {struct Cyc_List_List*_T70=
Cyc_List_imp_rev(ans);_npop_handler(0);return _T70;}}}_pop_region();}
# 2859
int Cyc_Tcutil_is_zero_ptr_deref(struct Cyc_Absyn_Exp*e1,void**ptr_type,int*is_fat,void**elt_type){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;struct Cyc_Warn_String_Warn_Warg_struct _T3;struct Cyc_Warn_Exp_Warn_Warg_struct _T4;int(*_T5)(struct _fat_ptr);void*(*_T6)(struct _fat_ptr);struct _fat_ptr _T7;struct Cyc_Absyn_Exp*_T8;void*_T9;void*_TA;void**_TB;int*_TC;void**_TD;int _TE;struct Cyc_Absyn_Exp*_TF;void*_T10;void*_T11;void**_T12;int*_T13;void**_T14;int _T15;struct Cyc_Warn_String_Warn_Warg_struct _T16;struct Cyc_Warn_Exp_Warn_Warg_struct _T17;int(*_T18)(struct _fat_ptr);void*(*_T19)(struct _fat_ptr);struct _fat_ptr _T1A;struct Cyc_Absyn_Exp*_T1B;void*_T1C;void*_T1D;void**_T1E;int*_T1F;void**_T20;int _T21;struct Cyc_Warn_String_Warn_Warg_struct _T22;struct Cyc_Warn_Exp_Warn_Warg_struct _T23;int(*_T24)(struct _fat_ptr);void*(*_T25)(struct _fat_ptr);struct _fat_ptr _T26;struct Cyc_Warn_String_Warn_Warg_struct _T27;struct Cyc_Warn_Exp_Warn_Warg_struct _T28;int(*_T29)(struct _fat_ptr);void*(*_T2A)(struct _fat_ptr);struct _fat_ptr _T2B;_T0=e1;{
# 2861
void*_T2C=_T0->r;struct Cyc_Absyn_Exp*_T2D;_T1=(int*)_T2C;_T2=*_T1;switch(_T2){case 14:{struct Cyc_Warn_String_Warn_Warg_struct _T2E;_T2E.tag=0;
_T2E.f1=_tag_fat("we have a cast in a lhs: ",sizeof(char),26U);_T3=_T2E;}{struct Cyc_Warn_String_Warn_Warg_struct _T2E=_T3;{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F;_T2F.tag=4;_T2F.f1=e1;_T4=_T2F;}{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F=_T4;void*_T30[2];_T30[0]=& _T2E;_T30[1]=& _T2F;_T6=Cyc_Warn_impos2;{int(*_T31)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T6;_T5=_T31;}_T7=_tag_fat(_T30,sizeof(void*),2);_T5(_T7);}}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T2E=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T2C;_T2D=_T2E->f1;}{struct Cyc_Absyn_Exp*e1a=_T2D;_T2D=e1a;goto _LL6;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T2E=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T2C;_T2D=_T2E->f1;}_LL6: {struct Cyc_Absyn_Exp*e1a=_T2D;_T8=e1a;_T9=_T8->topt;_TA=
# 2865
_check_null(_T9);_TB=ptr_type;_TC=is_fat;_TD=elt_type;_TE=Cyc_Tcutil_is_zero_ptr_type(_TA,_TB,_TC,_TD);return _TE;}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T2E=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T2C;_T2D=_T2E->f1;}{struct Cyc_Absyn_Exp*e1a=_T2D;_T2D=e1a;goto _LLA;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T2E=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T2C;_T2D=_T2E->f1;}_LLA: {struct Cyc_Absyn_Exp*e1a=_T2D;_TF=e1a;_T10=_TF->topt;_T11=
# 2869
_check_null(_T10);_T12=ptr_type;_T13=is_fat;_T14=elt_type;_T15=Cyc_Tcutil_is_zero_ptr_type(_T11,_T12,_T13,_T14);if(!_T15)goto _TL527;{struct Cyc_Warn_String_Warn_Warg_struct _T2E;_T2E.tag=0;
_T2E.f1=_tag_fat("found zero pointer aggregate member assignment: ",sizeof(char),49U);_T16=_T2E;}{struct Cyc_Warn_String_Warn_Warg_struct _T2E=_T16;{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F;_T2F.tag=4;_T2F.f1=e1;_T17=_T2F;}{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F=_T17;void*_T30[2];_T30[0]=& _T2E;_T30[1]=& _T2F;_T19=Cyc_Warn_impos2;{int(*_T31)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T19;_T18=_T31;}_T1A=_tag_fat(_T30,sizeof(void*),2);_T18(_T1A);}}goto _TL528;_TL527: _TL528:
 return 0;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_T2E=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_T2C;_T2D=_T2E->f1;}{struct Cyc_Absyn_Exp*e1a=_T2D;_T2D=e1a;goto _LLE;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_T2E=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_T2C;_T2D=_T2E->f1;}_LLE: {struct Cyc_Absyn_Exp*e1a=_T2D;_T1B=e1a;_T1C=_T1B->topt;_T1D=
# 2874
_check_null(_T1C);_T1E=ptr_type;_T1F=is_fat;_T20=elt_type;_T21=Cyc_Tcutil_is_zero_ptr_type(_T1D,_T1E,_T1F,_T20);if(!_T21)goto _TL529;{struct Cyc_Warn_String_Warn_Warg_struct _T2E;_T2E.tag=0;
_T2E.f1=_tag_fat("found zero pointer instantiate/noinstantiate: ",sizeof(char),47U);_T22=_T2E;}{struct Cyc_Warn_String_Warn_Warg_struct _T2E=_T22;{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F;_T2F.tag=4;_T2F.f1=e1;_T23=_T2F;}{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F=_T23;void*_T30[2];_T30[0]=& _T2E;_T30[1]=& _T2F;_T25=Cyc_Warn_impos2;{int(*_T31)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T25;_T24=_T31;}_T26=_tag_fat(_T30,sizeof(void*),2);_T24(_T26);}}goto _TL52A;_TL529: _TL52A:
 return 0;}case 1:
 return 0;default:{struct Cyc_Warn_String_Warn_Warg_struct _T2E;_T2E.tag=0;
_T2E.f1=_tag_fat("found bad lhs in is_zero_ptr_deref: ",sizeof(char),37U);_T27=_T2E;}{struct Cyc_Warn_String_Warn_Warg_struct _T2E=_T27;{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F;_T2F.tag=4;_T2F.f1=e1;_T28=_T2F;}{struct Cyc_Warn_Exp_Warn_Warg_struct _T2F=_T28;void*_T30[2];_T30[0]=& _T2E;_T30[1]=& _T2F;_T2A=Cyc_Warn_impos2;{int(*_T31)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T2A;_T29=_T31;}_T2B=_tag_fat(_T30,sizeof(void*),2);_T29(_T2B);}}};}}
# 2886
int Cyc_Tcutil_is_noalias_qual(void*aq,int must_be_unique){int*_T0;int _T1;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T2;void*_T3;int*_T4;unsigned _T5;void*_T6;enum Cyc_Absyn_AliasQualVal _T7;int _T8;int _T9;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TA;struct Cyc_List_List*_TB;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TC;struct Cyc_List_List*_TD;struct Cyc_List_List*_TE;void*_TF;int*_T10;unsigned _T11;struct Cyc_List_List*_T12;struct Cyc_List_List*_T13;struct Cyc_List_List*_T14;int*_T15;int _T16;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T17;void*_T18;int*_T19;int _T1A;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T1B;void*_T1C;struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*_T1D;enum Cyc_Absyn_AliasQualVal _T1E;int _T1F;int _T20;
void*_T21=Cyc_Absyn_compress(aq);struct Cyc_List_List*_T22;enum Cyc_Absyn_AliasQualVal _T23;_T0=(int*)_T21;_T1=*_T0;if(_T1!=0)goto _TL52B;_T2=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T3=_T2->f1;_T4=(int*)_T3;_T5=*_T4;switch(_T5){case 16:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T24=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T6=_T24->f1;{struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*_T25=(struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)_T6;_T23=_T25->f1;}}{enum Cyc_Absyn_AliasQualVal aqv=_T23;_T7=aqv;_T8=(int)_T7;switch(_T8){case Cyc_Absyn_Unique_qual:
# 2890
 return 1;case Cyc_Absyn_Refcnt_qual: goto _LLF;case Cyc_Absyn_Restricted_qual: _LLF: _T9=!must_be_unique;
# 2892
return _T9;default:
 return 0;};}case 17: _TA=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_TB=_TA->f2;if(_TB==0)goto _TL52F;_TC=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_TD=_TC->f2;_TE=(struct Cyc_List_List*)_TD;_TF=_TE->hd;_T10=(int*)_TF;_T11=*_T10;switch(_T11){case 2:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T24=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T12=_T24->f2;{struct Cyc_List_List _T25=*_T12;_T22=_T25.tl;}}{struct Cyc_List_List*bnd=_T22;_T22=bnd;goto _LL6;}case 0:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T24=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T21;_T13=_T24->f2;{struct Cyc_List_List _T25=*_T13;_T22=_T25.tl;}}_LL6: {struct Cyc_List_List*bnd=_T22;_T14=
# 2899
_check_null(bnd);{void*_T24=_T14->hd;_T15=(int*)_T24;_T16=*_T15;if(_T16!=0)goto _TL532;_T17=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T24;_T18=_T17->f1;_T19=(int*)_T18;_T1A=*_T19;if(_T1A!=16)goto _TL534;_T1B=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T24;_T1C=_T1B->f1;_T1D=(struct Cyc_Absyn_AqualConstCon_Absyn_TyCon_struct*)_T1C;_T1E=_T1D->f1;_T1F=(int)_T1E;switch(_T1F){case Cyc_Absyn_Unique_qual:
 return 1;case Cyc_Absyn_Refcnt_qual: goto _LL18;case Cyc_Absyn_Restricted_qual: _LL18: _T20=!must_be_unique;
# 2902
return _T20;default: goto _LL19;}goto _TL535;_TL534: goto _LL19;_TL535: goto _TL533;_TL532: _LL19:
 return 0;_TL533:;}}default: goto _LL7;}goto _TL530;_TL52F: goto _LL7;_TL530:;default: goto _LL7;}goto _TL52C;_TL52B: _LL7:
# 2907
 return 0;_TL52C:;}
# 2912
int Cyc_Tcutil_is_noalias_pointer(struct Cyc_List_List*aqb,void*t,int must_be_unique){int*_T0;unsigned _T1;struct Cyc_Absyn_PtrInfo _T2;struct Cyc_Absyn_PtrAtts _T3;int _T4;void*_T5;int _T6;int _T7;
void*_T8=Cyc_Absyn_compress(t);struct Cyc_Absyn_Tvar*_T9;void*_TA;_T0=(int*)_T8;_T1=*_T0;switch(_T1){case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TB=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T8;_T2=_TB->f1;_T3=_T2.ptr_atts;_TA=_T3.aqual;}{void*aq=_TA;_T4=
# 2915
Cyc_Tcutil_is_noalias_qual(aq,must_be_unique);return _T4;}case 2:{struct Cyc_Absyn_VarType_Absyn_Type_struct*_TB=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T8;_T9=_TB->f1;}{struct Cyc_Absyn_Tvar*tv=_T9;_T5=
# 2917
Cyc_Tcutil_lookup_aquals(aqb,t);_T6=must_be_unique;_T7=Cyc_Tcutil_is_noalias_qual(_T5,_T6);return _T7;}default:
 return 0;};}
# 2921
int Cyc_Tcutil_is_noalias_pointer_or_aggr(struct Cyc_List_List*aqb,void*t){int _T0;void*_T1;int*_T2;unsigned _T3;void*_T4;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T5;void*_T6;int*_T7;unsigned _T8;void*_T9;struct Cyc_Absyn_AppType_Absyn_Type_struct*_TA;void*_TB;struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_TC;union Cyc_Absyn_AggrInfo _TD;struct _union_AggrInfo_KnownAggr _TE;unsigned _TF;void*_T10;void*_T11;union Cyc_Absyn_AggrInfo _T12;struct _union_AggrInfo_KnownAggr _T13;struct Cyc_Absyn_Aggrdecl**_T14;struct Cyc_Absyn_Aggrdecl*_T15;struct Cyc_Absyn_AggrdeclImpl*_T16;struct Cyc_Absyn_Aggrdecl**_T17;struct Cyc_Absyn_Aggrdecl*_T18;struct Cyc_List_List*_T19;struct Cyc_List_List*_T1A;struct Cyc_Absyn_Aggrdecl**_T1B;struct Cyc_Absyn_Aggrdecl*_T1C;struct Cyc_Absyn_AggrdeclImpl*_T1D;struct Cyc_Absyn_AggrdeclImpl*_T1E;struct Cyc_List_List*_T1F;struct Cyc_List_List*_T20;struct Cyc_List_List*_T21;void*_T22;struct Cyc_Absyn_Aggrfield*_T23;void*_T24;void*_T25;int _T26;struct Cyc_List_List*_T27;void*_T28;void*_T29;union Cyc_Absyn_DatatypeFieldInfo _T2A;struct _union_DatatypeFieldInfo_UnknownDatatypefield _T2B;unsigned _T2C;union Cyc_Absyn_DatatypeFieldInfo _T2D;struct _union_DatatypeFieldInfo_KnownDatatypefield _T2E;struct _tuple2 _T2F;union Cyc_Absyn_DatatypeFieldInfo _T30;struct _union_DatatypeFieldInfo_KnownDatatypefield _T31;struct _tuple2 _T32;struct Cyc_Absyn_Datatypedecl*_T33;struct Cyc_List_List*_T34;struct Cyc_List_List*_T35;struct Cyc_Absyn_Datatypefield*_T36;void*_T37;struct Cyc_List_List*_T38;void*_T39;struct _tuple14*_T3A;struct _tuple14 _T3B;struct Cyc_List_List*_T3C;struct Cyc_List_List*_T3D;void*_T3E;struct _tuple14*_T3F;struct _tuple14 _T40;void*_T41;int _T42;struct Cyc_List_List*_T43;void*_T44;struct Cyc_List_List*_T45;struct Cyc_List_List*_T46;void*_T47;struct Cyc_Absyn_Aggrfield*_T48;void*_T49;int _T4A;struct Cyc_List_List*_T4B;
t=Cyc_Absyn_compress(t);_T0=
Cyc_Tcutil_is_noalias_pointer(aqb,t,0);if(!_T0)goto _TL538;
return 1;_TL538: {union Cyc_Absyn_DatatypeFieldInfo _T4C;struct Cyc_List_List*_T4D;struct Cyc_Absyn_Aggrdecl**_T4E;_T1=t;_T2=(int*)_T1;_T3=*_T2;switch(_T3){case 0: _T4=t;_T5=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T4;_T6=_T5->f1;_T7=(int*)_T6;_T8=*_T7;switch(_T8){case 24: _T9=t;_TA=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_TB=_TA->f1;_TC=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_TB;_TD=_TC->f1;_TE=_TD.KnownAggr;_TF=_TE.tag;if(_TF!=2)goto _TL53C;_T10=t;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4F=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T10;_T11=_T4F->f1;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T50=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T11;_T12=_T50->f1;_T13=_T12.KnownAggr;_T4E=_T13.val;}_T4D=_T4F->f2;}{struct Cyc_Absyn_Aggrdecl**adp=_T4E;struct Cyc_List_List*ts=_T4D;_T14=adp;_T15=*_T14;_T16=_T15->impl;
# 2927
if(_T16!=0)goto _TL53E;
return 0;_TL53E: _T17=adp;_T18=*_T17;_T19=_T18->tvs;_T1A=ts;{
struct Cyc_List_List*inst=Cyc_List_zip(_T19,_T1A);_T1B=adp;_T1C=*_T1B;_T1D=_T1C->impl;_T1E=
_check_null(_T1D);{struct Cyc_List_List*x=_T1E->fields;_TL543: if(x!=0)goto _TL541;else{goto _TL542;}
_TL541: _T1F=aqb;_T20=inst;_T21=x;_T22=_T21->hd;_T23=(struct Cyc_Absyn_Aggrfield*)_T22;_T24=_T23->type;_T25=Cyc_Tcutil_substitute(_T20,_T24);_T26=Cyc_Tcutil_is_noalias_pointer_or_aggr(_T1F,_T25);if(!_T26)goto _TL544;
return 1;_TL544: _T27=x;
# 2930
x=_T27->tl;goto _TL543;_TL542:;}
# 2933
return 0;}}_TL53C: goto _LL8;case 22: _LL8:
# 2940
 return 0;case 23: _T28=t;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4F=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T28;_T29=_T4F->f1;{struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*_T50=(struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_T29;_T4C=_T50->f1;}_T4D=_T4F->f2;}{union Cyc_Absyn_DatatypeFieldInfo tinfo=_T4C;struct Cyc_List_List*ts=_T4D;struct Cyc_Absyn_Datatypefield*_T4F;struct Cyc_Absyn_Datatypedecl*_T50;_T2A=tinfo;_T2B=_T2A.UnknownDatatypefield;_T2C=_T2B.tag;if(_T2C!=1)goto _TL546;
# 2944
return 0;_TL546: _T2D=tinfo;_T2E=_T2D.KnownDatatypefield;_T2F=_T2E.val;_T50=_T2F.f0;_T30=tinfo;_T31=_T30.KnownDatatypefield;_T32=_T31.val;_T4F=_T32.f1;{struct Cyc_Absyn_Datatypedecl*td=_T50;struct Cyc_Absyn_Datatypefield*fld=_T4F;_T33=td;_T34=_T33->tvs;_T35=ts;{
# 2946
struct Cyc_List_List*inst=Cyc_List_zip(_T34,_T35);_T36=fld;{
struct Cyc_List_List*typs=_T36->typs;_TL54B: if(typs!=0)goto _TL549;else{goto _TL54A;}
_TL549: if(inst!=0)goto _TL54C;_T38=typs;_T39=_T38->hd;_T3A=(struct _tuple14*)_T39;_T3B=*_T3A;_T37=_T3B.f1;goto _TL54D;_TL54C: _T3C=inst;_T3D=typs;_T3E=_T3D->hd;_T3F=(struct _tuple14*)_T3E;_T40=*_T3F;_T41=_T40.f1;_T37=Cyc_Tcutil_substitute(_T3C,_T41);_TL54D: t=_T37;_T42=
Cyc_Tcutil_is_noalias_pointer_or_aggr(aqb,t);if(!_T42)goto _TL54E;
return 1;_TL54E: _T43=typs;
# 2947
typs=_T43->tl;goto _TL54B;_TL54A:;}
# 2952
return 0;}};}default: goto _LLB;};case 7: _T44=t;{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T4F=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T44;_T4D=_T4F->f3;}{struct Cyc_List_List*x=_T4D;
# 2935
_TL553: if(x!=0)goto _TL551;else{goto _TL552;}
_TL551: _T45=aqb;_T46=x;_T47=_T46->hd;_T48=(struct Cyc_Absyn_Aggrfield*)_T47;_T49=_T48->type;_T4A=Cyc_Tcutil_is_noalias_pointer_or_aggr(_T45,_T49);if(!_T4A)goto _TL554;
return 1;_TL554: _T4B=x;
# 2935
x=_T4B->tl;goto _TL553;_TL552:
# 2938
 return 0;}default: _LLB:
# 2954
 return 0;};}}
# 2961
int Cyc_Tcutil_is_noalias_path(struct Cyc_List_List*aqb,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T3;void*_T4;int*_T5;int _T6;int _T7;struct Cyc_List_List*_T8;struct Cyc_Absyn_Exp*_T9;void*_TA;void*_TB;int _TC;int _TD;int _TE;int _TF;int _T10;struct Cyc_Absyn_Stmt*_T11;int*_T12;unsigned _T13;int _T14;struct Cyc_Warn_String_Warn_Warg_struct _T15;int(*_T16)(struct _fat_ptr);void*(*_T17)(struct _fat_ptr);struct _fat_ptr _T18;_T0=e;{
void*_T19=_T0->r;struct Cyc_Absyn_Stmt*_T1A;struct Cyc_Absyn_Exp*_T1B;struct _fat_ptr*_T1C;struct Cyc_Absyn_Exp*_T1D;_T1=(int*)_T19;_T2=*_T1;switch(_T2){case 1: _T3=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T19;_T4=_T3->f1;_T5=(int*)_T4;_T6=*_T5;if(_T6!=1)goto _TL557;
return 0;_TL557: goto _LL13;case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T19;_T1D=_T1E->f1;}{struct Cyc_Absyn_Exp*e1=_T1D;_T1D=e1;goto _LL6;}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T19;_T1D=_T1E->f1;}_LL6: {struct Cyc_Absyn_Exp*e1=_T1D;_T8=aqb;_T9=e1;_TA=_T9->topt;_TB=
# 2966
_check_null(_TA);_TC=Cyc_Tcutil_is_noalias_pointer(_T8,_TB,1);if(!_TC)goto _TL559;_T7=Cyc_Tcutil_is_noalias_path(aqb,e1);goto _TL55A;_TL559: _T7=0;_TL55A: return _T7;}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T19;_T1D=_T1E->f1;_T1C=_T1E->f2;}{struct Cyc_Absyn_Exp*e1=_T1D;struct _fat_ptr*f=_T1C;_TD=
Cyc_Tcutil_is_noalias_path(aqb,e1);return _TD;}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T19;_T1D=_T1E->f1;_T1B=_T1E->f2;}{struct Cyc_Absyn_Exp*e1=_T1D;struct Cyc_Absyn_Exp*e2=_T1B;
return 0;}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_T19;_T1D=_T1E->f2;_T1B=_T1E->f3;}{struct Cyc_Absyn_Exp*e1=_T1D;struct Cyc_Absyn_Exp*e2=_T1B;_TF=
# 2970
Cyc_Tcutil_is_noalias_path(aqb,e1);if(!_TF)goto _TL55B;_TE=Cyc_Tcutil_is_noalias_path(aqb,e2);goto _TL55C;_TL55B: _TE=0;_TL55C: return _TE;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_T19;_T1D=_T1E->f2;}{struct Cyc_Absyn_Exp*e2=_T1D;_T1D=e2;goto _LL10;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T19;_T1D=_T1E->f2;}_LL10: {struct Cyc_Absyn_Exp*e2=_T1D;_T10=
# 2972
Cyc_Tcutil_is_noalias_path(aqb,e2);return _T10;}case 38:{struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_T1E=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_T19;_T1A=_T1E->f1;}{struct Cyc_Absyn_Stmt*s=_T1A;
# 2974
_TL55D: if(1)goto _TL55E;else{goto _TL55F;}
_TL55E: _T11=s;{void*_T1E=_T11->r;struct Cyc_Absyn_Exp*_T1F;struct Cyc_Absyn_Decl*_T20;struct Cyc_Absyn_Stmt*_T21;struct Cyc_Absyn_Stmt*_T22;_T12=(int*)_T1E;_T13=*_T12;switch(_T13){case 2:{struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*_T23=(struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_T1E;_T22=_T23->f1;_T21=_T23->f2;}{struct Cyc_Absyn_Stmt*s1=_T22;struct Cyc_Absyn_Stmt*s2=_T21;
s=s2;goto _LL15;}case 12:{struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_T23=(struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_T1E;_T20=_T23->f1;_T22=_T23->f2;}{struct Cyc_Absyn_Decl*d=_T20;struct Cyc_Absyn_Stmt*s1=_T22;
s=s1;goto _LL15;}case 1:{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T23=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T1E;_T1F=_T23->f1;}{struct Cyc_Absyn_Exp*e=_T1F;_T14=
Cyc_Tcutil_is_noalias_path(aqb,e);return _T14;}default:{struct Cyc_Warn_String_Warn_Warg_struct _T23;_T23.tag=0;
_T23.f1=_tag_fat("is_noalias_stmt_exp: ill-formed StmtExp",sizeof(char),40U);_T15=_T23;}{struct Cyc_Warn_String_Warn_Warg_struct _T23=_T15;void*_T24[1];_T24[0]=& _T23;_T17=Cyc_Warn_impos2;{int(*_T25)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T17;_T16=_T25;}_T18=_tag_fat(_T24,sizeof(void*),1);_T16(_T18);}}_LL15:;}goto _TL55D;_TL55F:;}default: _LL13:
# 2981
 return 1;};}}
# 2998 "tcutil.cyc"
struct _tuple13 Cyc_Tcutil_addressof_props(struct Cyc_Absyn_Exp*e){struct _tuple13 _T0;struct Cyc_Absyn_Exp*_T1;int*_T2;unsigned _T3;void*_T4;void*_T5;int*_T6;unsigned _T7;struct _tuple13 _T8;void*_T9;struct Cyc_Absyn_Exp*_TA;void*_TB;void*_TC;int*_TD;int _TE;struct _tuple13 _TF;struct _tuple13 _T10;struct Cyc_Absyn_Vardecl*_T11;struct Cyc_Absyn_Tqual _T12;void*_T13;struct Cyc_Absyn_Exp*_T14;void*_T15;void*_T16;int*_T17;int _T18;struct _tuple13 _T19;struct Cyc_Absyn_Vardecl*_T1A;void*_T1B;struct Cyc_Absyn_Vardecl*_T1C;struct _tuple13 _T1D;struct Cyc_Absyn_Vardecl*_T1E;struct Cyc_Absyn_Tqual _T1F;struct Cyc_Absyn_Vardecl*_T20;void*_T21;void*_T22;void*_T23;struct Cyc_Absyn_Vardecl*_T24;struct _tuple13 _T25;struct Cyc_Absyn_Vardecl*_T26;struct Cyc_Absyn_Tqual _T27;struct Cyc_Absyn_Vardecl*_T28;void*_T29;int _T2A;struct _tuple13 _T2B;struct Cyc_Absyn_Exp*_T2C;void*_T2D;int*_T2E;unsigned _T2F;struct Cyc_Absyn_Vardecl*_T30;struct Cyc_Absyn_Aggrfield*_T31;struct Cyc_Absyn_Exp*_T32;struct _tuple13 _T33;int _T34;int _T35;struct Cyc_Absyn_Aggrfield*_T36;struct Cyc_Absyn_Tqual _T37;int _T38;struct _tuple13 _T39;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3A;void*_T3B;int*_T3C;int _T3D;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T3E;void*_T3F;struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T40;union Cyc_Absyn_AggrInfo _T41;struct _union_AggrInfo_KnownAggr _T42;unsigned _T43;void*_T44;union Cyc_Absyn_AggrInfo _T45;struct _union_AggrInfo_KnownAggr _T46;struct Cyc_Absyn_Aggrdecl**_T47;struct Cyc_Absyn_Aggrfield*_T48;struct Cyc_Absyn_Exp*_T49;struct _tuple13 _T4A;int _T4B;int _T4C;struct Cyc_Absyn_Aggrfield*_T4D;struct Cyc_Absyn_Tqual _T4E;int _T4F;struct _tuple13 _T50;struct _tuple13 _T51;int _T52;struct _tuple13 _T53;struct Cyc_Absyn_Exp*_T54;void*_T55;void*_T56;int*_T57;int _T58;struct Cyc_Absyn_PtrInfo _T59;struct Cyc_Absyn_PtrInfo _T5A;struct Cyc_Absyn_PtrAtts _T5B;int*_T5C;unsigned _T5D;struct Cyc_Absyn_Vardecl*_T5E;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T5F;void*_T60;int*_T61;int _T62;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T63;void*_T64;struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T65;union Cyc_Absyn_AggrInfo _T66;struct _union_AggrInfo_KnownAggr _T67;unsigned _T68;void*_T69;union Cyc_Absyn_AggrInfo _T6A;struct _union_AggrInfo_KnownAggr _T6B;struct Cyc_Absyn_Aggrdecl**_T6C;struct _tuple13 _T6D;struct Cyc_Absyn_Aggrfield*_T6E;struct Cyc_Absyn_Exp*_T6F;struct _tuple13 _T70;int _T71;int _T72;struct Cyc_Absyn_Aggrfield*_T73;struct Cyc_Absyn_Tqual _T74;struct _tuple13 _T75;struct _tuple13 _T76;struct Cyc_Absyn_Exp*_T77;void*_T78;void*_T79;int*_T7A;int _T7B;struct Cyc_Absyn_PtrInfo _T7C;struct Cyc_Absyn_PtrInfo _T7D;struct Cyc_Absyn_PtrAtts _T7E;struct _tuple13 _T7F;struct Cyc_Absyn_Tqual _T80;struct _tuple13 _T81;struct Cyc_Absyn_Exp*_T82;void*_T83;void*_T84;void*_T85;int*_T86;unsigned _T87;void*_T88;struct Cyc_Absyn_PtrInfo _T89;struct Cyc_Absyn_PtrInfo _T8A;struct Cyc_Absyn_PtrAtts _T8B;struct _tuple13 _T8C;struct Cyc_Absyn_Tqual _T8D;void*_T8E;struct Cyc_Absyn_ArrayInfo _T8F;struct _tuple13 _T90;struct Cyc_Absyn_Tqual _T91;struct _tuple13 _T92;struct _tuple13 _T93;struct Cyc_Warn_String_Warn_Warg_struct _T94;struct Cyc_Absyn_Exp*_T95;unsigned _T96;struct _fat_ptr _T97;struct _tuple13 _T98;{struct _tuple13 _T99;
# 3000
_T99.f0=0;_T99.f1=Cyc_Absyn_heap_rgn_type;_T0=_T99;}{struct _tuple13 bogus_ans=_T0;_T1=e;{
void*_T99=_T1->r;struct Cyc_Absyn_Exp*_T9A;int _T9B;struct _fat_ptr*_T9C;struct Cyc_Absyn_Exp*_T9D;void*_T9E;_T2=(int*)_T99;_T3=*_T2;switch(_T3){case 1:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T9F=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T99;_T4=_T9F->f1;_T9E=(void*)_T4;}{void*x=_T9E;struct Cyc_Absyn_Vardecl*_T9F;_T5=x;_T6=(int*)_T5;_T7=*_T6;switch(_T7){case 0: goto _LL11;case 2: _LL11: _T8=bogus_ans;
# 3006
return _T8;case 1: _T9=x;{struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_TA0=(struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_T9;_T9F=_TA0->f1;}{struct Cyc_Absyn_Vardecl*vd=_T9F;_TA=e;_TB=_TA->topt;_TC=
# 3008
_check_null(_TB);{void*_TA0=Cyc_Absyn_compress(_TC);_TD=(int*)_TA0;_TE=*_TD;if(_TE!=5)goto _TL563;{struct _tuple13 _TA1;
# 3010
_TA1.f0=1;_TA1.f1=Cyc_Absyn_heap_rgn_type;_TF=_TA1;}return _TF;_TL563:{struct _tuple13 _TA1;_T11=vd;_T12=_T11->tq;
_TA1.f0=_T12.real_const;_TA1.f1=Cyc_Absyn_heap_rgn_type;_T10=_TA1;}return _T10;;}}case 4: _T13=x;{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_TA0=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_T13;_T9F=_TA0->f1;}{struct Cyc_Absyn_Vardecl*vd=_T9F;_T14=e;_T15=_T14->topt;_T16=
# 3014
_check_null(_T15);{void*_TA0=Cyc_Absyn_compress(_T16);_T17=(int*)_TA0;_T18=*_T17;if(_T18!=5)goto _TL565;{struct _tuple13 _TA1;
_TA1.f0=1;_T1A=vd;_T1B=_T1A->rgn;_TA1.f1=_check_null(_T1B);_T19=_TA1;}return _T19;_TL565: _T1C=vd;
# 3017
_T1C->escapes=1;{struct _tuple13 _TA1;_T1E=vd;_T1F=_T1E->tq;
_TA1.f0=_T1F.real_const;_T20=vd;_T21=_T20->rgn;_TA1.f1=_check_null(_T21);_T1D=_TA1;}return _T1D;;}}case 5: _T22=x;{struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*_TA0=(struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_T22;_T9F=_TA0->f1;}{struct Cyc_Absyn_Vardecl*vd=_T9F;_T9F=vd;goto _LL19;}default: _T23=x;{struct Cyc_Absyn_Param_b_Absyn_Binding_struct*_TA0=(struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_T23;_T9F=_TA0->f1;}_LL19: {struct Cyc_Absyn_Vardecl*vd=_T9F;_T24=vd;
# 3022
_T24->escapes=1;{struct _tuple13 _TA0;_T26=vd;_T27=_T26->tq;
_TA0.f0=_T27.real_const;_T28=vd;_T29=_T28->rgn;_TA0.f1=_check_null(_T29);_T25=_TA0;}return _T25;}};}case 23:{struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*_T9F=(struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_T99;_T9D=_T9F->f1;_T9C=_T9F->f2;_T9B=_T9F->f3;}{struct Cyc_Absyn_Exp*e1=_T9D;struct _fat_ptr*f=_T9C;int is_tagged=_T9B;_T2A=is_tagged;
# 3027
if(!_T2A)goto _TL567;_T2B=bogus_ans;return _T2B;_TL567: {
# 3029
int subset=0;_T2C=e1;_T2D=_T2C->topt;{
void*e1t=_check_null(_T2D);
# 3033
LOOP1: {
void*_T9F=Cyc_Absyn_compress(e1t);struct Cyc_Absyn_Aggrdecl*_TA0;struct Cyc_List_List*_TA1;struct Cyc_Absyn_Vardecl*_TA2;_T2E=(int*)_T9F;_T2F=*_T2E;switch(_T2F){case 12:{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TA3=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_T9F;_TA2=_TA3->f1;}{struct Cyc_Absyn_Vardecl*vd=_TA2;
# 3036
subset=1;_T30=vd;
e1t=_T30->type;goto LOOP1;}case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_TA3=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T9F;_TA1=_TA3->f3;}{struct Cyc_List_List*fs=_TA1;
# 3040
struct Cyc_Absyn_Aggrfield*finfo=Cyc_Absyn_lookup_field(fs,f);
if(finfo==0)goto _TL56A;_T31=finfo;_T32=_T31->width;if(_T32!=0)goto _TL56A;{
struct _tuple13 _TA3=Cyc_Tcutil_addressof_props(e1);void*_TA4;int _TA5;_TA5=_TA3.f0;_TA4=_TA3.f1;{int c=_TA5;void*t=_TA4;{struct _tuple13 _TA6;_T35=subset;
if(_T35)goto _TL56E;else{goto _TL56F;}_TL56F: _T36=finfo;_T37=_T36->tq;_T38=_T37.real_const;if(_T38)goto _TL56E;else{goto _TL56C;}_TL56E: _T34=1;goto _TL56D;_TL56C: _T34=c;_TL56D: _TA6.f0=_T34;_TA6.f1=t;_T33=_TA6;}return _T33;}}_TL56A: _T39=bogus_ans;
# 3045
return _T39;}case 0: _T3A=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9F;_T3B=_T3A->f1;_T3C=(int*)_T3B;_T3D=*_T3C;if(_T3D!=24)goto _TL570;_T3E=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9F;_T3F=_T3E->f1;_T40=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T3F;_T41=_T40->f1;_T42=_T41.KnownAggr;_T43=_T42.tag;if(_T43!=2)goto _TL572;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TA3=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9F;_T44=_TA3->f1;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_TA4=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T44;_T45=_TA4->f1;_T46=_T45.KnownAggr;_T47=_T46.val;{struct Cyc_Absyn_Aggrdecl*_TA5=*_T47;_TA0=_TA5;}}}{struct Cyc_Absyn_Aggrdecl*ad=_TA0;
# 3047
struct Cyc_Absyn_Aggrfield*finfo=Cyc_Absyn_lookup_decl_field(ad,f);
if(finfo==0)goto _TL574;_T48=finfo;_T49=_T48->width;if(_T49!=0)goto _TL574;{
struct _tuple13 _TA3=Cyc_Tcutil_addressof_props(e1);void*_TA4;int _TA5;_TA5=_TA3.f0;_TA4=_TA3.f1;{int c=_TA5;void*t=_TA4;{struct _tuple13 _TA6;_T4C=subset;
if(_T4C)goto _TL578;else{goto _TL579;}_TL579: _T4D=finfo;_T4E=_T4D->tq;_T4F=_T4E.real_const;if(_T4F)goto _TL578;else{goto _TL576;}_TL578: _T4B=1;goto _TL577;_TL576: _T4B=c;_TL577: _TA6.f0=_T4B;_TA6.f1=t;_T4A=_TA6;}return _T4A;}}_TL574: _T50=bogus_ans;
# 3052
return _T50;}_TL572: goto _LL2B;_TL570: goto _LL2B;default: _LL2B: _T51=bogus_ans;
return _T51;};}}}}case 24:{struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*_T9F=(struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_T99;_T9D=_T9F->f1;_T9C=_T9F->f2;_T9B=_T9F->f3;}{struct Cyc_Absyn_Exp*e1=_T9D;struct _fat_ptr*f=_T9C;int is_tagged=_T9B;_T52=is_tagged;
# 3057
if(!_T52)goto _TL57A;_T53=bogus_ans;return _T53;_TL57A: {
# 3059
int subset=0;_T54=e1;_T55=_T54->topt;_T56=
# 3062
_check_null(_T55);{void*_T9F=Cyc_Absyn_compress(_T56);void*_TA0;void*_TA1;_T57=(int*)_T9F;_T58=*_T57;if(_T58!=4)goto _TL57C;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TA2=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T9F;_T59=_TA2->f1;_TA1=_T59.elt_type;_T5A=_TA2->f1;_T5B=_T5A.ptr_atts;_TA0=_T5B.eff;}{void*t1=_TA1;void*r=_TA0;
# 3064
struct Cyc_Absyn_Aggrfield*finfo;
LOOP2:{
void*_TA2=Cyc_Absyn_compress(t1);struct Cyc_Absyn_Aggrdecl*_TA3;struct Cyc_List_List*_TA4;struct Cyc_Absyn_Vardecl*_TA5;_T5C=(int*)_TA2;_T5D=*_T5C;switch(_T5D){case 12:{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TA6=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_TA2;_TA5=_TA6->f1;}{struct Cyc_Absyn_Vardecl*vd=_TA5;
# 3068
subset=1;_T5E=vd;
t1=_T5E->type;goto LOOP2;}case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_TA6=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_TA2;_TA4=_TA6->f3;}{struct Cyc_List_List*fs=_TA4;
# 3072
finfo=Cyc_Absyn_lookup_field(fs,f);goto _LL38;}case 0: _T5F=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA2;_T60=_T5F->f1;_T61=(int*)_T60;_T62=*_T61;if(_T62!=24)goto _TL57F;_T63=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA2;_T64=_T63->f1;_T65=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T64;_T66=_T65->f1;_T67=_T66.KnownAggr;_T68=_T67.tag;if(_T68!=2)goto _TL581;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TA6=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_TA2;_T69=_TA6->f1;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_TA7=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T69;_T6A=_TA7->f1;_T6B=_T6A.KnownAggr;_T6C=_T6B.val;{struct Cyc_Absyn_Aggrdecl*_TA8=*_T6C;_TA3=_TA8;}}}{struct Cyc_Absyn_Aggrdecl*ad=_TA3;
# 3074
finfo=Cyc_Absyn_lookup_decl_field(ad,f);goto _LL38;}_TL581: goto _LL3F;_TL57F: goto _LL3F;default: _LL3F: _T6D=bogus_ans;
return _T6D;}_LL38:;}
# 3077
if(finfo==0)goto _TL583;_T6E=finfo;_T6F=_T6E->width;if(_T6F!=0)goto _TL583;{struct _tuple13 _TA2;_T72=subset;
if(!_T72)goto _TL585;_T71=1;goto _TL586;_TL585: _T73=finfo;_T74=_T73->tq;_T71=_T74.real_const;_TL586: _TA2.f0=_T71;_TA2.f1=r;_T70=_TA2;}return _T70;_TL583: _T75=bogus_ans;
return _T75;}_TL57C: _T76=bogus_ans;
return _T76;;}}}case 22:{struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*_T9F=(struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_T99;_T9D=_T9F->f1;}{struct Cyc_Absyn_Exp*e1=_T9D;_T77=e1;_T78=_T77->topt;_T79=
# 3084
_check_null(_T78);{void*_T9F=Cyc_Absyn_compress(_T79);void*_TA0;struct Cyc_Absyn_Tqual _TA1;_T7A=(int*)_T9F;_T7B=*_T7A;if(_T7B!=4)goto _TL587;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TA2=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T9F;_T7C=_TA2->f1;_TA1=_T7C.elt_tq;_T7D=_TA2->f1;_T7E=_T7D.ptr_atts;_TA0=_T7E.eff;}{struct Cyc_Absyn_Tqual tq=_TA1;void*r=_TA0;{struct _tuple13 _TA2;_T80=tq;
# 3086
_TA2.f0=_T80.real_const;_TA2.f1=r;_T7F=_TA2;}return _T7F;}_TL587: _T81=bogus_ans;
return _T81;;}}case 25:{struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*_T9F=(struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_T99;_T9D=_T9F->f1;_T9A=_T9F->f2;}{struct Cyc_Absyn_Exp*e1=_T9D;struct Cyc_Absyn_Exp*e2=_T9A;_T82=e1;_T83=_T82->topt;_T84=
# 3092
_check_null(_T83);{void*t=Cyc_Absyn_compress(_T84);void*_T9F;struct Cyc_Absyn_Tqual _TA0;_T85=t;_T86=(int*)_T85;_T87=*_T86;switch(_T87){case 4: _T88=t;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TA1=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T88;_T89=_TA1->f1;_TA0=_T89.elt_tq;_T8A=_TA1->f1;_T8B=_T8A.ptr_atts;_T9F=_T8B.eff;}{struct Cyc_Absyn_Tqual tq=_TA0;void*r=_T9F;{struct _tuple13 _TA1;_T8D=tq;
# 3095
_TA1.f0=_T8D.real_const;_TA1.f1=r;_T8C=_TA1;}return _T8C;}case 5: _T8E=t;{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_TA1=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T8E;_T8F=_TA1->f1;_TA0=_T8F.tq;}{struct Cyc_Absyn_Tqual tq=_TA0;{struct _tuple13 _TA1;_T91=tq;
# 3101
_TA1.f0=_T91.real_const;_T92=Cyc_Tcutil_addressof_props(e1);_TA1.f1=_T92.f1;_T90=_TA1;}return _T90;}default: _T93=bogus_ans;
return _T93;};}}default:{struct Cyc_Warn_String_Warn_Warg_struct _T9F;_T9F.tag=0;
# 3105
_T9F.f1=_tag_fat("unary & applied to non-lvalue",sizeof(char),30U);_T94=_T9F;}{struct Cyc_Warn_String_Warn_Warg_struct _T9F=_T94;void*_TA0[1];_TA0[0]=& _T9F;_T95=e;_T96=_T95->loc;_T97=_tag_fat(_TA0,sizeof(void*),1);Cyc_Warn_err2(_T96,_T97);}_T98=bogus_ans;
return _T98;};}}}
# 3112
void Cyc_Tcutil_check_bound(unsigned loc,unsigned i,void*b,int do_warn){int _T0;unsigned _T1;void*_T2;void*_T3;void*_T4;void*_T5;int _T6;struct Cyc_Warn_String_Warn_Warg_struct _T7;void(*_T8)(unsigned,struct _fat_ptr);int _T9;unsigned _TA;struct _fat_ptr _TB;void*_TC;void*_TD;int _TE;struct Cyc_Warn_String_Warn_Warg_struct _TF;struct Cyc_Warn_Int_Warn_Warg_struct _T10;unsigned _T11;struct Cyc_Warn_String_Warn_Warg_struct _T12;struct Cyc_Warn_Int_Warn_Warg_struct _T13;unsigned _T14;void(*_T15)(unsigned,struct _fat_ptr);int _T16;unsigned _T17;struct _fat_ptr _T18;_T0=
Cyc_Tcutil_is_cvar_type(b);if(!_T0)goto _TL58A;
if(i <= 1U)goto _TL58C;_T1=loc;_T2=
Cyc_Tcutil_ptrbnd_cvar_equivalent(b);_T3=_check_null(_T2);_T4=
Cyc_Absyn_fatconst();_T5=
# 3115
Cyc_BansheeIf_equality_constraint(_T3,_T4);_T6=Cyc_BansheeIf_add_constraint(_T1,_T5);if(_T6)goto _TL58E;else{goto _TL590;}
# 3117
_TL590:{struct Cyc_Warn_String_Warn_Warg_struct _T19;_T19.tag=0;_T19.f1=_tag_fat("cannot infer an appropriate bound",sizeof(char),34U);_T7=_T19;}{struct Cyc_Warn_String_Warn_Warg_struct _T19=_T7;void*_T1A[1];_T1A[0]=& _T19;_T9=do_warn;if(!_T9)goto _TL591;_T8=Cyc_Warn_warn2;goto _TL592;_TL591: _T8=Cyc_Warn_err2;_TL592: _TA=loc;_TB=_tag_fat(_T1A,sizeof(void*),1);_T8(_TA,_TB);}goto _TL58F;_TL58E: _TL58F: goto _TL58D;_TL58C: _TL58D:
# 3120
 return;_TL58A: _TC=
# 3122
Cyc_Absyn_bounds_one();_TD=b;{struct Cyc_Absyn_Exp*eopt=Cyc_Tcutil_get_bounds_exp(_TC,_TD);
if(eopt!=0)goto _TL593;return;_TL593: {
struct _tuple12 _T19=Cyc_Evexp_eval_const_uint_exp(eopt);int _T1A;unsigned _T1B;_T1B=_T19.f0;_T1A=_T19.f1;{unsigned j=_T1B;int known=_T1A;_TE=known;
if(!_TE)goto _TL595;if(j > i)goto _TL595;{struct Cyc_Warn_String_Warn_Warg_struct _T1C;_T1C.tag=0;
_T1C.f1=_tag_fat("a dereference is out of bounds: ",sizeof(char),33U);_TF=_T1C;}{struct Cyc_Warn_String_Warn_Warg_struct _T1C=_TF;{struct Cyc_Warn_Int_Warn_Warg_struct _T1D;_T1D.tag=12;_T11=j;
_T1D.f1=(int)_T11;_T10=_T1D;}{struct Cyc_Warn_Int_Warn_Warg_struct _T1D=_T10;{struct Cyc_Warn_String_Warn_Warg_struct _T1E;_T1E.tag=0;_T1E.f1=_tag_fat(" <= ",sizeof(char),5U);_T12=_T1E;}{struct Cyc_Warn_String_Warn_Warg_struct _T1E=_T12;{struct Cyc_Warn_Int_Warn_Warg_struct _T1F;_T1F.tag=12;_T14=i;_T1F.f1=(int)_T14;_T13=_T1F;}{struct Cyc_Warn_Int_Warn_Warg_struct _T1F=_T13;void*_T20[4];_T20[0]=& _T1C;_T20[1]=& _T1D;_T20[2]=& _T1E;_T20[3]=& _T1F;_T16=do_warn;
# 3126
if(!_T16)goto _TL597;_T15=Cyc_Warn_warn2;goto _TL598;_TL597: _T15=Cyc_Warn_err2;_TL598: _T17=loc;_T18=_tag_fat(_T20,sizeof(void*),4);_T15(_T17,_T18);}}}}goto _TL596;_TL595: _TL596:
# 3128
 return;}}}}
# 3131
void Cyc_Tcutil_check_nonzero_bound(unsigned loc,void*b){
Cyc_Tcutil_check_bound(loc,0U,b,0);}
# 3139
static int Cyc_Tcutil_cnst_exp(int var_okay,int valueof_okay,struct Cyc_Absyn_Exp*e){struct Cyc_Absyn_Exp*_T0;int*_T1;unsigned _T2;void*_T3;void*_T4;int*_T5;unsigned _T6;void*_T7;struct Cyc_Absyn_Vardecl*_T8;void*_T9;int*_TA;unsigned _TB;int _TC;void*_TD;struct Cyc_Absyn_Vardecl*_TE;enum Cyc_Absyn_Scope _TF;int _T10;int _T11;struct Cyc_Absyn_Vardecl*_T12;void*_T13;int*_T14;unsigned _T15;int _T16;int _T17;int _T18;int _T19;int _T1A;int _T1B;int _T1C;int _T1D;int _T1E;int _T1F;int _T20;int _T21;int _T22;struct Cyc_List_List*_T23;void*_T24;struct _tuple17*_T25;struct _tuple17 _T26;struct Cyc_Absyn_Exp*_T27;int _T28;struct Cyc_List_List*_T29;int _T2A;struct Cyc_List_List*_T2B;void*_T2C;struct Cyc_Absyn_Exp*_T2D;int _T2E;struct Cyc_List_List*_T2F;int _T30;struct Cyc_Absyn_Stmt*_T31;int*_T32;int _T33;int _T34;
int(*rec)(int,int,struct Cyc_Absyn_Exp*)=Cyc_Tcutil_cnst_exp;_T0=e;{
void*_T35=_T0->r;struct Cyc_Absyn_Stmt*_T36;struct Cyc_List_List*_T37;struct Cyc_Absyn_Exp*_T38;struct Cyc_Absyn_Exp*_T39;struct Cyc_Absyn_Exp*_T3A;void*_T3B;_T1=(int*)_T35;_T2=*_T1;switch(_T2){case 0: goto _LL4;case 2: _LL4: goto _LL6;case 17: _LL6: goto _LL8;case 18: _LL8: goto _LLA;case 21: _LLA: goto _LLC;case 33: _LLC: goto _LLE;case 34: _LLE:
# 3148
 return 1;case 1:{struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_T35;_T3=_T3C->f1;_T3B=(void*)_T3;}{void*b=_T3B;struct Cyc_Absyn_Vardecl*_T3C;_T4=b;_T5=(int*)_T4;_T6=*_T5;switch(_T6){case 2:
# 3153
 return 1;case 1: _T7=b;{struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_T3D=(struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_T7;_T3C=_T3D->f1;}{struct Cyc_Absyn_Vardecl*vd=_T3C;_T8=vd;_T9=_T8->type;{
# 3155
void*_T3D=Cyc_Absyn_compress(_T9);_TA=(int*)_T3D;_TB=*_TA;switch(_TB){case 5: goto _LL42;case 6: _LL42:
# 3157
 return 1;default: _TC=var_okay;
return _TC;};}}case 4: _TD=b;{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_T3D=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_TD;_T3C=_T3D->f1;}{struct Cyc_Absyn_Vardecl*vd=_T3C;_TE=vd;_TF=_TE->sc;_T10=(int)_TF;
# 3161
if(_T10==0)goto _TL59C;_T11=var_okay;
return _T11;_TL59C: _T12=vd;_T13=_T12->type;{
void*_T3D=Cyc_Absyn_compress(_T13);_T14=(int*)_T3D;_T15=*_T14;switch(_T15){case 5: goto _LL49;case 6: _LL49:
# 3165
 return 1;default: _T16=var_okay;
return _T16;};}}case 0:
# 3168
 return 0;default: _T17=var_okay;
return _T17;};}case 6:{struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f1;_T39=_T3C->f2;_T38=_T3C->f3;}{struct Cyc_Absyn_Exp*e1=_T3A;struct Cyc_Absyn_Exp*e2=_T39;struct Cyc_Absyn_Exp*e3=_T38;_T19=
# 3172
rec(0,valueof_okay,e1);if(!_T19)goto _TL59F;_T1A=rec(0,valueof_okay,e2);if(!_T1A)goto _TL59F;_T18=
rec(0,valueof_okay,e3);goto _TL5A0;_TL59F: _T18=0;_TL5A0:
# 3172
 return _T18;}case 9:{struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f1;_T39=_T3C->f2;}{struct Cyc_Absyn_Exp*e1=_T3A;struct Cyc_Absyn_Exp*e2=_T39;_T1C=
# 3175
rec(0,valueof_okay,e1);if(!_T1C)goto _TL5A1;_T1B=rec(0,valueof_okay,e2);goto _TL5A2;_TL5A1: _T1B=0;_TL5A2: return _T1B;}case 42:{struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f1;}{struct Cyc_Absyn_Exp*e2=_T3A;_T3A=e2;goto _LL18;}case 12:{struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f1;}_LL18: {struct Cyc_Absyn_Exp*e2=_T3A;_T3A=e2;goto _LL1A;}case 13:{struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f1;}_LL1A: {struct Cyc_Absyn_Exp*e2=_T3A;_T3A=e2;goto _LL1C;}case 14:{struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f2;}_LL1C: {struct Cyc_Absyn_Exp*e2=_T3A;_T1D=
# 3181
Cyc_Tcutil_cnst_exp(var_okay,valueof_okay,e2);return _T1D;}case 15:{struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f1;}{struct Cyc_Absyn_Exp*e2=_T3A;_T1E=
rec(1,valueof_okay,e2);return _T1E;}case 28:{struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f2;_T39=_T3C->f3;}{struct Cyc_Absyn_Exp*e1=_T3A;struct Cyc_Absyn_Exp*e2=_T39;_T20=
# 3184
rec(0,valueof_okay,e1);if(!_T20)goto _TL5A3;_T1F=rec(0,valueof_okay,e2);goto _TL5A4;_TL5A3: _T1F=0;_TL5A4: return _T1F;}case 29:{struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_T35;_T3A=_T3C->f1;}{struct Cyc_Absyn_Exp*e=_T3A;_T21=
rec(0,valueof_okay,e);return _T21;}case 27:{struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_T35;_T37=_T3C->f1;}{struct Cyc_List_List*des=_T37;_T37=des;goto _LL26;}case 31:{struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_T35;_T37=_T3C->f3;}_LL26: {struct Cyc_List_List*des=_T37;_T37=des;goto _LL28;}case 30:{struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_T35;_T37=_T3C->f3;}_LL28: {struct Cyc_List_List*des=_T37;
# 3189
_TL5A8: if(des!=0)goto _TL5A6;else{goto _TL5A7;}
_TL5A6: _T22=valueof_okay;_T23=des;_T24=_T23->hd;_T25=(struct _tuple17*)_T24;_T26=*_T25;_T27=_T26.f1;_T28=Cyc_Tcutil_cnst_exp(0,_T22,_T27);if(_T28)goto _TL5A9;else{goto _TL5AB;}
_TL5AB: return 0;_TL5A9: _T29=des;
# 3189
des=_T29->tl;goto _TL5A8;_TL5A7:
# 3192
 return 1;}case 3:{struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_T35;_T37=_T3C->f2;}{struct Cyc_List_List*es=_T37;_T37=es;goto _LL2C;}case 32:{struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_T35;_T37=_T3C->f1;}_LL2C: {struct Cyc_List_List*es=_T37;
# 3195
_TL5AF: if(es!=0)goto _TL5AD;else{goto _TL5AE;}
_TL5AD: _T2A=valueof_okay;_T2B=es;_T2C=_T2B->hd;_T2D=(struct Cyc_Absyn_Exp*)_T2C;_T2E=Cyc_Tcutil_cnst_exp(0,_T2A,_T2D);if(_T2E)goto _TL5B0;else{goto _TL5B2;}
_TL5B2: return 0;_TL5B0: _T2F=es;
# 3195
es=_T2F->tl;goto _TL5AF;_TL5AE:
# 3198
 return 1;}case 40: _T30=valueof_okay;
return _T30;case 38:{struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*_T3C=(struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_T35;_T36=_T3C->f1;}{struct Cyc_Absyn_Stmt*s=_T36;_T31=s;{
# 3201
void*_T3C=_T31->r;struct Cyc_Absyn_Exp*_T3D;_T32=(int*)_T3C;_T33=*_T32;if(_T33!=1)goto _TL5B3;{struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*_T3E=(struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_T3C;_T3D=_T3E->f1;}{struct Cyc_Absyn_Exp*e=_T3D;_T34=
Cyc_Tcutil_cnst_exp(var_okay,valueof_okay,e);return _T34;}_TL5B3:
 return 0;;}}default:
# 3205
 return 0;};}}
# 3208
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*e){int _T0;_T0=
Cyc_Tcutil_cnst_exp(0,0,e);return _T0;}
# 3211
int Cyc_Tcutil_is_pure_exp(struct Cyc_Absyn_Exp*e){int _T0;_T0=
Cyc_Tcutil_cnst_exp(0,1,e);return _T0;}
# 3215
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_List_List*);
int Cyc_Tcutil_zeroable_type(void*t){int*_T0;unsigned _T1;void*_T2;void*_T3;int*_T4;unsigned _T5;void*_T6;struct Cyc_Absyn_Enumdecl*_T7;struct Cyc_Core_Opt*_T8;struct Cyc_Absyn_Enumdecl*_T9;struct Cyc_Core_Opt*_TA;void*_TB;void*_TC;struct Cyc_List_List*_TD;void*_TE;struct Cyc_Absyn_Enumfield*_TF;struct Cyc_Absyn_Exp*_T10;int _T11;struct Cyc_List_List*_T12;void*_T13;struct Cyc_Absyn_Enumfield*_T14;struct Cyc_Absyn_Exp*_T15;int _T16;struct Cyc_List_List*_T17;void*_T18;struct Cyc_Absyn_Aggrdecl*_T19;struct Cyc_Absyn_AggrdeclImpl*_T1A;struct Cyc_Absyn_Aggrdecl*_T1B;struct Cyc_Absyn_AggrdeclImpl*_T1C;struct Cyc_List_List*_T1D;struct Cyc_Absyn_Aggrdecl*_T1E;enum Cyc_Absyn_AggrKind _T1F;int _T20;struct Cyc_Absyn_Aggrdecl*_T21;struct Cyc_Absyn_AggrdeclImpl*_T22;int _T23;struct Cyc_Absyn_Aggrdecl*_T24;struct Cyc_List_List*_T25;struct Cyc_List_List*_T26;struct Cyc_Absyn_Aggrdecl*_T27;struct Cyc_Absyn_AggrdeclImpl*_T28;struct Cyc_List_List*_T29;int _T2A;struct Cyc_Absyn_PtrInfo _T2B;struct Cyc_Absyn_PtrAtts _T2C;int _T2D;struct Cyc_Absyn_ArrayInfo _T2E;int _T2F;int _T30;
void*_T31=Cyc_Absyn_compress(t);struct Cyc_List_List*_T32;void*_T33;_T0=(int*)_T31;_T1=*_T0;switch(_T1){case 0:{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T31;_T2=_T34->f1;_T33=(void*)_T2;_T32=_T34->f2;}{void*c=_T33;struct Cyc_List_List*ts=_T32;union Cyc_Absyn_AggrInfo _T34;struct Cyc_List_List*_T35;struct Cyc_Absyn_Enumdecl*_T36;_T3=c;_T4=(int*)_T3;_T5=*_T4;switch(_T5){case 0: goto _LLF;case 1: _LLF: goto _LL11;case 3: _LL11: goto _LL13;case 2: _LL13:
# 3223
 return 1;case 19: _T6=c;{struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*_T37=(struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_T6;_T36=_T37->f2;}{struct Cyc_Absyn_Enumdecl*edo=_T36;
# 3225
if(edo==0)goto _TL5B9;else{goto _TL5BA;}_TL5BA: _T7=edo;_T8=_T7->fields;if(_T8==0)goto _TL5B9;else{goto _TL5B7;}
_TL5B9: return 0;_TL5B7: _T9=edo;_TA=_T9->fields;_TB=_TA->v;_T35=(struct Cyc_List_List*)_TB;goto _LL17;}case 20: _TC=c;{struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*_T37=(struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_TC;_T35=_T37->f1;}_LL17: {struct Cyc_List_List*fs=_T35;{
# 3229
struct Cyc_List_List*fs2=fs;_TL5BE: if(fs2!=0)goto _TL5BC;else{goto _TL5BD;}
_TL5BC: _TD=fs2;_TE=_TD->hd;_TF=(struct Cyc_Absyn_Enumfield*)_TE;_T10=_TF->tag;if(_T10!=0)goto _TL5BF;_T11=fs2==fs;
return _T11;_TL5BF: _T12=fs2;_T13=_T12->hd;_T14=(struct Cyc_Absyn_Enumfield*)_T13;_T15=_T14->tag;{
struct _tuple12 _T37=Cyc_Evexp_eval_const_uint_exp(_T15);int _T38;unsigned _T39;_T39=_T37.f0;_T38=_T37.f1;{unsigned i=_T39;int known=_T38;_T16=known;
if(!_T16)goto _TL5C1;if(i!=0U)goto _TL5C1;
return 1;_TL5C1:;}}_T17=fs2;
# 3229
fs2=_T17->tl;goto _TL5BE;_TL5BD:;}
# 3236
return 0;}case 24: _T18=c;{struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*_T37=(struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_T18;_T34=_T37->f1;}{union Cyc_Absyn_AggrInfo info=_T34;
# 3239
struct Cyc_Absyn_Aggrdecl*ad=Cyc_Absyn_get_known_aggrdecl(info);_T19=ad;_T1A=_T19->impl;
if(_T1A!=0)goto _TL5C3;return 0;_TL5C3: _T1B=ad;_T1C=_T1B->impl;_T1D=_T1C->exist_vars;
if(_T1D==0)goto _TL5C5;return 0;_TL5C5: _T1E=ad;_T1F=_T1E->kind;_T20=(int)_T1F;
if(_T20!=1)goto _TL5C7;_T21=ad;_T22=_T21->impl;_T23=_T22->tagged;if(!_T23)goto _TL5C7;return 0;_TL5C7: _T24=ad;_T25=_T24->tvs;_T26=ts;_T27=ad;_T28=_T27->impl;_T29=_T28->fields;_T2A=
Cyc_Tcutil_fields_zeroable(_T25,_T26,_T29);return _T2A;}default:
 return 0;};}case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T31;_T2B=_T34->f1;_T2C=_T2B.ptr_atts;_T33=_T2C.nullable;}{void*n=_T33;_T2D=
# 3247
Cyc_Tcutil_force_type2bool(1,n);return _T2D;}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T31;_T2E=_T34->f1;_T33=_T2E.elt_type;}{void*t=_T33;_T2F=
Cyc_Tcutil_zeroable_type(t);return _T2F;}case 7:{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T34=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T31;_T32=_T34->f3;}{struct Cyc_List_List*fs=_T32;_T30=
Cyc_Tcutil_fields_zeroable(0,0,fs);return _T30;}default:
 return 0;};}
# 3253
static int Cyc_Tcutil_fields_zeroable(struct Cyc_List_List*tvs,struct Cyc_List_List*ts,struct Cyc_List_List*fs){struct Cyc_List_List*_T0;void*_T1;struct Cyc_Absyn_Aggrfield*_T2;int _T3;struct _RegionHandle*_T4;struct Cyc_List_List*_T5;struct Cyc_List_List*_T6;void*_T7;struct Cyc_Absyn_Aggrfield*_T8;void*_T9;int _TA;struct Cyc_List_List*_TB;struct _RegionHandle _TC=_new_region(0U,"rgn");struct _RegionHandle*rgn=& _TC;_push_region(rgn);{
# 3256
struct Cyc_List_List*inst=Cyc_List_rzip(rgn,rgn,tvs,ts);
_TL5CC: if(fs!=0)goto _TL5CA;else{goto _TL5CB;}
_TL5CA: _T0=fs;_T1=_T0->hd;_T2=(struct Cyc_Absyn_Aggrfield*)_T1;{void*t=_T2->type;_T3=
Cyc_Tcutil_zeroable_type(t);if(!_T3)goto _TL5CD;goto _TL5C9;_TL5CD: _T4=rgn;_T5=inst;_T6=fs;_T7=_T6->hd;_T8=(struct Cyc_Absyn_Aggrfield*)_T7;_T9=_T8->type;
t=Cyc_Tcutil_rsubstitute(_T4,_T5,_T9);_TA=
Cyc_Tcutil_zeroable_type(t);if(_TA)goto _TL5CF;else{goto _TL5D1;}_TL5D1:{int _TD=0;_npop_handler(0);return _TD;}_TL5CF:;}_TL5C9: _TB=fs;
# 3257
fs=_TB->tl;goto _TL5CC;_TL5CB: {int _TD=1;_npop_handler(0);return _TD;}}_pop_region();}
# 3267
void Cyc_Tcutil_check_no_qual(unsigned loc,void*t){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_Typedefdecl*_T4;struct Cyc_Absyn_Tqual _T5;int _T6;struct Cyc_Absyn_Tqual _T7;int _T8;struct Cyc_Absyn_Tqual _T9;int _TA;struct Cyc_Absyn_Tqual _TB;int _TC;struct Cyc_Warn_String_Warn_Warg_struct _TD;struct Cyc_Warn_Typ_Warn_Warg_struct _TE;struct Cyc_Warn_String_Warn_Warg_struct _TF;unsigned _T10;struct _fat_ptr _T11;struct Cyc_Absyn_Typedefdecl*_T12;_T0=t;_T1=(int*)_T0;_T2=*_T1;if(_T2!=8)goto _TL5D2;_T3=t;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T13=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T3;_T12=_T13->f3;}{struct Cyc_Absyn_Typedefdecl*tdopt=_T12;
# 3270
if(tdopt!=0)goto _TL5D4;
return;_TL5D4: _T4=tdopt;{
struct Cyc_Absyn_Tqual tq=_T4->tq;_T5=tq;_T6=_T5.print_const;
if(_T6)goto _TL5D8;else{goto _TL5DB;}_TL5DB: _T7=tq;_T8=_T7.q_volatile;if(_T8)goto _TL5D8;else{goto _TL5DA;}_TL5DA: _T9=tq;_TA=_T9.q_restrict;if(_TA)goto _TL5D8;else{goto _TL5D9;}_TL5D9: _TB=tq;_TC=_TB.real_const;if(_TC)goto _TL5D8;else{goto _TL5D6;}
_TL5D8:{struct Cyc_Warn_String_Warn_Warg_struct _T13;_T13.tag=0;_T13.f1=_tag_fat("qualifier within typedef type ",sizeof(char),31U);_TD=_T13;}{struct Cyc_Warn_String_Warn_Warg_struct _T13=_TD;{struct Cyc_Warn_Typ_Warn_Warg_struct _T14;_T14.tag=2;_T14.f1=t;_TE=_T14;}{struct Cyc_Warn_Typ_Warn_Warg_struct _T14=_TE;{struct Cyc_Warn_String_Warn_Warg_struct _T15;_T15.tag=0;_T15.f1=_tag_fat(" is ignored",sizeof(char),12U);_TF=_T15;}{struct Cyc_Warn_String_Warn_Warg_struct _T15=_TF;void*_T16[3];_T16[0]=& _T13;_T16[1]=& _T14;_T16[2]=& _T15;_T10=loc;_T11=_tag_fat(_T16,sizeof(void*),3);Cyc_Warn_warn2(_T10,_T11);}}}goto _TL5D7;_TL5D6: _TL5D7:
 return;}}_TL5D2:
 return;;}
# 3283
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*t){int*_T0;unsigned _T1;void*_T2;struct Cyc_Absyn_PtrInfo _T3;struct Cyc_Absyn_PtrAtts _T4;void*_T5;struct Cyc_Absyn_Exp*_T6;struct Cyc_Absyn_ArrayInfo _T7;struct Cyc_Absyn_Exp*_T8;
void*_T9=Cyc_Absyn_compress(t);struct Cyc_Absyn_Exp*_TA;struct Cyc_Absyn_PtrInfo _TB;_T0=(int*)_T9;_T1=*_T0;switch(_T1){case 4:{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T9;_TB=_TC->f1;}{struct Cyc_Absyn_PtrInfo pi=_TB;_T2=
Cyc_Absyn_bounds_one();_T3=pi;_T4=_T3.ptr_atts;_T5=_T4.bounds;_T6=Cyc_Tcutil_get_bounds_exp_constrain(_T2,_T5,1);return _T6;}case 5:{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T9;_T7=_TC->f1;_TA=_T7.num_elts;}{struct Cyc_Absyn_Exp*e=_TA;_T8=e;
return _T8;}default:
 return 0;};}
# 3293
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*b){void*_T0;int*_T1;unsigned _T2;void*_T3;void*_T4;void*_T5;void*_T6;struct Cyc_Absyn_Vardecl*_T7;struct Cyc_Absyn_Vardecl*_T8;int _T9;struct Cyc_Absyn_Vardecl*_TA;_T0=b;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 5: _T3=b;{struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*_TB=(struct Cyc_Absyn_Pat_b_Absyn_Binding_struct*)_T3;_TA=_TB->f1;}{struct Cyc_Absyn_Vardecl*x=_TA;_TA=x;goto _LL4;}case 4: _T4=b;{struct Cyc_Absyn_Local_b_Absyn_Binding_struct*_TB=(struct Cyc_Absyn_Local_b_Absyn_Binding_struct*)_T4;_TA=_TB->f1;}_LL4: {struct Cyc_Absyn_Vardecl*x=_TA;_TA=x;goto _LL6;}case 3: _T5=b;{struct Cyc_Absyn_Param_b_Absyn_Binding_struct*_TB=(struct Cyc_Absyn_Param_b_Absyn_Binding_struct*)_T5;_TA=_TB->f1;}_LL6: {struct Cyc_Absyn_Vardecl*x=_TA;_TA=x;goto _LL8;}case 1: _T6=b;{struct Cyc_Absyn_Global_b_Absyn_Binding_struct*_TB=(struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)_T6;_TA=_TB->f1;}_LL8: {struct Cyc_Absyn_Vardecl*x=_TA;_T8=x;_T9=_T8->escapes;
# 3298
if(!_T9)goto _TL5DE;_T7=0;goto _TL5DF;_TL5DE: _T7=x;_TL5DF: return _T7;}default:
 return 0;};}
# 3304
struct Cyc_List_List*Cyc_Tcutil_filter_nulls(struct Cyc_List_List*l){struct Cyc_List_List*_T0;void*_T1;void**_T2;struct Cyc_List_List*_T3;struct Cyc_List_List*_T4;void*_T5;void**_T6;struct Cyc_List_List*_T7;struct Cyc_List_List*_T8;
struct Cyc_List_List*res=0;
_TL5E3: if(l!=0)goto _TL5E1;else{goto _TL5E2;}
_TL5E1: _T0=l;_T1=_T0->hd;_T2=(void**)_T1;if(_T2==0)goto _TL5E4;{struct Cyc_List_List*_T9=_cycalloc(sizeof(struct Cyc_List_List));_T4=l;_T5=_T4->hd;_T6=(void**)_T5;_T9->hd=*_T6;_T9->tl=res;_T3=(struct Cyc_List_List*)_T9;}res=_T3;goto _TL5E5;_TL5E4: _TL5E5: _T7=l;
# 3306
l=_T7->tl;goto _TL5E3;_TL5E2: _T8=res;
# 3308
return _T8;}
# 3311
void*Cyc_Tcutil_promote_array(void*t,void*rgn,void*aqual,int convert_tag){int*_T0;int _T1;struct Cyc_Absyn_ArrayInfo _T2;struct Cyc_Absyn_ArrayInfo _T3;struct Cyc_Absyn_ArrayInfo _T4;struct Cyc_Absyn_ArrayInfo _T5;int _T6;struct Cyc_Absyn_Exp*_T7;void*_T8;struct Cyc_Warn_String_Warn_Warg_struct _T9;int(*_TA)(struct _fat_ptr);void*(*_TB)(struct _fat_ptr);struct _fat_ptr _TC;struct Cyc_Absyn_Exp*_TD;void*_TE;int*_TF;int _T10;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T11;void*_T12;int*_T13;int _T14;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T15;struct Cyc_List_List*_T16;struct Cyc_List_List*_T17;void*_T18;struct Cyc_Absyn_Exp*_T19;void*_T1A;int _T1B;void*_T1C;void*_T1D;
void*_T1E=Cyc_Absyn_compress(t);void*_T1F;struct Cyc_Absyn_Exp*_T20;struct Cyc_Absyn_Tqual _T21;void*_T22;_T0=(int*)_T1E;_T1=*_T0;if(_T1!=5)goto _TL5E6;{struct Cyc_Absyn_ArrayType_Absyn_Type_struct*_T23=(struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_T1E;_T2=_T23->f1;_T22=_T2.elt_type;_T3=_T23->f1;_T21=_T3.tq;_T4=_T23->f1;_T20=_T4.num_elts;_T5=_T23->f1;_T1F=_T5.zero_term;}{void*et=_T22;struct Cyc_Absyn_Tqual tq=_T21;struct Cyc_Absyn_Exp*eopt=_T20;void*zt=_T1F;
# 3314
void*b;
if(eopt!=0)goto _TL5E8;
b=Cyc_Absyn_fat_bound_type;goto _TL5E9;
_TL5E8: _T6=convert_tag;if(!_T6)goto _TL5EA;_T7=eopt;_T8=_T7->topt;
if(_T8!=0)goto _TL5EC;{struct Cyc_Warn_String_Warn_Warg_struct _T23;_T23.tag=0;
_T23.f1=_tag_fat("cannot convert tag without type!",sizeof(char),33U);_T9=_T23;}{struct Cyc_Warn_String_Warn_Warg_struct _T23=_T9;void*_T24[1];_T24[0]=& _T23;_TB=Cyc_Warn_impos2;{int(*_T25)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_TB;_TA=_T25;}_TC=_tag_fat(_T24,sizeof(void*),1);_TA(_TC);}goto _TL5ED;_TL5EC: _TL5ED: _TD=eopt;_TE=_TD->topt;{
void*_T23=Cyc_Absyn_compress(_TE);void*_T24;_TF=(int*)_T23;_T10=*_TF;if(_T10!=0)goto _TL5EE;_T11=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T23;_T12=_T11->f1;_T13=(int*)_T12;_T14=*_T13;if(_T14!=5)goto _TL5F0;_T15=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T23;_T16=_T15->f2;if(_T16==0)goto _TL5F2;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T25=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T23;_T17=_T25->f2;{struct Cyc_List_List _T26=*_T17;_T18=_T26.hd;_T24=(void*)_T18;}}{void*t=_T24;_T19=
# 3323
Cyc_Absyn_valueof_exp(t,0U);b=Cyc_Absyn_thin_bounds_exp(_T19);goto _LL5;}_TL5F2: goto _LL8;_TL5F0: goto _LL8;_TL5EE: _LL8: _T1B=
# 3325
Cyc_Tcutil_is_const_exp(eopt);if(!_T1B)goto _TL5F4;_T1A=Cyc_Absyn_thin_bounds_exp(eopt);goto _TL5F5;_TL5F4: _T1A=Cyc_Absyn_fat_bound_type;_TL5F5: b=_T1A;_LL5:;}goto _TL5EB;
# 3328
_TL5EA: b=Cyc_Absyn_thin_bounds_exp(eopt);_TL5EB: _TL5E9: _T1C=
Cyc_Absyn_atb_type(et,rgn,aqual,tq,b,zt,Cyc_Absyn_false_type);return _T1C;}_TL5E6: _T1D=t;
return _T1D;;}
# 3334
static int Cyc_Tcutil_aquals_bnds_pred(struct _tuple15*bnd){struct _tuple15*_T0;struct _tuple15 _T1;int*_T2;int _T3;struct Cyc_Absyn_AppType_Absyn_Type_struct*_T4;void*_T5;int*_T6;int _T7;_T0=bnd;_T1=*_T0;{
void*_T8=_T1.f0;_T2=(int*)_T8;_T3=*_T2;if(_T3!=0)goto _TL5F6;_T4=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T8;_T5=_T4->f1;_T6=(int*)_T5;_T7=*_T6;if(_T7!=15)goto _TL5F8;
# 3337
return 1;_TL5F8: goto _LL3;_TL5F6: _LL3:
# 3339
 return 0;;}}
# 3343
struct Cyc_List_List*Cyc_Tcutil_get_aquals_bounds(struct Cyc_Absyn_Fndecl*fd){struct Cyc_Absyn_Fndecl*_T0;struct Cyc_Absyn_FnInfo _T1;struct Cyc_List_List*_T2;_T0=fd;_T1=_T0->i;_T2=_T1.qual_bnd;
# 3345
return _T2;}
# 3349
void*Cyc_Tcutil_eval_aquals_of(struct Cyc_List_List*aqb,void*t){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_PtrInfo _T4;struct Cyc_Absyn_PtrAtts _T5;void*_T6;void*_T7;void*_T8;_T0=t;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 4: _T3=t;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T9=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T3;_T4=_T9->f1;_T5=_T4.ptr_atts;_T8=_T5.aqual;}{void*aq=_T8;_T6=aq;
# 3352
return _T6;}case 2: _T7=
# 3354
Cyc_Tcutil_lookup_aquals(aqb,t);return _T7;default:
# 3356
 return 0;};}struct _tuple29{struct Cyc_Absyn_Tvar*f0;int f1;};
# 3360
struct _tuple29*Cyc_Tcutil_tvar_bool_pair(int b,struct Cyc_Absyn_Tvar*t){struct _tuple29*_T0;{struct _tuple29*_T1=_cycalloc(sizeof(struct _tuple29));
_T1->f0=t;_T1->f1=b;_T0=(struct _tuple29*)_T1;}return _T0;}
# 3367
static void Cyc_Tcutil_visit_for_cvars(void*t,void(*fn)(void**,void*),void*env){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_PtrInfo _T4;void*_T5;struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T6;void**_T7;void*_T8;void*_T9;struct Cyc_Absyn_FnType_Absyn_Type_struct*_TA;struct Cyc_Absyn_FnInfo*_TB;struct Cyc_Absyn_FnInfo*_TC;void*_TD;void(*_TE)(void**,void*);void*_TF;struct Cyc_Absyn_FnInfo*_T10;struct Cyc_List_List*_T11;unsigned long _T12;struct Cyc_List_List*_T13;void*_T14;struct _tuple9*_T15;void*_T16;void(*_T17)(void**,void*);void*_T18;struct Cyc_List_List*_T19;void*_T1A;void*_T1B;struct Cyc_List_List*_T1C;unsigned long _T1D;struct Cyc_List_List*_T1E;void*_T1F;void(*_T20)(void**,void*);void*_T21;struct Cyc_List_List*_T22;void*_T23;struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T24;int _T25;void*_T26;struct Cyc_List_List*_T27;unsigned long _T28;struct Cyc_List_List*_T29;void*_T2A;struct Cyc_Absyn_Aggrfield*_T2B;void*_T2C;void(*_T2D)(void**,void*);void*_T2E;struct Cyc_List_List*_T2F;struct Cyc_List_List*_T30;void*_T31;void*_T32;_T0=t;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 4: _T3=t;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_T33=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T3;_T4=_T33->f1;_T32=_T4.elt_type;_T5=t;_T6=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T5;_T7=& _T6->f1.ptr_atts.bounds;_T31=(void**)_T7;}{void*ta=_T32;void**b=(void**)_T31;
# 3370
fn(b,env);
Cyc_Tcutil_visit_for_cvars(ta,fn,env);
return;}case 6: _T8=t;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_T33=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T8;_T9=t;_TA=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T9;_TB=& _TA->f1;_T32=(struct Cyc_Absyn_FnInfo*)_TB;}{struct Cyc_Absyn_FnInfo*i=(struct Cyc_Absyn_FnInfo*)_T32;_TC=i;_TD=_TC->ret_type;_TE=fn;_TF=env;
# 3374
Cyc_Tcutil_visit_for_cvars(_TD,_TE,_TF);_T10=i;{
struct Cyc_List_List*args=_T10->args;_TL5FF: _T11=args;_T12=(unsigned long)_T11;if(_T12)goto _TL5FD;else{goto _TL5FE;}
_TL5FD: _T13=args;_T14=_T13->hd;_T15=(struct _tuple9*)_T14;_T16=_T15->f2;_T17=fn;_T18=env;Cyc_Tcutil_visit_for_cvars(_T16,_T17,_T18);_T19=args;
# 3375
args=_T19->tl;goto _TL5FF;_TL5FE:;}
# 3378
return;}case 0: _T1A=t;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_T33=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T1A;_T30=_T33->f2;}{struct Cyc_List_List*ts=_T30;_T30=ts;goto _LL8;}case 8: _T1B=t;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_T33=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T1B;_T30=_T33->f2;}_LL8: {struct Cyc_List_List*ts=_T30;
# 3381
_TL603: _T1C=ts;_T1D=(unsigned long)_T1C;if(_T1D)goto _TL601;else{goto _TL602;}_TL601: _T1E=ts;_T1F=_T1E->hd;_T20=fn;_T21=env;Cyc_Tcutil_visit_for_cvars(_T1F,_T20,_T21);_T22=ts;ts=_T22->tl;goto _TL603;_TL602:
 return;}case 7: _T23=t;_T24=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T23;_T25=_T24->f2;if(_T25!=1)goto _TL604;_T26=t;{struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_T33=(struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_T26;_T30=_T33->f3;}{struct Cyc_List_List*fields=_T30;
# 3384
_TL609: _T27=fields;_T28=(unsigned long)_T27;if(_T28)goto _TL607;else{goto _TL608;}
_TL607: _T29=fields;_T2A=_T29->hd;_T2B=(struct Cyc_Absyn_Aggrfield*)_T2A;_T2C=_T2B->type;_T2D=fn;_T2E=env;Cyc_Tcutil_visit_for_cvars(_T2C,_T2D,_T2E);_T2F=fields;
# 3384
fields=_T2F->tl;goto _TL609;_TL608:
# 3387
 return;}_TL604: goto _LLB;default: _LLB:
# 3389
 return;};}
# 3393
static void Cyc_Tcutil_introduce_cvar_fn(void**b,void*env){void**_T0;void*_T1;void*_T2;void**_T3;struct Cyc_Core_Opt*_T4;struct Cyc_Core_Opt*_T5;_T0=
_check_null(b);_T1=*_T0;_T2=env;if(_T1!=_T2)goto _TL60A;_T3=b;_T4=& Cyc_Kinds_ptrbko;_T5=(struct Cyc_Core_Opt*)_T4;
*_T3=Cyc_Absyn_cvar_type(_T5);goto _TL60B;_TL60A: _TL60B:;}
# 3399
static void Cyc_Tcutil_replace_cvar_fn(void**b,void*env){void**_T0;void*_T1;int _T2;void**_T3;_T0=
_check_null(b);_T1=*_T0;_T2=Cyc_Tcutil_is_cvar_type(_T1);if(!_T2)goto _TL60C;_T3=b;
*_T3=env;goto _TL60D;_TL60C: _TL60D:;}
# 3405
static void Cyc_Tcutil_revert_one_cvar(void**b,int ignore){void**_T0;int*_T1;int _T2;int _T3;void**_T4;void**_T5;_T0=
_check_null(b);{void*_T6=*_T0;int _T7;_T1=(int*)_T6;_T2=*_T1;if(_T2!=3)goto _TL60E;{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_T8=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T6;_T7=_T8->f7;}{int ovfat=_T7;_T3=ovfat;
# 3408
if(_T3)goto _TL610;else{goto _TL612;}
_TL612: _T4=b;*_T4=Cyc_Absyn_bounds_one();goto _TL611;
# 3412
_TL610: _T5=b;*_T5=Cyc_Absyn_fat_bound_type;_TL611: goto _LL0;}_TL60E: goto _LL0;_LL0:;}}
# 3420
void Cyc_Tcutil_revert_cvars(void*t){void(*_T0)(void*,void(*)(void**,int),int);{
void(*_T1)(void*,void(*)(void**,int),int)=(void(*)(void*,void(*)(void**,int),int))Cyc_Tcutil_visit_for_cvars;_T0=_T1;}_T0(t,Cyc_Tcutil_revert_one_cvar,0);}
# 3424
void Cyc_Tcutil_introduce_cvars(void*t){void*_T0;void*_T1;_T0=t;_T1=
Cyc_Absyn_bounds_one();Cyc_Tcutil_visit_for_cvars(_T0,Cyc_Tcutil_introduce_cvar_fn,_T1);}
# 3428
void Cyc_Tcutil_replace_cvars(void*t,void*with){
Cyc_Tcutil_visit_for_cvars(t,Cyc_Tcutil_replace_cvar_fn,with);}
# 3433
int Cyc_Tcutil_is_main(struct _tuple1*n){struct _tuple1*_T0;union Cyc_Absyn_Nmspace _T1;struct _union_Nmspace_Abs_n _T2;unsigned _T3;union Cyc_Absyn_Nmspace _T4;struct _union_Nmspace_Abs_n _T5;struct Cyc_List_List*_T6;struct _fat_ptr*_T7;struct _fat_ptr _T8;struct _fat_ptr _T9;int _TA;int _TB;struct _fat_ptr*_TC;union Cyc_Absyn_Nmspace _TD;_T0=n;{struct _tuple1 _TE=*_T0;_TD=_TE.f0;_TC=_TE.f1;}{union Cyc_Absyn_Nmspace nms=_TD;struct _fat_ptr*v=_TC;_T1=nms;_T2=_T1.Abs_n;_T3=_T2.tag;if(_T3!=1)goto _TL613;_T4=nms;_T5=_T4.Abs_n;_T6=_T5.val;if(_T6!=0)goto _TL615;_T7=v;_T8=*_T7;_T9=
# 3436
_tag_fat("main",sizeof(char),5U);_TA=Cyc_strcmp(_T8,_T9);_TB=_TA==0;return _TB;_TL615: goto _LL6;_TL613: _LL6:
 return 0;;}}
# 3442
static void Cyc_Tcutil_deconstruct_tycon(void*tc){void*_T0;int*_T1;unsigned _T2;struct Cyc___cycFILE*_T3;struct _fat_ptr _T4;struct _fat_ptr _T5;struct Cyc___cycFILE*_T6;struct _fat_ptr _T7;struct _fat_ptr _T8;struct Cyc___cycFILE*_T9;struct _fat_ptr _TA;struct _fat_ptr _TB;struct Cyc___cycFILE*_TC;struct _fat_ptr _TD;struct _fat_ptr _TE;struct Cyc___cycFILE*_TF;struct _fat_ptr _T10;struct _fat_ptr _T11;struct Cyc___cycFILE*_T12;struct _fat_ptr _T13;struct _fat_ptr _T14;struct Cyc___cycFILE*_T15;struct _fat_ptr _T16;struct _fat_ptr _T17;struct Cyc___cycFILE*_T18;struct _fat_ptr _T19;struct _fat_ptr _T1A;struct Cyc___cycFILE*_T1B;struct _fat_ptr _T1C;struct _fat_ptr _T1D;struct Cyc___cycFILE*_T1E;struct _fat_ptr _T1F;struct _fat_ptr _T20;struct Cyc___cycFILE*_T21;struct _fat_ptr _T22;struct _fat_ptr _T23;struct Cyc___cycFILE*_T24;struct _fat_ptr _T25;struct _fat_ptr _T26;struct Cyc___cycFILE*_T27;struct _fat_ptr _T28;struct _fat_ptr _T29;struct Cyc___cycFILE*_T2A;struct _fat_ptr _T2B;struct _fat_ptr _T2C;struct Cyc___cycFILE*_T2D;struct _fat_ptr _T2E;struct _fat_ptr _T2F;struct Cyc___cycFILE*_T30;struct _fat_ptr _T31;struct _fat_ptr _T32;struct Cyc___cycFILE*_T33;struct _fat_ptr _T34;struct _fat_ptr _T35;struct Cyc___cycFILE*_T36;struct _fat_ptr _T37;struct _fat_ptr _T38;struct Cyc___cycFILE*_T39;struct _fat_ptr _T3A;struct _fat_ptr _T3B;struct Cyc___cycFILE*_T3C;struct _fat_ptr _T3D;struct _fat_ptr _T3E;struct Cyc___cycFILE*_T3F;struct _fat_ptr _T40;struct _fat_ptr _T41;struct Cyc___cycFILE*_T42;struct _fat_ptr _T43;struct _fat_ptr _T44;struct Cyc___cycFILE*_T45;struct _fat_ptr _T46;struct _fat_ptr _T47;struct Cyc___cycFILE*_T48;struct _fat_ptr _T49;struct _fat_ptr _T4A;struct Cyc___cycFILE*_T4B;struct _fat_ptr _T4C;struct _fat_ptr _T4D;_T0=tc;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 3: _T3=Cyc_stderr;_T4=
# 3444
_tag_fat("ComplexCon",sizeof(char),11U);_T5=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T3,_T4,_T5);goto _LL0;case 0: _T6=Cyc_stderr;_T7=
_tag_fat("VoidCon",sizeof(char),8U);_T8=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T6,_T7,_T8);goto _LL0;case 1: _T9=Cyc_stderr;_TA=
_tag_fat("IntCon",sizeof(char),7U);_TB=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T9,_TA,_TB);goto _LL0;case 2: _TC=Cyc_stderr;_TD=
_tag_fat("FloatCon",sizeof(char),9U);_TE=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_TC,_TD,_TE);goto _LL0;case 4: _TF=Cyc_stderr;_T10=
_tag_fat("RgnHandleCon",sizeof(char),13U);_T11=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_TF,_T10,_T11);goto _LL0;case 18: _T12=Cyc_stderr;_T13=
_tag_fat("AqualHandleCon",sizeof(char),15U);_T14=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T12,_T13,_T14);goto _LL0;case 5: _T15=Cyc_stderr;_T16=
_tag_fat("TagCon",sizeof(char),7U);_T17=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T15,_T16,_T17);goto _LL0;case 6: _T18=Cyc_stderr;_T19=
_tag_fat("HeapCon",sizeof(char),8U);_T1A=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T18,_T19,_T1A);goto _LL0;case 7: _T1B=Cyc_stderr;_T1C=
_tag_fat("UniqueCon",sizeof(char),10U);_T1D=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T1B,_T1C,_T1D);goto _LL0;case 8: _T1E=Cyc_stderr;_T1F=
_tag_fat("RefCntCon",sizeof(char),10U);_T20=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T1E,_T1F,_T20);goto _LL0;case 9: _T21=Cyc_stderr;_T22=
_tag_fat("JoinCon",sizeof(char),8U);_T23=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T21,_T22,_T23);goto _LL0;case 10: _T24=Cyc_stderr;_T25=
_tag_fat("RgnsCon",sizeof(char),8U);_T26=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T24,_T25,_T26);goto _LL0;case 11: _T27=Cyc_stderr;_T28=
_tag_fat("TrueCon",sizeof(char),8U);_T29=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T27,_T28,_T29);goto _LL0;case 12: _T2A=Cyc_stderr;_T2B=
_tag_fat("FalseCon",sizeof(char),9U);_T2C=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T2A,_T2B,_T2C);goto _LL0;case 13: _T2D=Cyc_stderr;_T2E=
_tag_fat("ThinCon",sizeof(char),8U);_T2F=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T2D,_T2E,_T2F);goto _LL0;case 14: _T30=Cyc_stderr;_T31=
_tag_fat("FatCon",sizeof(char),7U);_T32=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T30,_T31,_T32);goto _LL0;case 19: _T33=Cyc_stderr;_T34=
_tag_fat("EnumCon",sizeof(char),8U);_T35=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T33,_T34,_T35);goto _LL0;case 20: _T36=Cyc_stderr;_T37=
_tag_fat("AnonEnumCon",sizeof(char),12U);_T38=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T36,_T37,_T38);goto _LL0;case 21: _T39=Cyc_stderr;_T3A=
_tag_fat("BuiltinCon",sizeof(char),11U);_T3B=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T39,_T3A,_T3B);goto _LL0;case 22: _T3C=Cyc_stderr;_T3D=
_tag_fat("DatatypeCon",sizeof(char),12U);_T3E=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T3C,_T3D,_T3E);goto _LL0;case 23: _T3F=Cyc_stderr;_T40=
_tag_fat("DatatypeFieldCon",sizeof(char),17U);_T41=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T3F,_T40,_T41);goto _LL0;case 24: _T42=Cyc_stderr;_T43=
_tag_fat("AggrCon",sizeof(char),8U);_T44=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T42,_T43,_T44);goto _LL0;case 16: _T45=Cyc_stderr;_T46=
_tag_fat("AqualsConstCon",sizeof(char),15U);_T47=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T45,_T46,_T47);goto _LL0;case 15: _T48=Cyc_stderr;_T49=
_tag_fat("AqualsCon",sizeof(char),10U);_T4A=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T48,_T49,_T4A);goto _LL0;default: _T4B=Cyc_stderr;_T4C=
_tag_fat("AqualVarCon",sizeof(char),12U);_T4D=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T4B,_T4C,_T4D);goto _LL0;}_LL0:;}
# 3472
void Cyc_Tcutil_deconstruct_type(void*t){struct Cyc_Int_pa_PrintArg_struct _T0;void*_T1;struct Cyc_String_pa_PrintArg_struct _T2;struct Cyc___cycFILE*_T3;struct _fat_ptr _T4;struct _fat_ptr _T5;void*_T6;int*_T7;unsigned _T8;void*_T9;void*_TA;struct Cyc___cycFILE*_TB;struct _fat_ptr _TC;struct _fat_ptr _TD;struct Cyc___cycFILE*_TE;struct _fat_ptr _TF;struct _fat_ptr _T10;struct Cyc_List_List*_T11;unsigned long _T12;struct Cyc_List_List*_T13;void*_T14;struct Cyc_List_List*_T15;struct Cyc___cycFILE*_T16;struct _fat_ptr _T17;struct _fat_ptr _T18;void*_T19;void*_T1A;struct Cyc_Int_pa_PrintArg_struct _T1B;int _T1C;struct Cyc_String_pa_PrintArg_struct _T1D;struct _fat_ptr _T1E;void*_T1F;void*_T20;unsigned long _T21;struct Cyc_String_pa_PrintArg_struct _T22;struct _fat_ptr _T23;void*_T24;void*_T25;unsigned long _T26;struct Cyc___cycFILE*_T27;struct _fat_ptr _T28;struct _fat_ptr _T29;void*_T2A;unsigned long _T2B;struct Cyc___cycFILE*_T2C;struct _fat_ptr _T2D;struct _fat_ptr _T2E;struct Cyc___cycFILE*_T2F;struct _fat_ptr _T30;struct _fat_ptr _T31;void*_T32;void*_T33;struct Cyc___cycFILE*_T34;struct _fat_ptr _T35;struct _fat_ptr _T36;void*_T37;unsigned long _T38;struct Cyc___cycFILE*_T39;struct _fat_ptr _T3A;struct _fat_ptr _T3B;struct Cyc___cycFILE*_T3C;struct _fat_ptr _T3D;struct _fat_ptr _T3E;void*_T3F;struct Cyc_String_pa_PrintArg_struct _T40;struct Cyc_Absyn_Tvar*_T41;void*_T42;struct Cyc_Int_pa_PrintArg_struct _T43;struct Cyc_Absyn_Tvar*_T44;int _T45;struct Cyc___cycFILE*_T46;struct _fat_ptr _T47;struct _fat_ptr _T48;struct Cyc_Absyn_Tvar*_T49;void*_T4A;unsigned long _T4B;struct Cyc_String_pa_PrintArg_struct _T4C;struct Cyc_Absyn_Tvar*_T4D;void*_T4E;struct Cyc___cycFILE*_T4F;struct _fat_ptr _T50;struct _fat_ptr _T51;struct Cyc___cycFILE*_T52;struct _fat_ptr _T53;struct _fat_ptr _T54;void*_T55;struct Cyc_Absyn_PtrInfo _T56;struct Cyc_Absyn_PtrInfo _T57;struct Cyc_Absyn_PtrAtts _T58;struct Cyc_Absyn_PtrInfo _T59;struct Cyc_Absyn_PtrAtts _T5A;struct Cyc_Absyn_PtrInfo _T5B;struct Cyc_Absyn_PtrAtts _T5C;struct Cyc_String_pa_PrintArg_struct _T5D;struct Cyc___cycFILE*_T5E;struct _fat_ptr _T5F;struct _fat_ptr _T60;struct Cyc_String_pa_PrintArg_struct _T61;struct Cyc___cycFILE*_T62;struct _fat_ptr _T63;struct _fat_ptr _T64;struct Cyc___cycFILE*_T65;struct _fat_ptr _T66;struct _fat_ptr _T67;struct Cyc___cycFILE*_T68;struct _fat_ptr _T69;struct _fat_ptr _T6A;void*_T6B;void*_T6C;struct Cyc_Absyn_FnType_Absyn_Type_struct*_T6D;struct Cyc_Absyn_FnInfo*_T6E;struct Cyc___cycFILE*_T6F;struct _fat_ptr _T70;struct _fat_ptr _T71;struct Cyc___cycFILE*_T72;struct _fat_ptr _T73;struct _fat_ptr _T74;struct Cyc_Absyn_FnInfo*_T75;void*_T76;struct Cyc_Absyn_FnInfo*_T77;struct Cyc_List_List*_T78;void*_T79;struct Cyc___cycFILE*_T7A;struct _fat_ptr _T7B;struct _fat_ptr _T7C;struct Cyc_List_List*_T7D;struct Cyc_Absyn_FnInfo*_T7E;struct Cyc_List_List*_T7F;void*_T80;struct Cyc___cycFILE*_T81;struct _fat_ptr _T82;struct _fat_ptr _T83;struct Cyc___cycFILE*_T84;struct _fat_ptr _T85;struct _fat_ptr _T86;struct Cyc_List_List*_T87;struct Cyc___cycFILE*_T88;struct _fat_ptr _T89;struct _fat_ptr _T8A;struct Cyc_Absyn_FnInfo*_T8B;void*_T8C;unsigned long _T8D;struct Cyc_Absyn_FnInfo*_T8E;void*_T8F;struct Cyc___cycFILE*_T90;struct _fat_ptr _T91;struct _fat_ptr _T92;struct Cyc___cycFILE*_T93;struct _fat_ptr _T94;struct _fat_ptr _T95;void*_T96;void*_T97;struct Cyc___cycFILE*_T98;struct _fat_ptr _T99;struct _fat_ptr _T9A;void*_T9B;unsigned long _T9C;struct Cyc___cycFILE*_T9D;struct _fat_ptr _T9E;struct _fat_ptr _T9F;struct Cyc___cycFILE*_TA0;struct _fat_ptr _TA1;struct _fat_ptr _TA2;struct Cyc___cycFILE*_TA3;struct _fat_ptr _TA4;struct _fat_ptr _TA5;void*_TA6;struct Cyc___cycFILE*_TA7;struct _fat_ptr _TA8;struct _fat_ptr _TA9;{struct Cyc_Int_pa_PrintArg_struct _TAA;_TAA.tag=1;_T1=t;
_TAA.f1=(unsigned)_T1;_T0=_TAA;}{struct Cyc_Int_pa_PrintArg_struct _TAA=_T0;{struct Cyc_String_pa_PrintArg_struct _TAB;_TAB.tag=0;_TAB.f1=Cyc_Absynpp_typ2string(t);_T2=_TAB;}{struct Cyc_String_pa_PrintArg_struct _TAB=_T2;void*_TAC[2];_TAC[0]=& _TAA;_TAC[1]=& _TAB;_T3=Cyc_stderr;_T4=_tag_fat("Type 0x%x %s is ...",sizeof(char),20U);_T5=_tag_fat(_TAC,sizeof(void*),2);Cyc_fprintf(_T3,_T4,_T5);}}{struct Cyc_AssnDef_ExistAssnFn*_TAA;struct Cyc_Absyn_Exp*_TAB;struct Cyc_Absyn_Vardecl*_TAC;struct Cyc_Absyn_Typedefdecl*_TAD;struct Cyc_List_List*_TAE;struct _tuple1*_TAF;void*_TB0;void*_TB1;const char*_TB2;const char*_TB3;int _TB4;void*_TB5;void*_TB6;_T6=t;_T7=(int*)_T6;_T8=*_T7;switch(_T8){case 0: _T9=t;{struct Cyc_Absyn_AppType_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_AppType_Absyn_Type_struct*)_T9;_TA=_TB7->f1;_TB6=(void*)_TA;_TB5=_TB7->f2;}{void*tc=_TB6;struct Cyc_List_List*ts=_TB5;_TB=Cyc_stderr;_TC=
# 3476
_tag_fat(" AppType with constructor ",sizeof(char),27U);_TD=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_TB,_TC,_TD);Cyc_Tcutil_deconstruct_tycon(tc);_TE=Cyc_stderr;_TF=
_tag_fat("and args <",sizeof(char),11U);_T10=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_TE,_TF,_T10);
_TL619: _T11=ts;_T12=(unsigned long)_T11;if(_T12)goto _TL61A;else{goto _TL61B;}
_TL61A: _T13=ts;_T14=_T13->hd;Cyc_Tcutil_deconstruct_type(_T14);_T15=ts;
ts=_T15->tl;goto _TL619;_TL61B: _T16=Cyc_stderr;_T17=
# 3482
_tag_fat(">\n",sizeof(char),3U);_T18=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T16,_T17,_T18);goto _LL0;}case 3: _T19=t;{struct Cyc_Absyn_Cvar_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_Cvar_Absyn_Type_struct*)_T19;_T1A=_TB7->f2;_TB6=(void*)_T1A;_TB4=_TB7->f3;_TB3=_TB7->f5;_TB2=_TB7->f6;}{void*r=_TB6;int id=_TB4;const char*name=_TB3;const char*pos=_TB2;{struct Cyc_Int_pa_PrintArg_struct _TB7;_TB7.tag=1;_T1C=id;
# 3485
_TB7.f1=(unsigned long)_T1C;_T1B=_TB7;}{struct Cyc_Int_pa_PrintArg_struct _TB7=_T1B;{struct Cyc_String_pa_PrintArg_struct _TB8;_TB8.tag=0;{const char*_TB9=name;_T1F=(void*)_TB9;_T20=(void*)_TB9;_T21=_get_zero_arr_size_char(_T20,1U);_T1E=_tag_fat(_T1F,sizeof(char),_T21);}_TB8.f1=_T1E;_T1D=_TB8;}{struct Cyc_String_pa_PrintArg_struct _TB8=_T1D;{struct Cyc_String_pa_PrintArg_struct _TB9;_TB9.tag=0;{const char*_TBA=pos;_T24=(void*)_TBA;_T25=(void*)_TBA;_T26=_get_zero_arr_size_char(_T25,1U);_T23=_tag_fat(_T24,sizeof(char),_T26);}_TB9.f1=_T23;_T22=_TB9;}{struct Cyc_String_pa_PrintArg_struct _TB9=_T22;void*_TBA[3];_TBA[0]=& _TB7;_TBA[1]=& _TB8;_TBA[2]=& _TB9;_T27=Cyc_stderr;_T28=_tag_fat("Got Cvar id=%d, name=%s pos=%s",sizeof(char),31U);_T29=_tag_fat(_TBA,sizeof(void*),3);Cyc_fprintf(_T27,_T28,_T29);}}}_T2A=r;_T2B=(unsigned long)_T2A;
if(!_T2B)goto _TL61C;_T2C=Cyc_stderr;_T2D=
_tag_fat(" resolved to = ",sizeof(char),16U);_T2E=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T2C,_T2D,_T2E);
Cyc_Tcutil_deconstruct_type(r);goto _TL61D;_TL61C: _TL61D: _T2F=Cyc_stderr;_T30=
# 3490
_tag_fat("\n",sizeof(char),2U);_T31=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T2F,_T30,_T31);goto _LL0;}case 1: _T32=t;{struct Cyc_Absyn_Evar_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_Evar_Absyn_Type_struct*)_T32;_T33=_TB7->f2;_TB6=(void*)_T33;}{void*r=_TB6;_T34=Cyc_stderr;_T35=
# 3493
_tag_fat("Got Evar",sizeof(char),9U);_T36=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T34,_T35,_T36);_T37=r;_T38=(unsigned long)_T37;
if(!_T38)goto _TL61E;_T39=Cyc_stderr;_T3A=
_tag_fat(" with compression = ",sizeof(char),21U);_T3B=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T39,_T3A,_T3B);
Cyc_Tcutil_deconstruct_type(r);goto _TL61F;_TL61E: _TL61F: _T3C=Cyc_stderr;_T3D=
# 3498
_tag_fat("\n",sizeof(char),2U);_T3E=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T3C,_T3D,_T3E);goto _LL0;}case 2: _T3F=t;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_T3F;_TB6=_TB7->f1;}{struct Cyc_Absyn_Tvar*tv=_TB6;{struct Cyc_String_pa_PrintArg_struct _TB7;_TB7.tag=0;_T41=tv;_T42=_T41->kind;
# 3501
_TB7.f1=Cyc_Absynpp_kindbound2string(_T42);_T40=_TB7;}{struct Cyc_String_pa_PrintArg_struct _TB7=_T40;{struct Cyc_Int_pa_PrintArg_struct _TB8;_TB8.tag=1;_T44=tv;_T45=_T44->identity;_TB8.f1=(unsigned long)_T45;_T43=_TB8;}{struct Cyc_Int_pa_PrintArg_struct _TB8=_T43;void*_TB9[2];_TB9[0]=& _TB7;_TB9[1]=& _TB8;_T46=Cyc_stderr;_T47=_tag_fat("Got VarType -- kindbnd is %s, id is %d",sizeof(char),39U);_T48=_tag_fat(_TB9,sizeof(void*),2);Cyc_fprintf(_T46,_T47,_T48);}}_T49=tv;_T4A=_T49->aquals_bound;_T4B=(unsigned long)_T4A;
if(!_T4B)goto _TL620;{struct Cyc_String_pa_PrintArg_struct _TB7;_TB7.tag=0;_T4D=tv;_T4E=_T4D->aquals_bound;
_TB7.f1=Cyc_Absynpp_typ2string(_T4E);_T4C=_TB7;}{struct Cyc_String_pa_PrintArg_struct _TB7=_T4C;void*_TB8[1];_TB8[0]=& _TB7;_T4F=Cyc_stderr;_T50=_tag_fat(" AQbnd = %s",sizeof(char),12U);_T51=_tag_fat(_TB8,sizeof(void*),1);Cyc_fprintf(_T4F,_T50,_T51);}goto _TL621;_TL620: _TL621: _T52=Cyc_stderr;_T53=
_tag_fat("\n",sizeof(char),2U);_T54=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T52,_T53,_T54);goto _LL0;}case 4: _T55=t;{struct Cyc_Absyn_PointerType_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_T55;_T56=_TB7->f1;_TB6=_T56.elt_type;_T57=_TB7->f1;_T58=_T57.ptr_atts;_TB5=_T58.eff;_T59=_TB7->f1;_T5A=_T59.ptr_atts;_TB1=_T5A.nullable;_T5B=_TB7->f1;_T5C=_T5B.ptr_atts;_TB0=_T5C.aqual;}{void*t1=_TB6;void*rgn=_TB5;void*nlbl=_TB1;void*aq=_TB0;{struct Cyc_String_pa_PrintArg_struct _TB7;_TB7.tag=0;
# 3507
_TB7.f1=Cyc_Absynpp_typ2string(t);_T5D=_TB7;}{struct Cyc_String_pa_PrintArg_struct _TB7=_T5D;void*_TB8[1];_TB8[0]=& _TB7;_T5E=Cyc_stderr;_T5F=_tag_fat("Got PointerType(%s) ... alias qualifier is \n",sizeof(char),45U);_T60=_tag_fat(_TB8,sizeof(void*),1);Cyc_fprintf(_T5E,_T5F,_T60);}
Cyc_Tcutil_deconstruct_type(aq);{struct Cyc_String_pa_PrintArg_struct _TB7;_TB7.tag=0;
_TB7.f1=Cyc_Absynpp_typ2string(t);_T61=_TB7;}{struct Cyc_String_pa_PrintArg_struct _TB7=_T61;void*_TB8[1];_TB8[0]=& _TB7;_T62=Cyc_stderr;_T63=_tag_fat("Got PointerType(%s) ... region is \n",sizeof(char),36U);_T64=_tag_fat(_TB8,sizeof(void*),1);Cyc_fprintf(_T62,_T63,_T64);}
Cyc_Tcutil_deconstruct_type(rgn);_T65=Cyc_stderr;_T66=
_tag_fat("*******************************Element Type***************************\n",sizeof(char),72U);_T67=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T65,_T66,_T67);
Cyc_Tcutil_deconstruct_type(t1);goto _LL0;}case 5: _T68=Cyc_stderr;_T69=
# 3515
_tag_fat("Got ArrayType\n",sizeof(char),15U);_T6A=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T68,_T69,_T6A);goto _LL0;case 6: _T6B=t;{struct Cyc_Absyn_FnType_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T6B;_T6C=t;_T6D=(struct Cyc_Absyn_FnType_Absyn_Type_struct*)_T6C;_T6E=& _T6D->f1;_TB6=(struct Cyc_Absyn_FnInfo*)_T6E;}{struct Cyc_Absyn_FnInfo*i=_TB6;_T6F=Cyc_stderr;_T70=
# 3517
_tag_fat("Got FnType\n",sizeof(char),12U);_T71=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T6F,_T70,_T71);_T72=Cyc_stderr;_T73=
_tag_fat("ReturnType:",sizeof(char),12U);_T74=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T72,_T73,_T74);_T75=i;_T76=_T75->ret_type;
Cyc_Tcutil_deconstruct_type(_T76);_T77=i;{
struct Cyc_List_List*it=_T77->args;_TL625: if(it!=0)goto _TL623;else{goto _TL624;}
_TL623: _T78=it;_T79=_T78->hd;{struct _tuple9*_TB7=(struct _tuple9*)_T79;void*_TB8;{struct _tuple9 _TB9=*_TB7;_TB8=_TB9.f2;}{void*at=_TB8;_T7A=Cyc_stderr;_T7B=
_tag_fat("Argument:",sizeof(char),10U);_T7C=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T7A,_T7B,_T7C);
Cyc_Tcutil_deconstruct_type(at);}}_T7D=it;
# 3520
it=_T7D->tl;goto _TL625;_TL624:;}_T7E=i;{
# 3525
struct Cyc_List_List*qt=_T7E->qual_bnd;_TL629: if(qt!=0)goto _TL627;else{goto _TL628;}
_TL627: _T7F=qt;_T80=_T7F->hd;{struct _tuple15*_TB7=(struct _tuple15*)_T80;void*_TB8;void*_TB9;{struct _tuple15 _TBA=*_TB7;_TB9=_TBA.f0;_TB8=_TBA.f1;}{void*t1=_TB9;void*t2=_TB8;_T81=Cyc_stderr;_T82=
_tag_fat("Qualbnd:",sizeof(char),9U);_T83=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T81,_T82,_T83);
Cyc_Tcutil_deconstruct_type(t1);_T84=Cyc_stderr;_T85=
_tag_fat(" <= ",sizeof(char),5U);_T86=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T84,_T85,_T86);
Cyc_Tcutil_deconstruct_type(t2);}}_T87=qt;
# 3525
qt=_T87->tl;goto _TL629;_TL628:;}_T88=Cyc_stderr;_T89=
# 3532
_tag_fat("Effect: ",sizeof(char),9U);_T8A=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T88,_T89,_T8A);_T8B=i;_T8C=_T8B->effect;_T8D=(unsigned long)_T8C;
if(!_T8D)goto _TL62A;_T8E=i;_T8F=_T8E->effect;
Cyc_Tcutil_deconstruct_type(_T8F);goto _TL62B;
# 3536
_TL62A: _T90=Cyc_stderr;_T91=_tag_fat("NULL",sizeof(char),5U);_T92=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T90,_T91,_T92);_TL62B: goto _LL0;}case 7: _T93=Cyc_stderr;_T94=
# 3539
_tag_fat("Got AnonAggrType\n",sizeof(char),18U);_T95=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T93,_T94,_T95);goto _LL0;case 8: _T96=t;{struct Cyc_Absyn_TypedefType_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_T96;_TAF=_TB7->f1;_TAE=_TB7->f2;_TAD=_TB7->f3;_T97=_TB7->f4;_TB6=(void*)_T97;}{struct _tuple1*name=_TAF;struct Cyc_List_List*in=_TAE;struct Cyc_Absyn_Typedefdecl*d=_TAD;void*topt=_TB6;_T98=Cyc_stderr;_T99=
# 3541
_tag_fat("Got TypedefType\n",sizeof(char),17U);_T9A=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T98,_T99,_T9A);_T9B=topt;_T9C=(unsigned long)_T9B;
if(!_T9C)goto _TL62C;
Cyc_Tcutil_deconstruct_type(topt);goto _TL62D;_TL62C: _TL62D: goto _LL0;}case 9: _T9D=Cyc_stderr;_T9E=
# 3546
_tag_fat("Got ValueofType\n",sizeof(char),17U);_T9F=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_T9D,_T9E,_T9F);goto _LL0;case 10: _TA0=Cyc_stderr;_TA1=
# 3548
_tag_fat("Got TypeDeclType\n",sizeof(char),18U);_TA2=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_TA0,_TA1,_TA2);goto _LL0;case 11: _TA3=Cyc_stderr;_TA4=
# 3550
_tag_fat("Got TypeofType\n",sizeof(char),16U);_TA5=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_TA3,_TA4,_TA5);goto _LL0;default: _TA6=t;{struct Cyc_Absyn_SubsetType_Absyn_Type_struct*_TB7=(struct Cyc_Absyn_SubsetType_Absyn_Type_struct*)_TA6;_TAC=_TB7->f1;_TAB=_TB7->f2;_TAA=_TB7->f3;}{struct Cyc_Absyn_Vardecl*vd=_TAC;struct Cyc_Absyn_Exp*e=_TAB;struct Cyc_AssnDef_ExistAssnFn*a=_TAA;_TA7=Cyc_stderr;_TA8=
# 3552
_tag_fat("Got SubsetType\n",sizeof(char),16U);_TA9=_tag_fat(0U,sizeof(void*),0);Cyc_fprintf(_TA7,_TA8,_TA9);goto _LL0;}}_LL0:;}}

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
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr);
# 181 "absyn.h"
enum Cyc_Absyn_AliasHint{Cyc_Absyn_UniqueHint =0U,Cyc_Absyn_RefcntHint =1U,Cyc_Absyn_RestrictedHint =2U,Cyc_Absyn_NoHint =3U};
# 187
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_EffKind =3U,Cyc_Absyn_IntKind =4U,Cyc_Absyn_BoolKind =5U,Cyc_Absyn_PtrBndKind =6U,Cyc_Absyn_AqualKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasHint aliashint;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;void*aquals_bound;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};
# 953 "absyn.h"
void*Cyc_Absyn_compress(void*);
# 982
extern void*Cyc_Absyn_un_qual_type;extern void*Cyc_Absyn_rc_qual_type;extern void*Cyc_Absyn_rtd_qual_type;
# 37 "warn.h"
void Cyc_Warn_warn(unsigned,struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Kind_Warn_Warg_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Warn_Int_Warn_Warg_struct{int tag;int f1;};
# 75
void Cyc_Warn_err2(unsigned,struct _fat_ptr);
# 77
void Cyc_Warn_warn2(unsigned,struct _fat_ptr);
# 79
void*Cyc_Warn_impos2(struct _fat_ptr);
# 29 "kinds.h"
extern struct Cyc_Absyn_Kind Cyc_Kinds_ak;
extern struct Cyc_Absyn_Kind Cyc_Kinds_bk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_mk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ek;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ik;
extern struct Cyc_Absyn_Kind Cyc_Kinds_boolk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_ptrbk;
extern struct Cyc_Absyn_Kind Cyc_Kinds_aqk;
# 53 "kinds.h"
extern struct Cyc_Core_Opt Cyc_Kinds_ako;
extern struct Cyc_Core_Opt Cyc_Kinds_bko;
extern struct Cyc_Core_Opt Cyc_Kinds_mko;
extern struct Cyc_Core_Opt Cyc_Kinds_iko;
extern struct Cyc_Core_Opt Cyc_Kinds_eko;
# 59
extern struct Cyc_Core_Opt Cyc_Kinds_ptrbko;
extern struct Cyc_Core_Opt Cyc_Kinds_aqko;
# 27 "kinds.cyc"
struct Cyc_Absyn_Kind Cyc_Kinds_ak={Cyc_Absyn_AnyKind,Cyc_Absyn_NoHint};
struct Cyc_Absyn_Kind Cyc_Kinds_bk={Cyc_Absyn_BoxKind,Cyc_Absyn_NoHint};
struct Cyc_Absyn_Kind Cyc_Kinds_mk={Cyc_Absyn_MemKind,Cyc_Absyn_NoHint};
struct Cyc_Absyn_Kind Cyc_Kinds_ik={Cyc_Absyn_IntKind,Cyc_Absyn_NoHint};
struct Cyc_Absyn_Kind Cyc_Kinds_ek={Cyc_Absyn_EffKind,Cyc_Absyn_NoHint};
struct Cyc_Absyn_Kind Cyc_Kinds_boolk={Cyc_Absyn_BoolKind,Cyc_Absyn_NoHint};
struct Cyc_Absyn_Kind Cyc_Kinds_ptrbk={Cyc_Absyn_PtrBndKind,Cyc_Absyn_NoHint};
struct Cyc_Absyn_Kind Cyc_Kinds_aqk={Cyc_Absyn_AqualKind,Cyc_Absyn_NoHint};
# 36
struct Cyc_Absyn_Kind Cyc_Kinds_bk_rhint={Cyc_Absyn_BoxKind,Cyc_Absyn_RestrictedHint};
struct Cyc_Absyn_Kind Cyc_Kinds_bk_uhint={Cyc_Absyn_BoxKind,Cyc_Absyn_UniqueHint};
struct Cyc_Absyn_Kind Cyc_Kinds_bk_rchint={Cyc_Absyn_BoxKind,Cyc_Absyn_RefcntHint};
# 40
struct Cyc_Absyn_Kind Cyc_Kinds_mk_rhint={Cyc_Absyn_MemKind,Cyc_Absyn_RestrictedHint};
struct Cyc_Absyn_Kind Cyc_Kinds_mk_uhint={Cyc_Absyn_MemKind,Cyc_Absyn_UniqueHint};
struct Cyc_Absyn_Kind Cyc_Kinds_mk_rchint={Cyc_Absyn_MemKind,Cyc_Absyn_RefcntHint};
# 44
struct Cyc_Absyn_Kind Cyc_Kinds_ak_rhint={Cyc_Absyn_AnyKind,Cyc_Absyn_RestrictedHint};
struct Cyc_Absyn_Kind Cyc_Kinds_ak_uhint={Cyc_Absyn_AnyKind,Cyc_Absyn_UniqueHint};
struct Cyc_Absyn_Kind Cyc_Kinds_ak_rchint={Cyc_Absyn_AnyKind,Cyc_Absyn_RefcntHint};
# 55 "kinds.cyc"
struct Cyc_Core_Opt Cyc_Kinds_ako={(void*)& Cyc_Kinds_ak};
struct Cyc_Core_Opt Cyc_Kinds_bko={(void*)& Cyc_Kinds_bk};
struct Cyc_Core_Opt Cyc_Kinds_mko={(void*)& Cyc_Kinds_mk};
struct Cyc_Core_Opt Cyc_Kinds_iko={(void*)& Cyc_Kinds_ik};
struct Cyc_Core_Opt Cyc_Kinds_eko={(void*)& Cyc_Kinds_ek};
struct Cyc_Core_Opt Cyc_Kinds_boolko={(void*)& Cyc_Kinds_boolk};
struct Cyc_Core_Opt Cyc_Kinds_ptrbko={(void*)& Cyc_Kinds_ptrbk};
struct Cyc_Core_Opt Cyc_Kinds_aqko={(void*)& Cyc_Kinds_aqk};
# 64
struct Cyc_Core_Opt Cyc_Kinds_bk_rhinto={(void*)& Cyc_Kinds_bk_rhint};
struct Cyc_Core_Opt Cyc_Kinds_bk_uhinto={(void*)& Cyc_Kinds_bk_uhint};
struct Cyc_Core_Opt Cyc_Kinds_bk_rchinto={(void*)& Cyc_Kinds_bk_rchint};
# 68
struct Cyc_Core_Opt Cyc_Kinds_mk_rhinto={(void*)& Cyc_Kinds_mk_rhint};
struct Cyc_Core_Opt Cyc_Kinds_mk_uhinto={(void*)& Cyc_Kinds_mk_uhint};
struct Cyc_Core_Opt Cyc_Kinds_mk_rchinto={(void*)& Cyc_Kinds_mk_rchint};
# 72
struct Cyc_Core_Opt Cyc_Kinds_ak_rhinto={(void*)& Cyc_Kinds_ak_rhint};
struct Cyc_Core_Opt Cyc_Kinds_ak_uhinto={(void*)& Cyc_Kinds_ak_uhint};
struct Cyc_Core_Opt Cyc_Kinds_ak_rchinto={(void*)& Cyc_Kinds_ak_rchint};
# 85 "kinds.cyc"
struct Cyc_Core_Opt*Cyc_Kinds_kind_to_opt(struct Cyc_Absyn_Kind*kh){struct Cyc_Absyn_Kind*_T0;enum Cyc_Absyn_AliasHint _T1;int _T2;enum Cyc_Absyn_KindQual _T3;int _T4;struct Cyc_Core_Opt*_T5;struct Cyc_Core_Opt*_T6;struct Cyc_Core_Opt*_T7;struct Cyc_Core_Opt*_T8;struct Cyc_Core_Opt*_T9;struct Cyc_Core_Opt*_TA;struct Cyc_Core_Opt*_TB;struct Cyc_Core_Opt*_TC;struct Cyc_Core_Opt*_TD;struct Cyc_Core_Opt*_TE;struct Cyc_Core_Opt*_TF;struct Cyc_Core_Opt*_T10;struct Cyc_Core_Opt*_T11;struct Cyc_Core_Opt*_T12;struct Cyc_Core_Opt*_T13;struct Cyc_Core_Opt*_T14;struct Cyc_Warn_String_Warn_Warg_struct _T15;struct Cyc_Warn_Kind_Warn_Warg_struct _T16;int(*_T17)(struct _fat_ptr);void*(*_T18)(struct _fat_ptr);struct _fat_ptr _T19;enum Cyc_Absyn_KindQual _T1A;int _T1B;enum Cyc_Absyn_AliasHint _T1C;int _T1D;struct Cyc_Core_Opt*_T1E;struct Cyc_Core_Opt*_T1F;struct Cyc_Core_Opt*_T20;struct Cyc_Core_Opt*_T21;struct Cyc_Core_Opt*_T22;struct Cyc_Core_Opt*_T23;enum Cyc_Absyn_AliasHint _T24;int _T25;struct Cyc_Core_Opt*_T26;struct Cyc_Core_Opt*_T27;struct Cyc_Core_Opt*_T28;struct Cyc_Core_Opt*_T29;struct Cyc_Core_Opt*_T2A;struct Cyc_Core_Opt*_T2B;enum Cyc_Absyn_AliasHint _T2C;int _T2D;struct Cyc_Core_Opt*_T2E;struct Cyc_Core_Opt*_T2F;struct Cyc_Core_Opt*_T30;struct Cyc_Core_Opt*_T31;struct Cyc_Core_Opt*_T32;struct Cyc_Core_Opt*_T33;struct Cyc_Warn_String_Warn_Warg_struct _T34;struct Cyc_Warn_Kind_Warn_Warg_struct _T35;int(*_T36)(struct _fat_ptr);void*(*_T37)(struct _fat_ptr);struct _fat_ptr _T38;enum Cyc_Absyn_AliasHint _T39;enum Cyc_Absyn_KindQual _T3A;_T0=kh;{struct Cyc_Absyn_Kind _T3B=*_T0;_T3A=_T3B.kind;_T39=_T3B.aliashint;}{enum Cyc_Absyn_KindQual k=_T3A;enum Cyc_Absyn_AliasHint h=_T39;_T1=h;_T2=(int)_T1;
# 87
if(_T2!=3)goto _TL0;_T3=k;_T4=(int)_T3;switch(_T4){case Cyc_Absyn_AnyKind: _T5=& Cyc_Kinds_ako;_T6=(struct Cyc_Core_Opt*)_T5;
# 89
return _T6;case Cyc_Absyn_MemKind: _T7=& Cyc_Kinds_mko;_T8=(struct Cyc_Core_Opt*)_T7;
return _T8;case Cyc_Absyn_BoxKind: _T9=& Cyc_Kinds_bko;_TA=(struct Cyc_Core_Opt*)_T9;
return _TA;case Cyc_Absyn_EffKind: _TB=& Cyc_Kinds_eko;_TC=(struct Cyc_Core_Opt*)_TB;
# 93
return _TC;case Cyc_Absyn_IntKind: _TD=& Cyc_Kinds_iko;_TE=(struct Cyc_Core_Opt*)_TD;
return _TE;case Cyc_Absyn_BoolKind: _TF=& Cyc_Kinds_bko;_T10=(struct Cyc_Core_Opt*)_TF;
return _T10;case Cyc_Absyn_AqualKind: _T11=& Cyc_Kinds_aqko;_T12=(struct Cyc_Core_Opt*)_T11;
return _T12;case Cyc_Absyn_PtrBndKind: _T13=& Cyc_Kinds_ptrbko;_T14=(struct Cyc_Core_Opt*)_T13;
return _T14;default:{struct Cyc_Warn_String_Warn_Warg_struct _T3B;_T3B.tag=0;
_T3B.f1=_tag_fat("kind_to_opt: bad kind ",sizeof(char),23U);_T15=_T3B;}{struct Cyc_Warn_String_Warn_Warg_struct _T3B=_T15;{struct Cyc_Warn_Kind_Warn_Warg_struct _T3C;_T3C.tag=9;_T3C.f1=kh;_T16=_T3C;}{struct Cyc_Warn_Kind_Warn_Warg_struct _T3C=_T16;void*_T3D[2];_T3D[0]=& _T3B;_T3D[1]=& _T3C;_T18=Cyc_Warn_impos2;{int(*_T3E)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T18;_T17=_T3E;}_T19=_tag_fat(_T3D,sizeof(void*),2);_T17(_T19);}}}goto _TL1;
# 102
_TL0: _T1A=k;_T1B=(int)_T1A;switch(_T1B){case Cyc_Absyn_BoxKind: _T1C=h;_T1D=(int)_T1C;switch(_T1D){case Cyc_Absyn_RestrictedHint: _T1E=& Cyc_Kinds_bk_rhinto;_T1F=(struct Cyc_Core_Opt*)_T1E;
# 106
return _T1F;case Cyc_Absyn_UniqueHint: _T20=& Cyc_Kinds_bk_uhinto;_T21=(struct Cyc_Core_Opt*)_T20;
# 108
return _T21;default: _T22=& Cyc_Kinds_bk_rchinto;_T23=(struct Cyc_Core_Opt*)_T22;
# 110
return _T23;};case Cyc_Absyn_MemKind: _T24=h;_T25=(int)_T24;switch(_T25){case Cyc_Absyn_RestrictedHint: _T26=& Cyc_Kinds_mk_rhinto;_T27=(struct Cyc_Core_Opt*)_T26;
# 115
return _T27;case Cyc_Absyn_UniqueHint: _T28=& Cyc_Kinds_mk_uhinto;_T29=(struct Cyc_Core_Opt*)_T28;
# 117
return _T29;default: _T2A=& Cyc_Kinds_mk_rchinto;_T2B=(struct Cyc_Core_Opt*)_T2A;
# 119
return _T2B;};case Cyc_Absyn_AnyKind: _T2C=h;_T2D=(int)_T2C;switch(_T2D){case Cyc_Absyn_RestrictedHint: _T2E=& Cyc_Kinds_ak_rhinto;_T2F=(struct Cyc_Core_Opt*)_T2E;
# 124
return _T2F;case Cyc_Absyn_UniqueHint: _T30=& Cyc_Kinds_ak_uhinto;_T31=(struct Cyc_Core_Opt*)_T30;
# 126
return _T31;default: _T32=& Cyc_Kinds_ak_rchinto;_T33=(struct Cyc_Core_Opt*)_T32;
# 128
return _T33;};default:{struct Cyc_Warn_String_Warn_Warg_struct _T3B;_T3B.tag=0;
# 131
_T3B.f1=_tag_fat("kind_to_opt: bad kind ",sizeof(char),23U);_T34=_T3B;}{struct Cyc_Warn_String_Warn_Warg_struct _T3B=_T34;{struct Cyc_Warn_Kind_Warn_Warg_struct _T3C;_T3C.tag=9;_T3C.f1=kh;_T35=_T3C;}{struct Cyc_Warn_Kind_Warn_Warg_struct _T3C=_T35;void*_T3D[2];_T3D[0]=& _T3B;_T3D[1]=& _T3C;_T37=Cyc_Warn_impos2;{int(*_T3E)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T37;_T36=_T3E;}_T38=_tag_fat(_T3D,sizeof(void*),2);_T36(_T38);}}}_TL1:;}}
# 136
void*Cyc_Kinds_kind_to_bound(struct Cyc_Absyn_Kind*k){struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T0;void*_T1;{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T2=_cycalloc(sizeof(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct));_T2->tag=0;
_T2->f1=k;_T0=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T2;}_T1=(void*)_T0;return _T1;}
# 139
struct Cyc_Core_Opt*Cyc_Kinds_kind_to_bound_opt(struct Cyc_Absyn_Kind*k){struct Cyc_Core_Opt*_T0;{struct Cyc_Core_Opt*_T1=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T1->v=Cyc_Kinds_kind_to_bound(k);_T0=(struct Cyc_Core_Opt*)_T1;}return _T0;}
# 144
struct Cyc_Absyn_Kind*Cyc_Kinds_id_to_kind(struct _fat_ptr s,unsigned loc){unsigned long _T0;unsigned long _T1;struct _fat_ptr _T2;unsigned char*_T3;const char*_T4;const char*_T5;int _T6;struct Cyc_Absyn_Kind*_T7;struct Cyc_Absyn_Kind*_T8;struct Cyc_Absyn_Kind*_T9;struct Cyc_Absyn_Kind*_TA;struct Cyc_Absyn_Kind*_TB;struct Cyc_Absyn_Kind*_TC;unsigned _TD;struct _fat_ptr _TE;struct _fat_ptr _TF;struct Cyc_Absyn_Kind*_T10;struct Cyc_Absyn_Kind*_T11;struct Cyc_Absyn_Kind*_T12;struct Cyc_Absyn_Kind*_T13;struct Cyc_Absyn_Kind*_T14;struct Cyc_Absyn_Kind*_T15;struct Cyc_Absyn_Kind*_T16;struct Cyc_Absyn_Kind*_T17;unsigned long _T18;struct Cyc_Warn_String_Warn_Warg_struct _T19;struct Cyc_Warn_String_Warn_Warg_struct _T1A;unsigned _T1B;struct _fat_ptr _T1C;struct _fat_ptr _T1D;unsigned char*_T1E;const char*_T1F;char _T20;int _T21;struct _fat_ptr _T22;unsigned char*_T23;const char*_T24;int _T25;struct Cyc_Absyn_Kind*_T26;struct Cyc_Absyn_Kind*_T27;struct Cyc_Absyn_Kind*_T28;struct Cyc_Absyn_Kind*_T29;struct Cyc_Absyn_Kind*_T2A;struct Cyc_Absyn_Kind*_T2B;struct _fat_ptr _T2C;unsigned char*_T2D;const char*_T2E;char _T2F;int _T30;struct _fat_ptr _T31;unsigned char*_T32;const char*_T33;int _T34;struct Cyc_Absyn_Kind*_T35;struct Cyc_Absyn_Kind*_T36;struct Cyc_Absyn_Kind*_T37;struct Cyc_Absyn_Kind*_T38;struct Cyc_Absyn_Kind*_T39;struct Cyc_Absyn_Kind*_T3A;struct _fat_ptr _T3B;unsigned char*_T3C;const char*_T3D;char _T3E;int _T3F;struct _fat_ptr _T40;unsigned char*_T41;const char*_T42;int _T43;struct Cyc_Absyn_Kind*_T44;struct Cyc_Absyn_Kind*_T45;struct Cyc_Absyn_Kind*_T46;struct Cyc_Absyn_Kind*_T47;struct Cyc_Absyn_Kind*_T48;struct Cyc_Absyn_Kind*_T49;struct _fat_ptr _T4A;unsigned char*_T4B;const char*_T4C;char _T4D;int _T4E;unsigned _T4F;struct _fat_ptr _T50;struct _fat_ptr _T51;struct Cyc_Absyn_Kind*_T52;struct Cyc_Absyn_Kind*_T53;struct Cyc_Warn_String_Warn_Warg_struct _T54;struct Cyc_Warn_String_Warn_Warg_struct _T55;struct Cyc_Warn_String_Warn_Warg_struct _T56;struct Cyc_Warn_Int_Warn_Warg_struct _T57;unsigned long _T58;unsigned _T59;struct _fat_ptr _T5A;struct Cyc_Absyn_Kind*_T5B;struct Cyc_Absyn_Kind*_T5C;_T0=
Cyc_strlen(s);if(_T0==1U)goto _TL9;else{goto _TLA;}_TLA: _T1=Cyc_strlen(s);if(_T1==2U)goto _TL9;else{goto _TL7;}
_TL9: _T2=s;_T3=_T2.curr;_T4=(const char*)_T3;_T5=_check_null(_T4);{char _T5D=_T5[0];_T6=(int)_T5D;switch(_T6){case 65: _T7=& Cyc_Kinds_ak;_T8=(struct Cyc_Absyn_Kind*)_T7;
return _T8;case 77: _T9=& Cyc_Kinds_mk;_TA=(struct Cyc_Absyn_Kind*)_T9;
return _TA;case 66: _TB=& Cyc_Kinds_bk;_TC=(struct Cyc_Absyn_Kind*)_TB;
return _TC;case 82: _TD=loc;_TE=
_tag_fat("Region kind ::R is deprecated; use effect kind ::E instead",sizeof(char),59U);_TF=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_warn(_TD,_TE,_TF);_T10=& Cyc_Kinds_ek;_T11=(struct Cyc_Absyn_Kind*)_T10;return _T11;case 69: _T12=& Cyc_Kinds_ek;_T13=(struct Cyc_Absyn_Kind*)_T12;
return _T13;case 73: _T14=& Cyc_Kinds_ik;_T15=(struct Cyc_Absyn_Kind*)_T14;
return _T15;case 81: _T16=& Cyc_Kinds_aqk;_T17=(struct Cyc_Absyn_Kind*)_T16;
return _T17;default: _T18=
# 155
Cyc_strlen(s);if(_T18!=2U)goto _TLC;{struct Cyc_Warn_String_Warn_Warg_struct _T5E;_T5E.tag=0;
_T5E.f1=_tag_fat("Usage of deprecated kind: ",sizeof(char),27U);_T19=_T5E;}{struct Cyc_Warn_String_Warn_Warg_struct _T5E=_T19;{struct Cyc_Warn_String_Warn_Warg_struct _T5F;_T5F.tag=0;_T5F.f1=s;_T1A=_T5F;}{struct Cyc_Warn_String_Warn_Warg_struct _T5F=_T1A;void*_T60[2];_T60[0]=& _T5E;_T60[1]=& _T5F;_T1B=loc;_T1C=_tag_fat(_T60,sizeof(void*),2);Cyc_Warn_warn2(_T1B,_T1C);}}_T1D=s;_T1E=_T1D.curr;_T1F=(const char*)_T1E;_T20=_T1F[1];_T21=(int)_T20;
if(_T21!=66)goto _TLE;_T22=s;_T23=_T22.curr;_T24=(const char*)_T23;{
char _T5E=_T24[0];_T25=(int)_T5E;switch(_T25){case 85: _T26=& Cyc_Kinds_bk_uhint;_T27=(struct Cyc_Absyn_Kind*)_T26;
return _T27;case 84: _T28=& Cyc_Kinds_bk_rhint;_T29=(struct Cyc_Absyn_Kind*)_T28;
return _T29;case 67: _T2A=& Cyc_Kinds_bk_rchint;_T2B=(struct Cyc_Absyn_Kind*)_T2A;
return _T2B;default: goto _LL11;}_LL11:;}goto _TLF;_TLE: _TLF: _T2C=s;_T2D=_T2C.curr;_T2E=(const char*)_T2D;_T2F=_T2E[1];_T30=(int)_T2F;
# 165
if(_T30!=77)goto _TL11;_T31=s;_T32=_T31.curr;_T33=(const char*)_T32;{
char _T5E=_T33[0];_T34=(int)_T5E;switch(_T34){case 85: _T35=& Cyc_Kinds_mk_uhint;_T36=(struct Cyc_Absyn_Kind*)_T35;
return _T36;case 84: _T37=& Cyc_Kinds_mk_rhint;_T38=(struct Cyc_Absyn_Kind*)_T37;
return _T38;case 67: _T39=& Cyc_Kinds_mk_rchint;_T3A=(struct Cyc_Absyn_Kind*)_T39;
return _T3A;default: goto _LL1A;}_LL1A:;}goto _TL12;_TL11: _TL12: _T3B=s;_T3C=_T3B.curr;_T3D=(const char*)_T3C;_T3E=_T3D[1];_T3F=(int)_T3E;
# 173
if(_T3F!=65)goto _TL14;_T40=s;_T41=_T40.curr;_T42=(const char*)_T41;{
char _T5E=_T42[0];_T43=(int)_T5E;switch(_T43){case 85: _T44=& Cyc_Kinds_ak_uhint;_T45=(struct Cyc_Absyn_Kind*)_T44;
return _T45;case 84: _T46=& Cyc_Kinds_ak_rhint;_T47=(struct Cyc_Absyn_Kind*)_T46;
return _T47;case 67: _T48=& Cyc_Kinds_ak_rchint;_T49=(struct Cyc_Absyn_Kind*)_T48;
return _T49;default: goto _LL23;}_LL23:;}goto _TL15;_TL14: _TL15: _T4A=s;_T4B=_T4A.curr;_T4C=(const char*)_T4B;_T4D=_T4C[1];_T4E=(int)_T4D;
# 181
if(_T4E!=82)goto _TL17;_T4F=loc;_T50=
_tag_fat("Region kind ::R is deprecated; use effect kind ::E instead",sizeof(char),59U);_T51=_tag_fat(0U,sizeof(void*),0);Cyc_Warn_warn(_T4F,_T50,_T51);_T52=& Cyc_Kinds_ek;_T53=(struct Cyc_Absyn_Kind*)_T52;
return _T53;_TL17: goto _TLD;_TLC: _TLD: goto _LL0;}_LL0:;}goto _TL8;_TL7: _TL8:{struct Cyc_Warn_String_Warn_Warg_struct _T5D;_T5D.tag=0;
# 189
_T5D.f1=_tag_fat("bad kind: ",sizeof(char),11U);_T54=_T5D;}{struct Cyc_Warn_String_Warn_Warg_struct _T5D=_T54;{struct Cyc_Warn_String_Warn_Warg_struct _T5E;_T5E.tag=0;_T5E.f1=s;_T55=_T5E;}{struct Cyc_Warn_String_Warn_Warg_struct _T5E=_T55;{struct Cyc_Warn_String_Warn_Warg_struct _T5F;_T5F.tag=0;_T5F.f1=_tag_fat(" strlen=",sizeof(char),9U);_T56=_T5F;}{struct Cyc_Warn_String_Warn_Warg_struct _T5F=_T56;{struct Cyc_Warn_Int_Warn_Warg_struct _T60;_T60.tag=12;_T58=Cyc_strlen(s);_T60.f1=(int)_T58;_T57=_T60;}{struct Cyc_Warn_Int_Warn_Warg_struct _T60=_T57;void*_T61[4];_T61[0]=& _T5D;_T61[1]=& _T5E;_T61[2]=& _T5F;_T61[3]=& _T60;_T59=loc;_T5A=_tag_fat(_T61,sizeof(void*),4);Cyc_Warn_err2(_T59,_T5A);}}}}_T5B=& Cyc_Kinds_bk;_T5C=(struct Cyc_Absyn_Kind*)_T5B;
return _T5C;}
# 193
struct _fat_ptr Cyc_Kinds_kind2string(struct Cyc_Absyn_Kind*kh){struct Cyc_Absyn_Kind*_T0;enum Cyc_Absyn_AliasHint _T1;int _T2;enum Cyc_Absyn_KindQual _T3;int _T4;struct _fat_ptr _T5;struct _fat_ptr _T6;struct _fat_ptr _T7;struct _fat_ptr _T8;struct _fat_ptr _T9;struct _fat_ptr _TA;struct _fat_ptr _TB;struct _fat_ptr _TC;enum Cyc_Absyn_KindQual _TD;int _TE;enum Cyc_Absyn_AliasHint _TF;int _T10;struct _fat_ptr _T11;struct _fat_ptr _T12;struct _fat_ptr _T13;enum Cyc_Absyn_AliasHint _T14;int _T15;struct _fat_ptr _T16;struct _fat_ptr _T17;struct _fat_ptr _T18;enum Cyc_Absyn_AliasHint _T19;int _T1A;struct _fat_ptr _T1B;struct _fat_ptr _T1C;struct _fat_ptr _T1D;struct Cyc_Warn_String_Warn_Warg_struct _T1E;int(*_T1F)(struct _fat_ptr);void*(*_T20)(struct _fat_ptr);struct _fat_ptr _T21;enum Cyc_Absyn_AliasHint _T22;enum Cyc_Absyn_KindQual _T23;_T0=kh;{struct Cyc_Absyn_Kind _T24=*_T0;_T23=_T24.kind;_T22=_T24.aliashint;}{enum Cyc_Absyn_KindQual k=_T23;enum Cyc_Absyn_AliasHint h=_T22;_T1=h;_T2=(int)_T1;
# 195
if(_T2!=3)goto _TL19;_T3=k;_T4=(int)_T3;switch(_T4){case Cyc_Absyn_AnyKind: _T5=
# 197
_tag_fat("A",sizeof(char),2U);return _T5;case Cyc_Absyn_MemKind: _T6=
_tag_fat("M",sizeof(char),2U);return _T6;case Cyc_Absyn_BoxKind: _T7=
_tag_fat("B",sizeof(char),2U);return _T7;case Cyc_Absyn_AqualKind: _T8=
# 201
_tag_fat("Q",sizeof(char),2U);return _T8;case Cyc_Absyn_EffKind: _T9=
_tag_fat("E",sizeof(char),2U);return _T9;case Cyc_Absyn_IntKind: _TA=
_tag_fat("I",sizeof(char),2U);return _TA;case Cyc_Absyn_BoolKind: _TB=
_tag_fat("BOOL",sizeof(char),5U);return _TB;case Cyc_Absyn_PtrBndKind: goto _LL15;default: _LL15: _TC=
_tag_fat("PTRBND",sizeof(char),7U);return _TC;}goto _TL1A;
# 209
_TL19: _TD=k;_TE=(int)_TD;switch(_TE){case Cyc_Absyn_BoxKind: _TF=h;_T10=(int)_TF;switch(_T10){case Cyc_Absyn_RestrictedHint: _T11=
# 212
_tag_fat("TB",sizeof(char),3U);return _T11;case Cyc_Absyn_UniqueHint: _T12=
_tag_fat("UB",sizeof(char),3U);return _T12;default: _T13=
_tag_fat("CB",sizeof(char),3U);return _T13;};case Cyc_Absyn_MemKind: _T14=h;_T15=(int)_T14;switch(_T15){case Cyc_Absyn_RestrictedHint: _T16=
# 218
_tag_fat("TM",sizeof(char),3U);return _T16;case Cyc_Absyn_UniqueHint: _T17=
_tag_fat("UM",sizeof(char),3U);return _T17;default: _T18=
_tag_fat("CM",sizeof(char),3U);return _T18;};case Cyc_Absyn_AnyKind: _T19=h;_T1A=(int)_T19;switch(_T1A){case Cyc_Absyn_RestrictedHint: _T1B=
# 224
_tag_fat("TA",sizeof(char),3U);return _T1B;case Cyc_Absyn_UniqueHint: _T1C=
_tag_fat("UA",sizeof(char),3U);return _T1C;default: _T1D=
_tag_fat("CA",sizeof(char),3U);return _T1D;};default:{struct Cyc_Warn_String_Warn_Warg_struct _T24;_T24.tag=0;
# 229
_T24.f1=_tag_fat("Bad kind",sizeof(char),9U);_T1E=_T24;}{struct Cyc_Warn_String_Warn_Warg_struct _T24=_T1E;void*_T25[1];_T25[0]=& _T24;_T20=Cyc_Warn_impos2;{int(*_T26)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T20;_T1F=_T26;}_T21=_tag_fat(_T25,sizeof(void*),1);_T1F(_T21);}}_TL1A:;}}
# 234
void*Cyc_Kinds_compress_kb(void*k){void*_T0;int*_T1;unsigned _T2;void*_T3;struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T4;struct Cyc_Core_Opt*_T5;void*_T6;struct Cyc_Core_Opt*_T7;void*_T8;struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T9;struct Cyc_Core_Opt*_TA;void**_TB;void*_TC;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_TD;struct Cyc_Core_Opt*_TE;void*_TF;void*_T10;struct Cyc_Core_Opt*_T11;void*_T12;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T13;struct Cyc_Core_Opt*_T14;void**_T15;void**_T16;void**_T17;void*_T18;void**_T19;void*_T1A;void*_T1B;_T0=k;_T1=(int*)_T0;_T2=*_T1;switch(_T2){case 0: goto _LL4;case 1: _T3=k;_T4=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T3;_T5=_T4->f1;if(_T5!=0)goto _TL21;_LL4: goto _LL6;_TL21: _T6=k;{struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T1C=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T6;_T7=_T1C->f1;{struct Cyc_Core_Opt _T1D=*_T7;_T8=k;_T9=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T8;_TA=_T9->f1;_TB=& _TA->v;_T1B=(void**)_TB;}}{void**k2=_T1B;_T1B=k2;goto _LLA;}default: _TC=k;_TD=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_TC;_TE=_TD->f1;if(_TE!=0)goto _TL23;_LL6: _TF=k;
# 238
return _TF;_TL23: _T10=k;{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T1C=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T10;_T11=_T1C->f1;{struct Cyc_Core_Opt _T1D=*_T11;_T12=k;_T13=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T12;_T14=_T13->f1;_T15=& _T14->v;_T1B=(void**)_T15;}}_LLA: {void**k2=(void**)_T1B;_T16=k2;_T17=k2;_T18=*_T17;
# 241
*_T16=Cyc_Kinds_compress_kb(_T18);_T19=k2;_T1A=*_T19;
return _T1A;}};}
# 245
struct Cyc_Absyn_Kind*Cyc_Kinds_force_kb(void*kb){int*_T0;unsigned _T1;struct Cyc_Absyn_Kind*_T2;struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T3;struct Cyc_Core_Opt**_T4;struct Cyc_Absyn_Kind*_T5;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T6;struct Cyc_Core_Opt**_T7;struct Cyc_Core_Opt**_T8;struct Cyc_Core_Opt*_T9;struct Cyc_Absyn_Kind*_TA;
void*_TB=Cyc_Kinds_compress_kb(kb);struct Cyc_Absyn_Kind*_TC;void*_TD;_T0=(int*)_TB;_T1=*_T0;switch(_T1){case 0:{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_TE=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_TB;_TD=_TE->f1;}{struct Cyc_Absyn_Kind*k=_TD;_T2=k;
return _T2;}case 1:{struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_TE=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_TB;_T3=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_TB;_T4=& _T3->f1;_TD=(struct Cyc_Core_Opt**)_T4;}{struct Cyc_Core_Opt**f=_TD;_TD=f;_T5=& Cyc_Kinds_bk;_TC=(struct Cyc_Absyn_Kind*)_T5;goto _LL6;}default:{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_TE=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_TB;_T6=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_TB;_T7=& _T6->f1;_TD=(struct Cyc_Core_Opt**)_T7;_TC=_TE->f2;}_LL6: {struct Cyc_Core_Opt**f=(struct Cyc_Core_Opt**)_TD;struct Cyc_Absyn_Kind*k=_TC;_T8=f;{struct Cyc_Core_Opt*_TE=_cycalloc(sizeof(struct Cyc_Core_Opt));
# 250
_TE->v=Cyc_Kinds_kind_to_bound(k);_T9=(struct Cyc_Core_Opt*)_TE;}*_T8=_T9;_TA=k;
return _TA;}};}
# 258
void*Cyc_Kinds_copy_kindbound(void*kb){int*_T0;unsigned _T1;struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T2;void*_T3;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T4;void*_T5;void*_T6;
void*_T7=Cyc_Kinds_compress_kb(kb);struct Cyc_Absyn_Kind*_T8;_T0=(int*)_T7;_T1=*_T0;switch(_T1){case 1:{struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T9=_cycalloc(sizeof(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct));_T9->tag=1;
_T9->f1=0;_T2=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T9;}_T3=(void*)_T2;return _T3;case 2:{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T9=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T7;_T8=_T9->f2;}{struct Cyc_Absyn_Kind*k=_T8;{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T9=_cycalloc(sizeof(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct));_T9->tag=2;
_T9->f1=0;_T9->f2=k;_T4=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T9;}_T5=(void*)_T4;return _T5;}default: _T6=kb;
return _T6;};}struct _tuple11{enum Cyc_Absyn_KindQual f0;enum Cyc_Absyn_KindQual f1;};
# 274 "kinds.cyc"
int Cyc_Kinds_kind_leq(struct Cyc_Absyn_Kind*ka1,struct Cyc_Absyn_Kind*ka2){struct Cyc_Absyn_Kind*_T0;struct Cyc_Absyn_Kind*_T1;enum Cyc_Absyn_KindQual _T2;int _T3;enum Cyc_Absyn_KindQual _T4;int _T5;struct _tuple11 _T6;enum Cyc_Absyn_KindQual _T7;int _T8;enum Cyc_Absyn_KindQual _T9;int _TA;enum Cyc_Absyn_KindQual _TB;enum Cyc_Absyn_KindQual _TC;_T0=ka1;{struct Cyc_Absyn_Kind _TD=*_T0;_TC=_TD.kind;}{enum Cyc_Absyn_KindQual k1=_TC;enum Cyc_Absyn_KindQual _TD;_T1=ka2;{struct Cyc_Absyn_Kind _TE=*_T1;_TD=_TE.kind;}{enum Cyc_Absyn_KindQual k2=_TD;_T2=k1;_T3=(int)_T2;_T4=k2;_T5=(int)_T4;
# 278
if(_T3==_T5)goto _TL27;{struct _tuple11 _TE;
_TE.f0=k1;_TE.f1=k2;_T6=_TE;}{struct _tuple11 _TE=_T6;_T7=_TE.f0;_T8=(int)_T7;switch(_T8){case Cyc_Absyn_BoxKind: _T9=_TE.f1;_TA=(int)_T9;switch(_TA){case Cyc_Absyn_MemKind: goto _LLA;case Cyc_Absyn_AnyKind: _LLA: goto _LLC;default: goto _LLD;};case Cyc_Absyn_MemKind: _TB=_TE.f1;if(_TB!=Cyc_Absyn_AnyKind)goto _TL2B;_LLC: goto _LL6;_TL2B: goto _LLD;default: _LLD:
# 283
 return 0;}_LL6:;}goto _TL28;_TL27: _TL28:
# 292
 return 1;}}}
# 295
struct Cyc_Absyn_Kind*Cyc_Kinds_tvar_kind(struct Cyc_Absyn_Tvar*tv,struct Cyc_Absyn_Kind*def){struct Cyc_Absyn_Tvar*_T0;void*_T1;int*_T2;unsigned _T3;struct Cyc_Absyn_Kind*_T4;struct Cyc_Absyn_Kind*_T5;struct Cyc_Absyn_Tvar*_T6;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T7;struct Cyc_Absyn_Kind*_T8;_T0=tv;_T1=_T0->kind;{
void*_T9=Cyc_Kinds_compress_kb(_T1);struct Cyc_Absyn_Kind*_TA;_T2=(int*)_T9;_T3=*_T2;switch(_T3){case 0:{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_TB=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T9;_TA=_TB->f1;}{struct Cyc_Absyn_Kind*k=_TA;_T4=k;
return _T4;}case 2:{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_TB=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T9;_TA=_TB->f2;}{struct Cyc_Absyn_Kind*k=_TA;_T5=k;
return _T5;}default: _T6=tv;{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_TB=_cycalloc(sizeof(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct));_TB->tag=2;
_TB->f1=0;_TB->f2=def;_T7=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_TB;}_T6->kind=(void*)_T7;_T8=def;return _T8;};}}
# 302
struct Cyc_Core_Opt Cyc_Kinds_tvar_kind_opt(struct Cyc_Absyn_Tvar*tv){struct Cyc_Absyn_Tvar*_T0;void*_T1;int*_T2;unsigned _T3;struct Cyc_Core_Opt _T4;struct Cyc_Core_Opt _T5;struct Cyc_Core_Opt _T6;_T0=tv;_T1=_T0->kind;{
void*_T7=Cyc_Kinds_compress_kb(_T1);struct Cyc_Absyn_Kind*_T8;_T2=(int*)_T7;_T3=*_T2;switch(_T3){case 0:{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T9=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T7;_T8=_T9->f1;}{struct Cyc_Absyn_Kind*k=_T8;{struct Cyc_Core_Opt _T9;
_T9.v=k;_T4=_T9;}return _T4;}case 2:{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T9=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T7;_T8=_T9->f2;}{struct Cyc_Absyn_Kind*k=_T8;{struct Cyc_Core_Opt _T9;
_T9.v=k;_T5=_T9;}return _T5;}default:{struct Cyc_Core_Opt _T9;
_T9.v=0;_T6=_T9;}return _T6;};}}struct _tuple12{struct Cyc_Absyn_Tvar*f0;void*f1;};
# 310
struct _tuple12 Cyc_Kinds_swap_kind(void*t,void*kb){int*_T0;int _T1;struct Cyc_Absyn_Tvar*_T2;struct Cyc_Absyn_Tvar*_T3;struct _tuple12 _T4;struct Cyc_Warn_String_Warn_Warg_struct _T5;struct Cyc_Warn_Typ_Warn_Warg_struct _T6;int(*_T7)(struct _fat_ptr);void*(*_T8)(struct _fat_ptr);struct _fat_ptr _T9;
void*_TA=Cyc_Absyn_compress(t);struct Cyc_Absyn_Tvar*_TB;_T0=(int*)_TA;_T1=*_T0;if(_T1!=2)goto _TL2F;{struct Cyc_Absyn_VarType_Absyn_Type_struct*_TC=(struct Cyc_Absyn_VarType_Absyn_Type_struct*)_TA;_TB=_TC->f1;}{struct Cyc_Absyn_Tvar*tv=_TB;_T2=tv;{
# 313
void*oldkb=_T2->kind;_T3=tv;
_T3->kind=kb;{struct _tuple12 _TC;
_TC.f0=tv;_TC.f1=oldkb;_T4=_TC;}return _T4;}}_TL2F:{struct Cyc_Warn_String_Warn_Warg_struct _TC;_TC.tag=0;
_TC.f1=_tag_fat("swap_kind: cannot update the kind of ",sizeof(char),38U);_T5=_TC;}{struct Cyc_Warn_String_Warn_Warg_struct _TC=_T5;{struct Cyc_Warn_Typ_Warn_Warg_struct _TD;_TD.tag=2;_TD.f1=t;_T6=_TD;}{struct Cyc_Warn_Typ_Warn_Warg_struct _TD=_T6;void*_TE[2];_TE[0]=& _TC;_TE[1]=& _TD;_T8=Cyc_Warn_impos2;{int(*_TF)(struct _fat_ptr)=(int(*)(struct _fat_ptr))_T8;_T7=_TF;}_T9=_tag_fat(_TE,sizeof(void*),2);_T7(_T9);}};}
# 320
int Cyc_Kinds_kind_eq(struct Cyc_Absyn_Kind*k1,struct Cyc_Absyn_Kind*k2){int _T0;int _T1;struct Cyc_Absyn_Kind*_T2;enum Cyc_Absyn_KindQual _T3;int _T4;struct Cyc_Absyn_Kind*_T5;enum Cyc_Absyn_KindQual _T6;int _T7;struct Cyc_Absyn_Kind*_T8;enum Cyc_Absyn_AliasHint _T9;int _TA;struct Cyc_Absyn_Kind*_TB;enum Cyc_Absyn_AliasHint _TC;int _TD;
if(k1!=k2)goto _TL31;_T0=1;goto _TL32;_TL31: _T2=k1;_T3=_T2->kind;_T4=(int)_T3;_T5=k2;_T6=_T5->kind;_T7=(int)_T6;if(_T4!=_T7)goto _TL33;_T8=k1;_T9=_T8->aliashint;_TA=(int)_T9;_TB=k2;_TC=_TB->aliashint;_TD=(int)_TC;_T1=_TA==_TD;goto _TL34;_TL33: _T1=0;_TL34: _T0=_T1;_TL32: return _T0;}struct _tuple13{void*f0;void*f1;};
# 327
int Cyc_Kinds_constrain_kinds(void*c1,void*c2){struct _tuple13 _T0;void*_T1;int*_T2;int _T3;void*_T4;int*_T5;unsigned _T6;void*_T7;void*_T8;int _T9;void*_TA;void*_TB;void*_TC;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_TD;struct Cyc_Core_Opt**_TE;int _TF;struct Cyc_Core_Opt**_T10;struct Cyc_Core_Opt*_T11;void*_T12;int*_T13;int _T14;void*_T15;void*_T16;struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T17;struct Cyc_Core_Opt**_T18;struct Cyc_Core_Opt**_T19;struct Cyc_Core_Opt*_T1A;void*_T1B;int*_T1C;int _T1D;void*_T1E;void*_T1F;struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T20;struct Cyc_Core_Opt**_T21;struct Cyc_Core_Opt**_T22;struct Cyc_Core_Opt*_T23;void*_T24;int*_T25;int _T26;void*_T27;void*_T28;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T29;struct Cyc_Core_Opt**_T2A;void*_T2B;int _T2C;struct Cyc_Core_Opt**_T2D;struct Cyc_Core_Opt*_T2E;void*_T2F;void*_T30;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T31;struct Cyc_Core_Opt**_T32;void*_T33;void*_T34;struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T35;struct Cyc_Core_Opt**_T36;int _T37;struct Cyc_Core_Opt**_T38;struct Cyc_Core_Opt*_T39;int _T3A;struct Cyc_Core_Opt**_T3B;struct Cyc_Core_Opt*_T3C;
c1=Cyc_Kinds_compress_kb(c1);
c2=Cyc_Kinds_compress_kb(c2);
if(c1!=c2)goto _TL35;return 1;_TL35:{struct _tuple13 _T3D;
_T3D.f0=c1;_T3D.f1=c2;_T0=_T3D;}{struct _tuple13 _T3D=_T0;struct Cyc_Absyn_Kind*_T3E;struct Cyc_Absyn_Kind*_T3F;void*_T40;void*_T41;_T1=_T3D.f0;_T2=(int*)_T1;_T3=*_T2;if(_T3!=0)goto _TL37;_T4=_T3D.f1;_T5=(int*)_T4;_T6=*_T5;switch(_T6){case 0: _T7=_T3D.f0;{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T7;_T41=_T42->f1;}_T8=_T3D.f1;{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T8;_T40=_T42->f1;}{struct Cyc_Absyn_Kind*k1=_T41;struct Cyc_Absyn_Kind*k2=_T40;_T9=k1==k2;
return _T9;}case 1: goto _LL3;default: _TA=_T3D.f0;{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_TA;_T41=_T42->f1;}_TB=_T3D.f1;{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_TB;_TC=_T3D.f1;_TD=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_TC;_TE=& _TD->f1;_T40=(struct Cyc_Core_Opt**)_TE;_T3F=_T42->f2;}{struct Cyc_Absyn_Kind*k1=_T41;struct Cyc_Core_Opt**f=_T40;struct Cyc_Absyn_Kind*k2=_T3F;_TF=
# 341
Cyc_Kinds_kind_leq(k1,k2);if(_TF)goto _TL3A;else{goto _TL3C;}
_TL3C: return 0;_TL3A: _T10=f;{struct Cyc_Core_Opt*_T42=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T42->v=c1;_T11=(struct Cyc_Core_Opt*)_T42;}*_T10=_T11;
return 1;}}goto _TL38;_TL37: _T12=_T3D.f1;_T13=(int*)_T12;_T14=*_T13;if(_T14!=1)goto _TL3D;_LL3: _T15=_T3D.f1;{struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T15;_T16=_T3D.f1;_T17=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T16;_T18=& _T17->f1;_T41=(struct Cyc_Core_Opt**)_T18;}{struct Cyc_Core_Opt**f=_T41;_T19=f;{struct Cyc_Core_Opt*_T42=_cycalloc(sizeof(struct Cyc_Core_Opt));
# 333
_T42->v=c1;_T1A=(struct Cyc_Core_Opt*)_T42;}*_T19=_T1A;return 1;}_TL3D: _T1B=_T3D.f0;_T1C=(int*)_T1B;_T1D=*_T1C;if(_T1D!=1)goto _TL3F;_T1E=_T3D.f0;{struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T1E;_T1F=_T3D.f0;_T20=(struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_T1F;_T21=& _T20->f1;_T41=(struct Cyc_Core_Opt**)_T21;}{struct Cyc_Core_Opt**f=_T41;_T22=f;{struct Cyc_Core_Opt*_T42=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T42->v=c2;_T23=(struct Cyc_Core_Opt*)_T42;}*_T22=_T23;return 1;}_TL3F: _T24=_T3D.f1;_T25=(int*)_T24;_T26=*_T25;if(_T26!=0)goto _TL41;_T27=_T3D.f0;{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T27;_T28=_T3D.f0;_T29=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T28;_T2A=& _T29->f1;_T41=(struct Cyc_Core_Opt**)_T2A;_T3F=_T42->f2;}_T2B=_T3D.f1;{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T2B;_T40=_T42->f1;}{struct Cyc_Core_Opt**f=_T41;struct Cyc_Absyn_Kind*k1=_T3F;struct Cyc_Absyn_Kind*k2=_T40;_T2C=
# 336
Cyc_Kinds_kind_leq(k2,k1);if(_T2C)goto _TL43;else{goto _TL45;}
_TL45: return 0;_TL43: _T2D=f;{struct Cyc_Core_Opt*_T42=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T42->v=c2;_T2E=(struct Cyc_Core_Opt*)_T42;}*_T2D=_T2E;
return 1;}_TL41: _T2F=_T3D.f0;{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T2F;_T30=_T3D.f0;_T31=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T30;_T32=& _T31->f1;_T41=(struct Cyc_Core_Opt**)_T32;_T3F=_T42->f2;}_T33=_T3D.f1;{struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_T42=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T33;_T34=_T3D.f1;_T35=(struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_T34;_T36=& _T35->f1;_T40=(struct Cyc_Core_Opt**)_T36;_T3E=_T42->f2;}{struct Cyc_Core_Opt**f1=(struct Cyc_Core_Opt**)_T41;struct Cyc_Absyn_Kind*k1=_T3F;struct Cyc_Core_Opt**f2=(struct Cyc_Core_Opt**)_T40;struct Cyc_Absyn_Kind*k2=_T3E;_T37=
# 346
Cyc_Kinds_kind_leq(k1,k2);if(!_T37)goto _TL46;_T38=f2;{struct Cyc_Core_Opt*_T42=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T42->v=c1;_T39=(struct Cyc_Core_Opt*)_T42;}*_T38=_T39;
return 1;_TL46: _T3A=
# 350
Cyc_Kinds_kind_leq(k2,k1);if(!_T3A)goto _TL48;_T3B=f1;{struct Cyc_Core_Opt*_T42=_cycalloc(sizeof(struct Cyc_Core_Opt));
_T42->v=c2;_T3C=(struct Cyc_Core_Opt*)_T42;}*_T3B=_T3C;
return 1;_TL48:
# 354
 return 0;}_TL38:;}}
# 358
void*Cyc_Kinds_consistent_aliashint(unsigned loc,void*kb,void*aq){void*_T0;int*_T1;int _T2;void*_T3;struct Cyc_Absyn_Kind*_T4;enum Cyc_Absyn_AliasHint _T5;int _T6;void*_T7;void*_T8;struct Cyc_Warn_String_Warn_Warg_struct _T9;unsigned _TA;struct _fat_ptr _TB;void*_TC;struct Cyc_Warn_String_Warn_Warg_struct _TD;unsigned _TE;struct _fat_ptr _TF;void*_T10;struct Cyc_Warn_String_Warn_Warg_struct _T11;unsigned _T12;struct _fat_ptr _T13;struct Cyc_Warn_String_Warn_Warg_struct _T14;unsigned _T15;struct _fat_ptr _T16;void*_T17;{enum Cyc_Absyn_AliasHint _T18;_T0=kb;_T1=(int*)_T0;_T2=*_T1;if(_T2!=0)goto _TL4A;_T3=kb;{struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_T19=(struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_T3;_T4=_T19->f1;{struct Cyc_Absyn_Kind _T1A=*_T4;_T18=_T1A.aliashint;}}{enum Cyc_Absyn_AliasHint ah=_T18;_T5=ah;_T6=(int)_T5;switch(_T6){case Cyc_Absyn_NoHint: _T7=aq;
# 363
return _T7;case Cyc_Absyn_RestrictedHint:
# 365
 if(aq==0)goto _TL4F;else{goto _TL50;}_TL50: if(aq==Cyc_Absyn_rtd_qual_type)goto _TL4F;else{goto _TL4D;}
_TL4F: _T8=Cyc_Absyn_rtd_qual_type;return _T8;_TL4D:{struct Cyc_Warn_String_Warn_Warg_struct _T19;_T19.tag=0;
_T19.f1=_tag_fat("Inconsistent qualifier bounds",sizeof(char),30U);_T9=_T19;}{struct Cyc_Warn_String_Warn_Warg_struct _T19=_T9;void*_T1A[1];_T1A[0]=& _T19;_TA=loc;_TB=_tag_fat(_T1A,sizeof(void*),1);Cyc_Warn_err2(_TA,_TB);}goto _LL5;case Cyc_Absyn_UniqueHint:
# 370
 if(aq==0)goto _TL53;else{goto _TL54;}_TL54: if(aq==Cyc_Absyn_un_qual_type)goto _TL53;else{goto _TL51;}
_TL53: _TC=Cyc_Absyn_un_qual_type;return _TC;_TL51:{struct Cyc_Warn_String_Warn_Warg_struct _T19;_T19.tag=0;
_T19.f1=_tag_fat("Inconsistent qualifier bounds",sizeof(char),30U);_TD=_T19;}{struct Cyc_Warn_String_Warn_Warg_struct _T19=_TD;void*_T1A[1];_T1A[0]=& _T19;_TE=loc;_TF=_tag_fat(_T1A,sizeof(void*),1);Cyc_Warn_err2(_TE,_TF);}goto _LL5;case Cyc_Absyn_RefcntHint:
# 375
 if(aq==0)goto _TL57;else{goto _TL58;}_TL58: if(aq==Cyc_Absyn_rc_qual_type)goto _TL57;else{goto _TL55;}
_TL57: _T10=Cyc_Absyn_rc_qual_type;return _T10;_TL55:{struct Cyc_Warn_String_Warn_Warg_struct _T19;_T19.tag=0;
_T19.f1=_tag_fat("Inconsistent qualifier bounds",sizeof(char),30U);_T11=_T19;}{struct Cyc_Warn_String_Warn_Warg_struct _T19=_T11;void*_T1A[1];_T1A[0]=& _T19;_T12=loc;_T13=_tag_fat(_T1A,sizeof(void*),1);Cyc_Warn_err2(_T12,_T13);}goto _LL5;default:{struct Cyc_Warn_String_Warn_Warg_struct _T19;_T19.tag=0;
# 380
_T19.f1=_tag_fat("Inconsistent qualifier bounds",sizeof(char),30U);_T14=_T19;}{struct Cyc_Warn_String_Warn_Warg_struct _T19=_T14;void*_T1A[1];_T1A[0]=& _T19;_T15=loc;_T16=_tag_fat(_T1A,sizeof(void*),1);Cyc_Warn_err2(_T15,_T16);}}_LL5: goto _LL0;}_TL4A: goto _LL0;_LL0:;}_T17=aq;
# 386
return _T17;}

#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file lib/runtime_*.c */
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
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Discriminated Unions */
struct _xtunion_struct { char *tag; };

/* Regions */
struct _RegionPage
#ifdef CYC_REGION_PROFILE
{ unsigned total_bytes;
  unsigned free_bytes;
  struct _RegionPage *next;
  char data[1];
}
#endif
; // abstract -- defined in runtime_memory.c
struct _pool;
struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
  struct _pool *released_ptrs;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#else
  unsigned used_bytes;
  unsigned wasted_bytes;
#endif
};
struct _DynRegionFrame {
  struct _RuntimeStack s;
  struct _DynRegionHandle *x;
};
// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

struct _RegionHandle _new_region(const char*);
void* _region_malloc(struct _RegionHandle*, unsigned);
void* _region_calloc(struct _RegionHandle*, unsigned t, unsigned n);
void   _free_region(struct _RegionHandle*);
struct _RegionHandle*_open_dynregion(struct _DynRegionFrame*,struct _DynRegionHandle*);
void   _pop_dynregion();

/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};
void _push_handler(struct _handler_cons *);
void _push_region(struct _RegionHandle *);
void _npop_handler(int);
void _pop_handler();
void _pop_region();

#ifndef _throw
void* _throw_null_fn(const char*,unsigned);
void* _throw_arraybounds_fn(const char*,unsigned);
void* _throw_badalloc_fn(const char*,unsigned);
void* _throw_match_fn(const char*,unsigned);
void* _throw_fn(void*,const char*,unsigned);
void* _rethrow(void*);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

struct _xtunion_struct* Cyc_Core_get_exn_thrown();
/* Built-in Exceptions */
struct Cyc_Null_Exception_exn_struct { char *tag; };
struct Cyc_Array_bounds_exn_struct { char *tag; };
struct Cyc_Match_Exception_exn_struct { char *tag; };
struct Cyc_Bad_alloc_exn_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];

/* Built-in Run-time Checks and company */
#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#else
#define _INLINE inline
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ void*_cks_null = (void*)(ptr); \
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
#define _zero_arr_plus_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_char_fn _zero_arr_plus_fn
#define _zero_arr_plus_short_fn _zero_arr_plus_fn
#define _zero_arr_plus_int_fn _zero_arr_plus_fn
#define _zero_arr_plus_float_fn _zero_arr_plus_fn
#define _zero_arr_plus_double_fn _zero_arr_plus_fn
#define _zero_arr_plus_longdouble_fn _zero_arr_plus_fn
#define _zero_arr_plus_voidstar_fn _zero_arr_plus_fn
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
short* _zero_arr_plus_short_fn(short*,unsigned,int,const char*,unsigned);
int* _zero_arr_plus_int_fn(int*,unsigned,int,const char*,unsigned);
float* _zero_arr_plus_float_fn(float*,unsigned,int,const char*,unsigned);
double* _zero_arr_plus_double_fn(double*,unsigned,int,const char*,unsigned);
long double* _zero_arr_plus_longdouble_fn(long double*,unsigned,int,const char*, unsigned);
void** _zero_arr_plus_voidstar_fn(void**,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
int _get_zero_arr_size_char(const char*,unsigned);
int _get_zero_arr_size_short(const short*,unsigned);
int _get_zero_arr_size_int(const int*,unsigned);
int _get_zero_arr_size_float(const float*,unsigned);
int _get_zero_arr_size_double(const double*,unsigned);
int _get_zero_arr_size_longdouble(const long double*,unsigned);
int _get_zero_arr_size_voidstar(const void**,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_short_fn(short**,int,const char*,unsigned);
int* _zero_arr_inplace_plus_int(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_longdouble_fn(long double**,int,const char*,unsigned);
void** _zero_arr_inplace_plus_voidstar_fn(void***,int,const char*,unsigned);
/* like the previous functions, but does post-addition (as in e++) */
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_post_short_fn(short**x,int,const char*,unsigned);
int* _zero_arr_inplace_plus_post_int_fn(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_post_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_post_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_post_longdouble_fn(long double**,int,const char *,unsigned);
void** _zero_arr_inplace_plus_post_voidstar_fn(void***,int,const char*,unsigned);
#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_short(x,s,i) \
  (_zero_arr_plus_short_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_int(x,s,i) \
  (_zero_arr_plus_int_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_float(x,s,i) \
  (_zero_arr_plus_float_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_double(x,s,i) \
  (_zero_arr_plus_double_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_longdouble(x,s,i) \
  (_zero_arr_plus_longdouble_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_voidstar(x,s,i) \
  (_zero_arr_plus_voidstar_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_short(x,i) \
  _zero_arr_inplace_plus_short_fn((short **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_int(x,i) \
  _zero_arr_inplace_plus_int_fn((int **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_float(x,i) \
  _zero_arr_inplace_plus_float_fn((float **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_double(x,i) \
  _zero_arr_inplace_plus_double_fn((double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_longdouble(x,i) \
  _zero_arr_inplace_plus_longdouble_fn((long double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_voidstar(x,i) \
  _zero_arr_inplace_plus_voidstar_fn((void ***)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_short(x,i) \
  _zero_arr_inplace_plus_post_short_fn((short **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_int(x,i) \
  _zero_arr_inplace_plus_post_int_fn((int **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_float(x,i) \
  _zero_arr_inplace_plus_post_float_fn((float **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_double(x,i) \
  _zero_arr_inplace_plus_post_double_fn((double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_longdouble(x,i) \
  _zero_arr_inplace_plus_post_longdouble_fn((long double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_voidstar(x,i) \
  _zero_arr_inplace_plus_post_voidstar_fn((void***)(x),(i),__FILE__,__LINE__)

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char *)0) \
    _throw_arraybounds(); \
  _curr; })
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

/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);
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

static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#define _CYC_MAX_REGION_CONST 2
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static _INLINE void *_fast_region_malloc(struct _RegionHandle *r, unsigned orig_s) {  
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST && r->curr != 0) { 
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
  return _region_malloc(r,orig_s); 
}

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,unsigned,unsigned,const char *,const char*,int);
struct _RegionHandle _profile_new_region(const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
#define _new_region(n) _profile_new_region(n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,n,t) _profile_region_calloc(rh,n,t,__FILE__,__FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif
#endif
 struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;
# 171
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*,void*);};struct Cyc_Set_Set;extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 100
extern int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_timeval{long tv_sec;long tv_usec;};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 149 "absyn.h"
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 170
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 175
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable =0U,Cyc_Absyn_Unique =1U,Cyc_Absyn_Top =2U};
# 180
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_RgnKind =3U,Cyc_Absyn_EffKind =4U,Cyc_Absyn_IntKind =5U,Cyc_Absyn_BoolKind =6U,Cyc_Absyn_PtrBndKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;void*released;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_Absyn_Vardecl*return_value;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 392 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 465
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 472
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 490
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple8{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple8*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple10*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 864 "absyn.h"
void*Cyc_Absyn_compress(void*);
# 924
void*Cyc_Absyn_bounds_one (void);
# 974
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 41 "relations-ap.h"
extern union Cyc_Relations_RelnOp Cyc_Relations_RConst(unsigned);extern union Cyc_Relations_RelnOp Cyc_Relations_RNumelts(struct Cyc_Absyn_Vardecl*);
# 50
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 76
int Cyc_Relations_exp2relnop(struct Cyc_Absyn_Exp*,union Cyc_Relations_RelnOp*);
# 87
struct Cyc_List_List*Cyc_Relations_add_relation(struct _RegionHandle*,union Cyc_Relations_RelnOp,enum Cyc_Relations_Relation,union Cyc_Relations_RelnOp,struct Cyc_List_List*);
# 118
void Cyc_Relations_print_relns(struct Cyc___cycFILE*,struct Cyc_List_List*);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*);
# 41 "cf_flowinfo.h"
int Cyc_CfFlowInfo_anal_error;struct Cyc_CfFlowInfo_VarRoot_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_CfFlowInfo_MallocPt_CfFlowInfo_Root_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;};struct Cyc_CfFlowInfo_InitParam_CfFlowInfo_Root_struct{int tag;int f1;void*f2;};struct Cyc_CfFlowInfo_Dot_CfFlowInfo_PathCon_struct{int tag;int f1;};struct Cyc_CfFlowInfo_Star_CfFlowInfo_PathCon_struct{int tag;};struct Cyc_CfFlowInfo_Place{void*root;struct Cyc_List_List*path;};
# 74
enum Cyc_CfFlowInfo_InitLevel{Cyc_CfFlowInfo_NoneIL =0U,Cyc_CfFlowInfo_AllIL =1U};extern char Cyc_CfFlowInfo_IsZero[7U];struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_CfFlowInfo_NotZero[8U];struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};extern char Cyc_CfFlowInfo_UnknownZ[9U];struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_List_List*f1;};struct _union_AbsLVal_PlaceL{int tag;struct Cyc_CfFlowInfo_Place*val;};struct _union_AbsLVal_UnknownL{int tag;int val;};union Cyc_CfFlowInfo_AbsLVal{struct _union_AbsLVal_PlaceL PlaceL;struct _union_AbsLVal_UnknownL UnknownL;};struct Cyc_CfFlowInfo_UnionRInfo{int is_union;int fieldnum;};struct Cyc_CfFlowInfo_Zero_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_NotZeroAll_CfFlowInfo_AbsRVal_struct{int tag;};struct Cyc_CfFlowInfo_UnknownR_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_Esc_CfFlowInfo_AbsRVal_struct{int tag;enum Cyc_CfFlowInfo_InitLevel f1;};struct Cyc_CfFlowInfo_AddressOf_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_Place*f1;};struct Cyc_CfFlowInfo_UniquePtr_CfFlowInfo_AbsRVal_struct{int tag;void*f1;};struct Cyc_CfFlowInfo_Aggregate_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_CfFlowInfo_UnionRInfo f1;struct _fat_ptr f2;};struct Cyc_CfFlowInfo_Consumed_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;void*f3;int f4;};struct Cyc_CfFlowInfo_NamedLocation_CfFlowInfo_AbsRVal_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;};struct _union_FlowInfo_BottomFL{int tag;int val;};struct _tuple11{struct Cyc_Dict_Dict f1;struct Cyc_List_List*f2;};struct _union_FlowInfo_ReachableFL{int tag;struct _tuple11 val;};union Cyc_CfFlowInfo_FlowInfo{struct _union_FlowInfo_BottomFL BottomFL;struct _union_FlowInfo_ReachableFL ReachableFL;};struct Cyc_CfFlowInfo_FlowEnv{void*zero;void*notzeroall;void*unknown_none;void*unknown_all;void*esc_none;void*esc_all;struct Cyc_Dict_Dict mt_flowdict;struct Cyc_CfFlowInfo_Place*dummy_place;};struct Cyc_RgnOrder_RgnPO;
# 51 "tcutil.h"
int Cyc_Tcutil_is_zeroterm_pointer_type(void*);
# 71
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*,void*);
# 74
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_type_bound(void*);
# 83
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*,void*);
# 221
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*);
# 235
int Cyc_Tcutil_force_type2bool(int,void*);
# 32 "evexp.h"
extern int Cyc_Evexp_c_can_eval(struct Cyc_Absyn_Exp*);
# 42 "evexp.h"
extern int Cyc_Evexp_lte_const_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*);
# 29 "warn.h"
void Cyc_Warn_warn(unsigned,struct _fat_ptr,struct _fat_ptr);
# 35
void Cyc_Warn_err(unsigned,struct _fat_ptr,struct _fat_ptr);
# 46
void*Cyc_Warn_impos_loc(unsigned,struct _fat_ptr,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_TypOpt_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Stmt_Warn_Warg_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Warn_Aggrdecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Warn_Tvar_Warn_Warg_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Warn_KindBound_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Kind_Warn_Warg_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Warn_Attribute_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Vardecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Warn_Int_Warn_Warg_struct{int tag;int f1;};
# 58 "flags.h"
extern int Cyc_Flags_warn_bounds_checks;
# 62
extern int Cyc_Flags_warn_all_null_deref;
# 73
enum Cyc_Flags_C_Compilers{Cyc_Flags_Gcc_c =0U,Cyc_Flags_Vc_c =1U};
# 87 "flags.h"
enum Cyc_Flags_Cyclone_Passes{Cyc_Flags_Cpp =0U,Cyc_Flags_Parsing =1U,Cyc_Flags_Binding =2U,Cyc_Flags_CurrentRegion =3U,Cyc_Flags_TypeChecking =4U,Cyc_Flags_Jumps =5U,Cyc_Flags_FlowAnalysis =6U,Cyc_Flags_VCGen =7U,Cyc_Flags_CheckInsertion =8U,Cyc_Flags_Toc =9U,Cyc_Flags_AggregateRemoval =10U,Cyc_Flags_LabelRemoval =11U,Cyc_Flags_EvalOrder =12U,Cyc_Flags_CCompiler =13U,Cyc_Flags_AllPasses =14U};struct Cyc_Hashtable_Table;struct Cyc_JumpAnalysis_Jump_Anal_Result{struct Cyc_Hashtable_Table*pop_tables;struct Cyc_Hashtable_Table*succ_tables;struct Cyc_Hashtable_Table*pat_pop_tables;};extern char Cyc_InsertChecks_FatBound[9U];struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NoCheck[8U];struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndFatBound[16U];struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_NullAndThinBound[17U];struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};extern char Cyc_InsertChecks_NullOnly[9U];struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_InsertChecks_ThinBound[10U];struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 35 "insert_checks.h"
extern struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NoCheck_val;
extern struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullOnly_val;
extern struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullAndFatBound_val;
extern struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_FatBound_val;struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};struct Cyc_AssnDef_Const_AssnDef_Term_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_AssnDef_Var_AssnDef_Term_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_AssnDef_LogicVar_AssnDef_Term_struct{int tag;struct Cyc_Absyn_Vardecl*f1;int f2;};struct Cyc_AssnDef_Primop_AssnDef_Term_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_AssnDef_Cast_AssnDef_Term_struct{int tag;void*f1;void*f2;};
# 79 "assndef.h"
enum Cyc_AssnDef_Primreln{Cyc_AssnDef_Eq =0U,Cyc_AssnDef_Neq =1U,Cyc_AssnDef_SLt =2U,Cyc_AssnDef_SLte =3U,Cyc_AssnDef_ULt =4U,Cyc_AssnDef_ULte =5U};struct Cyc_AssnDef_True_AssnDef_Assn_struct{int tag;};struct Cyc_AssnDef_False_AssnDef_Assn_struct{int tag;};struct Cyc_AssnDef_Prim_AssnDef_Assn_struct{int tag;void*f1;enum Cyc_AssnDef_Primreln f2;void*f3;};struct Cyc_AssnDef_And_AssnDef_Assn_struct{int tag;void*f1;void*f2;};struct Cyc_AssnDef_Or_AssnDef_Assn_struct{int tag;void*f1;void*f2;};struct Cyc_AssnDef_Subst_AssnDef_Assn_struct{int tag;struct Cyc_Absyn_Vardecl*f1;void*f2;void*f3;};struct Cyc_AssnDef_Kill_AssnDef_Assn_struct{int tag;void*f1;};
# 32 "vcgen.h"
struct Cyc_Hashtable_Table**Cyc_Vcgen_new_assn_info (void);
extern void Cyc_Vcgen_vcgen_fundecl(struct Cyc_JumpAnalysis_Jump_Anal_Result*,struct Cyc_Absyn_Fndecl*,struct Cyc_Hashtable_Table**);struct _tuple12{void*f1;void*f2;};
# 36
struct _tuple12 Cyc_Vcgen_exp2ctxt_assn(struct Cyc_Hashtable_Table**,struct Cyc_Absyn_Exp*);
# 8 "pratt_prover.h"
int Cyc_PrattProver_constraint_prove(void*,void*);char Cyc_InsertChecks_FatBound[9U]="FatBound";char Cyc_InsertChecks_NoCheck[8U]="NoCheck";char Cyc_InsertChecks_NullAndFatBound[16U]="NullAndFatBound";char Cyc_InsertChecks_NullAndThinBound[17U]="NullAndThinBound";char Cyc_InsertChecks_NullOnly[9U]="NullOnly";char Cyc_InsertChecks_ThinBound[10U]="ThinBound";
# 63 "insert_checks.cyc"
struct Cyc_InsertChecks_NoCheck_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NoCheck_val={Cyc_InsertChecks_NoCheck};
struct Cyc_InsertChecks_NullOnly_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullOnly_val={Cyc_InsertChecks_NullOnly};
struct Cyc_InsertChecks_NullAndFatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_NullAndFatBound_val={Cyc_InsertChecks_NullAndFatBound};
struct Cyc_InsertChecks_FatBound_Absyn_AbsynAnnot_struct Cyc_InsertChecks_FatBound_val={Cyc_InsertChecks_FatBound};
# 69
unsigned Cyc_InsertChecks_total_bounds_checks=0U;
unsigned Cyc_InsertChecks_bounds_checks_eliminated=0U;
# 72
static struct Cyc_Absyn_Exp*Cyc_InsertChecks_shared_zero_exp (void){
static struct Cyc_Absyn_Exp*ans=0;
if(!((unsigned)ans))
ans=Cyc_Absyn_uint_exp(0U,0U);
return(struct Cyc_Absyn_Exp*)_check_null(ans);}struct Cyc_InsertChecks_Env{struct Cyc_JumpAnalysis_Jump_Anal_Result*tables;struct Cyc_Hashtable_Table**assn_info;int in_sizeof: 1;int at_toplevel: 1;int vcgen_r: 1;};
# 88
static struct Cyc_InsertChecks_Env Cyc_InsertChecks_toplevel_env(struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,int do_vcgen){
return({struct Cyc_InsertChecks_Env _tmp65;_tmp65.tables=tables,_tmp65.assn_info=0,_tmp65.in_sizeof=0,_tmp65.at_toplevel=1,_tmp65.vcgen_r=do_vcgen;_tmp65;});}
# 92
static struct Cyc_InsertChecks_Env Cyc_InsertChecks_fn_body_env(struct Cyc_InsertChecks_Env nv){
return({struct Cyc_InsertChecks_Env _tmp66;_tmp66.tables=nv.tables,_tmp66.assn_info=nv.assn_info,_tmp66.in_sizeof=0,_tmp66.at_toplevel=0,_tmp66.vcgen_r=nv.vcgen_r;_tmp66;});}
# 96
static struct Cyc_InsertChecks_Env Cyc_InsertChecks_enter_sizeof(struct Cyc_InsertChecks_Env nv){
return({struct Cyc_InsertChecks_Env _tmp67;_tmp67.tables=nv.tables,_tmp67.assn_info=nv.assn_info,_tmp67.in_sizeof=1,_tmp67.at_toplevel=nv.at_toplevel,_tmp67.vcgen_r=nv.vcgen_r;_tmp67;});}
# 102
static int Cyc_InsertChecks_need_null_check(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*e){
if(nv.in_sizeof)
return 0;
# 106
{void*_tmp0=Cyc_Absyn_compress((void*)_check_null(e->topt));void*_stmttmp0=_tmp0;void*_tmp1=_stmttmp0;void*_tmp2;if(*((int*)_tmp1)== 3){_tmp2=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp1)->f1).ptr_atts).nullable;{void*n=_tmp2;
# 108
if(!Cyc_Tcutil_force_type2bool(0,n))
return 0;
goto _LL0;}}else{
({void*_tmp3=0U;({unsigned _tmp6B=e->loc;struct _fat_ptr _tmp6A=({const char*_tmp4="need_null_check: non-pointer type";_tag_fat(_tmp4,sizeof(char),34U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp6B,_tmp6A,_tag_fat(_tmp3,sizeof(void*),0));});});}_LL0:;}
# 113
{void*_tmp5=e->annot;void*_stmttmp1=_tmp5;void*_tmp6=_stmttmp1;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp6)->tag == Cyc_CfFlowInfo_UnknownZ)
goto _LL5;else{if(((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmp6)->tag == Cyc_CfFlowInfo_NotZero)
return 0;else{if(((struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct*)_tmp6)->tag == Cyc_CfFlowInfo_IsZero){
# 117
({void*_tmp7=0U;({unsigned _tmp6D=e->loc;struct _fat_ptr _tmp6C=({const char*_tmp8="NULL pointer check will definitely fail";_tag_fat(_tmp8,sizeof(char),40U);});Cyc_Warn_err(_tmp6D,_tmp6C,_tag_fat(_tmp7,sizeof(void*),0));});});
return 0;}else{if(((struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct*)_tmp6)->tag == Cyc_Absyn_EmptyAnnot)
# 120
return 0;else{
({void*_tmp9=0U;({unsigned _tmp6F=e->loc;struct _fat_ptr _tmp6E=({const char*_tmpA="need_null_check: unexpected annotation";_tag_fat(_tmpA,sizeof(char),39U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp6F,_tmp6E,_tag_fat(_tmp9,sizeof(void*),0));});});}}}}_LL5:;}
# 123
if(nv.at_toplevel){
({void*_tmpB=0U;({unsigned _tmp71=e->loc;struct _fat_ptr _tmp70=({const char*_tmpC="cannot perform NULL-pointer check at toplevel";_tag_fat(_tmpC,sizeof(char),46U);});Cyc_Warn_err(_tmp71,_tmp70,_tag_fat(_tmpB,sizeof(void*),0));});});
return 0;}
# 127
if(Cyc_Flags_warn_all_null_deref)
({void*_tmpD=0U;({unsigned _tmp73=e->loc;struct _fat_ptr _tmp72=({const char*_tmpE="inserted null check";_tag_fat(_tmpE,sizeof(char),20U);});Cyc_Warn_warn(_tmp73,_tmp72,_tag_fat(_tmpD,sizeof(void*),0));});});
return 1;}
# 132
static struct Cyc_List_List*Cyc_InsertChecks_get_relns(struct Cyc_Absyn_Exp*e){
void*_tmpF=e->annot;void*_stmttmp2=_tmpF;void*_tmp10=_stmttmp2;void*_tmp11;if(((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp10)->tag == Cyc_CfFlowInfo_UnknownZ){_tmp11=((struct Cyc_CfFlowInfo_UnknownZ_Absyn_AbsynAnnot_struct*)_tmp10)->f1;{struct Cyc_List_List*r=_tmp11;
return r;}}else{if(((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmp10)->tag == Cyc_CfFlowInfo_NotZero){_tmp11=((struct Cyc_CfFlowInfo_NotZero_Absyn_AbsynAnnot_struct*)_tmp10)->f1;{struct Cyc_List_List*r=_tmp11;
return r;}}else{if(((struct Cyc_CfFlowInfo_IsZero_Absyn_AbsynAnnot_struct*)_tmp10)->tag == Cyc_CfFlowInfo_IsZero)
return 0;else{if(((struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct*)_tmp10)->tag == Cyc_Absyn_EmptyAnnot)
return 0;else{
({void*_tmp12=0U;({unsigned _tmp75=e->loc;struct _fat_ptr _tmp74=({const char*_tmp13="get_relns: unexpected annotation";_tag_fat(_tmp13,sizeof(char),33U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp75,_tmp74,_tag_fat(_tmp12,sizeof(void*),0));});});}}}};}
# 145
static int Cyc_InsertChecks_need_bounds_check(struct Cyc_InsertChecks_Env nv,struct Cyc_List_List*relns,struct Cyc_Absyn_Exp*a,struct Cyc_Absyn_Exp*i){
if(nv.in_sizeof)
return 0;
if(nv.at_toplevel)
({void*_tmp14=0U;({unsigned _tmp77=a->loc;struct _fat_ptr _tmp76=({const char*_tmp15="InsertChecks: memory read at top-level";_tag_fat(_tmp15,sizeof(char),39U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp77,_tmp76,_tag_fat(_tmp14,sizeof(void*),0));});});
++ Cyc_InsertChecks_total_bounds_checks;{
# 157
int bd_valid=0;union Cyc_Relations_RelnOp rop_bd=Cyc_Relations_RConst(0U);
int a_valid=0;union Cyc_Relations_RelnOp rop_a=Cyc_Relations_RConst(0U);
int i_valid=0;union Cyc_Relations_RelnOp rop_i=Cyc_Relations_RConst(0U);
int z_valid=0;union Cyc_Relations_RelnOp rop_z=Cyc_Relations_RConst(0U);
# 163
struct Cyc_Absyn_Exp*bound=Cyc_Tcutil_get_type_bound((void*)_check_null(a->topt));
if(bound != 0)
bd_valid=Cyc_Relations_exp2relnop(bound,& rop_bd);
# 167
{void*_tmp16=a->r;void*_stmttmp3=_tmp16;void*_tmp17=_stmttmp3;void*_tmp18;if(*((int*)_tmp17)== 1){_tmp18=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp17)->f1;{void*b=_tmp18;
# 169
struct Cyc_Absyn_Vardecl*_tmp19=Cyc_Tcutil_nonesc_vardecl(b);struct Cyc_Absyn_Vardecl*x=_tmp19;
if(x != 0){
a_valid=1;
rop_a=Cyc_Relations_RNumelts(x);}
# 174
goto _LL0;}}else{
goto _LL0;}_LL0:;}
# 178
i_valid=Cyc_Relations_exp2relnop(i,& rop_i);
# 180
if(!i_valid){
inner_loop: {
void*_tmp1A=i->r;void*_stmttmp4=_tmp1A;void*_tmp1B=_stmttmp4;void*_tmp1D;void*_tmp1C;switch(*((int*)_tmp1B)){case 14: _tmp1C=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp1B)->f1;_tmp1D=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp1B)->f2;{void*t=_tmp1C;struct Cyc_Absyn_Exp*e2=_tmp1D;
i=e2;goto inner_loop;}case 3: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1B)->f1 == Cyc_Absyn_Mod){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1B)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1B)->f2)->tl != 0){_tmp1C=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1B)->f2)->hd;_tmp1D=(struct Cyc_Absyn_Exp*)((((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp1B)->f2)->tl)->hd;{struct Cyc_Absyn_Exp*e1=_tmp1C;struct Cyc_Absyn_Exp*e2=_tmp1D;
# 185
z_valid=Cyc_Relations_exp2relnop(e2,& rop_z);goto _LL5;}}else{goto _LLA;}}else{goto _LLA;}}else{goto _LLA;}default: _LLA:
 goto _LL5;}_LL5:;}}{
# 191
union Cyc_Relations_RelnOp rop_left;
# 195
if(!bd_valid && !a_valid)
return 1;
if(bd_valid && a_valid){
relns=Cyc_Relations_add_relation(Cyc_Core_heap_region,rop_bd,Cyc_Relations_Rlte,rop_a,relns);
rop_left=rop_a;}else{
if(a_valid)
rop_left=rop_a;else{
# 203
rop_left=rop_bd;}}
# 205
if(i_valid)relns=Cyc_Relations_add_relation(Cyc_Core_heap_region,rop_left,Cyc_Relations_Rlte,rop_i,relns);else{
if(z_valid)relns=Cyc_Relations_add_relation(Cyc_Core_heap_region,rop_left,Cyc_Relations_Rlt,rop_z,relns);else{
return 1;}}{
# 212
int ans=Cyc_Relations_consistent_relations(relns);
# 216
if(!ans)
++ Cyc_InsertChecks_bounds_checks_eliminated;
if(Cyc_Flags_warn_bounds_checks && ans)
({void*_tmp1E=0U;({unsigned _tmp79=a->loc;struct _fat_ptr _tmp78=({const char*_tmp1F="inserted bounds check (location is pointer expression)";_tag_fat(_tmp1F,sizeof(char),55U);});Cyc_Warn_warn(_tmp79,_tmp78,_tag_fat(_tmp1E,sizeof(void*),0));});});
return ans;}}}}
# 223
static void Cyc_InsertChecks_do_stmt(struct Cyc_InsertChecks_Env,struct Cyc_Absyn_Stmt*);
static void Cyc_InsertChecks_do_exp(struct Cyc_InsertChecks_Env,struct Cyc_Absyn_Exp*);
# 226
static void Cyc_InsertChecks_do_exp_opt(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*eo){
if(eo != 0)
Cyc_InsertChecks_do_exp(nv,eo);}
# 231
static int Cyc_InsertChecks_need_bounds_check_vcgen(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*a){
struct _tuple12 _tmp20=Cyc_Vcgen_exp2ctxt_assn(nv.assn_info,a);struct _tuple12 _stmttmp5=_tmp20;struct _tuple12 _tmp21=_stmttmp5;void*_tmp23;void*_tmp22;_tmp22=_tmp21.f1;_tmp23=_tmp21.f2;{void*ctxt=_tmp22;void*assn=_tmp23;
return !Cyc_PrattProver_constraint_prove(ctxt,assn);}}struct _tuple13{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple14{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 244 "insert_checks.cyc"
static void Cyc_InsertChecks_do_exp(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Exp*e){
# 246
void*_tmp24=e->r;void*_stmttmp6=_tmp24;void*_tmp25=_stmttmp6;enum Cyc_Absyn_Coercion _tmp29;int _tmp28;enum Cyc_Absyn_Incrementor _tmp2A;void*_tmp2B;void*_tmp27;void*_tmp26;switch(*((int*)_tmp25)){case 40:
 goto _LL4;case 0: _LL4:
 goto _LL6;case 1: _LL6:
 goto _LL8;case 2: _LL8:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL0;case 31: _tmp26=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp25)->f1;{struct Cyc_List_List*es=_tmp26;
# 252
_tmp26=es;goto _LLC;}case 24: _tmp26=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LLC: {struct Cyc_List_List*es=_tmp26;
_tmp26=es;goto _LLE;}case 3: _tmp26=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_LLE: {struct Cyc_List_List*es=_tmp26;
# 255
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
for(1;es != 0;es=es->tl){
Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)es->hd);}
goto _LL0;}case 4: _tmp26=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp25)->f3;{struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
# 262
_tmp26=e1;_tmp27=e2;goto _LL12;}case 9: _tmp26=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
_tmp26=e1;_tmp27=e2;goto _LL14;}case 7: _tmp26=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
_tmp26=e1;_tmp27=e2;goto _LL16;}case 8: _tmp26=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
# 266
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
Cyc_InsertChecks_do_exp(nv,e1);
Cyc_InsertChecks_do_exp(nv,e2);
goto _LL0;}case 6: _tmp26=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_tmp2B=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp25)->f3;{struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;struct Cyc_Absyn_Exp*e3=_tmp2B;
# 271
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
Cyc_InsertChecks_do_exp(nv,e1);
Cyc_InsertChecks_do_exp(nv,e2);
Cyc_InsertChecks_do_exp(nv,e3);
goto _LL0;}case 5: _tmp26=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp2A=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp25)->f2;{struct Cyc_Absyn_Exp*e2=_tmp26;enum Cyc_Absyn_Incrementor incr=_tmp2A;
# 282
if((Cyc_Tcutil_get_type_bound((void*)_check_null(e2->topt))!= 0 &&
 Cyc_Tcutil_is_zeroterm_pointer_type((void*)_check_null(e2->topt)))&&
 Cyc_InsertChecks_need_null_check(nv,e2))
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;else{
# 287
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
Cyc_InsertChecks_do_exp(nv,e2);
goto _LL0;}case 10: _tmp26=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp25)->f2;{struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_List_List*es=_tmp27;
# 292
if(Cyc_InsertChecks_need_null_check(nv,e1))
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;else{
# 295
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
Cyc_InsertChecks_do_exp(nv,e1);
for(1;es != 0;es=es->tl){
Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)es->hd);}
goto _LL0;}case 14: _tmp26=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_tmp28=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp25)->f3;_tmp29=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp25)->f4;{void*t=_tmp26;struct Cyc_Absyn_Exp*e1=_tmp27;int user_inserted=_tmp28;enum Cyc_Absyn_Coercion coercion=_tmp29;
# 304
{struct _tuple12 _tmp2C=({struct _tuple12 _tmp69;({void*_tmp7B=Cyc_Absyn_compress((void*)_check_null(e1->topt));_tmp69.f1=_tmp7B;}),({void*_tmp7A=Cyc_Absyn_compress(t);_tmp69.f2=_tmp7A;});_tmp69;});struct _tuple12 _stmttmp7=_tmp2C;struct _tuple12 _tmp2D=_stmttmp7;struct Cyc_Absyn_PtrInfo _tmp2F;struct Cyc_Absyn_PtrInfo _tmp2E;if(*((int*)_tmp2D.f1)== 3){if(*((int*)_tmp2D.f2)== 3){_tmp2E=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D.f1)->f1;_tmp2F=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2D.f2)->f1;{struct Cyc_Absyn_PtrInfo p1=_tmp2E;struct Cyc_Absyn_PtrInfo p2=_tmp2F;
# 306
int _tmp30=Cyc_Tcutil_force_type2bool(0,(p1.ptr_atts).nullable);int n1=_tmp30;
int _tmp31=Cyc_Tcutil_force_type2bool(0,(p2.ptr_atts).nullable);int n2=_tmp31;
struct Cyc_Absyn_Exp*_tmp32=({void*_tmp7C=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmp7C,(p1.ptr_atts).bounds);});struct Cyc_Absyn_Exp*b1=_tmp32;
struct Cyc_Absyn_Exp*_tmp33=({void*_tmp7D=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmp7D,(p2.ptr_atts).bounds);});struct Cyc_Absyn_Exp*b2=_tmp33;
if((n1 && !n2)&&(int)coercion != (int)2U)
({void*_tmp34=0U;({unsigned _tmp7F=e->loc;struct _fat_ptr _tmp7E=({const char*_tmp35="null-check conversion mis-classified";_tag_fat(_tmp35,sizeof(char),37U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp7F,_tmp7E,_tag_fat(_tmp34,sizeof(void*),0));});});else{
if(((unsigned)b1 && !((unsigned)b2))&&(int)coercion == (int)2U)
({void*_tmp36=0U;({unsigned _tmp81=e->loc;struct _fat_ptr _tmp80=({const char*_tmp37="conversion mis-classified as null-check";_tag_fat(_tmp37,sizeof(char),40U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp81,_tmp80,_tag_fat(_tmp36,sizeof(void*),0));});});}
{struct _tuple13 _tmp38=({struct _tuple13 _tmp68;_tmp68.f1=b1,_tmp68.f2=b2;_tmp68;});struct _tuple13 _stmttmp8=_tmp38;struct _tuple13 _tmp39=_stmttmp8;if(_tmp39.f1 != 0){if(_tmp39.f2 != 0){
# 316
if(!({struct Cyc_Absyn_Exp*_tmp82=(struct Cyc_Absyn_Exp*)_check_null(b2);Cyc_Evexp_lte_const_exp(_tmp82,(struct Cyc_Absyn_Exp*)_check_null(b1));}))
({void*_tmp3A=0U;({unsigned _tmp84=e->loc;struct _fat_ptr _tmp83=({const char*_tmp3B="InsertChecks: cast to maybe-shorter pointer type";_tag_fat(_tmp3B,sizeof(char),49U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp84,_tmp83,_tag_fat(_tmp3A,sizeof(void*),0));});});
if((n1 && !n2)&& Cyc_InsertChecks_need_null_check(nv,e1)){
if(!user_inserted)
({void*_tmp3C=0U;({unsigned _tmp86=e->loc;struct _fat_ptr _tmp85=({const char*_tmp3D="inserted null check due to implicit cast from * to @ type";_tag_fat(_tmp3D,sizeof(char),58U);});Cyc_Warn_warn(_tmp86,_tmp85,_tag_fat(_tmp3C,sizeof(void*),0));});});
# 322
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;}else{
# 324
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}
goto _LL5C;}else{
# 328
if(!Cyc_Evexp_c_can_eval((struct Cyc_Absyn_Exp*)_check_null(b1)))
({void*_tmp3E=0U;({unsigned _tmp88=e->loc;struct _fat_ptr _tmp87=({const char*_tmp3F="cannot perform coercion: numelts is statically unknown";_tag_fat(_tmp3F,sizeof(char),55U);});Cyc_Warn_err(_tmp88,_tmp87,_tag_fat(_tmp3E,sizeof(void*),0));});});
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
goto _LL5C;}}else{if(_tmp39.f2 != 0){
# 333
if(nv.at_toplevel)
({void*_tmp40=0U;({unsigned _tmp8A=e->loc;struct _fat_ptr _tmp89=({const char*_tmp41="cannot coerce fat-pointer to thin-pointer at toplevel";_tag_fat(_tmp41,sizeof(char),54U);});Cyc_Warn_err(_tmp8A,_tmp89,_tag_fat(_tmp40,sizeof(void*),0));});});
# 336
if(!Cyc_Evexp_c_can_eval((struct Cyc_Absyn_Exp*)_check_null(b2)))
({void*_tmp42=0U;({unsigned _tmp8C=e->loc;struct _fat_ptr _tmp8B=({const char*_tmp43="cannot perform coercion: numelts is statically unknown";_tag_fat(_tmp43,sizeof(char),55U);});Cyc_Warn_err(_tmp8C,_tmp8B,_tag_fat(_tmp42,sizeof(void*),0));});});
# 340
++ Cyc_InsertChecks_total_bounds_checks;{
int bds_chk=0;
if(!n2 && Cyc_InsertChecks_need_null_check(nv,e1)){
if(!user_inserted)
({void*_tmp44=0U;({unsigned _tmp8E=e->loc;struct _fat_ptr _tmp8D=({const char*_tmp45="inserted null check due to implicit cast from *@fat (?) to @ type";_tag_fat(_tmp45,sizeof(char),66U);});Cyc_Warn_warn(_tmp8E,_tmp8D,_tag_fat(_tmp44,sizeof(void*),0));});});
# 346
e->annot=bds_chk?(void*)& Cyc_InsertChecks_NullAndFatBound_val:(void*)& Cyc_InsertChecks_NullOnly_val;}else{
# 348
e->annot=bds_chk?(void*)& Cyc_InsertChecks_FatBound_val:(void*)& Cyc_InsertChecks_NoCheck_val;}
goto _LL5C;}}else{
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL5C;}}_LL5C:;}
# 352
goto _LL57;}}else{goto _LL5A;}}else{_LL5A:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL57;}_LL57:;}
# 355
Cyc_InsertChecks_do_exp(nv,e1);
goto _LL0;}case 22: _tmp26=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp25)->f1;{struct Cyc_Absyn_Exp*e1=_tmp26;
# 358
_tmp26=e1;goto _LL22;}case 20: _tmp26=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp26;
_tmp26=e1;_tmp27=Cyc_InsertChecks_shared_zero_exp();goto _LL24;}case 23: _tmp26=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
# 361
{void*_tmp46=Cyc_Absyn_compress((void*)_check_null(e1->topt));void*_stmttmp9=_tmp46;void*_tmp47=_stmttmp9;void*_tmp4D;void*_tmp4C;void*_tmp4B;void*_tmp4A;struct Cyc_Absyn_Tqual _tmp49;void*_tmp48;switch(*((int*)_tmp47)){case 6:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL65;case 3: _tmp48=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp47)->f1).elt_type;_tmp49=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp47)->f1).elt_tq;_tmp4A=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp47)->f1).ptr_atts).rgn;_tmp4B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp47)->f1).ptr_atts).nullable;_tmp4C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp47)->f1).ptr_atts).bounds;_tmp4D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp47)->f1).ptr_atts).zero_term;{void*ta=_tmp48;struct Cyc_Absyn_Tqual tq=_tmp49;void*rgn=_tmp4A;void*n=_tmp4B;void*b=_tmp4C;void*zt=_tmp4D;
# 364
int bds_ck=({struct Cyc_InsertChecks_Env _tmp91=nv;struct Cyc_List_List*_tmp90=Cyc_InsertChecks_get_relns(e1);struct Cyc_Absyn_Exp*_tmp8F=e1;Cyc_InsertChecks_need_bounds_check(_tmp91,_tmp90,_tmp8F,e2);});
int bds_ck_vcgen;
if(nv.vcgen_r)
bds_ck_vcgen=Cyc_InsertChecks_need_bounds_check_vcgen(nv,e1);{
# 378 "insert_checks.cyc"
struct Cyc_Absyn_Exp*bd=({void*_tmp92=Cyc_Absyn_bounds_one();Cyc_Tcutil_get_bounds_exp(_tmp92,b);});
int fat_ptr=bd == 0;
int null_ck;
# 383
if(fat_ptr && !bds_ck)null_ck=0;else{
null_ck=Cyc_InsertChecks_need_null_check(nv,e1);}
if(bds_ck){
if((unsigned)bd){
if(!Cyc_Evexp_c_can_eval(bd)){
({void*_tmp4E=0U;({unsigned _tmp94=e->loc;struct _fat_ptr _tmp93=({const char*_tmp4F="cannot determine subscript is in bounds";_tag_fat(_tmp4F,sizeof(char),40U);});Cyc_Warn_err(_tmp94,_tmp93,_tag_fat(_tmp4E,sizeof(void*),0));});});
({void*_tmp50=0U;({struct Cyc___cycFILE*_tmp96=Cyc_stderr;struct _fat_ptr _tmp95=({const char*_tmp51="[all that I can prove is ";_tag_fat(_tmp51,sizeof(char),26U);});Cyc_fprintf(_tmp96,_tmp95,_tag_fat(_tmp50,sizeof(void*),0));});});
({struct Cyc___cycFILE*_tmp97=Cyc_stderr;Cyc_Relations_print_relns(_tmp97,Cyc_InsertChecks_get_relns(e1));});
({void*_tmp52=0U;({struct Cyc___cycFILE*_tmp99=Cyc_stderr;struct _fat_ptr _tmp98=({const char*_tmp53="]\n";_tag_fat(_tmp53,sizeof(char),3U);});Cyc_fprintf(_tmp99,_tmp98,_tag_fat(_tmp52,sizeof(void*),0));});});}
# 393
({void*_tmp9A=
null_ck?(void*)({struct Cyc_InsertChecks_NullAndThinBound_Absyn_AbsynAnnot_struct*_tmp54=_cycalloc(sizeof(*_tmp54));_tmp54->tag=Cyc_InsertChecks_NullAndThinBound,_tmp54->f1=bd;_tmp54;}):(void*)({struct Cyc_InsertChecks_ThinBound_Absyn_AbsynAnnot_struct*_tmp55=_cycalloc(sizeof(*_tmp55));_tmp55->tag=Cyc_InsertChecks_ThinBound,_tmp55->f1=bd;_tmp55;});
# 393
e->annot=_tmp9A;});}else{
# 396
e->annot=null_ck?(void*)& Cyc_InsertChecks_NullAndFatBound_val:(void*)& Cyc_InsertChecks_FatBound_val;}}else{
if(null_ck)
e->annot=(void*)& Cyc_InsertChecks_NullOnly_val;else{
# 400
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;}}
goto _LL65;}}default:
({void*_tmp56=0U;({unsigned _tmp9C=e->loc;struct _fat_ptr _tmp9B=({const char*_tmp57="InsertChecks: non-pointer-type on dereference";_tag_fat(_tmp57,sizeof(char),46U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp9C,_tmp9B,_tag_fat(_tmp56,sizeof(void*),0));});});}_LL65:;}
# 404
Cyc_InsertChecks_do_exp(nv,e1);
Cyc_InsertChecks_do_exp(nv,e2);
goto _LL0;}case 18: _tmp26=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp25)->f1;{struct Cyc_Absyn_Exp*e1=_tmp26;
# 409
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
({struct Cyc_InsertChecks_Env _tmp9D=Cyc_InsertChecks_enter_sizeof(nv);Cyc_InsertChecks_do_exp(_tmp9D,e1);});
goto _LL0;}case 16: _tmp26=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp25)->f2;{struct Cyc_Absyn_Exp*rgnopt=_tmp26;struct Cyc_Absyn_Exp*e1=_tmp27;
# 414
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
Cyc_InsertChecks_do_exp_opt(nv,rgnopt);
Cyc_InsertChecks_do_exp(nv,e1);
goto _LL0;}case 34: _tmp26=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp25)->f1).rgn;_tmp27=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp25)->f1).num_elts;{struct Cyc_Absyn_Exp*rgnopt=_tmp26;struct Cyc_Absyn_Exp*sizeexp=_tmp27;
# 420
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
Cyc_InsertChecks_do_exp_opt(nv,rgnopt);
Cyc_InsertChecks_do_exp(nv,sizeexp);
goto _LL0;}case 41: _tmp26=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp25)->f1;{struct Cyc_Absyn_Exp*e1=_tmp26;
# 425
_tmp26=e1;goto _LL2E;}case 11: _tmp26=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp26;
_tmp26=e1;goto _LL30;}case 12: _tmp26=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp26;
_tmp26=e1;goto _LL32;}case 13: _tmp26=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp26;
_tmp26=e1;goto _LL34;}case 15: _tmp26=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp26;
_tmp26=e1;goto _LL36;}case 42: _tmp26=((struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp26;
# 431
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
Cyc_InsertChecks_do_exp(nv,e1);
goto _LL0;}case 32:
 goto _LL3A;case 33: _LL3A:
 goto _LL3C;case 17: _LL3C:
 goto _LL3E;case 19: _LL3E:
 e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;goto _LL0;case 21: _tmp26=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp25)->f1;{struct Cyc_Absyn_Exp*e1=_tmp26;
_tmp26=e1;goto _LL42;}case 38: _tmp26=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL42: {struct Cyc_Absyn_Exp*e1=_tmp26;
_tmp26=e1;goto _LL44;}case 28: _tmp26=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_LL44: {struct Cyc_Absyn_Exp*e1=_tmp26;
# 441
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
Cyc_InsertChecks_do_exp(nv,e1);
goto _LL0;}case 27: _tmp26=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_tmp27=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp25)->f3;{struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
_tmp26=e1;_tmp27=e2;goto _LL48;}case 35: _tmp26=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp25)->f1;_tmp27=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_LL48: {struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
# 446
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
Cyc_InsertChecks_do_exp(nv,e1);
Cyc_InsertChecks_do_exp(nv,e2);
goto _LL0;}case 26: _tmp26=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp25)->f1;{struct Cyc_List_List*dles=_tmp26;
_tmp26=dles;goto _LL4C;}case 29: _tmp26=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp25)->f3;_LL4C: {struct Cyc_List_List*dles=_tmp26;
_tmp26=dles;goto _LL4E;}case 30: _tmp26=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp25)->f2;_LL4E: {struct Cyc_List_List*dles=_tmp26;
# 453
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;
for(1;dles != 0;dles=dles->tl){
Cyc_InsertChecks_do_exp(nv,(*((struct _tuple14*)dles->hd)).f2);}
goto _LL0;}case 37: _tmp26=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp25)->f1;{struct Cyc_Absyn_Stmt*s=_tmp26;
e->annot=(void*)& Cyc_InsertChecks_NoCheck_val;Cyc_InsertChecks_do_stmt(nv,s);goto _LL0;}case 36:
# 459
 goto _LL54;case 25: _LL54:
 goto _LL56;default: _LL56:
({void*_tmp58=0U;({unsigned _tmp9F=e->loc;struct _fat_ptr _tmp9E=({const char*_tmp59="InsertChecks, unexpected exp form";_tag_fat(_tmp59,sizeof(char),34U);});((int(*)(unsigned,struct _fat_ptr,struct _fat_ptr))Cyc_Warn_impos_loc)(_tmp9F,_tmp9E,_tag_fat(_tmp58,sizeof(void*),0));});});}_LL0:;}
# 465
static void Cyc_InsertChecks_do_decl(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Decl*d){
void*_tmp5A=d->r;void*_stmttmpA=_tmp5A;void*_tmp5B=_stmttmpA;void*_tmp5C;switch(*((int*)_tmp5B)){case 0: _tmp5C=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp5B)->f1;{struct Cyc_Absyn_Vardecl*vd=_tmp5C;
Cyc_InsertChecks_do_exp_opt(nv,vd->initializer);goto _LL0;}case 1: _tmp5C=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp5B)->f1;{struct Cyc_Absyn_Fndecl*fd=_tmp5C;
# 469
struct Cyc_InsertChecks_Env _tmp5D=Cyc_InsertChecks_fn_body_env(nv);struct Cyc_InsertChecks_Env nv=_tmp5D;
({struct Cyc_Hashtable_Table**_tmpA0=Cyc_Vcgen_new_assn_info();nv.assn_info=_tmpA0;});
if(nv.vcgen_r)
Cyc_Vcgen_vcgen_fundecl(nv.tables,fd,nv.assn_info);
Cyc_InsertChecks_do_stmt(nv,fd->body);
goto _LL0;}case 2: _tmp5C=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp5B)->f3;{struct Cyc_Absyn_Exp*e=_tmp5C;
Cyc_InsertChecks_do_exp(nv,e);goto _LL0;}case 3:
 goto _LL0;case 4: _tmp5C=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp5B)->f3;{struct Cyc_Absyn_Exp*eo=_tmp5C;
Cyc_InsertChecks_do_exp_opt(nv,eo);goto _LL0;}case 5:
 goto _LLE;case 6: _LLE:
 goto _LL10;case 7: _LL10:
 goto _LL12;case 8: _LL12:
 goto _LL14;case 13: _LL14:
 goto _LL16;case 14: _LL16:
 goto _LL18;case 15: _LL18:
 goto _LL1A;case 16: _LL1A:
 goto _LL1C;case 12: _LL1C:
 goto _LL0;case 11: _tmp5C=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp5B)->f1;{struct Cyc_List_List*ds=_tmp5C;
_tmp5C=ds;goto _LL20;}case 10: _tmp5C=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp5B)->f2;_LL20: {struct Cyc_List_List*ds=_tmp5C;
_tmp5C=ds;goto _LL22;}default: _tmp5C=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp5B)->f2;_LL22: {struct Cyc_List_List*ds=_tmp5C;
# 490
for(1;ds != 0;ds=ds->tl){
Cyc_InsertChecks_do_decl(nv,(struct Cyc_Absyn_Decl*)ds->hd);}}}_LL0:;}
# 495
static void Cyc_InsertChecks_do_scs(struct Cyc_InsertChecks_Env nv,struct Cyc_List_List*scs){
# 498
for(1;scs != 0;scs=scs->tl){
if(((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause != 0)
Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)_check_null(((struct Cyc_Absyn_Switch_clause*)scs->hd)->where_clause));
Cyc_InsertChecks_do_stmt(nv,((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);}}
# 505
static void Cyc_InsertChecks_do_stmt(struct Cyc_InsertChecks_Env nv,struct Cyc_Absyn_Stmt*s){
void*_tmp5E=s->r;void*_stmttmpB=_tmp5E;void*_tmp5F=_stmttmpB;void*_tmp63;void*_tmp62;void*_tmp61;void*_tmp60;switch(*((int*)_tmp5F)){case 0:
 goto _LL0;case 1: _tmp60=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;{struct Cyc_Absyn_Exp*e=_tmp60;
Cyc_InsertChecks_do_exp(nv,e);goto _LL0;}case 2: _tmp60=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;_tmp61=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2;{struct Cyc_Absyn_Stmt*s1=_tmp60;struct Cyc_Absyn_Stmt*s2=_tmp61;
Cyc_InsertChecks_do_stmt(nv,s1);Cyc_InsertChecks_do_stmt(nv,s2);goto _LL0;}case 3: _tmp60=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;{struct Cyc_Absyn_Exp*eopt=_tmp60;
if((unsigned)eopt)Cyc_InsertChecks_do_exp(nv,eopt);goto _LL0;}case 4: _tmp60=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;_tmp61=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2;_tmp62=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp5F)->f3;{struct Cyc_Absyn_Exp*e=_tmp60;struct Cyc_Absyn_Stmt*s1=_tmp61;struct Cyc_Absyn_Stmt*s2=_tmp62;
# 512
Cyc_InsertChecks_do_exp(nv,e);Cyc_InsertChecks_do_stmt(nv,s1);Cyc_InsertChecks_do_stmt(nv,s2);goto _LL0;}case 14: _tmp60=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;_tmp61=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2).f1;{struct Cyc_Absyn_Stmt*s1=_tmp60;struct Cyc_Absyn_Exp*e=_tmp61;
_tmp60=e;_tmp61=s1;goto _LLE;}case 5: _tmp60=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1).f1;_tmp61=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2;_LLE: {struct Cyc_Absyn_Exp*e=_tmp60;struct Cyc_Absyn_Stmt*s1=_tmp61;
Cyc_InsertChecks_do_exp(nv,e);Cyc_InsertChecks_do_stmt(nv,s1);goto _LL0;}case 9: _tmp60=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;_tmp61=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2).f1;_tmp62=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp5F)->f3).f1;_tmp63=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp5F)->f4;{struct Cyc_Absyn_Exp*e1=_tmp60;struct Cyc_Absyn_Exp*e2=_tmp61;struct Cyc_Absyn_Exp*e3=_tmp62;struct Cyc_Absyn_Stmt*s1=_tmp63;
# 516
Cyc_InsertChecks_do_exp(nv,e1);Cyc_InsertChecks_do_exp(nv,e2);Cyc_InsertChecks_do_exp(nv,e3);Cyc_InsertChecks_do_stmt(nv,s1);goto _LL0;}case 6:
 goto _LL14;case 7: _LL14:
 goto _LL16;case 8: _LL16:
 return;case 11: _tmp60=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;{struct Cyc_List_List*es=_tmp60;
# 521
for(1;es != 0;es=es->tl){
Cyc_InsertChecks_do_exp(nv,(struct Cyc_Absyn_Exp*)es->hd);}
goto _LL0;}case 13: _tmp60=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2;{struct Cyc_Absyn_Stmt*s1=_tmp60;
Cyc_InsertChecks_do_stmt(nv,s1);goto _LL0;}case 12: _tmp60=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;_tmp61=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2;{struct Cyc_Absyn_Decl*d=_tmp60;struct Cyc_Absyn_Stmt*s1=_tmp61;
Cyc_InsertChecks_do_decl(nv,d);Cyc_InsertChecks_do_stmt(nv,s1);goto _LL0;}case 10: _tmp60=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;_tmp61=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2;{struct Cyc_Absyn_Exp*e=_tmp60;struct Cyc_List_List*scs=_tmp61;
Cyc_InsertChecks_do_exp(nv,e);Cyc_InsertChecks_do_scs(nv,scs);goto _LL0;}default: _tmp60=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp5F)->f1;_tmp61=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp5F)->f2;{struct Cyc_Absyn_Stmt*s1=_tmp60;struct Cyc_List_List*scs=_tmp61;
Cyc_InsertChecks_do_stmt(nv,s1);Cyc_InsertChecks_do_scs(nv,scs);goto _LL0;}}_LL0:;}
# 531
void Cyc_InsertChecks_insert_checks(struct Cyc_List_List*tds,struct Cyc_JumpAnalysis_Jump_Anal_Result*tables,int do_vcgen){
# 533
struct Cyc_InsertChecks_Env _tmp64=Cyc_InsertChecks_toplevel_env(tables,do_vcgen);struct Cyc_InsertChecks_Env nv=_tmp64;
for(1;tds != 0;tds=tds->tl){
Cyc_InsertChecks_do_decl(nv,(struct Cyc_Absyn_Decl*)tds->hd);}}

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
 struct Cyc_Core_Opt{void*v;};
# 95 "core.h"
struct _fat_ptr Cyc_Core_new_string(unsigned);
# 114
void*Cyc_Core_identity(void*);
# 116
int Cyc_Core_intcmp(int,int);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 171
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 100 "cycboot.h"
extern int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_timeval{long tv_sec;long tv_usec;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
extern int Cyc_List_length(struct Cyc_List_List*);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr);
# 54 "string.h"
extern int Cyc_zstrptrcmp(struct _fat_ptr*,struct _fat_ptr*);
# 66
extern struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _fat_ptr);struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
extern struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int,int(*)(void*,void*),int(*)(void*));
# 50
extern void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*,void*,void*);
# 52
extern void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*,void*);
# 86
extern int Cyc_Hashtable_hash_stringptr(struct _fat_ptr*);struct Cyc_Fn_Function{void*(*f)(void*,void*);void*env;};
# 46 "fn.h"
extern struct Cyc_Fn_Function*Cyc_Fn_make_fn(void*(*)(void*,void*),void*);
# 49
extern struct Cyc_Fn_Function*Cyc_Fn_fp2fn(void*(*)(void*));
# 52
extern void*Cyc_Fn_apply(struct Cyc_Fn_Function*,void*);
# 39 "pp.h"
extern int Cyc_PP_tex_output;struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 67 "pp.h"
struct Cyc_PP_Doc*Cyc_PP_nil_doc (void);
# 72
struct Cyc_PP_Doc*Cyc_PP_line_doc (void);
# 74
struct Cyc_PP_Doc*Cyc_PP_oline_doc (void);
# 78
struct Cyc_PP_Doc*Cyc_PP_text(struct _fat_ptr);
# 94
struct Cyc_PP_Doc*Cyc_PP_cat(struct _fat_ptr);
# 97
struct Cyc_PP_Doc*Cyc_PP_cats(struct Cyc_List_List*);
# 103
struct Cyc_PP_Doc*Cyc_PP_doc_union(struct Cyc_PP_Doc*,struct Cyc_PP_Doc*);
# 105
struct Cyc_PP_Doc*Cyc_PP_tab(struct Cyc_PP_Doc*);
# 108
struct Cyc_PP_Doc*Cyc_PP_seq(struct _fat_ptr,struct Cyc_List_List*);
# 117
struct Cyc_PP_Doc*Cyc_PP_seql(struct _fat_ptr,struct Cyc_List_List*);struct Cyc_Xarray_Xarray{struct _fat_ptr elmts;int num_elmts;};
# 40 "xarray.h"
extern int Cyc_Xarray_length(struct Cyc_Xarray_Xarray*);
# 42
extern void*Cyc_Xarray_get(struct Cyc_Xarray_Xarray*,int);
# 45
extern void Cyc_Xarray_set(struct Cyc_Xarray_Xarray*,int,void*);
# 48
extern struct Cyc_Xarray_Xarray*Cyc_Xarray_create(int,void*);
# 66
extern void Cyc_Xarray_add(struct Cyc_Xarray_Xarray*,void*);
# 121
extern void Cyc_Xarray_reuse(struct Cyc_Xarray_Xarray*);struct Cyc_PP_Empty_PP_Alist_struct{int tag;int f1;};struct Cyc_PP_Single_PP_Alist_struct{int tag;void*f1;};struct Cyc_PP_Append_PP_Alist_struct{int tag;void*f1;void*f2;};
# 51 "pp.cyc"
struct Cyc_PP_Empty_PP_Alist_struct Cyc_PP_Empty_stringptr={0,0};
struct Cyc_PP_Empty_PP_Alist_struct Cyc_PP_Empty_link={0,0};struct _tuple0{void*f1;void*f2;};
# 54
void*Cyc_PP_append(void*a1,void*a2){
struct _tuple0 _tmp0=({struct _tuple0 _tmp9E;_tmp9E.f1=a1,_tmp9E.f2=a2;_tmp9E;});struct _tuple0 _stmttmp0=_tmp0;struct _tuple0 _tmp1=_stmttmp0;if(*((int*)_tmp1.f1)== 0)
return a2;else{if(*((int*)_tmp1.f2)== 0)
return a1;else{
return(void*)({struct Cyc_PP_Append_PP_Alist_struct*_tmp2=_cycalloc(sizeof(*_tmp2));_tmp2->tag=2,_tmp2->f1=a1,_tmp2->f2=a2;_tmp2;});}};}
# 62
struct Cyc_List_List*Cyc_PP_list_of_alist_f(void*y,struct Cyc_List_List*l){
void*_tmp3=y;void*_tmp5;void*_tmp4;void*_tmp6;switch(*((int*)_tmp3)){case 0:
 return l;case 1: _tmp6=(void*)((struct Cyc_PP_Single_PP_Alist_struct*)_tmp3)->f1;{void*z=_tmp6;
return({struct Cyc_List_List*_tmp7=_cycalloc(sizeof(*_tmp7));_tmp7->hd=z,_tmp7->tl=l;_tmp7;});}default: _tmp4=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmp3)->f1;_tmp5=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmp3)->f2;{void*a1=_tmp4;void*a2=_tmp5;
return({void*_tmpA0=a1;Cyc_PP_list_of_alist_f(_tmpA0,Cyc_PP_list_of_alist_f(a2,l));});}};}
# 69
struct Cyc_List_List*Cyc_PP_list_of_alist(void*x){
return Cyc_PP_list_of_alist_f(x,0);}struct Cyc_PP_Ppstate{int ci;int cc;int cl;int pw;int epw;};struct Cyc_PP_Out{int newcc;int newcl;void*ppout;void*links;};struct Cyc_PP_Doc{int mwo;int mw;struct Cyc_Fn_Function*f;};
# 98
static void Cyc_PP_dump_out(struct Cyc___cycFILE*f,void*al){
struct Cyc_Xarray_Xarray*_tmp8=((struct Cyc_Xarray_Xarray*(*)(int,void*))Cyc_Xarray_create)(16,al);struct Cyc_Xarray_Xarray*xarr=_tmp8;
((void(*)(struct Cyc_Xarray_Xarray*,void*))Cyc_Xarray_add)(xarr,al);{
int last=0;
while(last >= 0){
void*_tmp9=((void*(*)(struct Cyc_Xarray_Xarray*,int))Cyc_Xarray_get)(xarr,last);void*_stmttmp1=_tmp9;void*_tmpA=_stmttmp1;void*_tmpC;void*_tmpB;struct _fat_ptr*_tmpD;switch(*((int*)_tmpA)){case 0:
 -- last;goto _LL0;case 1: _tmpD=(struct _fat_ptr*)((struct Cyc_PP_Single_PP_Alist_struct*)_tmpA)->f1;{struct _fat_ptr*s=_tmpD;
-- last;({struct Cyc_String_pa_PrintArg_struct _tmp10=({struct Cyc_String_pa_PrintArg_struct _tmp9F;_tmp9F.tag=0,_tmp9F.f1=(struct _fat_ptr)((struct _fat_ptr)*s);_tmp9F;});void*_tmpE[1];_tmpE[0]=& _tmp10;({struct Cyc___cycFILE*_tmpA2=f;struct _fat_ptr _tmpA1=({const char*_tmpF="%s";_tag_fat(_tmpF,sizeof(char),3U);});Cyc_fprintf(_tmpA2,_tmpA1,_tag_fat(_tmpE,sizeof(void*),1));});});goto _LL0;}default: _tmpB=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmpA)->f1;_tmpC=(void*)((struct Cyc_PP_Append_PP_Alist_struct*)_tmpA)->f2;{void*a1=_tmpB;void*a2=_tmpC;
# 107
((void(*)(struct Cyc_Xarray_Xarray*,int,void*))Cyc_Xarray_set)(xarr,last,a2);
if(({int _tmpA3=last;_tmpA3 == ((int(*)(struct Cyc_Xarray_Xarray*))Cyc_Xarray_length)(xarr)- 1;}))
((void(*)(struct Cyc_Xarray_Xarray*,void*))Cyc_Xarray_add)(xarr,a1);else{
# 111
((void(*)(struct Cyc_Xarray_Xarray*,int,void*))Cyc_Xarray_set)(xarr,last + 1,a1);}
++ last;
goto _LL0;}}_LL0:;}
# 116
((void(*)(struct Cyc_Xarray_Xarray*))Cyc_Xarray_reuse)(xarr);}}
# 120
void Cyc_PP_file_of_doc(struct Cyc_PP_Doc*d,int w,struct Cyc___cycFILE*f){
struct Cyc_PP_Out*o=({struct Cyc_Fn_Function*_tmpA4=d->f;((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(_tmpA4,({struct Cyc_PP_Ppstate*_tmp11=_cycalloc(sizeof(*_tmp11));_tmp11->ci=0,_tmp11->cc=0,_tmp11->cl=1,_tmp11->pw=w,_tmp11->epw=w;_tmp11;}));});
Cyc_PP_dump_out(f,o->ppout);}
# 126
struct _fat_ptr Cyc_PP_string_of_doc(struct Cyc_PP_Doc*d,int w){
struct Cyc_PP_Out*o=({struct Cyc_Fn_Function*_tmpA5=d->f;((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(_tmpA5,({struct Cyc_PP_Ppstate*_tmp13=_cycalloc(sizeof(*_tmp13));_tmp13->ci=0,_tmp13->cc=0,_tmp13->cl=1,_tmp13->pw=w,_tmp13->epw=w;_tmp13;}));});
return(struct _fat_ptr)({struct Cyc_List_List*_tmpA6=((struct Cyc_List_List*(*)(void*))Cyc_PP_list_of_alist)(o->ppout);Cyc_str_sepstr(_tmpA6,({const char*_tmp12="";_tag_fat(_tmp12,sizeof(char),1U);}));});}struct _tuple1{struct _fat_ptr f1;struct Cyc_List_List*f2;};
# 133
struct _tuple1*Cyc_PP_string_and_links(struct Cyc_PP_Doc*d,int w){
struct Cyc_PP_Out*o=({struct Cyc_Fn_Function*_tmpA7=d->f;((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(_tmpA7,({struct Cyc_PP_Ppstate*_tmp16=_cycalloc(sizeof(*_tmp16));_tmp16->ci=0,_tmp16->cc=0,_tmp16->cl=1,_tmp16->pw=w,_tmp16->epw=w;_tmp16;}));});
return({struct _tuple1*_tmp15=_cycalloc(sizeof(*_tmp15));({struct _fat_ptr _tmpAA=(struct _fat_ptr)({
struct Cyc_List_List*_tmpA9=((struct Cyc_List_List*(*)(void*))Cyc_PP_list_of_alist)(o->ppout);Cyc_str_sepstr(_tmpA9,({const char*_tmp14="";_tag_fat(_tmp14,sizeof(char),1U);}));});
# 135
_tmp15->f1=_tmpAA;}),({
# 137
struct Cyc_List_List*_tmpA8=((struct Cyc_List_List*(*)(void*))Cyc_PP_list_of_alist)(o->links);_tmp15->f2=_tmpA8;});_tmp15;});}
# 140
static struct Cyc_Core_Opt*Cyc_PP_bhashtbl=0;
# 142
int Cyc_PP_tex_output=0;
# 144
struct _fat_ptr Cyc_PP_nlblanks(int i){
if(Cyc_PP_bhashtbl == 0)
Cyc_PP_bhashtbl=({struct Cyc_Core_Opt*_tmp17=_cycalloc(sizeof(*_tmp17));({struct Cyc_Hashtable_Table*_tmpAB=((struct Cyc_Hashtable_Table*(*)(int,int(*)(int,int),int(*)(int)))Cyc_Hashtable_create)(61,Cyc_Core_intcmp,(int(*)(int))Cyc_Core_identity);_tmp17->v=_tmpAB;});_tmp17;});
if(i < 0)(int)_throw((void*)({struct Cyc_Core_Failure_exn_struct*_tmp19=_cycalloc(sizeof(*_tmp19));_tmp19->tag=Cyc_Core_Failure,({struct _fat_ptr _tmpAC=({const char*_tmp18="nlblanks";_tag_fat(_tmp18,sizeof(char),9U);});_tmp19->f1=_tmpAC;});_tmp19;}));{
struct _handler_cons _tmp1A;_push_handler(& _tmp1A);{int _tmp1C=0;if(setjmp(_tmp1A.handler))_tmp1C=1;if(!_tmp1C){
{struct _fat_ptr _tmp1D=*((struct _fat_ptr*(*)(struct Cyc_Hashtable_Table*,int))Cyc_Hashtable_lookup)((struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v,i);_npop_handler(0);return _tmp1D;};_pop_handler();}else{void*_tmp1B=(void*)Cyc_Core_get_exn_thrown();void*_tmp1E=_tmp1B;void*_tmp1F;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp1E)->tag == Cyc_Core_Not_found){
# 152
if(!Cyc_PP_tex_output){
int _tmp20=i / 8;int num_tabs=_tmp20;
int _tmp21=i % 8;int num_spaces=_tmp21;
int _tmp22=(2 + num_tabs)+ num_spaces;int total=_tmp22;
struct _fat_ptr nlb=Cyc_Core_new_string((unsigned)total);
({struct _fat_ptr _tmp23=_fat_ptr_plus(nlb,sizeof(char),0);char _tmp24=*((char*)_check_fat_subscript(_tmp23,sizeof(char),0U));char _tmp25='\n';if(_get_fat_size(_tmp23,sizeof(char))== 1U &&(_tmp24 == 0 && _tmp25 != 0))_throw_arraybounds();*((char*)_tmp23.curr)=_tmp25;});
{int j=0;for(0;j < num_tabs;++ j){
({struct _fat_ptr _tmp26=_fat_ptr_plus(nlb,sizeof(char),j + 1);char _tmp27=*((char*)_check_fat_subscript(_tmp26,sizeof(char),0U));char _tmp28='\t';if(_get_fat_size(_tmp26,sizeof(char))== 1U &&(_tmp27 == 0 && _tmp28 != 0))_throw_arraybounds();*((char*)_tmp26.curr)=_tmp28;});}}
{int j=0;for(0;j < num_spaces;++ j){
({struct _fat_ptr _tmp29=_fat_ptr_plus(nlb,sizeof(char),(j + 1)+ num_tabs);char _tmp2A=*((char*)_check_fat_subscript(_tmp29,sizeof(char),0U));char _tmp2B=' ';if(_get_fat_size(_tmp29,sizeof(char))== 1U &&(_tmp2A == 0 && _tmp2B != 0))_throw_arraybounds();*((char*)_tmp29.curr)=_tmp2B;});}}
({struct Cyc_Hashtable_Table*_tmpAE=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v;int _tmpAD=i;((void(*)(struct Cyc_Hashtable_Table*,int,struct _fat_ptr*))Cyc_Hashtable_insert)(_tmpAE,_tmpAD,({unsigned _tmp2D=1;struct _fat_ptr*_tmp2C=_cycalloc(_check_times(_tmp2D,sizeof(struct _fat_ptr)));_tmp2C[0]=(struct _fat_ptr)nlb;_tmp2C;}));});
return(struct _fat_ptr)nlb;}else{
# 168
int _tmp2E=3 + i;int total=_tmp2E;
struct _fat_ptr nlb=Cyc_Core_new_string((unsigned)(total + 1));
({struct _fat_ptr _tmp2F=_fat_ptr_plus(nlb,sizeof(char),0);char _tmp30=*((char*)_check_fat_subscript(_tmp2F,sizeof(char),0U));char _tmp31='\\';if(_get_fat_size(_tmp2F,sizeof(char))== 1U &&(_tmp30 == 0 && _tmp31 != 0))_throw_arraybounds();*((char*)_tmp2F.curr)=_tmp31;});
({struct _fat_ptr _tmp32=_fat_ptr_plus(nlb,sizeof(char),1);char _tmp33=*((char*)_check_fat_subscript(_tmp32,sizeof(char),0U));char _tmp34='\\';if(_get_fat_size(_tmp32,sizeof(char))== 1U &&(_tmp33 == 0 && _tmp34 != 0))_throw_arraybounds();*((char*)_tmp32.curr)=_tmp34;});
({struct _fat_ptr _tmp35=_fat_ptr_plus(nlb,sizeof(char),2);char _tmp36=*((char*)_check_fat_subscript(_tmp35,sizeof(char),0U));char _tmp37='\n';if(_get_fat_size(_tmp35,sizeof(char))== 1U &&(_tmp36 == 0 && _tmp37 != 0))_throw_arraybounds();*((char*)_tmp35.curr)=_tmp37;});
{int j=3;for(0;j < total;++ j){
({struct _fat_ptr _tmp38=_fat_ptr_plus(nlb,sizeof(char),j);char _tmp39=*((char*)_check_fat_subscript(_tmp38,sizeof(char),0U));char _tmp3A='~';if(_get_fat_size(_tmp38,sizeof(char))== 1U &&(_tmp39 == 0 && _tmp3A != 0))_throw_arraybounds();*((char*)_tmp38.curr)=_tmp3A;});}}
({struct Cyc_Hashtable_Table*_tmpB0=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_bhashtbl))->v;int _tmpAF=i;((void(*)(struct Cyc_Hashtable_Table*,int,struct _fat_ptr*))Cyc_Hashtable_insert)(_tmpB0,_tmpAF,({unsigned _tmp3C=1;struct _fat_ptr*_tmp3B=_cycalloc(_check_times(_tmp3C,sizeof(struct _fat_ptr)));_tmp3B[0]=(struct _fat_ptr)nlb;_tmp3B;}));});
return(struct _fat_ptr)nlb;}}else{_tmp1F=_tmp1E;{void*exn=_tmp1F;(int)_rethrow(exn);}};}}}}
# 182
static struct Cyc_Core_Opt*Cyc_PP_str_hashtbl=0;
# 184
int Cyc_PP_infinity=9999999;struct _tuple2{int f1;struct _fat_ptr f2;};
# 186
static struct Cyc_PP_Out*Cyc_PP_text_doc_f(struct _tuple2*clo,struct Cyc_PP_Ppstate*st){
struct _tuple2*_tmp3D=clo;struct _fat_ptr _tmp3F;int _tmp3E;_tmp3E=_tmp3D->f1;_tmp3F=_tmp3D->f2;{int slen=_tmp3E;struct _fat_ptr s=_tmp3F;
return({struct Cyc_PP_Out*_tmp43=_cycalloc(sizeof(*_tmp43));_tmp43->newcc=st->cc + slen,_tmp43->newcl=st->cl,({
# 190
void*_tmpB2=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp42=_cycalloc(sizeof(*_tmp42));_tmp42->tag=1,({struct _fat_ptr*_tmpB1=({unsigned _tmp41=1;struct _fat_ptr*_tmp40=_cycalloc(_check_times(_tmp41,sizeof(struct _fat_ptr)));_tmp40[0]=s;_tmp40;});_tmp42->f1=_tmpB1;});_tmp42;});_tmp43->ppout=_tmpB2;}),_tmp43->links=(void*)& Cyc_PP_Empty_link;_tmp43;});}}
# 193
static struct Cyc_PP_Doc*Cyc_PP_text_doc(struct _fat_ptr s){
int slen=(int)Cyc_strlen((struct _fat_ptr)s);
return({struct Cyc_PP_Doc*_tmp45=_cycalloc(sizeof(*_tmp45));
_tmp45->mwo=slen,_tmp45->mw=Cyc_PP_infinity,({
# 198
struct Cyc_Fn_Function*_tmpB3=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*))Cyc_Fn_make_fn)(Cyc_PP_text_doc_f,({struct _tuple2*_tmp44=_cycalloc(sizeof(*_tmp44));
# 200
_tmp44->f1=slen,_tmp44->f2=s;_tmp44;}));
# 198
_tmp45->f=_tmpB3;});_tmp45;});}
# 202
struct Cyc_PP_Doc*Cyc_PP_text(struct _fat_ptr s){
struct Cyc_Hashtable_Table*t;
if(Cyc_PP_str_hashtbl == 0){
t=((struct Cyc_Hashtable_Table*(*)(int,int(*)(struct _fat_ptr*,struct _fat_ptr*),int(*)(struct _fat_ptr*)))Cyc_Hashtable_create)(101,Cyc_zstrptrcmp,Cyc_Hashtable_hash_stringptr);
Cyc_PP_str_hashtbl=({struct Cyc_Core_Opt*_tmp46=_cycalloc(sizeof(*_tmp46));_tmp46->v=t;_tmp46;});}else{
# 208
t=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_str_hashtbl))->v;}{
# 210
struct _handler_cons _tmp47;_push_handler(& _tmp47);{int _tmp49=0;if(setjmp(_tmp47.handler))_tmp49=1;if(!_tmp49){
{struct Cyc_PP_Doc*_tmp4B=({struct Cyc_Hashtable_Table*_tmpB4=t;((struct Cyc_PP_Doc*(*)(struct Cyc_Hashtable_Table*,struct _fat_ptr*))Cyc_Hashtable_lookup)(_tmpB4,({struct _fat_ptr*_tmp4A=_cycalloc(sizeof(*_tmp4A));*_tmp4A=s;_tmp4A;}));});_npop_handler(0);return _tmp4B;};_pop_handler();}else{void*_tmp48=(void*)Cyc_Core_get_exn_thrown();void*_tmp4C=_tmp48;void*_tmp4D;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp4C)->tag == Cyc_Core_Not_found){
# 214
struct Cyc_PP_Doc*d=Cyc_PP_text_doc(s);
({struct Cyc_Hashtable_Table*_tmpB6=t;struct _fat_ptr*_tmpB5=({struct _fat_ptr*_tmp4E=_cycalloc(sizeof(*_tmp4E));*_tmp4E=s;_tmp4E;});((void(*)(struct Cyc_Hashtable_Table*,struct _fat_ptr*,struct Cyc_PP_Doc*))Cyc_Hashtable_insert)(_tmpB6,_tmpB5,d);});
return d;}else{_tmp4D=_tmp4C;{void*exn=_tmp4D;(int)_rethrow(exn);}};}}}}
# 219
struct Cyc_PP_Doc*Cyc_PP_textptr(struct _fat_ptr*s){return Cyc_PP_text(*s);}
# 224
static struct Cyc_PP_Doc*Cyc_PP_text_width_doc(struct _fat_ptr s,int slen){
return({struct Cyc_PP_Doc*_tmp50=_cycalloc(sizeof(*_tmp50));
_tmp50->mwo=slen,_tmp50->mw=Cyc_PP_infinity,({
# 228
struct Cyc_Fn_Function*_tmpB7=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct _tuple2*,struct Cyc_PP_Ppstate*),struct _tuple2*))Cyc_Fn_make_fn)(Cyc_PP_text_doc_f,({struct _tuple2*_tmp4F=_cycalloc(sizeof(*_tmp4F));
# 230
_tmp4F->f1=slen,_tmp4F->f2=s;_tmp4F;}));
# 228
_tmp50->f=_tmpB7;});_tmp50;});}
# 232
static struct Cyc_Core_Opt*Cyc_PP_str_hashtbl2=0;
struct Cyc_PP_Doc*Cyc_PP_text_width(struct _fat_ptr s,int slen){
struct Cyc_Hashtable_Table*t;
if(Cyc_PP_str_hashtbl2 == 0){
t=((struct Cyc_Hashtable_Table*(*)(int,int(*)(struct _fat_ptr*,struct _fat_ptr*),int(*)(struct _fat_ptr*)))Cyc_Hashtable_create)(101,Cyc_zstrptrcmp,Cyc_Hashtable_hash_stringptr);
Cyc_PP_str_hashtbl2=({struct Cyc_Core_Opt*_tmp51=_cycalloc(sizeof(*_tmp51));_tmp51->v=t;_tmp51;});}else{
# 239
t=(struct Cyc_Hashtable_Table*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_str_hashtbl2))->v;}{
# 241
struct _handler_cons _tmp52;_push_handler(& _tmp52);{int _tmp54=0;if(setjmp(_tmp52.handler))_tmp54=1;if(!_tmp54){
{struct Cyc_PP_Doc*_tmp56=({struct Cyc_Hashtable_Table*_tmpB8=t;((struct Cyc_PP_Doc*(*)(struct Cyc_Hashtable_Table*,struct _fat_ptr*))Cyc_Hashtable_lookup)(_tmpB8,({struct _fat_ptr*_tmp55=_cycalloc(sizeof(*_tmp55));*_tmp55=s;_tmp55;}));});_npop_handler(0);return _tmp56;};_pop_handler();}else{void*_tmp53=(void*)Cyc_Core_get_exn_thrown();void*_tmp57=_tmp53;void*_tmp58;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp57)->tag == Cyc_Core_Not_found){
# 245
struct Cyc_PP_Doc*d=Cyc_PP_text_width_doc(s,slen);
({struct Cyc_Hashtable_Table*_tmpBA=t;struct _fat_ptr*_tmpB9=({struct _fat_ptr*_tmp59=_cycalloc(sizeof(*_tmp59));*_tmp59=s;_tmp59;});((void(*)(struct Cyc_Hashtable_Table*,struct _fat_ptr*,struct Cyc_PP_Doc*))Cyc_Hashtable_insert)(_tmpBA,_tmpB9,d);});
return d;}else{_tmp58=_tmp57;{void*exn=_tmp58;(int)_rethrow(exn);}};}}}}
# 251
struct Cyc_Core_Opt*Cyc_PP_nil_doc_opt=0;
struct Cyc_Core_Opt*Cyc_PP_blank_doc_opt=0;
struct Cyc_Core_Opt*Cyc_PP_line_doc_opt=0;
# 257
struct Cyc_PP_Doc*Cyc_PP_nil_doc (void){
if(Cyc_PP_nil_doc_opt == 0)
Cyc_PP_nil_doc_opt=({struct Cyc_Core_Opt*_tmp5B=_cycalloc(sizeof(*_tmp5B));({struct Cyc_PP_Doc*_tmpBB=Cyc_PP_text(({const char*_tmp5A="";_tag_fat(_tmp5A,sizeof(char),1U);}));_tmp5B->v=_tmpBB;});_tmp5B;});
return(struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_nil_doc_opt))->v;}
# 264
struct Cyc_PP_Doc*Cyc_PP_blank_doc (void){
if(Cyc_PP_blank_doc_opt == 0)
Cyc_PP_blank_doc_opt=({struct Cyc_Core_Opt*_tmp5D=_cycalloc(sizeof(*_tmp5D));({struct Cyc_PP_Doc*_tmpBC=Cyc_PP_text(({const char*_tmp5C="";_tag_fat(_tmp5C,sizeof(char),1U);}));_tmp5D->v=_tmpBC;});_tmp5D;});
return(struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_blank_doc_opt))->v;}struct _tuple3{int f1;struct _fat_ptr f2;struct _fat_ptr f3;};struct _tuple4{int f1;int f2;int f3;struct _fat_ptr f4;};
# 270
static struct Cyc_PP_Out*Cyc_PP_hyperlink_f(struct _tuple3*clo,struct Cyc_PP_Ppstate*st){
struct _tuple3*_tmp5E=clo;struct _fat_ptr _tmp61;struct _fat_ptr _tmp60;int _tmp5F;_tmp5F=_tmp5E->f1;_tmp60=_tmp5E->f2;_tmp61=_tmp5E->f3;{int slen=_tmp5F;struct _fat_ptr shrt=_tmp60;struct _fat_ptr full=_tmp61;
return({struct Cyc_PP_Out*_tmp67=_cycalloc(sizeof(*_tmp67));_tmp67->newcc=st->cc + slen,_tmp67->newcl=st->cl,({
# 274
void*_tmpC0=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp64=_cycalloc(sizeof(*_tmp64));_tmp64->tag=1,({struct _fat_ptr*_tmpBF=({unsigned _tmp63=1;struct _fat_ptr*_tmp62=_cycalloc(_check_times(_tmp63,sizeof(struct _fat_ptr)));_tmp62[0]=shrt;_tmp62;});_tmp64->f1=_tmpBF;});_tmp64;});_tmp67->ppout=_tmpC0;}),({
void*_tmpBE=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp66=_cycalloc(sizeof(*_tmp66));_tmp66->tag=1,({struct _tuple4*_tmpBD=({struct _tuple4*_tmp65=_cycalloc(sizeof(*_tmp65));_tmp65->f1=st->cl,_tmp65->f2=st->cc,_tmp65->f3=slen,_tmp65->f4=full;_tmp65;});_tmp66->f1=_tmpBD;});_tmp66;});_tmp67->links=_tmpBE;});_tmp67;});}}
# 277
struct Cyc_PP_Doc*Cyc_PP_hyperlink(struct _fat_ptr shrt,struct _fat_ptr full){
int slen=(int)Cyc_strlen((struct _fat_ptr)shrt);
return({struct Cyc_PP_Doc*_tmp69=_cycalloc(sizeof(*_tmp69));_tmp69->mwo=slen,_tmp69->mw=Cyc_PP_infinity,({
# 281
struct Cyc_Fn_Function*_tmpC1=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct _tuple3*,struct Cyc_PP_Ppstate*),struct _tuple3*))Cyc_Fn_make_fn)(Cyc_PP_hyperlink_f,({struct _tuple3*_tmp68=_cycalloc(sizeof(*_tmp68));_tmp68->f1=slen,_tmp68->f2=shrt,_tmp68->f3=full;_tmp68;}));_tmp69->f=_tmpC1;});_tmp69;});}
# 285
static struct Cyc_PP_Out*Cyc_PP_line_f(struct Cyc_PP_Ppstate*st){
return({struct Cyc_PP_Out*_tmp6D=_cycalloc(sizeof(*_tmp6D));_tmp6D->newcc=st->ci,_tmp6D->newcl=st->cl + 1,({
# 288
void*_tmpC4=(void*)({struct Cyc_PP_Single_PP_Alist_struct*_tmp6C=_cycalloc(sizeof(*_tmp6C));_tmp6C->tag=1,({struct _fat_ptr*_tmpC3=({unsigned _tmp6B=1;struct _fat_ptr*_tmp6A=_cycalloc(_check_times(_tmp6B,sizeof(struct _fat_ptr)));({struct _fat_ptr _tmpC2=Cyc_PP_nlblanks(st->ci);_tmp6A[0]=_tmpC2;});_tmp6A;});_tmp6C->f1=_tmpC3;});_tmp6C;});_tmp6D->ppout=_tmpC4;}),_tmp6D->links=(void*)& Cyc_PP_Empty_link;_tmp6D;});}
# 291
struct Cyc_PP_Doc*Cyc_PP_line_doc (void){
if(Cyc_PP_line_doc_opt == 0)
Cyc_PP_line_doc_opt=({struct Cyc_Core_Opt*_tmp6F=_cycalloc(sizeof(*_tmp6F));({struct Cyc_PP_Doc*_tmpC6=({struct Cyc_PP_Doc*_tmp6E=_cycalloc(sizeof(*_tmp6E));_tmp6E->mwo=0,_tmp6E->mw=0,({struct Cyc_Fn_Function*_tmpC5=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct Cyc_PP_Ppstate*)))Cyc_Fn_fp2fn)(Cyc_PP_line_f);_tmp6E->f=_tmpC5;});_tmp6E;});_tmp6F->v=_tmpC6;});_tmp6F;});
return(struct Cyc_PP_Doc*)((struct Cyc_Core_Opt*)_check_null(Cyc_PP_line_doc_opt))->v;}struct _tuple5{int f1;struct Cyc_PP_Doc*f2;};
# 297
static struct Cyc_PP_Out*Cyc_PP_nest_f(struct _tuple5*clo,struct Cyc_PP_Ppstate*st){
struct _tuple5*_tmp70=clo;struct Cyc_PP_Doc*_tmp72;int _tmp71;_tmp71=_tmp70->f1;_tmp72=_tmp70->f2;{int k=_tmp71;struct Cyc_PP_Doc*d=_tmp72;
struct Cyc_PP_Ppstate*st2=({struct Cyc_PP_Ppstate*_tmp73=_cycalloc(sizeof(*_tmp73));
_tmp73->ci=st->ci + k,_tmp73->cc=st->cc,_tmp73->cl=st->cl,_tmp73->pw=st->pw,_tmp73->epw=st->epw;_tmp73;});
# 305
return((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d->f,st2);}}
# 307
struct Cyc_PP_Doc*Cyc_PP_nest(int k,struct Cyc_PP_Doc*d){
return({struct Cyc_PP_Doc*_tmp75=_cycalloc(sizeof(*_tmp75));_tmp75->mwo=d->mwo,_tmp75->mw=d->mw,({
# 310
struct Cyc_Fn_Function*_tmpC7=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct _tuple5*,struct Cyc_PP_Ppstate*),struct _tuple5*))Cyc_Fn_make_fn)(Cyc_PP_nest_f,({struct _tuple5*_tmp74=_cycalloc(sizeof(*_tmp74));
# 312
_tmp74->f1=k,_tmp74->f2=d;_tmp74;}));
# 310
_tmp75->f=_tmpC7;});_tmp75;});}
# 315
int Cyc_PP_min(int x,int y){
if(x <= y)return x;else{
return y;}}
# 320
int Cyc_PP_max(int x,int y){
if(x >= y)return x;else{
return y;}}struct _tuple6{struct Cyc_PP_Doc*f1;struct Cyc_PP_Doc*f2;};
# 325
static struct Cyc_PP_Out*Cyc_PP_concat_f(struct _tuple6*clo,struct Cyc_PP_Ppstate*st){
struct _tuple6*_tmp76=clo;struct Cyc_PP_Doc*_tmp78;struct Cyc_PP_Doc*_tmp77;_tmp77=_tmp76->f1;_tmp78=_tmp76->f2;{struct Cyc_PP_Doc*d1=_tmp77;struct Cyc_PP_Doc*d2=_tmp78;
int epw2=Cyc_PP_max(st->pw - d2->mw,st->epw - d1->mwo);
struct Cyc_PP_Ppstate*st2=({struct Cyc_PP_Ppstate*_tmp7B=_cycalloc(sizeof(*_tmp7B));_tmp7B->ci=st->ci,_tmp7B->cc=st->cc,_tmp7B->cl=st->cl,_tmp7B->pw=st->pw,_tmp7B->epw=epw2;_tmp7B;});
struct Cyc_PP_Out*o1=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d1->f,st2);
struct Cyc_PP_Ppstate*st3=({struct Cyc_PP_Ppstate*_tmp7A=_cycalloc(sizeof(*_tmp7A));_tmp7A->ci=st->ci,_tmp7A->cc=o1->newcc,_tmp7A->cl=o1->newcl,_tmp7A->pw=st->pw,_tmp7A->epw=epw2;_tmp7A;});
struct Cyc_PP_Out*o2=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d2->f,st3);
struct Cyc_PP_Out*o3=({struct Cyc_PP_Out*_tmp79=_cycalloc(sizeof(*_tmp79));_tmp79->newcc=o2->newcc,_tmp79->newcl=o2->newcl,({
# 334
void*_tmpC9=((void*(*)(void*,void*))Cyc_PP_append)(o1->ppout,o2->ppout);_tmp79->ppout=_tmpC9;}),({
void*_tmpC8=((void*(*)(void*,void*))Cyc_PP_append)(o1->links,o2->links);_tmp79->links=_tmpC8;});_tmp79;});
return o3;}}
# 338
static struct Cyc_PP_Doc*Cyc_PP_concat(struct Cyc_PP_Doc*d1,struct Cyc_PP_Doc*d2){
return({struct Cyc_PP_Doc*_tmp7D=_cycalloc(sizeof(*_tmp7D));({int _tmpCC=Cyc_PP_min(d1->mw,d1->mwo + d2->mwo);_tmp7D->mwo=_tmpCC;}),({
int _tmpCB=Cyc_PP_min(d1->mw,d1->mwo + d2->mw);_tmp7D->mw=_tmpCB;}),({
struct Cyc_Fn_Function*_tmpCA=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*))Cyc_Fn_make_fn)(Cyc_PP_concat_f,({struct _tuple6*_tmp7C=_cycalloc(sizeof(*_tmp7C));_tmp7C->f1=d1,_tmp7C->f2=d2;_tmp7C;}));_tmp7D->f=_tmpCA;});_tmp7D;});}
# 343
struct Cyc_PP_Doc*Cyc_PP_cat(struct _fat_ptr l){
struct Cyc_PP_Doc*_tmp7E=Cyc_PP_nil_doc();struct Cyc_PP_Doc*d=_tmp7E;
{int i=(int)(_get_fat_size(l,sizeof(struct Cyc_PP_Doc*))- (unsigned)1);for(0;i >= 0;-- i){
d=Cyc_PP_concat(*((struct Cyc_PP_Doc**)_check_fat_subscript(l,sizeof(struct Cyc_PP_Doc*),i)),d);}}
# 348
return d;}
# 353
static struct Cyc_PP_Out*Cyc_PP_long_cats_f(struct Cyc_List_List*ds0,struct Cyc_PP_Ppstate*st){
struct Cyc_List_List*os=0;
{struct Cyc_List_List*_tmp7F=ds0;struct Cyc_List_List*ds=_tmp7F;for(0;ds != 0;ds=ds->tl){
struct Cyc_PP_Doc*d=(struct Cyc_PP_Doc*)ds->hd;
struct Cyc_PP_Out*o=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d->f,st);
st=({struct Cyc_PP_Ppstate*_tmp80=_cycalloc(sizeof(*_tmp80));_tmp80->ci=st->ci,_tmp80->cc=o->newcc,_tmp80->cl=o->newcl,_tmp80->pw=st->pw,_tmp80->epw=st->epw - d->mwo;_tmp80;});
os=({struct Cyc_List_List*_tmp81=_cycalloc(sizeof(*_tmp81));_tmp81->hd=o,_tmp81->tl=os;_tmp81;});}}{
# 361
int newcc=((struct Cyc_PP_Out*)((struct Cyc_List_List*)_check_null(os))->hd)->newcc;
int newcl=((struct Cyc_PP_Out*)os->hd)->newcl;
void*s=(void*)& Cyc_PP_Empty_stringptr;
void*links=(void*)& Cyc_PP_Empty_link;
for(1;os != 0;os=os->tl){
s=((void*(*)(void*,void*))Cyc_PP_append)(((struct Cyc_PP_Out*)os->hd)->ppout,s);
links=((void*(*)(void*,void*))Cyc_PP_append)(((struct Cyc_PP_Out*)os->hd)->links,links);}
# 369
return({struct Cyc_PP_Out*_tmp82=_cycalloc(sizeof(*_tmp82));_tmp82->newcc=newcc,_tmp82->newcl=newcl,_tmp82->ppout=s,_tmp82->links=links;_tmp82;});}}
# 371
static struct Cyc_PP_Doc*Cyc_PP_long_cats(struct Cyc_List_List*doclist){
# 375
struct Cyc_List_List*orig=doclist;
struct Cyc_PP_Doc*d=(struct Cyc_PP_Doc*)((struct Cyc_List_List*)_check_null(doclist))->hd;
doclist=doclist->tl;{
int mw=d->mw;
int mwo=d->mw;
# 381
{struct Cyc_List_List*_tmp83=doclist;struct Cyc_List_List*ds=_tmp83;for(0;ds != 0;ds=ds->tl){
int mw2=Cyc_PP_min(mw,mwo + ((struct Cyc_PP_Doc*)ds->hd)->mwo);
int mwo2=Cyc_PP_min(mw,mwo + ((struct Cyc_PP_Doc*)ds->hd)->mw);
mw=mw2;
mwo=mwo2;}}
# 387
return({struct Cyc_PP_Doc*_tmp84=_cycalloc(sizeof(*_tmp84));_tmp84->mwo=mw,_tmp84->mw=mwo,({struct Cyc_Fn_Function*_tmpCD=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct Cyc_List_List*,struct Cyc_PP_Ppstate*),struct Cyc_List_List*))Cyc_Fn_make_fn)(Cyc_PP_long_cats_f,orig);_tmp84->f=_tmpCD;});_tmp84;});}}
# 390
struct Cyc_PP_Doc*Cyc_PP_cats(struct Cyc_List_List*doclist){
if(doclist == 0)return Cyc_PP_nil_doc();else{
if(doclist->tl == 0)return(struct Cyc_PP_Doc*)doclist->hd;else{
# 394
if(((int(*)(struct Cyc_List_List*))Cyc_List_length)(doclist)> 30)return Cyc_PP_long_cats(doclist);else{
return({struct Cyc_PP_Doc*_tmpCE=(struct Cyc_PP_Doc*)doclist->hd;Cyc_PP_concat(_tmpCE,Cyc_PP_cats(doclist->tl));});}}}}
# 398
static struct Cyc_PP_Out*Cyc_PP_cats_arr_f(struct _fat_ptr*docs_ptr,struct Cyc_PP_Ppstate*st){
struct Cyc_List_List*os=0;
struct _fat_ptr docs=*docs_ptr;
int sz=(int)_get_fat_size(docs,sizeof(struct Cyc_PP_Doc*));
{int i=0;for(0;i < sz;++ i){
struct Cyc_PP_Doc*d=*((struct Cyc_PP_Doc**)_check_fat_subscript(docs,sizeof(struct Cyc_PP_Doc*),i));
struct Cyc_PP_Out*o=((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d->f,st);
st=({struct Cyc_PP_Ppstate*_tmp85=_cycalloc(sizeof(*_tmp85));_tmp85->ci=st->ci,_tmp85->cc=o->newcc,_tmp85->cl=o->newcl,_tmp85->pw=st->pw,_tmp85->epw=st->epw - d->mwo;_tmp85;});
os=({struct Cyc_List_List*_tmp86=_cycalloc(sizeof(*_tmp86));_tmp86->hd=o,_tmp86->tl=os;_tmp86;});}}{
# 408
int newcc=((struct Cyc_PP_Out*)((struct Cyc_List_List*)_check_null(os))->hd)->newcc;
int newcl=((struct Cyc_PP_Out*)os->hd)->newcl;
void*s=(void*)& Cyc_PP_Empty_stringptr;
void*links=(void*)& Cyc_PP_Empty_link;
for(1;os != 0;os=os->tl){
s=((void*(*)(void*,void*))Cyc_PP_append)(((struct Cyc_PP_Out*)os->hd)->ppout,s);
links=((void*(*)(void*,void*))Cyc_PP_append)(((struct Cyc_PP_Out*)os->hd)->links,links);}
# 416
return({struct Cyc_PP_Out*_tmp87=_cycalloc(sizeof(*_tmp87));_tmp87->newcc=newcc,_tmp87->newcl=newcl,_tmp87->ppout=s,_tmp87->links=links;_tmp87;});}}
# 419
struct Cyc_PP_Doc*Cyc_PP_cats_arr(struct _fat_ptr docs){
int sz=(int)_get_fat_size(docs,sizeof(struct Cyc_PP_Doc*));
if(sz == 0)
(int)_throw((void*)({struct Cyc_Core_Failure_exn_struct*_tmp89=_cycalloc(sizeof(*_tmp89));_tmp89->tag=Cyc_Core_Failure,({struct _fat_ptr _tmpCF=({const char*_tmp88="cats_arr -- size zero array";_tag_fat(_tmp88,sizeof(char),28U);});_tmp89->f1=_tmpCF;});_tmp89;}));{
struct Cyc_PP_Doc*d=*((struct Cyc_PP_Doc**)_check_fat_subscript(docs,sizeof(struct Cyc_PP_Doc*),0));
int mw=d->mw;
int mwo=d->mw;
{int i=1;for(0;i < sz;++ i){
int mw2=Cyc_PP_min(mw,mwo + (*((struct Cyc_PP_Doc**)_check_fat_subscript(docs,sizeof(struct Cyc_PP_Doc*),i)))->mwo);
int mwo2=Cyc_PP_min(mw,mwo + (((struct Cyc_PP_Doc**)docs.curr)[i])->mw);
mw=mw2;
mwo=mwo2;}}
# 432
return({struct Cyc_PP_Doc*_tmp8C=_cycalloc(sizeof(*_tmp8C));_tmp8C->mwo=mw,_tmp8C->mw=mwo,({struct Cyc_Fn_Function*_tmpD0=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct _fat_ptr*,struct Cyc_PP_Ppstate*),struct _fat_ptr*))Cyc_Fn_make_fn)(Cyc_PP_cats_arr_f,({unsigned _tmp8B=1;struct _fat_ptr*_tmp8A=_cycalloc(_check_times(_tmp8B,sizeof(struct _fat_ptr)));_tmp8A[0]=docs;_tmp8A;}));_tmp8C->f=_tmpD0;});_tmp8C;});}}
# 435
static struct Cyc_PP_Out*Cyc_PP_doc_union_f(struct _tuple6*clo,struct Cyc_PP_Ppstate*st){
struct _tuple6*_tmp8D=clo;struct Cyc_PP_Doc*_tmp8F;struct Cyc_PP_Doc*_tmp8E;_tmp8E=_tmp8D->f1;_tmp8F=_tmp8D->f2;{struct Cyc_PP_Doc*d=_tmp8E;struct Cyc_PP_Doc*d2=_tmp8F;
int dfits=st->cc + d->mwo <= st->epw || st->cc + d->mw <= st->pw;
# 439
if(dfits)return((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d->f,st);else{
return((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d2->f,st);}}}
# 442
struct Cyc_PP_Doc*Cyc_PP_doc_union(struct Cyc_PP_Doc*d,struct Cyc_PP_Doc*d2){
return({struct Cyc_PP_Doc*_tmp91=_cycalloc(sizeof(*_tmp91));({int _tmpD3=Cyc_PP_min(d->mwo,d2->mwo);_tmp91->mwo=_tmpD3;}),({
int _tmpD2=Cyc_PP_min(d->mw,d2->mw);_tmp91->mw=_tmpD2;}),({
struct Cyc_Fn_Function*_tmpD1=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct _tuple6*,struct Cyc_PP_Ppstate*),struct _tuple6*))Cyc_Fn_make_fn)(Cyc_PP_doc_union_f,({struct _tuple6*_tmp90=_cycalloc(sizeof(*_tmp90));_tmp90->f1=d,_tmp90->f2=d2;_tmp90;}));_tmp91->f=_tmpD1;});_tmp91;});}
# 449
struct Cyc_PP_Doc*Cyc_PP_oline_doc (void){
return({struct Cyc_PP_Doc*_tmpD4=Cyc_PP_nil_doc();Cyc_PP_doc_union(_tmpD4,Cyc_PP_line_doc());});}
# 453
static struct Cyc_PP_Out*Cyc_PP_tab_f(struct Cyc_PP_Doc*d,struct Cyc_PP_Ppstate*st){
struct Cyc_PP_Ppstate*st2=({struct Cyc_PP_Ppstate*_tmp92=_cycalloc(sizeof(*_tmp92));_tmp92->ci=st->cc,_tmp92->cc=st->cc,_tmp92->cl=st->cl,_tmp92->pw=st->pw,_tmp92->epw=st->epw;_tmp92;});
return((struct Cyc_PP_Out*(*)(struct Cyc_Fn_Function*,struct Cyc_PP_Ppstate*))Cyc_Fn_apply)(d->f,st2);}
# 457
struct Cyc_PP_Doc*Cyc_PP_tab(struct Cyc_PP_Doc*d){
struct Cyc_PP_Doc*d2=({struct Cyc_PP_Doc*_tmp93=_cycalloc(sizeof(*_tmp93));
_tmp93->mwo=d->mwo,_tmp93->mw=d->mw,({
# 461
struct Cyc_Fn_Function*_tmpD5=((struct Cyc_Fn_Function*(*)(struct Cyc_PP_Out*(*)(struct Cyc_PP_Doc*,struct Cyc_PP_Ppstate*),struct Cyc_PP_Doc*))Cyc_Fn_make_fn)(Cyc_PP_tab_f,d);_tmp93->f=_tmpD5;});_tmp93;});
return d2;}
# 467
static struct Cyc_PP_Doc*Cyc_PP_ppseq_f(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
if(l == 0)return Cyc_PP_nil_doc();else{
if(l->tl == 0)return pp(l->hd);else{
return({struct Cyc_PP_Doc*_tmp94[4];({struct Cyc_PP_Doc*_tmpD9=pp(l->hd);_tmp94[0]=_tmpD9;}),({struct Cyc_PP_Doc*_tmpD8=Cyc_PP_text(sep);_tmp94[1]=_tmpD8;}),({struct Cyc_PP_Doc*_tmpD7=Cyc_PP_oline_doc();_tmp94[2]=_tmpD7;}),({struct Cyc_PP_Doc*_tmpD6=Cyc_PP_ppseq_f(pp,sep,l->tl);_tmp94[3]=_tmpD6;});Cyc_PP_cat(_tag_fat(_tmp94,sizeof(struct Cyc_PP_Doc*),4));});}}}
# 473
struct Cyc_PP_Doc*Cyc_PP_ppseq(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
return Cyc_PP_tab(Cyc_PP_ppseq_f(pp,sep,l));}
# 477
struct Cyc_PP_Doc*Cyc_PP_seq_f(struct _fat_ptr sep,struct Cyc_List_List*l){
if(l == 0)return Cyc_PP_nil_doc();else{
if(l->tl == 0)return(struct Cyc_PP_Doc*)l->hd;else{
# 481
struct Cyc_PP_Doc*sep2=Cyc_PP_text(sep);
struct Cyc_PP_Doc*oline=Cyc_PP_oline_doc();
struct Cyc_List_List*_tmp95=l;struct Cyc_List_List*x=_tmp95;
while(((struct Cyc_List_List*)_check_null(x))->tl != 0){
struct Cyc_List_List*_tmp96=x->tl;struct Cyc_List_List*temp=_tmp96;
({struct Cyc_List_List*_tmpDB=({struct Cyc_List_List*_tmp98=_cycalloc(sizeof(*_tmp98));_tmp98->hd=sep2,({struct Cyc_List_List*_tmpDA=({struct Cyc_List_List*_tmp97=_cycalloc(sizeof(*_tmp97));_tmp97->hd=oline,_tmp97->tl=temp;_tmp97;});_tmp98->tl=_tmpDA;});_tmp98;});x->tl=_tmpDB;});
x=temp;}
# 489
return Cyc_PP_cats(l);}}}
# 497
struct Cyc_PP_Doc*Cyc_PP_seq(struct _fat_ptr sep,struct Cyc_List_List*l){
return Cyc_PP_tab(Cyc_PP_seq_f(sep,l));}
# 502
struct Cyc_PP_Doc*Cyc_PP_ppseql_f(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
if(l == 0)return Cyc_PP_nil_doc();else{
if(l->tl == 0)return pp(l->hd);else{
return({struct Cyc_PP_Doc*_tmp99[4];({struct Cyc_PP_Doc*_tmpDF=pp(l->hd);_tmp99[0]=_tmpDF;}),({struct Cyc_PP_Doc*_tmpDE=Cyc_PP_text(sep);_tmp99[1]=_tmpDE;}),({struct Cyc_PP_Doc*_tmpDD=Cyc_PP_line_doc();_tmp99[2]=_tmpDD;}),({struct Cyc_PP_Doc*_tmpDC=Cyc_PP_ppseql_f(pp,sep,l->tl);_tmp99[3]=_tmpDC;});Cyc_PP_cat(_tag_fat(_tmp99,sizeof(struct Cyc_PP_Doc*),4));});}}}
# 507
struct Cyc_PP_Doc*Cyc_PP_ppseql(struct Cyc_PP_Doc*(*pp)(void*),struct _fat_ptr sep,struct Cyc_List_List*l){
return Cyc_PP_tab(Cyc_PP_ppseql_f(pp,sep,l));}
# 511
static struct Cyc_PP_Doc*Cyc_PP_seql_f(struct _fat_ptr sep,struct Cyc_List_List*l){
if(l == 0)return Cyc_PP_nil_doc();else{
if(l->tl == 0)return(struct Cyc_PP_Doc*)l->hd;else{
return({struct Cyc_PP_Doc*_tmp9A[4];_tmp9A[0]=(struct Cyc_PP_Doc*)l->hd,({struct Cyc_PP_Doc*_tmpE2=Cyc_PP_text(sep);_tmp9A[1]=_tmpE2;}),({struct Cyc_PP_Doc*_tmpE1=Cyc_PP_line_doc();_tmp9A[2]=_tmpE1;}),({struct Cyc_PP_Doc*_tmpE0=Cyc_PP_seql_f(sep,l->tl);_tmp9A[3]=_tmpE0;});Cyc_PP_cat(_tag_fat(_tmp9A,sizeof(struct Cyc_PP_Doc*),4));});}}}
# 516
struct Cyc_PP_Doc*Cyc_PP_seql(struct _fat_ptr sep,struct Cyc_List_List*l0){
return Cyc_PP_tab(Cyc_PP_seql_f(sep,l0));}
# 521
struct Cyc_PP_Doc*Cyc_PP_group(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*ss){
return({struct Cyc_PP_Doc*_tmp9B[3];({struct Cyc_PP_Doc*_tmpE5=Cyc_PP_text(start);_tmp9B[0]=_tmpE5;}),({
struct Cyc_PP_Doc*_tmpE4=Cyc_PP_seq(sep,ss);_tmp9B[1]=_tmpE4;}),({
struct Cyc_PP_Doc*_tmpE3=Cyc_PP_text(stop);_tmp9B[2]=_tmpE3;});Cyc_PP_cat(_tag_fat(_tmp9B,sizeof(struct Cyc_PP_Doc*),3));});}
# 528
struct Cyc_PP_Doc*Cyc_PP_egroup(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*ss){
if(ss == 0)return Cyc_PP_nil_doc();else{
return({struct Cyc_PP_Doc*_tmp9C[3];({struct Cyc_PP_Doc*_tmpE8=Cyc_PP_text(start);_tmp9C[0]=_tmpE8;}),({
struct Cyc_PP_Doc*_tmpE7=Cyc_PP_seq(sep,ss);_tmp9C[1]=_tmpE7;}),({
struct Cyc_PP_Doc*_tmpE6=Cyc_PP_text(stop);_tmp9C[2]=_tmpE6;});Cyc_PP_cat(_tag_fat(_tmp9C,sizeof(struct Cyc_PP_Doc*),3));});}}
# 535
struct Cyc_PP_Doc*Cyc_PP_groupl(struct _fat_ptr start,struct _fat_ptr stop,struct _fat_ptr sep,struct Cyc_List_List*ss){
return({struct Cyc_PP_Doc*_tmp9D[3];({struct Cyc_PP_Doc*_tmpEB=Cyc_PP_text(start);_tmp9D[0]=_tmpEB;}),({
struct Cyc_PP_Doc*_tmpEA=Cyc_PP_seql(sep,ss);_tmp9D[1]=_tmpEA;}),({
struct Cyc_PP_Doc*_tmpE9=Cyc_PP_text(stop);_tmp9D[2]=_tmpE9;});Cyc_PP_cat(_tag_fat(_tmp9D,sizeof(struct Cyc_PP_Doc*),3));});}

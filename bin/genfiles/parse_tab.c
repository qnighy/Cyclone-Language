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
  /* MWH: wish we didn't have to include the stuff below ... */
  struct _RegionPage *next;
  char data[1];
}
#endif
; // abstract -- defined in runtime_memory.c
struct _RegionHandle {
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
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
# 173 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
extern struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
extern int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_timeval{long tv_sec;long tv_usec;};extern char Cyc_Lexing_Error[6U];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _fat_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _fat_ptr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{struct _fat_ptr lex_base;struct _fat_ptr lex_backtrk;struct _fat_ptr lex_default;struct _fat_ptr lex_trans;struct _fat_ptr lex_check;};
# 78 "lexing.h"
extern struct Cyc_Lexing_lexbuf*Cyc_Lexing_from_file(struct Cyc___cycFILE*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
extern struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
extern int Cyc_List_length(struct Cyc_List_List*x);
# 76
extern struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
extern struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
extern void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 153
extern void*Cyc_List_fold_right(void*(*f)(void*,void*),struct Cyc_List_List*x,void*accum);
# 172
extern struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
extern struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
extern struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 195
extern struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 276
extern struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);
# 38 "string.h"
extern unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
extern int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
extern int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 52
extern int Cyc_zstrcmp(struct _fat_ptr,struct _fat_ptr);
# 54
extern int Cyc_zstrptrcmp(struct _fat_ptr*,struct _fat_ptr*);
# 60
extern struct _fat_ptr Cyc_strcat(struct _fat_ptr dest,struct _fat_ptr src);
# 71
extern struct _fat_ptr Cyc_strcpy(struct _fat_ptr dest,struct _fat_ptr src);
# 109 "string.h"
extern struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 95 "absyn.h"
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 158
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 179
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 184
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable =0U,Cyc_Absyn_Unique =1U,Cyc_Absyn_Top =2U};
# 189
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_RgnKind =3U,Cyc_Absyn_EffKind =4U,Cyc_Absyn_IntKind =5U,Cyc_Absyn_BoolKind =6U,Cyc_Absyn_PtrBndKind =7U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_Absyn_Vardecl*return_value;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 291
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};
# 304
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_UnknownDatatypefield(struct Cyc_Absyn_UnknownDatatypeFieldInfo);struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 311
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple0*,struct Cyc_Core_Opt*);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;};
# 414 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};
# 450
extern struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct Cyc_Absyn_Stdcall_att_val;
extern struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct Cyc_Absyn_Cdecl_att_val;
extern struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct Cyc_Absyn_Fastcall_att_val;
extern struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct Cyc_Absyn_Noreturn_att_val;
extern struct Cyc_Absyn_Const_att_Absyn_Attribute_struct Cyc_Absyn_Const_att_val;
extern struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct Cyc_Absyn_Packed_att_val;
# 457
extern struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct Cyc_Absyn_Shared_att_val;
extern struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct Cyc_Absyn_Unused_att_val;
extern struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct Cyc_Absyn_Weak_att_val;
extern struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct Cyc_Absyn_Dllimport_att_val;
extern struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct Cyc_Absyn_Dllexport_att_val;
extern struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct Cyc_Absyn_No_instrument_function_att_val;
extern struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct Cyc_Absyn_Constructor_att_val;
extern struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct Cyc_Absyn_Destructor_att_val;
extern struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct Cyc_Absyn_No_check_memory_usage_att_val;
extern struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct Cyc_Absyn_Pure_att_val;
extern struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct Cyc_Absyn_Always_inline_att_val;struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 503
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 510
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 528
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple8{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple8*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Assert_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple9{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple9 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple9 f2;struct _tuple9 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple9 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 696 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;
extern struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct Cyc_Absyn_Null_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;int is_proto;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;enum Cyc_Absyn_Scope orig_scope;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple10*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};
# 858
extern struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct Cyc_Absyn_Porton_d_val;
extern struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct Cyc_Absyn_Portoff_d_val;
extern struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct Cyc_Absyn_Tempeston_d_val;
extern struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct Cyc_Absyn_Tempestoff_d_val;struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 887
int Cyc_Absyn_is_qvar_qualified(struct _tuple0*);
# 891
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_combine_tqual(struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Tqual);
# 896
void*Cyc_Absyn_compress_kb(void*);
# 911
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 913
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);
void*Cyc_Absyn_int_type(enum Cyc_Absyn_Sign,enum Cyc_Absyn_Size_of);
# 916
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uint_type;
# 918
extern void*Cyc_Absyn_sint_type;
# 920
extern void*Cyc_Absyn_float_type;extern void*Cyc_Absyn_double_type;extern void*Cyc_Absyn_long_double_type;
# 923
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;extern void*Cyc_Absyn_refcnt_rgn_type;
# 927
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 929
extern void*Cyc_Absyn_void_type;extern void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);extern void*Cyc_Absyn_tag_type(void*);extern void*Cyc_Absyn_rgn_handle_type(void*);extern void*Cyc_Absyn_valueof_type(struct Cyc_Absyn_Exp*);extern void*Cyc_Absyn_typeof_type(struct Cyc_Absyn_Exp*);extern void*Cyc_Absyn_access_eff(void*);extern void*Cyc_Absyn_join_eff(struct Cyc_List_List*);extern void*Cyc_Absyn_regionsof_eff(void*);extern void*Cyc_Absyn_enum_type(struct _tuple0*n,struct Cyc_Absyn_Enumdecl*d);extern void*Cyc_Absyn_anon_enum_type(struct Cyc_List_List*);extern void*Cyc_Absyn_builtin_type(struct _fat_ptr s,struct Cyc_Absyn_Kind*k);extern void*Cyc_Absyn_typedef_type(struct _tuple0*,struct Cyc_List_List*,struct Cyc_Absyn_Typedefdecl*,void*);
# 954
extern void*Cyc_Absyn_fat_bound_type;
void*Cyc_Absyn_thin_bounds_type(void*);
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
# 958
void*Cyc_Absyn_bounds_one (void);
# 960
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo);
# 980
void*Cyc_Absyn_array_type(void*elt_type,struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc);
# 983
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args);
void*Cyc_Absyn_datatype_field_type(union Cyc_Absyn_DatatypeFieldInfo,struct Cyc_List_List*args);
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 988
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned);
# 991
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(unsigned);
# 994
struct Cyc_Absyn_Exp*Cyc_Absyn_true_exp(unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_false_exp(unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_int_exp(enum Cyc_Absyn_Sign,int,unsigned);
# 998
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_char_exp(char,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_wchar_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_float_exp(struct _fat_ptr,int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_string_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_wstring_exp(struct _fat_ptr,unsigned);
# 1006
struct Cyc_Absyn_Exp*Cyc_Absyn_unknownid_exp(struct _tuple0*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_pragma_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_prim1_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_prim2_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_swap_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1015
struct Cyc_Absyn_Exp*Cyc_Absyn_eq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_neq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_gt_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_lt_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_gte_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_lte_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned);
# 1023
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_unknowncall_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
# 1030
struct Cyc_Absyn_Exp*Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1032
struct Cyc_Absyn_Exp*Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*e,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrmember_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrarrow_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1047
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned);
# 1051
struct Cyc_Absyn_Exp*Cyc_Absyn_extension_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assert_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1060
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1065
struct Cyc_Absyn_Stmt*Cyc_Absyn_return_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_ifthenelse_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_while_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_break_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_continue_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_for_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_switch_stmt(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_fallthru_stmt(struct Cyc_List_List*,unsigned);
# 1076
struct Cyc_Absyn_Stmt*Cyc_Absyn_do_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_goto_stmt(struct _fat_ptr*,unsigned);
# 1079
struct Cyc_Absyn_Stmt*Cyc_Absyn_trycatch_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_List_List*,unsigned);
# 1082
struct Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*,unsigned);
struct Cyc_Absyn_Pat*Cyc_Absyn_exp_pat(struct Cyc_Absyn_Exp*);
# 1086
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
struct Cyc_Absyn_Decl*Cyc_Absyn_let_decl(struct Cyc_Absyn_Pat*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Decl*Cyc_Absyn_letv_decl(struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Decl*Cyc_Absyn_region_decl(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned);
# 1091
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init);
# 1093
struct Cyc_Absyn_AggrdeclImpl*Cyc_Absyn_aggrdecl_impl(struct Cyc_List_List*exists,struct Cyc_List_List*po,struct Cyc_List_List*fs,int tagged);
# 1100
struct Cyc_Absyn_TypeDecl*Cyc_Absyn_aggr_tdecl(enum Cyc_Absyn_AggrKind,enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*,struct Cyc_List_List*,unsigned);
# 1107
struct Cyc_Absyn_Decl*Cyc_Absyn_datatype_decl(enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Core_Opt*fs,int is_extensible,unsigned);
# 1110
struct Cyc_Absyn_TypeDecl*Cyc_Absyn_datatype_tdecl(enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Core_Opt*fs,int is_extensible,unsigned);
# 1115
void*Cyc_Absyn_function_type(struct Cyc_List_List*tvs,void*eff_typ,struct Cyc_Absyn_Tqual ret_tqual,void*ret_type,struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,struct Cyc_List_List*rgn_po,struct Cyc_List_List*,struct Cyc_Absyn_Exp*requires_clause,struct Cyc_Absyn_Exp*ensures_clause);
# 1163
extern int Cyc_Absyn_porting_c_code;
# 29 "warn.h"
void Cyc_Warn_warn(unsigned,struct _fat_ptr fmt,struct _fat_ptr);
# 33
void Cyc_Warn_verr(unsigned,struct _fat_ptr fmt,struct _fat_ptr);
# 35
void Cyc_Warn_err(unsigned,struct _fat_ptr fmt,struct _fat_ptr);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_TypOpt_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Stmt_Warn_Warg_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Warn_Aggrdecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Warn_Tvar_Warn_Warg_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Warn_KindBound_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Kind_Warn_Warg_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Warn_Attribute_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Vardecl_Warn_Warg_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Warn_Int_Warn_Warg_struct{int tag;int f1;};
# 67
void Cyc_Warn_err2(unsigned,struct _fat_ptr);
# 69
void Cyc_Warn_warn2(unsigned,struct _fat_ptr);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;
# 46 "tcutil.h"
int Cyc_Tcutil_is_array_type(void*);
# 93
void*Cyc_Tcutil_copy_type(void*);
# 106
void*Cyc_Tcutil_compress(void*);
# 134
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_mk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ek;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ik;
# 143
extern struct Cyc_Absyn_Kind Cyc_Tcutil_trk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tbk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 148
extern struct Cyc_Absyn_Kind Cyc_Tcutil_urk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_uak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ubk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_umk;
# 153
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;
# 155
extern struct Cyc_Core_Opt Cyc_Tcutil_bko;
# 157
extern struct Cyc_Core_Opt Cyc_Tcutil_iko;
# 162
extern struct Cyc_Core_Opt Cyc_Tcutil_trko;
# 172
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_opt(struct Cyc_Absyn_Kind*k);
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 256
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 284
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 294
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*);
# 41 "attributes.h"
int Cyc_Atts_fntype_att(void*);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;int in_extern_c_inc_repeat: 1;};
# 89 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 29 "currgn.h"
struct _fat_ptr Cyc_CurRgn_curr_rgn_name;
# 31
void*Cyc_CurRgn_curr_rgn_type (void);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 68 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_cnst2string(union Cyc_Absyn_Cnst);
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 71 "parse.y"
extern void Cyc_Lex_register_typedef(struct _tuple0*s);
extern void Cyc_Lex_enter_namespace(struct _fat_ptr*);
extern void Cyc_Lex_leave_namespace (void);
extern void Cyc_Lex_enter_using(struct _tuple0*);
extern void Cyc_Lex_leave_using (void);
extern void Cyc_Lex_enter_extern_c (void);
extern void Cyc_Lex_leave_extern_c (void);
extern struct _tuple0*Cyc_Lex_token_qvar;
extern struct _fat_ptr Cyc_Lex_token_string;
# 96 "parse.y"
int Cyc_Parse_parsing_tempest=0;struct Cyc_Parse_FlatList{struct Cyc_Parse_FlatList*tl;void*hd[0U] __attribute__((aligned )) ;};
# 102
struct Cyc_Parse_FlatList*Cyc_Parse_flat_imp_rev(struct Cyc_Parse_FlatList*x){
if(x == 0)return x;else{
# 105
struct Cyc_Parse_FlatList*first=x;
struct Cyc_Parse_FlatList*second=x->tl;
x->tl=0;
while(second != 0){
struct Cyc_Parse_FlatList*temp=second->tl;
second->tl=first;
first=second;
second=temp;}
# 114
return first;}}
# 119
int Cyc_Parse_no_register=0;char Cyc_Parse_Exit[5U]="Exit";struct Cyc_Parse_Exit_exn_struct{char*tag;};struct Cyc_Parse_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned loc;};
# 136
enum Cyc_Parse_Storage_class{Cyc_Parse_Typedef_sc =0U,Cyc_Parse_Extern_sc =1U,Cyc_Parse_ExternC_sc =2U,Cyc_Parse_Static_sc =3U,Cyc_Parse_Auto_sc =4U,Cyc_Parse_Register_sc =5U,Cyc_Parse_Abstract_sc =6U};struct Cyc_Parse_Declaration_spec{enum Cyc_Parse_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Parse_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};struct Cyc_Parse_Declarator{struct _tuple0*id;unsigned varloc;struct Cyc_List_List*tms;};struct _tuple11{struct _tuple11*tl;struct Cyc_Parse_Declarator hd  __attribute__((aligned )) ;};struct _tuple12{struct Cyc_Parse_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple13{struct _tuple13*tl;struct _tuple12 hd  __attribute__((aligned )) ;};struct Cyc_Parse_Numelts_ptrqual_Parse_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Parse_Region_ptrqual_Parse_Pointer_qual_struct{int tag;void*f1;};struct Cyc_Parse_Thin_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Fat_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Zeroterm_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Nozeroterm_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Notnull_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Nullable_ptrqual_Parse_Pointer_qual_struct{int tag;};
# 172
static void Cyc_Parse_decl_split(struct _RegionHandle*r,struct _tuple13*ds,struct _tuple11**decls,struct Cyc_List_List**es){
# 176
struct _tuple11*declarators=0;
struct Cyc_List_List*exprs=0;
for(0;ds != 0;ds=ds->tl){
struct _tuple12 _tmp0=ds->hd;struct _tuple12 _stmttmp0=_tmp0;struct _tuple12 _tmp1=_stmttmp0;struct Cyc_Absyn_Exp*_tmp3;struct Cyc_Parse_Declarator _tmp2;_LL1: _tmp2=_tmp1.f1;_tmp3=_tmp1.f2;_LL2: {struct Cyc_Parse_Declarator d=_tmp2;struct Cyc_Absyn_Exp*e=_tmp3;
declarators=({struct _tuple11*_tmp4=_region_malloc(r,sizeof(*_tmp4));_tmp4->tl=declarators,_tmp4->hd=d;_tmp4;});
exprs=({struct Cyc_List_List*_tmp5=_region_malloc(r,sizeof(*_tmp5));_tmp5->hd=e,_tmp5->tl=exprs;_tmp5;});}}
# 183
({struct Cyc_List_List*_tmp77A=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(exprs);*es=_tmp77A;});
({struct _tuple11*_tmp77B=((struct _tuple11*(*)(struct _tuple11*x))Cyc_Parse_flat_imp_rev)(declarators);*decls=_tmp77B;});}struct Cyc_Parse_Abstractdeclarator{struct Cyc_List_List*tms;};
# 193
static void*Cyc_Parse_collapse_type_specifiers(struct Cyc_Parse_Type_specifier ts,unsigned loc);struct _tuple14{struct Cyc_Absyn_Tqual f1;void*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;};
static struct _tuple14 Cyc_Parse_apply_tms(struct Cyc_Absyn_Tqual,void*,struct Cyc_List_List*,struct Cyc_List_List*);
# 199
static struct Cyc_List_List*Cyc_Parse_parse_result=0;
# 201
static void*Cyc_Parse_parse_abort(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap){
# 203
Cyc_Warn_verr(loc,fmt,ap);
(int)_throw((void*)({struct Cyc_Parse_Exit_exn_struct*_tmp6=_cycalloc(sizeof(*_tmp6));_tmp6->tag=Cyc_Parse_Exit;_tmp6;}));}
# 207
static void*Cyc_Parse_type_name_to_type(struct _tuple8*tqt,unsigned loc){
# 209
struct _tuple8*_tmp7=tqt;void*_tmp9;struct Cyc_Absyn_Tqual _tmp8;_LL1: _tmp8=_tmp7->f2;_tmp9=_tmp7->f3;_LL2: {struct Cyc_Absyn_Tqual tq=_tmp8;void*t=_tmp9;
if((tq.print_const || tq.q_volatile)|| tq.q_restrict){
if(tq.loc != (unsigned)0)loc=tq.loc;
({void*_tmpA=0U;({unsigned _tmp77D=loc;struct _fat_ptr _tmp77C=({const char*_tmpB="qualifier on type is ignored";_tag_fat(_tmpB,sizeof(char),29U);});Cyc_Warn_warn(_tmp77D,_tmp77C,_tag_fat(_tmpA,sizeof(void*),0U));});});}
# 214
return t;}}struct _tuple15{void*f1;void*f2;void*f3;void*f4;};
# 217
static struct _tuple15 Cyc_Parse_collapse_pointer_quals(unsigned loc,void*nullable,void*bound,void*rgn,struct Cyc_List_List*pqs){
# 223
void*zeroterm=Cyc_Tcutil_any_bool(0);
for(0;pqs != 0;pqs=pqs->tl){
void*_tmpC=(void*)pqs->hd;void*_stmttmp1=_tmpC;void*_tmpD=_stmttmp1;void*_tmpE;struct Cyc_Absyn_Exp*_tmpF;switch(*((int*)_tmpD)){case 4U: _LL1: _LL2:
 zeroterm=Cyc_Absyn_true_type;goto _LL0;case 5U: _LL3: _LL4:
 zeroterm=Cyc_Absyn_false_type;goto _LL0;case 7U: _LL5: _LL6:
 nullable=Cyc_Absyn_true_type;goto _LL0;case 6U: _LL7: _LL8:
 nullable=Cyc_Absyn_false_type;goto _LL0;case 3U: _LL9: _LLA:
 bound=Cyc_Absyn_fat_bound_type;goto _LL0;case 2U: _LLB: _LLC:
 bound=Cyc_Absyn_bounds_one();goto _LL0;case 0U: _LLD: _tmpF=((struct Cyc_Parse_Numelts_ptrqual_Parse_Pointer_qual_struct*)_tmpD)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmpF;
bound=Cyc_Absyn_thin_bounds_exp(e);goto _LL0;}default: _LLF: _tmpE=(void*)((struct Cyc_Parse_Region_ptrqual_Parse_Pointer_qual_struct*)_tmpD)->f1;_LL10: {void*t=_tmpE;
rgn=t;goto _LL0;}}_LL0:;}
# 235
return({struct _tuple15 _tmp6D4;_tmp6D4.f1=nullable,_tmp6D4.f2=bound,_tmp6D4.f3=zeroterm,_tmp6D4.f4=rgn;_tmp6D4;});}
# 241
struct _tuple0*Cyc_Parse_gensym_enum (void){
# 243
static int enum_counter=0;
return({struct _tuple0*_tmp14=_cycalloc(sizeof(*_tmp14));({union Cyc_Absyn_Nmspace _tmp781=Cyc_Absyn_Rel_n(0);_tmp14->f1=_tmp781;}),({
struct _fat_ptr*_tmp780=({struct _fat_ptr*_tmp13=_cycalloc(sizeof(*_tmp13));({struct _fat_ptr _tmp77F=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp12=({struct Cyc_Int_pa_PrintArg_struct _tmp6D5;_tmp6D5.tag=1U,_tmp6D5.f1=(unsigned long)enum_counter ++;_tmp6D5;});void*_tmp10[1U];_tmp10[0]=& _tmp12;({struct _fat_ptr _tmp77E=({const char*_tmp11="__anonymous_enum_%d__";_tag_fat(_tmp11,sizeof(char),22U);});Cyc_aprintf(_tmp77E,_tag_fat(_tmp10,sizeof(void*),1U));});});*_tmp13=_tmp77F;});_tmp13;});_tmp14->f2=_tmp780;});_tmp14;});}struct _tuple16{unsigned f1;struct _tuple0*f2;struct Cyc_Absyn_Tqual f3;void*f4;struct Cyc_List_List*f5;struct Cyc_List_List*f6;};struct _tuple17{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple18{struct _tuple16*f1;struct _tuple17*f2;};
# 248
static struct Cyc_Absyn_Aggrfield*Cyc_Parse_make_aggr_field(unsigned loc,struct _tuple18*field_info){
# 253
struct _tuple18*_tmp15=field_info;struct Cyc_Absyn_Exp*_tmp1D;struct Cyc_Absyn_Exp*_tmp1C;struct Cyc_List_List*_tmp1B;struct Cyc_List_List*_tmp1A;void*_tmp19;struct Cyc_Absyn_Tqual _tmp18;struct _tuple0*_tmp17;unsigned _tmp16;_LL1: _tmp16=(_tmp15->f1)->f1;_tmp17=(_tmp15->f1)->f2;_tmp18=(_tmp15->f1)->f3;_tmp19=(_tmp15->f1)->f4;_tmp1A=(_tmp15->f1)->f5;_tmp1B=(_tmp15->f1)->f6;_tmp1C=(_tmp15->f2)->f1;_tmp1D=(_tmp15->f2)->f2;_LL2: {unsigned varloc=_tmp16;struct _tuple0*qid=_tmp17;struct Cyc_Absyn_Tqual tq=_tmp18;void*t=_tmp19;struct Cyc_List_List*tvs=_tmp1A;struct Cyc_List_List*atts=_tmp1B;struct Cyc_Absyn_Exp*widthopt=_tmp1C;struct Cyc_Absyn_Exp*reqopt=_tmp1D;
if(tvs != 0)
({void*_tmp1E=0U;({unsigned _tmp783=loc;struct _fat_ptr _tmp782=({const char*_tmp1F="bad type params in struct field";_tag_fat(_tmp1F,sizeof(char),32U);});Cyc_Warn_err(_tmp783,_tmp782,_tag_fat(_tmp1E,sizeof(void*),0U));});});
if(Cyc_Absyn_is_qvar_qualified(qid))
({void*_tmp20=0U;({unsigned _tmp785=loc;struct _fat_ptr _tmp784=({const char*_tmp21="struct or union field cannot be qualified with a namespace";_tag_fat(_tmp21,sizeof(char),59U);});Cyc_Warn_err(_tmp785,_tmp784,_tag_fat(_tmp20,sizeof(void*),0U));});});
return({struct Cyc_Absyn_Aggrfield*_tmp22=_cycalloc(sizeof(*_tmp22));_tmp22->name=(*qid).f2,_tmp22->tq=tq,_tmp22->type=t,_tmp22->width=widthopt,_tmp22->attributes=atts,_tmp22->requires_clause=reqopt;_tmp22;});}}
# 263
static struct Cyc_Parse_Type_specifier Cyc_Parse_empty_spec(unsigned loc){
return({struct Cyc_Parse_Type_specifier _tmp6D6;_tmp6D6.Signed_spec=0,_tmp6D6.Unsigned_spec=0,_tmp6D6.Short_spec=0,_tmp6D6.Long_spec=0,_tmp6D6.Long_Long_spec=0,_tmp6D6.Valid_type_spec=0,_tmp6D6.Type_spec=Cyc_Absyn_sint_type,_tmp6D6.loc=loc;_tmp6D6;});}
# 274
static struct Cyc_Parse_Type_specifier Cyc_Parse_type_spec(void*t,unsigned loc){
struct Cyc_Parse_Type_specifier _tmp23=Cyc_Parse_empty_spec(loc);struct Cyc_Parse_Type_specifier s=_tmp23;
s.Type_spec=t;
s.Valid_type_spec=1;
return s;}
# 280
static struct Cyc_Parse_Type_specifier Cyc_Parse_signed_spec(unsigned loc){
struct Cyc_Parse_Type_specifier _tmp24=Cyc_Parse_empty_spec(loc);struct Cyc_Parse_Type_specifier s=_tmp24;
s.Signed_spec=1;
return s;}
# 285
static struct Cyc_Parse_Type_specifier Cyc_Parse_unsigned_spec(unsigned loc){
struct Cyc_Parse_Type_specifier _tmp25=Cyc_Parse_empty_spec(loc);struct Cyc_Parse_Type_specifier s=_tmp25;
s.Unsigned_spec=1;
return s;}
# 290
static struct Cyc_Parse_Type_specifier Cyc_Parse_short_spec(unsigned loc){
struct Cyc_Parse_Type_specifier _tmp26=Cyc_Parse_empty_spec(loc);struct Cyc_Parse_Type_specifier s=_tmp26;
s.Short_spec=1;
return s;}
# 295
static struct Cyc_Parse_Type_specifier Cyc_Parse_long_spec(unsigned loc){
struct Cyc_Parse_Type_specifier _tmp27=Cyc_Parse_empty_spec(loc);struct Cyc_Parse_Type_specifier s=_tmp27;
s.Long_spec=1;
return s;}
# 302
static void*Cyc_Parse_array2ptr(void*t,int argposn){
# 304
return Cyc_Tcutil_is_array_type(t)?({
void*_tmp786=t;Cyc_Tcutil_promote_array(_tmp786,argposn?Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,0): Cyc_Absyn_heap_rgn_type,0);}): t;}struct _tuple19{struct _fat_ptr*f1;void*f2;};
# 317 "parse.y"
static struct Cyc_List_List*Cyc_Parse_get_arg_tags(struct Cyc_List_List*x){
struct Cyc_List_List*_tmp28=0;struct Cyc_List_List*res=_tmp28;
for(0;x != 0;x=x->tl){
struct _tuple8*_tmp29=(struct _tuple8*)x->hd;struct _tuple8*_stmttmp2=_tmp29;struct _tuple8*_tmp2A=_stmttmp2;void**_tmp2C;struct _fat_ptr _tmp2B;void*_tmp2E;struct _fat_ptr*_tmp2D;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->f1)->tag == 4U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->f2)->tl == 0){_LL1: _tmp2D=_tmp2A->f1;_tmp2E=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A->f3)->f2)->hd;if(_tmp2D != 0){_LL2: {struct _fat_ptr*v=_tmp2D;void*i=_tmp2E;
# 322
{void*_tmp2F=i;void**_tmp30;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2F)->tag == 1U){_LL8: _tmp30=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2F)->f2;_LL9: {void**z=_tmp30;
# 326
struct _fat_ptr*nm=({struct _fat_ptr*_tmp36=_cycalloc(sizeof(*_tmp36));({struct _fat_ptr _tmp788=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp35=({struct Cyc_String_pa_PrintArg_struct _tmp6D7;_tmp6D7.tag=0U,_tmp6D7.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmp6D7;});void*_tmp33[1U];_tmp33[0]=& _tmp35;({struct _fat_ptr _tmp787=({const char*_tmp34="`%s";_tag_fat(_tmp34,sizeof(char),4U);});Cyc_aprintf(_tmp787,_tag_fat(_tmp33,sizeof(void*),1U));});});*_tmp36=_tmp788;});_tmp36;});
({void*_tmp78A=Cyc_Absyn_var_type(({struct Cyc_Absyn_Tvar*_tmp32=_cycalloc(sizeof(*_tmp32));_tmp32->name=nm,_tmp32->identity=- 1,({void*_tmp789=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp31=_cycalloc(sizeof(*_tmp31));_tmp31->tag=0U,_tmp31->f1=& Cyc_Tcutil_ik;_tmp31;});_tmp32->kind=_tmp789;});_tmp32;}));*z=_tmp78A;});
goto _LL7;}}else{_LLA: _LLB:
 goto _LL7;}_LL7:;}
# 331
res=({struct Cyc_List_List*_tmp38=_cycalloc(sizeof(*_tmp38));({struct _tuple19*_tmp78B=({struct _tuple19*_tmp37=_cycalloc(sizeof(*_tmp37));_tmp37->f1=v,_tmp37->f2=i;_tmp37;});_tmp38->hd=_tmp78B;}),_tmp38->tl=res;_tmp38;});goto _LL0;}}else{if(((struct _tuple8*)_tmp2A)->f1 != 0)goto _LL5;else{goto _LL5;}}}else{if(((struct _tuple8*)_tmp2A)->f1 != 0)goto _LL5;else{goto _LL5;}}}else{if(((struct _tuple8*)_tmp2A)->f1 != 0)goto _LL5;else{goto _LL5;}}}else{if(((struct _tuple8*)_tmp2A)->f1 != 0){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->f2 != 0){if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->f2)->hd)->tag == 1U){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple8*)_tmp2A)->f3)->f2)->tl == 0){_LL3: _tmp2B=*_tmp2A->f1;_tmp2C=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2A->f3)->f2)->hd)->f2;_LL4: {struct _fat_ptr v=_tmp2B;void**z=_tmp2C;
# 335
struct _fat_ptr*nm=({struct _fat_ptr*_tmp3E=_cycalloc(sizeof(*_tmp3E));({struct _fat_ptr _tmp78D=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3D=({struct Cyc_String_pa_PrintArg_struct _tmp6D8;_tmp6D8.tag=0U,_tmp6D8.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmp6D8;});void*_tmp3B[1U];_tmp3B[0]=& _tmp3D;({struct _fat_ptr _tmp78C=({const char*_tmp3C="`%s";_tag_fat(_tmp3C,sizeof(char),4U);});Cyc_aprintf(_tmp78C,_tag_fat(_tmp3B,sizeof(void*),1U));});});*_tmp3E=_tmp78D;});_tmp3E;});
({void*_tmp78F=Cyc_Absyn_var_type(({struct Cyc_Absyn_Tvar*_tmp3A=_cycalloc(sizeof(*_tmp3A));_tmp3A->name=nm,_tmp3A->identity=- 1,({void*_tmp78E=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp39=_cycalloc(sizeof(*_tmp39));_tmp39->tag=0U,_tmp39->f1=& Cyc_Tcutil_rk;_tmp39;});_tmp3A->kind=_tmp78E;});_tmp3A;}));*z=_tmp78F;});
goto _LL0;}}else{goto _LL5;}}else{goto _LL5;}}else{goto _LL5;}}else{goto _LL5;}}else{goto _LL5;}}}else{if(((struct _tuple8*)_tmp2A)->f1 != 0)goto _LL5;else{_LL5: _LL6:
 goto _LL0;}}_LL0:;}
# 341
return res;}
# 345
static struct Cyc_List_List*Cyc_Parse_get_aggrfield_tags(struct Cyc_List_List*x){
struct Cyc_List_List*_tmp3F=0;struct Cyc_List_List*res=_tmp3F;
for(0;x != 0;x=x->tl){
void*_tmp40=((struct Cyc_Absyn_Aggrfield*)x->hd)->type;void*_stmttmp3=_tmp40;void*_tmp41=_stmttmp3;void*_tmp42;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41)->f1)->tag == 4U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41)->f2)->tl == 0){_LL1: _tmp42=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp41)->f2)->hd;_LL2: {void*i=_tmp42;
# 350
res=({struct Cyc_List_List*_tmp44=_cycalloc(sizeof(*_tmp44));({struct _tuple19*_tmp790=({struct _tuple19*_tmp43=_cycalloc(sizeof(*_tmp43));_tmp43->f1=((struct Cyc_Absyn_Aggrfield*)x->hd)->name,_tmp43->f2=i;_tmp43;});_tmp44->hd=_tmp790;}),_tmp44->tl=res;_tmp44;});goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 354
return res;}
# 358
static struct Cyc_Absyn_Exp*Cyc_Parse_substitute_tags_exp(struct Cyc_List_List*tags,struct Cyc_Absyn_Exp*e){
{void*_tmp45=e->r;void*_stmttmp4=_tmp45;void*_tmp46=_stmttmp4;struct _fat_ptr*_tmp47;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp46)->tag == 1U){if(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp46)->f1)->tag == 0U){if(((((struct _tuple0*)((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp46)->f1)->f1)->f1).Rel_n).tag == 1){if(((((struct _tuple0*)((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp46)->f1)->f1)->f1).Rel_n).val == 0){_LL1: _tmp47=(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp46)->f1)->f1)->f2;_LL2: {struct _fat_ptr*y=_tmp47;
# 361
{struct Cyc_List_List*_tmp48=tags;struct Cyc_List_List*ts=_tmp48;for(0;ts != 0;ts=ts->tl){
struct _tuple19*_tmp49=(struct _tuple19*)ts->hd;struct _tuple19*_stmttmp5=_tmp49;struct _tuple19*_tmp4A=_stmttmp5;void*_tmp4C;struct _fat_ptr*_tmp4B;_LL6: _tmp4B=_tmp4A->f1;_tmp4C=_tmp4A->f2;_LL7: {struct _fat_ptr*x=_tmp4B;void*i=_tmp4C;
if(Cyc_strptrcmp(x,y)== 0)
return({void*_tmp792=(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmp4D=_cycalloc(sizeof(*_tmp4D));_tmp4D->tag=39U,({void*_tmp791=Cyc_Tcutil_copy_type(i);_tmp4D->f1=_tmp791;});_tmp4D;});Cyc_Absyn_new_exp(_tmp792,e->loc);});}}}
# 366
goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 369
return e;}
# 374
static void*Cyc_Parse_substitute_tags(struct Cyc_List_List*tags,void*t){
{void*_tmp4E=t;struct Cyc_Absyn_Exp*_tmp4F;void*_tmp50;struct Cyc_Absyn_PtrLoc*_tmp57;void*_tmp56;void*_tmp55;void*_tmp54;void*_tmp53;struct Cyc_Absyn_Tqual _tmp52;void*_tmp51;unsigned _tmp5C;void*_tmp5B;struct Cyc_Absyn_Exp*_tmp5A;struct Cyc_Absyn_Tqual _tmp59;void*_tmp58;switch(*((int*)_tmp4E)){case 4U: _LL1: _tmp58=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4E)->f1).elt_type;_tmp59=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4E)->f1).tq;_tmp5A=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4E)->f1).num_elts;_tmp5B=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4E)->f1).zero_term;_tmp5C=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp4E)->f1).zt_loc;_LL2: {void*et=_tmp58;struct Cyc_Absyn_Tqual tq=_tmp59;struct Cyc_Absyn_Exp*nelts=_tmp5A;void*zt=_tmp5B;unsigned ztloc=_tmp5C;
# 377
struct Cyc_Absyn_Exp*nelts2=nelts;
if(nelts != 0)
nelts2=Cyc_Parse_substitute_tags_exp(tags,nelts);{
# 381
void*_tmp5D=Cyc_Parse_substitute_tags(tags,et);void*et2=_tmp5D;
if(nelts != nelts2 || et != et2)
return Cyc_Absyn_array_type(et2,tq,nelts2,zt,ztloc);
goto _LL0;}}case 3U: _LL3: _tmp51=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4E)->f1).elt_type;_tmp52=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4E)->f1).elt_tq;_tmp53=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4E)->f1).ptr_atts).rgn;_tmp54=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4E)->f1).ptr_atts).nullable;_tmp55=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4E)->f1).ptr_atts).bounds;_tmp56=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4E)->f1).ptr_atts).zero_term;_tmp57=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp4E)->f1).ptr_atts).ptrloc;_LL4: {void*et=_tmp51;struct Cyc_Absyn_Tqual tq=_tmp52;void*r=_tmp53;void*n=_tmp54;void*b=_tmp55;void*zt=_tmp56;struct Cyc_Absyn_PtrLoc*pl=_tmp57;
# 386
void*_tmp5E=Cyc_Parse_substitute_tags(tags,et);void*et2=_tmp5E;
void*_tmp5F=Cyc_Parse_substitute_tags(tags,b);void*b2=_tmp5F;
if(et2 != et || b2 != b)
return Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp6D9;_tmp6D9.elt_type=et2,_tmp6D9.elt_tq=tq,(_tmp6D9.ptr_atts).rgn=r,(_tmp6D9.ptr_atts).nullable=n,(_tmp6D9.ptr_atts).bounds=b2,(_tmp6D9.ptr_atts).zero_term=zt,(_tmp6D9.ptr_atts).ptrloc=pl;_tmp6D9;}));
goto _LL0;}case 0U: if(((struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E)->f1)->tag == 13U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E)->f2)->tl == 0){_LL5: _tmp50=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp4E)->f2)->hd;_LL6: {void*t=_tmp50;
# 392
void*_tmp60=Cyc_Parse_substitute_tags(tags,t);void*t2=_tmp60;
if(t != t2)return Cyc_Absyn_thin_bounds_type(t2);
goto _LL0;}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}case 9U: _LL7: _tmp4F=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp4E)->f1;_LL8: {struct Cyc_Absyn_Exp*e=_tmp4F;
# 396
struct Cyc_Absyn_Exp*_tmp61=Cyc_Parse_substitute_tags_exp(tags,e);struct Cyc_Absyn_Exp*e2=_tmp61;
if(e2 != e)return Cyc_Absyn_valueof_type(e2);
goto _LL0;}default: _LL9: _LLA:
# 402
 goto _LL0;}_LL0:;}
# 404
return t;}
# 409
static void Cyc_Parse_substitute_aggrfield_tags(struct Cyc_List_List*tags,struct Cyc_Absyn_Aggrfield*x){
({void*_tmp793=Cyc_Parse_substitute_tags(tags,x->type);x->type=_tmp793;});}struct _tuple20{struct Cyc_Absyn_Tqual f1;void*f2;};
# 416
static struct _tuple20*Cyc_Parse_get_tqual_typ(unsigned loc,struct _tuple8*t){
# 418
return({struct _tuple20*_tmp62=_cycalloc(sizeof(*_tmp62));_tmp62->f1=(*t).f2,_tmp62->f2=(*t).f3;_tmp62;});}
# 421
static int Cyc_Parse_is_typeparam(void*tm){
void*_tmp63=tm;if(((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp63)->tag == 4U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 430
static void*Cyc_Parse_id2type(struct _fat_ptr s,void*k){
if(({struct _fat_ptr _tmp794=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp794,({const char*_tmp64="`H";_tag_fat(_tmp64,sizeof(char),3U);}));})== 0)
return Cyc_Absyn_heap_rgn_type;else{
if(({struct _fat_ptr _tmp795=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp795,({const char*_tmp65="`U";_tag_fat(_tmp65,sizeof(char),3U);}));})== 0)
return Cyc_Absyn_unique_rgn_type;else{
if(({struct _fat_ptr _tmp796=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp796,({const char*_tmp66="`RC";_tag_fat(_tmp66,sizeof(char),4U);}));})== 0)
return Cyc_Absyn_refcnt_rgn_type;else{
if(Cyc_zstrcmp((struct _fat_ptr)s,(struct _fat_ptr)Cyc_CurRgn_curr_rgn_name)== 0)
return Cyc_CurRgn_curr_rgn_type();else{
# 440
return Cyc_Absyn_var_type(({struct Cyc_Absyn_Tvar*_tmp68=_cycalloc(sizeof(*_tmp68));({struct _fat_ptr*_tmp797=({struct _fat_ptr*_tmp67=_cycalloc(sizeof(*_tmp67));*_tmp67=s;_tmp67;});_tmp68->name=_tmp797;}),_tmp68->identity=- 1,_tmp68->kind=k;_tmp68;}));}}}}}
# 443
static int Cyc_Parse_tvar_ok(struct _fat_ptr s,struct _fat_ptr*err){
if(({struct _fat_ptr _tmp798=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp798,({const char*_tmp69="`H";_tag_fat(_tmp69,sizeof(char),3U);}));})== 0){
({struct _fat_ptr _tmp799=({const char*_tmp6A="bad occurrence of heap region";_tag_fat(_tmp6A,sizeof(char),30U);});*err=_tmp799;});
return 0;}
# 448
if(({struct _fat_ptr _tmp79A=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp79A,({const char*_tmp6B="`U";_tag_fat(_tmp6B,sizeof(char),3U);}));})== 0){
({struct _fat_ptr _tmp79B=({const char*_tmp6C="bad occurrence of unique region";_tag_fat(_tmp6C,sizeof(char),32U);});*err=_tmp79B;});
return 0;}
# 452
if(({struct _fat_ptr _tmp79C=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp79C,({const char*_tmp6D="`RC";_tag_fat(_tmp6D,sizeof(char),4U);}));})== 0){
({struct _fat_ptr _tmp79D=({const char*_tmp6E="bad occurrence of refcounted region";_tag_fat(_tmp6E,sizeof(char),36U);});*err=_tmp79D;});
return 0;}
# 456
if(Cyc_zstrcmp((struct _fat_ptr)s,(struct _fat_ptr)Cyc_CurRgn_curr_rgn_name)== 0){
({struct _fat_ptr _tmp79E=({const char*_tmp6F="bad occurrence of \"current\" region";_tag_fat(_tmp6F,sizeof(char),35U);});*err=_tmp79E;});
return 0;}
# 460
return 1;}
# 467
static struct Cyc_Absyn_Tvar*Cyc_Parse_typ2tvar(unsigned loc,void*t){
void*_tmp70=t;struct Cyc_Absyn_Tvar*_tmp71;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp70)->tag == 2U){_LL1: _tmp71=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp70)->f1;_LL2: {struct Cyc_Absyn_Tvar*pr=_tmp71;
return pr;}}else{_LL3: _LL4:
({void*_tmp72=0U;({unsigned _tmp7A0=loc;struct _fat_ptr _tmp79F=({const char*_tmp73="expecting a list of type variables, not types";_tag_fat(_tmp73,sizeof(char),46U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7A0,_tmp79F,_tag_fat(_tmp72,sizeof(void*),0U));});});}_LL0:;}
# 475
static void Cyc_Parse_set_vartyp_kind(void*t,struct Cyc_Absyn_Kind*k,int leq){
void*_tmp74=Cyc_Tcutil_compress(t);void*_stmttmp6=_tmp74;void*_tmp75=_stmttmp6;void**_tmp76;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp75)->tag == 2U){_LL1: _tmp76=(void**)&(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp75)->f1)->kind;_LL2: {void**cptr=_tmp76;
# 478
void*_tmp77=Cyc_Absyn_compress_kb(*cptr);void*_stmttmp7=_tmp77;void*_tmp78=_stmttmp7;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp78)->tag == 1U){_LL6: _LL7:
# 480
 if(!leq)({void*_tmp7A1=Cyc_Tcutil_kind_to_bound(k);*cptr=_tmp7A1;});else{
({void*_tmp7A2=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp79=_cycalloc(sizeof(*_tmp79));_tmp79->tag=2U,_tmp79->f1=0,_tmp79->f2=k;_tmp79;});*cptr=_tmp7A2;});}
return;}else{_LL8: _LL9:
 return;}_LL5:;}}else{_LL3: _LL4:
# 485
 return;}_LL0:;}
# 490
static struct Cyc_List_List*Cyc_Parse_oldstyle2newstyle(struct _RegionHandle*yy,struct Cyc_List_List*tms,struct Cyc_List_List*tds,unsigned loc){
# 496
if(tds == 0)return tms;
# 501
if(tms == 0)return 0;{
# 503
void*_tmp7A=(void*)tms->hd;void*_stmttmp8=_tmp7A;void*_tmp7B=_stmttmp8;void*_tmp7C;if(((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp7B)->tag == 3U){_LL1: _tmp7C=(void*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp7B)->f1;_LL2: {void*args=_tmp7C;
# 506
if(tms->tl == 0 ||
 Cyc_Parse_is_typeparam((void*)((struct Cyc_List_List*)_check_null(tms->tl))->hd)&&((struct Cyc_List_List*)_check_null(tms->tl))->tl == 0){
# 509
void*_tmp7D=args;struct Cyc_List_List*_tmp7E;if(((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp7D)->tag == 1U){_LL6: _LL7:
# 511
({void*_tmp7F=0U;({unsigned _tmp7A4=loc;struct _fat_ptr _tmp7A3=({const char*_tmp80="function declaration with both new- and old-style parameter declarations; ignoring old-style";_tag_fat(_tmp80,sizeof(char),93U);});Cyc_Warn_warn(_tmp7A4,_tmp7A3,_tag_fat(_tmp7F,sizeof(void*),0U));});});
# 513
return tms;}else{_LL8: _tmp7E=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)_tmp7D)->f1;_LL9: {struct Cyc_List_List*ids=_tmp7E;
# 515
if(({int _tmp7A5=((int(*)(struct Cyc_List_List*x))Cyc_List_length)(ids);_tmp7A5 != ((int(*)(struct Cyc_List_List*x))Cyc_List_length)(tds);}))
({void*_tmp81=0U;({unsigned _tmp7A7=loc;struct _fat_ptr _tmp7A6=({const char*_tmp82="wrong number of parameter declarations in old-style function declaration";_tag_fat(_tmp82,sizeof(char),73U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7A7,_tmp7A6,_tag_fat(_tmp81,sizeof(void*),0U));});});{
# 519
struct Cyc_List_List*rev_new_params=0;
for(0;ids != 0;ids=ids->tl){
struct Cyc_List_List*_tmp83=tds;struct Cyc_List_List*tds2=_tmp83;
for(0;tds2 != 0;tds2=tds2->tl){
struct Cyc_Absyn_Decl*_tmp84=(struct Cyc_Absyn_Decl*)tds2->hd;struct Cyc_Absyn_Decl*x=_tmp84;
void*_tmp85=x->r;void*_stmttmp9=_tmp85;void*_tmp86=_stmttmp9;struct Cyc_Absyn_Vardecl*_tmp87;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp86)->tag == 0U){_LLB: _tmp87=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp86)->f1;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp87;
# 526
if(Cyc_zstrptrcmp((*vd->name).f2,(struct _fat_ptr*)ids->hd)!= 0)
continue;
if(vd->initializer != 0)
({void*_tmp88=0U;({unsigned _tmp7A9=x->loc;struct _fat_ptr _tmp7A8=({const char*_tmp89="initializer found in parameter declaration";_tag_fat(_tmp89,sizeof(char),43U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7A9,_tmp7A8,_tag_fat(_tmp88,sizeof(void*),0U));});});
if(Cyc_Absyn_is_qvar_qualified(vd->name))
({void*_tmp8A=0U;({unsigned _tmp7AB=x->loc;struct _fat_ptr _tmp7AA=({const char*_tmp8B="namespaces forbidden in parameter declarations";_tag_fat(_tmp8B,sizeof(char),47U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7AB,_tmp7AA,_tag_fat(_tmp8A,sizeof(void*),0U));});});
rev_new_params=({struct Cyc_List_List*_tmp8D=_cycalloc(sizeof(*_tmp8D));
({struct _tuple8*_tmp7AC=({struct _tuple8*_tmp8C=_cycalloc(sizeof(*_tmp8C));_tmp8C->f1=(*vd->name).f2,_tmp8C->f2=vd->tq,_tmp8C->f3=vd->type;_tmp8C;});_tmp8D->hd=_tmp7AC;}),_tmp8D->tl=rev_new_params;_tmp8D;});
# 535
goto L;}}else{_LLD: _LLE:
({void*_tmp8E=0U;({unsigned _tmp7AE=x->loc;struct _fat_ptr _tmp7AD=({const char*_tmp8F="nonvariable declaration in parameter type";_tag_fat(_tmp8F,sizeof(char),42U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7AE,_tmp7AD,_tag_fat(_tmp8E,sizeof(void*),0U));});});}_LLA:;}
# 539
L: if(tds2 == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp92=({struct Cyc_String_pa_PrintArg_struct _tmp6DA;_tmp6DA.tag=0U,_tmp6DA.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)ids->hd));_tmp6DA;});void*_tmp90[1U];_tmp90[0]=& _tmp92;({unsigned _tmp7B0=loc;struct _fat_ptr _tmp7AF=({const char*_tmp91="%s is not given a type";_tag_fat(_tmp91,sizeof(char),23U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7B0,_tmp7AF,_tag_fat(_tmp90,sizeof(void*),1U));});});}
# 542
return({struct Cyc_List_List*_tmp95=_region_malloc(yy,sizeof(*_tmp95));
({void*_tmp7B3=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp94=_region_malloc(yy,sizeof(*_tmp94));_tmp94->tag=3U,({void*_tmp7B2=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp93=_region_malloc(yy,sizeof(*_tmp93));_tmp93->tag=1U,({struct Cyc_List_List*_tmp7B1=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(rev_new_params);_tmp93->f1=_tmp7B1;}),_tmp93->f2=0,_tmp93->f3=0,_tmp93->f4=0,_tmp93->f5=0,_tmp93->f6=0,_tmp93->f7=0;_tmp93;});_tmp94->f1=_tmp7B2;});_tmp94;});_tmp95->hd=_tmp7B3;}),_tmp95->tl=0;_tmp95;});}}}_LL5:;}
# 549
goto _LL4;}}else{_LL3: _LL4:
 return({struct Cyc_List_List*_tmp96=_region_malloc(yy,sizeof(*_tmp96));_tmp96->hd=(void*)tms->hd,({struct Cyc_List_List*_tmp7B4=Cyc_Parse_oldstyle2newstyle(yy,tms->tl,tds,loc);_tmp96->tl=_tmp7B4;});_tmp96;});}_LL0:;}}
# 557
static struct Cyc_Absyn_Fndecl*Cyc_Parse_make_function(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc){
# 561
if(tds != 0)
d=({struct Cyc_Parse_Declarator _tmp6DB;_tmp6DB.id=d.id,_tmp6DB.varloc=d.varloc,({struct Cyc_List_List*_tmp7B5=Cyc_Parse_oldstyle2newstyle(yy,d.tms,tds,loc);_tmp6DB.tms=_tmp7B5;});_tmp6DB;});{
# 564
enum Cyc_Absyn_Scope sc=2U;
struct Cyc_Parse_Type_specifier tss=Cyc_Parse_empty_spec(loc);
struct Cyc_Absyn_Tqual tq=Cyc_Absyn_empty_tqual(0U);
int is_inline=0;
struct Cyc_List_List*atts=0;
# 570
if(dso != 0){
tss=dso->type_specs;
tq=dso->tq;
is_inline=dso->is_inline;
atts=dso->attributes;
# 576
if(dso->sc != 0){
enum Cyc_Parse_Storage_class _tmp97=*((enum Cyc_Parse_Storage_class*)_check_null(dso->sc));enum Cyc_Parse_Storage_class _stmttmpA=_tmp97;enum Cyc_Parse_Storage_class _tmp98=_stmttmpA;switch(_tmp98){case Cyc_Parse_Extern_sc: _LL1: _LL2:
 sc=3U;goto _LL0;case Cyc_Parse_Static_sc: _LL3: _LL4:
 sc=0U;goto _LL0;default: _LL5: _LL6:
({void*_tmp99=0U;({unsigned _tmp7B7=loc;struct _fat_ptr _tmp7B6=({const char*_tmp9A="bad storage class on function";_tag_fat(_tmp9A,sizeof(char),30U);});Cyc_Warn_err(_tmp7B7,_tmp7B6,_tag_fat(_tmp99,sizeof(void*),0U));});});goto _LL0;}_LL0:;}}{
# 583
void*_tmp9B=Cyc_Parse_collapse_type_specifiers(tss,loc);void*t=_tmp9B;
struct _tuple14 _tmp9C=Cyc_Parse_apply_tms(tq,t,atts,d.tms);struct _tuple14 _stmttmpB=_tmp9C;struct _tuple14 _tmp9D=_stmttmpB;struct Cyc_List_List*_tmpA1;struct Cyc_List_List*_tmpA0;void*_tmp9F;struct Cyc_Absyn_Tqual _tmp9E;_LL8: _tmp9E=_tmp9D.f1;_tmp9F=_tmp9D.f2;_tmpA0=_tmp9D.f3;_tmpA1=_tmp9D.f4;_LL9: {struct Cyc_Absyn_Tqual fn_tqual=_tmp9E;void*fn_type=_tmp9F;struct Cyc_List_List*x=_tmpA0;struct Cyc_List_List*out_atts=_tmpA1;
# 588
if(x != 0)
# 591
({void*_tmpA2=0U;({unsigned _tmp7B9=loc;struct _fat_ptr _tmp7B8=({const char*_tmpA3="bad type params, ignoring";_tag_fat(_tmpA3,sizeof(char),26U);});Cyc_Warn_warn(_tmp7B9,_tmp7B8,_tag_fat(_tmpA2,sizeof(void*),0U));});});{
# 593
void*_tmpA4=fn_type;struct Cyc_Absyn_FnInfo _tmpA5;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA4)->tag == 5U){_LLB: _tmpA5=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA4)->f1;_LLC: {struct Cyc_Absyn_FnInfo i=_tmpA5;
# 595
{struct Cyc_List_List*_tmpA6=i.args;struct Cyc_List_List*args2=_tmpA6;for(0;args2 != 0;args2=args2->tl){
if((*((struct _tuple8*)args2->hd)).f1 == 0){
({void*_tmpA7=0U;({unsigned _tmp7BB=loc;struct _fat_ptr _tmp7BA=({const char*_tmpA8="missing argument variable in function prototype";_tag_fat(_tmpA8,sizeof(char),48U);});Cyc_Warn_err(_tmp7BB,_tmp7BA,_tag_fat(_tmpA7,sizeof(void*),0U));});});
({struct _fat_ptr*_tmp7BD=({struct _fat_ptr*_tmpAA=_cycalloc(sizeof(*_tmpAA));({struct _fat_ptr _tmp7BC=({const char*_tmpA9="?";_tag_fat(_tmpA9,sizeof(char),2U);});*_tmpAA=_tmp7BC;});_tmpAA;});(*((struct _tuple8*)args2->hd)).f1=_tmp7BD;});}}}
# 602
({struct Cyc_List_List*_tmp7BE=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(i.attributes,out_atts);i.attributes=_tmp7BE;});
return({struct Cyc_Absyn_Fndecl*_tmpAB=_cycalloc(sizeof(*_tmpAB));_tmpAB->sc=sc,_tmpAB->is_inline=is_inline,_tmpAB->name=d.id,_tmpAB->body=body,_tmpAB->i=i,_tmpAB->cached_type=0,_tmpAB->param_vardecls=0,_tmpAB->fn_vardecl=0,_tmpAB->orig_scope=sc;_tmpAB;});}}else{_LLD: _LLE:
# 607
({void*_tmpAC=0U;({unsigned _tmp7C0=loc;struct _fat_ptr _tmp7BF=({const char*_tmpAD="declarator is not a function prototype";_tag_fat(_tmpAD,sizeof(char),39U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7C0,_tmp7BF,_tag_fat(_tmpAC,sizeof(void*),0U));});});}_LLA:;}}}}}static char _tmpAE[76U]="at most one type may appear within a type specifier \n\t(missing ';' or ','?)";
# 611
static struct _fat_ptr Cyc_Parse_msg1={_tmpAE,_tmpAE,_tmpAE + 76U};static char _tmpAF[87U]="const or volatile may appear only once within a type specifier \n\t(missing ';' or ','?)";
# 613
static struct _fat_ptr Cyc_Parse_msg2={_tmpAF,_tmpAF,_tmpAF + 87U};static char _tmpB0[74U]="type specifier includes more than one declaration \n\t(missing ';' or ','?)";
# 615
static struct _fat_ptr Cyc_Parse_msg3={_tmpB0,_tmpB0,_tmpB0 + 74U};static char _tmpB1[84U]="sign specifier may appear only once within a type specifier \n\t(missing ';' or ','?)";
# 617
static struct _fat_ptr Cyc_Parse_msg4={_tmpB1,_tmpB1,_tmpB1 + 84U};
# 624
static struct Cyc_Parse_Type_specifier Cyc_Parse_combine_specifiers(unsigned loc,struct Cyc_Parse_Type_specifier s1,struct Cyc_Parse_Type_specifier s2){
# 627
if(s1.Signed_spec && s2.Signed_spec)
({void*_tmpB2=0U;({unsigned _tmp7C2=loc;struct _fat_ptr _tmp7C1=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp7C2,_tmp7C1,_tag_fat(_tmpB2,sizeof(void*),0U));});});
s1.Signed_spec |=s2.Signed_spec;
if(s1.Unsigned_spec && s2.Unsigned_spec)
({void*_tmpB3=0U;({unsigned _tmp7C4=loc;struct _fat_ptr _tmp7C3=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp7C4,_tmp7C3,_tag_fat(_tmpB3,sizeof(void*),0U));});});
s1.Unsigned_spec |=s2.Unsigned_spec;
if(s1.Short_spec && s2.Short_spec)
({void*_tmpB4=0U;({unsigned _tmp7C6=loc;struct _fat_ptr _tmp7C5=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp7C6,_tmp7C5,_tag_fat(_tmpB4,sizeof(void*),0U));});});
s1.Short_spec |=s2.Short_spec;
if((s1.Long_Long_spec && s2.Long_Long_spec ||
 s1.Long_Long_spec && s2.Long_spec)||
 s2.Long_Long_spec && s1.Long_spec)
({void*_tmpB5=0U;({unsigned _tmp7C8=loc;struct _fat_ptr _tmp7C7=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp7C8,_tmp7C7,_tag_fat(_tmpB5,sizeof(void*),0U));});});
s1.Long_Long_spec=
(s1.Long_Long_spec || s2.Long_Long_spec)|| s1.Long_spec && s2.Long_spec;
s1.Long_spec=!s1.Long_Long_spec &&(s1.Long_spec || s2.Long_spec);
if(s1.Valid_type_spec && s2.Valid_type_spec)
({void*_tmpB6=0U;({unsigned _tmp7CA=loc;struct _fat_ptr _tmp7C9=Cyc_Parse_msg1;Cyc_Warn_err(_tmp7CA,_tmp7C9,_tag_fat(_tmpB6,sizeof(void*),0U));});});else{
if(s2.Valid_type_spec){
s1.Type_spec=s2.Type_spec;
s1.Valid_type_spec=1;}}
# 649
return s1;}
# 655
static void*Cyc_Parse_collapse_type_specifiers(struct Cyc_Parse_Type_specifier ts,unsigned loc){
int seen_type=ts.Valid_type_spec;
int seen_sign=ts.Signed_spec || ts.Unsigned_spec;
int seen_size=(ts.Short_spec || ts.Long_spec)|| ts.Long_Long_spec;
void*t=seen_type?ts.Type_spec: Cyc_Absyn_void_type;
enum Cyc_Absyn_Size_of sz=2U;
enum Cyc_Absyn_Sign sgn=0U;
# 663
if(ts.Signed_spec && ts.Unsigned_spec)
({void*_tmpB7=0U;({unsigned _tmp7CC=loc;struct _fat_ptr _tmp7CB=Cyc_Parse_msg4;Cyc_Warn_err(_tmp7CC,_tmp7CB,_tag_fat(_tmpB7,sizeof(void*),0U));});});
if(ts.Unsigned_spec)sgn=1U;
if(ts.Short_spec &&(ts.Long_spec || ts.Long_Long_spec)||
 ts.Long_spec && ts.Long_Long_spec)
({void*_tmpB8=0U;({unsigned _tmp7CE=loc;struct _fat_ptr _tmp7CD=Cyc_Parse_msg4;Cyc_Warn_err(_tmp7CE,_tmp7CD,_tag_fat(_tmpB8,sizeof(void*),0U));});});
if(ts.Short_spec)sz=1U;
if(ts.Long_spec)sz=3U;
if(ts.Long_Long_spec)sz=4U;
# 675
if(!seen_type){
if(!seen_sign && !seen_size)
({void*_tmpB9=0U;({unsigned _tmp7D0=loc;struct _fat_ptr _tmp7CF=({const char*_tmpBA="missing type within specifier";_tag_fat(_tmpBA,sizeof(char),30U);});Cyc_Warn_warn(_tmp7D0,_tmp7CF,_tag_fat(_tmpB9,sizeof(void*),0U));});});
t=Cyc_Absyn_int_type(sgn,sz);}else{
# 680
if(seen_sign){
void*_tmpBB=t;enum Cyc_Absyn_Size_of _tmpBD;enum Cyc_Absyn_Sign _tmpBC;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBB)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBB)->f1)->tag == 1U){_LL1: _tmpBC=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBB)->f1)->f1;_tmpBD=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBB)->f1)->f2;_LL2: {enum Cyc_Absyn_Sign sgn2=_tmpBC;enum Cyc_Absyn_Size_of sz2=_tmpBD;
# 683
if((int)sgn2 != (int)sgn)
t=Cyc_Absyn_int_type(sgn,sz2);
goto _LL0;}}else{goto _LL3;}}else{_LL3: _LL4:
({void*_tmpBE=0U;({unsigned _tmp7D2=loc;struct _fat_ptr _tmp7D1=({const char*_tmpBF="sign specification on non-integral type";_tag_fat(_tmpBF,sizeof(char),40U);});Cyc_Warn_err(_tmp7D2,_tmp7D1,_tag_fat(_tmpBE,sizeof(void*),0U));});});goto _LL0;}_LL0:;}
# 688
if(seen_size){
void*_tmpC0=t;enum Cyc_Absyn_Size_of _tmpC2;enum Cyc_Absyn_Sign _tmpC1;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC0)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC0)->f1)){case 1U: _LL6: _tmpC1=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC0)->f1)->f1;_tmpC2=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC0)->f1)->f2;_LL7: {enum Cyc_Absyn_Sign sgn2=_tmpC1;enum Cyc_Absyn_Size_of sz2=_tmpC2;
# 691
if((int)sz2 != (int)sz)
t=Cyc_Absyn_int_type(sgn2,sz);
goto _LL5;}case 2U: _LL8: _LL9:
# 695
 t=Cyc_Absyn_long_double_type;goto _LL5;default: goto _LLA;}else{_LLA: _LLB:
({void*_tmpC3=0U;({unsigned _tmp7D4=loc;struct _fat_ptr _tmp7D3=({const char*_tmpC4="size qualifier on non-integral type";_tag_fat(_tmpC4,sizeof(char),36U);});Cyc_Warn_err(_tmp7D4,_tmp7D3,_tag_fat(_tmpC3,sizeof(void*),0U));});});goto _LL5;}_LL5:;}}
# 699
return t;}
# 702
static struct Cyc_List_List*Cyc_Parse_apply_tmss(struct _RegionHandle*r,struct Cyc_Absyn_Tqual tq,void*t,struct _tuple11*ds,struct Cyc_List_List*shared_atts){
# 706
if(ds == 0)return 0;{
struct Cyc_Parse_Declarator d=ds->hd;
struct _tuple0*_tmpC5=d.id;struct _tuple0*q=_tmpC5;
unsigned _tmpC6=d.varloc;unsigned varloc=_tmpC6;
struct _tuple14 _tmpC7=Cyc_Parse_apply_tms(tq,t,shared_atts,d.tms);struct _tuple14 _stmttmpC=_tmpC7;struct _tuple14 _tmpC8=_stmttmpC;struct Cyc_List_List*_tmpCC;struct Cyc_List_List*_tmpCB;void*_tmpCA;struct Cyc_Absyn_Tqual _tmpC9;_LL1: _tmpC9=_tmpC8.f1;_tmpCA=_tmpC8.f2;_tmpCB=_tmpC8.f3;_tmpCC=_tmpC8.f4;_LL2: {struct Cyc_Absyn_Tqual tq2=_tmpC9;void*new_typ=_tmpCA;struct Cyc_List_List*tvs=_tmpCB;struct Cyc_List_List*atts=_tmpCC;
# 713
if(ds->tl == 0)
return({struct Cyc_List_List*_tmpCE=_region_malloc(r,sizeof(*_tmpCE));({struct _tuple16*_tmp7D5=({struct _tuple16*_tmpCD=_region_malloc(r,sizeof(*_tmpCD));_tmpCD->f1=varloc,_tmpCD->f2=q,_tmpCD->f3=tq2,_tmpCD->f4=new_typ,_tmpCD->f5=tvs,_tmpCD->f6=atts;_tmpCD;});_tmpCE->hd=_tmp7D5;}),_tmpCE->tl=0;_tmpCE;});else{
# 716
return({struct Cyc_List_List*_tmpD0=_region_malloc(r,sizeof(*_tmpD0));({struct _tuple16*_tmp7DB=({struct _tuple16*_tmpCF=_region_malloc(r,sizeof(*_tmpCF));_tmpCF->f1=varloc,_tmpCF->f2=q,_tmpCF->f3=tq2,_tmpCF->f4=new_typ,_tmpCF->f5=tvs,_tmpCF->f6=atts;_tmpCF;});_tmpD0->hd=_tmp7DB;}),({
struct Cyc_List_List*_tmp7DA=({struct _RegionHandle*_tmp7D9=r;struct Cyc_Absyn_Tqual _tmp7D8=tq;void*_tmp7D7=Cyc_Tcutil_copy_type(t);struct _tuple11*_tmp7D6=ds->tl;Cyc_Parse_apply_tmss(_tmp7D9,_tmp7D8,_tmp7D7,_tmp7D6,shared_atts);});_tmpD0->tl=_tmp7DA;});_tmpD0;});}}}}
# 720
static struct _tuple14 Cyc_Parse_apply_tms(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*atts,struct Cyc_List_List*tms){
# 723
if(tms == 0)return({struct _tuple14 _tmp6DC;_tmp6DC.f1=tq,_tmp6DC.f2=t,_tmp6DC.f3=0,_tmp6DC.f4=atts;_tmp6DC;});{
void*_tmpD1=(void*)tms->hd;void*_stmttmpD=_tmpD1;void*_tmpD2=_stmttmpD;struct Cyc_List_List*_tmpD4;unsigned _tmpD3;struct Cyc_Absyn_Tqual _tmpD6;struct Cyc_Absyn_PtrAtts _tmpD5;unsigned _tmpD8;struct Cyc_List_List*_tmpD7;void*_tmpD9;unsigned _tmpDC;void*_tmpDB;struct Cyc_Absyn_Exp*_tmpDA;unsigned _tmpDE;void*_tmpDD;switch(*((int*)_tmpD2)){case 0U: _LL1: _tmpDD=(void*)((struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*)_tmpD2)->f1;_tmpDE=((struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*)_tmpD2)->f2;_LL2: {void*zeroterm=_tmpDD;unsigned ztloc=_tmpDE;
# 726
return({struct Cyc_Absyn_Tqual _tmp7DE=Cyc_Absyn_empty_tqual(0U);void*_tmp7DD=
Cyc_Absyn_array_type(t,tq,0,zeroterm,ztloc);
# 726
struct Cyc_List_List*_tmp7DC=atts;Cyc_Parse_apply_tms(_tmp7DE,_tmp7DD,_tmp7DC,tms->tl);});}case 1U: _LL3: _tmpDA=((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmpD2)->f1;_tmpDB=(void*)((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmpD2)->f2;_tmpDC=((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmpD2)->f3;_LL4: {struct Cyc_Absyn_Exp*e=_tmpDA;void*zeroterm=_tmpDB;unsigned ztloc=_tmpDC;
# 729
return({struct Cyc_Absyn_Tqual _tmp7E1=Cyc_Absyn_empty_tqual(0U);void*_tmp7E0=
Cyc_Absyn_array_type(t,tq,e,zeroterm,ztloc);
# 729
struct Cyc_List_List*_tmp7DF=atts;Cyc_Parse_apply_tms(_tmp7E1,_tmp7E0,_tmp7DF,tms->tl);});}case 3U: _LL5: _tmpD9=(void*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmpD2)->f1;_LL6: {void*args=_tmpD9;
# 732
void*_tmpDF=args;unsigned _tmpE0;struct Cyc_Absyn_Exp*_tmpE7;struct Cyc_Absyn_Exp*_tmpE6;struct Cyc_List_List*_tmpE5;void*_tmpE4;struct Cyc_Absyn_VarargInfo*_tmpE3;int _tmpE2;struct Cyc_List_List*_tmpE1;if(((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->tag == 1U){_LLE: _tmpE1=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->f1;_tmpE2=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->f2;_tmpE3=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->f3;_tmpE4=(void*)((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->f4;_tmpE5=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->f5;_tmpE6=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->f6;_tmpE7=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpDF)->f7;_LLF: {struct Cyc_List_List*args2=_tmpE1;int c_vararg=_tmpE2;struct Cyc_Absyn_VarargInfo*cyc_vararg=_tmpE3;void*eff=_tmpE4;struct Cyc_List_List*rgn_po=_tmpE5;struct Cyc_Absyn_Exp*req=_tmpE6;struct Cyc_Absyn_Exp*ens=_tmpE7;
# 734
struct Cyc_List_List*typvars=0;
# 736
struct Cyc_List_List*fn_atts=0;struct Cyc_List_List*new_atts=0;
{struct Cyc_List_List*as=atts;for(0;as != 0;as=as->tl){
if(Cyc_Atts_fntype_att((void*)as->hd))
fn_atts=({struct Cyc_List_List*_tmpE8=_cycalloc(sizeof(*_tmpE8));_tmpE8->hd=(void*)as->hd,_tmpE8->tl=fn_atts;_tmpE8;});else{
# 741
new_atts=({struct Cyc_List_List*_tmpE9=_cycalloc(sizeof(*_tmpE9));_tmpE9->hd=(void*)as->hd,_tmpE9->tl=new_atts;_tmpE9;});}}}
# 744
if(tms->tl != 0){
void*_tmpEA=(void*)((struct Cyc_List_List*)_check_null(tms->tl))->hd;void*_stmttmpE=_tmpEA;void*_tmpEB=_stmttmpE;struct Cyc_List_List*_tmpEC;if(((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpEB)->tag == 4U){_LL13: _tmpEC=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpEB)->f1;_LL14: {struct Cyc_List_List*ts=_tmpEC;
# 747
typvars=ts;
tms=tms->tl;
goto _LL12;}}else{_LL15: _LL16:
 goto _LL12;}_LL12:;}
# 754
if(((((!c_vararg && cyc_vararg == 0)&& args2 != 0)&& args2->tl == 0)&&(*((struct _tuple8*)args2->hd)).f1 == 0)&&(*((struct _tuple8*)args2->hd)).f3 == Cyc_Absyn_void_type)
# 759
args2=0;{
# 762
struct Cyc_List_List*_tmpED=Cyc_Parse_get_arg_tags(args2);struct Cyc_List_List*tags=_tmpED;
# 764
if(tags != 0)
t=Cyc_Parse_substitute_tags(tags,t);
t=Cyc_Parse_array2ptr(t,0);
# 769
{struct Cyc_List_List*_tmpEE=args2;struct Cyc_List_List*a=_tmpEE;for(0;a != 0;a=a->tl){
struct _tuple8*_tmpEF=(struct _tuple8*)a->hd;struct _tuple8*_stmttmpF=_tmpEF;struct _tuple8*_tmpF0=_stmttmpF;void**_tmpF3;struct Cyc_Absyn_Tqual _tmpF2;struct _fat_ptr*_tmpF1;_LL18: _tmpF1=_tmpF0->f1;_tmpF2=_tmpF0->f2;_tmpF3=(void**)& _tmpF0->f3;_LL19: {struct _fat_ptr*vopt=_tmpF1;struct Cyc_Absyn_Tqual tq=_tmpF2;void**t=_tmpF3;
if(tags != 0)
({void*_tmp7E2=Cyc_Parse_substitute_tags(tags,*t);*t=_tmp7E2;});
({void*_tmp7E3=Cyc_Parse_array2ptr(*t,1);*t=_tmp7E3;});}}}
# 781
return({struct Cyc_Absyn_Tqual _tmp7E6=Cyc_Absyn_empty_tqual(tq.loc);void*_tmp7E5=
Cyc_Absyn_function_type(typvars,eff,tq,t,args2,c_vararg,cyc_vararg,rgn_po,fn_atts,req,ens);
# 781
struct Cyc_List_List*_tmp7E4=new_atts;Cyc_Parse_apply_tms(_tmp7E6,_tmp7E5,_tmp7E4,((struct Cyc_List_List*)_check_null(tms))->tl);});}}}else{_LL10: _tmpE0=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)_tmpDF)->f2;_LL11: {unsigned loc=_tmpE0;
# 788
({void*_tmpF4=0U;({unsigned _tmp7E8=loc;struct _fat_ptr _tmp7E7=({const char*_tmpF5="function declaration without parameter types";_tag_fat(_tmpF5,sizeof(char),45U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7E8,_tmp7E7,_tag_fat(_tmpF4,sizeof(void*),0U));});});}}_LLD:;}case 4U: _LL7: _tmpD7=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpD2)->f1;_tmpD8=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpD2)->f2;_LL8: {struct Cyc_List_List*ts=_tmpD7;unsigned loc=_tmpD8;
# 795
if(tms->tl == 0)
return({struct _tuple14 _tmp6DD;_tmp6DD.f1=tq,_tmp6DD.f2=t,_tmp6DD.f3=ts,_tmp6DD.f4=atts;_tmp6DD;});
# 800
({void*_tmpF6=0U;({unsigned _tmp7EA=loc;struct _fat_ptr _tmp7E9=({const char*_tmpF7="type parameters must appear before function arguments in declarator";_tag_fat(_tmpF7,sizeof(char),68U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp7EA,_tmp7E9,_tag_fat(_tmpF6,sizeof(void*),0U));});});}case 2U: _LL9: _tmpD5=((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmpD2)->f1;_tmpD6=((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmpD2)->f2;_LLA: {struct Cyc_Absyn_PtrAtts ptratts=_tmpD5;struct Cyc_Absyn_Tqual tq2=_tmpD6;
# 803
return({struct Cyc_Absyn_Tqual _tmp7ED=tq2;void*_tmp7EC=Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmp6DE;_tmp6DE.elt_type=t,_tmp6DE.elt_tq=tq,_tmp6DE.ptr_atts=ptratts;_tmp6DE;}));struct Cyc_List_List*_tmp7EB=atts;Cyc_Parse_apply_tms(_tmp7ED,_tmp7EC,_tmp7EB,tms->tl);});}default: _LLB: _tmpD3=((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmpD2)->f1;_tmpD4=((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmpD2)->f2;_LLC: {unsigned loc=_tmpD3;struct Cyc_List_List*atts2=_tmpD4;
# 808
return({struct Cyc_Absyn_Tqual _tmp7F0=tq;void*_tmp7EF=t;struct Cyc_List_List*_tmp7EE=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(atts,atts2);Cyc_Parse_apply_tms(_tmp7F0,_tmp7EF,_tmp7EE,tms->tl);});}}_LL0:;}}
# 814
void*Cyc_Parse_speclist2typ(struct Cyc_Parse_Type_specifier tss,unsigned loc){
return Cyc_Parse_collapse_type_specifiers(tss,loc);}
# 823
static struct Cyc_Absyn_Decl*Cyc_Parse_v_typ_to_typedef(unsigned loc,struct _tuple16*t){
struct _tuple16*_tmpF8=t;struct Cyc_List_List*_tmpFE;struct Cyc_List_List*_tmpFD;void*_tmpFC;struct Cyc_Absyn_Tqual _tmpFB;struct _tuple0*_tmpFA;unsigned _tmpF9;_LL1: _tmpF9=_tmpF8->f1;_tmpFA=_tmpF8->f2;_tmpFB=_tmpF8->f3;_tmpFC=_tmpF8->f4;_tmpFD=_tmpF8->f5;_tmpFE=_tmpF8->f6;_LL2: {unsigned varloc=_tmpF9;struct _tuple0*x=_tmpFA;struct Cyc_Absyn_Tqual tq=_tmpFB;void*typ=_tmpFC;struct Cyc_List_List*tvs=_tmpFD;struct Cyc_List_List*atts=_tmpFE;
# 826
Cyc_Lex_register_typedef(x);{
# 828
struct Cyc_Core_Opt*kind;
void*type;
{void*_tmpFF=typ;struct Cyc_Core_Opt*_tmp100;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmpFF)->tag == 1U){_LL4: _tmp100=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmpFF)->f1;_LL5: {struct Cyc_Core_Opt*kopt=_tmp100;
# 832
type=0;
if(kopt == 0)kind=& Cyc_Tcutil_bko;else{
kind=kopt;}
goto _LL3;}}else{_LL6: _LL7:
 kind=0;type=typ;goto _LL3;}_LL3:;}
# 838
return({void*_tmp7F2=(void*)({struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_tmp102=_cycalloc(sizeof(*_tmp102));_tmp102->tag=8U,({struct Cyc_Absyn_Typedefdecl*_tmp7F1=({struct Cyc_Absyn_Typedefdecl*_tmp101=_cycalloc(sizeof(*_tmp101));_tmp101->name=x,_tmp101->tvs=tvs,_tmp101->kind=kind,_tmp101->defn=type,_tmp101->atts=atts,_tmp101->tq=tq,_tmp101->extern_c=0;_tmp101;});_tmp102->f1=_tmp7F1;});_tmp102;});Cyc_Absyn_new_decl(_tmp7F2,loc);});}}}
# 845
static struct Cyc_Absyn_Stmt*Cyc_Parse_flatten_decl(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s){
return({void*_tmp7F3=(void*)({struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_tmp103=_cycalloc(sizeof(*_tmp103));_tmp103->tag=12U,_tmp103->f1=d,_tmp103->f2=s;_tmp103;});Cyc_Absyn_new_stmt(_tmp7F3,d->loc);});}
# 849
static struct Cyc_Absyn_Stmt*Cyc_Parse_flatten_declarations(struct Cyc_List_List*ds,struct Cyc_Absyn_Stmt*s){
return((struct Cyc_Absyn_Stmt*(*)(struct Cyc_Absyn_Stmt*(*f)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum))Cyc_List_fold_right)(Cyc_Parse_flatten_decl,ds,s);}
# 858
static struct Cyc_List_List*Cyc_Parse_make_declarations(struct Cyc_Parse_Declaration_spec ds,struct _tuple13*ids,unsigned tqual_loc,unsigned loc){
# 862
struct _RegionHandle _tmp104=_new_region("mkrgn");struct _RegionHandle*mkrgn=& _tmp104;_push_region(mkrgn);
{struct Cyc_Parse_Declaration_spec _tmp105=ds;struct Cyc_List_List*_tmp108;struct Cyc_Parse_Type_specifier _tmp107;struct Cyc_Absyn_Tqual _tmp106;_LL1: _tmp106=_tmp105.tq;_tmp107=_tmp105.type_specs;_tmp108=_tmp105.attributes;_LL2: {struct Cyc_Absyn_Tqual tq=_tmp106;struct Cyc_Parse_Type_specifier tss=_tmp107;struct Cyc_List_List*atts=_tmp108;
if(tq.loc == (unsigned)0)tq.loc=tqual_loc;
if(ds.is_inline)
({void*_tmp109=0U;({unsigned _tmp7F5=loc;struct _fat_ptr _tmp7F4=({const char*_tmp10A="inline qualifier on non-function definition";_tag_fat(_tmp10A,sizeof(char),44U);});Cyc_Warn_warn(_tmp7F5,_tmp7F4,_tag_fat(_tmp109,sizeof(void*),0U));});});{
# 868
enum Cyc_Absyn_Scope s=2U;
int istypedef=0;
if(ds.sc != 0){
enum Cyc_Parse_Storage_class _tmp10B=*ds.sc;enum Cyc_Parse_Storage_class _stmttmp10=_tmp10B;enum Cyc_Parse_Storage_class _tmp10C=_stmttmp10;switch(_tmp10C){case Cyc_Parse_Typedef_sc: _LL4: _LL5:
 istypedef=1;goto _LL3;case Cyc_Parse_Extern_sc: _LL6: _LL7:
 s=3U;goto _LL3;case Cyc_Parse_ExternC_sc: _LL8: _LL9:
 s=4U;goto _LL3;case Cyc_Parse_Static_sc: _LLA: _LLB:
 s=0U;goto _LL3;case Cyc_Parse_Auto_sc: _LLC: _LLD:
 s=2U;goto _LL3;case Cyc_Parse_Register_sc: _LLE: _LLF:
 s=Cyc_Parse_no_register?Cyc_Absyn_Public: Cyc_Absyn_Register;goto _LL3;case Cyc_Parse_Abstract_sc: _LL10: _LL11:
 goto _LL13;default: _LL12: _LL13:
 s=1U;goto _LL3;}_LL3:;}{
# 885
struct _tuple11*declarators=0;
struct Cyc_List_List*exprs=0;
Cyc_Parse_decl_split(mkrgn,ids,& declarators,& exprs);{
# 889
int exps_empty=1;
{struct Cyc_List_List*es=exprs;for(0;es != 0;es=es->tl){
if((struct Cyc_Absyn_Exp*)es->hd != 0){
exps_empty=0;
break;}}}{
# 897
void*_tmp10D=Cyc_Parse_collapse_type_specifiers(tss,loc);void*base_type=_tmp10D;
if(declarators == 0){
# 901
void*_tmp10E=base_type;struct Cyc_List_List*_tmp10F;struct _tuple0*_tmp110;struct Cyc_List_List*_tmp113;int _tmp112;struct _tuple0*_tmp111;struct Cyc_Absyn_Datatypedecl**_tmp114;struct Cyc_List_List*_tmp117;struct _tuple0*_tmp116;enum Cyc_Absyn_AggrKind _tmp115;struct Cyc_Absyn_Datatypedecl*_tmp118;struct Cyc_Absyn_Enumdecl*_tmp119;struct Cyc_Absyn_Aggrdecl*_tmp11A;switch(*((int*)_tmp10E)){case 10U: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp10E)->f1)->r)){case 0U: _LL15: _tmp11A=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp10E)->f1)->r)->f1;_LL16: {struct Cyc_Absyn_Aggrdecl*ad=_tmp11A;
# 903
({struct Cyc_List_List*_tmp7F6=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(ad->attributes,atts);ad->attributes=_tmp7F6;});
ad->sc=s;{
struct Cyc_List_List*_tmp11D=({struct Cyc_List_List*_tmp11C=_cycalloc(sizeof(*_tmp11C));({struct Cyc_Absyn_Decl*_tmp7F8=({void*_tmp7F7=(void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp11B=_cycalloc(sizeof(*_tmp11B));_tmp11B->tag=5U,_tmp11B->f1=ad;_tmp11B;});Cyc_Absyn_new_decl(_tmp7F7,loc);});_tmp11C->hd=_tmp7F8;}),_tmp11C->tl=0;_tmp11C;});_npop_handler(0U);return _tmp11D;}}case 1U: _LL17: _tmp119=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp10E)->f1)->r)->f1;_LL18: {struct Cyc_Absyn_Enumdecl*ed=_tmp119;
# 907
if(atts != 0)({void*_tmp11E=0U;({unsigned _tmp7FA=loc;struct _fat_ptr _tmp7F9=({const char*_tmp11F="attributes on enum not supported";_tag_fat(_tmp11F,sizeof(char),33U);});Cyc_Warn_err(_tmp7FA,_tmp7F9,_tag_fat(_tmp11E,sizeof(void*),0U));});});
ed->sc=s;{
struct Cyc_List_List*_tmp122=({struct Cyc_List_List*_tmp121=_cycalloc(sizeof(*_tmp121));({struct Cyc_Absyn_Decl*_tmp7FC=({void*_tmp7FB=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp120=_cycalloc(sizeof(*_tmp120));_tmp120->tag=7U,_tmp120->f1=ed;_tmp120;});Cyc_Absyn_new_decl(_tmp7FB,loc);});_tmp121->hd=_tmp7FC;}),_tmp121->tl=0;_tmp121;});_npop_handler(0U);return _tmp122;}}default: _LL19: _tmp118=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp10E)->f1)->r)->f1;_LL1A: {struct Cyc_Absyn_Datatypedecl*dd=_tmp118;
# 911
if(atts != 0)({void*_tmp123=0U;({unsigned _tmp7FE=loc;struct _fat_ptr _tmp7FD=({const char*_tmp124="attributes on datatypes not supported";_tag_fat(_tmp124,sizeof(char),38U);});Cyc_Warn_err(_tmp7FE,_tmp7FD,_tag_fat(_tmp123,sizeof(void*),0U));});});
dd->sc=s;{
struct Cyc_List_List*_tmp127=({struct Cyc_List_List*_tmp126=_cycalloc(sizeof(*_tmp126));({struct Cyc_Absyn_Decl*_tmp800=({void*_tmp7FF=(void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp125=_cycalloc(sizeof(*_tmp125));_tmp125->tag=6U,_tmp125->f1=dd;_tmp125;});Cyc_Absyn_new_decl(_tmp7FF,loc);});_tmp126->hd=_tmp800;}),_tmp126->tl=0;_tmp126;});_npop_handler(0U);return _tmp127;}}}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)){case 20U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1).UnknownAggr).tag == 1){_LL1B: _tmp115=(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1).UnknownAggr).val).f1;_tmp116=(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1).UnknownAggr).val).f2;_tmp117=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f2;_LL1C: {enum Cyc_Absyn_AggrKind k=_tmp115;struct _tuple0*n=_tmp116;struct Cyc_List_List*ts=_tmp117;
# 915
struct Cyc_List_List*_tmp128=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,loc,ts);struct Cyc_List_List*ts2=_tmp128;
struct Cyc_Absyn_Aggrdecl*_tmp129=({struct Cyc_Absyn_Aggrdecl*_tmp12F=_cycalloc(sizeof(*_tmp12F));_tmp12F->kind=k,_tmp12F->sc=s,_tmp12F->name=n,_tmp12F->tvs=ts2,_tmp12F->impl=0,_tmp12F->attributes=0,_tmp12F->expected_mem_kind=0;_tmp12F;});struct Cyc_Absyn_Aggrdecl*ad=_tmp129;
if(atts != 0)({void*_tmp12A=0U;({unsigned _tmp802=loc;struct _fat_ptr _tmp801=({const char*_tmp12B="bad attributes on type declaration";_tag_fat(_tmp12B,sizeof(char),35U);});Cyc_Warn_err(_tmp802,_tmp801,_tag_fat(_tmp12A,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp12E=({struct Cyc_List_List*_tmp12D=_cycalloc(sizeof(*_tmp12D));({struct Cyc_Absyn_Decl*_tmp804=({void*_tmp803=(void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp12C=_cycalloc(sizeof(*_tmp12C));_tmp12C->tag=5U,_tmp12C->f1=ad;_tmp12C;});Cyc_Absyn_new_decl(_tmp803,loc);});_tmp12D->hd=_tmp804;}),_tmp12D->tl=0;_tmp12D;});_npop_handler(0U);return _tmp12E;}}}else{goto _LL25;}case 18U: if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1).KnownDatatype).tag == 2){_LL1D: _tmp114=((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1).KnownDatatype).val;_LL1E: {struct Cyc_Absyn_Datatypedecl**tudp=_tmp114;
# 920
if(atts != 0)({void*_tmp130=0U;({unsigned _tmp806=loc;struct _fat_ptr _tmp805=({const char*_tmp131="bad attributes on datatype";_tag_fat(_tmp131,sizeof(char),27U);});Cyc_Warn_err(_tmp806,_tmp805,_tag_fat(_tmp130,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp134=({struct Cyc_List_List*_tmp133=_cycalloc(sizeof(*_tmp133));({struct Cyc_Absyn_Decl*_tmp808=({void*_tmp807=(void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp132=_cycalloc(sizeof(*_tmp132));_tmp132->tag=6U,_tmp132->f1=*tudp;_tmp132;});Cyc_Absyn_new_decl(_tmp807,loc);});_tmp133->hd=_tmp808;}),_tmp133->tl=0;_tmp133;});_npop_handler(0U);return _tmp134;}}}else{_LL1F: _tmp111=(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1).UnknownDatatype).val).name;_tmp112=(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1).UnknownDatatype).val).is_extensible;_tmp113=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f2;_LL20: {struct _tuple0*n=_tmp111;int isx=_tmp112;struct Cyc_List_List*ts=_tmp113;
# 923
struct Cyc_List_List*_tmp135=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,loc,ts);struct Cyc_List_List*ts2=_tmp135;
struct Cyc_Absyn_Decl*_tmp136=Cyc_Absyn_datatype_decl(s,n,ts2,0,isx,loc);struct Cyc_Absyn_Decl*tud=_tmp136;
if(atts != 0)({void*_tmp137=0U;({unsigned _tmp80A=loc;struct _fat_ptr _tmp809=({const char*_tmp138="bad attributes on datatype";_tag_fat(_tmp138,sizeof(char),27U);});Cyc_Warn_err(_tmp80A,_tmp809,_tag_fat(_tmp137,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp13A=({struct Cyc_List_List*_tmp139=_cycalloc(sizeof(*_tmp139));_tmp139->hd=tud,_tmp139->tl=0;_tmp139;});_npop_handler(0U);return _tmp13A;}}}case 15U: _LL21: _tmp110=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1;_LL22: {struct _tuple0*n=_tmp110;
# 928
struct Cyc_Absyn_Enumdecl*_tmp13B=({struct Cyc_Absyn_Enumdecl*_tmp142=_cycalloc(sizeof(*_tmp142));_tmp142->sc=s,_tmp142->name=n,_tmp142->fields=0;_tmp142;});struct Cyc_Absyn_Enumdecl*ed=_tmp13B;
if(atts != 0)({void*_tmp13C=0U;({unsigned _tmp80C=loc;struct _fat_ptr _tmp80B=({const char*_tmp13D="bad attributes on enum";_tag_fat(_tmp13D,sizeof(char),23U);});Cyc_Warn_err(_tmp80C,_tmp80B,_tag_fat(_tmp13C,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp141=({struct Cyc_List_List*_tmp140=_cycalloc(sizeof(*_tmp140));({struct Cyc_Absyn_Decl*_tmp80E=({struct Cyc_Absyn_Decl*_tmp13F=_cycalloc(sizeof(*_tmp13F));({void*_tmp80D=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp13E=_cycalloc(sizeof(*_tmp13E));_tmp13E->tag=7U,_tmp13E->f1=ed;_tmp13E;});_tmp13F->r=_tmp80D;}),_tmp13F->loc=loc;_tmp13F;});_tmp140->hd=_tmp80E;}),_tmp140->tl=0;_tmp140;});_npop_handler(0U);return _tmp141;}}case 16U: _LL23: _tmp10F=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10E)->f1)->f1;_LL24: {struct Cyc_List_List*fs=_tmp10F;
# 934
struct Cyc_Absyn_Enumdecl*_tmp143=({struct Cyc_Absyn_Enumdecl*_tmp14B=_cycalloc(sizeof(*_tmp14B));_tmp14B->sc=s,({struct _tuple0*_tmp810=Cyc_Parse_gensym_enum();_tmp14B->name=_tmp810;}),({struct Cyc_Core_Opt*_tmp80F=({struct Cyc_Core_Opt*_tmp14A=_cycalloc(sizeof(*_tmp14A));_tmp14A->v=fs;_tmp14A;});_tmp14B->fields=_tmp80F;});_tmp14B;});struct Cyc_Absyn_Enumdecl*ed=_tmp143;
if(atts != 0)({void*_tmp144=0U;({unsigned _tmp812=loc;struct _fat_ptr _tmp811=({const char*_tmp145="bad attributes on enum";_tag_fat(_tmp145,sizeof(char),23U);});Cyc_Warn_err(_tmp812,_tmp811,_tag_fat(_tmp144,sizeof(void*),0U));});});{
struct Cyc_List_List*_tmp149=({struct Cyc_List_List*_tmp148=_cycalloc(sizeof(*_tmp148));({struct Cyc_Absyn_Decl*_tmp814=({struct Cyc_Absyn_Decl*_tmp147=_cycalloc(sizeof(*_tmp147));({void*_tmp813=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp146=_cycalloc(sizeof(*_tmp146));_tmp146->tag=7U,_tmp146->f1=ed;_tmp146;});_tmp147->r=_tmp813;}),_tmp147->loc=loc;_tmp147;});_tmp148->hd=_tmp814;}),_tmp148->tl=0;_tmp148;});_npop_handler(0U);return _tmp149;}}default: goto _LL25;}default: _LL25: _LL26:
({void*_tmp14C=0U;({unsigned _tmp816=loc;struct _fat_ptr _tmp815=({const char*_tmp14D="missing declarator";_tag_fat(_tmp14D,sizeof(char),19U);});Cyc_Warn_err(_tmp816,_tmp815,_tag_fat(_tmp14C,sizeof(void*),0U));});});{struct Cyc_List_List*_tmp14E=0;_npop_handler(0U);return _tmp14E;}}_LL14:;}else{
# 941
struct Cyc_List_List*_tmp14F=Cyc_Parse_apply_tmss(mkrgn,tq,base_type,declarators,atts);struct Cyc_List_List*fields=_tmp14F;
if(istypedef){
# 946
if(!exps_empty)
({void*_tmp150=0U;({unsigned _tmp818=loc;struct _fat_ptr _tmp817=({const char*_tmp151="initializer in typedef declaration";_tag_fat(_tmp151,sizeof(char),35U);});Cyc_Warn_err(_tmp818,_tmp817,_tag_fat(_tmp150,sizeof(void*),0U));});});{
struct Cyc_List_List*decls=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Decl*(*f)(unsigned,struct _tuple16*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_v_typ_to_typedef,loc,fields);
struct Cyc_List_List*_tmp152=decls;_npop_handler(0U);return _tmp152;}}else{
# 952
struct Cyc_List_List*decls=0;
{struct Cyc_List_List*_tmp153=fields;struct Cyc_List_List*ds=_tmp153;for(0;ds != 0;(ds=ds->tl,exprs=((struct Cyc_List_List*)_check_null(exprs))->tl)){
struct _tuple16*_tmp154=(struct _tuple16*)ds->hd;struct _tuple16*_stmttmp11=_tmp154;struct _tuple16*_tmp155=_stmttmp11;struct Cyc_List_List*_tmp15B;struct Cyc_List_List*_tmp15A;void*_tmp159;struct Cyc_Absyn_Tqual _tmp158;struct _tuple0*_tmp157;unsigned _tmp156;_LL28: _tmp156=_tmp155->f1;_tmp157=_tmp155->f2;_tmp158=_tmp155->f3;_tmp159=_tmp155->f4;_tmp15A=_tmp155->f5;_tmp15B=_tmp155->f6;_LL29: {unsigned varloc=_tmp156;struct _tuple0*x=_tmp157;struct Cyc_Absyn_Tqual tq2=_tmp158;void*t2=_tmp159;struct Cyc_List_List*tvs2=_tmp15A;struct Cyc_List_List*atts2=_tmp15B;
if(tvs2 != 0)
({void*_tmp15C=0U;({unsigned _tmp81A=loc;struct _fat_ptr _tmp819=({const char*_tmp15D="bad type params, ignoring";_tag_fat(_tmp15D,sizeof(char),26U);});Cyc_Warn_warn(_tmp81A,_tmp819,_tag_fat(_tmp15C,sizeof(void*),0U));});});
if(exprs == 0)
({void*_tmp15E=0U;({unsigned _tmp81C=loc;struct _fat_ptr _tmp81B=({const char*_tmp15F="unexpected NULL in parse!";_tag_fat(_tmp15F,sizeof(char),26U);});((int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort)(_tmp81C,_tmp81B,_tag_fat(_tmp15E,sizeof(void*),0U));});});{
struct Cyc_Absyn_Exp*_tmp160=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(exprs))->hd;struct Cyc_Absyn_Exp*eopt=_tmp160;
struct Cyc_Absyn_Vardecl*_tmp161=Cyc_Absyn_new_vardecl(varloc,x,t2,eopt);struct Cyc_Absyn_Vardecl*vd=_tmp161;
vd->tq=tq2;
vd->sc=s;
vd->attributes=atts2;{
struct Cyc_Absyn_Decl*_tmp162=({struct Cyc_Absyn_Decl*_tmp165=_cycalloc(sizeof(*_tmp165));({void*_tmp81D=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp164=_cycalloc(sizeof(*_tmp164));_tmp164->tag=0U,_tmp164->f1=vd;_tmp164;});_tmp165->r=_tmp81D;}),_tmp165->loc=loc;_tmp165;});struct Cyc_Absyn_Decl*d=_tmp162;
decls=({struct Cyc_List_List*_tmp163=_cycalloc(sizeof(*_tmp163));_tmp163->hd=d,_tmp163->tl=decls;_tmp163;});}}}}}{
# 967
struct Cyc_List_List*_tmp166=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(decls);_npop_handler(0U);return _tmp166;}}}}}}}}}
# 863
;_pop_region();}
# 973
static struct Cyc_Absyn_Kind*Cyc_Parse_id_to_kind(struct _fat_ptr s,unsigned loc){
if(Cyc_strlen((struct _fat_ptr)s)== (unsigned long)1 || Cyc_strlen((struct _fat_ptr)s)== (unsigned long)2){
char _tmp167=*((const char*)_check_fat_subscript(s,sizeof(char),0));char _stmttmp12=_tmp167;char _tmp168=_stmttmp12;switch(_tmp168){case 65U: _LL1: _LL2:
 return& Cyc_Tcutil_ak;case 77U: _LL3: _LL4:
 return& Cyc_Tcutil_mk;case 66U: _LL5: _LL6:
 return& Cyc_Tcutil_bk;case 82U: _LL7: _LL8:
 return& Cyc_Tcutil_rk;case 69U: _LL9: _LLA:
 return& Cyc_Tcutil_ek;case 73U: _LLB: _LLC:
 return& Cyc_Tcutil_ik;case 85U: _LLD: _LLE:
# 983
{char _tmp169=*((const char*)_check_fat_subscript(s,sizeof(char),1));char _stmttmp13=_tmp169;char _tmp16A=_stmttmp13;switch(_tmp16A){case 82U: _LL14: _LL15:
 return& Cyc_Tcutil_urk;case 65U: _LL16: _LL17:
 return& Cyc_Tcutil_uak;case 77U: _LL18: _LL19:
 return& Cyc_Tcutil_umk;case 66U: _LL1A: _LL1B:
 return& Cyc_Tcutil_ubk;default: _LL1C: _LL1D:
 goto _LL13;}_LL13:;}
# 990
goto _LL0;case 84U: _LLF: _LL10:
# 992
{char _tmp16B=*((const char*)_check_fat_subscript(s,sizeof(char),1));char _stmttmp14=_tmp16B;char _tmp16C=_stmttmp14;switch(_tmp16C){case 82U: _LL1F: _LL20:
 return& Cyc_Tcutil_trk;case 65U: _LL21: _LL22:
 return& Cyc_Tcutil_tak;case 77U: _LL23: _LL24:
 return& Cyc_Tcutil_tmk;case 66U: _LL25: _LL26:
 return& Cyc_Tcutil_tbk;default: _LL27: _LL28:
 goto _LL1E;}_LL1E:;}
# 999
goto _LL0;default: _LL11: _LL12:
 goto _LL0;}_LL0:;}
# 1002
({struct Cyc_String_pa_PrintArg_struct _tmp16F=({struct Cyc_String_pa_PrintArg_struct _tmp6E0;_tmp6E0.tag=0U,_tmp6E0.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp6E0;});struct Cyc_Int_pa_PrintArg_struct _tmp170=({struct Cyc_Int_pa_PrintArg_struct _tmp6DF;_tmp6DF.tag=1U,({unsigned long _tmp81E=(unsigned long)((int)Cyc_strlen((struct _fat_ptr)s));_tmp6DF.f1=_tmp81E;});_tmp6DF;});void*_tmp16D[2U];_tmp16D[0]=& _tmp16F,_tmp16D[1]=& _tmp170;({unsigned _tmp820=loc;struct _fat_ptr _tmp81F=({const char*_tmp16E="bad kind: %s; strlen=%d";_tag_fat(_tmp16E,sizeof(char),24U);});Cyc_Warn_err(_tmp820,_tmp81F,_tag_fat(_tmp16D,sizeof(void*),2U));});});
return& Cyc_Tcutil_bk;}
# 1007
static int Cyc_Parse_exp2int(unsigned loc,struct Cyc_Absyn_Exp*e){
void*_tmp171=e->r;void*_stmttmp15=_tmp171;void*_tmp172=_stmttmp15;int _tmp173;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp172)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp172)->f1).Int_c).tag == 5){_LL1: _tmp173=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp172)->f1).Int_c).val).f2;_LL2: {int i=_tmp173;
return i;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1011
({void*_tmp174=0U;({unsigned _tmp822=loc;struct _fat_ptr _tmp821=({const char*_tmp175="expecting integer constant";_tag_fat(_tmp175,sizeof(char),27U);});Cyc_Warn_err(_tmp822,_tmp821,_tag_fat(_tmp174,sizeof(void*),0U));});});
return 0;}_LL0:;}
# 1017
static struct _fat_ptr Cyc_Parse_exp2string(unsigned loc,struct Cyc_Absyn_Exp*e){
void*_tmp176=e->r;void*_stmttmp16=_tmp176;void*_tmp177=_stmttmp16;struct _fat_ptr _tmp178;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp177)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp177)->f1).String_c).tag == 8){_LL1: _tmp178=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp177)->f1).String_c).val;_LL2: {struct _fat_ptr s=_tmp178;
return s;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1021
({void*_tmp179=0U;({unsigned _tmp824=loc;struct _fat_ptr _tmp823=({const char*_tmp17A="expecting string constant";_tag_fat(_tmp17A,sizeof(char),26U);});Cyc_Warn_err(_tmp824,_tmp823,_tag_fat(_tmp179,sizeof(void*),0U));});});
return _tag_fat(0,0,0);}_LL0:;}
# 1027
static unsigned Cyc_Parse_cnst2uint(unsigned loc,union Cyc_Absyn_Cnst x){
union Cyc_Absyn_Cnst _tmp17B=x;long long _tmp17C;char _tmp17D;int _tmp17E;switch((_tmp17B.LongLong_c).tag){case 5U: _LL1: _tmp17E=((_tmp17B.Int_c).val).f2;_LL2: {int i=_tmp17E;
return(unsigned)i;}case 2U: _LL3: _tmp17D=((_tmp17B.Char_c).val).f2;_LL4: {char c=_tmp17D;
return(unsigned)c;}case 6U: _LL5: _tmp17C=((_tmp17B.LongLong_c).val).f2;_LL6: {long long x=_tmp17C;
# 1032
unsigned long long y=(unsigned long long)x;
if(y > (unsigned long long)-1)
({void*_tmp17F=0U;({unsigned _tmp826=loc;struct _fat_ptr _tmp825=({const char*_tmp180="integer constant too large";_tag_fat(_tmp180,sizeof(char),27U);});Cyc_Warn_err(_tmp826,_tmp825,_tag_fat(_tmp17F,sizeof(void*),0U));});});
return(unsigned)x;}default: _LL7: _LL8:
# 1037
({struct Cyc_String_pa_PrintArg_struct _tmp183=({struct Cyc_String_pa_PrintArg_struct _tmp6E1;_tmp6E1.tag=0U,({struct _fat_ptr _tmp827=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_cnst2string(x));_tmp6E1.f1=_tmp827;});_tmp6E1;});void*_tmp181[1U];_tmp181[0]=& _tmp183;({unsigned _tmp829=loc;struct _fat_ptr _tmp828=({const char*_tmp182="expected integer constant but found %s";_tag_fat(_tmp182,sizeof(char),39U);});Cyc_Warn_err(_tmp829,_tmp828,_tag_fat(_tmp181,sizeof(void*),1U));});});
return 0U;}_LL0:;}
# 1043
static struct Cyc_Absyn_Exp*Cyc_Parse_pat2exp(struct Cyc_Absyn_Pat*p){
void*_tmp184=p->r;void*_stmttmp17=_tmp184;void*_tmp185=_stmttmp17;struct Cyc_Absyn_Exp*_tmp186;struct Cyc_List_List*_tmp188;struct _tuple0*_tmp187;int _tmp18A;struct _fat_ptr _tmp189;char _tmp18B;int _tmp18D;enum Cyc_Absyn_Sign _tmp18C;struct Cyc_Absyn_Pat*_tmp18E;struct Cyc_Absyn_Vardecl*_tmp18F;struct _tuple0*_tmp190;switch(*((int*)_tmp185)){case 15U: _LL1: _tmp190=((struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_LL2: {struct _tuple0*x=_tmp190;
return Cyc_Absyn_unknownid_exp(x,p->loc);}case 3U: if(((struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp185)->f2)->r)->tag == 0U){_LL3: _tmp18F=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp18F;
# 1047
return({struct Cyc_Absyn_Exp*_tmp82A=Cyc_Absyn_unknownid_exp(vd->name,p->loc);Cyc_Absyn_deref_exp(_tmp82A,p->loc);});}}else{goto _LL13;}case 6U: _LL5: _tmp18E=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_LL6: {struct Cyc_Absyn_Pat*p2=_tmp18E;
return({struct Cyc_Absyn_Exp*_tmp82B=Cyc_Parse_pat2exp(p2);Cyc_Absyn_address_exp(_tmp82B,p->loc);});}case 9U: _LL7: _LL8:
 return Cyc_Absyn_null_exp(p->loc);case 10U: _LL9: _tmp18C=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_tmp18D=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp185)->f2;_LLA: {enum Cyc_Absyn_Sign s=_tmp18C;int i=_tmp18D;
return Cyc_Absyn_int_exp(s,i,p->loc);}case 11U: _LLB: _tmp18B=((struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_LLC: {char c=_tmp18B;
return Cyc_Absyn_char_exp(c,p->loc);}case 12U: _LLD: _tmp189=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_tmp18A=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp185)->f2;_LLE: {struct _fat_ptr s=_tmp189;int i=_tmp18A;
return Cyc_Absyn_float_exp(s,i,p->loc);}case 16U: if(((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp185)->f3 == 0){_LLF: _tmp187=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_tmp188=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp185)->f2;_LL10: {struct _tuple0*x=_tmp187;struct Cyc_List_List*ps=_tmp188;
# 1054
struct Cyc_Absyn_Exp*e1=Cyc_Absyn_unknownid_exp(x,p->loc);
struct Cyc_List_List*es=((struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x))Cyc_List_map)(Cyc_Parse_pat2exp,ps);
return Cyc_Absyn_unknowncall_exp(e1,es,p->loc);}}else{goto _LL13;}case 17U: _LL11: _tmp186=((struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_tmp185)->f1;_LL12: {struct Cyc_Absyn_Exp*e=_tmp186;
return e;}default: _LL13: _LL14:
# 1059
({void*_tmp191=0U;({unsigned _tmp82D=p->loc;struct _fat_ptr _tmp82C=({const char*_tmp192="cannot mix patterns and expressions in case";_tag_fat(_tmp192,sizeof(char),44U);});Cyc_Warn_err(_tmp82D,_tmp82C,_tag_fat(_tmp191,sizeof(void*),0U));});});
return Cyc_Absyn_null_exp(p->loc);}_LL0:;}struct _union_YYSTYPE_Int_tok{int tag;union Cyc_Absyn_Cnst val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _fat_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple0*val;};struct _tuple21{int f1;struct _fat_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple21 val;};struct _union_YYSTYPE_Exp_tok{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_Stmt_tok{int tag;struct Cyc_Absyn_Stmt*val;};struct _tuple22{unsigned f1;void*f2;void*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple22*val;};struct _union_YYSTYPE_YY2{int tag;void*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY6{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY8{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple23{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY10{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _tuple24{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY12{int tag;struct _tuple24*val;};struct _union_YYSTYPE_YY13{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY14{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY15{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct Cyc_Parse_Declaration_spec val;};struct _union_YYSTYPE_YY18{int tag;struct _tuple12 val;};struct _union_YYSTYPE_YY19{int tag;struct _tuple13*val;};struct _union_YYSTYPE_YY20{int tag;enum Cyc_Parse_Storage_class*val;};struct _union_YYSTYPE_YY21{int tag;struct Cyc_Parse_Type_specifier val;};struct _union_YYSTYPE_YY22{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY23{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY25{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_Parse_Declarator val;};struct _tuple25{struct Cyc_Parse_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY28{int tag;struct _tuple25*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Parse_Abstractdeclarator val;};struct _union_YYSTYPE_YY31{int tag;int val;};struct _union_YYSTYPE_YY32{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY34{int tag;struct Cyc_List_List*val;};struct _tuple26{struct Cyc_Absyn_Tqual f1;struct Cyc_Parse_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY35{int tag;struct _tuple26 val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY37{int tag;struct _tuple8*val;};struct _union_YYSTYPE_YY38{int tag;struct Cyc_List_List*val;};struct _tuple27{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY39{int tag;struct _tuple27*val;};struct _union_YYSTYPE_YY40{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY42{int tag;void*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY44{int tag;void*val;};struct _union_YYSTYPE_YY45{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY46{int tag;void*val;};struct _union_YYSTYPE_YY47{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY51{int tag;void*val;};struct _union_YYSTYPE_YY52{int tag;struct Cyc_List_List*val;};struct _tuple28{struct Cyc_List_List*f1;unsigned f2;};struct _union_YYSTYPE_YY53{int tag;struct _tuple28*val;};struct _union_YYSTYPE_YY54{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY55{int tag;void*val;};struct _union_YYSTYPE_YY56{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY58{int tag;void*val;};struct _tuple29{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY59{int tag;struct _tuple29*val;};struct _tuple30{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct _union_YYSTYPE_YY60{int tag;struct _tuple30*val;};struct _union_YYSTYPE_YY61{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY62{int tag;struct Cyc_List_List*val;};struct _tuple31{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};struct _union_YYSTYPE_YY63{int tag;struct _tuple31*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_Exp_tok Exp_tok;struct _union_YYSTYPE_Stmt_tok Stmt_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YY58 YY58;struct _union_YYSTYPE_YY59 YY59;struct _union_YYSTYPE_YY60 YY60;struct _union_YYSTYPE_YY61 YY61;struct _union_YYSTYPE_YY62 YY62;struct _union_YYSTYPE_YY63 YY63;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};
# 1144
static void Cyc_yythrowfail(struct _fat_ptr s){
(int)_throw((void*)({struct Cyc_Core_Failure_exn_struct*_tmp193=_cycalloc(sizeof(*_tmp193));_tmp193->tag=Cyc_Core_Failure,_tmp193->f1=s;_tmp193;}));}static char _tmp196[7U]="cnst_t";
# 1116 "parse.y"
static union Cyc_Absyn_Cnst Cyc_yyget_Int_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp196,_tmp196,_tmp196 + 7U};
union Cyc_YYSTYPE*_tmp194=yy1;union Cyc_Absyn_Cnst _tmp195;if((((union Cyc_YYSTYPE*)_tmp194)->Int_tok).tag == 1){_LL1: _tmp195=(_tmp194->Int_tok).val;_LL2: {union Cyc_Absyn_Cnst yy=_tmp195;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1123
static union Cyc_YYSTYPE Cyc_Int_tok(union Cyc_Absyn_Cnst yy1){return({union Cyc_YYSTYPE _tmp6E2;(_tmp6E2.Int_tok).tag=1U,(_tmp6E2.Int_tok).val=yy1;_tmp6E2;});}static char _tmp199[5U]="char";
# 1117 "parse.y"
static char Cyc_yyget_Char_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp199,_tmp199,_tmp199 + 5U};
union Cyc_YYSTYPE*_tmp197=yy1;char _tmp198;if((((union Cyc_YYSTYPE*)_tmp197)->Char_tok).tag == 2){_LL1: _tmp198=(_tmp197->Char_tok).val;_LL2: {char yy=_tmp198;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1124
static union Cyc_YYSTYPE Cyc_Char_tok(char yy1){return({union Cyc_YYSTYPE _tmp6E3;(_tmp6E3.Char_tok).tag=2U,(_tmp6E3.Char_tok).val=yy1;_tmp6E3;});}static char _tmp19C[13U]="string_t<`H>";
# 1118 "parse.y"
static struct _fat_ptr Cyc_yyget_String_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp19C,_tmp19C,_tmp19C + 13U};
union Cyc_YYSTYPE*_tmp19A=yy1;struct _fat_ptr _tmp19B;if((((union Cyc_YYSTYPE*)_tmp19A)->String_tok).tag == 3){_LL1: _tmp19B=(_tmp19A->String_tok).val;_LL2: {struct _fat_ptr yy=_tmp19B;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1125
static union Cyc_YYSTYPE Cyc_String_tok(struct _fat_ptr yy1){return({union Cyc_YYSTYPE _tmp6E4;(_tmp6E4.String_tok).tag=3U,(_tmp6E4.String_tok).val=yy1;_tmp6E4;});}static char _tmp19F[45U]="$(Position::seg_t,booltype_t, ptrbound_t)@`H";
# 1121 "parse.y"
static struct _tuple22*Cyc_yyget_YY1(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp19F,_tmp19F,_tmp19F + 45U};
union Cyc_YYSTYPE*_tmp19D=yy1;struct _tuple22*_tmp19E;if((((union Cyc_YYSTYPE*)_tmp19D)->YY1).tag == 9){_LL1: _tmp19E=(_tmp19D->YY1).val;_LL2: {struct _tuple22*yy=_tmp19E;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1128
static union Cyc_YYSTYPE Cyc_YY1(struct _tuple22*yy1){return({union Cyc_YYSTYPE _tmp6E5;(_tmp6E5.YY1).tag=9U,(_tmp6E5.YY1).val=yy1;_tmp6E5;});}static char _tmp1A2[11U]="ptrbound_t";
# 1122 "parse.y"
static void*Cyc_yyget_YY2(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1A2,_tmp1A2,_tmp1A2 + 11U};
union Cyc_YYSTYPE*_tmp1A0=yy1;void*_tmp1A1;if((((union Cyc_YYSTYPE*)_tmp1A0)->YY2).tag == 10){_LL1: _tmp1A1=(_tmp1A0->YY2).val;_LL2: {void*yy=_tmp1A1;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1129
static union Cyc_YYSTYPE Cyc_YY2(void*yy1){return({union Cyc_YYSTYPE _tmp6E6;(_tmp6E6.YY2).tag=10U,(_tmp6E6.YY2).val=yy1;_tmp6E6;});}static char _tmp1A5[28U]="list_t<offsetof_field_t,`H>";
# 1123 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY3(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1A5,_tmp1A5,_tmp1A5 + 28U};
union Cyc_YYSTYPE*_tmp1A3=yy1;struct Cyc_List_List*_tmp1A4;if((((union Cyc_YYSTYPE*)_tmp1A3)->YY3).tag == 11){_LL1: _tmp1A4=(_tmp1A3->YY3).val;_LL2: {struct Cyc_List_List*yy=_tmp1A4;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1130
static union Cyc_YYSTYPE Cyc_YY3(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6E7;(_tmp6E7.YY3).tag=11U,(_tmp6E7.YY3).val=yy1;_tmp6E7;});}static char _tmp1A8[6U]="exp_t";
# 1124 "parse.y"
static struct Cyc_Absyn_Exp*Cyc_yyget_Exp_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1A8,_tmp1A8,_tmp1A8 + 6U};
union Cyc_YYSTYPE*_tmp1A6=yy1;struct Cyc_Absyn_Exp*_tmp1A7;if((((union Cyc_YYSTYPE*)_tmp1A6)->Exp_tok).tag == 7){_LL1: _tmp1A7=(_tmp1A6->Exp_tok).val;_LL2: {struct Cyc_Absyn_Exp*yy=_tmp1A7;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1131
static union Cyc_YYSTYPE Cyc_Exp_tok(struct Cyc_Absyn_Exp*yy1){return({union Cyc_YYSTYPE _tmp6E8;(_tmp6E8.Exp_tok).tag=7U,(_tmp6E8.Exp_tok).val=yy1;_tmp6E8;});}static char _tmp1AB[17U]="list_t<exp_t,`H>";
static struct Cyc_List_List*Cyc_yyget_YY4(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1AB,_tmp1AB,_tmp1AB + 17U};
union Cyc_YYSTYPE*_tmp1A9=yy1;struct Cyc_List_List*_tmp1AA;if((((union Cyc_YYSTYPE*)_tmp1A9)->YY4).tag == 12){_LL1: _tmp1AA=(_tmp1A9->YY4).val;_LL2: {struct Cyc_List_List*yy=_tmp1AA;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1139
static union Cyc_YYSTYPE Cyc_YY4(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6E9;(_tmp6E9.YY4).tag=12U,(_tmp6E9.YY4).val=yy1;_tmp6E9;});}static char _tmp1AE[47U]="list_t<$(list_t<designator_t,`H>,exp_t)@`H,`H>";
# 1133 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY5(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1AE,_tmp1AE,_tmp1AE + 47U};
union Cyc_YYSTYPE*_tmp1AC=yy1;struct Cyc_List_List*_tmp1AD;if((((union Cyc_YYSTYPE*)_tmp1AC)->YY5).tag == 13){_LL1: _tmp1AD=(_tmp1AC->YY5).val;_LL2: {struct Cyc_List_List*yy=_tmp1AD;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1140
static union Cyc_YYSTYPE Cyc_YY5(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6EA;(_tmp6EA.YY5).tag=13U,(_tmp6EA.YY5).val=yy1;_tmp6EA;});}static char _tmp1B1[9U]="primop_t";
# 1134 "parse.y"
static enum Cyc_Absyn_Primop Cyc_yyget_YY6(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1B1,_tmp1B1,_tmp1B1 + 9U};
union Cyc_YYSTYPE*_tmp1AF=yy1;enum Cyc_Absyn_Primop _tmp1B0;if((((union Cyc_YYSTYPE*)_tmp1AF)->YY6).tag == 14){_LL1: _tmp1B0=(_tmp1AF->YY6).val;_LL2: {enum Cyc_Absyn_Primop yy=_tmp1B0;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1141
static union Cyc_YYSTYPE Cyc_YY6(enum Cyc_Absyn_Primop yy1){return({union Cyc_YYSTYPE _tmp6EB;(_tmp6EB.YY6).tag=14U,(_tmp6EB.YY6).val=yy1;_tmp6EB;});}static char _tmp1B4[19U]="opt_t<primop_t,`H>";
# 1135 "parse.y"
static struct Cyc_Core_Opt*Cyc_yyget_YY7(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1B4,_tmp1B4,_tmp1B4 + 19U};
union Cyc_YYSTYPE*_tmp1B2=yy1;struct Cyc_Core_Opt*_tmp1B3;if((((union Cyc_YYSTYPE*)_tmp1B2)->YY7).tag == 15){_LL1: _tmp1B3=(_tmp1B2->YY7).val;_LL2: {struct Cyc_Core_Opt*yy=_tmp1B3;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1142
static union Cyc_YYSTYPE Cyc_YY7(struct Cyc_Core_Opt*yy1){return({union Cyc_YYSTYPE _tmp6EC;(_tmp6EC.YY7).tag=15U,(_tmp6EC.YY7).val=yy1;_tmp6EC;});}static char _tmp1B7[7U]="qvar_t";
# 1136 "parse.y"
static struct _tuple0*Cyc_yyget_QualId_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1B7,_tmp1B7,_tmp1B7 + 7U};
union Cyc_YYSTYPE*_tmp1B5=yy1;struct _tuple0*_tmp1B6;if((((union Cyc_YYSTYPE*)_tmp1B5)->QualId_tok).tag == 5){_LL1: _tmp1B6=(_tmp1B5->QualId_tok).val;_LL2: {struct _tuple0*yy=_tmp1B6;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1143
static union Cyc_YYSTYPE Cyc_QualId_tok(struct _tuple0*yy1){return({union Cyc_YYSTYPE _tmp6ED;(_tmp6ED.QualId_tok).tag=5U,(_tmp6ED.QualId_tok).val=yy1;_tmp6ED;});}static char _tmp1BA[7U]="stmt_t";
# 1139 "parse.y"
static struct Cyc_Absyn_Stmt*Cyc_yyget_Stmt_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1BA,_tmp1BA,_tmp1BA + 7U};
union Cyc_YYSTYPE*_tmp1B8=yy1;struct Cyc_Absyn_Stmt*_tmp1B9;if((((union Cyc_YYSTYPE*)_tmp1B8)->Stmt_tok).tag == 8){_LL1: _tmp1B9=(_tmp1B8->Stmt_tok).val;_LL2: {struct Cyc_Absyn_Stmt*yy=_tmp1B9;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1146
static union Cyc_YYSTYPE Cyc_Stmt_tok(struct Cyc_Absyn_Stmt*yy1){return({union Cyc_YYSTYPE _tmp6EE;(_tmp6EE.Stmt_tok).tag=8U,(_tmp6EE.Stmt_tok).val=yy1;_tmp6EE;});}static char _tmp1BD[27U]="list_t<switch_clause_t,`H>";
# 1143 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY8(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1BD,_tmp1BD,_tmp1BD + 27U};
union Cyc_YYSTYPE*_tmp1BB=yy1;struct Cyc_List_List*_tmp1BC;if((((union Cyc_YYSTYPE*)_tmp1BB)->YY8).tag == 16){_LL1: _tmp1BC=(_tmp1BB->YY8).val;_LL2: {struct Cyc_List_List*yy=_tmp1BC;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1150
static union Cyc_YYSTYPE Cyc_YY8(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6EF;(_tmp6EF.YY8).tag=16U,(_tmp6EF.YY8).val=yy1;_tmp6EF;});}static char _tmp1C0[6U]="pat_t";
# 1144 "parse.y"
static struct Cyc_Absyn_Pat*Cyc_yyget_YY9(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1C0,_tmp1C0,_tmp1C0 + 6U};
union Cyc_YYSTYPE*_tmp1BE=yy1;struct Cyc_Absyn_Pat*_tmp1BF;if((((union Cyc_YYSTYPE*)_tmp1BE)->YY9).tag == 17){_LL1: _tmp1BF=(_tmp1BE->YY9).val;_LL2: {struct Cyc_Absyn_Pat*yy=_tmp1BF;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1151
static union Cyc_YYSTYPE Cyc_YY9(struct Cyc_Absyn_Pat*yy1){return({union Cyc_YYSTYPE _tmp6F0;(_tmp6F0.YY9).tag=17U,(_tmp6F0.YY9).val=yy1;_tmp6F0;});}static char _tmp1C3[28U]="$(list_t<pat_t,`H>,bool)@`H";
# 1149 "parse.y"
static struct _tuple23*Cyc_yyget_YY10(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1C3,_tmp1C3,_tmp1C3 + 28U};
union Cyc_YYSTYPE*_tmp1C1=yy1;struct _tuple23*_tmp1C2;if((((union Cyc_YYSTYPE*)_tmp1C1)->YY10).tag == 18){_LL1: _tmp1C2=(_tmp1C1->YY10).val;_LL2: {struct _tuple23*yy=_tmp1C2;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1156
static union Cyc_YYSTYPE Cyc_YY10(struct _tuple23*yy1){return({union Cyc_YYSTYPE _tmp6F1;(_tmp6F1.YY10).tag=18U,(_tmp6F1.YY10).val=yy1;_tmp6F1;});}static char _tmp1C6[17U]="list_t<pat_t,`H>";
# 1150 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY11(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1C6,_tmp1C6,_tmp1C6 + 17U};
union Cyc_YYSTYPE*_tmp1C4=yy1;struct Cyc_List_List*_tmp1C5;if((((union Cyc_YYSTYPE*)_tmp1C4)->YY11).tag == 19){_LL1: _tmp1C5=(_tmp1C4->YY11).val;_LL2: {struct Cyc_List_List*yy=_tmp1C5;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1157
static union Cyc_YYSTYPE Cyc_YY11(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6F2;(_tmp6F2.YY11).tag=19U,(_tmp6F2.YY11).val=yy1;_tmp6F2;});}static char _tmp1C9[36U]="$(list_t<designator_t,`H>,pat_t)@`H";
# 1151 "parse.y"
static struct _tuple24*Cyc_yyget_YY12(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1C9,_tmp1C9,_tmp1C9 + 36U};
union Cyc_YYSTYPE*_tmp1C7=yy1;struct _tuple24*_tmp1C8;if((((union Cyc_YYSTYPE*)_tmp1C7)->YY12).tag == 20){_LL1: _tmp1C8=(_tmp1C7->YY12).val;_LL2: {struct _tuple24*yy=_tmp1C8;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1158
static union Cyc_YYSTYPE Cyc_YY12(struct _tuple24*yy1){return({union Cyc_YYSTYPE _tmp6F3;(_tmp6F3.YY12).tag=20U,(_tmp6F3.YY12).val=yy1;_tmp6F3;});}static char _tmp1CC[47U]="list_t<$(list_t<designator_t,`H>,pat_t)@`H,`H>";
# 1152 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY13(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1CC,_tmp1CC,_tmp1CC + 47U};
union Cyc_YYSTYPE*_tmp1CA=yy1;struct Cyc_List_List*_tmp1CB;if((((union Cyc_YYSTYPE*)_tmp1CA)->YY13).tag == 21){_LL1: _tmp1CB=(_tmp1CA->YY13).val;_LL2: {struct Cyc_List_List*yy=_tmp1CB;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1159
static union Cyc_YYSTYPE Cyc_YY13(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6F4;(_tmp6F4.YY13).tag=21U,(_tmp6F4.YY13).val=yy1;_tmp6F4;});}static char _tmp1CF[58U]="$(list_t<$(list_t<designator_t,`H>,pat_t)@`H,`H>,bool)@`H";
# 1153 "parse.y"
static struct _tuple23*Cyc_yyget_YY14(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1CF,_tmp1CF,_tmp1CF + 58U};
union Cyc_YYSTYPE*_tmp1CD=yy1;struct _tuple23*_tmp1CE;if((((union Cyc_YYSTYPE*)_tmp1CD)->YY14).tag == 22){_LL1: _tmp1CE=(_tmp1CD->YY14).val;_LL2: {struct _tuple23*yy=_tmp1CE;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1160
static union Cyc_YYSTYPE Cyc_YY14(struct _tuple23*yy1){return({union Cyc_YYSTYPE _tmp6F5;(_tmp6F5.YY14).tag=22U,(_tmp6F5.YY14).val=yy1;_tmp6F5;});}static char _tmp1D2[9U]="fndecl_t";
# 1154 "parse.y"
static struct Cyc_Absyn_Fndecl*Cyc_yyget_YY15(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1D2,_tmp1D2,_tmp1D2 + 9U};
union Cyc_YYSTYPE*_tmp1D0=yy1;struct Cyc_Absyn_Fndecl*_tmp1D1;if((((union Cyc_YYSTYPE*)_tmp1D0)->YY15).tag == 23){_LL1: _tmp1D1=(_tmp1D0->YY15).val;_LL2: {struct Cyc_Absyn_Fndecl*yy=_tmp1D1;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1161
static union Cyc_YYSTYPE Cyc_YY15(struct Cyc_Absyn_Fndecl*yy1){return({union Cyc_YYSTYPE _tmp6F6;(_tmp6F6.YY15).tag=23U,(_tmp6F6.YY15).val=yy1;_tmp6F6;});}static char _tmp1D5[18U]="list_t<decl_t,`H>";
# 1155 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY16(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1D5,_tmp1D5,_tmp1D5 + 18U};
union Cyc_YYSTYPE*_tmp1D3=yy1;struct Cyc_List_List*_tmp1D4;if((((union Cyc_YYSTYPE*)_tmp1D3)->YY16).tag == 24){_LL1: _tmp1D4=(_tmp1D3->YY16).val;_LL2: {struct Cyc_List_List*yy=_tmp1D4;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1162
static union Cyc_YYSTYPE Cyc_YY16(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6F7;(_tmp6F7.YY16).tag=24U,(_tmp6F7.YY16).val=yy1;_tmp6F7;});}static char _tmp1D8[12U]="decl_spec_t";
# 1158 "parse.y"
static struct Cyc_Parse_Declaration_spec Cyc_yyget_YY17(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1D8,_tmp1D8,_tmp1D8 + 12U};
union Cyc_YYSTYPE*_tmp1D6=yy1;struct Cyc_Parse_Declaration_spec _tmp1D7;if((((union Cyc_YYSTYPE*)_tmp1D6)->YY17).tag == 25){_LL1: _tmp1D7=(_tmp1D6->YY17).val;_LL2: {struct Cyc_Parse_Declaration_spec yy=_tmp1D7;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1165
static union Cyc_YYSTYPE Cyc_YY17(struct Cyc_Parse_Declaration_spec yy1){return({union Cyc_YYSTYPE _tmp6F8;(_tmp6F8.YY17).tag=25U,(_tmp6F8.YY17).val=yy1;_tmp6F8;});}static char _tmp1DB[31U]="$(declarator_t<`yy>,exp_opt_t)";
# 1159 "parse.y"
static struct _tuple12 Cyc_yyget_YY18(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1DB,_tmp1DB,_tmp1DB + 31U};
union Cyc_YYSTYPE*_tmp1D9=yy1;struct _tuple12 _tmp1DA;if((((union Cyc_YYSTYPE*)_tmp1D9)->YY18).tag == 26){_LL1: _tmp1DA=(_tmp1D9->YY18).val;_LL2: {struct _tuple12 yy=_tmp1DA;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1166
static union Cyc_YYSTYPE Cyc_YY18(struct _tuple12 yy1){return({union Cyc_YYSTYPE _tmp6F9;(_tmp6F9.YY18).tag=26U,(_tmp6F9.YY18).val=yy1;_tmp6F9;});}static char _tmp1DE[23U]="declarator_list_t<`yy>";
# 1160 "parse.y"
static struct _tuple13*Cyc_yyget_YY19(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1DE,_tmp1DE,_tmp1DE + 23U};
union Cyc_YYSTYPE*_tmp1DC=yy1;struct _tuple13*_tmp1DD;if((((union Cyc_YYSTYPE*)_tmp1DC)->YY19).tag == 27){_LL1: _tmp1DD=(_tmp1DC->YY19).val;_LL2: {struct _tuple13*yy=_tmp1DD;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1167
static union Cyc_YYSTYPE Cyc_YY19(struct _tuple13*yy1){return({union Cyc_YYSTYPE _tmp6FA;(_tmp6FA.YY19).tag=27U,(_tmp6FA.YY19).val=yy1;_tmp6FA;});}static char _tmp1E1[19U]="storage_class_t@`H";
# 1161 "parse.y"
static enum Cyc_Parse_Storage_class*Cyc_yyget_YY20(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1E1,_tmp1E1,_tmp1E1 + 19U};
union Cyc_YYSTYPE*_tmp1DF=yy1;enum Cyc_Parse_Storage_class*_tmp1E0;if((((union Cyc_YYSTYPE*)_tmp1DF)->YY20).tag == 28){_LL1: _tmp1E0=(_tmp1DF->YY20).val;_LL2: {enum Cyc_Parse_Storage_class*yy=_tmp1E0;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1168
static union Cyc_YYSTYPE Cyc_YY20(enum Cyc_Parse_Storage_class*yy1){return({union Cyc_YYSTYPE _tmp6FB;(_tmp6FB.YY20).tag=28U,(_tmp6FB.YY20).val=yy1;_tmp6FB;});}static char _tmp1E4[17U]="type_specifier_t";
# 1162 "parse.y"
static struct Cyc_Parse_Type_specifier Cyc_yyget_YY21(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1E4,_tmp1E4,_tmp1E4 + 17U};
union Cyc_YYSTYPE*_tmp1E2=yy1;struct Cyc_Parse_Type_specifier _tmp1E3;if((((union Cyc_YYSTYPE*)_tmp1E2)->YY21).tag == 29){_LL1: _tmp1E3=(_tmp1E2->YY21).val;_LL2: {struct Cyc_Parse_Type_specifier yy=_tmp1E3;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1169
static union Cyc_YYSTYPE Cyc_YY21(struct Cyc_Parse_Type_specifier yy1){return({union Cyc_YYSTYPE _tmp6FC;(_tmp6FC.YY21).tag=29U,(_tmp6FC.YY21).val=yy1;_tmp6FC;});}static char _tmp1E7[12U]="aggr_kind_t";
# 1164 "parse.y"
static enum Cyc_Absyn_AggrKind Cyc_yyget_YY22(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1E7,_tmp1E7,_tmp1E7 + 12U};
union Cyc_YYSTYPE*_tmp1E5=yy1;enum Cyc_Absyn_AggrKind _tmp1E6;if((((union Cyc_YYSTYPE*)_tmp1E5)->YY22).tag == 30){_LL1: _tmp1E6=(_tmp1E5->YY22).val;_LL2: {enum Cyc_Absyn_AggrKind yy=_tmp1E6;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1171
static union Cyc_YYSTYPE Cyc_YY22(enum Cyc_Absyn_AggrKind yy1){return({union Cyc_YYSTYPE _tmp6FD;(_tmp6FD.YY22).tag=30U,(_tmp6FD.YY22).val=yy1;_tmp6FD;});}static char _tmp1EA[8U]="tqual_t";
# 1165 "parse.y"
static struct Cyc_Absyn_Tqual Cyc_yyget_YY23(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1EA,_tmp1EA,_tmp1EA + 8U};
union Cyc_YYSTYPE*_tmp1E8=yy1;struct Cyc_Absyn_Tqual _tmp1E9;if((((union Cyc_YYSTYPE*)_tmp1E8)->YY23).tag == 31){_LL1: _tmp1E9=(_tmp1E8->YY23).val;_LL2: {struct Cyc_Absyn_Tqual yy=_tmp1E9;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1172
static union Cyc_YYSTYPE Cyc_YY23(struct Cyc_Absyn_Tqual yy1){return({union Cyc_YYSTYPE _tmp6FE;(_tmp6FE.YY23).tag=31U,(_tmp6FE.YY23).val=yy1;_tmp6FE;});}static char _tmp1ED[23U]="list_t<aggrfield_t,`H>";
# 1166 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY24(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1ED,_tmp1ED,_tmp1ED + 23U};
union Cyc_YYSTYPE*_tmp1EB=yy1;struct Cyc_List_List*_tmp1EC;if((((union Cyc_YYSTYPE*)_tmp1EB)->YY24).tag == 32){_LL1: _tmp1EC=(_tmp1EB->YY24).val;_LL2: {struct Cyc_List_List*yy=_tmp1EC;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1173
static union Cyc_YYSTYPE Cyc_YY24(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp6FF;(_tmp6FF.YY24).tag=32U,(_tmp6FF.YY24).val=yy1;_tmp6FF;});}static char _tmp1F0[34U]="list_t<list_t<aggrfield_t,`H>,`H>";
# 1167 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY25(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1F0,_tmp1F0,_tmp1F0 + 34U};
union Cyc_YYSTYPE*_tmp1EE=yy1;struct Cyc_List_List*_tmp1EF;if((((union Cyc_YYSTYPE*)_tmp1EE)->YY25).tag == 33){_LL1: _tmp1EF=(_tmp1EE->YY25).val;_LL2: {struct Cyc_List_List*yy=_tmp1EF;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1174
static union Cyc_YYSTYPE Cyc_YY25(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp700;(_tmp700.YY25).tag=33U,(_tmp700.YY25).val=yy1;_tmp700;});}static char _tmp1F3[33U]="list_t<type_modifier_t<`yy>,`yy>";
# 1168 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY26(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1F3,_tmp1F3,_tmp1F3 + 33U};
union Cyc_YYSTYPE*_tmp1F1=yy1;struct Cyc_List_List*_tmp1F2;if((((union Cyc_YYSTYPE*)_tmp1F1)->YY26).tag == 34){_LL1: _tmp1F2=(_tmp1F1->YY26).val;_LL2: {struct Cyc_List_List*yy=_tmp1F2;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1175
static union Cyc_YYSTYPE Cyc_YY26(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp701;(_tmp701.YY26).tag=34U,(_tmp701.YY26).val=yy1;_tmp701;});}static char _tmp1F6[18U]="declarator_t<`yy>";
# 1169 "parse.y"
static struct Cyc_Parse_Declarator Cyc_yyget_YY27(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1F6,_tmp1F6,_tmp1F6 + 18U};
union Cyc_YYSTYPE*_tmp1F4=yy1;struct Cyc_Parse_Declarator _tmp1F5;if((((union Cyc_YYSTYPE*)_tmp1F4)->YY27).tag == 35){_LL1: _tmp1F5=(_tmp1F4->YY27).val;_LL2: {struct Cyc_Parse_Declarator yy=_tmp1F5;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1176
static union Cyc_YYSTYPE Cyc_YY27(struct Cyc_Parse_Declarator yy1){return({union Cyc_YYSTYPE _tmp702;(_tmp702.YY27).tag=35U,(_tmp702.YY27).val=yy1;_tmp702;});}static char _tmp1F9[45U]="$(declarator_t<`yy>,exp_opt_t,exp_opt_t)@`yy";
# 1170 "parse.y"
static struct _tuple25*Cyc_yyget_YY28(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1F9,_tmp1F9,_tmp1F9 + 45U};
union Cyc_YYSTYPE*_tmp1F7=yy1;struct _tuple25*_tmp1F8;if((((union Cyc_YYSTYPE*)_tmp1F7)->YY28).tag == 36){_LL1: _tmp1F8=(_tmp1F7->YY28).val;_LL2: {struct _tuple25*yy=_tmp1F8;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1177
static union Cyc_YYSTYPE Cyc_YY28(struct _tuple25*yy1){return({union Cyc_YYSTYPE _tmp703;(_tmp703.YY28).tag=36U,(_tmp703.YY28).val=yy1;_tmp703;});}static char _tmp1FC[57U]="list_t<$(declarator_t<`yy>,exp_opt_t,exp_opt_t)@`yy,`yy>";
# 1171 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY29(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1FC,_tmp1FC,_tmp1FC + 57U};
union Cyc_YYSTYPE*_tmp1FA=yy1;struct Cyc_List_List*_tmp1FB;if((((union Cyc_YYSTYPE*)_tmp1FA)->YY29).tag == 37){_LL1: _tmp1FB=(_tmp1FA->YY29).val;_LL2: {struct Cyc_List_List*yy=_tmp1FB;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1178
static union Cyc_YYSTYPE Cyc_YY29(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp704;(_tmp704.YY29).tag=37U,(_tmp704.YY29).val=yy1;_tmp704;});}static char _tmp1FF[26U]="abstractdeclarator_t<`yy>";
# 1172 "parse.y"
static struct Cyc_Parse_Abstractdeclarator Cyc_yyget_YY30(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1FF,_tmp1FF,_tmp1FF + 26U};
union Cyc_YYSTYPE*_tmp1FD=yy1;struct Cyc_Parse_Abstractdeclarator _tmp1FE;if((((union Cyc_YYSTYPE*)_tmp1FD)->YY30).tag == 38){_LL1: _tmp1FE=(_tmp1FD->YY30).val;_LL2: {struct Cyc_Parse_Abstractdeclarator yy=_tmp1FE;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1179
static union Cyc_YYSTYPE Cyc_YY30(struct Cyc_Parse_Abstractdeclarator yy1){return({union Cyc_YYSTYPE _tmp705;(_tmp705.YY30).tag=38U,(_tmp705.YY30).val=yy1;_tmp705;});}static char _tmp202[5U]="bool";
# 1173 "parse.y"
static int Cyc_yyget_YY31(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp202,_tmp202,_tmp202 + 5U};
union Cyc_YYSTYPE*_tmp200=yy1;int _tmp201;if((((union Cyc_YYSTYPE*)_tmp200)->YY31).tag == 39){_LL1: _tmp201=(_tmp200->YY31).val;_LL2: {int yy=_tmp201;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1180
static union Cyc_YYSTYPE Cyc_YY31(int yy1){return({union Cyc_YYSTYPE _tmp706;(_tmp706.YY31).tag=39U,(_tmp706.YY31).val=yy1;_tmp706;});}static char _tmp205[8U]="scope_t";
# 1174 "parse.y"
static enum Cyc_Absyn_Scope Cyc_yyget_YY32(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp205,_tmp205,_tmp205 + 8U};
union Cyc_YYSTYPE*_tmp203=yy1;enum Cyc_Absyn_Scope _tmp204;if((((union Cyc_YYSTYPE*)_tmp203)->YY32).tag == 40){_LL1: _tmp204=(_tmp203->YY32).val;_LL2: {enum Cyc_Absyn_Scope yy=_tmp204;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1181
static union Cyc_YYSTYPE Cyc_YY32(enum Cyc_Absyn_Scope yy1){return({union Cyc_YYSTYPE _tmp707;(_tmp707.YY32).tag=40U,(_tmp707.YY32).val=yy1;_tmp707;});}static char _tmp208[16U]="datatypefield_t";
# 1175 "parse.y"
static struct Cyc_Absyn_Datatypefield*Cyc_yyget_YY33(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp208,_tmp208,_tmp208 + 16U};
union Cyc_YYSTYPE*_tmp206=yy1;struct Cyc_Absyn_Datatypefield*_tmp207;if((((union Cyc_YYSTYPE*)_tmp206)->YY33).tag == 41){_LL1: _tmp207=(_tmp206->YY33).val;_LL2: {struct Cyc_Absyn_Datatypefield*yy=_tmp207;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1182
static union Cyc_YYSTYPE Cyc_YY33(struct Cyc_Absyn_Datatypefield*yy1){return({union Cyc_YYSTYPE _tmp708;(_tmp708.YY33).tag=41U,(_tmp708.YY33).val=yy1;_tmp708;});}static char _tmp20B[27U]="list_t<datatypefield_t,`H>";
# 1176 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY34(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp20B,_tmp20B,_tmp20B + 27U};
union Cyc_YYSTYPE*_tmp209=yy1;struct Cyc_List_List*_tmp20A;if((((union Cyc_YYSTYPE*)_tmp209)->YY34).tag == 42){_LL1: _tmp20A=(_tmp209->YY34).val;_LL2: {struct Cyc_List_List*yy=_tmp20A;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1183
static union Cyc_YYSTYPE Cyc_YY34(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp709;(_tmp709.YY34).tag=42U,(_tmp709.YY34).val=yy1;_tmp709;});}static char _tmp20E[41U]="$(tqual_t,type_specifier_t,attributes_t)";
# 1177 "parse.y"
static struct _tuple26 Cyc_yyget_YY35(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp20E,_tmp20E,_tmp20E + 41U};
union Cyc_YYSTYPE*_tmp20C=yy1;struct _tuple26 _tmp20D;if((((union Cyc_YYSTYPE*)_tmp20C)->YY35).tag == 43){_LL1: _tmp20D=(_tmp20C->YY35).val;_LL2: {struct _tuple26 yy=_tmp20D;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1184
static union Cyc_YYSTYPE Cyc_YY35(struct _tuple26 yy1){return({union Cyc_YYSTYPE _tmp70A;(_tmp70A.YY35).tag=43U,(_tmp70A.YY35).val=yy1;_tmp70A;});}static char _tmp211[17U]="list_t<var_t,`H>";
# 1178 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY36(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp211,_tmp211,_tmp211 + 17U};
union Cyc_YYSTYPE*_tmp20F=yy1;struct Cyc_List_List*_tmp210;if((((union Cyc_YYSTYPE*)_tmp20F)->YY36).tag == 44){_LL1: _tmp210=(_tmp20F->YY36).val;_LL2: {struct Cyc_List_List*yy=_tmp210;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1185
static union Cyc_YYSTYPE Cyc_YY36(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp70B;(_tmp70B.YY36).tag=44U,(_tmp70B.YY36).val=yy1;_tmp70B;});}static char _tmp214[31U]="$(var_opt_t,tqual_t,type_t)@`H";
# 1179 "parse.y"
static struct _tuple8*Cyc_yyget_YY37(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp214,_tmp214,_tmp214 + 31U};
union Cyc_YYSTYPE*_tmp212=yy1;struct _tuple8*_tmp213;if((((union Cyc_YYSTYPE*)_tmp212)->YY37).tag == 45){_LL1: _tmp213=(_tmp212->YY37).val;_LL2: {struct _tuple8*yy=_tmp213;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1186
static union Cyc_YYSTYPE Cyc_YY37(struct _tuple8*yy1){return({union Cyc_YYSTYPE _tmp70C;(_tmp70C.YY37).tag=45U,(_tmp70C.YY37).val=yy1;_tmp70C;});}static char _tmp217[42U]="list_t<$(var_opt_t,tqual_t,type_t)@`H,`H>";
# 1180 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY38(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp217,_tmp217,_tmp217 + 42U};
union Cyc_YYSTYPE*_tmp215=yy1;struct Cyc_List_List*_tmp216;if((((union Cyc_YYSTYPE*)_tmp215)->YY38).tag == 46){_LL1: _tmp216=(_tmp215->YY38).val;_LL2: {struct Cyc_List_List*yy=_tmp216;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1187
static union Cyc_YYSTYPE Cyc_YY38(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp70D;(_tmp70D.YY38).tag=46U,(_tmp70D.YY38).val=yy1;_tmp70D;});}static char _tmp21A[115U]="$(list_t<$(var_opt_t,tqual_t,type_t)@`H,`H>, bool,vararg_info_t *`H,type_opt_t, list_t<$(type_t,type_t)@`H,`H>)@`H";
# 1181 "parse.y"
static struct _tuple27*Cyc_yyget_YY39(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp21A,_tmp21A,_tmp21A + 115U};
union Cyc_YYSTYPE*_tmp218=yy1;struct _tuple27*_tmp219;if((((union Cyc_YYSTYPE*)_tmp218)->YY39).tag == 47){_LL1: _tmp219=(_tmp218->YY39).val;_LL2: {struct _tuple27*yy=_tmp219;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1188
static union Cyc_YYSTYPE Cyc_YY39(struct _tuple27*yy1){return({union Cyc_YYSTYPE _tmp70E;(_tmp70E.YY39).tag=47U,(_tmp70E.YY39).val=yy1;_tmp70E;});}static char _tmp21D[8U]="types_t";
# 1182 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY40(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp21D,_tmp21D,_tmp21D + 8U};
union Cyc_YYSTYPE*_tmp21B=yy1;struct Cyc_List_List*_tmp21C;if((((union Cyc_YYSTYPE*)_tmp21B)->YY40).tag == 48){_LL1: _tmp21C=(_tmp21B->YY40).val;_LL2: {struct Cyc_List_List*yy=_tmp21C;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1189
static union Cyc_YYSTYPE Cyc_YY40(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp70F;(_tmp70F.YY40).tag=48U,(_tmp70F.YY40).val=yy1;_tmp70F;});}static char _tmp220[24U]="list_t<designator_t,`H>";
# 1184 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY41(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp220,_tmp220,_tmp220 + 24U};
union Cyc_YYSTYPE*_tmp21E=yy1;struct Cyc_List_List*_tmp21F;if((((union Cyc_YYSTYPE*)_tmp21E)->YY41).tag == 49){_LL1: _tmp21F=(_tmp21E->YY41).val;_LL2: {struct Cyc_List_List*yy=_tmp21F;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1191
static union Cyc_YYSTYPE Cyc_YY41(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp710;(_tmp710.YY41).tag=49U,(_tmp710.YY41).val=yy1;_tmp710;});}static char _tmp223[13U]="designator_t";
# 1185 "parse.y"
static void*Cyc_yyget_YY42(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp223,_tmp223,_tmp223 + 13U};
union Cyc_YYSTYPE*_tmp221=yy1;void*_tmp222;if((((union Cyc_YYSTYPE*)_tmp221)->YY42).tag == 50){_LL1: _tmp222=(_tmp221->YY42).val;_LL2: {void*yy=_tmp222;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1192
static union Cyc_YYSTYPE Cyc_YY42(void*yy1){return({union Cyc_YYSTYPE _tmp711;(_tmp711.YY42).tag=50U,(_tmp711.YY42).val=yy1;_tmp711;});}static char _tmp226[7U]="kind_t";
# 1186 "parse.y"
static struct Cyc_Absyn_Kind*Cyc_yyget_YY43(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp226,_tmp226,_tmp226 + 7U};
union Cyc_YYSTYPE*_tmp224=yy1;struct Cyc_Absyn_Kind*_tmp225;if((((union Cyc_YYSTYPE*)_tmp224)->YY43).tag == 51){_LL1: _tmp225=(_tmp224->YY43).val;_LL2: {struct Cyc_Absyn_Kind*yy=_tmp225;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1193
static union Cyc_YYSTYPE Cyc_YY43(struct Cyc_Absyn_Kind*yy1){return({union Cyc_YYSTYPE _tmp712;(_tmp712.YY43).tag=51U,(_tmp712.YY43).val=yy1;_tmp712;});}static char _tmp229[7U]="type_t";
# 1187 "parse.y"
static void*Cyc_yyget_YY44(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp229,_tmp229,_tmp229 + 7U};
union Cyc_YYSTYPE*_tmp227=yy1;void*_tmp228;if((((union Cyc_YYSTYPE*)_tmp227)->YY44).tag == 52){_LL1: _tmp228=(_tmp227->YY44).val;_LL2: {void*yy=_tmp228;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1194
static union Cyc_YYSTYPE Cyc_YY44(void*yy1){return({union Cyc_YYSTYPE _tmp713;(_tmp713.YY44).tag=52U,(_tmp713.YY44).val=yy1;_tmp713;});}static char _tmp22C[23U]="list_t<attribute_t,`H>";
# 1188 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY45(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp22C,_tmp22C,_tmp22C + 23U};
union Cyc_YYSTYPE*_tmp22A=yy1;struct Cyc_List_List*_tmp22B;if((((union Cyc_YYSTYPE*)_tmp22A)->YY45).tag == 53){_LL1: _tmp22B=(_tmp22A->YY45).val;_LL2: {struct Cyc_List_List*yy=_tmp22B;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1195
static union Cyc_YYSTYPE Cyc_YY45(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp714;(_tmp714.YY45).tag=53U,(_tmp714.YY45).val=yy1;_tmp714;});}static char _tmp22F[12U]="attribute_t";
# 1189 "parse.y"
static void*Cyc_yyget_YY46(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp22F,_tmp22F,_tmp22F + 12U};
union Cyc_YYSTYPE*_tmp22D=yy1;void*_tmp22E;if((((union Cyc_YYSTYPE*)_tmp22D)->YY46).tag == 54){_LL1: _tmp22E=(_tmp22D->YY46).val;_LL2: {void*yy=_tmp22E;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1196
static union Cyc_YYSTYPE Cyc_YY46(void*yy1){return({union Cyc_YYSTYPE _tmp715;(_tmp715.YY46).tag=54U,(_tmp715.YY46).val=yy1;_tmp715;});}static char _tmp232[12U]="enumfield_t";
# 1190 "parse.y"
static struct Cyc_Absyn_Enumfield*Cyc_yyget_YY47(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp232,_tmp232,_tmp232 + 12U};
union Cyc_YYSTYPE*_tmp230=yy1;struct Cyc_Absyn_Enumfield*_tmp231;if((((union Cyc_YYSTYPE*)_tmp230)->YY47).tag == 55){_LL1: _tmp231=(_tmp230->YY47).val;_LL2: {struct Cyc_Absyn_Enumfield*yy=_tmp231;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1197
static union Cyc_YYSTYPE Cyc_YY47(struct Cyc_Absyn_Enumfield*yy1){return({union Cyc_YYSTYPE _tmp716;(_tmp716.YY47).tag=55U,(_tmp716.YY47).val=yy1;_tmp716;});}static char _tmp235[23U]="list_t<enumfield_t,`H>";
# 1191 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY48(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp235,_tmp235,_tmp235 + 23U};
union Cyc_YYSTYPE*_tmp233=yy1;struct Cyc_List_List*_tmp234;if((((union Cyc_YYSTYPE*)_tmp233)->YY48).tag == 56){_LL1: _tmp234=(_tmp233->YY48).val;_LL2: {struct Cyc_List_List*yy=_tmp234;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1198
static union Cyc_YYSTYPE Cyc_YY48(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp717;(_tmp717.YY48).tag=56U,(_tmp717.YY48).val=yy1;_tmp717;});}static char _tmp238[11U]="type_opt_t";
# 1192 "parse.y"
static void*Cyc_yyget_YY49(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp238,_tmp238,_tmp238 + 11U};
union Cyc_YYSTYPE*_tmp236=yy1;void*_tmp237;if((((union Cyc_YYSTYPE*)_tmp236)->YY49).tag == 57){_LL1: _tmp237=(_tmp236->YY49).val;_LL2: {void*yy=_tmp237;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1199
static union Cyc_YYSTYPE Cyc_YY49(void*yy1){return({union Cyc_YYSTYPE _tmp718;(_tmp718.YY49).tag=57U,(_tmp718.YY49).val=yy1;_tmp718;});}static char _tmp23B[31U]="list_t<$(type_t,type_t)@`H,`H>";
# 1193 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY50(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp23B,_tmp23B,_tmp23B + 31U};
union Cyc_YYSTYPE*_tmp239=yy1;struct Cyc_List_List*_tmp23A;if((((union Cyc_YYSTYPE*)_tmp239)->YY50).tag == 58){_LL1: _tmp23A=(_tmp239->YY50).val;_LL2: {struct Cyc_List_List*yy=_tmp23A;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1200
static union Cyc_YYSTYPE Cyc_YY50(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp719;(_tmp719.YY50).tag=58U,(_tmp719.YY50).val=yy1;_tmp719;});}static char _tmp23E[11U]="booltype_t";
# 1194 "parse.y"
static void*Cyc_yyget_YY51(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp23E,_tmp23E,_tmp23E + 11U};
union Cyc_YYSTYPE*_tmp23C=yy1;void*_tmp23D;if((((union Cyc_YYSTYPE*)_tmp23C)->YY51).tag == 59){_LL1: _tmp23D=(_tmp23C->YY51).val;_LL2: {void*yy=_tmp23D;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1201
static union Cyc_YYSTYPE Cyc_YY51(void*yy1){return({union Cyc_YYSTYPE _tmp71A;(_tmp71A.YY51).tag=59U,(_tmp71A.YY51).val=yy1;_tmp71A;});}static char _tmp241[45U]="list_t<$(Position::seg_t,qvar_t,bool)@`H,`H>";
# 1195 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY52(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp241,_tmp241,_tmp241 + 45U};
union Cyc_YYSTYPE*_tmp23F=yy1;struct Cyc_List_List*_tmp240;if((((union Cyc_YYSTYPE*)_tmp23F)->YY52).tag == 60){_LL1: _tmp240=(_tmp23F->YY52).val;_LL2: {struct Cyc_List_List*yy=_tmp240;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1202
static union Cyc_YYSTYPE Cyc_YY52(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp71B;(_tmp71B.YY52).tag=60U,(_tmp71B.YY52).val=yy1;_tmp71B;});}static char _tmp244[58U]="$(list_t<$(Position::seg_t,qvar_t,bool)@`H,`H>, seg_t)@`H";
# 1196 "parse.y"
static struct _tuple28*Cyc_yyget_YY53(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp244,_tmp244,_tmp244 + 58U};
union Cyc_YYSTYPE*_tmp242=yy1;struct _tuple28*_tmp243;if((((union Cyc_YYSTYPE*)_tmp242)->YY53).tag == 61){_LL1: _tmp243=(_tmp242->YY53).val;_LL2: {struct _tuple28*yy=_tmp243;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1203
static union Cyc_YYSTYPE Cyc_YY53(struct _tuple28*yy1){return({union Cyc_YYSTYPE _tmp71C;(_tmp71C.YY53).tag=61U,(_tmp71C.YY53).val=yy1;_tmp71C;});}static char _tmp247[18U]="list_t<qvar_t,`H>";
# 1197 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY54(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp247,_tmp247,_tmp247 + 18U};
union Cyc_YYSTYPE*_tmp245=yy1;struct Cyc_List_List*_tmp246;if((((union Cyc_YYSTYPE*)_tmp245)->YY54).tag == 62){_LL1: _tmp246=(_tmp245->YY54).val;_LL2: {struct Cyc_List_List*yy=_tmp246;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1204
static union Cyc_YYSTYPE Cyc_YY54(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp71D;(_tmp71D.YY54).tag=62U,(_tmp71D.YY54).val=yy1;_tmp71D;});}static char _tmp24A[20U]="pointer_qual_t<`yy>";
# 1198 "parse.y"
static void*Cyc_yyget_YY55(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp24A,_tmp24A,_tmp24A + 20U};
union Cyc_YYSTYPE*_tmp248=yy1;void*_tmp249;if((((union Cyc_YYSTYPE*)_tmp248)->YY55).tag == 63){_LL1: _tmp249=(_tmp248->YY55).val;_LL2: {void*yy=_tmp249;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1205
static union Cyc_YYSTYPE Cyc_YY55(void*yy1){return({union Cyc_YYSTYPE _tmp71E;(_tmp71E.YY55).tag=63U,(_tmp71E.YY55).val=yy1;_tmp71E;});}static char _tmp24D[21U]="pointer_quals_t<`yy>";
# 1199 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY56(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp24D,_tmp24D,_tmp24D + 21U};
union Cyc_YYSTYPE*_tmp24B=yy1;struct Cyc_List_List*_tmp24C;if((((union Cyc_YYSTYPE*)_tmp24B)->YY56).tag == 64){_LL1: _tmp24C=(_tmp24B->YY56).val;_LL2: {struct Cyc_List_List*yy=_tmp24C;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1206
static union Cyc_YYSTYPE Cyc_YY56(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp71F;(_tmp71F.YY56).tag=64U,(_tmp71F.YY56).val=yy1;_tmp71F;});}static char _tmp250[10U]="exp_opt_t";
# 1200 "parse.y"
static struct Cyc_Absyn_Exp*Cyc_yyget_YY57(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp250,_tmp250,_tmp250 + 10U};
union Cyc_YYSTYPE*_tmp24E=yy1;struct Cyc_Absyn_Exp*_tmp24F;if((((union Cyc_YYSTYPE*)_tmp24E)->YY57).tag == 65){_LL1: _tmp24F=(_tmp24E->YY57).val;_LL2: {struct Cyc_Absyn_Exp*yy=_tmp24F;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1207
static union Cyc_YYSTYPE Cyc_YY57(struct Cyc_Absyn_Exp*yy1){return({union Cyc_YYSTYPE _tmp720;(_tmp720.YY57).tag=65U,(_tmp720.YY57).val=yy1;_tmp720;});}static char _tmp253[10U]="raw_exp_t";
# 1201 "parse.y"
static void*Cyc_yyget_YY58(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp253,_tmp253,_tmp253 + 10U};
union Cyc_YYSTYPE*_tmp251=yy1;void*_tmp252;if((((union Cyc_YYSTYPE*)_tmp251)->YY58).tag == 66){_LL1: _tmp252=(_tmp251->YY58).val;_LL2: {void*yy=_tmp252;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1208
static union Cyc_YYSTYPE Cyc_YY58(void*yy1){return({union Cyc_YYSTYPE _tmp721;(_tmp721.YY58).tag=66U,(_tmp721.YY58).val=yy1;_tmp721;});}static char _tmp256[112U]="$(list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<string_t<`H>@`H, `H>)@`H";
# 1203 "parse.y"
static struct _tuple29*Cyc_yyget_YY59(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp256,_tmp256,_tmp256 + 112U};
union Cyc_YYSTYPE*_tmp254=yy1;struct _tuple29*_tmp255;if((((union Cyc_YYSTYPE*)_tmp254)->YY59).tag == 67){_LL1: _tmp255=(_tmp254->YY59).val;_LL2: {struct _tuple29*yy=_tmp255;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1210
static union Cyc_YYSTYPE Cyc_YY59(struct _tuple29*yy1){return({union Cyc_YYSTYPE _tmp722;(_tmp722.YY59).tag=67U,(_tmp722.YY59).val=yy1;_tmp722;});}static char _tmp259[73U]="$(list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<string_t<`H>@`H, `H>)@`H";
# 1204 "parse.y"
static struct _tuple30*Cyc_yyget_YY60(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp259,_tmp259,_tmp259 + 73U};
union Cyc_YYSTYPE*_tmp257=yy1;struct _tuple30*_tmp258;if((((union Cyc_YYSTYPE*)_tmp257)->YY60).tag == 68){_LL1: _tmp258=(_tmp257->YY60).val;_LL2: {struct _tuple30*yy=_tmp258;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1211
static union Cyc_YYSTYPE Cyc_YY60(struct _tuple30*yy1){return({union Cyc_YYSTYPE _tmp723;(_tmp723.YY60).tag=68U,(_tmp723.YY60).val=yy1;_tmp723;});}static char _tmp25C[28U]="list_t<string_t<`H>@`H, `H>";
# 1205 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY61(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp25C,_tmp25C,_tmp25C + 28U};
union Cyc_YYSTYPE*_tmp25A=yy1;struct Cyc_List_List*_tmp25B;if((((union Cyc_YYSTYPE*)_tmp25A)->YY61).tag == 69){_LL1: _tmp25B=(_tmp25A->YY61).val;_LL2: {struct Cyc_List_List*yy=_tmp25B;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1212
static union Cyc_YYSTYPE Cyc_YY61(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp724;(_tmp724.YY61).tag=69U,(_tmp724.YY61).val=yy1;_tmp724;});}static char _tmp25F[38U]="list_t<$(string_t<`H>, exp_t)@`H, `H>";
# 1206 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY62(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp25F,_tmp25F,_tmp25F + 38U};
union Cyc_YYSTYPE*_tmp25D=yy1;struct Cyc_List_List*_tmp25E;if((((union Cyc_YYSTYPE*)_tmp25D)->YY62).tag == 70){_LL1: _tmp25E=(_tmp25D->YY62).val;_LL2: {struct Cyc_List_List*yy=_tmp25E;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1213
static union Cyc_YYSTYPE Cyc_YY62(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmp725;(_tmp725.YY62).tag=70U,(_tmp725.YY62).val=yy1;_tmp725;});}static char _tmp262[26U]="$(string_t<`H>, exp_t)@`H";
# 1207 "parse.y"
static struct _tuple31*Cyc_yyget_YY63(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp262,_tmp262,_tmp262 + 26U};
union Cyc_YYSTYPE*_tmp260=yy1;struct _tuple31*_tmp261;if((((union Cyc_YYSTYPE*)_tmp260)->YY63).tag == 71){_LL1: _tmp261=(_tmp260->YY63).val;_LL2: {struct _tuple31*yy=_tmp261;
return yy;}}else{_LL3: _LL4:
 Cyc_yythrowfail(s);}_LL0:;}
# 1214
static union Cyc_YYSTYPE Cyc_YY63(struct _tuple31*yy1){return({union Cyc_YYSTYPE _tmp726;(_tmp726.YY63).tag=71U,(_tmp726.YY63).val=yy1;_tmp726;});}struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};
# 1230
struct Cyc_Yyltype Cyc_yynewloc (void){
return({struct Cyc_Yyltype _tmp727;_tmp727.timestamp=0,_tmp727.first_line=0,_tmp727.first_column=0,_tmp727.last_line=0,_tmp727.last_column=0;_tmp727;});}
# 1233
struct Cyc_Yyltype Cyc_yylloc={0,0,0,0,0};
# 1244 "parse.y"
static short Cyc_yytranslate[380U]={0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,151,2,2,136,149,146,2,133,134,129,143,128,147,138,148,2,2,2,2,2,2,2,2,2,2,137,125,131,130,132,142,141,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,139,2,140,145,135,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,126,144,127,150,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124};static char _tmp263[2U]="$";static char _tmp264[6U]="error";static char _tmp265[12U]="$undefined.";static char _tmp266[5U]="AUTO";static char _tmp267[9U]="REGISTER";static char _tmp268[7U]="STATIC";static char _tmp269[7U]="EXTERN";static char _tmp26A[8U]="TYPEDEF";static char _tmp26B[5U]="VOID";static char _tmp26C[5U]="CHAR";static char _tmp26D[6U]="SHORT";static char _tmp26E[4U]="INT";static char _tmp26F[5U]="LONG";static char _tmp270[6U]="FLOAT";static char _tmp271[7U]="DOUBLE";static char _tmp272[7U]="SIGNED";static char _tmp273[9U]="UNSIGNED";static char _tmp274[6U]="CONST";static char _tmp275[9U]="VOLATILE";static char _tmp276[9U]="RESTRICT";static char _tmp277[7U]="STRUCT";static char _tmp278[6U]="UNION";static char _tmp279[5U]="CASE";static char _tmp27A[8U]="DEFAULT";static char _tmp27B[7U]="INLINE";static char _tmp27C[7U]="SIZEOF";static char _tmp27D[9U]="OFFSETOF";static char _tmp27E[3U]="IF";static char _tmp27F[5U]="ELSE";static char _tmp280[7U]="SWITCH";static char _tmp281[6U]="WHILE";static char _tmp282[3U]="DO";static char _tmp283[4U]="FOR";static char _tmp284[5U]="GOTO";static char _tmp285[9U]="CONTINUE";static char _tmp286[6U]="BREAK";static char _tmp287[7U]="RETURN";static char _tmp288[5U]="ENUM";static char _tmp289[7U]="TYPEOF";static char _tmp28A[16U]="BUILTIN_VA_LIST";static char _tmp28B[10U]="EXTENSION";static char _tmp28C[8U]="NULL_kw";static char _tmp28D[4U]="LET";static char _tmp28E[6U]="THROW";static char _tmp28F[4U]="TRY";static char _tmp290[6U]="CATCH";static char _tmp291[7U]="EXPORT";static char _tmp292[9U]="OVERRIDE";static char _tmp293[5U]="HIDE";static char _tmp294[4U]="NEW";static char _tmp295[9U]="ABSTRACT";static char _tmp296[9U]="FALLTHRU";static char _tmp297[6U]="USING";static char _tmp298[10U]="NAMESPACE";static char _tmp299[9U]="DATATYPE";static char _tmp29A[7U]="MALLOC";static char _tmp29B[8U]="RMALLOC";static char _tmp29C[15U]="RMALLOC_INLINE";static char _tmp29D[7U]="CALLOC";static char _tmp29E[8U]="RCALLOC";static char _tmp29F[5U]="SWAP";static char _tmp2A0[9U]="REGION_T";static char _tmp2A1[6U]="TAG_T";static char _tmp2A2[7U]="REGION";static char _tmp2A3[5U]="RNEW";static char _tmp2A4[8U]="REGIONS";static char _tmp2A5[7U]="PORTON";static char _tmp2A6[8U]="PORTOFF";static char _tmp2A7[7U]="PRAGMA";static char _tmp2A8[10U]="TEMPESTON";static char _tmp2A9[11U]="TEMPESTOFF";static char _tmp2AA[8U]="NUMELTS";static char _tmp2AB[8U]="VALUEOF";static char _tmp2AC[10U]="VALUEOF_T";static char _tmp2AD[9U]="TAGCHECK";static char _tmp2AE[13U]="NUMELTS_QUAL";static char _tmp2AF[10U]="THIN_QUAL";static char _tmp2B0[9U]="FAT_QUAL";static char _tmp2B1[13U]="NOTNULL_QUAL";static char _tmp2B2[14U]="NULLABLE_QUAL";static char _tmp2B3[14U]="REQUIRES_QUAL";static char _tmp2B4[13U]="ENSURES_QUAL";static char _tmp2B5[12U]="REGION_QUAL";static char _tmp2B6[16U]="NOZEROTERM_QUAL";static char _tmp2B7[14U]="ZEROTERM_QUAL";static char _tmp2B8[12U]="TAGGED_QUAL";static char _tmp2B9[12U]="ASSERT_QUAL";static char _tmp2BA[16U]="EXTENSIBLE_QUAL";static char _tmp2BB[7U]="PTR_OP";static char _tmp2BC[7U]="INC_OP";static char _tmp2BD[7U]="DEC_OP";static char _tmp2BE[8U]="LEFT_OP";static char _tmp2BF[9U]="RIGHT_OP";static char _tmp2C0[6U]="LE_OP";static char _tmp2C1[6U]="GE_OP";static char _tmp2C2[6U]="EQ_OP";static char _tmp2C3[6U]="NE_OP";static char _tmp2C4[7U]="AND_OP";static char _tmp2C5[6U]="OR_OP";static char _tmp2C6[11U]="MUL_ASSIGN";static char _tmp2C7[11U]="DIV_ASSIGN";static char _tmp2C8[11U]="MOD_ASSIGN";static char _tmp2C9[11U]="ADD_ASSIGN";static char _tmp2CA[11U]="SUB_ASSIGN";static char _tmp2CB[12U]="LEFT_ASSIGN";static char _tmp2CC[13U]="RIGHT_ASSIGN";static char _tmp2CD[11U]="AND_ASSIGN";static char _tmp2CE[11U]="XOR_ASSIGN";static char _tmp2CF[10U]="OR_ASSIGN";static char _tmp2D0[9U]="ELLIPSIS";static char _tmp2D1[11U]="LEFT_RIGHT";static char _tmp2D2[12U]="COLON_COLON";static char _tmp2D3[11U]="IDENTIFIER";static char _tmp2D4[17U]="INTEGER_CONSTANT";static char _tmp2D5[7U]="STRING";static char _tmp2D6[8U]="WSTRING";static char _tmp2D7[19U]="CHARACTER_CONSTANT";static char _tmp2D8[20U]="WCHARACTER_CONSTANT";static char _tmp2D9[18U]="FLOATING_CONSTANT";static char _tmp2DA[9U]="TYPE_VAR";static char _tmp2DB[13U]="TYPEDEF_NAME";static char _tmp2DC[16U]="QUAL_IDENTIFIER";static char _tmp2DD[18U]="QUAL_TYPEDEF_NAME";static char _tmp2DE[10U]="ATTRIBUTE";static char _tmp2DF[8U]="ASM_TOK";static char _tmp2E0[4U]="';'";static char _tmp2E1[4U]="'{'";static char _tmp2E2[4U]="'}'";static char _tmp2E3[4U]="','";static char _tmp2E4[4U]="'*'";static char _tmp2E5[4U]="'='";static char _tmp2E6[4U]="'<'";static char _tmp2E7[4U]="'>'";static char _tmp2E8[4U]="'('";static char _tmp2E9[4U]="')'";static char _tmp2EA[4U]="'_'";static char _tmp2EB[4U]="'$'";static char _tmp2EC[4U]="':'";static char _tmp2ED[4U]="'.'";static char _tmp2EE[4U]="'['";static char _tmp2EF[4U]="']'";static char _tmp2F0[4U]="'@'";static char _tmp2F1[4U]="'?'";static char _tmp2F2[4U]="'+'";static char _tmp2F3[4U]="'|'";static char _tmp2F4[4U]="'^'";static char _tmp2F5[4U]="'&'";static char _tmp2F6[4U]="'-'";static char _tmp2F7[4U]="'/'";static char _tmp2F8[4U]="'%'";static char _tmp2F9[4U]="'~'";static char _tmp2FA[4U]="'!'";static char _tmp2FB[5U]="prog";static char _tmp2FC[17U]="translation_unit";static char _tmp2FD[18U]="tempest_on_action";static char _tmp2FE[19U]="tempest_off_action";static char _tmp2FF[16U]="extern_c_action";static char _tmp300[13U]="end_extern_c";static char _tmp301[14U]="hide_list_opt";static char _tmp302[17U]="hide_list_values";static char _tmp303[16U]="export_list_opt";static char _tmp304[12U]="export_list";static char _tmp305[19U]="export_list_values";static char _tmp306[13U]="override_opt";static char _tmp307[21U]="external_declaration";static char _tmp308[15U]="optional_comma";static char _tmp309[20U]="function_definition";static char _tmp30A[21U]="function_definition2";static char _tmp30B[13U]="using_action";static char _tmp30C[15U]="unusing_action";static char _tmp30D[17U]="namespace_action";static char _tmp30E[19U]="unnamespace_action";static char _tmp30F[12U]="declaration";static char _tmp310[17U]="declaration_list";static char _tmp311[23U]="declaration_specifiers";static char _tmp312[24U]="storage_class_specifier";static char _tmp313[15U]="attributes_opt";static char _tmp314[11U]="attributes";static char _tmp315[15U]="attribute_list";static char _tmp316[10U]="attribute";static char _tmp317[15U]="type_specifier";static char _tmp318[25U]="type_specifier_notypedef";static char _tmp319[5U]="kind";static char _tmp31A[15U]="type_qualifier";static char _tmp31B[15U]="enum_specifier";static char _tmp31C[11U]="enum_field";static char _tmp31D[22U]="enum_declaration_list";static char _tmp31E[26U]="struct_or_union_specifier";static char _tmp31F[16U]="type_params_opt";static char _tmp320[16U]="struct_or_union";static char _tmp321[24U]="struct_declaration_list";static char _tmp322[25U]="struct_declaration_list0";static char _tmp323[21U]="init_declarator_list";static char _tmp324[22U]="init_declarator_list0";static char _tmp325[16U]="init_declarator";static char _tmp326[19U]="struct_declaration";static char _tmp327[25U]="specifier_qualifier_list";static char _tmp328[35U]="notypedef_specifier_qualifier_list";static char _tmp329[23U]="struct_declarator_list";static char _tmp32A[24U]="struct_declarator_list0";static char _tmp32B[18U]="struct_declarator";static char _tmp32C[20U]="requires_clause_opt";static char _tmp32D[19U]="ensures_clause_opt";static char _tmp32E[19U]="datatype_specifier";static char _tmp32F[14U]="qual_datatype";static char _tmp330[19U]="datatypefield_list";static char _tmp331[20U]="datatypefield_scope";static char _tmp332[14U]="datatypefield";static char _tmp333[11U]="declarator";static char _tmp334[23U]="declarator_withtypedef";static char _tmp335[18U]="direct_declarator";static char _tmp336[30U]="direct_declarator_withtypedef";static char _tmp337[8U]="pointer";static char _tmp338[12U]="one_pointer";static char _tmp339[14U]="pointer_quals";static char _tmp33A[13U]="pointer_qual";static char _tmp33B[23U]="pointer_null_and_bound";static char _tmp33C[14U]="pointer_bound";static char _tmp33D[18U]="zeroterm_qual_opt";static char _tmp33E[8U]="rgn_opt";static char _tmp33F[11U]="tqual_list";static char _tmp340[20U]="parameter_type_list";static char _tmp341[9U]="type_var";static char _tmp342[16U]="optional_effect";static char _tmp343[19U]="optional_rgn_order";static char _tmp344[10U]="rgn_order";static char _tmp345[16U]="optional_inject";static char _tmp346[11U]="effect_set";static char _tmp347[14U]="atomic_effect";static char _tmp348[11U]="region_set";static char _tmp349[15U]="parameter_list";static char _tmp34A[22U]="parameter_declaration";static char _tmp34B[16U]="identifier_list";static char _tmp34C[17U]="identifier_list0";static char _tmp34D[12U]="initializer";static char _tmp34E[18U]="array_initializer";static char _tmp34F[17U]="initializer_list";static char _tmp350[12U]="designation";static char _tmp351[16U]="designator_list";static char _tmp352[11U]="designator";static char _tmp353[10U]="type_name";static char _tmp354[14U]="any_type_name";static char _tmp355[15U]="type_name_list";static char _tmp356[20U]="abstract_declarator";static char _tmp357[27U]="direct_abstract_declarator";static char _tmp358[10U]="statement";static char _tmp359[18U]="labeled_statement";static char _tmp35A[21U]="expression_statement";static char _tmp35B[19U]="compound_statement";static char _tmp35C[16U]="block_item_list";static char _tmp35D[20U]="selection_statement";static char _tmp35E[15U]="switch_clauses";static char _tmp35F[20U]="iteration_statement";static char _tmp360[15U]="jump_statement";static char _tmp361[12U]="exp_pattern";static char _tmp362[20U]="conditional_pattern";static char _tmp363[19U]="logical_or_pattern";static char _tmp364[20U]="logical_and_pattern";static char _tmp365[21U]="inclusive_or_pattern";static char _tmp366[21U]="exclusive_or_pattern";static char _tmp367[12U]="and_pattern";static char _tmp368[17U]="equality_pattern";static char _tmp369[19U]="relational_pattern";static char _tmp36A[14U]="shift_pattern";static char _tmp36B[17U]="additive_pattern";static char _tmp36C[23U]="multiplicative_pattern";static char _tmp36D[13U]="cast_pattern";static char _tmp36E[14U]="unary_pattern";static char _tmp36F[16U]="postfix_pattern";static char _tmp370[16U]="primary_pattern";static char _tmp371[8U]="pattern";static char _tmp372[19U]="tuple_pattern_list";static char _tmp373[20U]="tuple_pattern_list0";static char _tmp374[14U]="field_pattern";static char _tmp375[19U]="field_pattern_list";static char _tmp376[20U]="field_pattern_list0";static char _tmp377[11U]="expression";static char _tmp378[22U]="assignment_expression";static char _tmp379[20U]="assignment_operator";static char _tmp37A[23U]="conditional_expression";static char _tmp37B[20U]="constant_expression";static char _tmp37C[22U]="logical_or_expression";static char _tmp37D[23U]="logical_and_expression";static char _tmp37E[24U]="inclusive_or_expression";static char _tmp37F[24U]="exclusive_or_expression";static char _tmp380[15U]="and_expression";static char _tmp381[20U]="equality_expression";static char _tmp382[22U]="relational_expression";static char _tmp383[17U]="shift_expression";static char _tmp384[20U]="additive_expression";static char _tmp385[26U]="multiplicative_expression";static char _tmp386[16U]="cast_expression";static char _tmp387[17U]="unary_expression";static char _tmp388[9U]="asm_expr";static char _tmp389[13U]="volatile_opt";static char _tmp38A[12U]="asm_out_opt";static char _tmp38B[12U]="asm_outlist";static char _tmp38C[11U]="asm_in_opt";static char _tmp38D[11U]="asm_inlist";static char _tmp38E[11U]="asm_io_elt";static char _tmp38F[16U]="asm_clobber_opt";static char _tmp390[17U]="asm_clobber_list";static char _tmp391[15U]="unary_operator";static char _tmp392[19U]="postfix_expression";static char _tmp393[17U]="field_expression";static char _tmp394[19U]="primary_expression";static char _tmp395[25U]="argument_expression_list";static char _tmp396[26U]="argument_expression_list0";static char _tmp397[9U]="constant";static char _tmp398[20U]="qual_opt_identifier";static char _tmp399[17U]="qual_opt_typedef";static char _tmp39A[18U]="struct_union_name";static char _tmp39B[11U]="field_name";static char _tmp39C[12U]="right_angle";
# 1616 "parse.y"
static struct _fat_ptr Cyc_yytname[314U]={{_tmp263,_tmp263,_tmp263 + 2U},{_tmp264,_tmp264,_tmp264 + 6U},{_tmp265,_tmp265,_tmp265 + 12U},{_tmp266,_tmp266,_tmp266 + 5U},{_tmp267,_tmp267,_tmp267 + 9U},{_tmp268,_tmp268,_tmp268 + 7U},{_tmp269,_tmp269,_tmp269 + 7U},{_tmp26A,_tmp26A,_tmp26A + 8U},{_tmp26B,_tmp26B,_tmp26B + 5U},{_tmp26C,_tmp26C,_tmp26C + 5U},{_tmp26D,_tmp26D,_tmp26D + 6U},{_tmp26E,_tmp26E,_tmp26E + 4U},{_tmp26F,_tmp26F,_tmp26F + 5U},{_tmp270,_tmp270,_tmp270 + 6U},{_tmp271,_tmp271,_tmp271 + 7U},{_tmp272,_tmp272,_tmp272 + 7U},{_tmp273,_tmp273,_tmp273 + 9U},{_tmp274,_tmp274,_tmp274 + 6U},{_tmp275,_tmp275,_tmp275 + 9U},{_tmp276,_tmp276,_tmp276 + 9U},{_tmp277,_tmp277,_tmp277 + 7U},{_tmp278,_tmp278,_tmp278 + 6U},{_tmp279,_tmp279,_tmp279 + 5U},{_tmp27A,_tmp27A,_tmp27A + 8U},{_tmp27B,_tmp27B,_tmp27B + 7U},{_tmp27C,_tmp27C,_tmp27C + 7U},{_tmp27D,_tmp27D,_tmp27D + 9U},{_tmp27E,_tmp27E,_tmp27E + 3U},{_tmp27F,_tmp27F,_tmp27F + 5U},{_tmp280,_tmp280,_tmp280 + 7U},{_tmp281,_tmp281,_tmp281 + 6U},{_tmp282,_tmp282,_tmp282 + 3U},{_tmp283,_tmp283,_tmp283 + 4U},{_tmp284,_tmp284,_tmp284 + 5U},{_tmp285,_tmp285,_tmp285 + 9U},{_tmp286,_tmp286,_tmp286 + 6U},{_tmp287,_tmp287,_tmp287 + 7U},{_tmp288,_tmp288,_tmp288 + 5U},{_tmp289,_tmp289,_tmp289 + 7U},{_tmp28A,_tmp28A,_tmp28A + 16U},{_tmp28B,_tmp28B,_tmp28B + 10U},{_tmp28C,_tmp28C,_tmp28C + 8U},{_tmp28D,_tmp28D,_tmp28D + 4U},{_tmp28E,_tmp28E,_tmp28E + 6U},{_tmp28F,_tmp28F,_tmp28F + 4U},{_tmp290,_tmp290,_tmp290 + 6U},{_tmp291,_tmp291,_tmp291 + 7U},{_tmp292,_tmp292,_tmp292 + 9U},{_tmp293,_tmp293,_tmp293 + 5U},{_tmp294,_tmp294,_tmp294 + 4U},{_tmp295,_tmp295,_tmp295 + 9U},{_tmp296,_tmp296,_tmp296 + 9U},{_tmp297,_tmp297,_tmp297 + 6U},{_tmp298,_tmp298,_tmp298 + 10U},{_tmp299,_tmp299,_tmp299 + 9U},{_tmp29A,_tmp29A,_tmp29A + 7U},{_tmp29B,_tmp29B,_tmp29B + 8U},{_tmp29C,_tmp29C,_tmp29C + 15U},{_tmp29D,_tmp29D,_tmp29D + 7U},{_tmp29E,_tmp29E,_tmp29E + 8U},{_tmp29F,_tmp29F,_tmp29F + 5U},{_tmp2A0,_tmp2A0,_tmp2A0 + 9U},{_tmp2A1,_tmp2A1,_tmp2A1 + 6U},{_tmp2A2,_tmp2A2,_tmp2A2 + 7U},{_tmp2A3,_tmp2A3,_tmp2A3 + 5U},{_tmp2A4,_tmp2A4,_tmp2A4 + 8U},{_tmp2A5,_tmp2A5,_tmp2A5 + 7U},{_tmp2A6,_tmp2A6,_tmp2A6 + 8U},{_tmp2A7,_tmp2A7,_tmp2A7 + 7U},{_tmp2A8,_tmp2A8,_tmp2A8 + 10U},{_tmp2A9,_tmp2A9,_tmp2A9 + 11U},{_tmp2AA,_tmp2AA,_tmp2AA + 8U},{_tmp2AB,_tmp2AB,_tmp2AB + 8U},{_tmp2AC,_tmp2AC,_tmp2AC + 10U},{_tmp2AD,_tmp2AD,_tmp2AD + 9U},{_tmp2AE,_tmp2AE,_tmp2AE + 13U},{_tmp2AF,_tmp2AF,_tmp2AF + 10U},{_tmp2B0,_tmp2B0,_tmp2B0 + 9U},{_tmp2B1,_tmp2B1,_tmp2B1 + 13U},{_tmp2B2,_tmp2B2,_tmp2B2 + 14U},{_tmp2B3,_tmp2B3,_tmp2B3 + 14U},{_tmp2B4,_tmp2B4,_tmp2B4 + 13U},{_tmp2B5,_tmp2B5,_tmp2B5 + 12U},{_tmp2B6,_tmp2B6,_tmp2B6 + 16U},{_tmp2B7,_tmp2B7,_tmp2B7 + 14U},{_tmp2B8,_tmp2B8,_tmp2B8 + 12U},{_tmp2B9,_tmp2B9,_tmp2B9 + 12U},{_tmp2BA,_tmp2BA,_tmp2BA + 16U},{_tmp2BB,_tmp2BB,_tmp2BB + 7U},{_tmp2BC,_tmp2BC,_tmp2BC + 7U},{_tmp2BD,_tmp2BD,_tmp2BD + 7U},{_tmp2BE,_tmp2BE,_tmp2BE + 8U},{_tmp2BF,_tmp2BF,_tmp2BF + 9U},{_tmp2C0,_tmp2C0,_tmp2C0 + 6U},{_tmp2C1,_tmp2C1,_tmp2C1 + 6U},{_tmp2C2,_tmp2C2,_tmp2C2 + 6U},{_tmp2C3,_tmp2C3,_tmp2C3 + 6U},{_tmp2C4,_tmp2C4,_tmp2C4 + 7U},{_tmp2C5,_tmp2C5,_tmp2C5 + 6U},{_tmp2C6,_tmp2C6,_tmp2C6 + 11U},{_tmp2C7,_tmp2C7,_tmp2C7 + 11U},{_tmp2C8,_tmp2C8,_tmp2C8 + 11U},{_tmp2C9,_tmp2C9,_tmp2C9 + 11U},{_tmp2CA,_tmp2CA,_tmp2CA + 11U},{_tmp2CB,_tmp2CB,_tmp2CB + 12U},{_tmp2CC,_tmp2CC,_tmp2CC + 13U},{_tmp2CD,_tmp2CD,_tmp2CD + 11U},{_tmp2CE,_tmp2CE,_tmp2CE + 11U},{_tmp2CF,_tmp2CF,_tmp2CF + 10U},{_tmp2D0,_tmp2D0,_tmp2D0 + 9U},{_tmp2D1,_tmp2D1,_tmp2D1 + 11U},{_tmp2D2,_tmp2D2,_tmp2D2 + 12U},{_tmp2D3,_tmp2D3,_tmp2D3 + 11U},{_tmp2D4,_tmp2D4,_tmp2D4 + 17U},{_tmp2D5,_tmp2D5,_tmp2D5 + 7U},{_tmp2D6,_tmp2D6,_tmp2D6 + 8U},{_tmp2D7,_tmp2D7,_tmp2D7 + 19U},{_tmp2D8,_tmp2D8,_tmp2D8 + 20U},{_tmp2D9,_tmp2D9,_tmp2D9 + 18U},{_tmp2DA,_tmp2DA,_tmp2DA + 9U},{_tmp2DB,_tmp2DB,_tmp2DB + 13U},{_tmp2DC,_tmp2DC,_tmp2DC + 16U},{_tmp2DD,_tmp2DD,_tmp2DD + 18U},{_tmp2DE,_tmp2DE,_tmp2DE + 10U},{_tmp2DF,_tmp2DF,_tmp2DF + 8U},{_tmp2E0,_tmp2E0,_tmp2E0 + 4U},{_tmp2E1,_tmp2E1,_tmp2E1 + 4U},{_tmp2E2,_tmp2E2,_tmp2E2 + 4U},{_tmp2E3,_tmp2E3,_tmp2E3 + 4U},{_tmp2E4,_tmp2E4,_tmp2E4 + 4U},{_tmp2E5,_tmp2E5,_tmp2E5 + 4U},{_tmp2E6,_tmp2E6,_tmp2E6 + 4U},{_tmp2E7,_tmp2E7,_tmp2E7 + 4U},{_tmp2E8,_tmp2E8,_tmp2E8 + 4U},{_tmp2E9,_tmp2E9,_tmp2E9 + 4U},{_tmp2EA,_tmp2EA,_tmp2EA + 4U},{_tmp2EB,_tmp2EB,_tmp2EB + 4U},{_tmp2EC,_tmp2EC,_tmp2EC + 4U},{_tmp2ED,_tmp2ED,_tmp2ED + 4U},{_tmp2EE,_tmp2EE,_tmp2EE + 4U},{_tmp2EF,_tmp2EF,_tmp2EF + 4U},{_tmp2F0,_tmp2F0,_tmp2F0 + 4U},{_tmp2F1,_tmp2F1,_tmp2F1 + 4U},{_tmp2F2,_tmp2F2,_tmp2F2 + 4U},{_tmp2F3,_tmp2F3,_tmp2F3 + 4U},{_tmp2F4,_tmp2F4,_tmp2F4 + 4U},{_tmp2F5,_tmp2F5,_tmp2F5 + 4U},{_tmp2F6,_tmp2F6,_tmp2F6 + 4U},{_tmp2F7,_tmp2F7,_tmp2F7 + 4U},{_tmp2F8,_tmp2F8,_tmp2F8 + 4U},{_tmp2F9,_tmp2F9,_tmp2F9 + 4U},{_tmp2FA,_tmp2FA,_tmp2FA + 4U},{_tmp2FB,_tmp2FB,_tmp2FB + 5U},{_tmp2FC,_tmp2FC,_tmp2FC + 17U},{_tmp2FD,_tmp2FD,_tmp2FD + 18U},{_tmp2FE,_tmp2FE,_tmp2FE + 19U},{_tmp2FF,_tmp2FF,_tmp2FF + 16U},{_tmp300,_tmp300,_tmp300 + 13U},{_tmp301,_tmp301,_tmp301 + 14U},{_tmp302,_tmp302,_tmp302 + 17U},{_tmp303,_tmp303,_tmp303 + 16U},{_tmp304,_tmp304,_tmp304 + 12U},{_tmp305,_tmp305,_tmp305 + 19U},{_tmp306,_tmp306,_tmp306 + 13U},{_tmp307,_tmp307,_tmp307 + 21U},{_tmp308,_tmp308,_tmp308 + 15U},{_tmp309,_tmp309,_tmp309 + 20U},{_tmp30A,_tmp30A,_tmp30A + 21U},{_tmp30B,_tmp30B,_tmp30B + 13U},{_tmp30C,_tmp30C,_tmp30C + 15U},{_tmp30D,_tmp30D,_tmp30D + 17U},{_tmp30E,_tmp30E,_tmp30E + 19U},{_tmp30F,_tmp30F,_tmp30F + 12U},{_tmp310,_tmp310,_tmp310 + 17U},{_tmp311,_tmp311,_tmp311 + 23U},{_tmp312,_tmp312,_tmp312 + 24U},{_tmp313,_tmp313,_tmp313 + 15U},{_tmp314,_tmp314,_tmp314 + 11U},{_tmp315,_tmp315,_tmp315 + 15U},{_tmp316,_tmp316,_tmp316 + 10U},{_tmp317,_tmp317,_tmp317 + 15U},{_tmp318,_tmp318,_tmp318 + 25U},{_tmp319,_tmp319,_tmp319 + 5U},{_tmp31A,_tmp31A,_tmp31A + 15U},{_tmp31B,_tmp31B,_tmp31B + 15U},{_tmp31C,_tmp31C,_tmp31C + 11U},{_tmp31D,_tmp31D,_tmp31D + 22U},{_tmp31E,_tmp31E,_tmp31E + 26U},{_tmp31F,_tmp31F,_tmp31F + 16U},{_tmp320,_tmp320,_tmp320 + 16U},{_tmp321,_tmp321,_tmp321 + 24U},{_tmp322,_tmp322,_tmp322 + 25U},{_tmp323,_tmp323,_tmp323 + 21U},{_tmp324,_tmp324,_tmp324 + 22U},{_tmp325,_tmp325,_tmp325 + 16U},{_tmp326,_tmp326,_tmp326 + 19U},{_tmp327,_tmp327,_tmp327 + 25U},{_tmp328,_tmp328,_tmp328 + 35U},{_tmp329,_tmp329,_tmp329 + 23U},{_tmp32A,_tmp32A,_tmp32A + 24U},{_tmp32B,_tmp32B,_tmp32B + 18U},{_tmp32C,_tmp32C,_tmp32C + 20U},{_tmp32D,_tmp32D,_tmp32D + 19U},{_tmp32E,_tmp32E,_tmp32E + 19U},{_tmp32F,_tmp32F,_tmp32F + 14U},{_tmp330,_tmp330,_tmp330 + 19U},{_tmp331,_tmp331,_tmp331 + 20U},{_tmp332,_tmp332,_tmp332 + 14U},{_tmp333,_tmp333,_tmp333 + 11U},{_tmp334,_tmp334,_tmp334 + 23U},{_tmp335,_tmp335,_tmp335 + 18U},{_tmp336,_tmp336,_tmp336 + 30U},{_tmp337,_tmp337,_tmp337 + 8U},{_tmp338,_tmp338,_tmp338 + 12U},{_tmp339,_tmp339,_tmp339 + 14U},{_tmp33A,_tmp33A,_tmp33A + 13U},{_tmp33B,_tmp33B,_tmp33B + 23U},{_tmp33C,_tmp33C,_tmp33C + 14U},{_tmp33D,_tmp33D,_tmp33D + 18U},{_tmp33E,_tmp33E,_tmp33E + 8U},{_tmp33F,_tmp33F,_tmp33F + 11U},{_tmp340,_tmp340,_tmp340 + 20U},{_tmp341,_tmp341,_tmp341 + 9U},{_tmp342,_tmp342,_tmp342 + 16U},{_tmp343,_tmp343,_tmp343 + 19U},{_tmp344,_tmp344,_tmp344 + 10U},{_tmp345,_tmp345,_tmp345 + 16U},{_tmp346,_tmp346,_tmp346 + 11U},{_tmp347,_tmp347,_tmp347 + 14U},{_tmp348,_tmp348,_tmp348 + 11U},{_tmp349,_tmp349,_tmp349 + 15U},{_tmp34A,_tmp34A,_tmp34A + 22U},{_tmp34B,_tmp34B,_tmp34B + 16U},{_tmp34C,_tmp34C,_tmp34C + 17U},{_tmp34D,_tmp34D,_tmp34D + 12U},{_tmp34E,_tmp34E,_tmp34E + 18U},{_tmp34F,_tmp34F,_tmp34F + 17U},{_tmp350,_tmp350,_tmp350 + 12U},{_tmp351,_tmp351,_tmp351 + 16U},{_tmp352,_tmp352,_tmp352 + 11U},{_tmp353,_tmp353,_tmp353 + 10U},{_tmp354,_tmp354,_tmp354 + 14U},{_tmp355,_tmp355,_tmp355 + 15U},{_tmp356,_tmp356,_tmp356 + 20U},{_tmp357,_tmp357,_tmp357 + 27U},{_tmp358,_tmp358,_tmp358 + 10U},{_tmp359,_tmp359,_tmp359 + 18U},{_tmp35A,_tmp35A,_tmp35A + 21U},{_tmp35B,_tmp35B,_tmp35B + 19U},{_tmp35C,_tmp35C,_tmp35C + 16U},{_tmp35D,_tmp35D,_tmp35D + 20U},{_tmp35E,_tmp35E,_tmp35E + 15U},{_tmp35F,_tmp35F,_tmp35F + 20U},{_tmp360,_tmp360,_tmp360 + 15U},{_tmp361,_tmp361,_tmp361 + 12U},{_tmp362,_tmp362,_tmp362 + 20U},{_tmp363,_tmp363,_tmp363 + 19U},{_tmp364,_tmp364,_tmp364 + 20U},{_tmp365,_tmp365,_tmp365 + 21U},{_tmp366,_tmp366,_tmp366 + 21U},{_tmp367,_tmp367,_tmp367 + 12U},{_tmp368,_tmp368,_tmp368 + 17U},{_tmp369,_tmp369,_tmp369 + 19U},{_tmp36A,_tmp36A,_tmp36A + 14U},{_tmp36B,_tmp36B,_tmp36B + 17U},{_tmp36C,_tmp36C,_tmp36C + 23U},{_tmp36D,_tmp36D,_tmp36D + 13U},{_tmp36E,_tmp36E,_tmp36E + 14U},{_tmp36F,_tmp36F,_tmp36F + 16U},{_tmp370,_tmp370,_tmp370 + 16U},{_tmp371,_tmp371,_tmp371 + 8U},{_tmp372,_tmp372,_tmp372 + 19U},{_tmp373,_tmp373,_tmp373 + 20U},{_tmp374,_tmp374,_tmp374 + 14U},{_tmp375,_tmp375,_tmp375 + 19U},{_tmp376,_tmp376,_tmp376 + 20U},{_tmp377,_tmp377,_tmp377 + 11U},{_tmp378,_tmp378,_tmp378 + 22U},{_tmp379,_tmp379,_tmp379 + 20U},{_tmp37A,_tmp37A,_tmp37A + 23U},{_tmp37B,_tmp37B,_tmp37B + 20U},{_tmp37C,_tmp37C,_tmp37C + 22U},{_tmp37D,_tmp37D,_tmp37D + 23U},{_tmp37E,_tmp37E,_tmp37E + 24U},{_tmp37F,_tmp37F,_tmp37F + 24U},{_tmp380,_tmp380,_tmp380 + 15U},{_tmp381,_tmp381,_tmp381 + 20U},{_tmp382,_tmp382,_tmp382 + 22U},{_tmp383,_tmp383,_tmp383 + 17U},{_tmp384,_tmp384,_tmp384 + 20U},{_tmp385,_tmp385,_tmp385 + 26U},{_tmp386,_tmp386,_tmp386 + 16U},{_tmp387,_tmp387,_tmp387 + 17U},{_tmp388,_tmp388,_tmp388 + 9U},{_tmp389,_tmp389,_tmp389 + 13U},{_tmp38A,_tmp38A,_tmp38A + 12U},{_tmp38B,_tmp38B,_tmp38B + 12U},{_tmp38C,_tmp38C,_tmp38C + 11U},{_tmp38D,_tmp38D,_tmp38D + 11U},{_tmp38E,_tmp38E,_tmp38E + 11U},{_tmp38F,_tmp38F,_tmp38F + 16U},{_tmp390,_tmp390,_tmp390 + 17U},{_tmp391,_tmp391,_tmp391 + 15U},{_tmp392,_tmp392,_tmp392 + 19U},{_tmp393,_tmp393,_tmp393 + 17U},{_tmp394,_tmp394,_tmp394 + 19U},{_tmp395,_tmp395,_tmp395 + 25U},{_tmp396,_tmp396,_tmp396 + 26U},{_tmp397,_tmp397,_tmp397 + 9U},{_tmp398,_tmp398,_tmp398 + 20U},{_tmp399,_tmp399,_tmp399 + 17U},{_tmp39A,_tmp39A,_tmp39A + 18U},{_tmp39B,_tmp39B,_tmp39B + 11U},{_tmp39C,_tmp39C,_tmp39C + 12U}};
# 1674
static short Cyc_yyr1[562U]={0,152,153,153,153,153,153,153,153,153,153,153,153,154,155,156,157,158,158,159,159,159,160,160,161,161,161,162,162,162,163,163,164,164,164,165,165,166,166,166,166,167,167,168,169,170,171,172,172,172,172,172,172,172,173,173,174,174,174,174,174,174,174,174,174,174,174,175,175,175,175,175,175,175,176,176,177,178,178,179,179,179,179,180,180,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,181,182,183,183,183,184,184,184,185,185,186,186,186,187,187,187,187,187,188,188,189,189,190,190,191,191,192,193,193,194,194,195,196,196,196,196,196,196,197,197,197,197,197,197,198,199,199,200,200,200,200,201,201,202,202,203,203,203,204,204,205,205,205,205,206,206,206,207,207,208,208,209,209,210,210,210,210,210,210,210,210,210,210,211,211,211,211,211,211,211,211,211,211,211,212,212,213,214,214,215,215,215,215,215,215,215,215,216,216,216,217,217,218,218,218,219,219,219,220,220,221,221,221,221,222,222,223,223,224,224,225,225,226,226,227,227,228,228,228,228,229,229,230,230,231,231,231,232,233,233,234,234,235,235,235,235,235,236,236,236,236,237,237,238,238,239,239,240,240,241,241,241,241,241,242,242,243,243,243,244,244,244,244,244,244,244,244,244,244,244,245,245,245,245,245,245,246,247,247,248,248,249,249,249,249,249,249,249,249,250,250,250,250,250,250,251,251,251,251,251,251,252,252,252,252,252,252,252,252,252,252,252,252,252,252,253,253,253,253,253,253,253,253,254,255,255,256,256,257,257,258,258,259,259,260,260,261,261,261,262,262,262,262,262,263,263,263,264,264,264,265,265,265,265,266,266,267,267,267,267,267,267,268,269,270,270,270,270,270,270,270,270,270,270,270,270,270,270,270,270,270,271,271,271,272,272,273,273,274,274,274,275,275,276,276,277,277,277,278,278,278,278,278,278,278,278,278,278,278,279,279,279,279,279,279,279,280,281,281,282,282,283,283,284,284,285,285,286,286,286,287,287,287,287,287,288,288,288,289,289,289,290,290,290,290,291,291,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,292,293,294,294,295,295,295,296,296,297,297,297,298,298,299,300,300,300,301,301,302,302,302,303,303,303,303,303,303,303,303,303,303,303,304,304,304,304,305,305,305,305,305,305,305,305,305,305,305,306,307,307,308,308,308,308,308,309,309,310,310,311,311,312,312,313,313};
# 1734
static short Cyc_yyr2[562U]={0,1,2,3,5,3,5,8,3,3,3,3,0,1,1,2,1,0,4,1,2,3,0,1,4,3,4,1,2,3,0,4,1,1,1,1,0,3,4,4,5,3,4,2,1,2,1,2,3,5,3,6,3,8,1,2,1,2,2,1,2,1,2,1,2,1,2,1,1,1,1,2,1,1,0,1,6,1,3,1,1,4,8,1,2,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,3,4,4,1,4,1,4,1,1,1,1,5,2,4,1,3,1,2,3,4,9,8,4,3,0,3,1,1,0,1,1,2,1,1,3,1,3,3,1,2,1,2,1,2,1,2,1,2,1,2,1,1,3,2,2,0,3,4,0,4,0,6,3,5,1,2,1,2,3,3,0,1,1,2,5,1,2,1,2,1,3,4,4,5,6,7,4,4,2,1,1,3,4,4,5,6,7,4,4,2,1,2,5,0,2,4,4,1,1,1,1,1,1,2,2,1,0,3,0,1,1,0,1,1,0,2,3,5,5,7,1,3,0,2,0,2,3,5,0,1,1,3,2,3,4,1,1,3,1,3,2,1,2,1,1,3,1,1,2,3,4,8,8,1,2,3,4,2,2,1,2,3,2,1,2,1,2,3,4,3,1,3,1,1,2,3,3,4,4,5,6,5,7,6,4,2,1,1,1,1,1,1,3,1,2,2,3,1,2,3,4,1,2,1,2,5,7,7,5,8,6,0,4,4,5,6,7,5,7,6,7,7,8,7,8,8,9,6,7,7,8,3,2,2,2,3,2,4,5,1,1,5,1,3,1,3,1,3,1,3,1,3,1,3,3,1,3,3,3,3,1,3,3,1,3,3,1,3,3,3,1,4,1,2,2,4,2,6,1,1,1,3,1,1,3,6,6,4,4,5,4,2,2,2,4,4,4,1,3,1,1,3,1,2,1,3,1,1,3,1,3,1,3,3,1,1,1,1,1,1,1,1,1,1,1,1,5,2,2,2,5,5,1,1,3,1,3,1,3,1,3,1,3,1,3,3,1,3,3,3,3,1,3,3,1,3,3,1,3,3,3,1,4,1,2,2,2,2,2,2,4,2,6,4,6,6,9,11,4,6,6,4,2,2,4,5,0,1,0,2,3,1,3,0,2,3,1,3,4,0,1,2,1,3,1,1,1,1,4,3,4,3,3,2,2,5,6,7,1,1,3,3,1,4,1,1,1,3,2,5,4,5,5,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
# 1794
static short Cyc_yydefact[1137U]={0,34,67,68,69,70,72,85,86,87,88,89,90,91,92,93,109,110,111,127,128,63,0,0,97,0,0,73,0,0,165,104,106,0,0,0,13,14,0,0,0,552,231,554,553,555,0,217,0,100,0,217,216,1,0,0,0,0,32,0,0,33,0,56,65,59,83,61,94,95,0,98,0,0,176,0,201,204,99,180,125,71,70,64,0,113,0,58,551,0,552,547,548,549,550,0,125,0,0,391,0,0,0,254,0,393,394,43,45,0,0,0,0,0,0,0,0,166,0,0,0,214,0,0,0,0,215,0,0,0,2,0,0,0,0,47,0,133,134,136,57,66,60,62,129,556,557,125,125,0,54,0,0,36,0,233,0,189,177,202,0,208,209,212,213,0,211,210,222,204,0,84,71,117,0,115,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,536,537,497,0,0,0,0,0,517,515,516,0,420,422,436,444,446,448,450,452,454,457,462,465,468,472,0,474,518,535,533,552,403,0,0,0,0,404,0,0,402,50,0,0,125,0,0,0,143,139,141,274,276,0,0,52,0,0,8,9,0,125,558,559,232,108,0,0,0,181,101,252,0,249,10,11,0,3,0,5,0,48,0,0,0,36,0,130,131,156,124,0,163,0,0,0,0,0,0,0,0,0,0,0,0,552,304,306,0,314,308,0,312,297,298,299,0,300,301,302,0,55,36,136,35,37,281,0,239,255,0,0,235,233,0,219,0,0,0,224,74,223,205,0,118,114,0,0,0,482,0,0,494,438,472,0,439,440,0,0,0,0,0,0,0,0,0,0,0,475,476,498,493,0,478,0,0,0,0,479,477,0,96,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,426,427,428,429,430,431,432,433,434,435,425,0,480,0,524,525,0,0,0,539,0,125,395,0,0,0,417,552,559,0,0,0,0,270,413,418,0,415,0,0,392,410,411,0,408,256,0,0,0,0,277,0,247,144,149,145,147,140,142,233,0,283,275,284,561,560,0,103,105,0,0,107,123,80,79,0,77,218,182,233,251,178,283,253,190,191,0,102,16,30,44,0,46,0,135,137,258,257,36,38,120,132,0,0,0,151,152,159,0,125,125,171,0,0,0,0,0,552,0,0,0,343,344,345,0,0,347,0,0,0,315,309,136,313,307,305,39,0,188,240,0,0,0,246,234,241,159,0,0,0,235,187,221,220,183,219,0,0,225,75,126,119,443,116,112,0,0,0,0,552,259,264,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,538,545,0,544,421,445,0,447,449,451,453,455,456,460,461,458,459,463,464,466,467,469,470,471,424,423,523,520,0,522,0,0,0,406,407,0,273,0,414,268,271,401,0,269,405,398,0,49,0,399,0,278,0,150,146,148,0,235,0,219,0,285,0,233,0,296,280,0,0,125,0,0,0,143,0,125,0,233,0,200,179,250,0,22,4,6,40,0,155,138,156,0,0,154,235,164,173,172,0,0,167,0,0,0,322,0,0,0,0,0,0,342,346,0,0,0,310,303,0,41,282,233,0,243,0,0,161,236,0,159,239,227,184,206,207,225,203,481,0,0,0,260,0,265,484,0,0,0,0,0,534,489,492,0,0,495,499,0,0,473,541,0,0,521,519,0,0,0,0,272,416,419,409,412,400,279,248,159,0,286,287,219,0,0,235,219,0,0,51,235,552,0,76,78,0,192,0,0,235,0,219,0,0,0,17,23,153,0,157,129,162,174,171,171,0,0,0,0,0,0,0,0,0,0,0,0,0,322,348,0,311,42,235,0,244,242,0,185,0,161,235,0,226,530,0,529,0,261,266,0,0,0,0,0,441,442,523,522,504,0,543,526,0,546,437,540,542,0,396,397,161,159,289,295,159,0,288,219,0,129,0,81,193,199,159,0,198,194,219,0,0,0,0,0,0,0,170,169,316,322,0,0,0,0,0,0,350,351,353,355,357,359,361,363,366,371,374,377,381,383,389,390,0,0,319,328,0,0,0,0,0,0,0,0,0,0,349,229,245,0,237,186,228,233,483,0,0,267,485,486,0,0,491,490,0,510,504,500,502,496,527,0,292,161,161,159,290,53,0,0,161,159,195,31,25,0,0,27,0,7,158,122,0,0,0,322,0,387,0,0,384,322,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,385,322,0,330,0,0,0,338,0,0,0,0,0,0,321,0,0,235,532,531,0,0,0,0,511,510,507,505,0,501,528,291,294,161,121,0,196,161,26,24,28,0,0,19,175,317,318,0,0,0,0,322,324,354,0,356,358,360,362,364,365,369,370,367,368,372,373,375,376,378,379,380,0,323,329,331,332,0,340,339,0,334,0,0,0,160,238,230,0,0,0,0,0,513,512,0,506,503,293,0,197,29,18,20,0,320,386,0,382,325,0,322,333,341,335,336,0,263,262,487,0,509,0,508,82,21,0,352,322,326,337,0,514,388,327,488,0,0,0};
# 1911
static short Cyc_yydefgoto[162U]={1134,53,54,55,56,487,878,1040,790,791,962,669,57,320,58,304,59,489,60,491,61,151,62,63,556,243,473,474,244,66,259,245,68,173,174,69,171,70,281,282,136,137,138,283,246,455,502,503,504,679,823,71,72,684,685,686,73,505,74,479,75,76,168,169,77,121,552,335,722,642,78,643,546,713,538,542,543,449,328,268,102,103,569,494,570,429,430,431,247,321,322,644,461,307,308,309,310,311,312,805,313,314,891,892,893,894,895,896,897,898,899,900,901,902,903,904,905,906,432,441,442,433,434,435,315,207,409,208,561,209,210,211,212,213,214,215,216,217,218,219,220,367,368,845,942,943,1023,944,1025,1092,221,222,830,223,588,589,224,225,80,963,436,465};
# 1931
static short Cyc_yypact[1137U]={3023,- -32768,- -32768,- -32768,- -32768,- 26,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,3799,284,74,- -32768,3799,1492,- -32768,38,40,- -32768,110,136,58,157,185,- -32768,- -32768,203,100,264,- -32768,236,- -32768,- -32768,- -32768,227,263,1018,276,299,263,- -32768,- -32768,272,331,352,2880,- -32768,496,570,- -32768,1025,3799,3799,3799,- -32768,3799,- -32768,- -32768,476,- -32768,38,3709,71,199,250,750,- -32768,- -32768,392,407,429,- -32768,38,443,7242,- -32768,- -32768,2862,290,- -32768,- -32768,- -32768,- -32768,440,392,470,7242,- -32768,460,2862,459,484,504,- -32768,286,- -32768,- -32768,4015,4015,428,495,2880,2880,7242,424,- -32768,- 52,531,7242,- -32768,133,537,- 52,4495,- -32768,2880,2880,3165,- -32768,2880,3165,2880,3165,- -32768,553,554,- -32768,3575,- -32768,- -32768,- -32768,- -32768,4495,- -32768,- -32768,392,337,1844,- -32768,3709,1025,581,4015,3886,5338,- -32768,71,- -32768,580,- -32768,- -32768,- -32768,- -32768,596,- -32768,- -32768,159,750,4015,- -32768,- -32768,605,641,612,38,7553,633,7650,7242,7405,661,669,672,679,683,687,697,706,721,729,738,7650,7650,- -32768,- -32768,806,7701,2584,740,7701,7701,- -32768,- -32768,- -32768,360,- -32768,- -32768,45,761,716,732,741,280,132,691,138,72,- -32768,671,7701,91,- 39,- -32768,766,130,- -32768,2862,222,775,1133,786,393,1315,- -32768,- -32768,788,7242,392,1315,768,4234,4495,4582,4495,433,- -32768,12,12,- -32768,793,774,- -32768,- -32768,413,392,- -32768,- -32768,- -32768,- -32768,28,781,779,- -32768,- -32768,287,451,- -32768,- -32768,- -32768,787,- -32768,791,- -32768,792,- -32768,133,5450,3709,581,798,4495,- -32768,728,789,38,800,795,336,802,4651,803,819,808,820,5562,2440,4651,- 9,807,- -32768,- -32768,814,1993,1993,1025,1993,- -32768,- -32768,- -32768,822,- -32768,- -32768,- -32768,233,- -32768,581,818,- -32768,- -32768,810,81,838,- -32768,- 4,823,825,385,826,727,811,7242,4015,- -32768,831,- -32768,- -32768,81,38,- -32768,7242,829,2584,- -32768,4495,2584,- -32768,- -32768,- -32768,4763,- -32768,861,7242,7242,7242,7242,7242,7242,853,7242,4495,932,7242,- -32768,- -32768,- -32768,- -32768,834,- -32768,1993,841,453,7242,- -32768,- -32768,7242,- -32768,7701,7242,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7242,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,7242,- -32768,- 52,- -32768,- -32768,5674,- 52,7242,- -32768,837,392,- -32768,847,848,851,- -32768,26,440,- 52,7242,2862,183,- -32768,- -32768,- -32768,857,859,852,2862,- -32768,- -32768,- -32768,854,865,- -32768,467,1133,864,4015,- -32768,858,871,- -32768,4582,4582,4582,- -32768,- -32768,3300,5786,478,- -32768,307,- -32768,- -32768,- 4,- -32768,- -32768,868,890,- -32768,879,- -32768,875,877,884,- -32768,- -32768,1458,- -32768,308,261,- -32768,- -32768,- -32768,4495,- -32768,- -32768,966,- -32768,2880,- -32768,2880,- -32768,- -32768,- -32768,- -32768,581,- -32768,- -32768,- -32768,849,7242,889,891,- -32768,19,232,392,392,817,7242,7242,888,898,7242,880,996,2291,902,- -32768,- -32768,- -32768,475,983,- -32768,5898,2142,2728,- -32768,- -32768,3575,- -32768,- -32768,- -32768,- -32768,4015,- -32768,- -32768,4495,897,4321,- -32768,- -32768,894,951,- 4,900,4408,825,- -32768,- -32768,- -32768,- -32768,727,904,208,636,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,905,912,909,920,914,- -32768,- -32768,709,5450,918,926,927,928,929,516,924,935,936,239,937,946,934,7502,- -32768,- -32768,938,968,- -32768,761,152,716,732,741,280,132,132,691,691,691,691,138,138,72,72,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,940,- -32768,68,4015,5223,4495,- -32768,4495,- -32768,957,- -32768,- -32768,- -32768,- -32768,1265,- -32768,- -32768,- -32768,1440,- -32768,939,- -32768,317,- -32768,4495,- -32768,- -32768,- -32768,964,825,965,727,960,307,4015,4105,6010,- -32768,- -32768,7242,979,392,7354,971,28,3435,972,392,4015,3886,6122,- -32768,308,- -32768,985,1061,- -32768,- -32768,- -32768,874,- -32768,- -32768,728,984,7242,- -32768,825,- -32768,- -32768,- -32768,991,38,508,545,546,7242,821,555,4651,986,6234,6346,566,- -32768,- -32768,994,987,988,1993,- -32768,3709,- -32768,810,999,4015,- -32768,998,- 4,1040,- -32768,995,951,102,- -32768,- -32768,- -32768,- -32768,636,- -32768,1000,204,1000,1001,- -32768,4878,- -32768,- -32768,7242,7242,1104,7242,7405,- -32768,- -32768,- -32768,- 52,- 52,- -32768,997,1006,4993,- -32768,- -32768,7242,7242,- -32768,- -32768,81,719,1021,1023,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,951,1011,- -32768,- -32768,727,81,1022,825,727,1024,564,- -32768,825,1034,1029,- -32768,- -32768,1031,- -32768,81,1035,825,1036,727,1028,3165,1045,1124,- -32768,- -32768,7242,- -32768,4495,- -32768,1042,88,817,4651,1047,1044,1207,1043,1055,4651,7242,6458,592,6570,639,6682,821,- -32768,1058,- -32768,- -32768,825,339,- -32768,- -32768,1051,- -32768,1068,1040,825,4495,- -32768,- -32768,342,- -32768,7242,- -32768,- -32768,5450,1056,1057,1059,1065,- -32768,861,1060,1064,4,1066,- -32768,- -32768,725,- -32768,- -32768,- -32768,- -32768,5223,- -32768,- -32768,1040,951,- -32768,- -32768,951,1069,- -32768,727,1070,4495,1076,- -32768,- -32768,- -32768,951,1073,- -32768,- -32768,727,1072,981,1078,2880,1074,1082,4495,- -32768,- -32768,1182,821,1085,7798,1080,2728,7701,1079,- -32768,50,- -32768,1118,1075,710,771,194,790,349,147,- -32768,- -32768,- -32768,- -32768,1121,7701,1993,- -32768,- -32768,588,4651,602,6794,4651,604,6906,7018,662,1093,- -32768,- -32768,- -32768,7242,1094,- -32768,- -32768,999,- -32768,323,362,- -32768,- -32768,- -32768,4495,1200,- -32768,- -32768,1095,120,365,- -32768,- -32768,- -32768,- -32768,5108,- -32768,1040,1040,951,- -32768,- -32768,1102,1106,1040,951,- -32768,- -32768,- -32768,1108,1109,664,424,- -32768,- -32768,- -32768,613,4651,1110,821,2584,- -32768,4495,1105,- -32768,1695,7701,7242,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7701,7242,- -32768,821,1113,- -32768,4651,4651,615,- -32768,4651,4651,616,4651,617,7130,- -32768,1107,- 4,825,- -32768,- -32768,2728,1123,1111,7242,1126,370,- -32768,- -32768,1129,- -32768,- -32768,- -32768,- -32768,1040,- -32768,1134,- -32768,1040,- -32768,- -32768,- -32768,424,1125,668,- -32768,- -32768,- -32768,1128,1127,1135,7701,821,- -32768,761,381,716,732,732,280,132,132,691,691,691,691,138,138,72,72,- -32768,- -32768,- -32768,397,- -32768,- -32768,- -32768,- -32768,4651,- -32768,- -32768,4651,- -32768,4651,4651,625,- -32768,- -32768,- -32768,1137,757,1131,4495,628,- -32768,1139,1129,- -32768,- -32768,- -32768,1136,- -32768,- -32768,- -32768,- -32768,424,- -32768,1000,204,- -32768,- -32768,7242,1695,- -32768,- -32768,- -32768,- -32768,4651,- -32768,- -32768,- -32768,1140,- -32768,1159,- -32768,- -32768,- -32768,422,- -32768,821,- -32768,- -32768,1141,- -32768,- -32768,- -32768,- -32768,1276,1277,- -32768};
# 2048
static short Cyc_yypgoto[162U]={- -32768,119,- -32768,- -32768,- -32768,- -32768,- -32768,176,- -32768,- -32768,242,- -32768,- -32768,- 237,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- 67,- 117,- 14,- -32768,- -32768,0,624,- -32768,235,- 182,1161,33,- -32768,- -32768,- 139,- -32768,30,1244,- 748,- -32768,- -32768,- -32768,1009,1007,659,355,- -32768,- -32768,614,- 211,- 231,- -32768,- -32768,92,- -32768,- -32768,121,- 154,1216,- 401,455,- -32768,1138,- -32768,- -32768,1241,- 452,- -32768,572,- 114,- 150,- 143,- 425,282,578,586,- 434,- 502,- 123,- 393,- 135,- -32768,- 270,- 168,- 576,- 247,- -32768,870,- 155,- 24,- 147,- 210,- 306,146,- -32768,- -32768,- 56,- 278,- -32768,- 423,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- 23,1062,- -32768,675,860,- -32768,369,567,- -32768,- 170,- 298,- 165,- 363,- 361,- 353,930,- 351,- 345,- 279,- 321,- 317,- 197,695,- -32768,- -32768,- -32768,- -32768,367,- -32768,- 907,292,- -32768,507,954,216,- -32768,- 383,- -32768,- 12,529,- 35,- 61,- 72,- 54};
# 2072
static short Cyc_yytable[7950U]={64,369,267,104,374,375,150,83,493,147,348,87,327,351,105,591,352,153,336,593,329,64,279,338,410,64,528,529,594,531,652,614,596,67,1024,146,460,342,710,597,598,326,752,497,371,471,260,880,122,140,141,142,260,143,67,256,481,64,67,152,257,539,453,64,64,64,227,64,258,603,604,417,150,64,157,605,606,105,235,666,534,146,305,280,316,248,249,450,81,105,67,667,584,682,683,317,67,67,67,677,67,718,418,571,462,666,67,599,600,601,602,714,478,64,64,42,524,954,940,1095,19,20,540,717,525,152,231,64,64,64,623,64,64,64,64,306,762,152,228,64,472,941,701,378,463,707,67,67,978,64,41,64,108,647,667,464,678,229,157,44,67,67,67,- 558,67,67,67,67,848,123,111,560,67,462,647,541,130,285,287,411,412,413,67,139,67,548,1121,379,563,112,564,565,979,766,46,466,376,607,608,609,559,394,154,674,155,420,580,86,750,535,156,440,316,463,537,- 168,105,440,764,105,395,396,105,496,414,386,387,325,105,415,416,483,253,254,940,65,- 233,305,305,- 233,305,109,228,263,452,41,269,270,271,483,272,273,274,275,44,795,65,1022,560,672,65,229,47,388,389,152,48,110,536,445,453,453,453,318,51,52,994,454,42,64,376,392,113,87,558,393,470,986,987,748,306,306,65,306,334,995,996,64,65,65,65,729,65,305,64,64,802,64,65,555,114,41,67,625,541,858,257,829,117,862,44,427,428,660,258,988,989,739,412,413,67,560,48,712,873,557,115,67,67,612,67,421,720,615,41,861,660,118,65,65,865,464,43,44,45,622,306,422,533,871,119,376,65,65,65,500,65,65,65,65,64,571,414,41,65,384,385,740,416,47,794,43,44,45,65,450,65,124,745,120,921,51,52,923,477,541,41,127,318,41,458,928,228,67,267,44,624,43,44,45,84,952,239,704,630,- 255,47,105,- 255,240,477,229,958,636,816,105,458,530,51,52,46,46,125,105,929,257,1016,516,648,662,649,663,661,258,523,483,650,664,41,618,695,761,452,452,452,206,128,44,834,702,464,651,970,150,753,483,754,233,170,511,751,483,512,924,705,286,930,659,129,665,931,835,464,450,255,454,454,454,376,64,376,64,992,1026,377,879,993,571,1093,1018,673,768,941,152,825,780,770,1022,560,376,325,706,152,547,65,782,152,64,1108,780,784,376,67,170,67,376,64,438,785,79,64,159,65,- 15,1109,769,41,680,681,65,65,376,65,172,43,44,45,469,1045,783,67,85,856,250,1050,106,1131,107,251,67,931,541,47,67,818,933,457,840,372,176,841,230,458,826,51,52,1071,79,850,484,714,376,232,834,236,485,79,586,41,721,1085,79,633,234,927,376,43,44,45,145,698,148,144,376,79,65,835,444,670,759,671,457,237,175,252,1051,105,458,106,1053,105,131,132,560,661,948,1107,1013,1054,1055,106,999,1056,798,238,305,799,316,483,1057,1058,483,79,79,376,145,949,651,817,950,735,79,831,16,17,18,79,79,79,956,79,79,79,79,261,522,665,842,843,1063,1064,264,703,376,376,1065,1066,834,276,800,801,79,277,376,819,775,1127,262,306,806,152,812,376,976,376,133,134,851,864,1049,835,459,64,1132,64,175,319,1059,1060,1061,1062,998,372,332,859,372,376,915,1029,1030,376,480,1000,331,65,1034,65,577,869,333,376,397,376,339,975,67,1002,67,1006,506,1031,484,341,376,376,376,1035,1042,592,1075,1078,1080,65,376,721,560,376,106,968,1114,106,65,1119,106,918,65,345,376,340,106,398,399,400,401,402,403,404,405,406,407,907,1019,390,391,266,616,1015,1011,64,1038,376,105,1039,1101,353,482,1102,1067,1068,1069,1096,408,354,284,1098,355,79,639,640,641,550,551,356,482,266,508,357,1046,513,1047,358,67,682,683,366,160,161,162,163,164,359,1126,165,166,167,79,727,728,703,360,41,146,305,803,804,495,852,853,43,44,45,1106,946,947,361,982,983,47,380,1017,381,500,362,1086,541,501,984,985,175,51,52,363,344,373,347,349,349,382,64,687,688,990,991,691,1116,376,696,383,364,365,882,883,419,349,423,306,349,349,437,554,443,447,451,1041,456,467,468,875,475,64,305,67,459,476,486,507,349,495,488,490,572,573,574,575,576,498,509,579,510,146,582,518,480,519,1118,514,517,65,1125,65,587,284,67,590,526,520,884,527,278,532,537,553,911,464,46,506,562,544,106,378,549,41,545,306,610,578,106,583,617,43,44,45,46,88,106,585,611,64,47,619,620,587,500,621,627,637,41,628,631,629,51,52,347,632,43,44,45,965,635,638,188,653,654,47,146,655,482,500,656,482,67,657,658,668,675,51,52,692,79,676,79,689,773,65,690,646,693,697,699,482,708,677,726,831,715,482,349,711,719,723,724,1041,305,725,41,91,195,196,92,93,94,- 558,730,44,731,732,733,734,736,1001,742,743,1005,809,811,346,760,146,200,737,738,741,746,349,749,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,587,41,587,306,747,755,763,765,767,43,44,45,774,778,781,789,960,64,961,788,814,65,506,1043,266,793,796,807,813,822,815,349,325,820,744,824,506,838,41,506,832,854,844,855,266,41,495,44,846,46,67,266,65,857,44,47,1073,1074,135,48,1076,1077,47,1079,860,106,48,51,52,106,866,867,863,868,51,52,874,870,872,876,877,885,88,881,912,914,886,917,909,920,910,922,925,495,266,926,482,955,934,935,936,937,938,953,349,266,939,959,945,932,482,951,964,482,266,957,966,967,969,971,65,974,797,980,977,772,997,981,1012,1110,1014,777,1111,1020,1112,1113,1021,1032,89,787,887,888,1033,1036,1037,1044,1072,1048,1091,1083,424,940,1089,425,91,1097,88,92,93,94,1100,426,44,1103,587,1088,233,96,1128,1104,97,1105,1115,1117,98,1120,99,100,1122,427,428,1130,1129,1133,1135,1136,1123,101,349,1099,779,116,1004,265,492,1008,1010,499,792,158,126,828,827,495,1084,821,836,837,626,839,446,757,89,634,88,337,266,1027,908,495,595,0,849,1094,581,79,451,226,91,1124,266,92,93,94,0,95,44,0,0,0,106,96,0,0,97,0,0,0,889,372,99,100,65,0,0,0,1052,0,890,0,0,101,203,0,88,204,205,0,0,0,89,0,0,0,1070,0,0,0,0,0,0,349,756,0,0,425,91,0,1082,92,93,94,0,426,44,1087,0,0,1090,96,0,0,97,0,0,0,98,0,99,100,495,427,428,145,0,79,0,0,0,101,89,0,0,0,0,0,0,0,495,0,0,0,439,0,0,226,91,0,349,92,93,94,0,95,44,0,0,0,0,96,0,349,97,0,0,0,98,0,99,100,0,0,284,0,0,0,0,0,0,101,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,88,0,0,0,0,266,0,349,0,0,0,0,145,0,22,23,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,495,0,0,0,0,31,32,0,0,0,284,0,0,0,0,0,0,38,0,88,0,0,0,89,0,0,266,0,0,39,0,40,0,0,0,758,0,0,226,91,0,0,92,93,94,0,95,44,0,0,0,0,96,323,145,97,41,0,0,98,0,99,100,42,43,44,45,46,973,325,0,349,101,47,0,89,0,477,0,49,50,0,0,458,0,51,52,0,0,349,90,91,0,0,92,93,94,0,95,44,0,0,0,0,96,0,349,97,0,0,0,98,0,99,100,0,0,145,0,0,0,0,0,0,101,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,349,0,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,349,0,0,0,0,0,0,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,803,804,21,177,178,288,0,289,290,291,292,293,294,295,296,22,23,24,297,88,26,180,298,0,0,0,349,181,27,299,0,0,30,182,183,184,185,186,0,31,32,33,187,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,349,0,0,0,300,91,195,196,92,93,94,42,43,44,45,46,197,301,149,0,0,198,0,0,0,199,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,288,0,289,290,291,292,293,294,295,296,22,23,24,297,88,26,180,298,0,0,0,0,181,27,299,0,0,30,182,183,184,185,186,0,31,32,33,187,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,300,91,195,196,92,93,94,42,43,44,45,46,197,301,149,302,0,198,0,0,0,199,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,288,0,289,290,291,292,293,294,295,296,22,23,24,297,88,26,180,298,0,0,0,0,181,27,299,0,0,30,182,183,184,185,186,0,31,32,33,187,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,300,91,195,196,92,93,94,42,43,44,45,46,197,301,149,0,0,198,0,0,0,199,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,288,0,289,290,291,292,293,294,295,296,22,23,24,297,88,26,180,298,0,0,0,0,181,27,299,0,0,30,182,183,184,185,186,0,31,32,33,187,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,515,91,195,196,92,93,94,42,43,44,45,46,197,301,149,0,0,198,0,0,0,199,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,297,88,26,180,0,0,0,0,0,181,27,0,0,0,30,182,183,184,185,186,0,31,32,33,187,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,694,0,0,0,198,0,0,0,199,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,297,88,0,0,0,0,0,0,0,0,27,0,0,0,30,182,183,184,185,186,0,31,32,0,0,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,0,0,0,0,198,0,0,0,346,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,179,88,0,180,0,0,0,0,0,181,0,0,0,0,30,182,183,184,185,186,0,31,32,0,187,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,0,370,0,0,198,0,0,0,199,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,179,88,0,180,0,0,0,0,0,181,0,0,0,0,30,182,183,184,185,186,0,31,32,0,187,0,0,0,188,0,0,189,190,38,191,0,0,0,0,0,0,0,0,0,0,39,192,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,0,0,0,0,198,0,0,0,199,0,49,303,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205,- 12,1,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,88,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,28,29,30,0,0,0,0,0,0,31,32,33,0,0,34,35,0,36,37,0,0,38,0,0,0,0,0,89,0,0,0,0,0,39,0,40,0,0,0,0,0,0,226,91,0,0,92,93,94,0,95,44,0,0,0,0,96,0,0,97,41,0,0,98,0,99,100,42,43,44,45,46,0,0,0,- 12,101,47,0,0,0,48,0,49,50,0,0,0,0,51,52,- 12,1,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,28,29,30,0,0,0,0,0,0,31,32,33,0,0,34,35,0,36,37,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,0,0,0,0,0,0,42,43,44,45,46,0,0,0,0,0,47,0,0,0,48,0,49,50,0,0,0,0,51,52,1,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,28,29,30,0,0,0,0,0,0,31,32,33,0,0,34,35,0,36,37,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,0,0,0,0,0,0,42,43,44,45,46,0,0,0,- 12,0,47,0,0,0,48,0,49,50,0,0,0,0,51,52,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,323,0,0,0,0,0,0,0,0,0,42,43,0,45,46,0,325,0,0,0,47,0,0,0,457,0,49,50,0,0,458,0,51,52,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,0,0,0,0,0,0,42,43,44,45,46,0,0,0,0,0,47,0,0,0,500,0,49,50,0,0,0,0,51,52,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,0,0,30,0,0,0,0,0,0,31,32,33,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,42,43,0,45,46,0,0,149,0,0,0,278,0,0,0,0,49,50,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,0,0,30,0,0,0,0,0,0,31,32,33,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,42,43,0,45,46,0,0,149,22,23,24,25,0,0,0,0,49,50,0,0,0,27,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,42,43,0,45,46,22,23,24,0,0,0,0,0,0,0,0,49,50,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,323,0,0,324,0,0,0,0,0,0,42,43,0,45,46,0,325,0,0,0,0,0,0,0,0,0,49,50,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,241,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,42,43,0,45,46,0,0,242,22,23,24,0,0,0,0,0,49,50,0,0,0,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,323,0,0,0,0,0,0,0,0,0,42,43,0,45,46,0,325,0,0,0,0,0,0,0,0,0,49,50,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,42,43,0,45,46,22,23,24,448,0,0,0,0,0,0,0,49,50,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,42,43,0,45,46,22,23,24,709,0,0,0,0,0,0,0,49,50,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,716,0,0,0,0,0,0,0,0,0,42,43,0,45,46,22,23,24,0,0,0,0,0,0,0,0,49,50,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,42,43,0,45,46,22,23,24,0,0,0,0,0,0,0,0,49,50,0,0,0,0,30,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,39,0,40,0,0,0,0,0,0,177,178,288,0,289,290,291,292,293,294,295,296,0,0,0,179,88,0,180,298,0,0,0,0,181,42,299,0,0,46,182,183,184,185,186,0,0,0,0,187,0,49,50,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,515,91,195,196,92,93,94,0,0,44,0,0,197,301,149,0,0,198,0,0,0,199,0,0,200,177,178,0,0,0,0,201,566,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,567,91,195,196,92,93,94,0,258,44,0,0,197,0,350,568,0,198,0,0,0,199,0,0,200,0,427,428,177,178,0,201,0,0,202,203,0,0,204,205,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,567,91,195,196,92,93,94,0,258,44,0,0,197,0,350,833,0,198,0,0,0,199,0,0,200,0,427,428,177,178,0,201,0,0,202,203,0,0,204,205,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,567,91,195,196,92,93,94,0,258,44,0,0,197,0,350,847,0,198,0,0,0,199,0,0,200,0,427,428,177,178,0,201,0,0,202,203,0,0,204,205,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,567,91,195,196,92,93,94,0,258,44,0,0,197,0,350,1028,0,198,0,0,0,199,0,0,200,0,427,428,177,178,0,201,0,0,202,203,0,0,204,205,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,567,91,195,196,92,93,94,0,258,44,0,0,197,0,350,0,0,198,0,0,0,199,0,0,200,0,427,428,177,178,0,201,0,0,202,203,0,0,204,205,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,200,177,178,0,330,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,350,0,0,198,0,0,0,199,0,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,521,0,0,0,198,0,0,0,199,0,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,613,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,200,177,178,0,645,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,700,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,200,177,178,0,771,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,200,177,178,0,786,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,808,0,0,0,198,0,0,0,199,0,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,810,0,0,0,198,0,0,0,199,0,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,913,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,916,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,919,0,0,0,198,0,0,0,199,0,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,1003,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,1007,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,1009,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,1081,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,200,177,178,0,0,0,0,201,0,0,202,203,0,0,204,205,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,0,0,0,0,187,0,0,0,188,0,0,189,190,0,191,0,177,178,0,0,0,0,0,0,0,0,192,0,0,193,194,179,88,0,0,0,0,0,0,0,0,0,0,0,0,0,182,183,184,185,186,0,776,91,195,196,92,93,94,188,0,44,189,190,197,191,0,0,0,198,0,0,0,199,0,0,200,192,0,0,193,194,0,201,0,0,202,203,0,0,204,205,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,177,178,197,0,350,0,0,198,0,0,0,199,0,0,200,179,88,0,0,0,0,201,0,0,202,203,0,0,204,205,182,183,184,185,186,0,0,0,0,0,0,0,0,188,0,0,189,190,0,191,0,177,178,0,0,0,0,0,0,0,0,192,0,0,193,194,179,88,0,0,0,0,0,0,0,0,0,0,0,0,0,182,183,184,185,186,0,41,91,195,196,92,93,94,188,0,44,189,190,197,191,744,0,0,198,0,0,0,199,0,0,200,192,0,0,193,194,0,201,0,0,202,203,0,0,204,205,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,177,178,197,0,0,0,0,198,0,0,0,343,0,0,200,179,88,0,0,0,0,201,0,0,202,203,0,0,204,205,182,183,184,185,186,0,0,0,0,0,0,0,0,188,0,0,189,190,0,191,0,177,178,0,0,0,0,0,0,0,0,192,0,0,193,194,179,88,0,0,0,0,0,0,0,0,0,0,0,0,0,182,183,184,185,186,0,41,91,195,196,92,93,94,188,0,44,189,190,197,191,0,0,0,198,0,0,0,346,0,0,200,192,0,0,193,194,0,201,0,0,202,203,0,0,204,205,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,177,178,197,0,0,0,0,198,0,0,0,199,0,0,200,179,88,0,0,0,0,201,0,0,202,203,0,0,204,205,182,183,184,185,186,0,0,0,0,0,0,0,0,188,0,0,189,190,0,191,0,0,0,0,0,0,0,0,0,0,0,192,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,972,0,0,200,0,0,0,0,0,0,201,0,0,202,203,0,0,204,205};
# 2870
static short Cyc_yycheck[7950U]={0,198,125,26,201,202,73,21,278,70,180,25,155,181,26,378,181,73,168,380,155,21,139,170,221,25,304,305,381,307,464,414,383,0,941,70,246,176,540,384,385,155,618,280,199,17,118,795,48,63,64,65,124,67,21,116,266,57,25,73,112,65,244,63,64,65,89,67,120,390,391,110,139,73,74,392,393,89,101,480,317,116,149,139,151,109,110,242,114,101,57,484,370,5,6,151,63,64,65,80,67,553,141,350,92,506,73,386,387,388,389,545,266,113,114,119,125,865,114,1026,20,21,126,548,133,139,96,127,128,129,428,131,132,133,134,149,638,151,112,139,112,137,525,98,132,538,113,114,98,149,112,151,112,459,547,143,137,131,158,121,127,128,129,137,131,132,133,134,744,48,112,341,139,92,480,325,57,147,148,88,89,90,149,62,151,328,1093,142,343,131,345,346,142,645,123,249,128,394,395,396,339,129,131,501,133,228,361,133,140,128,139,234,279,132,112,127,228,240,643,231,148,149,234,279,133,93,94,125,240,138,139,266,113,114,114,0,134,304,305,137,307,131,112,122,244,112,127,128,129,284,131,132,133,134,121,680,21,137,428,496,25,131,129,131,132,279,133,131,322,239,452,453,454,152,141,142,129,244,119,279,128,143,125,297,338,147,256,93,94,137,304,305,57,307,135,148,149,297,63,64,65,571,67,370,304,305,689,307,73,333,125,112,279,130,464,767,112,113,54,771,121,138,139,477,120,131,132,88,89,90,297,501,133,544,786,335,133,304,305,411,307,119,134,415,112,770,500,111,113,114,775,143,120,121,122,427,370,135,125,784,133,128,127,128,129,133,131,132,133,134,370,618,133,112,139,95,96,138,139,129,678,120,121,122,149,540,151,111,585,126,813,141,142,818,133,545,112,125,277,112,139,826,112,370,527,121,429,120,121,122,126,863,126,530,437,125,129,429,128,133,133,131,874,447,702,437,139,306,141,142,123,123,133,445,827,112,113,291,131,131,133,133,477,120,298,480,139,139,112,419,517,134,452,453,454,86,125,121,728,526,143,461,885,530,619,500,621,98,131,133,617,506,136,134,530,138,134,477,126,479,138,728,143,638,115,452,453,454,128,489,128,491,143,128,134,793,147,744,128,137,500,648,137,517,715,659,649,137,678,128,125,535,526,128,279,662,530,517,137,673,663,128,489,131,491,128,526,134,663,0,530,76,297,126,137,649,112,507,508,304,305,128,307,114,120,121,122,134,971,663,517,22,763,125,977,26,134,28,130,526,138,711,129,530,707,835,133,735,199,126,735,131,139,716,141,142,999,48,748,128,1014,128,112,853,125,134,57,134,112,556,1015,62,125,133,825,128,120,121,122,70,125,72,126,128,75,370,853,238,489,632,491,133,128,84,119,978,628,139,89,980,632,125,126,793,659,856,1049,925,981,982,101,909,983,125,130,702,128,704,673,984,985,676,113,114,128,116,857,647,704,860,134,122,724,17,18,19,127,128,129,870,131,132,133,134,133,296,666,739,740,990,991,134,526,128,128,992,993,947,125,134,134,152,128,128,708,655,1109,120,702,134,704,125,128,890,128,125,126,751,134,977,947,246,702,1126,704,176,125,986,987,988,989,908,343,133,768,346,128,125,949,950,128,266,134,156,489,956,491,358,782,133,128,60,128,128,889,702,134,704,134,284,951,128,130,128,128,128,957,134,379,134,134,134,517,128,721,925,128,228,881,134,231,526,134,234,125,530,133,128,127,240,99,100,101,102,103,104,105,106,107,108,803,936,91,92,125,416,929,125,788,125,128,803,128,125,133,266,128,994,995,996,1031,130,133,144,1035,133,277,452,453,454,83,84,133,284,155,286,133,972,289,974,133,788,5,6,18,75,76,77,78,79,133,1109,82,83,84,306,127,128,692,133,112,876,909,22,23,278,127,128,120,121,122,1048,127,128,133,145,146,129,97,931,144,133,133,1018,1014,137,95,96,339,141,142,133,177,133,179,180,181,145,878,510,511,91,92,514,127,128,517,146,193,194,798,799,126,198,119,909,201,202,112,332,112,133,243,964,245,112,132,788,127,909,977,878,457,134,127,126,221,350,127,127,353,354,355,356,357,127,126,360,133,964,363,112,477,125,1089,133,133,702,1108,704,373,282,909,376,137,125,800,133,130,127,112,140,806,143,123,500,127,134,429,98,134,112,137,977,397,112,437,133,131,120,121,122,123,41,445,134,409,977,129,132,132,414,133,132,127,127,112,128,134,137,141,142,297,128,120,121,122,878,134,128,68,133,112,129,1039,126,477,133,133,480,977,134,128,47,125,141,142,137,489,128,491,133,653,788,126,458,30,125,45,500,133,80,112,1105,134,506,341,143,134,134,128,1102,1109,134,112,113,114,115,116,117,118,137,134,121,128,128,128,128,134,913,114,127,916,694,695,133,127,1102,136,134,134,134,134,378,134,380,381,382,383,384,385,386,387,388,389,390,391,392,393,394,395,396,525,112,527,1109,128,140,134,134,140,120,121,122,125,134,134,46,127,1109,129,126,125,878,659,969,457,133,127,133,126,81,134,428,125,127,126,132,673,25,112,676,131,112,137,112,477,112,571,121,134,123,1109,484,909,134,121,129,1002,1003,125,133,1006,1007,129,1009,134,628,133,141,142,632,128,134,140,134,141,142,140,134,134,126,48,126,41,133,807,808,134,810,137,812,127,125,133,618,527,119,659,113,134,134,133,128,134,125,501,538,134,127,134,832,673,134,126,676,547,134,134,127,28,126,977,133,685,97,137,650,97,144,127,1075,128,656,1078,25,1080,1081,133,127,97,664,25,26,128,127,127,127,125,134,114,134,109,114,133,112,113,113,41,116,117,118,127,120,121,127,689,134,889,126,1114,134,129,128,127,134,133,128,135,136,134,138,139,114,134,134,0,0,1102,146,585,1039,658,39,915,124,277,918,919,282,676,75,51,721,716,728,1014,711,731,732,430,734,240,628,97,445,41,169,649,942,803,744,382,- 1,747,1023,362,788,659,112,113,1105,663,116,117,118,- 1,120,121,- 1,- 1,- 1,803,126,- 1,- 1,129,- 1,- 1,- 1,133,972,135,136,1109,- 1,- 1,- 1,979,- 1,143,- 1,- 1,146,147,- 1,41,150,151,- 1,- 1,- 1,97,- 1,- 1,- 1,997,- 1,- 1,- 1,- 1,- 1,- 1,678,109,- 1,- 1,112,113,- 1,1011,116,117,118,- 1,120,121,1018,- 1,- 1,1021,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,835,138,139,876,- 1,878,- 1,- 1,- 1,146,97,- 1,- 1,- 1,- 1,- 1,- 1,- 1,853,- 1,- 1,- 1,109,- 1,- 1,112,113,- 1,735,116,117,118,- 1,120,121,- 1,- 1,- 1,- 1,126,- 1,748,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,795,- 1,- 1,- 1,- 1,- 1,- 1,146,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,41,- 1,- 1,- 1,- 1,827,- 1,793,- 1,- 1,- 1,- 1,964,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,947,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,865,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,41,- 1,- 1,- 1,97,- 1,- 1,881,- 1,- 1,85,- 1,87,- 1,- 1,- 1,109,- 1,- 1,112,113,- 1,- 1,116,117,118,- 1,120,121,- 1,- 1,- 1,- 1,126,109,1039,129,112,- 1,- 1,133,- 1,135,136,119,120,121,122,123,887,125,- 1,890,146,129,- 1,97,- 1,133,- 1,135,136,- 1,- 1,139,- 1,141,142,- 1,- 1,908,112,113,- 1,- 1,116,117,118,- 1,120,121,- 1,- 1,- 1,- 1,126,- 1,925,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,1102,- 1,- 1,- 1,- 1,- 1,- 1,146,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,978,- 1,980,981,982,983,984,985,986,987,988,989,990,991,992,993,994,995,996,- 1,- 1,- 1,- 1,- 1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,- 1,- 1,- 1,1048,49,50,51,- 1,- 1,54,55,56,57,58,59,- 1,61,62,63,64,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,1108,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,- 1,- 1,- 1,- 1,49,50,51,- 1,- 1,54,55,56,57,58,59,- 1,61,62,63,64,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,- 1,- 1,- 1,- 1,49,50,51,- 1,- 1,54,55,56,57,58,59,- 1,61,62,63,64,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,- 1,- 1,- 1,- 1,49,50,51,- 1,- 1,54,55,56,57,58,59,- 1,61,62,63,64,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,42,43,- 1,- 1,- 1,- 1,- 1,49,50,- 1,- 1,- 1,54,55,56,57,58,59,- 1,61,62,63,64,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,125,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,- 1,- 1,54,55,56,57,58,59,- 1,61,62,- 1,- 1,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,54,55,56,57,58,59,- 1,61,62,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,- 1,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,54,55,56,57,58,59,- 1,61,62,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,73,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,86,87,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,119,120,121,122,123,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,0,1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,41,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,63,- 1,- 1,66,67,- 1,69,70,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,97,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,112,113,- 1,- 1,116,117,118,- 1,120,121,- 1,- 1,- 1,- 1,126,- 1,- 1,129,112,- 1,- 1,133,- 1,135,136,119,120,121,122,123,- 1,- 1,- 1,127,146,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,141,142,0,1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,63,- 1,- 1,66,67,- 1,69,70,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,- 1,- 1,- 1,- 1,- 1,- 1,119,120,121,122,123,- 1,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,141,142,1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,63,- 1,- 1,66,67,- 1,69,70,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,- 1,- 1,- 1,- 1,- 1,- 1,119,120,121,122,123,- 1,- 1,- 1,127,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,141,142,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,109,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,- 1,125,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,139,- 1,141,142,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,- 1,- 1,- 1,- 1,- 1,- 1,119,120,121,122,123,- 1,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,135,136,- 1,- 1,- 1,- 1,141,142,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,- 1,- 1,126,- 1,- 1,- 1,130,- 1,- 1,- 1,- 1,135,136,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,119,120,- 1,122,123,- 1,- 1,126,37,38,39,40,- 1,- 1,- 1,- 1,135,136,- 1,- 1,- 1,50,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,135,136,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,109,- 1,- 1,112,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,- 1,125,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,135,136,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,65,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,- 1,- 1,126,37,38,39,- 1,- 1,- 1,- 1,- 1,135,136,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,109,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,- 1,125,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,135,136,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,37,38,39,127,- 1,- 1,- 1,- 1,- 1,- 1,- 1,135,136,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,37,38,39,127,- 1,- 1,- 1,- 1,- 1,- 1,- 1,135,136,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,109,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,135,136,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,119,120,- 1,122,123,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,135,136,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,61,62,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,73,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,85,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,25,26,27,- 1,29,30,31,32,33,34,35,36,- 1,- 1,- 1,40,41,- 1,43,44,- 1,- 1,- 1,- 1,49,119,51,- 1,- 1,123,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,135,136,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,125,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,- 1,- 1,- 1,143,32,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,120,121,- 1,- 1,124,- 1,126,127,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,- 1,138,139,25,26,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,120,121,- 1,- 1,124,- 1,126,127,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,- 1,138,139,25,26,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,120,121,- 1,- 1,124,- 1,126,127,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,- 1,138,139,25,26,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,120,121,- 1,- 1,124,- 1,126,127,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,- 1,138,139,25,26,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,120,121,- 1,- 1,124,- 1,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,- 1,138,139,25,26,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,140,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,125,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,140,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,140,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,140,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,125,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,125,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,125,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,134,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,25,26,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,- 1,- 1,- 1,64,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,112,113,114,115,116,117,118,68,- 1,121,71,72,124,74,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,86,- 1,- 1,89,90,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,25,26,124,- 1,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,40,41,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,55,56,57,58,59,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,112,113,114,115,116,117,118,68,- 1,121,71,72,124,74,126,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,86,- 1,- 1,89,90,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,25,26,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,40,41,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,55,56,57,58,59,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,- 1,112,113,114,115,116,117,118,68,- 1,121,71,72,124,74,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,86,- 1,- 1,89,90,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,25,26,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,40,41,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151,55,56,57,58,59,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,68,- 1,- 1,71,72,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,86,- 1,- 1,89,90,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,112,113,114,115,116,117,118,- 1,- 1,121,- 1,- 1,124,- 1,- 1,- 1,- 1,129,- 1,- 1,- 1,133,- 1,- 1,136,- 1,- 1,- 1,- 1,- 1,- 1,143,- 1,- 1,146,147,- 1,- 1,150,151};char Cyc_Yystack_overflow[17U]="Yystack_overflow";struct Cyc_Yystack_overflow_exn_struct{char*tag;int f1;};
# 45 "cycbison.simple"
struct Cyc_Yystack_overflow_exn_struct Cyc_Yystack_overflow_val={Cyc_Yystack_overflow,0};
# 72 "cycbison.simple"
extern void Cyc_yyerror(struct _fat_ptr,int state,int token);
# 82 "cycbison.simple"
extern int Cyc_yylex(struct Cyc_Lexing_lexbuf*,union Cyc_YYSTYPE*yylval_ptr,struct Cyc_Yyltype*yylloc);struct Cyc_Yystacktype{union Cyc_YYSTYPE v;struct Cyc_Yyltype l;};struct _tuple32{unsigned f1;struct _tuple0*f2;int f3;};struct _tuple33{struct _fat_ptr f1;void*f2;};static char _tmp429[8U]="stdcall";static char _tmp42A[6U]="cdecl";static char _tmp42B[9U]="fastcall";static char _tmp42C[9U]="noreturn";static char _tmp42D[6U]="const";static char _tmp42E[8U]="aligned";static char _tmp42F[7U]="packed";static char _tmp430[7U]="shared";static char _tmp431[7U]="unused";static char _tmp432[5U]="weak";static char _tmp433[10U]="dllimport";static char _tmp434[10U]="dllexport";static char _tmp435[23U]="no_instrument_function";static char _tmp436[12U]="constructor";static char _tmp437[11U]="destructor";static char _tmp438[22U]="no_check_memory_usage";static char _tmp439[5U]="pure";static char _tmp43A[14U]="always_inline";struct _tuple34{void*f1;void*f2;};struct _tuple35{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 145 "cycbison.simple"
int Cyc_yyparse(struct _RegionHandle*yyr,struct Cyc_Lexing_lexbuf*yylex_buf){
# 148
struct _RegionHandle _tmp39D=_new_region("yyregion");struct _RegionHandle*yyregion=& _tmp39D;_push_region(yyregion);
{int yystate;
int yyn=0;
int yyerrstatus;
int yychar1=0;
# 154
int yychar;
union Cyc_YYSTYPE yylval=({union Cyc_YYSTYPE _tmp773;(_tmp773.YYINITIALSVAL).tag=72U,(_tmp773.YYINITIALSVAL).val=0;_tmp773;});
int yynerrs;
# 158
struct Cyc_Yyltype yylloc;
# 162
int yyssp_offset;
# 164
struct _fat_ptr yyss=({unsigned _tmp6B5=200U;_tag_fat(_region_calloc(yyregion,sizeof(short),_tmp6B5),sizeof(short),_tmp6B5);});
# 166
int yyvsp_offset;
# 168
struct _fat_ptr yyvs=
_tag_fat(({unsigned _tmp6B4=200U;struct Cyc_Yystacktype*_tmp6B3=({struct _RegionHandle*_tmp82E=yyregion;_region_malloc(_tmp82E,_check_times(_tmp6B4,sizeof(struct Cyc_Yystacktype)));});({{unsigned _tmp772=200U;unsigned i;for(i=0;i < _tmp772;++ i){(_tmp6B3[i]).v=yylval,({struct Cyc_Yyltype _tmp82F=Cyc_yynewloc();(_tmp6B3[i]).l=_tmp82F;});}}0;});_tmp6B3;}),sizeof(struct Cyc_Yystacktype),200U);
# 174
struct Cyc_Yystacktype*yyyvsp;
# 177
int yystacksize=200;
# 179
union Cyc_YYSTYPE yyval=yylval;
# 183
int yylen;
# 190
yystate=0;
yyerrstatus=0;
yynerrs=0;
yychar=-2;
# 200
yyssp_offset=-1;
yyvsp_offset=0;
# 206
yynewstate:
# 208
*((short*)_check_fat_subscript(yyss,sizeof(short),++ yyssp_offset))=(short)yystate;
# 210
if(yyssp_offset >= (yystacksize - 1)- 12){
# 212
if(yystacksize >= 10000){
({struct _fat_ptr _tmp831=({const char*_tmp39E="parser stack overflow";_tag_fat(_tmp39E,sizeof(char),22U);});int _tmp830=yystate;Cyc_yyerror(_tmp831,_tmp830,yychar);});
(int)_throw((void*)& Cyc_Yystack_overflow_val);}
# 216
yystacksize *=2;
if(yystacksize > 10000)
yystacksize=10000;{
struct _fat_ptr yyss1=({unsigned _tmp3A2=(unsigned)yystacksize;short*_tmp3A1=({struct _RegionHandle*_tmp832=yyregion;_region_malloc(_tmp832,_check_times(_tmp3A2,sizeof(short)));});({{unsigned _tmp729=(unsigned)yystacksize;unsigned i;for(i=0;i < _tmp729;++ i){
i <= (unsigned)yyssp_offset?_tmp3A1[i]=*((short*)_check_fat_subscript(yyss,sizeof(short),(int)i)):(_tmp3A1[i]=0);}}0;});_tag_fat(_tmp3A1,sizeof(short),_tmp3A2);});
# 222
struct _fat_ptr yyvs1=({unsigned _tmp3A0=(unsigned)yystacksize;struct Cyc_Yystacktype*_tmp39F=({struct _RegionHandle*_tmp833=yyregion;_region_malloc(_tmp833,_check_times(_tmp3A0,sizeof(struct Cyc_Yystacktype)));});({{unsigned _tmp728=(unsigned)yystacksize;unsigned i;for(i=0;i < _tmp728;++ i){
# 224
i <= (unsigned)yyssp_offset?_tmp39F[i]=*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),(int)i)):(_tmp39F[i]=*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),0)));}}0;});_tag_fat(_tmp39F,sizeof(struct Cyc_Yystacktype),_tmp3A0);});
# 230
yyss=yyss1;
yyvs=yyvs1;}}
# 240
goto yybackup;
# 242
yybackup:
# 254 "cycbison.simple"
 yyn=(int)*((short*)_check_known_subscript_notnull(Cyc_yypact,1137U,sizeof(short),yystate));
if(yyn == -32768)goto yydefault;
# 261
if(yychar == -2)
# 267
yychar=Cyc_yylex(yylex_buf,& yylval,& yylloc);
# 271
if(yychar <= 0){
# 273
yychar1=0;
yychar=0;}else{
# 282
yychar1=yychar > 0 && yychar <= 379?(int)*((short*)_check_known_subscript_notnull(Cyc_yytranslate,380U,sizeof(short),yychar)): 314;}
# 299 "cycbison.simple"
yyn +=yychar1;
if((yyn < 0 || yyn > 7949)||(int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,7950U,sizeof(short),yyn))!= yychar1)goto yydefault;
# 302
yyn=(int)Cyc_yytable[yyn];
# 309
if(yyn < 0){
# 311
if(yyn == -32768)goto yyerrlab;
yyn=- yyn;
goto yyreduce;}else{
# 315
if(yyn == 0)goto yyerrlab;}
# 317
if(yyn == 1136){
int _tmp3A3=0;_npop_handler(0U);return _tmp3A3;}
# 328 "cycbison.simple"
if(yychar != 0)
yychar=-2;
# 332
({struct Cyc_Yystacktype _tmp834=({struct Cyc_Yystacktype _tmp72A;_tmp72A.v=yylval,_tmp72A.l=yylloc;_tmp72A;});*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),++ yyvsp_offset))=_tmp834;});
# 338
if(yyerrstatus != 0)-- yyerrstatus;
# 340
yystate=yyn;
goto yynewstate;
# 344
yydefault:
# 346
 yyn=(int)Cyc_yydefact[yystate];
if(yyn == 0)goto yyerrlab;
# 351
yyreduce:
# 353
 yylen=(int)*((short*)_check_known_subscript_notnull(Cyc_yyr2,562U,sizeof(short),yyn));
yyyvsp=(struct Cyc_Yystacktype*)_check_null(_untag_fat_ptr(_fat_ptr_plus(yyvs,sizeof(struct Cyc_Yystacktype),(yyvsp_offset + 1)- yylen),sizeof(struct Cyc_Yystacktype),12U));
if(yylen > 0)
yyval=(yyyvsp[0]).v;
# 370 "cycbison.simple"
{int _tmp3A4=yyn;switch(_tmp3A4){case 1U: _LL1: _LL2:
# 1214 "parse.y"
 yyval=(yyyvsp[0]).v;
Cyc_Parse_parse_result=Cyc_yyget_YY16(&(yyyvsp[0]).v);
# 1217
goto _LL0;case 2U: _LL3: _LL4:
# 1220 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp835=Cyc_yyget_YY16(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp835,Cyc_yyget_YY16(&(yyyvsp[1]).v));}));
goto _LL0;case 3U: _LL5: _LL6:
# 1224 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3A7=_cycalloc(sizeof(*_tmp3A7));({struct Cyc_Absyn_Decl*_tmp839=({struct Cyc_Absyn_Decl*_tmp3A6=_cycalloc(sizeof(*_tmp3A6));({void*_tmp838=(void*)({struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_tmp3A5=_cycalloc(sizeof(*_tmp3A5));_tmp3A5->tag=10U,({struct _tuple0*_tmp837=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3A5->f1=_tmp837;}),({struct Cyc_List_List*_tmp836=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3A5->f2=_tmp836;});_tmp3A5;});_tmp3A6->r=_tmp838;}),_tmp3A6->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3A6;});_tmp3A7->hd=_tmp839;}),_tmp3A7->tl=0;_tmp3A7;}));
Cyc_Lex_leave_using();
# 1227
goto _LL0;case 4U: _LL7: _LL8:
# 1228 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3AA=_cycalloc(sizeof(*_tmp3AA));({struct Cyc_Absyn_Decl*_tmp83E=({struct Cyc_Absyn_Decl*_tmp3A9=_cycalloc(sizeof(*_tmp3A9));({void*_tmp83D=(void*)({struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_tmp3A8=_cycalloc(sizeof(*_tmp3A8));_tmp3A8->tag=10U,({struct _tuple0*_tmp83C=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3A8->f1=_tmp83C;}),({struct Cyc_List_List*_tmp83B=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3A8->f2=_tmp83B;});_tmp3A8;});_tmp3A9->r=_tmp83D;}),_tmp3A9->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3A9;});_tmp3AA->hd=_tmp83E;}),({struct Cyc_List_List*_tmp83A=Cyc_yyget_YY16(&(yyyvsp[4]).v);_tmp3AA->tl=_tmp83A;});_tmp3AA;}));
goto _LL0;case 5U: _LL9: _LLA:
# 1231
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3AE=_cycalloc(sizeof(*_tmp3AE));({struct Cyc_Absyn_Decl*_tmp843=({struct Cyc_Absyn_Decl*_tmp3AD=_cycalloc(sizeof(*_tmp3AD));({void*_tmp842=(void*)({struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp3AC=_cycalloc(sizeof(*_tmp3AC));_tmp3AC->tag=9U,({struct _fat_ptr*_tmp841=({struct _fat_ptr*_tmp3AB=_cycalloc(sizeof(*_tmp3AB));({struct _fat_ptr _tmp840=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp3AB=_tmp840;});_tmp3AB;});_tmp3AC->f1=_tmp841;}),({struct Cyc_List_List*_tmp83F=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3AC->f2=_tmp83F;});_tmp3AC;});_tmp3AD->r=_tmp842;}),_tmp3AD->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3AD;});_tmp3AE->hd=_tmp843;}),_tmp3AE->tl=0;_tmp3AE;}));
Cyc_Lex_leave_namespace();
# 1234
goto _LL0;case 6U: _LLB: _LLC:
# 1235 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3B2=_cycalloc(sizeof(*_tmp3B2));({struct Cyc_Absyn_Decl*_tmp849=({struct Cyc_Absyn_Decl*_tmp3B1=_cycalloc(sizeof(*_tmp3B1));({void*_tmp848=(void*)({struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp3B0=_cycalloc(sizeof(*_tmp3B0));_tmp3B0->tag=9U,({struct _fat_ptr*_tmp847=({struct _fat_ptr*_tmp3AF=_cycalloc(sizeof(*_tmp3AF));({struct _fat_ptr _tmp846=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp3AF=_tmp846;});_tmp3AF;});_tmp3B0->f1=_tmp847;}),({struct Cyc_List_List*_tmp845=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3B0->f2=_tmp845;});_tmp3B0;});_tmp3B1->r=_tmp848;}),_tmp3B1->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3B1;});_tmp3B2->hd=_tmp849;}),({struct Cyc_List_List*_tmp844=Cyc_yyget_YY16(&(yyyvsp[4]).v);_tmp3B2->tl=_tmp844;});_tmp3B2;}));
goto _LL0;case 7U: _LLD: _LLE: {
# 1237 "parse.y"
int _tmp3B3=Cyc_yyget_YY31(&(yyyvsp[0]).v);int is_c_include=_tmp3B3;
struct Cyc_List_List*cycdecls=Cyc_yyget_YY16(&(yyyvsp[4]).v);
struct _tuple28*_tmp3B4=Cyc_yyget_YY53(&(yyyvsp[5]).v);struct _tuple28*_stmttmp18=_tmp3B4;struct _tuple28*_tmp3B5=_stmttmp18;unsigned _tmp3B7;struct Cyc_List_List*_tmp3B6;_LL462: _tmp3B6=_tmp3B5->f1;_tmp3B7=_tmp3B5->f2;_LL463: {struct Cyc_List_List*exs=_tmp3B6;unsigned wc=_tmp3B7;
struct Cyc_List_List*_tmp3B8=Cyc_yyget_YY54(&(yyyvsp[6]).v);struct Cyc_List_List*hides=_tmp3B8;
if(exs != 0 && hides != 0)
({void*_tmp3B9=0U;({unsigned _tmp84B=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp84A=({const char*_tmp3BA="hide list can only be used with export { * }, or no export block";_tag_fat(_tmp3BA,sizeof(char),65U);});Cyc_Warn_err(_tmp84B,_tmp84A,_tag_fat(_tmp3B9,sizeof(void*),0U));});});
# 1244
if((unsigned)hides && !((int)wc))
wc=(unsigned)((yyyvsp[6]).l).first_line;
# 1247
if(!is_c_include){
if(exs != 0 || cycdecls != 0){
({void*_tmp3BB=0U;({unsigned _tmp84D=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp84C=({const char*_tmp3BC="expecting \"C include\"";_tag_fat(_tmp3BC,sizeof(char),22U);});Cyc_Warn_err(_tmp84D,_tmp84C,_tag_fat(_tmp3BB,sizeof(void*),0U));});});
yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3C0=_cycalloc(sizeof(*_tmp3C0));({struct Cyc_Absyn_Decl*_tmp852=({struct Cyc_Absyn_Decl*_tmp3BF=_cycalloc(sizeof(*_tmp3BF));({void*_tmp851=(void*)({struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_tmp3BE=_cycalloc(sizeof(*_tmp3BE));_tmp3BE->tag=12U,({struct Cyc_List_List*_tmp850=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3BE->f1=_tmp850;}),_tmp3BE->f2=cycdecls,_tmp3BE->f3=exs,({struct _tuple10*_tmp84F=({struct _tuple10*_tmp3BD=_cycalloc(sizeof(*_tmp3BD));_tmp3BD->f1=wc,_tmp3BD->f2=hides;_tmp3BD;});_tmp3BE->f4=_tmp84F;});_tmp3BE;});_tmp3BF->r=_tmp851;}),_tmp3BF->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3BF;});_tmp3C0->hd=_tmp852;}),({struct Cyc_List_List*_tmp84E=Cyc_yyget_YY16(&(yyyvsp[7]).v);_tmp3C0->tl=_tmp84E;});_tmp3C0;}));}else{
# 1253
yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3C3=_cycalloc(sizeof(*_tmp3C3));({struct Cyc_Absyn_Decl*_tmp856=({struct Cyc_Absyn_Decl*_tmp3C2=_cycalloc(sizeof(*_tmp3C2));({void*_tmp855=(void*)({struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*_tmp3C1=_cycalloc(sizeof(*_tmp3C1));_tmp3C1->tag=11U,({struct Cyc_List_List*_tmp854=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3C1->f1=_tmp854;});_tmp3C1;});_tmp3C2->r=_tmp855;}),_tmp3C2->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3C2;});_tmp3C3->hd=_tmp856;}),({struct Cyc_List_List*_tmp853=Cyc_yyget_YY16(&(yyyvsp[7]).v);_tmp3C3->tl=_tmp853;});_tmp3C3;}));}}else{
# 1257
yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3C7=_cycalloc(sizeof(*_tmp3C7));({struct Cyc_Absyn_Decl*_tmp85B=({struct Cyc_Absyn_Decl*_tmp3C6=_cycalloc(sizeof(*_tmp3C6));({void*_tmp85A=(void*)({struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_tmp3C5=_cycalloc(sizeof(*_tmp3C5));_tmp3C5->tag=12U,({struct Cyc_List_List*_tmp859=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3C5->f1=_tmp859;}),_tmp3C5->f2=cycdecls,_tmp3C5->f3=exs,({struct _tuple10*_tmp858=({struct _tuple10*_tmp3C4=_cycalloc(sizeof(*_tmp3C4));_tmp3C4->f1=wc,_tmp3C4->f2=hides;_tmp3C4;});_tmp3C5->f4=_tmp858;});_tmp3C5;});_tmp3C6->r=_tmp85A;}),_tmp3C6->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3C6;});_tmp3C7->hd=_tmp85B;}),({struct Cyc_List_List*_tmp857=Cyc_yyget_YY16(&(yyyvsp[7]).v);_tmp3C7->tl=_tmp857;});_tmp3C7;}));}
# 1260
goto _LL0;}}case 8U: _LLF: _LL10:
# 1261 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3C9=_cycalloc(sizeof(*_tmp3C9));({struct Cyc_Absyn_Decl*_tmp85D=({struct Cyc_Absyn_Decl*_tmp3C8=_cycalloc(sizeof(*_tmp3C8));_tmp3C8->r=(void*)& Cyc_Absyn_Porton_d_val,_tmp3C8->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3C8;});_tmp3C9->hd=_tmp85D;}),({struct Cyc_List_List*_tmp85C=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3C9->tl=_tmp85C;});_tmp3C9;}));
goto _LL0;case 9U: _LL11: _LL12:
# 1263 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3CB=_cycalloc(sizeof(*_tmp3CB));({struct Cyc_Absyn_Decl*_tmp85F=({struct Cyc_Absyn_Decl*_tmp3CA=_cycalloc(sizeof(*_tmp3CA));_tmp3CA->r=(void*)& Cyc_Absyn_Portoff_d_val,_tmp3CA->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3CA;});_tmp3CB->hd=_tmp85F;}),({struct Cyc_List_List*_tmp85E=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3CB->tl=_tmp85E;});_tmp3CB;}));
goto _LL0;case 10U: _LL13: _LL14:
# 1265 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3CD=_cycalloc(sizeof(*_tmp3CD));({struct Cyc_Absyn_Decl*_tmp861=({struct Cyc_Absyn_Decl*_tmp3CC=_cycalloc(sizeof(*_tmp3CC));_tmp3CC->r=(void*)& Cyc_Absyn_Tempeston_d_val,_tmp3CC->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3CC;});_tmp3CD->hd=_tmp861;}),({struct Cyc_List_List*_tmp860=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3CD->tl=_tmp860;});_tmp3CD;}));
goto _LL0;case 11U: _LL15: _LL16:
# 1267 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3CF=_cycalloc(sizeof(*_tmp3CF));({struct Cyc_Absyn_Decl*_tmp863=({struct Cyc_Absyn_Decl*_tmp3CE=_cycalloc(sizeof(*_tmp3CE));_tmp3CE->r=(void*)& Cyc_Absyn_Tempestoff_d_val,_tmp3CE->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp3CE;});_tmp3CF->hd=_tmp863;}),({struct Cyc_List_List*_tmp862=Cyc_yyget_YY16(&(yyyvsp[2]).v);_tmp3CF->tl=_tmp862;});_tmp3CF;}));
goto _LL0;case 12U: _LL17: _LL18:
# 1268 "parse.y"
 yyval=Cyc_YY16(0);
goto _LL0;case 13U: _LL19: _LL1A:
# 1273 "parse.y"
 Cyc_Parse_parsing_tempest=1;
goto _LL0;case 14U: _LL1B: _LL1C:
# 1278 "parse.y"
 Cyc_Parse_parsing_tempest=0;
goto _LL0;case 15U: _LL1D: _LL1E: {
# 1283 "parse.y"
struct _fat_ptr _tmp3D0=Cyc_yyget_String_tok(&(yyyvsp[1]).v);struct _fat_ptr two=_tmp3D0;
Cyc_Lex_enter_extern_c();
if(({struct _fat_ptr _tmp864=(struct _fat_ptr)two;Cyc_strcmp(_tmp864,({const char*_tmp3D1="C";_tag_fat(_tmp3D1,sizeof(char),2U);}));})== 0)
yyval=Cyc_YY31(0);else{
if(({struct _fat_ptr _tmp865=(struct _fat_ptr)two;Cyc_strcmp(_tmp865,({const char*_tmp3D2="C include";_tag_fat(_tmp3D2,sizeof(char),10U);}));})== 0)
yyval=Cyc_YY31(1);else{
# 1290
({void*_tmp3D3=0U;({unsigned _tmp867=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp866=({const char*_tmp3D4="expecting \"C\" or \"C include\"";_tag_fat(_tmp3D4,sizeof(char),29U);});Cyc_Warn_err(_tmp867,_tmp866,_tag_fat(_tmp3D3,sizeof(void*),0U));});});
yyval=Cyc_YY31(1);}}
# 1294
goto _LL0;}case 16U: _LL1F: _LL20:
# 1297 "parse.y"
 Cyc_Lex_leave_extern_c();
goto _LL0;case 17U: _LL21: _LL22:
# 1301 "parse.y"
 yyval=Cyc_YY54(0);
goto _LL0;case 18U: _LL23: _LL24:
# 1302 "parse.y"
 yyval=(yyyvsp[2]).v;
goto _LL0;case 19U: _LL25: _LL26:
# 1306 "parse.y"
 yyval=Cyc_YY54(({struct Cyc_List_List*_tmp3D5=_cycalloc(sizeof(*_tmp3D5));({struct _tuple0*_tmp868=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3D5->hd=_tmp868;}),_tmp3D5->tl=0;_tmp3D5;}));
goto _LL0;case 20U: _LL27: _LL28:
# 1307 "parse.y"
 yyval=Cyc_YY54(({struct Cyc_List_List*_tmp3D6=_cycalloc(sizeof(*_tmp3D6));({struct _tuple0*_tmp869=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3D6->hd=_tmp869;}),_tmp3D6->tl=0;_tmp3D6;}));
goto _LL0;case 21U: _LL29: _LL2A:
# 1309 "parse.y"
 yyval=Cyc_YY54(({struct Cyc_List_List*_tmp3D7=_cycalloc(sizeof(*_tmp3D7));({struct _tuple0*_tmp86B=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3D7->hd=_tmp86B;}),({struct Cyc_List_List*_tmp86A=Cyc_yyget_YY54(&(yyyvsp[2]).v);_tmp3D7->tl=_tmp86A;});_tmp3D7;}));
goto _LL0;case 22U: _LL2B: _LL2C:
# 1313 "parse.y"
 yyval=Cyc_YY53(({struct _tuple28*_tmp3D8=_cycalloc(sizeof(*_tmp3D8));_tmp3D8->f1=0,_tmp3D8->f2=0U;_tmp3D8;}));
goto _LL0;case 23U: _LL2D: _LL2E:
# 1314 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 24U: _LL2F: _LL30:
# 1318 "parse.y"
 yyval=Cyc_YY53(({struct _tuple28*_tmp3D9=_cycalloc(sizeof(*_tmp3D9));({struct Cyc_List_List*_tmp86C=Cyc_yyget_YY52(&(yyyvsp[2]).v);_tmp3D9->f1=_tmp86C;}),_tmp3D9->f2=0U;_tmp3D9;}));
goto _LL0;case 25U: _LL31: _LL32:
# 1319 "parse.y"
 yyval=Cyc_YY53(({struct _tuple28*_tmp3DA=_cycalloc(sizeof(*_tmp3DA));_tmp3DA->f1=0,_tmp3DA->f2=0U;_tmp3DA;}));
goto _LL0;case 26U: _LL33: _LL34:
# 1320 "parse.y"
 yyval=Cyc_YY53(({struct _tuple28*_tmp3DB=_cycalloc(sizeof(*_tmp3DB));_tmp3DB->f1=0,_tmp3DB->f2=(unsigned)((yyyvsp[0]).l).first_line;_tmp3DB;}));
goto _LL0;case 27U: _LL35: _LL36:
# 1324 "parse.y"
 yyval=Cyc_YY52(({struct Cyc_List_List*_tmp3DD=_cycalloc(sizeof(*_tmp3DD));({struct _tuple32*_tmp86E=({struct _tuple32*_tmp3DC=_cycalloc(sizeof(*_tmp3DC));_tmp3DC->f1=(unsigned)((yyyvsp[0]).l).first_line,({struct _tuple0*_tmp86D=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3DC->f2=_tmp86D;}),_tmp3DC->f3=0;_tmp3DC;});_tmp3DD->hd=_tmp86E;}),_tmp3DD->tl=0;_tmp3DD;}));
goto _LL0;case 28U: _LL37: _LL38:
# 1325 "parse.y"
 yyval=Cyc_YY52(({struct Cyc_List_List*_tmp3DF=_cycalloc(sizeof(*_tmp3DF));({struct _tuple32*_tmp870=({struct _tuple32*_tmp3DE=_cycalloc(sizeof(*_tmp3DE));_tmp3DE->f1=(unsigned)((yyyvsp[0]).l).first_line,({struct _tuple0*_tmp86F=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3DE->f2=_tmp86F;}),_tmp3DE->f3=0;_tmp3DE;});_tmp3DF->hd=_tmp870;}),_tmp3DF->tl=0;_tmp3DF;}));
goto _LL0;case 29U: _LL39: _LL3A:
# 1327 "parse.y"
 yyval=Cyc_YY52(({struct Cyc_List_List*_tmp3E1=_cycalloc(sizeof(*_tmp3E1));({struct _tuple32*_tmp873=({struct _tuple32*_tmp3E0=_cycalloc(sizeof(*_tmp3E0));_tmp3E0->f1=(unsigned)((yyyvsp[0]).l).first_line,({struct _tuple0*_tmp872=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp3E0->f2=_tmp872;}),_tmp3E0->f3=0;_tmp3E0;});_tmp3E1->hd=_tmp873;}),({struct Cyc_List_List*_tmp871=Cyc_yyget_YY52(&(yyyvsp[2]).v);_tmp3E1->tl=_tmp871;});_tmp3E1;}));
goto _LL0;case 30U: _LL3B: _LL3C:
# 1331 "parse.y"
 yyval=Cyc_YY16(0);
goto _LL0;case 31U: _LL3D: _LL3E:
# 1332 "parse.y"
 yyval=(yyyvsp[2]).v;
goto _LL0;case 32U: _LL3F: _LL40:
# 1336 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3E3=_cycalloc(sizeof(*_tmp3E3));({struct Cyc_Absyn_Decl*_tmp876=({void*_tmp875=(void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp3E2=_cycalloc(sizeof(*_tmp3E2));_tmp3E2->tag=1U,({struct Cyc_Absyn_Fndecl*_tmp874=Cyc_yyget_YY15(&(yyyvsp[0]).v);_tmp3E2->f1=_tmp874;});_tmp3E2;});Cyc_Absyn_new_decl(_tmp875,(unsigned)((yyyvsp[0]).l).first_line);});_tmp3E3->hd=_tmp876;}),_tmp3E3->tl=0;_tmp3E3;}));
goto _LL0;case 33U: _LL41: _LL42:
# 1337 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 34U: _LL43: _LL44:
# 1338 "parse.y"
 yyval=Cyc_YY16(0);
goto _LL0;case 37U: _LL45: _LL46:
# 1347 "parse.y"
 yyval=Cyc_YY15(({struct _RegionHandle*_tmp879=yyr;struct Cyc_Parse_Declarator _tmp878=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Absyn_Stmt*_tmp877=Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);Cyc_Parse_make_function(_tmp879,0,_tmp878,0,_tmp877,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 38U: _LL47: _LL48: {
# 1349 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp3E4=Cyc_yyget_YY17(&(yyyvsp[0]).v);struct Cyc_Parse_Declaration_spec d=_tmp3E4;
yyval=Cyc_YY15(({struct _RegionHandle*_tmp87C=yyr;struct Cyc_Parse_Declarator _tmp87B=Cyc_yyget_YY27(&(yyyvsp[1]).v);struct Cyc_Absyn_Stmt*_tmp87A=Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);Cyc_Parse_make_function(_tmp87C,& d,_tmp87B,0,_tmp87A,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;}case 39U: _LL49: _LL4A:
# 1362 "parse.y"
 yyval=Cyc_YY15(({struct _RegionHandle*_tmp880=yyr;struct Cyc_Parse_Declarator _tmp87F=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_List_List*_tmp87E=Cyc_yyget_YY16(&(yyyvsp[1]).v);struct Cyc_Absyn_Stmt*_tmp87D=Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);Cyc_Parse_make_function(_tmp880,0,_tmp87F,_tmp87E,_tmp87D,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 40U: _LL4B: _LL4C: {
# 1364 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp3E5=Cyc_yyget_YY17(&(yyyvsp[0]).v);struct Cyc_Parse_Declaration_spec d=_tmp3E5;
yyval=Cyc_YY15(({struct _RegionHandle*_tmp884=yyr;struct Cyc_Parse_Declarator _tmp883=Cyc_yyget_YY27(&(yyyvsp[1]).v);struct Cyc_List_List*_tmp882=Cyc_yyget_YY16(&(yyyvsp[2]).v);struct Cyc_Absyn_Stmt*_tmp881=Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v);Cyc_Parse_make_function(_tmp884,& d,_tmp883,_tmp882,_tmp881,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;}case 41U: _LL4D: _LL4E: {
# 1372 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp3E6=Cyc_yyget_YY17(&(yyyvsp[0]).v);struct Cyc_Parse_Declaration_spec d=_tmp3E6;
yyval=Cyc_YY15(({struct _RegionHandle*_tmp887=yyr;struct Cyc_Parse_Declarator _tmp886=Cyc_yyget_YY27(&(yyyvsp[1]).v);struct Cyc_Absyn_Stmt*_tmp885=Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);Cyc_Parse_make_function(_tmp887,& d,_tmp886,0,_tmp885,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;}case 42U: _LL4F: _LL50: {
# 1375 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp3E7=Cyc_yyget_YY17(&(yyyvsp[0]).v);struct Cyc_Parse_Declaration_spec d=_tmp3E7;
yyval=Cyc_YY15(({struct _RegionHandle*_tmp88B=yyr;struct Cyc_Parse_Declarator _tmp88A=Cyc_yyget_YY27(&(yyyvsp[1]).v);struct Cyc_List_List*_tmp889=Cyc_yyget_YY16(&(yyyvsp[2]).v);struct Cyc_Absyn_Stmt*_tmp888=Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v);Cyc_Parse_make_function(_tmp88B,& d,_tmp88A,_tmp889,_tmp888,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;}case 43U: _LL51: _LL52:
# 1380 "parse.y"
 Cyc_Lex_enter_using(Cyc_yyget_QualId_tok(&(yyyvsp[1]).v));yyval=(yyyvsp[1]).v;
goto _LL0;case 44U: _LL53: _LL54:
# 1383
 Cyc_Lex_leave_using();
goto _LL0;case 45U: _LL55: _LL56:
# 1386
 Cyc_Lex_enter_namespace(({struct _fat_ptr*_tmp3E8=_cycalloc(sizeof(*_tmp3E8));({struct _fat_ptr _tmp88C=Cyc_yyget_String_tok(&(yyyvsp[1]).v);*_tmp3E8=_tmp88C;});_tmp3E8;}));yyval=(yyyvsp[1]).v;
goto _LL0;case 46U: _LL57: _LL58:
# 1389
 Cyc_Lex_leave_namespace();
goto _LL0;case 47U: _LL59: _LL5A: {
# 1395 "parse.y"
int _tmp3E9=((yyyvsp[0]).l).first_line;int location=_tmp3E9;
yyval=Cyc_YY16(({struct Cyc_Parse_Declaration_spec _tmp88E=Cyc_yyget_YY17(&(yyyvsp[0]).v);unsigned _tmp88D=(unsigned)location;Cyc_Parse_make_declarations(_tmp88E,0,_tmp88D,(unsigned)location);}));
goto _LL0;}case 48U: _LL5B: _LL5C: {
# 1398 "parse.y"
int _tmp3EA=((yyyvsp[0]).l).first_line;int location=_tmp3EA;
yyval=Cyc_YY16(({struct Cyc_Parse_Declaration_spec _tmp891=Cyc_yyget_YY17(&(yyyvsp[0]).v);struct _tuple13*_tmp890=Cyc_yyget_YY19(&(yyyvsp[1]).v);unsigned _tmp88F=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Parse_make_declarations(_tmp891,_tmp890,_tmp88F,(unsigned)location);}));
goto _LL0;}case 49U: _LL5D: _LL5E:
# 1402
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3EB=_cycalloc(sizeof(*_tmp3EB));({struct Cyc_Absyn_Decl*_tmp894=({struct Cyc_Absyn_Pat*_tmp893=Cyc_yyget_YY9(&(yyyvsp[1]).v);struct Cyc_Absyn_Exp*_tmp892=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);Cyc_Absyn_let_decl(_tmp893,_tmp892,(unsigned)((yyyvsp[0]).l).first_line);});_tmp3EB->hd=_tmp894;}),_tmp3EB->tl=0;_tmp3EB;}));
goto _LL0;case 50U: _LL5F: _LL60: {
# 1404 "parse.y"
struct Cyc_List_List*_tmp3EC=0;struct Cyc_List_List*vds=_tmp3EC;
{struct Cyc_List_List*_tmp3ED=Cyc_yyget_YY36(&(yyyvsp[1]).v);struct Cyc_List_List*ids=_tmp3ED;for(0;ids != 0;ids=ids->tl){
struct _fat_ptr*_tmp3EE=(struct _fat_ptr*)ids->hd;struct _fat_ptr*id=_tmp3EE;
struct _tuple0*qv=({struct _tuple0*_tmp3F1=_cycalloc(sizeof(*_tmp3F1));({union Cyc_Absyn_Nmspace _tmp895=Cyc_Absyn_Rel_n(0);_tmp3F1->f1=_tmp895;}),_tmp3F1->f2=id;_tmp3F1;});
struct Cyc_Absyn_Vardecl*_tmp3EF=({struct _tuple0*_tmp896=qv;Cyc_Absyn_new_vardecl(0U,_tmp896,Cyc_Absyn_wildtyp(0),0);});struct Cyc_Absyn_Vardecl*vd=_tmp3EF;
vds=({struct Cyc_List_List*_tmp3F0=_cycalloc(sizeof(*_tmp3F0));_tmp3F0->hd=vd,_tmp3F0->tl=vds;_tmp3F0;});}}
# 1411
vds=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(vds);
yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3F2=_cycalloc(sizeof(*_tmp3F2));({struct Cyc_Absyn_Decl*_tmp897=Cyc_Absyn_letv_decl(vds,(unsigned)((yyyvsp[0]).l).first_line);_tmp3F2->hd=_tmp897;}),_tmp3F2->tl=0;_tmp3F2;}));
# 1414
goto _LL0;}case 51U: _LL61: _LL62: {
# 1417 "parse.y"
struct _fat_ptr _tmp3F3=Cyc_yyget_String_tok(&(yyyvsp[2]).v);struct _fat_ptr three=_tmp3F3;
struct _fat_ptr err=({const char*_tmp3FA="";_tag_fat(_tmp3FA,sizeof(char),1U);});
if(!Cyc_Parse_tvar_ok(three,& err))({void*_tmp3F4=0U;({unsigned _tmp899=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmp898=err;Cyc_Warn_err(_tmp899,_tmp898,_tag_fat(_tmp3F4,sizeof(void*),0U));});});{
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp3F9=_cycalloc(sizeof(*_tmp3F9));({struct _fat_ptr*_tmp89B=({struct _fat_ptr*_tmp3F8=_cycalloc(sizeof(*_tmp3F8));*_tmp3F8=three;_tmp3F8;});_tmp3F9->name=_tmp89B;}),_tmp3F9->identity=- 1,({void*_tmp89A=Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);_tmp3F9->kind=_tmp89A;});_tmp3F9;});
void*t=Cyc_Absyn_var_type(tv);
struct Cyc_Absyn_Vardecl*vd=({unsigned _tmp89F=(unsigned)((yyyvsp[4]).l).first_line;struct _tuple0*_tmp89E=({struct _tuple0*_tmp3F7=_cycalloc(sizeof(*_tmp3F7));_tmp3F7->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp89D=({struct _fat_ptr*_tmp3F6=_cycalloc(sizeof(*_tmp3F6));({struct _fat_ptr _tmp89C=Cyc_yyget_String_tok(&(yyyvsp[4]).v);*_tmp3F6=_tmp89C;});_tmp3F6;});_tmp3F7->f2=_tmp89D;});_tmp3F7;});Cyc_Absyn_new_vardecl(_tmp89F,_tmp89E,Cyc_Absyn_rgn_handle_type(t),0);});
yyval=Cyc_YY16(({struct Cyc_List_List*_tmp3F5=_cycalloc(sizeof(*_tmp3F5));({struct Cyc_Absyn_Decl*_tmp8A0=Cyc_Absyn_region_decl(tv,vd,0,(unsigned)((yyyvsp[0]).l).first_line);_tmp3F5->hd=_tmp8A0;}),_tmp3F5->tl=0;_tmp3F5;}));
# 1425
goto _LL0;}}case 52U: _LL63: _LL64: {
# 1427
struct _fat_ptr _tmp3FB=Cyc_yyget_String_tok(&(yyyvsp[1]).v);struct _fat_ptr two=_tmp3FB;
if(({struct _fat_ptr _tmp8A1=(struct _fat_ptr)two;Cyc_zstrcmp(_tmp8A1,({const char*_tmp3FC="H";_tag_fat(_tmp3FC,sizeof(char),2U);}));})== 0)
({void*_tmp3FD=0U;({unsigned _tmp8A3=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp8A2=({const char*_tmp3FE="bad occurrence of heap region `H";_tag_fat(_tmp3FE,sizeof(char),33U);});Cyc_Warn_err(_tmp8A3,_tmp8A2,_tag_fat(_tmp3FD,sizeof(void*),0U));});});
if(({struct _fat_ptr _tmp8A4=(struct _fat_ptr)two;Cyc_zstrcmp(_tmp8A4,({const char*_tmp3FF="U";_tag_fat(_tmp3FF,sizeof(char),2U);}));})== 0)
({void*_tmp400=0U;({unsigned _tmp8A6=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp8A5=({const char*_tmp401="bad occurrence of unique region `U";_tag_fat(_tmp401,sizeof(char),35U);});Cyc_Warn_err(_tmp8A6,_tmp8A5,_tag_fat(_tmp400,sizeof(void*),0U));});});{
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp409=_cycalloc(sizeof(*_tmp409));({struct _fat_ptr*_tmp8AA=({struct _fat_ptr*_tmp408=_cycalloc(sizeof(*_tmp408));({struct _fat_ptr _tmp8A9=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp407=({struct Cyc_String_pa_PrintArg_struct _tmp72B;_tmp72B.tag=0U,_tmp72B.f1=(struct _fat_ptr)((struct _fat_ptr)two);_tmp72B;});void*_tmp405[1U];_tmp405[0]=& _tmp407;({struct _fat_ptr _tmp8A8=({const char*_tmp406="`%s";_tag_fat(_tmp406,sizeof(char),4U);});Cyc_aprintf(_tmp8A8,_tag_fat(_tmp405,sizeof(void*),1U));});});*_tmp408=_tmp8A9;});_tmp408;});_tmp409->name=_tmp8AA;}),_tmp409->identity=- 1,({
void*_tmp8A7=Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);_tmp409->kind=_tmp8A7;});_tmp409;});
void*t=Cyc_Absyn_var_type(tv);
struct Cyc_Absyn_Vardecl*vd=({unsigned _tmp8AD=(unsigned)((yyyvsp[1]).l).first_line;struct _tuple0*_tmp8AC=({struct _tuple0*_tmp404=_cycalloc(sizeof(*_tmp404));_tmp404->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp8AB=({struct _fat_ptr*_tmp403=_cycalloc(sizeof(*_tmp403));*_tmp403=two;_tmp403;});_tmp404->f2=_tmp8AB;});_tmp404;});Cyc_Absyn_new_vardecl(_tmp8AD,_tmp8AC,Cyc_Absyn_rgn_handle_type(t),0);});
yyval=Cyc_YY16(({struct Cyc_List_List*_tmp402=_cycalloc(sizeof(*_tmp402));({struct Cyc_Absyn_Decl*_tmp8AE=Cyc_Absyn_region_decl(tv,vd,0,(unsigned)((yyyvsp[0]).l).first_line);_tmp402->hd=_tmp8AE;}),_tmp402->tl=0;_tmp402;}));
# 1438
goto _LL0;}}case 53U: _LL65: _LL66: {
# 1440
struct _fat_ptr _tmp40A=Cyc_yyget_String_tok(&(yyyvsp[1]).v);struct _fat_ptr two=_tmp40A;
struct _fat_ptr _tmp40B=Cyc_yyget_String_tok(&(yyyvsp[3]).v);struct _fat_ptr four=_tmp40B;
struct Cyc_Absyn_Exp*_tmp40C=Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);struct Cyc_Absyn_Exp*six=_tmp40C;
if(({struct _fat_ptr _tmp8AF=(struct _fat_ptr)four;Cyc_strcmp(_tmp8AF,({const char*_tmp40D="open";_tag_fat(_tmp40D,sizeof(char),5U);}));})!= 0)({void*_tmp40E=0U;({unsigned _tmp8B1=(unsigned)((yyyvsp[3]).l).first_line;struct _fat_ptr _tmp8B0=({const char*_tmp40F="expecting `open'";_tag_fat(_tmp40F,sizeof(char),17U);});Cyc_Warn_err(_tmp8B1,_tmp8B0,_tag_fat(_tmp40E,sizeof(void*),0U));});});{
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp417=_cycalloc(sizeof(*_tmp417));({struct _fat_ptr*_tmp8B5=({struct _fat_ptr*_tmp416=_cycalloc(sizeof(*_tmp416));({struct _fat_ptr _tmp8B4=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp415=({struct Cyc_String_pa_PrintArg_struct _tmp72C;_tmp72C.tag=0U,_tmp72C.f1=(struct _fat_ptr)((struct _fat_ptr)two);_tmp72C;});void*_tmp413[1U];_tmp413[0]=& _tmp415;({struct _fat_ptr _tmp8B3=({const char*_tmp414="`%s";_tag_fat(_tmp414,sizeof(char),4U);});Cyc_aprintf(_tmp8B3,_tag_fat(_tmp413,sizeof(void*),1U));});});*_tmp416=_tmp8B4;});_tmp416;});_tmp417->name=_tmp8B5;}),_tmp417->identity=- 1,({
void*_tmp8B2=Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);_tmp417->kind=_tmp8B2;});_tmp417;});
void*t=Cyc_Absyn_var_type(tv);
struct Cyc_Absyn_Vardecl*vd=({unsigned _tmp8B8=(unsigned)((yyyvsp[2]).l).first_line;struct _tuple0*_tmp8B7=({struct _tuple0*_tmp412=_cycalloc(sizeof(*_tmp412));_tmp412->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp8B6=({struct _fat_ptr*_tmp411=_cycalloc(sizeof(*_tmp411));*_tmp411=two;_tmp411;});_tmp412->f2=_tmp8B6;});_tmp412;});Cyc_Absyn_new_vardecl(_tmp8B8,_tmp8B7,Cyc_Absyn_rgn_handle_type(t),0);});
yyval=Cyc_YY16(({struct Cyc_List_List*_tmp410=_cycalloc(sizeof(*_tmp410));({struct Cyc_Absyn_Decl*_tmp8B9=Cyc_Absyn_region_decl(tv,vd,six,(unsigned)((yyyvsp[0]).l).first_line);_tmp410->hd=_tmp8B9;}),_tmp410->tl=0;_tmp410;}));
# 1450
goto _LL0;}}case 54U: _LL67: _LL68:
# 1454 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 55U: _LL69: _LL6A:
# 1456 "parse.y"
 yyval=Cyc_YY16(({struct Cyc_List_List*_tmp8BA=Cyc_yyget_YY16(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp8BA,Cyc_yyget_YY16(&(yyyvsp[1]).v));}));
goto _LL0;case 56U: _LL6B: _LL6C:
# 1462 "parse.y"
 yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp72D;({enum Cyc_Parse_Storage_class*_tmp8BD=Cyc_yyget_YY20(&(yyyvsp[0]).v);_tmp72D.sc=_tmp8BD;}),({struct Cyc_Absyn_Tqual _tmp8BC=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp72D.tq=_tmp8BC;}),({
struct Cyc_Parse_Type_specifier _tmp8BB=Cyc_Parse_empty_spec(0U);_tmp72D.type_specs=_tmp8BB;}),_tmp72D.is_inline=0,_tmp72D.attributes=0;_tmp72D;}));
goto _LL0;case 57U: _LL6D: _LL6E: {
# 1465 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp418=Cyc_yyget_YY17(&(yyyvsp[1]).v);struct Cyc_Parse_Declaration_spec two=_tmp418;
if(two.sc != 0)
({void*_tmp419=0U;({unsigned _tmp8BF=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp8BE=({const char*_tmp41A="Only one storage class is allowed in a declaration (missing ';' or ','?)";_tag_fat(_tmp41A,sizeof(char),73U);});Cyc_Warn_warn(_tmp8BF,_tmp8BE,_tag_fat(_tmp419,sizeof(void*),0U));});});
# 1469
yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp72E;({enum Cyc_Parse_Storage_class*_tmp8C0=Cyc_yyget_YY20(&(yyyvsp[0]).v);_tmp72E.sc=_tmp8C0;}),_tmp72E.tq=two.tq,_tmp72E.type_specs=two.type_specs,_tmp72E.is_inline=two.is_inline,_tmp72E.attributes=two.attributes;_tmp72E;}));
# 1473
goto _LL0;}case 58U: _LL6F: _LL70:
# 1474 "parse.y"
({void*_tmp41B=0U;({unsigned _tmp8C2=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp8C1=({const char*_tmp41C="__extension__ keyword ignored in declaration";_tag_fat(_tmp41C,sizeof(char),45U);});Cyc_Warn_warn(_tmp8C2,_tmp8C1,_tag_fat(_tmp41B,sizeof(void*),0U));});});
yyval=(yyyvsp[1]).v;
# 1477
goto _LL0;case 59U: _LL71: _LL72:
# 1478 "parse.y"
 yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp72F;_tmp72F.sc=0,({struct Cyc_Absyn_Tqual _tmp8C4=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp72F.tq=_tmp8C4;}),({
struct Cyc_Parse_Type_specifier _tmp8C3=Cyc_yyget_YY21(&(yyyvsp[0]).v);_tmp72F.type_specs=_tmp8C3;}),_tmp72F.is_inline=0,_tmp72F.attributes=0;_tmp72F;}));
goto _LL0;case 60U: _LL73: _LL74: {
# 1481 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp41D=Cyc_yyget_YY17(&(yyyvsp[1]).v);struct Cyc_Parse_Declaration_spec two=_tmp41D;
yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp730;_tmp730.sc=two.sc,_tmp730.tq=two.tq,({
struct Cyc_Parse_Type_specifier _tmp8C7=({unsigned _tmp8C6=(unsigned)((yyyvsp[0]).l).first_line;struct Cyc_Parse_Type_specifier _tmp8C5=two.type_specs;Cyc_Parse_combine_specifiers(_tmp8C6,_tmp8C5,Cyc_yyget_YY21(&(yyyvsp[0]).v));});_tmp730.type_specs=_tmp8C7;}),_tmp730.is_inline=two.is_inline,_tmp730.attributes=two.attributes;_tmp730;}));
# 1487
goto _LL0;}case 61U: _LL75: _LL76:
# 1488 "parse.y"
 yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp731;_tmp731.sc=0,({struct Cyc_Absyn_Tqual _tmp8C9=Cyc_yyget_YY23(&(yyyvsp[0]).v);_tmp731.tq=_tmp8C9;}),({struct Cyc_Parse_Type_specifier _tmp8C8=Cyc_Parse_empty_spec(0U);_tmp731.type_specs=_tmp8C8;}),_tmp731.is_inline=0,_tmp731.attributes=0;_tmp731;}));
goto _LL0;case 62U: _LL77: _LL78: {
# 1490 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp41E=Cyc_yyget_YY17(&(yyyvsp[1]).v);struct Cyc_Parse_Declaration_spec two=_tmp41E;
yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp732;_tmp732.sc=two.sc,({struct Cyc_Absyn_Tqual _tmp8CB=({struct Cyc_Absyn_Tqual _tmp8CA=Cyc_yyget_YY23(&(yyyvsp[0]).v);Cyc_Absyn_combine_tqual(_tmp8CA,two.tq);});_tmp732.tq=_tmp8CB;}),_tmp732.type_specs=two.type_specs,_tmp732.is_inline=two.is_inline,_tmp732.attributes=two.attributes;_tmp732;}));
# 1495
goto _LL0;}case 63U: _LL79: _LL7A:
# 1496 "parse.y"
 yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp733;_tmp733.sc=0,({struct Cyc_Absyn_Tqual _tmp8CD=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp733.tq=_tmp8CD;}),({
struct Cyc_Parse_Type_specifier _tmp8CC=Cyc_Parse_empty_spec(0U);_tmp733.type_specs=_tmp8CC;}),_tmp733.is_inline=1,_tmp733.attributes=0;_tmp733;}));
goto _LL0;case 64U: _LL7B: _LL7C: {
# 1499 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp41F=Cyc_yyget_YY17(&(yyyvsp[1]).v);struct Cyc_Parse_Declaration_spec two=_tmp41F;
yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp734;_tmp734.sc=two.sc,_tmp734.tq=two.tq,_tmp734.type_specs=two.type_specs,_tmp734.is_inline=1,_tmp734.attributes=two.attributes;_tmp734;}));
# 1503
goto _LL0;}case 65U: _LL7D: _LL7E:
# 1504 "parse.y"
 yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp735;_tmp735.sc=0,({struct Cyc_Absyn_Tqual _tmp8D0=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp735.tq=_tmp8D0;}),({
struct Cyc_Parse_Type_specifier _tmp8CF=Cyc_Parse_empty_spec(0U);_tmp735.type_specs=_tmp8CF;}),_tmp735.is_inline=0,({struct Cyc_List_List*_tmp8CE=Cyc_yyget_YY45(&(yyyvsp[0]).v);_tmp735.attributes=_tmp8CE;});_tmp735;}));
goto _LL0;case 66U: _LL7F: _LL80: {
# 1507 "parse.y"
struct Cyc_Parse_Declaration_spec _tmp420=Cyc_yyget_YY17(&(yyyvsp[1]).v);struct Cyc_Parse_Declaration_spec two=_tmp420;
yyval=Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmp736;_tmp736.sc=two.sc,_tmp736.tq=two.tq,_tmp736.type_specs=two.type_specs,_tmp736.is_inline=two.is_inline,({
# 1510
struct Cyc_List_List*_tmp8D2=({struct Cyc_List_List*_tmp8D1=Cyc_yyget_YY45(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp8D1,two.attributes);});_tmp736.attributes=_tmp8D2;});_tmp736;}));
goto _LL0;}case 67U: _LL81: _LL82: {
# 1514 "parse.y"
static enum Cyc_Parse_Storage_class auto_sc=Cyc_Parse_Auto_sc;
yyval=Cyc_YY20(& auto_sc);
goto _LL0;}case 68U: _LL83: _LL84: {
# 1516 "parse.y"
static enum Cyc_Parse_Storage_class register_sc=Cyc_Parse_Register_sc;
yyval=Cyc_YY20(& register_sc);
goto _LL0;}case 69U: _LL85: _LL86: {
# 1518 "parse.y"
static enum Cyc_Parse_Storage_class static_sc=Cyc_Parse_Static_sc;
yyval=Cyc_YY20(& static_sc);
goto _LL0;}case 70U: _LL87: _LL88: {
# 1520 "parse.y"
static enum Cyc_Parse_Storage_class extern_sc=Cyc_Parse_Extern_sc;
yyval=Cyc_YY20(& extern_sc);
goto _LL0;}case 71U: _LL89: _LL8A: {
# 1523 "parse.y"
static enum Cyc_Parse_Storage_class externC_sc=Cyc_Parse_ExternC_sc;
if(({struct _fat_ptr _tmp8D3=(struct _fat_ptr)Cyc_yyget_String_tok(&(yyyvsp[1]).v);Cyc_strcmp(_tmp8D3,({const char*_tmp421="C";_tag_fat(_tmp421,sizeof(char),2U);}));})!= 0)
({void*_tmp422=0U;({unsigned _tmp8D5=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp8D4=({const char*_tmp423="only extern or extern \"C\" is allowed";_tag_fat(_tmp423,sizeof(char),37U);});Cyc_Warn_err(_tmp8D5,_tmp8D4,_tag_fat(_tmp422,sizeof(void*),0U));});});
yyval=Cyc_YY20(& externC_sc);
# 1528
goto _LL0;}case 72U: _LL8B: _LL8C: {
# 1528 "parse.y"
static enum Cyc_Parse_Storage_class typedef_sc=Cyc_Parse_Typedef_sc;
yyval=Cyc_YY20(& typedef_sc);
goto _LL0;}case 73U: _LL8D: _LL8E: {
# 1531 "parse.y"
static enum Cyc_Parse_Storage_class abstract_sc=Cyc_Parse_Abstract_sc;
yyval=Cyc_YY20(& abstract_sc);
goto _LL0;}case 74U: _LL8F: _LL90:
# 1537 "parse.y"
 yyval=Cyc_YY45(0);
goto _LL0;case 75U: _LL91: _LL92:
# 1538 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 76U: _LL93: _LL94:
# 1543 "parse.y"
 yyval=(yyyvsp[3]).v;
goto _LL0;case 77U: _LL95: _LL96:
# 1547 "parse.y"
 yyval=Cyc_YY45(({struct Cyc_List_List*_tmp424=_cycalloc(sizeof(*_tmp424));({void*_tmp8D6=Cyc_yyget_YY46(&(yyyvsp[0]).v);_tmp424->hd=_tmp8D6;}),_tmp424->tl=0;_tmp424;}));
goto _LL0;case 78U: _LL97: _LL98:
# 1548 "parse.y"
 yyval=Cyc_YY45(({struct Cyc_List_List*_tmp425=_cycalloc(sizeof(*_tmp425));({void*_tmp8D8=Cyc_yyget_YY46(&(yyyvsp[0]).v);_tmp425->hd=_tmp8D8;}),({struct Cyc_List_List*_tmp8D7=Cyc_yyget_YY45(&(yyyvsp[2]).v);_tmp425->tl=_tmp8D7;});_tmp425;}));
goto _LL0;case 79U: _LL99: _LL9A: {
# 1553 "parse.y"
static struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct att_aligned={6U,0};
static struct _tuple33 att_map[18U]={{{_tmp429,_tmp429,_tmp429 + 8U},(void*)& Cyc_Absyn_Stdcall_att_val},{{_tmp42A,_tmp42A,_tmp42A + 6U},(void*)& Cyc_Absyn_Cdecl_att_val},{{_tmp42B,_tmp42B,_tmp42B + 9U},(void*)& Cyc_Absyn_Fastcall_att_val},{{_tmp42C,_tmp42C,_tmp42C + 9U},(void*)& Cyc_Absyn_Noreturn_att_val},{{_tmp42D,_tmp42D,_tmp42D + 6U},(void*)& Cyc_Absyn_Const_att_val},{{_tmp42E,_tmp42E,_tmp42E + 8U},(void*)& att_aligned},{{_tmp42F,_tmp42F,_tmp42F + 7U},(void*)& Cyc_Absyn_Packed_att_val},{{_tmp430,_tmp430,_tmp430 + 7U},(void*)& Cyc_Absyn_Shared_att_val},{{_tmp431,_tmp431,_tmp431 + 7U},(void*)& Cyc_Absyn_Unused_att_val},{{_tmp432,_tmp432,_tmp432 + 5U},(void*)& Cyc_Absyn_Weak_att_val},{{_tmp433,_tmp433,_tmp433 + 10U},(void*)& Cyc_Absyn_Dllimport_att_val},{{_tmp434,_tmp434,_tmp434 + 10U},(void*)& Cyc_Absyn_Dllexport_att_val},{{_tmp435,_tmp435,_tmp435 + 23U},(void*)& Cyc_Absyn_No_instrument_function_att_val},{{_tmp436,_tmp436,_tmp436 + 12U},(void*)& Cyc_Absyn_Constructor_att_val},{{_tmp437,_tmp437,_tmp437 + 11U},(void*)& Cyc_Absyn_Destructor_att_val},{{_tmp438,_tmp438,_tmp438 + 22U},(void*)& Cyc_Absyn_No_check_memory_usage_att_val},{{_tmp439,_tmp439,_tmp439 + 5U},(void*)& Cyc_Absyn_Pure_att_val},{{_tmp43A,_tmp43A,_tmp43A + 14U},(void*)& Cyc_Absyn_Always_inline_att_val}};
# 1574
struct _fat_ptr _tmp426=Cyc_yyget_String_tok(&(yyyvsp[0]).v);struct _fat_ptr s=_tmp426;
# 1576
if((((_get_fat_size(s,sizeof(char))> (unsigned)4 &&(int)((const char*)s.curr)[0]== (int)'_')&&(int)((const char*)s.curr)[1]== (int)'_')&&(int)*((const char*)_check_fat_subscript(s,sizeof(char),(int)(_get_fat_size(s,sizeof(char))- (unsigned)2)))== (int)'_')&&(int)*((const char*)_check_fat_subscript(s,sizeof(char),(int)(_get_fat_size(s,sizeof(char))- (unsigned)3)))== (int)'_')
# 1578
s=(struct _fat_ptr)Cyc_substring((struct _fat_ptr)s,2,_get_fat_size(s,sizeof(char))- (unsigned)5);{
int i=0;
for(0;(unsigned)i < 18U;++ i){
if(Cyc_strcmp((struct _fat_ptr)s,(struct _fat_ptr)(*((struct _tuple33*)_check_known_subscript_notnull(att_map,18U,sizeof(struct _tuple33),i))).f1)== 0){
yyval=Cyc_YY46((att_map[i]).f2);
break;}}
# 1585
if((unsigned)i == 18U){
({void*_tmp427=0U;({unsigned _tmp8DA=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp8D9=({const char*_tmp428="unrecognized attribute";_tag_fat(_tmp428,sizeof(char),23U);});Cyc_Warn_err(_tmp8DA,_tmp8D9,_tag_fat(_tmp427,sizeof(void*),0U));});});
yyval=Cyc_YY46((void*)& Cyc_Absyn_Cdecl_att_val);}
# 1590
goto _LL0;}}case 80U: _LL9B: _LL9C:
# 1590 "parse.y"
 yyval=Cyc_YY46((void*)& Cyc_Absyn_Const_att_val);
goto _LL0;case 81U: _LL9D: _LL9E: {
# 1592 "parse.y"
struct _fat_ptr _tmp43B=Cyc_yyget_String_tok(&(yyyvsp[0]).v);struct _fat_ptr s=_tmp43B;
struct Cyc_Absyn_Exp*_tmp43C=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*e=_tmp43C;
void*a;
if(({struct _fat_ptr _tmp8DC=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8DC,({const char*_tmp43D="aligned";_tag_fat(_tmp43D,sizeof(char),8U);}));})== 0 ||({struct _fat_ptr _tmp8DB=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8DB,({const char*_tmp43E="__aligned__";_tag_fat(_tmp43E,sizeof(char),12U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct*_tmp43F=_cycalloc(sizeof(*_tmp43F));_tmp43F->tag=6U,_tmp43F->f1=e;_tmp43F;});else{
if(({struct _fat_ptr _tmp8DE=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8DE,({const char*_tmp440="section";_tag_fat(_tmp440,sizeof(char),8U);}));})== 0 ||({struct _fat_ptr _tmp8DD=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8DD,({const char*_tmp441="__section__";_tag_fat(_tmp441,sizeof(char),12U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Section_att_Absyn_Attribute_struct*_tmp442=_cycalloc(sizeof(*_tmp442));_tmp442->tag=8U,({struct _fat_ptr _tmp8DF=Cyc_Parse_exp2string((unsigned)((yyyvsp[2]).l).first_line,e);_tmp442->f1=_tmp8DF;});_tmp442;});else{
if(({struct _fat_ptr _tmp8E0=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8E0,({const char*_tmp443="__mode__";_tag_fat(_tmp443,sizeof(char),9U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct*_tmp444=_cycalloc(sizeof(*_tmp444));_tmp444->tag=24U,({struct _fat_ptr _tmp8E1=Cyc_Parse_exp2string((unsigned)((yyyvsp[2]).l).first_line,e);_tmp444->f1=_tmp8E1;});_tmp444;});else{
if(({struct _fat_ptr _tmp8E2=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8E2,({const char*_tmp445="alias";_tag_fat(_tmp445,sizeof(char),6U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct*_tmp446=_cycalloc(sizeof(*_tmp446));_tmp446->tag=25U,({struct _fat_ptr _tmp8E3=Cyc_Parse_exp2string((unsigned)((yyyvsp[2]).l).first_line,e);_tmp446->f1=_tmp8E3;});_tmp446;});else{
# 1604
int n=Cyc_Parse_exp2int((unsigned)((yyyvsp[2]).l).first_line,e);
if(({struct _fat_ptr _tmp8E5=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8E5,({const char*_tmp447="regparm";_tag_fat(_tmp447,sizeof(char),8U);}));})== 0 ||({struct _fat_ptr _tmp8E4=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8E4,({const char*_tmp448="__regparm__";_tag_fat(_tmp448,sizeof(char),12U);}));})== 0){
if(n < 0 || n > 3)
({void*_tmp449=0U;({unsigned _tmp8E7=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmp8E6=({const char*_tmp44A="regparm requires value between 0 and 3";_tag_fat(_tmp44A,sizeof(char),39U);});Cyc_Warn_err(_tmp8E7,_tmp8E6,_tag_fat(_tmp449,sizeof(void*),0U));});});
a=(void*)({struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct*_tmp44B=_cycalloc(sizeof(*_tmp44B));_tmp44B->tag=0U,_tmp44B->f1=n;_tmp44B;});}else{
if(({struct _fat_ptr _tmp8E9=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8E9,({const char*_tmp44C="initializes";_tag_fat(_tmp44C,sizeof(char),12U);}));})== 0 ||({struct _fat_ptr _tmp8E8=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8E8,({const char*_tmp44D="__initializes__";_tag_fat(_tmp44D,sizeof(char),16U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct*_tmp44E=_cycalloc(sizeof(*_tmp44E));_tmp44E->tag=20U,_tmp44E->f1=n;_tmp44E;});else{
if(({struct _fat_ptr _tmp8EB=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8EB,({const char*_tmp44F="noliveunique";_tag_fat(_tmp44F,sizeof(char),13U);}));})== 0 ||({struct _fat_ptr _tmp8EA=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8EA,({const char*_tmp450="__noliveunique__";_tag_fat(_tmp450,sizeof(char),17U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct*_tmp451=_cycalloc(sizeof(*_tmp451));_tmp451->tag=21U,_tmp451->f1=n;_tmp451;});else{
if(({struct _fat_ptr _tmp8ED=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8ED,({const char*_tmp452="consume";_tag_fat(_tmp452,sizeof(char),8U);}));})== 0 ||({struct _fat_ptr _tmp8EC=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8EC,({const char*_tmp453="__consume__";_tag_fat(_tmp453,sizeof(char),12U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct*_tmp454=_cycalloc(sizeof(*_tmp454));_tmp454->tag=22U,_tmp454->f1=n;_tmp454;});else{
# 1616
({void*_tmp455=0U;({unsigned _tmp8EF=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp8EE=({const char*_tmp456="unrecognized attribute";_tag_fat(_tmp456,sizeof(char),23U);});Cyc_Warn_err(_tmp8EF,_tmp8EE,_tag_fat(_tmp455,sizeof(void*),0U));});});
a=(void*)& Cyc_Absyn_Cdecl_att_val;}}}}}}}}
# 1620
yyval=Cyc_YY46(a);
# 1622
goto _LL0;}case 82U: _LL9F: _LLA0: {
# 1623 "parse.y"
struct _fat_ptr _tmp457=Cyc_yyget_String_tok(&(yyyvsp[0]).v);struct _fat_ptr s=_tmp457;
struct _fat_ptr _tmp458=Cyc_yyget_String_tok(&(yyyvsp[2]).v);struct _fat_ptr t=_tmp458;
unsigned _tmp459=({unsigned _tmp8F0=(unsigned)((yyyvsp[4]).l).first_line;Cyc_Parse_cnst2uint(_tmp8F0,Cyc_yyget_Int_tok(&(yyyvsp[4]).v));});unsigned n=_tmp459;
unsigned _tmp45A=({unsigned _tmp8F1=(unsigned)((yyyvsp[6]).l).first_line;Cyc_Parse_cnst2uint(_tmp8F1,Cyc_yyget_Int_tok(&(yyyvsp[6]).v));});unsigned m=_tmp45A;
void*a=(void*)& Cyc_Absyn_Cdecl_att_val;
if(({struct _fat_ptr _tmp8F3=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8F3,({const char*_tmp45B="format";_tag_fat(_tmp45B,sizeof(char),7U);}));})== 0 ||({struct _fat_ptr _tmp8F2=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp8F2,({const char*_tmp45C="__format__";_tag_fat(_tmp45C,sizeof(char),11U);}));})== 0){
if(({struct _fat_ptr _tmp8F5=(struct _fat_ptr)t;Cyc_zstrcmp(_tmp8F5,({const char*_tmp45D="printf";_tag_fat(_tmp45D,sizeof(char),7U);}));})== 0 ||({struct _fat_ptr _tmp8F4=(struct _fat_ptr)t;Cyc_zstrcmp(_tmp8F4,({const char*_tmp45E="__printf__";_tag_fat(_tmp45E,sizeof(char),11U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*_tmp45F=_cycalloc(sizeof(*_tmp45F));_tmp45F->tag=19U,_tmp45F->f1=Cyc_Absyn_Printf_ft,_tmp45F->f2=(int)n,_tmp45F->f3=(int)m;_tmp45F;});else{
if(({struct _fat_ptr _tmp8F7=(struct _fat_ptr)t;Cyc_zstrcmp(_tmp8F7,({const char*_tmp460="scanf";_tag_fat(_tmp460,sizeof(char),6U);}));})== 0 ||({struct _fat_ptr _tmp8F6=(struct _fat_ptr)t;Cyc_zstrcmp(_tmp8F6,({const char*_tmp461="__scanf__";_tag_fat(_tmp461,sizeof(char),10U);}));})== 0)
a=(void*)({struct Cyc_Absyn_Format_att_Absyn_Attribute_struct*_tmp462=_cycalloc(sizeof(*_tmp462));_tmp462->tag=19U,_tmp462->f1=Cyc_Absyn_Scanf_ft,_tmp462->f2=(int)n,_tmp462->f3=(int)m;_tmp462;});else{
# 1634
({void*_tmp463=0U;({unsigned _tmp8F9=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmp8F8=({const char*_tmp464="unrecognized format type";_tag_fat(_tmp464,sizeof(char),25U);});Cyc_Warn_err(_tmp8F9,_tmp8F8,_tag_fat(_tmp463,sizeof(void*),0U));});});}}}else{
# 1636
({void*_tmp465=0U;({unsigned _tmp8FB=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp8FA=({const char*_tmp466="unrecognized attribute";_tag_fat(_tmp466,sizeof(char),23U);});Cyc_Warn_err(_tmp8FB,_tmp8FA,_tag_fat(_tmp465,sizeof(void*),0U));});});}
yyval=Cyc_YY46(a);
# 1639
goto _LL0;}case 83U: _LLA1: _LLA2:
# 1648 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 84U: _LLA3: _LLA4:
# 1650 "parse.y"
 yyval=Cyc_YY21(({void*_tmp8FD=({struct _tuple0*_tmp8FC=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);Cyc_Absyn_typedef_type(_tmp8FC,Cyc_yyget_YY40(&(yyyvsp[1]).v),0,0);});Cyc_Parse_type_spec(_tmp8FD,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 85U: _LLA5: _LLA6:
# 1654 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_type_spec(Cyc_Absyn_void_type,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 86U: _LLA7: _LLA8:
# 1655 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_type_spec(Cyc_Absyn_char_type,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 87U: _LLA9: _LLAA:
# 1656 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_short_spec((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 88U: _LLAB: _LLAC:
# 1657 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_type_spec(Cyc_Absyn_sint_type,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 89U: _LLAD: _LLAE:
# 1658 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_long_spec((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 90U: _LLAF: _LLB0:
# 1659 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_type_spec(Cyc_Absyn_float_type,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 91U: _LLB1: _LLB2:
# 1660 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_type_spec(Cyc_Absyn_double_type,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 92U: _LLB3: _LLB4:
# 1661 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_signed_spec((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 93U: _LLB5: _LLB6:
# 1662 "parse.y"
 yyval=Cyc_YY21(Cyc_Parse_unsigned_spec((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 94U: _LLB7: _LLB8:
# 1663 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 95U: _LLB9: _LLBA:
# 1664 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 96U: _LLBB: _LLBC:
# 1667
 yyval=Cyc_YY21(({void*_tmp8FE=Cyc_Absyn_typeof_type(Cyc_yyget_Exp_tok(&(yyyvsp[2]).v));Cyc_Parse_type_spec(_tmp8FE,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 97U: _LLBD: _LLBE:
# 1669 "parse.y"
 yyval=Cyc_YY21(({void*_tmp8FF=Cyc_Absyn_builtin_type(({const char*_tmp467="__builtin_va_list";_tag_fat(_tmp467,sizeof(char),18U);}),& Cyc_Tcutil_bk);Cyc_Parse_type_spec(_tmp8FF,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 98U: _LLBF: _LLC0:
# 1671 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 99U: _LLC1: _LLC2:
# 1673 "parse.y"
 yyval=Cyc_YY21(({void*_tmp900=Cyc_yyget_YY44(&(yyyvsp[0]).v);Cyc_Parse_type_spec(_tmp900,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 100U: _LLC3: _LLC4:
# 1675 "parse.y"
 yyval=Cyc_YY21(({void*_tmp901=Cyc_Absyn_new_evar(0,0);Cyc_Parse_type_spec(_tmp901,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 101U: _LLC5: _LLC6:
# 1677 "parse.y"
 yyval=Cyc_YY21(({void*_tmp902=Cyc_Absyn_new_evar(Cyc_Tcutil_kind_to_opt(Cyc_yyget_YY43(&(yyyvsp[2]).v)),0);Cyc_Parse_type_spec(_tmp902,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 102U: _LLC7: _LLC8:
# 1679 "parse.y"
 yyval=Cyc_YY21(({void*_tmp905=(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp468=_cycalloc(sizeof(*_tmp468));_tmp468->tag=6U,({struct Cyc_List_List*_tmp904=({unsigned _tmp903=(unsigned)((yyyvsp[2]).l).first_line;((struct Cyc_List_List*(*)(struct _tuple20*(*f)(unsigned,struct _tuple8*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_get_tqual_typ,_tmp903,
((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY38(&(yyyvsp[2]).v)));});
# 1679
_tmp468->f1=_tmp904;});_tmp468;});Cyc_Parse_type_spec(_tmp905,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1682
goto _LL0;case 103U: _LLC9: _LLCA:
# 1683 "parse.y"
 yyval=Cyc_YY21(({void*_tmp906=Cyc_Absyn_rgn_handle_type(Cyc_yyget_YY44(&(yyyvsp[2]).v));Cyc_Parse_type_spec(_tmp906,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 104U: _LLCB: _LLCC:
# 1685 "parse.y"
 yyval=Cyc_YY21(({void*_tmp907=Cyc_Absyn_rgn_handle_type(Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,0));Cyc_Parse_type_spec(_tmp907,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1687
goto _LL0;case 105U: _LLCD: _LLCE:
# 1688 "parse.y"
 yyval=Cyc_YY21(({void*_tmp908=Cyc_Absyn_tag_type(Cyc_yyget_YY44(&(yyyvsp[2]).v));Cyc_Parse_type_spec(_tmp908,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 106U: _LLCF: _LLD0:
# 1690 "parse.y"
 yyval=Cyc_YY21(({void*_tmp909=Cyc_Absyn_tag_type(Cyc_Absyn_new_evar(& Cyc_Tcutil_iko,0));Cyc_Parse_type_spec(_tmp909,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 107U: _LLD1: _LLD2:
# 1692 "parse.y"
 yyval=Cyc_YY21(({void*_tmp90A=Cyc_Absyn_valueof_type(Cyc_yyget_Exp_tok(&(yyyvsp[2]).v));Cyc_Parse_type_spec(_tmp90A,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 108U: _LLD3: _LLD4:
# 1698 "parse.y"
 yyval=Cyc_YY43(({struct _fat_ptr _tmp90B=Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_Parse_id_to_kind(_tmp90B,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 109U: _LLD5: _LLD6: {
# 1702 "parse.y"
unsigned loc=(unsigned)(Cyc_Absyn_porting_c_code?((yyyvsp[0]).l).first_line:(int)0U);
yyval=Cyc_YY23(({struct Cyc_Absyn_Tqual _tmp737;_tmp737.print_const=1,_tmp737.q_volatile=0,_tmp737.q_restrict=0,_tmp737.real_const=1,_tmp737.loc=loc;_tmp737;}));
goto _LL0;}case 110U: _LLD7: _LLD8:
# 1704 "parse.y"
 yyval=Cyc_YY23(({struct Cyc_Absyn_Tqual _tmp738;_tmp738.print_const=0,_tmp738.q_volatile=1,_tmp738.q_restrict=0,_tmp738.real_const=0,_tmp738.loc=0U;_tmp738;}));
goto _LL0;case 111U: _LLD9: _LLDA:
# 1705 "parse.y"
 yyval=Cyc_YY23(({struct Cyc_Absyn_Tqual _tmp739;_tmp739.print_const=0,_tmp739.q_volatile=0,_tmp739.q_restrict=1,_tmp739.real_const=0,_tmp739.loc=0U;_tmp739;}));
goto _LL0;case 112U: _LLDB: _LLDC: {
# 1711 "parse.y"
struct Cyc_Absyn_TypeDecl*_tmp469=({struct Cyc_Absyn_TypeDecl*_tmp46E=_cycalloc(sizeof(*_tmp46E));({void*_tmp910=(void*)({struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*_tmp46D=_cycalloc(sizeof(*_tmp46D));_tmp46D->tag=1U,({struct Cyc_Absyn_Enumdecl*_tmp90F=({struct Cyc_Absyn_Enumdecl*_tmp46C=_cycalloc(sizeof(*_tmp46C));_tmp46C->sc=Cyc_Absyn_Public,({struct _tuple0*_tmp90E=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);_tmp46C->name=_tmp90E;}),({struct Cyc_Core_Opt*_tmp90D=({struct Cyc_Core_Opt*_tmp46B=_cycalloc(sizeof(*_tmp46B));({struct Cyc_List_List*_tmp90C=Cyc_yyget_YY48(&(yyyvsp[3]).v);_tmp46B->v=_tmp90C;});_tmp46B;});_tmp46C->fields=_tmp90D;});_tmp46C;});_tmp46D->f1=_tmp90F;});_tmp46D;});_tmp46E->r=_tmp910;}),_tmp46E->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp46E;});struct Cyc_Absyn_TypeDecl*ed=_tmp469;
# 1713
yyval=Cyc_YY21(({void*_tmp911=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp46A=_cycalloc(sizeof(*_tmp46A));_tmp46A->tag=10U,_tmp46A->f1=ed,_tmp46A->f2=0;_tmp46A;});Cyc_Parse_type_spec(_tmp911,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1715
goto _LL0;}case 113U: _LLDD: _LLDE:
# 1716 "parse.y"
 yyval=Cyc_YY21(({void*_tmp912=Cyc_Absyn_enum_type(Cyc_yyget_QualId_tok(&(yyyvsp[1]).v),0);Cyc_Parse_type_spec(_tmp912,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 114U: _LLDF: _LLE0:
# 1718 "parse.y"
 yyval=Cyc_YY21(({void*_tmp913=Cyc_Absyn_anon_enum_type(Cyc_yyget_YY48(&(yyyvsp[2]).v));Cyc_Parse_type_spec(_tmp913,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 115U: _LLE1: _LLE2:
# 1724 "parse.y"
 yyval=Cyc_YY47(({struct Cyc_Absyn_Enumfield*_tmp46F=_cycalloc(sizeof(*_tmp46F));({struct _tuple0*_tmp914=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp46F->name=_tmp914;}),_tmp46F->tag=0,_tmp46F->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp46F;}));
goto _LL0;case 116U: _LLE3: _LLE4:
# 1726 "parse.y"
 yyval=Cyc_YY47(({struct Cyc_Absyn_Enumfield*_tmp470=_cycalloc(sizeof(*_tmp470));({struct _tuple0*_tmp916=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp470->name=_tmp916;}),({struct Cyc_Absyn_Exp*_tmp915=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp470->tag=_tmp915;}),_tmp470->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp470;}));
goto _LL0;case 117U: _LLE5: _LLE6:
# 1730 "parse.y"
 yyval=Cyc_YY48(({struct Cyc_List_List*_tmp471=_cycalloc(sizeof(*_tmp471));({struct Cyc_Absyn_Enumfield*_tmp917=Cyc_yyget_YY47(&(yyyvsp[0]).v);_tmp471->hd=_tmp917;}),_tmp471->tl=0;_tmp471;}));
goto _LL0;case 118U: _LLE7: _LLE8:
# 1731 "parse.y"
 yyval=Cyc_YY48(({struct Cyc_List_List*_tmp472=_cycalloc(sizeof(*_tmp472));({struct Cyc_Absyn_Enumfield*_tmp918=Cyc_yyget_YY47(&(yyyvsp[0]).v);_tmp472->hd=_tmp918;}),_tmp472->tl=0;_tmp472;}));
goto _LL0;case 119U: _LLE9: _LLEA:
# 1732 "parse.y"
 yyval=Cyc_YY48(({struct Cyc_List_List*_tmp473=_cycalloc(sizeof(*_tmp473));({struct Cyc_Absyn_Enumfield*_tmp91A=Cyc_yyget_YY47(&(yyyvsp[0]).v);_tmp473->hd=_tmp91A;}),({struct Cyc_List_List*_tmp919=Cyc_yyget_YY48(&(yyyvsp[2]).v);_tmp473->tl=_tmp919;});_tmp473;}));
goto _LL0;case 120U: _LLEB: _LLEC:
# 1738 "parse.y"
 yyval=Cyc_YY21(({void*_tmp91D=(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_tmp474=_cycalloc(sizeof(*_tmp474));_tmp474->tag=7U,({enum Cyc_Absyn_AggrKind _tmp91C=Cyc_yyget_YY22(&(yyyvsp[0]).v);_tmp474->f1=_tmp91C;}),({struct Cyc_List_List*_tmp91B=Cyc_yyget_YY24(&(yyyvsp[2]).v);_tmp474->f2=_tmp91B;});_tmp474;});Cyc_Parse_type_spec(_tmp91D,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 121U: _LLED: _LLEE: {
# 1744 "parse.y"
struct Cyc_List_List*_tmp475=({unsigned _tmp91E=(unsigned)((yyyvsp[3]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmp91E,Cyc_yyget_YY40(&(yyyvsp[3]).v));});struct Cyc_List_List*ts=_tmp475;
struct Cyc_List_List*_tmp476=({unsigned _tmp91F=(unsigned)((yyyvsp[5]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmp91F,Cyc_yyget_YY40(&(yyyvsp[5]).v));});struct Cyc_List_List*exist_ts=_tmp476;
struct Cyc_Absyn_TypeDecl*_tmp477=({enum Cyc_Absyn_AggrKind _tmp925=Cyc_yyget_YY22(&(yyyvsp[1]).v);struct _tuple0*_tmp924=Cyc_yyget_QualId_tok(&(yyyvsp[2]).v);struct Cyc_List_List*_tmp923=ts;struct Cyc_Absyn_AggrdeclImpl*_tmp922=({
struct Cyc_List_List*_tmp921=exist_ts;struct Cyc_List_List*_tmp920=Cyc_yyget_YY50(&(yyyvsp[6]).v);Cyc_Absyn_aggrdecl_impl(_tmp921,_tmp920,Cyc_yyget_YY24(&(yyyvsp[7]).v),1);});
# 1746
Cyc_Absyn_aggr_tdecl(_tmp925,Cyc_Absyn_Public,_tmp924,_tmp923,_tmp922,0,(unsigned)((yyyvsp[0]).l).first_line);});struct Cyc_Absyn_TypeDecl*td=_tmp477;
# 1749
yyval=Cyc_YY21(({void*_tmp926=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp478=_cycalloc(sizeof(*_tmp478));_tmp478->tag=10U,_tmp478->f1=td,_tmp478->f2=0;_tmp478;});Cyc_Parse_type_spec(_tmp926,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1751
goto _LL0;}case 122U: _LLEF: _LLF0: {
# 1755 "parse.y"
struct Cyc_List_List*_tmp479=({unsigned _tmp927=(unsigned)((yyyvsp[2]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmp927,Cyc_yyget_YY40(&(yyyvsp[2]).v));});struct Cyc_List_List*ts=_tmp479;
struct Cyc_List_List*_tmp47A=({unsigned _tmp928=(unsigned)((yyyvsp[4]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmp928,Cyc_yyget_YY40(&(yyyvsp[4]).v));});struct Cyc_List_List*exist_ts=_tmp47A;
struct Cyc_Absyn_TypeDecl*_tmp47B=({enum Cyc_Absyn_AggrKind _tmp92E=Cyc_yyget_YY22(&(yyyvsp[0]).v);struct _tuple0*_tmp92D=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);struct Cyc_List_List*_tmp92C=ts;struct Cyc_Absyn_AggrdeclImpl*_tmp92B=({
struct Cyc_List_List*_tmp92A=exist_ts;struct Cyc_List_List*_tmp929=Cyc_yyget_YY50(&(yyyvsp[5]).v);Cyc_Absyn_aggrdecl_impl(_tmp92A,_tmp929,Cyc_yyget_YY24(&(yyyvsp[6]).v),0);});
# 1757
Cyc_Absyn_aggr_tdecl(_tmp92E,Cyc_Absyn_Public,_tmp92D,_tmp92C,_tmp92B,0,(unsigned)((yyyvsp[0]).l).first_line);});struct Cyc_Absyn_TypeDecl*td=_tmp47B;
# 1760
yyval=Cyc_YY21(({void*_tmp92F=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp47C=_cycalloc(sizeof(*_tmp47C));_tmp47C->tag=10U,_tmp47C->f1=td,_tmp47C->f2=0;_tmp47C;});Cyc_Parse_type_spec(_tmp92F,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1762
goto _LL0;}case 123U: _LLF1: _LLF2:
# 1763 "parse.y"
 yyval=Cyc_YY21(({void*_tmp933=({union Cyc_Absyn_AggrInfo _tmp932=({enum Cyc_Absyn_AggrKind _tmp931=Cyc_yyget_YY22(&(yyyvsp[1]).v);struct _tuple0*_tmp930=Cyc_yyget_QualId_tok(&(yyyvsp[2]).v);Cyc_Absyn_UnknownAggr(_tmp931,_tmp930,({struct Cyc_Core_Opt*_tmp47D=_cycalloc(sizeof(*_tmp47D));_tmp47D->v=(void*)1;_tmp47D;}));});Cyc_Absyn_aggr_type(_tmp932,Cyc_yyget_YY40(&(yyyvsp[3]).v));});Cyc_Parse_type_spec(_tmp933,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1766
goto _LL0;case 124U: _LLF3: _LLF4:
# 1767 "parse.y"
 yyval=Cyc_YY21(({void*_tmp936=({union Cyc_Absyn_AggrInfo _tmp935=({enum Cyc_Absyn_AggrKind _tmp934=Cyc_yyget_YY22(&(yyyvsp[0]).v);Cyc_Absyn_UnknownAggr(_tmp934,Cyc_yyget_QualId_tok(&(yyyvsp[1]).v),0);});Cyc_Absyn_aggr_type(_tmp935,Cyc_yyget_YY40(&(yyyvsp[2]).v));});Cyc_Parse_type_spec(_tmp936,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 125U: _LLF5: _LLF6:
# 1772 "parse.y"
 yyval=Cyc_YY40(0);
goto _LL0;case 126U: _LLF7: _LLF8:
# 1774 "parse.y"
 yyval=Cyc_YY40(((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY40(&(yyyvsp[1]).v)));
goto _LL0;case 127U: _LLF9: _LLFA:
# 1778 "parse.y"
 yyval=Cyc_YY22(Cyc_Absyn_StructA);
goto _LL0;case 128U: _LLFB: _LLFC:
# 1779 "parse.y"
 yyval=Cyc_YY22(Cyc_Absyn_UnionA);
goto _LL0;case 129U: _LLFD: _LLFE:
# 1784 "parse.y"
 yyval=Cyc_YY24(0);
goto _LL0;case 130U: _LLFF: _LL100: {
# 1786 "parse.y"
struct Cyc_List_List*decls=0;
{struct Cyc_List_List*_tmp47E=Cyc_yyget_YY25(&(yyyvsp[0]).v);struct Cyc_List_List*x=_tmp47E;for(0;x != 0;x=x->tl){
decls=((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)((struct Cyc_List_List*)x->hd,decls);}}{
# 1790
struct Cyc_List_List*_tmp47F=Cyc_Parse_get_aggrfield_tags(decls);struct Cyc_List_List*tags=_tmp47F;
if(tags != 0)
((void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c)(Cyc_Parse_substitute_aggrfield_tags,tags,decls);
yyval=Cyc_YY24(decls);
# 1795
goto _LL0;}}case 131U: _LL101: _LL102:
# 1800 "parse.y"
 yyval=Cyc_YY25(({struct Cyc_List_List*_tmp480=_cycalloc(sizeof(*_tmp480));({struct Cyc_List_List*_tmp937=Cyc_yyget_YY24(&(yyyvsp[0]).v);_tmp480->hd=_tmp937;}),_tmp480->tl=0;_tmp480;}));
goto _LL0;case 132U: _LL103: _LL104:
# 1802 "parse.y"
 yyval=Cyc_YY25(({struct Cyc_List_List*_tmp481=_cycalloc(sizeof(*_tmp481));({struct Cyc_List_List*_tmp939=Cyc_yyget_YY24(&(yyyvsp[1]).v);_tmp481->hd=_tmp939;}),({struct Cyc_List_List*_tmp938=Cyc_yyget_YY25(&(yyyvsp[0]).v);_tmp481->tl=_tmp938;});_tmp481;}));
goto _LL0;case 133U: _LL105: _LL106:
# 1806 "parse.y"
 yyval=Cyc_YY19(((struct _tuple13*(*)(struct _tuple13*x))Cyc_Parse_flat_imp_rev)(Cyc_yyget_YY19(&(yyyvsp[0]).v)));
goto _LL0;case 134U: _LL107: _LL108:
# 1812 "parse.y"
 yyval=Cyc_YY19(({struct _tuple13*_tmp482=_region_malloc(yyr,sizeof(*_tmp482));_tmp482->tl=0,({struct _tuple12 _tmp93A=Cyc_yyget_YY18(&(yyyvsp[0]).v);_tmp482->hd=_tmp93A;});_tmp482;}));
goto _LL0;case 135U: _LL109: _LL10A:
# 1814 "parse.y"
 yyval=Cyc_YY19(({struct _tuple13*_tmp483=_region_malloc(yyr,sizeof(*_tmp483));({struct _tuple13*_tmp93C=Cyc_yyget_YY19(&(yyyvsp[0]).v);_tmp483->tl=_tmp93C;}),({struct _tuple12 _tmp93B=Cyc_yyget_YY18(&(yyyvsp[2]).v);_tmp483->hd=_tmp93B;});_tmp483;}));
goto _LL0;case 136U: _LL10B: _LL10C:
# 1819 "parse.y"
 yyval=Cyc_YY18(({struct _tuple12 _tmp73A;({struct Cyc_Parse_Declarator _tmp93D=Cyc_yyget_YY27(&(yyyvsp[0]).v);_tmp73A.f1=_tmp93D;}),_tmp73A.f2=0;_tmp73A;}));
goto _LL0;case 137U: _LL10D: _LL10E:
# 1821 "parse.y"
 yyval=Cyc_YY18(({struct _tuple12 _tmp73B;({struct Cyc_Parse_Declarator _tmp93F=Cyc_yyget_YY27(&(yyyvsp[0]).v);_tmp73B.f1=_tmp93F;}),({struct Cyc_Absyn_Exp*_tmp93E=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp73B.f2=_tmp93E;});_tmp73B;}));
goto _LL0;case 138U: _LL10F: _LL110: {
# 1827 "parse.y"
struct _RegionHandle _tmp484=_new_region("temp");struct _RegionHandle*temp=& _tmp484;_push_region(temp);
{struct _tuple26 _tmp485=Cyc_yyget_YY35(&(yyyvsp[0]).v);struct _tuple26 _stmttmp19=_tmp485;struct _tuple26 _tmp486=_stmttmp19;struct Cyc_List_List*_tmp489;struct Cyc_Parse_Type_specifier _tmp488;struct Cyc_Absyn_Tqual _tmp487;_LL465: _tmp487=_tmp486.f1;_tmp488=_tmp486.f2;_tmp489=_tmp486.f3;_LL466: {struct Cyc_Absyn_Tqual tq=_tmp487;struct Cyc_Parse_Type_specifier tspecs=_tmp488;struct Cyc_List_List*atts=_tmp489;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{
struct _tuple11*decls=0;
struct Cyc_List_List*widths_and_reqs=0;
{struct Cyc_List_List*_tmp48A=Cyc_yyget_YY29(&(yyyvsp[1]).v);struct Cyc_List_List*x=_tmp48A;for(0;x != 0;x=x->tl){
struct _tuple25*_tmp48B=(struct _tuple25*)x->hd;struct _tuple25*_stmttmp1A=_tmp48B;struct _tuple25*_tmp48C=_stmttmp1A;struct Cyc_Absyn_Exp*_tmp48F;struct Cyc_Absyn_Exp*_tmp48E;struct Cyc_Parse_Declarator _tmp48D;_LL468: _tmp48D=_tmp48C->f1;_tmp48E=_tmp48C->f2;_tmp48F=_tmp48C->f3;_LL469: {struct Cyc_Parse_Declarator d=_tmp48D;struct Cyc_Absyn_Exp*wd=_tmp48E;struct Cyc_Absyn_Exp*wh=_tmp48F;
decls=({struct _tuple11*_tmp490=_region_malloc(temp,sizeof(*_tmp490));_tmp490->tl=decls,_tmp490->hd=d;_tmp490;});
widths_and_reqs=({struct Cyc_List_List*_tmp492=_region_malloc(temp,sizeof(*_tmp492));
({struct _tuple17*_tmp940=({struct _tuple17*_tmp491=_region_malloc(temp,sizeof(*_tmp491));_tmp491->f1=wd,_tmp491->f2=wh;_tmp491;});_tmp492->hd=_tmp940;}),_tmp492->tl=widths_and_reqs;_tmp492;});}}}
# 1838
decls=((struct _tuple11*(*)(struct _tuple11*x))Cyc_Parse_flat_imp_rev)(decls);
widths_and_reqs=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(widths_and_reqs);{
void*_tmp493=Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);void*t=_tmp493;
struct Cyc_List_List*_tmp494=({struct _RegionHandle*_tmp943=temp;struct _RegionHandle*_tmp942=temp;struct Cyc_List_List*_tmp941=
Cyc_Parse_apply_tmss(temp,tq,t,decls,atts);
# 1841
((struct Cyc_List_List*(*)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_rzip)(_tmp943,_tmp942,_tmp941,widths_and_reqs);});struct Cyc_List_List*info=_tmp494;
# 1844
yyval=Cyc_YY24(((struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*f)(unsigned,struct _tuple18*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_make_aggr_field,(unsigned)((yyyvsp[0]).l).first_line,info));
# 1846
_npop_handler(0U);goto _LL0;}}}}
# 1828
;_pop_region();}case 139U: _LL111: _LL112:
# 1854 "parse.y"
 yyval=Cyc_YY35(({struct _tuple26 _tmp73C;({struct Cyc_Absyn_Tqual _tmp945=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp73C.f1=_tmp945;}),({struct Cyc_Parse_Type_specifier _tmp944=Cyc_yyget_YY21(&(yyyvsp[0]).v);_tmp73C.f2=_tmp944;}),_tmp73C.f3=0;_tmp73C;}));
goto _LL0;case 140U: _LL113: _LL114: {
# 1856 "parse.y"
struct _tuple26 _tmp495=Cyc_yyget_YY35(&(yyyvsp[1]).v);struct _tuple26 two=_tmp495;yyval=Cyc_YY35(({struct _tuple26 _tmp73D;_tmp73D.f1=two.f1,({struct Cyc_Parse_Type_specifier _tmp948=({unsigned _tmp947=(unsigned)((yyyvsp[0]).l).first_line;struct Cyc_Parse_Type_specifier _tmp946=Cyc_yyget_YY21(&(yyyvsp[0]).v);Cyc_Parse_combine_specifiers(_tmp947,_tmp946,two.f2);});_tmp73D.f2=_tmp948;}),_tmp73D.f3=two.f3;_tmp73D;}));
goto _LL0;}case 141U: _LL115: _LL116:
# 1858 "parse.y"
 yyval=Cyc_YY35(({struct _tuple26 _tmp73E;({struct Cyc_Absyn_Tqual _tmp94A=Cyc_yyget_YY23(&(yyyvsp[0]).v);_tmp73E.f1=_tmp94A;}),({struct Cyc_Parse_Type_specifier _tmp949=Cyc_Parse_empty_spec(0U);_tmp73E.f2=_tmp949;}),_tmp73E.f3=0;_tmp73E;}));
goto _LL0;case 142U: _LL117: _LL118: {
# 1860 "parse.y"
struct _tuple26 _tmp496=Cyc_yyget_YY35(&(yyyvsp[1]).v);struct _tuple26 two=_tmp496;
yyval=Cyc_YY35(({struct _tuple26 _tmp73F;({struct Cyc_Absyn_Tqual _tmp94C=({struct Cyc_Absyn_Tqual _tmp94B=Cyc_yyget_YY23(&(yyyvsp[0]).v);Cyc_Absyn_combine_tqual(_tmp94B,two.f1);});_tmp73F.f1=_tmp94C;}),_tmp73F.f2=two.f2,_tmp73F.f3=two.f3;_tmp73F;}));
goto _LL0;}case 143U: _LL119: _LL11A:
# 1863 "parse.y"
 yyval=Cyc_YY35(({struct _tuple26 _tmp740;({struct Cyc_Absyn_Tqual _tmp94F=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp740.f1=_tmp94F;}),({struct Cyc_Parse_Type_specifier _tmp94E=Cyc_Parse_empty_spec(0U);_tmp740.f2=_tmp94E;}),({struct Cyc_List_List*_tmp94D=Cyc_yyget_YY45(&(yyyvsp[0]).v);_tmp740.f3=_tmp94D;});_tmp740;}));
goto _LL0;case 144U: _LL11B: _LL11C: {
# 1865 "parse.y"
struct _tuple26 _tmp497=Cyc_yyget_YY35(&(yyyvsp[1]).v);struct _tuple26 two=_tmp497;yyval=Cyc_YY35(({struct _tuple26 _tmp741;_tmp741.f1=two.f1,_tmp741.f2=two.f2,({struct Cyc_List_List*_tmp951=({struct Cyc_List_List*_tmp950=Cyc_yyget_YY45(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(_tmp950,two.f3);});_tmp741.f3=_tmp951;});_tmp741;}));
goto _LL0;}case 145U: _LL11D: _LL11E:
# 1871 "parse.y"
 yyval=Cyc_YY35(({struct _tuple26 _tmp742;({struct Cyc_Absyn_Tqual _tmp953=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp742.f1=_tmp953;}),({struct Cyc_Parse_Type_specifier _tmp952=Cyc_yyget_YY21(&(yyyvsp[0]).v);_tmp742.f2=_tmp952;}),_tmp742.f3=0;_tmp742;}));
goto _LL0;case 146U: _LL11F: _LL120: {
# 1873 "parse.y"
struct _tuple26 _tmp498=Cyc_yyget_YY35(&(yyyvsp[1]).v);struct _tuple26 two=_tmp498;yyval=Cyc_YY35(({struct _tuple26 _tmp743;_tmp743.f1=two.f1,({struct Cyc_Parse_Type_specifier _tmp956=({unsigned _tmp955=(unsigned)((yyyvsp[0]).l).first_line;struct Cyc_Parse_Type_specifier _tmp954=Cyc_yyget_YY21(&(yyyvsp[0]).v);Cyc_Parse_combine_specifiers(_tmp955,_tmp954,two.f2);});_tmp743.f2=_tmp956;}),_tmp743.f3=two.f3;_tmp743;}));
goto _LL0;}case 147U: _LL121: _LL122:
# 1875 "parse.y"
 yyval=Cyc_YY35(({struct _tuple26 _tmp744;({struct Cyc_Absyn_Tqual _tmp958=Cyc_yyget_YY23(&(yyyvsp[0]).v);_tmp744.f1=_tmp958;}),({struct Cyc_Parse_Type_specifier _tmp957=Cyc_Parse_empty_spec(0U);_tmp744.f2=_tmp957;}),_tmp744.f3=0;_tmp744;}));
goto _LL0;case 148U: _LL123: _LL124: {
# 1877 "parse.y"
struct _tuple26 _tmp499=Cyc_yyget_YY35(&(yyyvsp[1]).v);struct _tuple26 two=_tmp499;
yyval=Cyc_YY35(({struct _tuple26 _tmp745;({struct Cyc_Absyn_Tqual _tmp95A=({struct Cyc_Absyn_Tqual _tmp959=Cyc_yyget_YY23(&(yyyvsp[0]).v);Cyc_Absyn_combine_tqual(_tmp959,two.f1);});_tmp745.f1=_tmp95A;}),_tmp745.f2=two.f2,_tmp745.f3=two.f3;_tmp745;}));
goto _LL0;}case 149U: _LL125: _LL126:
# 1880 "parse.y"
 yyval=Cyc_YY35(({struct _tuple26 _tmp746;({struct Cyc_Absyn_Tqual _tmp95D=Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);_tmp746.f1=_tmp95D;}),({struct Cyc_Parse_Type_specifier _tmp95C=Cyc_Parse_empty_spec(0U);_tmp746.f2=_tmp95C;}),({struct Cyc_List_List*_tmp95B=Cyc_yyget_YY45(&(yyyvsp[0]).v);_tmp746.f3=_tmp95B;});_tmp746;}));
goto _LL0;case 150U: _LL127: _LL128: {
# 1882 "parse.y"
struct _tuple26 _tmp49A=Cyc_yyget_YY35(&(yyyvsp[1]).v);struct _tuple26 two=_tmp49A;yyval=Cyc_YY35(({struct _tuple26 _tmp747;_tmp747.f1=two.f1,_tmp747.f2=two.f2,({struct Cyc_List_List*_tmp95F=({struct Cyc_List_List*_tmp95E=Cyc_yyget_YY45(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_append)(_tmp95E,two.f3);});_tmp747.f3=_tmp95F;});_tmp747;}));
goto _LL0;}case 151U: _LL129: _LL12A:
# 1886 "parse.y"
 yyval=Cyc_YY29(((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY29(&(yyyvsp[0]).v)));
goto _LL0;case 152U: _LL12B: _LL12C:
# 1892 "parse.y"
 yyval=Cyc_YY29(({struct Cyc_List_List*_tmp49B=_region_malloc(yyr,sizeof(*_tmp49B));({struct _tuple25*_tmp960=Cyc_yyget_YY28(&(yyyvsp[0]).v);_tmp49B->hd=_tmp960;}),_tmp49B->tl=0;_tmp49B;}));
goto _LL0;case 153U: _LL12D: _LL12E:
# 1894 "parse.y"
 yyval=Cyc_YY29(({struct Cyc_List_List*_tmp49C=_region_malloc(yyr,sizeof(*_tmp49C));({struct _tuple25*_tmp962=Cyc_yyget_YY28(&(yyyvsp[2]).v);_tmp49C->hd=_tmp962;}),({struct Cyc_List_List*_tmp961=Cyc_yyget_YY29(&(yyyvsp[0]).v);_tmp49C->tl=_tmp961;});_tmp49C;}));
goto _LL0;case 154U: _LL12F: _LL130:
# 1899 "parse.y"
 yyval=Cyc_YY28(({struct _tuple25*_tmp49D=_region_malloc(yyr,sizeof(*_tmp49D));({struct Cyc_Parse_Declarator _tmp964=Cyc_yyget_YY27(&(yyyvsp[0]).v);_tmp49D->f1=_tmp964;}),_tmp49D->f2=0,({struct Cyc_Absyn_Exp*_tmp963=Cyc_yyget_YY57(&(yyyvsp[1]).v);_tmp49D->f3=_tmp963;});_tmp49D;}));
goto _LL0;case 155U: _LL131: _LL132:
# 1903 "parse.y"
 yyval=Cyc_YY28(({struct _tuple25*_tmp4A1=_region_malloc(yyr,sizeof(*_tmp4A1));({struct _tuple0*_tmp969=({struct _tuple0*_tmp4A0=_cycalloc(sizeof(*_tmp4A0));({union Cyc_Absyn_Nmspace _tmp968=Cyc_Absyn_Rel_n(0);_tmp4A0->f1=_tmp968;}),({struct _fat_ptr*_tmp967=({struct _fat_ptr*_tmp49F=_cycalloc(sizeof(*_tmp49F));({struct _fat_ptr _tmp966=({const char*_tmp49E="";_tag_fat(_tmp49E,sizeof(char),1U);});*_tmp49F=_tmp966;});_tmp49F;});_tmp4A0->f2=_tmp967;});_tmp4A0;});(_tmp4A1->f1).id=_tmp969;}),(_tmp4A1->f1).varloc=0U,(_tmp4A1->f1).tms=0,({struct Cyc_Absyn_Exp*_tmp965=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);_tmp4A1->f2=_tmp965;}),_tmp4A1->f3=0;_tmp4A1;}));
# 1905
goto _LL0;case 156U: _LL133: _LL134:
# 1908 "parse.y"
 yyval=Cyc_YY28(({struct _tuple25*_tmp4A5=_region_malloc(yyr,sizeof(*_tmp4A5));({struct _tuple0*_tmp96D=({struct _tuple0*_tmp4A4=_cycalloc(sizeof(*_tmp4A4));({union Cyc_Absyn_Nmspace _tmp96C=Cyc_Absyn_Rel_n(0);_tmp4A4->f1=_tmp96C;}),({struct _fat_ptr*_tmp96B=({struct _fat_ptr*_tmp4A3=_cycalloc(sizeof(*_tmp4A3));({struct _fat_ptr _tmp96A=({const char*_tmp4A2="";_tag_fat(_tmp4A2,sizeof(char),1U);});*_tmp4A3=_tmp96A;});_tmp4A3;});_tmp4A4->f2=_tmp96B;});_tmp4A4;});(_tmp4A5->f1).id=_tmp96D;}),(_tmp4A5->f1).varloc=0U,(_tmp4A5->f1).tms=0,_tmp4A5->f2=0,_tmp4A5->f3=0;_tmp4A5;}));
# 1910
goto _LL0;case 157U: _LL135: _LL136:
# 1911 "parse.y"
 yyval=Cyc_YY28(({struct _tuple25*_tmp4A6=_region_malloc(yyr,sizeof(*_tmp4A6));({struct Cyc_Parse_Declarator _tmp96F=Cyc_yyget_YY27(&(yyyvsp[0]).v);_tmp4A6->f1=_tmp96F;}),({struct Cyc_Absyn_Exp*_tmp96E=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp4A6->f2=_tmp96E;}),_tmp4A6->f3=0;_tmp4A6;}));
goto _LL0;case 158U: _LL137: _LL138:
# 1915 "parse.y"
 yyval=Cyc_YY57(Cyc_yyget_Exp_tok(&(yyyvsp[2]).v));
goto _LL0;case 159U: _LL139: _LL13A:
# 1916 "parse.y"
 yyval=Cyc_YY57(0);
goto _LL0;case 160U: _LL13B: _LL13C:
# 1920 "parse.y"
 yyval=Cyc_YY57(Cyc_yyget_Exp_tok(&(yyyvsp[2]).v));
goto _LL0;case 161U: _LL13D: _LL13E:
# 1921 "parse.y"
 yyval=Cyc_YY57(0);
goto _LL0;case 162U: _LL13F: _LL140: {
# 1927 "parse.y"
int _tmp4A7=Cyc_yyget_YY31(&(yyyvsp[0]).v);int is_extensible=_tmp4A7;
struct Cyc_List_List*_tmp4A8=({unsigned _tmp970=(unsigned)((yyyvsp[2]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmp970,Cyc_yyget_YY40(&(yyyvsp[2]).v));});struct Cyc_List_List*ts=_tmp4A8;
struct Cyc_Absyn_TypeDecl*_tmp4A9=({struct _tuple0*_tmp975=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);struct Cyc_List_List*_tmp974=ts;struct Cyc_Core_Opt*_tmp973=({struct Cyc_Core_Opt*_tmp4AB=_cycalloc(sizeof(*_tmp4AB));({struct Cyc_List_List*_tmp971=Cyc_yyget_YY34(&(yyyvsp[4]).v);_tmp4AB->v=_tmp971;});_tmp4AB;});int _tmp972=is_extensible;Cyc_Absyn_datatype_tdecl(Cyc_Absyn_Public,_tmp975,_tmp974,_tmp973,_tmp972,(unsigned)((yyyvsp[0]).l).first_line);});struct Cyc_Absyn_TypeDecl*dd=_tmp4A9;
# 1931
yyval=Cyc_YY21(({void*_tmp976=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp4AA=_cycalloc(sizeof(*_tmp4AA));_tmp4AA->tag=10U,_tmp4AA->f1=dd,_tmp4AA->f2=0;_tmp4AA;});Cyc_Parse_type_spec(_tmp976,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1933
goto _LL0;}case 163U: _LL141: _LL142: {
# 1934 "parse.y"
int _tmp4AC=Cyc_yyget_YY31(&(yyyvsp[0]).v);int is_extensible=_tmp4AC;
yyval=Cyc_YY21(({void*_tmp979=({union Cyc_Absyn_DatatypeInfo _tmp978=Cyc_Absyn_UnknownDatatype(({struct Cyc_Absyn_UnknownDatatypeInfo _tmp748;({struct _tuple0*_tmp977=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);_tmp748.name=_tmp977;}),_tmp748.is_extensible=is_extensible;_tmp748;}));Cyc_Absyn_datatype_type(_tmp978,Cyc_yyget_YY40(&(yyyvsp[2]).v));});Cyc_Parse_type_spec(_tmp979,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1937
goto _LL0;}case 164U: _LL143: _LL144: {
# 1938 "parse.y"
int _tmp4AD=Cyc_yyget_YY31(&(yyyvsp[0]).v);int is_extensible=_tmp4AD;
yyval=Cyc_YY21(({void*_tmp97D=({union Cyc_Absyn_DatatypeFieldInfo _tmp97C=Cyc_Absyn_UnknownDatatypefield(({struct Cyc_Absyn_UnknownDatatypeFieldInfo _tmp749;({struct _tuple0*_tmp97B=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);_tmp749.datatype_name=_tmp97B;}),({struct _tuple0*_tmp97A=Cyc_yyget_QualId_tok(&(yyyvsp[3]).v);_tmp749.field_name=_tmp97A;}),_tmp749.is_extensible=is_extensible;_tmp749;}));Cyc_Absyn_datatype_field_type(_tmp97C,Cyc_yyget_YY40(&(yyyvsp[4]).v));});Cyc_Parse_type_spec(_tmp97D,(unsigned)((yyyvsp[0]).l).first_line);}));
# 1941
goto _LL0;}case 165U: _LL145: _LL146:
# 1944 "parse.y"
 yyval=Cyc_YY31(0);
goto _LL0;case 166U: _LL147: _LL148:
# 1945 "parse.y"
 yyval=Cyc_YY31(1);
goto _LL0;case 167U: _LL149: _LL14A:
# 1949 "parse.y"
 yyval=Cyc_YY34(({struct Cyc_List_List*_tmp4AE=_cycalloc(sizeof(*_tmp4AE));({struct Cyc_Absyn_Datatypefield*_tmp97E=Cyc_yyget_YY33(&(yyyvsp[0]).v);_tmp4AE->hd=_tmp97E;}),_tmp4AE->tl=0;_tmp4AE;}));
goto _LL0;case 168U: _LL14B: _LL14C:
# 1950 "parse.y"
 yyval=Cyc_YY34(({struct Cyc_List_List*_tmp4AF=_cycalloc(sizeof(*_tmp4AF));({struct Cyc_Absyn_Datatypefield*_tmp97F=Cyc_yyget_YY33(&(yyyvsp[0]).v);_tmp4AF->hd=_tmp97F;}),_tmp4AF->tl=0;_tmp4AF;}));
goto _LL0;case 169U: _LL14D: _LL14E:
# 1951 "parse.y"
 yyval=Cyc_YY34(({struct Cyc_List_List*_tmp4B0=_cycalloc(sizeof(*_tmp4B0));({struct Cyc_Absyn_Datatypefield*_tmp981=Cyc_yyget_YY33(&(yyyvsp[0]).v);_tmp4B0->hd=_tmp981;}),({struct Cyc_List_List*_tmp980=Cyc_yyget_YY34(&(yyyvsp[2]).v);_tmp4B0->tl=_tmp980;});_tmp4B0;}));
goto _LL0;case 170U: _LL14F: _LL150:
# 1952 "parse.y"
 yyval=Cyc_YY34(({struct Cyc_List_List*_tmp4B1=_cycalloc(sizeof(*_tmp4B1));({struct Cyc_Absyn_Datatypefield*_tmp983=Cyc_yyget_YY33(&(yyyvsp[0]).v);_tmp4B1->hd=_tmp983;}),({struct Cyc_List_List*_tmp982=Cyc_yyget_YY34(&(yyyvsp[2]).v);_tmp4B1->tl=_tmp982;});_tmp4B1;}));
goto _LL0;case 171U: _LL151: _LL152:
# 1956 "parse.y"
 yyval=Cyc_YY32(Cyc_Absyn_Public);
goto _LL0;case 172U: _LL153: _LL154:
# 1957 "parse.y"
 yyval=Cyc_YY32(Cyc_Absyn_Extern);
goto _LL0;case 173U: _LL155: _LL156:
# 1958 "parse.y"
 yyval=Cyc_YY32(Cyc_Absyn_Static);
goto _LL0;case 174U: _LL157: _LL158:
# 1962 "parse.y"
 yyval=Cyc_YY33(({struct Cyc_Absyn_Datatypefield*_tmp4B2=_cycalloc(sizeof(*_tmp4B2));({struct _tuple0*_tmp985=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);_tmp4B2->name=_tmp985;}),_tmp4B2->typs=0,_tmp4B2->loc=(unsigned)((yyyvsp[0]).l).first_line,({enum Cyc_Absyn_Scope _tmp984=Cyc_yyget_YY32(&(yyyvsp[0]).v);_tmp4B2->sc=_tmp984;});_tmp4B2;}));
goto _LL0;case 175U: _LL159: _LL15A: {
# 1964 "parse.y"
struct Cyc_List_List*_tmp4B3=({unsigned _tmp986=(unsigned)((yyyvsp[3]).l).first_line;((struct Cyc_List_List*(*)(struct _tuple20*(*f)(unsigned,struct _tuple8*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_get_tqual_typ,_tmp986,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY38(&(yyyvsp[3]).v)));});struct Cyc_List_List*typs=_tmp4B3;
yyval=Cyc_YY33(({struct Cyc_Absyn_Datatypefield*_tmp4B4=_cycalloc(sizeof(*_tmp4B4));({struct _tuple0*_tmp988=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);_tmp4B4->name=_tmp988;}),_tmp4B4->typs=typs,_tmp4B4->loc=(unsigned)((yyyvsp[0]).l).first_line,({enum Cyc_Absyn_Scope _tmp987=Cyc_yyget_YY32(&(yyyvsp[0]).v);_tmp4B4->sc=_tmp987;});_tmp4B4;}));
goto _LL0;}case 176U: _LL15B: _LL15C:
# 1970 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 177U: _LL15D: _LL15E: {
# 1972 "parse.y"
struct Cyc_Parse_Declarator _tmp4B5=Cyc_yyget_YY27(&(yyyvsp[1]).v);struct Cyc_Parse_Declarator two=_tmp4B5;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp74A;_tmp74A.id=two.id,_tmp74A.varloc=two.varloc,({struct Cyc_List_List*_tmp98A=({struct Cyc_List_List*_tmp989=Cyc_yyget_YY26(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp989,two.tms);});_tmp74A.tms=_tmp98A;});_tmp74A;}));
goto _LL0;}case 178U: _LL15F: _LL160:
# 1979 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 179U: _LL161: _LL162: {
# 1981 "parse.y"
struct Cyc_Parse_Declarator _tmp4B6=Cyc_yyget_YY27(&(yyyvsp[1]).v);struct Cyc_Parse_Declarator two=_tmp4B6;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp74B;_tmp74B.id=two.id,_tmp74B.varloc=two.varloc,({struct Cyc_List_List*_tmp98C=({struct Cyc_List_List*_tmp98B=Cyc_yyget_YY26(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp98B,two.tms);});_tmp74B.tms=_tmp98C;});_tmp74B;}));
goto _LL0;}case 180U: _LL163: _LL164:
# 1987 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp74C;({struct _tuple0*_tmp98D=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp74C.id=_tmp98D;}),_tmp74C.varloc=(unsigned)((yyyvsp[0]).l).first_line,_tmp74C.tms=0;_tmp74C;}));
goto _LL0;case 181U: _LL165: _LL166:
# 1989 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 182U: _LL167: _LL168: {
# 1993 "parse.y"
struct Cyc_Parse_Declarator _tmp4B7=Cyc_yyget_YY27(&(yyyvsp[2]).v);struct Cyc_Parse_Declarator d=_tmp4B7;
({struct Cyc_List_List*_tmp990=({struct Cyc_List_List*_tmp4B9=_region_malloc(yyr,sizeof(*_tmp4B9));({void*_tmp98F=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp4B8=_region_malloc(yyr,sizeof(*_tmp4B8));_tmp4B8->tag=5U,_tmp4B8->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp98E=Cyc_yyget_YY45(&(yyyvsp[1]).v);_tmp4B8->f2=_tmp98E;});_tmp4B8;});_tmp4B9->hd=_tmp98F;}),_tmp4B9->tl=d.tms;_tmp4B9;});d.tms=_tmp990;});
yyval=(yyyvsp[2]).v;
# 1997
goto _LL0;}case 183U: _LL169: _LL16A:
# 1998 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp74D;({struct _tuple0*_tmp996=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).id;_tmp74D.id=_tmp996;}),({unsigned _tmp995=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).varloc;_tmp74D.varloc=_tmp995;}),({struct Cyc_List_List*_tmp994=({struct Cyc_List_List*_tmp4BB=_region_malloc(yyr,sizeof(*_tmp4BB));({void*_tmp993=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp4BA=_region_malloc(yyr,sizeof(*_tmp4BA));_tmp4BA->tag=0U,({void*_tmp992=Cyc_yyget_YY51(&(yyyvsp[3]).v);_tmp4BA->f1=_tmp992;}),_tmp4BA->f2=(unsigned)((yyyvsp[3]).l).first_line;_tmp4BA;});_tmp4BB->hd=_tmp993;}),({struct Cyc_List_List*_tmp991=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).tms;_tmp4BB->tl=_tmp991;});_tmp4BB;});_tmp74D.tms=_tmp994;});_tmp74D;}));
goto _LL0;case 184U: _LL16B: _LL16C:
# 2000 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp74E;({struct _tuple0*_tmp99D=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).id;_tmp74E.id=_tmp99D;}),({unsigned _tmp99C=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).varloc;_tmp74E.varloc=_tmp99C;}),({
struct Cyc_List_List*_tmp99B=({struct Cyc_List_List*_tmp4BD=_region_malloc(yyr,sizeof(*_tmp4BD));({void*_tmp99A=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp4BC=_region_malloc(yyr,sizeof(*_tmp4BC));_tmp4BC->tag=1U,({struct Cyc_Absyn_Exp*_tmp999=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp4BC->f1=_tmp999;}),({void*_tmp998=Cyc_yyget_YY51(&(yyyvsp[4]).v);_tmp4BC->f2=_tmp998;}),_tmp4BC->f3=(unsigned)((yyyvsp[4]).l).first_line;_tmp4BC;});_tmp4BD->hd=_tmp99A;}),({struct Cyc_List_List*_tmp997=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).tms;_tmp4BD->tl=_tmp997;});_tmp4BD;});_tmp74E.tms=_tmp99B;});_tmp74E;}));
goto _LL0;case 185U: _LL16D: _LL16E: {
# 2003 "parse.y"
struct _tuple27*_tmp4BE=Cyc_yyget_YY39(&(yyyvsp[2]).v);struct _tuple27*_stmttmp1B=_tmp4BE;struct _tuple27*_tmp4BF=_stmttmp1B;struct Cyc_List_List*_tmp4C4;void*_tmp4C3;struct Cyc_Absyn_VarargInfo*_tmp4C2;int _tmp4C1;struct Cyc_List_List*_tmp4C0;_LL46B: _tmp4C0=_tmp4BF->f1;_tmp4C1=_tmp4BF->f2;_tmp4C2=_tmp4BF->f3;_tmp4C3=_tmp4BF->f4;_tmp4C4=_tmp4BF->f5;_LL46C: {struct Cyc_List_List*lis=_tmp4C0;int b=_tmp4C1;struct Cyc_Absyn_VarargInfo*c=_tmp4C2;void*eff=_tmp4C3;struct Cyc_List_List*po=_tmp4C4;
struct Cyc_Absyn_Exp*_tmp4C5=Cyc_yyget_YY57(&(yyyvsp[4]).v);struct Cyc_Absyn_Exp*req=_tmp4C5;
struct Cyc_Absyn_Exp*_tmp4C6=Cyc_yyget_YY57(&(yyyvsp[5]).v);struct Cyc_Absyn_Exp*ens=_tmp4C6;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp74F;({struct _tuple0*_tmp9A3=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).id;_tmp74F.id=_tmp9A3;}),({unsigned _tmp9A2=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).varloc;_tmp74F.varloc=_tmp9A2;}),({struct Cyc_List_List*_tmp9A1=({struct Cyc_List_List*_tmp4C9=_region_malloc(yyr,sizeof(*_tmp4C9));({void*_tmp9A0=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp4C8=_region_malloc(yyr,sizeof(*_tmp4C8));_tmp4C8->tag=3U,({void*_tmp99F=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp4C7=_region_malloc(yyr,sizeof(*_tmp4C7));_tmp4C7->tag=1U,_tmp4C7->f1=lis,_tmp4C7->f2=b,_tmp4C7->f3=c,_tmp4C7->f4=eff,_tmp4C7->f5=po,_tmp4C7->f6=req,_tmp4C7->f7=ens;_tmp4C7;});_tmp4C8->f1=_tmp99F;});_tmp4C8;});_tmp4C9->hd=_tmp9A0;}),({struct Cyc_List_List*_tmp99E=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).tms;_tmp4C9->tl=_tmp99E;});_tmp4C9;});_tmp74F.tms=_tmp9A1;});_tmp74F;}));
# 2008
goto _LL0;}}case 186U: _LL16F: _LL170:
# 2009 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp750;({struct _tuple0*_tmp9AD=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).id;_tmp750.id=_tmp9AD;}),({unsigned _tmp9AC=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).varloc;_tmp750.varloc=_tmp9AC;}),({
struct Cyc_List_List*_tmp9AB=({struct Cyc_List_List*_tmp4CC=_region_malloc(yyr,sizeof(*_tmp4CC));({void*_tmp9AA=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp4CB=_region_malloc(yyr,sizeof(*_tmp4CB));_tmp4CB->tag=3U,({void*_tmp9A9=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp4CA=_region_malloc(yyr,sizeof(*_tmp4CA));_tmp4CA->tag=1U,_tmp4CA->f1=0,_tmp4CA->f2=0,_tmp4CA->f3=0,({
# 2012
void*_tmp9A8=Cyc_yyget_YY49(&(yyyvsp[2]).v);_tmp4CA->f4=_tmp9A8;}),({struct Cyc_List_List*_tmp9A7=Cyc_yyget_YY50(&(yyyvsp[3]).v);_tmp4CA->f5=_tmp9A7;}),({struct Cyc_Absyn_Exp*_tmp9A6=Cyc_yyget_YY57(&(yyyvsp[5]).v);_tmp4CA->f6=_tmp9A6;}),({struct Cyc_Absyn_Exp*_tmp9A5=Cyc_yyget_YY57(&(yyyvsp[6]).v);_tmp4CA->f7=_tmp9A5;});_tmp4CA;});
# 2010
_tmp4CB->f1=_tmp9A9;});_tmp4CB;});_tmp4CC->hd=_tmp9AA;}),({
# 2013
struct Cyc_List_List*_tmp9A4=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).tms;_tmp4CC->tl=_tmp9A4;});_tmp4CC;});
# 2010
_tmp750.tms=_tmp9AB;});_tmp750;}));
# 2015
goto _LL0;case 187U: _LL171: _LL172:
# 2016 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp751;({struct _tuple0*_tmp9B4=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).id;_tmp751.id=_tmp9B4;}),({unsigned _tmp9B3=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).varloc;_tmp751.varloc=_tmp9B3;}),({struct Cyc_List_List*_tmp9B2=({struct Cyc_List_List*_tmp4CF=_region_malloc(yyr,sizeof(*_tmp4CF));({void*_tmp9B1=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp4CE=_region_malloc(yyr,sizeof(*_tmp4CE));_tmp4CE->tag=3U,({void*_tmp9B0=(void*)({struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*_tmp4CD=_region_malloc(yyr,sizeof(*_tmp4CD));_tmp4CD->tag=0U,({struct Cyc_List_List*_tmp9AF=Cyc_yyget_YY36(&(yyyvsp[2]).v);_tmp4CD->f1=_tmp9AF;}),_tmp4CD->f2=(unsigned)((yyyvsp[0]).l).first_line;_tmp4CD;});_tmp4CE->f1=_tmp9B0;});_tmp4CE;});_tmp4CF->hd=_tmp9B1;}),({struct Cyc_List_List*_tmp9AE=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).tms;_tmp4CF->tl=_tmp9AE;});_tmp4CF;});_tmp751.tms=_tmp9B2;});_tmp751;}));
goto _LL0;case 188U: _LL173: _LL174: {
# 2019
struct Cyc_List_List*_tmp4D0=({unsigned _tmp9B5=(unsigned)((yyyvsp[1]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmp9B5,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY40(&(yyyvsp[2]).v)));});struct Cyc_List_List*ts=_tmp4D0;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp752;({struct _tuple0*_tmp9BA=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).id;_tmp752.id=_tmp9BA;}),({unsigned _tmp9B9=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).varloc;_tmp752.varloc=_tmp9B9;}),({struct Cyc_List_List*_tmp9B8=({struct Cyc_List_List*_tmp4D2=_region_malloc(yyr,sizeof(*_tmp4D2));({void*_tmp9B7=(void*)({struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_tmp4D1=_region_malloc(yyr,sizeof(*_tmp4D1));_tmp4D1->tag=4U,_tmp4D1->f1=ts,_tmp4D1->f2=(unsigned)((yyyvsp[0]).l).first_line,_tmp4D1->f3=0;_tmp4D1;});_tmp4D2->hd=_tmp9B7;}),({struct Cyc_List_List*_tmp9B6=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).tms;_tmp4D2->tl=_tmp9B6;});_tmp4D2;});_tmp752.tms=_tmp9B8;});_tmp752;}));
# 2022
goto _LL0;}case 189U: _LL175: _LL176:
# 2023 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp753;({struct _tuple0*_tmp9C0=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).id;_tmp753.id=_tmp9C0;}),({unsigned _tmp9BF=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).varloc;_tmp753.varloc=_tmp9BF;}),({struct Cyc_List_List*_tmp9BE=({struct Cyc_List_List*_tmp4D4=_region_malloc(yyr,sizeof(*_tmp4D4));({void*_tmp9BD=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp4D3=_region_malloc(yyr,sizeof(*_tmp4D3));_tmp4D3->tag=5U,_tmp4D3->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp9BC=Cyc_yyget_YY45(&(yyyvsp[1]).v);_tmp4D3->f2=_tmp9BC;});_tmp4D3;});_tmp4D4->hd=_tmp9BD;}),({
struct Cyc_List_List*_tmp9BB=(Cyc_yyget_YY27(&(yyyvsp[0]).v)).tms;_tmp4D4->tl=_tmp9BB;});_tmp4D4;});
# 2023
_tmp753.tms=_tmp9BE;});_tmp753;}));
# 2026
goto _LL0;case 190U: _LL177: _LL178:
# 2031 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp754;({struct _tuple0*_tmp9C1=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp754.id=_tmp9C1;}),_tmp754.varloc=(unsigned)((yyyvsp[0]).l).first_line,_tmp754.tms=0;_tmp754;}));
goto _LL0;case 191U: _LL179: _LL17A:
# 2033 "parse.y"
 yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp755;({struct _tuple0*_tmp9C2=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp755.id=_tmp9C2;}),_tmp755.varloc=(unsigned)((yyyvsp[0]).l).first_line,_tmp755.tms=0;_tmp755;}));
goto _LL0;case 192U: _LL17B: _LL17C:
# 2035 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 193U: _LL17D: _LL17E: {
# 2039 "parse.y"
struct Cyc_Parse_Declarator _tmp4D5=Cyc_yyget_YY27(&(yyyvsp[2]).v);struct Cyc_Parse_Declarator d=_tmp4D5;
({struct Cyc_List_List*_tmp9C5=({struct Cyc_List_List*_tmp4D7=_region_malloc(yyr,sizeof(*_tmp4D7));({void*_tmp9C4=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp4D6=_region_malloc(yyr,sizeof(*_tmp4D6));_tmp4D6->tag=5U,_tmp4D6->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp9C3=Cyc_yyget_YY45(&(yyyvsp[1]).v);_tmp4D6->f2=_tmp9C3;});_tmp4D6;});_tmp4D7->hd=_tmp9C4;}),_tmp4D7->tl=d.tms;_tmp4D7;});d.tms=_tmp9C5;});
yyval=(yyyvsp[2]).v;
# 2043
goto _LL0;}case 194U: _LL17F: _LL180: {
# 2044 "parse.y"
struct Cyc_Parse_Declarator _tmp4D8=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Parse_Declarator one=_tmp4D8;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp756;_tmp756.id=one.id,_tmp756.varloc=one.varloc,({
struct Cyc_List_List*_tmp9C8=({struct Cyc_List_List*_tmp4DA=_region_malloc(yyr,sizeof(*_tmp4DA));({void*_tmp9C7=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp4D9=_region_malloc(yyr,sizeof(*_tmp4D9));_tmp4D9->tag=0U,({void*_tmp9C6=Cyc_yyget_YY51(&(yyyvsp[3]).v);_tmp4D9->f1=_tmp9C6;}),_tmp4D9->f2=(unsigned)((yyyvsp[3]).l).first_line;_tmp4D9;});_tmp4DA->hd=_tmp9C7;}),_tmp4DA->tl=one.tms;_tmp4DA;});_tmp756.tms=_tmp9C8;});_tmp756;}));
goto _LL0;}case 195U: _LL181: _LL182: {
# 2048 "parse.y"
struct Cyc_Parse_Declarator _tmp4DB=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Parse_Declarator one=_tmp4DB;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp757;_tmp757.id=one.id,_tmp757.varloc=one.varloc,({
struct Cyc_List_List*_tmp9CC=({struct Cyc_List_List*_tmp4DD=_region_malloc(yyr,sizeof(*_tmp4DD));({void*_tmp9CB=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp4DC=_region_malloc(yyr,sizeof(*_tmp4DC));_tmp4DC->tag=1U,({struct Cyc_Absyn_Exp*_tmp9CA=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp4DC->f1=_tmp9CA;}),({void*_tmp9C9=Cyc_yyget_YY51(&(yyyvsp[4]).v);_tmp4DC->f2=_tmp9C9;}),_tmp4DC->f3=(unsigned)((yyyvsp[4]).l).first_line;_tmp4DC;});_tmp4DD->hd=_tmp9CB;}),_tmp4DD->tl=one.tms;_tmp4DD;});_tmp757.tms=_tmp9CC;});_tmp757;}));
# 2052
goto _LL0;}case 196U: _LL183: _LL184: {
# 2053 "parse.y"
struct _tuple27*_tmp4DE=Cyc_yyget_YY39(&(yyyvsp[2]).v);struct _tuple27*_stmttmp1C=_tmp4DE;struct _tuple27*_tmp4DF=_stmttmp1C;struct Cyc_List_List*_tmp4E4;void*_tmp4E3;struct Cyc_Absyn_VarargInfo*_tmp4E2;int _tmp4E1;struct Cyc_List_List*_tmp4E0;_LL46E: _tmp4E0=_tmp4DF->f1;_tmp4E1=_tmp4DF->f2;_tmp4E2=_tmp4DF->f3;_tmp4E3=_tmp4DF->f4;_tmp4E4=_tmp4DF->f5;_LL46F: {struct Cyc_List_List*lis=_tmp4E0;int b=_tmp4E1;struct Cyc_Absyn_VarargInfo*c=_tmp4E2;void*eff=_tmp4E3;struct Cyc_List_List*po=_tmp4E4;
struct Cyc_Absyn_Exp*_tmp4E5=Cyc_yyget_YY57(&(yyyvsp[4]).v);struct Cyc_Absyn_Exp*req=_tmp4E5;
struct Cyc_Absyn_Exp*_tmp4E6=Cyc_yyget_YY57(&(yyyvsp[5]).v);struct Cyc_Absyn_Exp*ens=_tmp4E6;
struct Cyc_Parse_Declarator _tmp4E7=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Parse_Declarator one=_tmp4E7;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp758;_tmp758.id=one.id,_tmp758.varloc=one.varloc,({struct Cyc_List_List*_tmp9CF=({struct Cyc_List_List*_tmp4EA=_region_malloc(yyr,sizeof(*_tmp4EA));({void*_tmp9CE=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp4E9=_region_malloc(yyr,sizeof(*_tmp4E9));_tmp4E9->tag=3U,({void*_tmp9CD=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp4E8=_region_malloc(yyr,sizeof(*_tmp4E8));_tmp4E8->tag=1U,_tmp4E8->f1=lis,_tmp4E8->f2=b,_tmp4E8->f3=c,_tmp4E8->f4=eff,_tmp4E8->f5=po,_tmp4E8->f6=req,_tmp4E8->f7=ens;_tmp4E8;});_tmp4E9->f1=_tmp9CD;});_tmp4E9;});_tmp4EA->hd=_tmp9CE;}),_tmp4EA->tl=one.tms;_tmp4EA;});_tmp758.tms=_tmp9CF;});_tmp758;}));
# 2059
goto _LL0;}}case 197U: _LL185: _LL186: {
# 2060 "parse.y"
struct Cyc_Parse_Declarator _tmp4EB=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Parse_Declarator one=_tmp4EB;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp759;_tmp759.id=one.id,_tmp759.varloc=one.varloc,({
struct Cyc_List_List*_tmp9D6=({struct Cyc_List_List*_tmp4EE=_region_malloc(yyr,sizeof(*_tmp4EE));({void*_tmp9D5=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp4ED=_region_malloc(yyr,sizeof(*_tmp4ED));_tmp4ED->tag=3U,({void*_tmp9D4=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp4EC=_region_malloc(yyr,sizeof(*_tmp4EC));_tmp4EC->tag=1U,_tmp4EC->f1=0,_tmp4EC->f2=0,_tmp4EC->f3=0,({
# 2064
void*_tmp9D3=Cyc_yyget_YY49(&(yyyvsp[2]).v);_tmp4EC->f4=_tmp9D3;}),({struct Cyc_List_List*_tmp9D2=Cyc_yyget_YY50(&(yyyvsp[3]).v);_tmp4EC->f5=_tmp9D2;}),({struct Cyc_Absyn_Exp*_tmp9D1=Cyc_yyget_YY57(&(yyyvsp[5]).v);_tmp4EC->f6=_tmp9D1;}),({struct Cyc_Absyn_Exp*_tmp9D0=Cyc_yyget_YY57(&(yyyvsp[6]).v);_tmp4EC->f7=_tmp9D0;});_tmp4EC;});
# 2062
_tmp4ED->f1=_tmp9D4;});_tmp4ED;});_tmp4EE->hd=_tmp9D5;}),_tmp4EE->tl=one.tms;_tmp4EE;});_tmp759.tms=_tmp9D6;});_tmp759;}));
# 2067
goto _LL0;}case 198U: _LL187: _LL188: {
# 2068 "parse.y"
struct Cyc_Parse_Declarator _tmp4EF=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Parse_Declarator one=_tmp4EF;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp75A;_tmp75A.id=one.id,_tmp75A.varloc=one.varloc,({struct Cyc_List_List*_tmp9DA=({struct Cyc_List_List*_tmp4F2=_region_malloc(yyr,sizeof(*_tmp4F2));({void*_tmp9D9=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp4F1=_region_malloc(yyr,sizeof(*_tmp4F1));_tmp4F1->tag=3U,({void*_tmp9D8=(void*)({struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*_tmp4F0=_region_malloc(yyr,sizeof(*_tmp4F0));_tmp4F0->tag=0U,({struct Cyc_List_List*_tmp9D7=Cyc_yyget_YY36(&(yyyvsp[2]).v);_tmp4F0->f1=_tmp9D7;}),_tmp4F0->f2=(unsigned)((yyyvsp[0]).l).first_line;_tmp4F0;});_tmp4F1->f1=_tmp9D8;});_tmp4F1;});_tmp4F2->hd=_tmp9D9;}),_tmp4F2->tl=one.tms;_tmp4F2;});_tmp75A.tms=_tmp9DA;});_tmp75A;}));
goto _LL0;}case 199U: _LL189: _LL18A: {
# 2072
struct Cyc_List_List*_tmp4F3=({unsigned _tmp9DB=(unsigned)((yyyvsp[1]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmp9DB,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY40(&(yyyvsp[2]).v)));});struct Cyc_List_List*ts=_tmp4F3;
struct Cyc_Parse_Declarator _tmp4F4=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Parse_Declarator one=_tmp4F4;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp75B;_tmp75B.id=one.id,_tmp75B.varloc=one.varloc,({struct Cyc_List_List*_tmp9DD=({struct Cyc_List_List*_tmp4F6=_region_malloc(yyr,sizeof(*_tmp4F6));({void*_tmp9DC=(void*)({struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_tmp4F5=_region_malloc(yyr,sizeof(*_tmp4F5));_tmp4F5->tag=4U,_tmp4F5->f1=ts,_tmp4F5->f2=(unsigned)((yyyvsp[0]).l).first_line,_tmp4F5->f3=0;_tmp4F5;});_tmp4F6->hd=_tmp9DC;}),_tmp4F6->tl=one.tms;_tmp4F6;});_tmp75B.tms=_tmp9DD;});_tmp75B;}));
# 2076
goto _LL0;}case 200U: _LL18B: _LL18C: {
# 2077 "parse.y"
struct Cyc_Parse_Declarator _tmp4F7=Cyc_yyget_YY27(&(yyyvsp[0]).v);struct Cyc_Parse_Declarator one=_tmp4F7;
yyval=Cyc_YY27(({struct Cyc_Parse_Declarator _tmp75C;_tmp75C.id=one.id,_tmp75C.varloc=one.varloc,({struct Cyc_List_List*_tmp9E0=({struct Cyc_List_List*_tmp4F9=_region_malloc(yyr,sizeof(*_tmp4F9));({void*_tmp9DF=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp4F8=_region_malloc(yyr,sizeof(*_tmp4F8));_tmp4F8->tag=5U,_tmp4F8->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp9DE=Cyc_yyget_YY45(&(yyyvsp[1]).v);_tmp4F8->f2=_tmp9DE;});_tmp4F8;});_tmp4F9->hd=_tmp9DF;}),_tmp4F9->tl=one.tms;_tmp4F9;});_tmp75C.tms=_tmp9E0;});_tmp75C;}));
# 2080
goto _LL0;}case 201U: _LL18D: _LL18E:
# 2084 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 202U: _LL18F: _LL190:
# 2085 "parse.y"
 yyval=Cyc_YY26(({struct Cyc_List_List*_tmp9E1=Cyc_yyget_YY26(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmp9E1,Cyc_yyget_YY26(&(yyyvsp[1]).v));}));
goto _LL0;case 203U: _LL191: _LL192: {
# 2089 "parse.y"
struct Cyc_List_List*ans=0;
if(Cyc_yyget_YY45(&(yyyvsp[3]).v)!= 0)
ans=({struct Cyc_List_List*_tmp4FB=_region_malloc(yyr,sizeof(*_tmp4FB));({void*_tmp9E3=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp4FA=_region_malloc(yyr,sizeof(*_tmp4FA));_tmp4FA->tag=5U,_tmp4FA->f1=(unsigned)((yyyvsp[3]).l).first_line,({struct Cyc_List_List*_tmp9E2=Cyc_yyget_YY45(&(yyyvsp[3]).v);_tmp4FA->f2=_tmp9E2;});_tmp4FA;});_tmp4FB->hd=_tmp9E3;}),_tmp4FB->tl=ans;_tmp4FB;});{
# 2093
struct Cyc_Absyn_PtrLoc*ptrloc=0;
struct _tuple22 _tmp4FC=*Cyc_yyget_YY1(&(yyyvsp[0]).v);struct _tuple22 _stmttmp1D=_tmp4FC;struct _tuple22 _tmp4FD=_stmttmp1D;void*_tmp500;void*_tmp4FF;unsigned _tmp4FE;_LL471: _tmp4FE=_tmp4FD.f1;_tmp4FF=_tmp4FD.f2;_tmp500=_tmp4FD.f3;_LL472: {unsigned ploc=_tmp4FE;void*nullable=_tmp4FF;void*bound=_tmp500;
if(Cyc_Absyn_porting_c_code)
ptrloc=({struct Cyc_Absyn_PtrLoc*_tmp501=_cycalloc(sizeof(*_tmp501));_tmp501->ptr_loc=ploc,_tmp501->rgn_loc=(unsigned)((yyyvsp[2]).l).first_line,_tmp501->zt_loc=(unsigned)((yyyvsp[1]).l).first_line;_tmp501;});{
# 2098
struct _tuple15 _tmp502=({unsigned _tmp9E7=ploc;void*_tmp9E6=nullable;void*_tmp9E5=bound;void*_tmp9E4=Cyc_yyget_YY44(&(yyyvsp[2]).v);Cyc_Parse_collapse_pointer_quals(_tmp9E7,_tmp9E6,_tmp9E5,_tmp9E4,Cyc_yyget_YY56(&(yyyvsp[1]).v));});struct _tuple15 _stmttmp1E=_tmp502;struct _tuple15 _tmp503=_stmttmp1E;void*_tmp507;void*_tmp506;void*_tmp505;void*_tmp504;_LL474: _tmp504=_tmp503.f1;_tmp505=_tmp503.f2;_tmp506=_tmp503.f3;_tmp507=_tmp503.f4;_LL475: {void*nullable=_tmp504;void*bound=_tmp505;void*zeroterm=_tmp506;void*rgn_opt=_tmp507;
ans=({struct Cyc_List_List*_tmp509=_region_malloc(yyr,sizeof(*_tmp509));({void*_tmp9E9=(void*)({struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*_tmp508=_region_malloc(yyr,sizeof(*_tmp508));_tmp508->tag=2U,(_tmp508->f1).rgn=rgn_opt,(_tmp508->f1).nullable=nullable,(_tmp508->f1).bounds=bound,(_tmp508->f1).zero_term=zeroterm,(_tmp508->f1).ptrloc=ptrloc,({struct Cyc_Absyn_Tqual _tmp9E8=Cyc_yyget_YY23(&(yyyvsp[4]).v);_tmp508->f2=_tmp9E8;});_tmp508;});_tmp509->hd=_tmp9E9;}),_tmp509->tl=ans;_tmp509;});
yyval=Cyc_YY26(ans);
# 2102
goto _LL0;}}}}}case 204U: _LL193: _LL194:
# 2104
 yyval=Cyc_YY56(0);
goto _LL0;case 205U: _LL195: _LL196:
# 2105 "parse.y"
 yyval=Cyc_YY56(({struct Cyc_List_List*_tmp50A=_region_malloc(yyr,sizeof(*_tmp50A));({void*_tmp9EB=Cyc_yyget_YY55(&(yyyvsp[0]).v);_tmp50A->hd=_tmp9EB;}),({struct Cyc_List_List*_tmp9EA=Cyc_yyget_YY56(&(yyyvsp[1]).v);_tmp50A->tl=_tmp9EA;});_tmp50A;}));
goto _LL0;case 206U: _LL197: _LL198:
# 2110 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Numelts_ptrqual_Parse_Pointer_qual_struct*_tmp50B=_region_malloc(yyr,sizeof(*_tmp50B));_tmp50B->tag=0U,({struct Cyc_Absyn_Exp*_tmp9EC=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp50B->f1=_tmp9EC;});_tmp50B;}));
goto _LL0;case 207U: _LL199: _LL19A:
# 2112 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Region_ptrqual_Parse_Pointer_qual_struct*_tmp50C=_region_malloc(yyr,sizeof(*_tmp50C));_tmp50C->tag=1U,({void*_tmp9ED=Cyc_yyget_YY44(&(yyyvsp[2]).v);_tmp50C->f1=_tmp9ED;});_tmp50C;}));
goto _LL0;case 208U: _LL19B: _LL19C:
# 2113 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Thin_ptrqual_Parse_Pointer_qual_struct*_tmp50D=_region_malloc(yyr,sizeof(*_tmp50D));_tmp50D->tag=2U;_tmp50D;}));
goto _LL0;case 209U: _LL19D: _LL19E:
# 2114 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Fat_ptrqual_Parse_Pointer_qual_struct*_tmp50E=_region_malloc(yyr,sizeof(*_tmp50E));_tmp50E->tag=3U;_tmp50E;}));
goto _LL0;case 210U: _LL19F: _LL1A0:
# 2115 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Zeroterm_ptrqual_Parse_Pointer_qual_struct*_tmp50F=_region_malloc(yyr,sizeof(*_tmp50F));_tmp50F->tag=4U;_tmp50F;}));
goto _LL0;case 211U: _LL1A1: _LL1A2:
# 2116 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Nozeroterm_ptrqual_Parse_Pointer_qual_struct*_tmp510=_region_malloc(yyr,sizeof(*_tmp510));_tmp510->tag=5U;_tmp510;}));
goto _LL0;case 212U: _LL1A3: _LL1A4:
# 2117 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Notnull_ptrqual_Parse_Pointer_qual_struct*_tmp511=_region_malloc(yyr,sizeof(*_tmp511));_tmp511->tag=6U;_tmp511;}));
goto _LL0;case 213U: _LL1A5: _LL1A6:
# 2118 "parse.y"
 yyval=Cyc_YY55((void*)({struct Cyc_Parse_Nullable_ptrqual_Parse_Pointer_qual_struct*_tmp512=_region_malloc(yyr,sizeof(*_tmp512));_tmp512->tag=7U;_tmp512;}));
goto _LL0;case 214U: _LL1A7: _LL1A8:
# 2124 "parse.y"
 yyval=Cyc_YY1(({struct _tuple22*_tmp513=_cycalloc(sizeof(*_tmp513));_tmp513->f1=(unsigned)((yyyvsp[0]).l).first_line,_tmp513->f2=Cyc_Absyn_true_type,Cyc_Parse_parsing_tempest?_tmp513->f3=Cyc_Absyn_fat_bound_type:({void*_tmp9EE=Cyc_yyget_YY2(&(yyyvsp[1]).v);_tmp513->f3=_tmp9EE;});_tmp513;}));
# 2126
goto _LL0;case 215U: _LL1A9: _LL1AA:
# 2126 "parse.y"
 yyval=Cyc_YY1(({struct _tuple22*_tmp514=_cycalloc(sizeof(*_tmp514));_tmp514->f1=(unsigned)((yyyvsp[0]).l).first_line,_tmp514->f2=Cyc_Absyn_false_type,({void*_tmp9EF=Cyc_yyget_YY2(&(yyyvsp[1]).v);_tmp514->f3=_tmp9EF;});_tmp514;}));
goto _LL0;case 216U: _LL1AB: _LL1AC:
# 2127 "parse.y"
 yyval=Cyc_YY1(({struct _tuple22*_tmp515=_cycalloc(sizeof(*_tmp515));_tmp515->f1=(unsigned)((yyyvsp[0]).l).first_line,_tmp515->f2=Cyc_Absyn_true_type,_tmp515->f3=Cyc_Absyn_fat_bound_type;_tmp515;}));
goto _LL0;case 217U: _LL1AD: _LL1AE:
# 2130
 yyval=Cyc_YY2(Cyc_Absyn_bounds_one());
goto _LL0;case 218U: _LL1AF: _LL1B0:
# 2131 "parse.y"
 yyval=Cyc_YY2(Cyc_Absyn_thin_bounds_exp(Cyc_yyget_Exp_tok(&(yyyvsp[1]).v)));
goto _LL0;case 219U: _LL1B1: _LL1B2:
# 2134
 yyval=Cyc_YY51(Cyc_Tcutil_any_bool(0));
goto _LL0;case 220U: _LL1B3: _LL1B4:
# 2135 "parse.y"
 yyval=Cyc_YY51(Cyc_Absyn_true_type);
goto _LL0;case 221U: _LL1B5: _LL1B6:
# 2136 "parse.y"
 yyval=Cyc_YY51(Cyc_Absyn_false_type);
goto _LL0;case 222U: _LL1B7: _LL1B8:
# 2141 "parse.y"
 yyval=Cyc_YY44(Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,0));
goto _LL0;case 223U: _LL1B9: _LL1BA:
# 2142 "parse.y"
 Cyc_Parse_set_vartyp_kind(Cyc_yyget_YY44(&(yyyvsp[0]).v),& Cyc_Tcutil_trk,1);yyval=(yyyvsp[0]).v;
goto _LL0;case 224U: _LL1BB: _LL1BC:
# 2143 "parse.y"
 yyval=Cyc_YY44(Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,0));
goto _LL0;case 225U: _LL1BD: _LL1BE:
# 2147 "parse.y"
 yyval=Cyc_YY23(Cyc_Absyn_empty_tqual((unsigned)((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),yyvsp_offset + 1))).l).first_line));
goto _LL0;case 226U: _LL1BF: _LL1C0:
# 2148 "parse.y"
 yyval=Cyc_YY23(({struct Cyc_Absyn_Tqual _tmp9F0=Cyc_yyget_YY23(&(yyyvsp[0]).v);Cyc_Absyn_combine_tqual(_tmp9F0,Cyc_yyget_YY23(&(yyyvsp[1]).v));}));
goto _LL0;case 227U: _LL1C1: _LL1C2:
# 2153 "parse.y"
 yyval=Cyc_YY39(({struct _tuple27*_tmp516=_cycalloc(sizeof(*_tmp516));({struct Cyc_List_List*_tmp9F3=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY38(&(yyyvsp[0]).v));_tmp516->f1=_tmp9F3;}),_tmp516->f2=0,_tmp516->f3=0,({void*_tmp9F2=Cyc_yyget_YY49(&(yyyvsp[1]).v);_tmp516->f4=_tmp9F2;}),({struct Cyc_List_List*_tmp9F1=Cyc_yyget_YY50(&(yyyvsp[2]).v);_tmp516->f5=_tmp9F1;});_tmp516;}));
goto _LL0;case 228U: _LL1C3: _LL1C4:
# 2155 "parse.y"
 yyval=Cyc_YY39(({struct _tuple27*_tmp517=_cycalloc(sizeof(*_tmp517));({struct Cyc_List_List*_tmp9F6=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY38(&(yyyvsp[0]).v));_tmp517->f1=_tmp9F6;}),_tmp517->f2=1,_tmp517->f3=0,({void*_tmp9F5=Cyc_yyget_YY49(&(yyyvsp[3]).v);_tmp517->f4=_tmp9F5;}),({struct Cyc_List_List*_tmp9F4=Cyc_yyget_YY50(&(yyyvsp[4]).v);_tmp517->f5=_tmp9F4;});_tmp517;}));
goto _LL0;case 229U: _LL1C5: _LL1C6: {
# 2158
struct _tuple8*_tmp518=Cyc_yyget_YY37(&(yyyvsp[2]).v);struct _tuple8*_stmttmp1F=_tmp518;struct _tuple8*_tmp519=_stmttmp1F;void*_tmp51C;struct Cyc_Absyn_Tqual _tmp51B;struct _fat_ptr*_tmp51A;_LL477: _tmp51A=_tmp519->f1;_tmp51B=_tmp519->f2;_tmp51C=_tmp519->f3;_LL478: {struct _fat_ptr*n=_tmp51A;struct Cyc_Absyn_Tqual tq=_tmp51B;void*t=_tmp51C;
struct Cyc_Absyn_VarargInfo*_tmp51D=({struct Cyc_Absyn_VarargInfo*_tmp51F=_cycalloc(sizeof(*_tmp51F));_tmp51F->name=n,_tmp51F->tq=tq,_tmp51F->type=t,({int _tmp9F7=Cyc_yyget_YY31(&(yyyvsp[1]).v);_tmp51F->inject=_tmp9F7;});_tmp51F;});struct Cyc_Absyn_VarargInfo*v=_tmp51D;
yyval=Cyc_YY39(({struct _tuple27*_tmp51E=_cycalloc(sizeof(*_tmp51E));_tmp51E->f1=0,_tmp51E->f2=0,_tmp51E->f3=v,({void*_tmp9F9=Cyc_yyget_YY49(&(yyyvsp[3]).v);_tmp51E->f4=_tmp9F9;}),({struct Cyc_List_List*_tmp9F8=Cyc_yyget_YY50(&(yyyvsp[4]).v);_tmp51E->f5=_tmp9F8;});_tmp51E;}));
# 2162
goto _LL0;}}case 230U: _LL1C7: _LL1C8: {
# 2164
struct _tuple8*_tmp520=Cyc_yyget_YY37(&(yyyvsp[4]).v);struct _tuple8*_stmttmp20=_tmp520;struct _tuple8*_tmp521=_stmttmp20;void*_tmp524;struct Cyc_Absyn_Tqual _tmp523;struct _fat_ptr*_tmp522;_LL47A: _tmp522=_tmp521->f1;_tmp523=_tmp521->f2;_tmp524=_tmp521->f3;_LL47B: {struct _fat_ptr*n=_tmp522;struct Cyc_Absyn_Tqual tq=_tmp523;void*t=_tmp524;
struct Cyc_Absyn_VarargInfo*_tmp525=({struct Cyc_Absyn_VarargInfo*_tmp527=_cycalloc(sizeof(*_tmp527));_tmp527->name=n,_tmp527->tq=tq,_tmp527->type=t,({int _tmp9FA=Cyc_yyget_YY31(&(yyyvsp[3]).v);_tmp527->inject=_tmp9FA;});_tmp527;});struct Cyc_Absyn_VarargInfo*v=_tmp525;
yyval=Cyc_YY39(({struct _tuple27*_tmp526=_cycalloc(sizeof(*_tmp526));({struct Cyc_List_List*_tmp9FD=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY38(&(yyyvsp[0]).v));_tmp526->f1=_tmp9FD;}),_tmp526->f2=0,_tmp526->f3=v,({void*_tmp9FC=Cyc_yyget_YY49(&(yyyvsp[5]).v);_tmp526->f4=_tmp9FC;}),({struct Cyc_List_List*_tmp9FB=Cyc_yyget_YY50(&(yyyvsp[6]).v);_tmp526->f5=_tmp9FB;});_tmp526;}));
# 2168
goto _LL0;}}case 231U: _LL1C9: _LL1CA:
# 2172 "parse.y"
 yyval=Cyc_YY44(({struct _fat_ptr _tmp9FE=Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_Parse_id2type(_tmp9FE,(void*)({struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_tmp528=_cycalloc(sizeof(*_tmp528));_tmp528->tag=1U,_tmp528->f1=0;_tmp528;}));}));
goto _LL0;case 232U: _LL1CB: _LL1CC:
# 2173 "parse.y"
 yyval=Cyc_YY44(({struct _fat_ptr _tmp9FF=Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_Parse_id2type(_tmp9FF,Cyc_Tcutil_kind_to_bound(Cyc_yyget_YY43(&(yyyvsp[2]).v)));}));
goto _LL0;case 233U: _LL1CD: _LL1CE:
# 2176
 yyval=Cyc_YY49(0);
goto _LL0;case 234U: _LL1CF: _LL1D0:
# 2177 "parse.y"
 yyval=Cyc_YY49(Cyc_Absyn_join_eff(Cyc_yyget_YY40(&(yyyvsp[1]).v)));
goto _LL0;case 235U: _LL1D1: _LL1D2:
# 2181 "parse.y"
 yyval=Cyc_YY50(0);
goto _LL0;case 236U: _LL1D3: _LL1D4:
# 2182 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 237U: _LL1D5: _LL1D6: {
# 2190 "parse.y"
struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp529=({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp52D=_cycalloc(sizeof(*_tmp52D));_tmp52D->tag=2U,_tmp52D->f1=0,_tmp52D->f2=& Cyc_Tcutil_trk;_tmp52D;});struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*kb=_tmp529;
void*_tmp52A=({struct _fat_ptr _tmpA00=Cyc_yyget_String_tok(&(yyyvsp[2]).v);Cyc_Parse_id2type(_tmpA00,(void*)kb);});void*t=_tmp52A;
yyval=Cyc_YY50(({struct Cyc_List_List*_tmp52C=_cycalloc(sizeof(*_tmp52C));({struct _tuple34*_tmpA02=({struct _tuple34*_tmp52B=_cycalloc(sizeof(*_tmp52B));({void*_tmpA01=Cyc_Absyn_join_eff(Cyc_yyget_YY40(&(yyyvsp[0]).v));_tmp52B->f1=_tmpA01;}),_tmp52B->f2=t;_tmp52B;});_tmp52C->hd=_tmpA02;}),_tmp52C->tl=0;_tmp52C;}));
# 2194
goto _LL0;}case 238U: _LL1D7: _LL1D8: {
# 2196 "parse.y"
struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp52E=({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp532=_cycalloc(sizeof(*_tmp532));_tmp532->tag=2U,_tmp532->f1=0,_tmp532->f2=& Cyc_Tcutil_trk;_tmp532;});struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*kb=_tmp52E;
void*_tmp52F=({struct _fat_ptr _tmpA03=Cyc_yyget_String_tok(&(yyyvsp[2]).v);Cyc_Parse_id2type(_tmpA03,(void*)kb);});void*t=_tmp52F;
yyval=Cyc_YY50(({struct Cyc_List_List*_tmp531=_cycalloc(sizeof(*_tmp531));({struct _tuple34*_tmpA06=({struct _tuple34*_tmp530=_cycalloc(sizeof(*_tmp530));({void*_tmpA05=Cyc_Absyn_join_eff(Cyc_yyget_YY40(&(yyyvsp[0]).v));_tmp530->f1=_tmpA05;}),_tmp530->f2=t;_tmp530;});_tmp531->hd=_tmpA06;}),({struct Cyc_List_List*_tmpA04=Cyc_yyget_YY50(&(yyyvsp[4]).v);_tmp531->tl=_tmpA04;});_tmp531;}));
# 2200
goto _LL0;}case 239U: _LL1D9: _LL1DA:
# 2204 "parse.y"
 yyval=Cyc_YY31(0);
goto _LL0;case 240U: _LL1DB: _LL1DC:
# 2206 "parse.y"
 if(({struct _fat_ptr _tmpA07=(struct _fat_ptr)Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_zstrcmp(_tmpA07,({const char*_tmp533="inject";_tag_fat(_tmp533,sizeof(char),7U);}));})!= 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp535=({struct Cyc_Warn_String_Warn_Warg_struct _tmp75D;_tmp75D.tag=0U,({struct _fat_ptr _tmpA08=({const char*_tmp536="missing type in function declaration";_tag_fat(_tmp536,sizeof(char),37U);});_tmp75D.f1=_tmpA08;});_tmp75D;});void*_tmp534[1U];_tmp534[0]=& _tmp535;({unsigned _tmpA09=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Warn_err2(_tmpA09,_tag_fat(_tmp534,sizeof(void*),1U));});});
yyval=Cyc_YY31(1);
# 2210
goto _LL0;case 241U: _LL1DD: _LL1DE:
# 2213 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 242U: _LL1DF: _LL1E0:
# 2214 "parse.y"
 yyval=Cyc_YY40(({struct Cyc_List_List*_tmpA0A=Cyc_yyget_YY40(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmpA0A,Cyc_yyget_YY40(&(yyyvsp[2]).v));}));
goto _LL0;case 243U: _LL1E1: _LL1E2:
# 2218 "parse.y"
 yyval=Cyc_YY40(0);
goto _LL0;case 244U: _LL1E3: _LL1E4:
# 2219 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 245U: _LL1E5: _LL1E6:
# 2221 "parse.y"
 yyval=Cyc_YY40(({struct Cyc_List_List*_tmp537=_cycalloc(sizeof(*_tmp537));({void*_tmpA0B=Cyc_Absyn_regionsof_eff(Cyc_yyget_YY44(&(yyyvsp[2]).v));_tmp537->hd=_tmpA0B;}),_tmp537->tl=0;_tmp537;}));
goto _LL0;case 246U: _LL1E7: _LL1E8:
# 2223 "parse.y"
 Cyc_Parse_set_vartyp_kind(Cyc_yyget_YY44(&(yyyvsp[0]).v),& Cyc_Tcutil_ek,0);
yyval=Cyc_YY40(({struct Cyc_List_List*_tmp538=_cycalloc(sizeof(*_tmp538));({void*_tmpA0C=Cyc_yyget_YY44(&(yyyvsp[0]).v);_tmp538->hd=_tmpA0C;}),_tmp538->tl=0;_tmp538;}));
# 2226
goto _LL0;case 247U: _LL1E9: _LL1EA:
# 2243 "parse.y"
 yyval=Cyc_YY40(({struct Cyc_List_List*_tmp539=_cycalloc(sizeof(*_tmp539));({void*_tmpA0E=Cyc_Absyn_access_eff(({struct _tuple8*_tmpA0D=Cyc_yyget_YY37(&(yyyvsp[0]).v);Cyc_Parse_type_name_to_type(_tmpA0D,(unsigned)((yyyvsp[0]).l).first_line);}));_tmp539->hd=_tmpA0E;}),_tmp539->tl=0;_tmp539;}));
goto _LL0;case 248U: _LL1EB: _LL1EC:
# 2245 "parse.y"
 yyval=Cyc_YY40(({struct Cyc_List_List*_tmp53A=_cycalloc(sizeof(*_tmp53A));({void*_tmpA11=Cyc_Absyn_access_eff(({struct _tuple8*_tmpA10=Cyc_yyget_YY37(&(yyyvsp[0]).v);Cyc_Parse_type_name_to_type(_tmpA10,(unsigned)((yyyvsp[0]).l).first_line);}));_tmp53A->hd=_tmpA11;}),({struct Cyc_List_List*_tmpA0F=Cyc_yyget_YY40(&(yyyvsp[2]).v);_tmp53A->tl=_tmpA0F;});_tmp53A;}));
goto _LL0;case 249U: _LL1ED: _LL1EE:
# 2251 "parse.y"
 yyval=Cyc_YY38(({struct Cyc_List_List*_tmp53B=_cycalloc(sizeof(*_tmp53B));({struct _tuple8*_tmpA12=Cyc_yyget_YY37(&(yyyvsp[0]).v);_tmp53B->hd=_tmpA12;}),_tmp53B->tl=0;_tmp53B;}));
goto _LL0;case 250U: _LL1EF: _LL1F0:
# 2253 "parse.y"
 yyval=Cyc_YY38(({struct Cyc_List_List*_tmp53C=_cycalloc(sizeof(*_tmp53C));({struct _tuple8*_tmpA14=Cyc_yyget_YY37(&(yyyvsp[2]).v);_tmp53C->hd=_tmpA14;}),({struct Cyc_List_List*_tmpA13=Cyc_yyget_YY38(&(yyyvsp[0]).v);_tmp53C->tl=_tmpA13;});_tmp53C;}));
goto _LL0;case 251U: _LL1F1: _LL1F2: {
# 2259 "parse.y"
struct _tuple26 _tmp53D=Cyc_yyget_YY35(&(yyyvsp[0]).v);struct _tuple26 _stmttmp21=_tmp53D;struct _tuple26 _tmp53E=_stmttmp21;struct Cyc_List_List*_tmp541;struct Cyc_Parse_Type_specifier _tmp540;struct Cyc_Absyn_Tqual _tmp53F;_LL47D: _tmp53F=_tmp53E.f1;_tmp540=_tmp53E.f2;_tmp541=_tmp53E.f3;_LL47E: {struct Cyc_Absyn_Tqual tq=_tmp53F;struct Cyc_Parse_Type_specifier tspecs=_tmp540;struct Cyc_List_List*atts=_tmp541;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{
struct Cyc_Parse_Declarator _tmp542=Cyc_yyget_YY27(&(yyyvsp[1]).v);struct Cyc_Parse_Declarator _stmttmp22=_tmp542;struct Cyc_Parse_Declarator _tmp543=_stmttmp22;struct Cyc_List_List*_tmp546;unsigned _tmp545;struct _tuple0*_tmp544;_LL480: _tmp544=_tmp543.id;_tmp545=_tmp543.varloc;_tmp546=_tmp543.tms;_LL481: {struct _tuple0*qv=_tmp544;unsigned varloc=_tmp545;struct Cyc_List_List*tms=_tmp546;
void*_tmp547=Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);void*t=_tmp547;
struct _tuple14 _tmp548=Cyc_Parse_apply_tms(tq,t,atts,tms);struct _tuple14 _stmttmp23=_tmp548;struct _tuple14 _tmp549=_stmttmp23;struct Cyc_List_List*_tmp54D;struct Cyc_List_List*_tmp54C;void*_tmp54B;struct Cyc_Absyn_Tqual _tmp54A;_LL483: _tmp54A=_tmp549.f1;_tmp54B=_tmp549.f2;_tmp54C=_tmp549.f3;_tmp54D=_tmp549.f4;_LL484: {struct Cyc_Absyn_Tqual tq2=_tmp54A;void*t2=_tmp54B;struct Cyc_List_List*tvs=_tmp54C;struct Cyc_List_List*atts2=_tmp54D;
if(tvs != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp54F=({struct Cyc_Warn_String_Warn_Warg_struct _tmp75E;_tmp75E.tag=0U,({struct _fat_ptr _tmpA15=({const char*_tmp550="parameter with bad type params";_tag_fat(_tmp550,sizeof(char),31U);});_tmp75E.f1=_tmpA15;});_tmp75E;});void*_tmp54E[1U];_tmp54E[0]=& _tmp54F;({unsigned _tmpA16=(unsigned)((yyyvsp[1]).l).first_line;Cyc_Warn_err2(_tmpA16,_tag_fat(_tmp54E,sizeof(void*),1U));});});
if(Cyc_Absyn_is_qvar_qualified(qv))
({struct Cyc_Warn_String_Warn_Warg_struct _tmp552=({struct Cyc_Warn_String_Warn_Warg_struct _tmp75F;_tmp75F.tag=0U,({struct _fat_ptr _tmpA17=({const char*_tmp553="parameter cannot be qualified with a namespace";_tag_fat(_tmp553,sizeof(char),47U);});_tmp75F.f1=_tmpA17;});_tmp75F;});void*_tmp551[1U];_tmp551[0]=& _tmp552;({unsigned _tmpA18=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Warn_err2(_tmpA18,_tag_fat(_tmp551,sizeof(void*),1U));});});{
struct _fat_ptr*idopt=(*qv).f2;
if(atts2 != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp555=({struct Cyc_Warn_String_Warn_Warg_struct _tmp760;_tmp760.tag=0U,({struct _fat_ptr _tmpA19=({const char*_tmp556="extra attributes on parameter, ignoring";_tag_fat(_tmp556,sizeof(char),40U);});_tmp760.f1=_tmpA19;});_tmp760;});void*_tmp554[1U];_tmp554[0]=& _tmp555;({unsigned _tmpA1A=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Warn_warn2(_tmpA1A,_tag_fat(_tmp554,sizeof(void*),1U));});});
yyval=Cyc_YY37(({struct _tuple8*_tmp557=_cycalloc(sizeof(*_tmp557));_tmp557->f1=idopt,_tmp557->f2=tq2,_tmp557->f3=t2;_tmp557;}));
# 2273
goto _LL0;}}}}}}case 252U: _LL1F3: _LL1F4: {
# 2274 "parse.y"
struct _tuple26 _tmp558=Cyc_yyget_YY35(&(yyyvsp[0]).v);struct _tuple26 _stmttmp24=_tmp558;struct _tuple26 _tmp559=_stmttmp24;struct Cyc_List_List*_tmp55C;struct Cyc_Parse_Type_specifier _tmp55B;struct Cyc_Absyn_Tqual _tmp55A;_LL486: _tmp55A=_tmp559.f1;_tmp55B=_tmp559.f2;_tmp55C=_tmp559.f3;_LL487: {struct Cyc_Absyn_Tqual tq=_tmp55A;struct Cyc_Parse_Type_specifier tspecs=_tmp55B;struct Cyc_List_List*atts=_tmp55C;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{
void*_tmp55D=Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);void*t=_tmp55D;
if(atts != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp55F=({struct Cyc_Warn_String_Warn_Warg_struct _tmp761;_tmp761.tag=0U,({struct _fat_ptr _tmpA1B=({const char*_tmp560="bad attributes on parameter, ignoring";_tag_fat(_tmp560,sizeof(char),38U);});_tmp761.f1=_tmpA1B;});_tmp761;});void*_tmp55E[1U];_tmp55E[0]=& _tmp55F;({unsigned _tmpA1C=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Warn_warn2(_tmpA1C,_tag_fat(_tmp55E,sizeof(void*),1U));});});
yyval=Cyc_YY37(({struct _tuple8*_tmp561=_cycalloc(sizeof(*_tmp561));_tmp561->f1=0,_tmp561->f2=tq,_tmp561->f3=t;_tmp561;}));
# 2281
goto _LL0;}}}case 253U: _LL1F5: _LL1F6: {
# 2282 "parse.y"
struct _tuple26 _tmp562=Cyc_yyget_YY35(&(yyyvsp[0]).v);struct _tuple26 _stmttmp25=_tmp562;struct _tuple26 _tmp563=_stmttmp25;struct Cyc_List_List*_tmp566;struct Cyc_Parse_Type_specifier _tmp565;struct Cyc_Absyn_Tqual _tmp564;_LL489: _tmp564=_tmp563.f1;_tmp565=_tmp563.f2;_tmp566=_tmp563.f3;_LL48A: {struct Cyc_Absyn_Tqual tq=_tmp564;struct Cyc_Parse_Type_specifier tspecs=_tmp565;struct Cyc_List_List*atts=_tmp566;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{
void*_tmp567=Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);void*t=_tmp567;
struct Cyc_List_List*_tmp568=(Cyc_yyget_YY30(&(yyyvsp[1]).v)).tms;struct Cyc_List_List*tms=_tmp568;
struct _tuple14 _tmp569=Cyc_Parse_apply_tms(tq,t,atts,tms);struct _tuple14 _stmttmp26=_tmp569;struct _tuple14 _tmp56A=_stmttmp26;struct Cyc_List_List*_tmp56E;struct Cyc_List_List*_tmp56D;void*_tmp56C;struct Cyc_Absyn_Tqual _tmp56B;_LL48C: _tmp56B=_tmp56A.f1;_tmp56C=_tmp56A.f2;_tmp56D=_tmp56A.f3;_tmp56E=_tmp56A.f4;_LL48D: {struct Cyc_Absyn_Tqual tq2=_tmp56B;void*t2=_tmp56C;struct Cyc_List_List*tvs=_tmp56D;struct Cyc_List_List*atts2=_tmp56E;
if(tvs != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp570=({struct Cyc_Warn_String_Warn_Warg_struct _tmp762;_tmp762.tag=0U,({
struct _fat_ptr _tmpA1D=({const char*_tmp571="bad type parameters on formal argument, ignoring";_tag_fat(_tmp571,sizeof(char),49U);});_tmp762.f1=_tmpA1D;});_tmp762;});void*_tmp56F[1U];_tmp56F[0]=& _tmp570;({unsigned _tmpA1E=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Warn_warn2(_tmpA1E,_tag_fat(_tmp56F,sizeof(void*),1U));});});
if(atts2 != 0)
({struct Cyc_Warn_String_Warn_Warg_struct _tmp573=({struct Cyc_Warn_String_Warn_Warg_struct _tmp763;_tmp763.tag=0U,({struct _fat_ptr _tmpA1F=({const char*_tmp574="bad attributes on parameter, ignoring";_tag_fat(_tmp574,sizeof(char),38U);});_tmp763.f1=_tmpA1F;});_tmp763;});void*_tmp572[1U];_tmp572[0]=& _tmp573;({unsigned _tmpA20=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Warn_warn2(_tmpA20,_tag_fat(_tmp572,sizeof(void*),1U));});});
yyval=Cyc_YY37(({struct _tuple8*_tmp575=_cycalloc(sizeof(*_tmp575));_tmp575->f1=0,_tmp575->f2=tq2,_tmp575->f3=t2;_tmp575;}));
# 2294
goto _LL0;}}}}case 254U: _LL1F7: _LL1F8:
# 2298 "parse.y"
 yyval=Cyc_YY36(((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY36(&(yyyvsp[0]).v)));
goto _LL0;case 255U: _LL1F9: _LL1FA:
# 2302 "parse.y"
 yyval=Cyc_YY36(({struct Cyc_List_List*_tmp577=_cycalloc(sizeof(*_tmp577));({struct _fat_ptr*_tmpA22=({struct _fat_ptr*_tmp576=_cycalloc(sizeof(*_tmp576));({struct _fat_ptr _tmpA21=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp576=_tmpA21;});_tmp576;});_tmp577->hd=_tmpA22;}),_tmp577->tl=0;_tmp577;}));
goto _LL0;case 256U: _LL1FB: _LL1FC:
# 2304 "parse.y"
 yyval=Cyc_YY36(({struct Cyc_List_List*_tmp579=_cycalloc(sizeof(*_tmp579));({struct _fat_ptr*_tmpA25=({struct _fat_ptr*_tmp578=_cycalloc(sizeof(*_tmp578));({struct _fat_ptr _tmpA24=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp578=_tmpA24;});_tmp578;});_tmp579->hd=_tmpA25;}),({struct Cyc_List_List*_tmpA23=Cyc_yyget_YY36(&(yyyvsp[0]).v);_tmp579->tl=_tmpA23;});_tmp579;}));
goto _LL0;case 257U: _LL1FD: _LL1FE:
# 2308 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 258U: _LL1FF: _LL200:
# 2309 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 259U: _LL201: _LL202:
# 2314 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpA26=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp57A=_cycalloc(sizeof(*_tmp57A));_tmp57A->tag=36U,_tmp57A->f1=0,_tmp57A->f2=0;_tmp57A;});Cyc_Absyn_new_exp(_tmpA26,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 260U: _LL203: _LL204:
# 2316 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpA28=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp57B=_cycalloc(sizeof(*_tmp57B));_tmp57B->tag=36U,_tmp57B->f1=0,({struct Cyc_List_List*_tmpA27=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY5(&(yyyvsp[1]).v));_tmp57B->f2=_tmpA27;});_tmp57B;});Cyc_Absyn_new_exp(_tmpA28,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 261U: _LL205: _LL206:
# 2318 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpA2A=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp57C=_cycalloc(sizeof(*_tmp57C));_tmp57C->tag=36U,_tmp57C->f1=0,({struct Cyc_List_List*_tmpA29=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY5(&(yyyvsp[1]).v));_tmp57C->f2=_tmpA29;});_tmp57C;});Cyc_Absyn_new_exp(_tmpA2A,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 262U: _LL207: _LL208: {
# 2320 "parse.y"
struct Cyc_Absyn_Vardecl*_tmp57D=({unsigned _tmpA2F=(unsigned)((yyyvsp[2]).l).first_line;struct _tuple0*_tmpA2E=({struct _tuple0*_tmp580=_cycalloc(sizeof(*_tmp580));_tmp580->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpA2C=({struct _fat_ptr*_tmp57F=_cycalloc(sizeof(*_tmp57F));({struct _fat_ptr _tmpA2B=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp57F=_tmpA2B;});_tmp57F;});_tmp580->f2=_tmpA2C;});_tmp580;});void*_tmpA2D=Cyc_Absyn_uint_type;Cyc_Absyn_new_vardecl(_tmpA2F,_tmpA2E,_tmpA2D,
Cyc_Absyn_uint_exp(0U,(unsigned)((yyyvsp[2]).l).first_line));});
# 2320
struct Cyc_Absyn_Vardecl*vd=_tmp57D;
# 2323
(vd->tq).real_const=1;
yyval=Cyc_Exp_tok(({void*_tmpA32=(void*)({struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_tmp57E=_cycalloc(sizeof(*_tmp57E));_tmp57E->tag=27U,_tmp57E->f1=vd,({struct Cyc_Absyn_Exp*_tmpA31=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);_tmp57E->f2=_tmpA31;}),({struct Cyc_Absyn_Exp*_tmpA30=Cyc_yyget_Exp_tok(&(yyyvsp[6]).v);_tmp57E->f3=_tmpA30;}),_tmp57E->f4=0;_tmp57E;});Cyc_Absyn_new_exp(_tmpA32,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2326
goto _LL0;}case 263U: _LL209: _LL20A: {
# 2328 "parse.y"
void*_tmp581=({struct _tuple8*_tmpA33=Cyc_yyget_YY37(&(yyyvsp[6]).v);Cyc_Parse_type_name_to_type(_tmpA33,(unsigned)((yyyvsp[6]).l).first_line);});void*t=_tmp581;
yyval=Cyc_Exp_tok(({void*_tmpA35=(void*)({struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_tmp582=_cycalloc(sizeof(*_tmp582));_tmp582->tag=28U,({struct Cyc_Absyn_Exp*_tmpA34=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);_tmp582->f1=_tmpA34;}),_tmp582->f2=t,_tmp582->f3=0;_tmp582;});Cyc_Absyn_new_exp(_tmpA35,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2331
goto _LL0;}case 264U: _LL20B: _LL20C:
# 2336 "parse.y"
 yyval=Cyc_YY5(({struct Cyc_List_List*_tmp584=_cycalloc(sizeof(*_tmp584));({struct _tuple35*_tmpA37=({struct _tuple35*_tmp583=_cycalloc(sizeof(*_tmp583));_tmp583->f1=0,({struct Cyc_Absyn_Exp*_tmpA36=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);_tmp583->f2=_tmpA36;});_tmp583;});_tmp584->hd=_tmpA37;}),_tmp584->tl=0;_tmp584;}));
goto _LL0;case 265U: _LL20D: _LL20E:
# 2338 "parse.y"
 yyval=Cyc_YY5(({struct Cyc_List_List*_tmp586=_cycalloc(sizeof(*_tmp586));({struct _tuple35*_tmpA3A=({struct _tuple35*_tmp585=_cycalloc(sizeof(*_tmp585));({struct Cyc_List_List*_tmpA39=Cyc_yyget_YY41(&(yyyvsp[0]).v);_tmp585->f1=_tmpA39;}),({struct Cyc_Absyn_Exp*_tmpA38=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);_tmp585->f2=_tmpA38;});_tmp585;});_tmp586->hd=_tmpA3A;}),_tmp586->tl=0;_tmp586;}));
goto _LL0;case 266U: _LL20F: _LL210:
# 2340 "parse.y"
 yyval=Cyc_YY5(({struct Cyc_List_List*_tmp588=_cycalloc(sizeof(*_tmp588));({struct _tuple35*_tmpA3D=({struct _tuple35*_tmp587=_cycalloc(sizeof(*_tmp587));_tmp587->f1=0,({struct Cyc_Absyn_Exp*_tmpA3C=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp587->f2=_tmpA3C;});_tmp587;});_tmp588->hd=_tmpA3D;}),({struct Cyc_List_List*_tmpA3B=Cyc_yyget_YY5(&(yyyvsp[0]).v);_tmp588->tl=_tmpA3B;});_tmp588;}));
goto _LL0;case 267U: _LL211: _LL212:
# 2342 "parse.y"
 yyval=Cyc_YY5(({struct Cyc_List_List*_tmp58A=_cycalloc(sizeof(*_tmp58A));({struct _tuple35*_tmpA41=({struct _tuple35*_tmp589=_cycalloc(sizeof(*_tmp589));({struct Cyc_List_List*_tmpA40=Cyc_yyget_YY41(&(yyyvsp[2]).v);_tmp589->f1=_tmpA40;}),({struct Cyc_Absyn_Exp*_tmpA3F=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);_tmp589->f2=_tmpA3F;});_tmp589;});_tmp58A->hd=_tmpA41;}),({struct Cyc_List_List*_tmpA3E=Cyc_yyget_YY5(&(yyyvsp[0]).v);_tmp58A->tl=_tmpA3E;});_tmp58A;}));
goto _LL0;case 268U: _LL213: _LL214:
# 2346 "parse.y"
 yyval=Cyc_YY41(((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY41(&(yyyvsp[0]).v)));
goto _LL0;case 269U: _LL215: _LL216:
# 2347 "parse.y"
 yyval=Cyc_YY41(({struct Cyc_List_List*_tmp58D=_cycalloc(sizeof(*_tmp58D));({void*_tmpA44=(void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_tmp58C=_cycalloc(sizeof(*_tmp58C));_tmp58C->tag=1U,({struct _fat_ptr*_tmpA43=({struct _fat_ptr*_tmp58B=_cycalloc(sizeof(*_tmp58B));({struct _fat_ptr _tmpA42=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp58B=_tmpA42;});_tmp58B;});_tmp58C->f1=_tmpA43;});_tmp58C;});_tmp58D->hd=_tmpA44;}),_tmp58D->tl=0;_tmp58D;}));
goto _LL0;case 270U: _LL217: _LL218:
# 2352 "parse.y"
 yyval=Cyc_YY41(({struct Cyc_List_List*_tmp58E=_cycalloc(sizeof(*_tmp58E));({void*_tmpA45=Cyc_yyget_YY42(&(yyyvsp[0]).v);_tmp58E->hd=_tmpA45;}),_tmp58E->tl=0;_tmp58E;}));
goto _LL0;case 271U: _LL219: _LL21A:
# 2353 "parse.y"
 yyval=Cyc_YY41(({struct Cyc_List_List*_tmp58F=_cycalloc(sizeof(*_tmp58F));({void*_tmpA47=Cyc_yyget_YY42(&(yyyvsp[1]).v);_tmp58F->hd=_tmpA47;}),({struct Cyc_List_List*_tmpA46=Cyc_yyget_YY41(&(yyyvsp[0]).v);_tmp58F->tl=_tmpA46;});_tmp58F;}));
goto _LL0;case 272U: _LL21B: _LL21C:
# 2357 "parse.y"
 yyval=Cyc_YY42((void*)({struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_tmp590=_cycalloc(sizeof(*_tmp590));_tmp590->tag=0U,({struct Cyc_Absyn_Exp*_tmpA48=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);_tmp590->f1=_tmpA48;});_tmp590;}));
goto _LL0;case 273U: _LL21D: _LL21E:
# 2358 "parse.y"
 yyval=Cyc_YY42((void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_tmp592=_cycalloc(sizeof(*_tmp592));_tmp592->tag=1U,({struct _fat_ptr*_tmpA4A=({struct _fat_ptr*_tmp591=_cycalloc(sizeof(*_tmp591));({struct _fat_ptr _tmpA49=Cyc_yyget_String_tok(&(yyyvsp[1]).v);*_tmp591=_tmpA49;});_tmp591;});_tmp592->f1=_tmpA4A;});_tmp592;}));
goto _LL0;case 274U: _LL21F: _LL220: {
# 2363 "parse.y"
struct _tuple26 _tmp593=Cyc_yyget_YY35(&(yyyvsp[0]).v);struct _tuple26 _stmttmp27=_tmp593;struct _tuple26 _tmp594=_stmttmp27;struct Cyc_List_List*_tmp597;struct Cyc_Parse_Type_specifier _tmp596;struct Cyc_Absyn_Tqual _tmp595;_LL48F: _tmp595=_tmp594.f1;_tmp596=_tmp594.f2;_tmp597=_tmp594.f3;_LL490: {struct Cyc_Absyn_Tqual tq=_tmp595;struct Cyc_Parse_Type_specifier tss=_tmp596;struct Cyc_List_List*atts=_tmp597;
void*_tmp598=Cyc_Parse_speclist2typ(tss,(unsigned)((yyyvsp[0]).l).first_line);void*t=_tmp598;
if(atts != 0)
({void*_tmp599=0U;({unsigned _tmpA4C=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmpA4B=({const char*_tmp59A="ignoring attributes in type";_tag_fat(_tmp59A,sizeof(char),28U);});Cyc_Warn_warn(_tmpA4C,_tmpA4B,_tag_fat(_tmp599,sizeof(void*),0U));});});
yyval=Cyc_YY37(({struct _tuple8*_tmp59B=_cycalloc(sizeof(*_tmp59B));_tmp59B->f1=0,_tmp59B->f2=tq,_tmp59B->f3=t;_tmp59B;}));
# 2369
goto _LL0;}}case 275U: _LL221: _LL222: {
# 2370 "parse.y"
struct _tuple26 _tmp59C=Cyc_yyget_YY35(&(yyyvsp[0]).v);struct _tuple26 _stmttmp28=_tmp59C;struct _tuple26 _tmp59D=_stmttmp28;struct Cyc_List_List*_tmp5A0;struct Cyc_Parse_Type_specifier _tmp59F;struct Cyc_Absyn_Tqual _tmp59E;_LL492: _tmp59E=_tmp59D.f1;_tmp59F=_tmp59D.f2;_tmp5A0=_tmp59D.f3;_LL493: {struct Cyc_Absyn_Tqual tq=_tmp59E;struct Cyc_Parse_Type_specifier tss=_tmp59F;struct Cyc_List_List*atts=_tmp5A0;
void*_tmp5A1=Cyc_Parse_speclist2typ(tss,(unsigned)((yyyvsp[0]).l).first_line);void*t=_tmp5A1;
struct Cyc_List_List*_tmp5A2=(Cyc_yyget_YY30(&(yyyvsp[1]).v)).tms;struct Cyc_List_List*tms=_tmp5A2;
struct _tuple14 _tmp5A3=Cyc_Parse_apply_tms(tq,t,atts,tms);struct _tuple14 t_info=_tmp5A3;
if(t_info.f3 != 0)
# 2376
({void*_tmp5A4=0U;({unsigned _tmpA4E=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmpA4D=({const char*_tmp5A5="bad type params, ignoring";_tag_fat(_tmp5A5,sizeof(char),26U);});Cyc_Warn_warn(_tmpA4E,_tmpA4D,_tag_fat(_tmp5A4,sizeof(void*),0U));});});
if(t_info.f4 != 0)
({void*_tmp5A6=0U;({unsigned _tmpA50=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmpA4F=({const char*_tmp5A7="bad specifiers, ignoring";_tag_fat(_tmp5A7,sizeof(char),25U);});Cyc_Warn_warn(_tmpA50,_tmpA4F,_tag_fat(_tmp5A6,sizeof(void*),0U));});});
yyval=Cyc_YY37(({struct _tuple8*_tmp5A8=_cycalloc(sizeof(*_tmp5A8));_tmp5A8->f1=0,_tmp5A8->f2=t_info.f1,_tmp5A8->f3=t_info.f2;_tmp5A8;}));
# 2381
goto _LL0;}}case 276U: _LL223: _LL224:
# 2384 "parse.y"
 yyval=Cyc_YY44(({struct _tuple8*_tmpA51=Cyc_yyget_YY37(&(yyyvsp[0]).v);Cyc_Parse_type_name_to_type(_tmpA51,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 277U: _LL225: _LL226:
# 2385 "parse.y"
 yyval=Cyc_YY44(Cyc_Absyn_join_eff(0));
goto _LL0;case 278U: _LL227: _LL228:
# 2386 "parse.y"
 yyval=Cyc_YY44(Cyc_Absyn_join_eff(Cyc_yyget_YY40(&(yyyvsp[1]).v)));
goto _LL0;case 279U: _LL229: _LL22A:
# 2387 "parse.y"
 yyval=Cyc_YY44(Cyc_Absyn_regionsof_eff(Cyc_yyget_YY44(&(yyyvsp[2]).v)));
goto _LL0;case 280U: _LL22B: _LL22C:
# 2388 "parse.y"
 yyval=Cyc_YY44(Cyc_Absyn_join_eff(({struct Cyc_List_List*_tmp5A9=_cycalloc(sizeof(*_tmp5A9));({void*_tmpA53=Cyc_yyget_YY44(&(yyyvsp[0]).v);_tmp5A9->hd=_tmpA53;}),({struct Cyc_List_List*_tmpA52=Cyc_yyget_YY40(&(yyyvsp[2]).v);_tmp5A9->tl=_tmpA52;});_tmp5A9;})));
goto _LL0;case 281U: _LL22D: _LL22E:
# 2394 "parse.y"
 yyval=Cyc_YY40(({struct Cyc_List_List*_tmp5AA=_cycalloc(sizeof(*_tmp5AA));({void*_tmpA54=Cyc_yyget_YY44(&(yyyvsp[0]).v);_tmp5AA->hd=_tmpA54;}),_tmp5AA->tl=0;_tmp5AA;}));
goto _LL0;case 282U: _LL22F: _LL230:
# 2395 "parse.y"
 yyval=Cyc_YY40(({struct Cyc_List_List*_tmp5AB=_cycalloc(sizeof(*_tmp5AB));({void*_tmpA56=Cyc_yyget_YY44(&(yyyvsp[2]).v);_tmp5AB->hd=_tmpA56;}),({struct Cyc_List_List*_tmpA55=Cyc_yyget_YY40(&(yyyvsp[0]).v);_tmp5AB->tl=_tmpA55;});_tmp5AB;}));
goto _LL0;case 283U: _LL231: _LL232:
# 2400 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp764;({struct Cyc_List_List*_tmpA57=Cyc_yyget_YY26(&(yyyvsp[0]).v);_tmp764.tms=_tmpA57;});_tmp764;}));
goto _LL0;case 284U: _LL233: _LL234:
# 2402 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 285U: _LL235: _LL236:
# 2404 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp765;({struct Cyc_List_List*_tmpA59=({struct Cyc_List_List*_tmpA58=Cyc_yyget_YY26(&(yyyvsp[0]).v);((struct Cyc_List_List*(*)(struct Cyc_List_List*x,struct Cyc_List_List*y))Cyc_List_imp_append)(_tmpA58,(Cyc_yyget_YY30(&(yyyvsp[1]).v)).tms);});_tmp765.tms=_tmpA59;});_tmp765;}));
goto _LL0;case 286U: _LL237: _LL238:
# 2409 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 287U: _LL239: _LL23A:
# 2411 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp766;({struct Cyc_List_List*_tmpA5C=({struct Cyc_List_List*_tmp5AD=_region_malloc(yyr,sizeof(*_tmp5AD));({void*_tmpA5B=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp5AC=_region_malloc(yyr,sizeof(*_tmp5AC));_tmp5AC->tag=0U,({void*_tmpA5A=Cyc_yyget_YY51(&(yyyvsp[2]).v);_tmp5AC->f1=_tmpA5A;}),_tmp5AC->f2=(unsigned)((yyyvsp[2]).l).first_line;_tmp5AC;});_tmp5AD->hd=_tmpA5B;}),_tmp5AD->tl=0;_tmp5AD;});_tmp766.tms=_tmpA5C;});_tmp766;}));
goto _LL0;case 288U: _LL23B: _LL23C:
# 2413 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp767;({struct Cyc_List_List*_tmpA60=({struct Cyc_List_List*_tmp5AF=_region_malloc(yyr,sizeof(*_tmp5AF));({void*_tmpA5F=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp5AE=_region_malloc(yyr,sizeof(*_tmp5AE));_tmp5AE->tag=0U,({void*_tmpA5E=Cyc_yyget_YY51(&(yyyvsp[3]).v);_tmp5AE->f1=_tmpA5E;}),_tmp5AE->f2=(unsigned)((yyyvsp[3]).l).first_line;_tmp5AE;});_tmp5AF->hd=_tmpA5F;}),({struct Cyc_List_List*_tmpA5D=(Cyc_yyget_YY30(&(yyyvsp[0]).v)).tms;_tmp5AF->tl=_tmpA5D;});_tmp5AF;});_tmp767.tms=_tmpA60;});_tmp767;}));
goto _LL0;case 289U: _LL23D: _LL23E:
# 2415 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp768;({struct Cyc_List_List*_tmpA64=({struct Cyc_List_List*_tmp5B1=_region_malloc(yyr,sizeof(*_tmp5B1));({void*_tmpA63=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp5B0=_region_malloc(yyr,sizeof(*_tmp5B0));_tmp5B0->tag=1U,({struct Cyc_Absyn_Exp*_tmpA62=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);_tmp5B0->f1=_tmpA62;}),({void*_tmpA61=Cyc_yyget_YY51(&(yyyvsp[3]).v);_tmp5B0->f2=_tmpA61;}),_tmp5B0->f3=(unsigned)((yyyvsp[3]).l).first_line;_tmp5B0;});_tmp5B1->hd=_tmpA63;}),_tmp5B1->tl=0;_tmp5B1;});_tmp768.tms=_tmpA64;});_tmp768;}));
goto _LL0;case 290U: _LL23F: _LL240:
# 2417 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp769;({struct Cyc_List_List*_tmpA69=({struct Cyc_List_List*_tmp5B3=_region_malloc(yyr,sizeof(*_tmp5B3));({void*_tmpA68=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp5B2=_region_malloc(yyr,sizeof(*_tmp5B2));_tmp5B2->tag=1U,({struct Cyc_Absyn_Exp*_tmpA67=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp5B2->f1=_tmpA67;}),({void*_tmpA66=Cyc_yyget_YY51(&(yyyvsp[4]).v);_tmp5B2->f2=_tmpA66;}),_tmp5B2->f3=(unsigned)((yyyvsp[4]).l).first_line;_tmp5B2;});_tmp5B3->hd=_tmpA68;}),({
struct Cyc_List_List*_tmpA65=(Cyc_yyget_YY30(&(yyyvsp[0]).v)).tms;_tmp5B3->tl=_tmpA65;});_tmp5B3;});
# 2417
_tmp769.tms=_tmpA69;});_tmp769;}));
# 2420
goto _LL0;case 291U: _LL241: _LL242:
# 2421 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp76A;({struct Cyc_List_List*_tmpA70=({struct Cyc_List_List*_tmp5B6=_region_malloc(yyr,sizeof(*_tmp5B6));({void*_tmpA6F=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp5B5=_region_malloc(yyr,sizeof(*_tmp5B5));_tmp5B5->tag=3U,({void*_tmpA6E=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp5B4=_region_malloc(yyr,sizeof(*_tmp5B4));_tmp5B4->tag=1U,_tmp5B4->f1=0,_tmp5B4->f2=0,_tmp5B4->f3=0,({void*_tmpA6D=Cyc_yyget_YY49(&(yyyvsp[1]).v);_tmp5B4->f4=_tmpA6D;}),({struct Cyc_List_List*_tmpA6C=Cyc_yyget_YY50(&(yyyvsp[2]).v);_tmp5B4->f5=_tmpA6C;}),({struct Cyc_Absyn_Exp*_tmpA6B=Cyc_yyget_YY57(&(yyyvsp[4]).v);_tmp5B4->f6=_tmpA6B;}),({struct Cyc_Absyn_Exp*_tmpA6A=Cyc_yyget_YY57(&(yyyvsp[5]).v);_tmp5B4->f7=_tmpA6A;});_tmp5B4;});_tmp5B5->f1=_tmpA6E;});_tmp5B5;});_tmp5B6->hd=_tmpA6F;}),_tmp5B6->tl=0;_tmp5B6;});_tmp76A.tms=_tmpA70;});_tmp76A;}));
# 2423
goto _LL0;case 292U: _LL243: _LL244: {
# 2424 "parse.y"
struct _tuple27*_tmp5B7=Cyc_yyget_YY39(&(yyyvsp[1]).v);struct _tuple27*_stmttmp29=_tmp5B7;struct _tuple27*_tmp5B8=_stmttmp29;struct Cyc_List_List*_tmp5BD;void*_tmp5BC;struct Cyc_Absyn_VarargInfo*_tmp5BB;int _tmp5BA;struct Cyc_List_List*_tmp5B9;_LL495: _tmp5B9=_tmp5B8->f1;_tmp5BA=_tmp5B8->f2;_tmp5BB=_tmp5B8->f3;_tmp5BC=_tmp5B8->f4;_tmp5BD=_tmp5B8->f5;_LL496: {struct Cyc_List_List*lis=_tmp5B9;int b=_tmp5BA;struct Cyc_Absyn_VarargInfo*c=_tmp5BB;void*eff=_tmp5BC;struct Cyc_List_List*po=_tmp5BD;
yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp76B;({struct Cyc_List_List*_tmpA75=({struct Cyc_List_List*_tmp5C0=_region_malloc(yyr,sizeof(*_tmp5C0));({void*_tmpA74=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp5BF=_region_malloc(yyr,sizeof(*_tmp5BF));_tmp5BF->tag=3U,({void*_tmpA73=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp5BE=_region_malloc(yyr,sizeof(*_tmp5BE));_tmp5BE->tag=1U,_tmp5BE->f1=lis,_tmp5BE->f2=b,_tmp5BE->f3=c,_tmp5BE->f4=eff,_tmp5BE->f5=po,({struct Cyc_Absyn_Exp*_tmpA72=Cyc_yyget_YY57(&(yyyvsp[3]).v);_tmp5BE->f6=_tmpA72;}),({struct Cyc_Absyn_Exp*_tmpA71=Cyc_yyget_YY57(&(yyyvsp[4]).v);_tmp5BE->f7=_tmpA71;});_tmp5BE;});_tmp5BF->f1=_tmpA73;});_tmp5BF;});_tmp5C0->hd=_tmpA74;}),_tmp5C0->tl=0;_tmp5C0;});_tmp76B.tms=_tmpA75;});_tmp76B;}));
# 2427
goto _LL0;}}case 293U: _LL245: _LL246:
# 2428 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp76C;({struct Cyc_List_List*_tmpA7D=({struct Cyc_List_List*_tmp5C3=_region_malloc(yyr,sizeof(*_tmp5C3));({void*_tmpA7C=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp5C2=_region_malloc(yyr,sizeof(*_tmp5C2));_tmp5C2->tag=3U,({void*_tmpA7B=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp5C1=_region_malloc(yyr,sizeof(*_tmp5C1));_tmp5C1->tag=1U,_tmp5C1->f1=0,_tmp5C1->f2=0,_tmp5C1->f3=0,({void*_tmpA7A=Cyc_yyget_YY49(&(yyyvsp[2]).v);_tmp5C1->f4=_tmpA7A;}),({struct Cyc_List_List*_tmpA79=Cyc_yyget_YY50(&(yyyvsp[3]).v);_tmp5C1->f5=_tmpA79;}),({struct Cyc_Absyn_Exp*_tmpA78=Cyc_yyget_YY57(&(yyyvsp[5]).v);_tmp5C1->f6=_tmpA78;}),({struct Cyc_Absyn_Exp*_tmpA77=Cyc_yyget_YY57(&(yyyvsp[6]).v);_tmp5C1->f7=_tmpA77;});_tmp5C1;});_tmp5C2->f1=_tmpA7B;});_tmp5C2;});_tmp5C3->hd=_tmpA7C;}),({
struct Cyc_List_List*_tmpA76=(Cyc_yyget_YY30(&(yyyvsp[0]).v)).tms;_tmp5C3->tl=_tmpA76;});_tmp5C3;});
# 2428
_tmp76C.tms=_tmpA7D;});_tmp76C;}));
# 2431
goto _LL0;case 294U: _LL247: _LL248: {
# 2432 "parse.y"
struct _tuple27*_tmp5C4=Cyc_yyget_YY39(&(yyyvsp[2]).v);struct _tuple27*_stmttmp2A=_tmp5C4;struct _tuple27*_tmp5C5=_stmttmp2A;struct Cyc_List_List*_tmp5CA;void*_tmp5C9;struct Cyc_Absyn_VarargInfo*_tmp5C8;int _tmp5C7;struct Cyc_List_List*_tmp5C6;_LL498: _tmp5C6=_tmp5C5->f1;_tmp5C7=_tmp5C5->f2;_tmp5C8=_tmp5C5->f3;_tmp5C9=_tmp5C5->f4;_tmp5CA=_tmp5C5->f5;_LL499: {struct Cyc_List_List*lis=_tmp5C6;int b=_tmp5C7;struct Cyc_Absyn_VarargInfo*c=_tmp5C8;void*eff=_tmp5C9;struct Cyc_List_List*po=_tmp5CA;
yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp76D;({struct Cyc_List_List*_tmpA83=({struct Cyc_List_List*_tmp5CD=_region_malloc(yyr,sizeof(*_tmp5CD));({void*_tmpA82=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp5CC=_region_malloc(yyr,sizeof(*_tmp5CC));_tmp5CC->tag=3U,({void*_tmpA81=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp5CB=_region_malloc(yyr,sizeof(*_tmp5CB));_tmp5CB->tag=1U,_tmp5CB->f1=lis,_tmp5CB->f2=b,_tmp5CB->f3=c,_tmp5CB->f4=eff,_tmp5CB->f5=po,({
struct Cyc_Absyn_Exp*_tmpA80=Cyc_yyget_YY57(&(yyyvsp[4]).v);_tmp5CB->f6=_tmpA80;}),({struct Cyc_Absyn_Exp*_tmpA7F=Cyc_yyget_YY57(&(yyyvsp[5]).v);_tmp5CB->f7=_tmpA7F;});_tmp5CB;});
# 2433
_tmp5CC->f1=_tmpA81;});_tmp5CC;});_tmp5CD->hd=_tmpA82;}),({
struct Cyc_List_List*_tmpA7E=(Cyc_yyget_YY30(&(yyyvsp[0]).v)).tms;_tmp5CD->tl=_tmpA7E;});_tmp5CD;});
# 2433
_tmp76D.tms=_tmpA83;});_tmp76D;}));
# 2436
goto _LL0;}}case 295U: _LL249: _LL24A: {
# 2438
struct Cyc_List_List*_tmp5CE=({unsigned _tmpA84=(unsigned)((yyyvsp[1]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmpA84,((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY40(&(yyyvsp[2]).v)));});struct Cyc_List_List*ts=_tmp5CE;
yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp76E;({struct Cyc_List_List*_tmpA87=({struct Cyc_List_List*_tmp5D0=_region_malloc(yyr,sizeof(*_tmp5D0));({void*_tmpA86=(void*)({struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_tmp5CF=_region_malloc(yyr,sizeof(*_tmp5CF));_tmp5CF->tag=4U,_tmp5CF->f1=ts,_tmp5CF->f2=(unsigned)((yyyvsp[1]).l).first_line,_tmp5CF->f3=0;_tmp5CF;});_tmp5D0->hd=_tmpA86;}),({
struct Cyc_List_List*_tmpA85=(Cyc_yyget_YY30(&(yyyvsp[0]).v)).tms;_tmp5D0->tl=_tmpA85;});_tmp5D0;});
# 2439
_tmp76E.tms=_tmpA87;});_tmp76E;}));
# 2442
goto _LL0;}case 296U: _LL24B: _LL24C:
# 2443 "parse.y"
 yyval=Cyc_YY30(({struct Cyc_Parse_Abstractdeclarator _tmp76F;({struct Cyc_List_List*_tmpA8B=({struct Cyc_List_List*_tmp5D2=_region_malloc(yyr,sizeof(*_tmp5D2));({void*_tmpA8A=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp5D1=_region_malloc(yyr,sizeof(*_tmp5D1));_tmp5D1->tag=5U,_tmp5D1->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmpA89=Cyc_yyget_YY45(&(yyyvsp[1]).v);_tmp5D1->f2=_tmpA89;});_tmp5D1;});_tmp5D2->hd=_tmpA8A;}),({struct Cyc_List_List*_tmpA88=(Cyc_yyget_YY30(&(yyyvsp[0]).v)).tms;_tmp5D2->tl=_tmpA88;});_tmp5D2;});_tmp76F.tms=_tmpA8B;});_tmp76F;}));
# 2445
goto _LL0;case 297U: _LL24D: _LL24E:
# 2449 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 298U: _LL24F: _LL250:
# 2450 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 299U: _LL251: _LL252:
# 2451 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 300U: _LL253: _LL254:
# 2452 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 301U: _LL255: _LL256:
# 2453 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 302U: _LL257: _LL258:
# 2454 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 303U: _LL259: _LL25A:
# 2460 "parse.y"
 yyval=Cyc_Stmt_tok(({void*_tmpA8F=(void*)({struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_tmp5D4=_cycalloc(sizeof(*_tmp5D4));_tmp5D4->tag=13U,({struct _fat_ptr*_tmpA8E=({struct _fat_ptr*_tmp5D3=_cycalloc(sizeof(*_tmp5D3));({struct _fat_ptr _tmpA8D=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp5D3=_tmpA8D;});_tmp5D3;});_tmp5D4->f1=_tmpA8E;}),({struct Cyc_Absyn_Stmt*_tmpA8C=Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);_tmp5D4->f2=_tmpA8C;});_tmp5D4;});Cyc_Absyn_new_stmt(_tmpA8F,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 304U: _LL25B: _LL25C:
# 2464 "parse.y"
 yyval=Cyc_Stmt_tok(Cyc_Absyn_skip_stmt((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 305U: _LL25D: _LL25E:
# 2465 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpA90=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);Cyc_Absyn_exp_stmt(_tmpA90,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 306U: _LL25F: _LL260:
# 2470 "parse.y"
 yyval=Cyc_Stmt_tok(Cyc_Absyn_skip_stmt((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 307U: _LL261: _LL262:
# 2471 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 308U: _LL263: _LL264:
# 2476 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_List_List*_tmpA91=Cyc_yyget_YY16(&(yyyvsp[0]).v);Cyc_Parse_flatten_declarations(_tmpA91,Cyc_Absyn_skip_stmt((unsigned)((yyyvsp[0]).l).first_line));}));
goto _LL0;case 309U: _LL265: _LL266:
# 2477 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_List_List*_tmpA92=Cyc_yyget_YY16(&(yyyvsp[0]).v);Cyc_Parse_flatten_declarations(_tmpA92,Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v));}));
goto _LL0;case 310U: _LL267: _LL268:
# 2478 "parse.y"
 yyval=Cyc_Stmt_tok(({void*_tmpA97=(void*)({struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_tmp5D6=_cycalloc(sizeof(*_tmp5D6));_tmp5D6->tag=13U,({struct _fat_ptr*_tmpA96=({struct _fat_ptr*_tmp5D5=_cycalloc(sizeof(*_tmp5D5));({struct _fat_ptr _tmpA95=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp5D5=_tmpA95;});_tmp5D5;});_tmp5D6->f1=_tmpA96;}),({struct Cyc_Absyn_Stmt*_tmpA94=({struct Cyc_List_List*_tmpA93=Cyc_yyget_YY16(&(yyyvsp[2]).v);Cyc_Parse_flatten_declarations(_tmpA93,Cyc_Absyn_skip_stmt(0U));});_tmp5D6->f2=_tmpA94;});_tmp5D6;});Cyc_Absyn_new_stmt(_tmpA97,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2480
goto _LL0;case 311U: _LL269: _LL26A:
# 2480 "parse.y"
 yyval=Cyc_Stmt_tok(({void*_tmpA9C=(void*)({struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_tmp5D8=_cycalloc(sizeof(*_tmp5D8));_tmp5D8->tag=13U,({struct _fat_ptr*_tmpA9B=({struct _fat_ptr*_tmp5D7=_cycalloc(sizeof(*_tmp5D7));({struct _fat_ptr _tmpA9A=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp5D7=_tmpA9A;});_tmp5D7;});_tmp5D8->f1=_tmpA9B;}),({struct Cyc_Absyn_Stmt*_tmpA99=({struct Cyc_List_List*_tmpA98=Cyc_yyget_YY16(&(yyyvsp[2]).v);Cyc_Parse_flatten_declarations(_tmpA98,Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v));});_tmp5D8->f2=_tmpA99;});_tmp5D8;});Cyc_Absyn_new_stmt(_tmpA9C,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2482
goto _LL0;case 312U: _LL26B: _LL26C:
# 2482 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 313U: _LL26D: _LL26E:
# 2483 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Stmt*_tmpA9E=Cyc_yyget_Stmt_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Stmt*_tmpA9D=Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);Cyc_Absyn_seq_stmt(_tmpA9E,_tmpA9D,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 314U: _LL26F: _LL270:
# 2484 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Decl*_tmpAA1=({void*_tmpAA0=(void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp5D9=_cycalloc(sizeof(*_tmp5D9));_tmp5D9->tag=1U,({struct Cyc_Absyn_Fndecl*_tmpA9F=Cyc_yyget_YY15(&(yyyvsp[0]).v);_tmp5D9->f1=_tmpA9F;});_tmp5D9;});Cyc_Absyn_new_decl(_tmpAA0,(unsigned)((yyyvsp[0]).l).first_line);});Cyc_Parse_flatten_decl(_tmpAA1,
Cyc_Absyn_skip_stmt(0U));}));
goto _LL0;case 315U: _LL271: _LL272:
# 2487 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Decl*_tmpAA4=({void*_tmpAA3=(void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp5DA=_cycalloc(sizeof(*_tmp5DA));_tmp5DA->tag=1U,({struct Cyc_Absyn_Fndecl*_tmpAA2=Cyc_yyget_YY15(&(yyyvsp[0]).v);_tmp5DA->f1=_tmpAA2;});_tmp5DA;});Cyc_Absyn_new_decl(_tmpAA3,(unsigned)((yyyvsp[0]).l).first_line);});Cyc_Parse_flatten_decl(_tmpAA4,Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v));}));
goto _LL0;case 316U: _LL273: _LL274:
# 2492 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAA7=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Stmt*_tmpAA6=Cyc_yyget_Stmt_tok(&(yyyvsp[4]).v);struct Cyc_Absyn_Stmt*_tmpAA5=Cyc_Absyn_skip_stmt(0U);Cyc_Absyn_ifthenelse_stmt(_tmpAA7,_tmpAA6,_tmpAA5,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 317U: _LL275: _LL276:
# 2494 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAAA=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Stmt*_tmpAA9=Cyc_yyget_Stmt_tok(&(yyyvsp[4]).v);struct Cyc_Absyn_Stmt*_tmpAA8=Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);Cyc_Absyn_ifthenelse_stmt(_tmpAAA,_tmpAA9,_tmpAA8,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 318U: _LL277: _LL278:
# 2500 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAAC=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_List_List*_tmpAAB=Cyc_yyget_YY8(&(yyyvsp[5]).v);Cyc_Absyn_switch_stmt(_tmpAAC,_tmpAAB,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 319U: _LL279: _LL27A: {
# 2503
struct Cyc_Absyn_Exp*_tmp5DB=({struct _tuple0*_tmpAAD=Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);Cyc_Absyn_unknownid_exp(_tmpAAD,(unsigned)((yyyvsp[1]).l).first_line);});struct Cyc_Absyn_Exp*e=_tmp5DB;
yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAAF=e;struct Cyc_List_List*_tmpAAE=Cyc_yyget_YY8(&(yyyvsp[3]).v);Cyc_Absyn_switch_stmt(_tmpAAF,_tmpAAE,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;}case 320U: _LL27B: _LL27C: {
# 2507
struct Cyc_Absyn_Exp*_tmp5DC=({struct Cyc_List_List*_tmpAB0=Cyc_yyget_YY4(&(yyyvsp[3]).v);Cyc_Absyn_tuple_exp(_tmpAB0,(unsigned)((yyyvsp[1]).l).first_line);});struct Cyc_Absyn_Exp*e=_tmp5DC;
yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAB2=e;struct Cyc_List_List*_tmpAB1=Cyc_yyget_YY8(&(yyyvsp[6]).v);Cyc_Absyn_switch_stmt(_tmpAB2,_tmpAB1,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2510
goto _LL0;}case 321U: _LL27D: _LL27E:
# 2513 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Stmt*_tmpAB4=Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);struct Cyc_List_List*_tmpAB3=Cyc_yyget_YY8(&(yyyvsp[4]).v);Cyc_Absyn_trycatch_stmt(_tmpAB4,_tmpAB3,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 322U: _LL27F: _LL280:
# 2527 "parse.y"
 yyval=Cyc_YY8(0);
goto _LL0;case 323U: _LL281: _LL282:
# 2530 "parse.y"
 yyval=Cyc_YY8(({struct Cyc_List_List*_tmp5DE=_cycalloc(sizeof(*_tmp5DE));({struct Cyc_Absyn_Switch_clause*_tmpAB8=({struct Cyc_Absyn_Switch_clause*_tmp5DD=_cycalloc(sizeof(*_tmp5DD));({struct Cyc_Absyn_Pat*_tmpAB7=Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,(unsigned)((yyyvsp[0]).l).first_line);_tmp5DD->pattern=_tmpAB7;}),_tmp5DD->pat_vars=0,_tmp5DD->where_clause=0,({
struct Cyc_Absyn_Stmt*_tmpAB6=Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);_tmp5DD->body=_tmpAB6;}),_tmp5DD->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp5DD;});
# 2530
_tmp5DE->hd=_tmpAB8;}),({
# 2532
struct Cyc_List_List*_tmpAB5=Cyc_yyget_YY8(&(yyyvsp[3]).v);_tmp5DE->tl=_tmpAB5;});_tmp5DE;}));
goto _LL0;case 324U: _LL283: _LL284:
# 2534 "parse.y"
 yyval=Cyc_YY8(({struct Cyc_List_List*_tmp5E0=_cycalloc(sizeof(*_tmp5E0));({struct Cyc_Absyn_Switch_clause*_tmpABC=({struct Cyc_Absyn_Switch_clause*_tmp5DF=_cycalloc(sizeof(*_tmp5DF));({struct Cyc_Absyn_Pat*_tmpABB=Cyc_yyget_YY9(&(yyyvsp[1]).v);_tmp5DF->pattern=_tmpABB;}),_tmp5DF->pat_vars=0,_tmp5DF->where_clause=0,({
struct Cyc_Absyn_Stmt*_tmpABA=Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[2]).l).first_line);_tmp5DF->body=_tmpABA;}),_tmp5DF->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp5DF;});
# 2534
_tmp5E0->hd=_tmpABC;}),({
# 2536
struct Cyc_List_List*_tmpAB9=Cyc_yyget_YY8(&(yyyvsp[3]).v);_tmp5E0->tl=_tmpAB9;});_tmp5E0;}));
goto _LL0;case 325U: _LL285: _LL286:
# 2538 "parse.y"
 yyval=Cyc_YY8(({struct Cyc_List_List*_tmp5E2=_cycalloc(sizeof(*_tmp5E2));({struct Cyc_Absyn_Switch_clause*_tmpAC0=({struct Cyc_Absyn_Switch_clause*_tmp5E1=_cycalloc(sizeof(*_tmp5E1));({struct Cyc_Absyn_Pat*_tmpABF=Cyc_yyget_YY9(&(yyyvsp[1]).v);_tmp5E1->pattern=_tmpABF;}),_tmp5E1->pat_vars=0,_tmp5E1->where_clause=0,({struct Cyc_Absyn_Stmt*_tmpABE=Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v);_tmp5E1->body=_tmpABE;}),_tmp5E1->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp5E1;});_tmp5E2->hd=_tmpAC0;}),({struct Cyc_List_List*_tmpABD=Cyc_yyget_YY8(&(yyyvsp[4]).v);_tmp5E2->tl=_tmpABD;});_tmp5E2;}));
goto _LL0;case 326U: _LL287: _LL288:
# 2540 "parse.y"
 yyval=Cyc_YY8(({struct Cyc_List_List*_tmp5E4=_cycalloc(sizeof(*_tmp5E4));({struct Cyc_Absyn_Switch_clause*_tmpAC5=({struct Cyc_Absyn_Switch_clause*_tmp5E3=_cycalloc(sizeof(*_tmp5E3));({struct Cyc_Absyn_Pat*_tmpAC4=Cyc_yyget_YY9(&(yyyvsp[1]).v);_tmp5E3->pattern=_tmpAC4;}),_tmp5E3->pat_vars=0,({struct Cyc_Absyn_Exp*_tmpAC3=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);_tmp5E3->where_clause=_tmpAC3;}),({
struct Cyc_Absyn_Stmt*_tmpAC2=Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[4]).l).first_line);_tmp5E3->body=_tmpAC2;}),_tmp5E3->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp5E3;});
# 2540
_tmp5E4->hd=_tmpAC5;}),({
# 2542
struct Cyc_List_List*_tmpAC1=Cyc_yyget_YY8(&(yyyvsp[5]).v);_tmp5E4->tl=_tmpAC1;});_tmp5E4;}));
goto _LL0;case 327U: _LL289: _LL28A:
# 2544 "parse.y"
 yyval=Cyc_YY8(({struct Cyc_List_List*_tmp5E6=_cycalloc(sizeof(*_tmp5E6));({struct Cyc_Absyn_Switch_clause*_tmpACA=({struct Cyc_Absyn_Switch_clause*_tmp5E5=_cycalloc(sizeof(*_tmp5E5));({struct Cyc_Absyn_Pat*_tmpAC9=Cyc_yyget_YY9(&(yyyvsp[1]).v);_tmp5E5->pattern=_tmpAC9;}),_tmp5E5->pat_vars=0,({struct Cyc_Absyn_Exp*_tmpAC8=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);_tmp5E5->where_clause=_tmpAC8;}),({struct Cyc_Absyn_Stmt*_tmpAC7=Cyc_yyget_Stmt_tok(&(yyyvsp[5]).v);_tmp5E5->body=_tmpAC7;}),_tmp5E5->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp5E5;});_tmp5E6->hd=_tmpACA;}),({struct Cyc_List_List*_tmpAC6=Cyc_yyget_YY8(&(yyyvsp[6]).v);_tmp5E6->tl=_tmpAC6;});_tmp5E6;}));
goto _LL0;case 328U: _LL28B: _LL28C:
# 2551 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpACC=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Stmt*_tmpACB=Cyc_yyget_Stmt_tok(&(yyyvsp[4]).v);Cyc_Absyn_while_stmt(_tmpACC,_tmpACB,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 329U: _LL28D: _LL28E:
# 2555 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Stmt*_tmpACE=Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);struct Cyc_Absyn_Exp*_tmpACD=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);Cyc_Absyn_do_stmt(_tmpACE,_tmpACD,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 330U: _LL28F: _LL290:
# 2559 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAD2=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpAD1=Cyc_Absyn_true_exp(0U);struct Cyc_Absyn_Exp*_tmpAD0=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Stmt*_tmpACF=
Cyc_yyget_Stmt_tok(&(yyyvsp[5]).v);
# 2559
Cyc_Absyn_for_stmt(_tmpAD2,_tmpAD1,_tmpAD0,_tmpACF,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2561
goto _LL0;case 331U: _LL291: _LL292:
# 2562 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAD6=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpAD5=Cyc_Absyn_true_exp(0U);struct Cyc_Absyn_Exp*_tmpAD4=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);struct Cyc_Absyn_Stmt*_tmpAD3=
Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);
# 2562
Cyc_Absyn_for_stmt(_tmpAD6,_tmpAD5,_tmpAD4,_tmpAD3,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2564
goto _LL0;case 332U: _LL293: _LL294:
# 2565 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpADA=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpAD9=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);struct Cyc_Absyn_Exp*_tmpAD8=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Stmt*_tmpAD7=
Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);
# 2565
Cyc_Absyn_for_stmt(_tmpADA,_tmpAD9,_tmpAD8,_tmpAD7,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2567
goto _LL0;case 333U: _LL295: _LL296:
# 2568 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpADE=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpADD=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);struct Cyc_Absyn_Exp*_tmpADC=Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);struct Cyc_Absyn_Stmt*_tmpADB=
Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);
# 2568
Cyc_Absyn_for_stmt(_tmpADE,_tmpADD,_tmpADC,_tmpADB,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2570
goto _LL0;case 334U: _LL297: _LL298:
# 2571 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAE2=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpAE1=Cyc_Absyn_true_exp(0U);struct Cyc_Absyn_Exp*_tmpAE0=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Stmt*_tmpADF=
Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);
# 2571
Cyc_Absyn_for_stmt(_tmpAE2,_tmpAE1,_tmpAE0,_tmpADF,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2573
goto _LL0;case 335U: _LL299: _LL29A:
# 2574 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAE6=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpAE5=Cyc_Absyn_true_exp(0U);struct Cyc_Absyn_Exp*_tmpAE4=Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);struct Cyc_Absyn_Stmt*_tmpAE3=
Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);
# 2574
Cyc_Absyn_for_stmt(_tmpAE6,_tmpAE5,_tmpAE4,_tmpAE3,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2576
goto _LL0;case 336U: _LL29B: _LL29C:
# 2577 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAEA=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpAE9=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);struct Cyc_Absyn_Exp*_tmpAE8=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Stmt*_tmpAE7=
Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);
# 2577
Cyc_Absyn_for_stmt(_tmpAEA,_tmpAE9,_tmpAE8,_tmpAE7,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2579
goto _LL0;case 337U: _LL29D: _LL29E:
# 2580 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpAEE=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpAED=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);struct Cyc_Absyn_Exp*_tmpAEC=Cyc_yyget_Exp_tok(&(yyyvsp[6]).v);struct Cyc_Absyn_Stmt*_tmpAEB=
Cyc_yyget_Stmt_tok(&(yyyvsp[8]).v);
# 2580
Cyc_Absyn_for_stmt(_tmpAEE,_tmpAED,_tmpAEC,_tmpAEB,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2582
goto _LL0;case 338U: _LL29F: _LL2A0: {
# 2583 "parse.y"
struct Cyc_Absyn_Stmt*_tmp5E7=({struct Cyc_Absyn_Exp*_tmpAF2=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpAF1=Cyc_Absyn_true_exp(0U);struct Cyc_Absyn_Exp*_tmpAF0=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Stmt*_tmpAEF=
Cyc_yyget_Stmt_tok(&(yyyvsp[5]).v);
# 2583
Cyc_Absyn_for_stmt(_tmpAF2,_tmpAF1,_tmpAF0,_tmpAEF,(unsigned)((yyyvsp[0]).l).first_line);});struct Cyc_Absyn_Stmt*s=_tmp5E7;
# 2585
yyval=Cyc_Stmt_tok(({struct Cyc_List_List*_tmpAF3=Cyc_yyget_YY16(&(yyyvsp[2]).v);Cyc_Parse_flatten_declarations(_tmpAF3,s);}));
# 2587
goto _LL0;}case 339U: _LL2A1: _LL2A2: {
# 2588 "parse.y"
struct Cyc_Absyn_Stmt*_tmp5E8=({struct Cyc_Absyn_Exp*_tmpAF7=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpAF6=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);struct Cyc_Absyn_Exp*_tmpAF5=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Stmt*_tmpAF4=Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);Cyc_Absyn_for_stmt(_tmpAF7,_tmpAF6,_tmpAF5,_tmpAF4,(unsigned)((yyyvsp[0]).l).first_line);});struct Cyc_Absyn_Stmt*s=_tmp5E8;
yyval=Cyc_Stmt_tok(({struct Cyc_List_List*_tmpAF8=Cyc_yyget_YY16(&(yyyvsp[2]).v);Cyc_Parse_flatten_declarations(_tmpAF8,s);}));
# 2591
goto _LL0;}case 340U: _LL2A3: _LL2A4: {
# 2592 "parse.y"
struct Cyc_Absyn_Stmt*_tmp5E9=({struct Cyc_Absyn_Exp*_tmpAFC=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpAFB=Cyc_Absyn_true_exp(0U);struct Cyc_Absyn_Exp*_tmpAFA=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);struct Cyc_Absyn_Stmt*_tmpAF9=Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);Cyc_Absyn_for_stmt(_tmpAFC,_tmpAFB,_tmpAFA,_tmpAF9,(unsigned)((yyyvsp[0]).l).first_line);});struct Cyc_Absyn_Stmt*s=_tmp5E9;
yyval=Cyc_Stmt_tok(({struct Cyc_List_List*_tmpAFD=Cyc_yyget_YY16(&(yyyvsp[2]).v);Cyc_Parse_flatten_declarations(_tmpAFD,s);}));
# 2595
goto _LL0;}case 341U: _LL2A5: _LL2A6: {
# 2596 "parse.y"
struct Cyc_Absyn_Stmt*_tmp5EA=({struct Cyc_Absyn_Exp*_tmpB01=Cyc_Absyn_false_exp(0U);struct Cyc_Absyn_Exp*_tmpB00=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);struct Cyc_Absyn_Exp*_tmpAFF=Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);struct Cyc_Absyn_Stmt*_tmpAFE=Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);Cyc_Absyn_for_stmt(_tmpB01,_tmpB00,_tmpAFF,_tmpAFE,(unsigned)((yyyvsp[0]).l).first_line);});struct Cyc_Absyn_Stmt*s=_tmp5EA;
yyval=Cyc_Stmt_tok(({struct Cyc_List_List*_tmpB02=Cyc_yyget_YY16(&(yyyvsp[2]).v);Cyc_Parse_flatten_declarations(_tmpB02,s);}));
# 2599
goto _LL0;}case 342U: _LL2A7: _LL2A8:
# 2602 "parse.y"
 yyval=Cyc_Stmt_tok(({struct _fat_ptr*_tmpB04=({struct _fat_ptr*_tmp5EB=_cycalloc(sizeof(*_tmp5EB));({struct _fat_ptr _tmpB03=Cyc_yyget_String_tok(&(yyyvsp[1]).v);*_tmp5EB=_tmpB03;});_tmp5EB;});Cyc_Absyn_goto_stmt(_tmpB04,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 343U: _LL2A9: _LL2AA:
# 2603 "parse.y"
 yyval=Cyc_Stmt_tok(Cyc_Absyn_continue_stmt((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 344U: _LL2AB: _LL2AC:
# 2604 "parse.y"
 yyval=Cyc_Stmt_tok(Cyc_Absyn_break_stmt((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 345U: _LL2AD: _LL2AE:
# 2605 "parse.y"
 yyval=Cyc_Stmt_tok(Cyc_Absyn_return_stmt(0,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 346U: _LL2AF: _LL2B0:
# 2606 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_Absyn_Exp*_tmpB05=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_return_stmt(_tmpB05,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 347U: _LL2B1: _LL2B2:
# 2608 "parse.y"
 yyval=Cyc_Stmt_tok(Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 348U: _LL2B3: _LL2B4:
# 2609 "parse.y"
 yyval=Cyc_Stmt_tok(Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 349U: _LL2B5: _LL2B6:
# 2611 "parse.y"
 yyval=Cyc_Stmt_tok(({struct Cyc_List_List*_tmpB06=Cyc_yyget_YY4(&(yyyvsp[2]).v);Cyc_Absyn_fallthru_stmt(_tmpB06,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 350U: _LL2B7: _LL2B8:
# 2620 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 351U: _LL2B9: _LL2BA:
# 2623
 yyval=(yyyvsp[0]).v;
goto _LL0;case 352U: _LL2BB: _LL2BC:
# 2625 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB09=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB08=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpB07=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);Cyc_Absyn_conditional_exp(_tmpB09,_tmpB08,_tmpB07,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 353U: _LL2BD: _LL2BE:
# 2628
 yyval=(yyyvsp[0]).v;
goto _LL0;case 354U: _LL2BF: _LL2C0:
# 2630 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB0B=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB0A=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_or_exp(_tmpB0B,_tmpB0A,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 355U: _LL2C1: _LL2C2:
# 2633
 yyval=(yyyvsp[0]).v;
goto _LL0;case 356U: _LL2C3: _LL2C4:
# 2635 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB0D=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB0C=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_and_exp(_tmpB0D,_tmpB0C,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 357U: _LL2C5: _LL2C6:
# 2638
 yyval=(yyyvsp[0]).v;
goto _LL0;case 358U: _LL2C7: _LL2C8:
# 2640 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB0F=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB0E=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitor,_tmpB0F,_tmpB0E,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 359U: _LL2C9: _LL2CA:
# 2643
 yyval=(yyyvsp[0]).v;
goto _LL0;case 360U: _LL2CB: _LL2CC:
# 2645 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB11=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB10=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitxor,_tmpB11,_tmpB10,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 361U: _LL2CD: _LL2CE:
# 2648
 yyval=(yyyvsp[0]).v;
goto _LL0;case 362U: _LL2CF: _LL2D0:
# 2650 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB13=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB12=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitand,_tmpB13,_tmpB12,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 363U: _LL2D1: _LL2D2:
# 2653
 yyval=(yyyvsp[0]).v;
goto _LL0;case 364U: _LL2D3: _LL2D4:
# 2655 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB15=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB14=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_eq_exp(_tmpB15,_tmpB14,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 365U: _LL2D5: _LL2D6:
# 2657 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB17=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB16=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_neq_exp(_tmpB17,_tmpB16,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 366U: _LL2D7: _LL2D8:
# 2660
 yyval=(yyyvsp[0]).v;
goto _LL0;case 367U: _LL2D9: _LL2DA:
# 2662 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB19=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB18=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_lt_exp(_tmpB19,_tmpB18,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 368U: _LL2DB: _LL2DC:
# 2664 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB1B=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB1A=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_gt_exp(_tmpB1B,_tmpB1A,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 369U: _LL2DD: _LL2DE:
# 2666 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB1D=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB1C=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_lte_exp(_tmpB1D,_tmpB1C,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 370U: _LL2DF: _LL2E0:
# 2668 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB1F=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB1E=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_gte_exp(_tmpB1F,_tmpB1E,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 371U: _LL2E1: _LL2E2:
# 2671
 yyval=(yyyvsp[0]).v;
goto _LL0;case 372U: _LL2E3: _LL2E4:
# 2673 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB21=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB20=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitlshift,_tmpB21,_tmpB20,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 373U: _LL2E5: _LL2E6:
# 2675 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB23=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB22=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitlrshift,_tmpB23,_tmpB22,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 374U: _LL2E7: _LL2E8:
# 2678
 yyval=(yyyvsp[0]).v;
goto _LL0;case 375U: _LL2E9: _LL2EA:
# 2680 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB25=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB24=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Plus,_tmpB25,_tmpB24,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 376U: _LL2EB: _LL2EC:
# 2682 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB27=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB26=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Minus,_tmpB27,_tmpB26,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 377U: _LL2ED: _LL2EE:
# 2685
 yyval=(yyyvsp[0]).v;
goto _LL0;case 378U: _LL2EF: _LL2F0:
# 2687 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB29=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB28=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Times,_tmpB29,_tmpB28,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 379U: _LL2F1: _LL2F2:
# 2689 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB2B=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB2A=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Div,_tmpB2B,_tmpB2A,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 380U: _LL2F3: _LL2F4:
# 2691 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB2D=Cyc_Parse_pat2exp(Cyc_yyget_YY9(&(yyyvsp[0]).v));struct Cyc_Absyn_Exp*_tmpB2C=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Mod,_tmpB2D,_tmpB2C,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 381U: _LL2F5: _LL2F6:
# 2694
 yyval=(yyyvsp[0]).v;
goto _LL0;case 382U: _LL2F7: _LL2F8: {
# 2696 "parse.y"
void*_tmp5EC=({struct _tuple8*_tmpB2E=Cyc_yyget_YY37(&(yyyvsp[1]).v);Cyc_Parse_type_name_to_type(_tmpB2E,(unsigned)((yyyvsp[1]).l).first_line);});void*t=_tmp5EC;
yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({void*_tmpB30=t;struct Cyc_Absyn_Exp*_tmpB2F=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);Cyc_Absyn_cast_exp(_tmpB30,_tmpB2F,1,Cyc_Absyn_Unknown_coercion,(unsigned)((yyyvsp[0]).l).first_line);})));
# 2699
goto _LL0;}case 383U: _LL2F9: _LL2FA:
# 2702 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 384U: _LL2FB: _LL2FC:
# 2705
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(Cyc_yyget_Exp_tok(&(yyyvsp[1]).v)));
goto _LL0;case 385U: _LL2FD: _LL2FE:
# 2707 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({enum Cyc_Absyn_Primop _tmpB32=Cyc_yyget_YY6(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpB31=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_prim1_exp(_tmpB32,_tmpB31,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 386U: _LL2FF: _LL300: {
# 2709 "parse.y"
void*_tmp5ED=({struct _tuple8*_tmpB33=Cyc_yyget_YY37(&(yyyvsp[2]).v);Cyc_Parse_type_name_to_type(_tmpB33,(unsigned)((yyyvsp[2]).l).first_line);});void*t=_tmp5ED;
yyval=Cyc_YY9(Cyc_Absyn_exp_pat(Cyc_Absyn_sizeoftype_exp(t,(unsigned)((yyyvsp[0]).l).first_line)));
# 2712
goto _LL0;}case 387U: _LL301: _LL302:
# 2713 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({struct Cyc_Absyn_Exp*_tmpB34=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_sizeofexp_exp(_tmpB34,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 388U: _LL303: _LL304:
# 2715 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(({void*_tmpB36=(*Cyc_yyget_YY37(&(yyyvsp[2]).v)).f3;struct Cyc_List_List*_tmpB35=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY3(&(yyyvsp[4]).v));Cyc_Absyn_offsetof_exp(_tmpB36,_tmpB35,(unsigned)((yyyvsp[0]).l).first_line);})));
goto _LL0;case 389U: _LL305: _LL306:
# 2720 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 390U: _LL307: _LL308:
# 2728 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 391U: _LL309: _LL30A:
# 2733 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 392U: _LL30B: _LL30C:
# 2735 "parse.y"
 yyval=Cyc_YY9(Cyc_Absyn_exp_pat(Cyc_yyget_Exp_tok(&(yyyvsp[1]).v)));
goto _LL0;case 393U: _LL30D: _LL30E: {
# 2737 "parse.y"
struct Cyc_Absyn_Exp*e=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);
{void*_tmp5EE=e->r;void*_stmttmp2B=_tmp5EE;void*_tmp5EF=_stmttmp2B;int _tmp5F1;struct _fat_ptr _tmp5F0;int _tmp5F3;enum Cyc_Absyn_Sign _tmp5F2;short _tmp5F5;enum Cyc_Absyn_Sign _tmp5F4;char _tmp5F7;enum Cyc_Absyn_Sign _tmp5F6;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->tag == 0U)switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).LongLong_c).tag){case 2U: _LL49B: _tmp5F6=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Char_c).val).f1;_tmp5F7=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Char_c).val).f2;_LL49C: {enum Cyc_Absyn_Sign s=_tmp5F6;char i=_tmp5F7;
# 2741
yyval=Cyc_YY9(({void*_tmpB37=(void*)({struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*_tmp5F8=_cycalloc(sizeof(*_tmp5F8));_tmp5F8->tag=11U,_tmp5F8->f1=i;_tmp5F8;});Cyc_Absyn_new_pat(_tmpB37,e->loc);}));goto _LL49A;}case 4U: _LL49D: _tmp5F4=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Short_c).val).f1;_tmp5F5=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Short_c).val).f2;_LL49E: {enum Cyc_Absyn_Sign s=_tmp5F4;short i=_tmp5F5;
# 2743
yyval=Cyc_YY9(({void*_tmpB38=(void*)({struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_tmp5F9=_cycalloc(sizeof(*_tmp5F9));_tmp5F9->tag=10U,_tmp5F9->f1=s,_tmp5F9->f2=(int)i;_tmp5F9;});Cyc_Absyn_new_pat(_tmpB38,e->loc);}));goto _LL49A;}case 5U: _LL49F: _tmp5F2=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Int_c).val).f1;_tmp5F3=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Int_c).val).f2;_LL4A0: {enum Cyc_Absyn_Sign s=_tmp5F2;int i=_tmp5F3;
# 2745
yyval=Cyc_YY9(({void*_tmpB39=(void*)({struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_tmp5FA=_cycalloc(sizeof(*_tmp5FA));_tmp5FA->tag=10U,_tmp5FA->f1=s,_tmp5FA->f2=i;_tmp5FA;});Cyc_Absyn_new_pat(_tmpB39,e->loc);}));goto _LL49A;}case 7U: _LL4A1: _tmp5F0=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Float_c).val).f1;_tmp5F1=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp5EF)->f1).Float_c).val).f2;_LL4A2: {struct _fat_ptr s=_tmp5F0;int i=_tmp5F1;
# 2747
yyval=Cyc_YY9(({void*_tmpB3A=(void*)({struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*_tmp5FB=_cycalloc(sizeof(*_tmp5FB));_tmp5FB->tag=12U,_tmp5FB->f1=s,_tmp5FB->f2=i;_tmp5FB;});Cyc_Absyn_new_pat(_tmpB3A,e->loc);}));goto _LL49A;}case 1U: _LL4A3: _LL4A4:
# 2749
 yyval=Cyc_YY9(Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Null_p_val,e->loc));goto _LL49A;case 8U: _LL4A5: _LL4A6:
 goto _LL4A8;case 9U: _LL4A7: _LL4A8:
# 2752
({void*_tmp5FC=0U;({unsigned _tmpB3C=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmpB3B=({const char*_tmp5FD="strings cannot occur within patterns";_tag_fat(_tmp5FD,sizeof(char),37U);});Cyc_Warn_err(_tmpB3C,_tmpB3B,_tag_fat(_tmp5FC,sizeof(void*),0U));});});goto _LL49A;case 6U: _LL4A9: _LL4AA:
# 2754
({void*_tmp5FE=0U;({unsigned _tmpB3E=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmpB3D=({const char*_tmp5FF="long long's in patterns not yet implemented";_tag_fat(_tmp5FF,sizeof(char),44U);});Cyc_Warn_err(_tmpB3E,_tmpB3D,_tag_fat(_tmp5FE,sizeof(void*),0U));});});goto _LL49A;default: goto _LL4AB;}else{_LL4AB: _LL4AC:
# 2756
({void*_tmp600=0U;({unsigned _tmpB40=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmpB3F=({const char*_tmp601="bad constant in case";_tag_fat(_tmp601,sizeof(char),21U);});Cyc_Warn_err(_tmpB40,_tmpB3F,_tag_fat(_tmp600,sizeof(void*),0U));});});}_LL49A:;}
# 2759
goto _LL0;}case 394U: _LL30F: _LL310:
# 2760 "parse.y"
 yyval=Cyc_YY9(({void*_tmpB42=(void*)({struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*_tmp602=_cycalloc(sizeof(*_tmp602));_tmp602->tag=15U,({struct _tuple0*_tmpB41=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp602->f1=_tmpB41;});_tmp602;});Cyc_Absyn_new_pat(_tmpB42,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 395U: _LL311: _LL312:
# 2762 "parse.y"
 if(({struct _fat_ptr _tmpB43=(struct _fat_ptr)Cyc_yyget_String_tok(&(yyyvsp[1]).v);Cyc_strcmp(_tmpB43,({const char*_tmp603="as";_tag_fat(_tmp603,sizeof(char),3U);}));})!= 0)
({void*_tmp604=0U;({unsigned _tmpB45=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmpB44=({const char*_tmp605="expecting `as'";_tag_fat(_tmp605,sizeof(char),15U);});Cyc_Warn_err(_tmpB45,_tmpB44,_tag_fat(_tmp604,sizeof(void*),0U));});});
yyval=Cyc_YY9(({void*_tmpB4C=(void*)({struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_tmp608=_cycalloc(sizeof(*_tmp608));_tmp608->tag=1U,({struct Cyc_Absyn_Vardecl*_tmpB4B=({unsigned _tmpB4A=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpB49=({struct _tuple0*_tmp607=_cycalloc(sizeof(*_tmp607));_tmp607->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpB48=({struct _fat_ptr*_tmp606=_cycalloc(sizeof(*_tmp606));({struct _fat_ptr _tmpB47=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp606=_tmpB47;});_tmp606;});_tmp607->f2=_tmpB48;});_tmp607;});Cyc_Absyn_new_vardecl(_tmpB4A,_tmpB49,Cyc_Absyn_void_type,0);});_tmp608->f1=_tmpB4B;}),({
struct Cyc_Absyn_Pat*_tmpB46=Cyc_yyget_YY9(&(yyyvsp[2]).v);_tmp608->f2=_tmpB46;});_tmp608;});
# 2764
Cyc_Absyn_new_pat(_tmpB4C,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2767
goto _LL0;case 396U: _LL313: _LL314:
# 2768 "parse.y"
 if(({struct _fat_ptr _tmpB4D=(struct _fat_ptr)Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_strcmp(_tmpB4D,({const char*_tmp609="alias";_tag_fat(_tmp609,sizeof(char),6U);}));})!= 0)
({void*_tmp60A=0U;({unsigned _tmpB4F=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmpB4E=({const char*_tmp60B="expecting `alias'";_tag_fat(_tmp60B,sizeof(char),18U);});Cyc_Warn_err(_tmpB4F,_tmpB4E,_tag_fat(_tmp60A,sizeof(void*),0U));});});{
int _tmp60C=((yyyvsp[0]).l).first_line;int location=_tmp60C;
struct _fat_ptr err=({const char*_tmp614="";_tag_fat(_tmp614,sizeof(char),1U);});
if(!Cyc_Parse_tvar_ok(Cyc_yyget_String_tok(&(yyyvsp[2]).v),& err))({void*_tmp60D=0U;({unsigned _tmpB51=(unsigned)location;struct _fat_ptr _tmpB50=err;Cyc_Warn_err(_tmpB51,_tmpB50,_tag_fat(_tmp60D,sizeof(void*),0U));});});{
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp613=_cycalloc(sizeof(*_tmp613));({struct _fat_ptr*_tmpB54=({struct _fat_ptr*_tmp611=_cycalloc(sizeof(*_tmp611));({struct _fat_ptr _tmpB53=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp611=_tmpB53;});_tmp611;});_tmp613->name=_tmpB54;}),_tmp613->identity=- 1,({void*_tmpB52=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp612=_cycalloc(sizeof(*_tmp612));_tmp612->tag=0U,_tmp612->f1=& Cyc_Tcutil_rk;_tmp612;});_tmp613->kind=_tmpB52;});_tmp613;});
struct Cyc_Absyn_Vardecl*vd=({unsigned _tmpB59=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpB58=({struct _tuple0*_tmp610=_cycalloc(sizeof(*_tmp610));_tmp610->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpB56=({struct _fat_ptr*_tmp60F=_cycalloc(sizeof(*_tmp60F));({struct _fat_ptr _tmpB55=Cyc_yyget_String_tok(&(yyyvsp[5]).v);*_tmp60F=_tmpB55;});_tmp60F;});_tmp610->f2=_tmpB56;});_tmp610;});Cyc_Absyn_new_vardecl(_tmpB59,_tmpB58,({
struct _tuple8*_tmpB57=Cyc_yyget_YY37(&(yyyvsp[4]).v);Cyc_Parse_type_name_to_type(_tmpB57,(unsigned)((yyyvsp[4]).l).first_line);}),0);});
yyval=Cyc_YY9(({void*_tmpB5A=(void*)({struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*_tmp60E=_cycalloc(sizeof(*_tmp60E));_tmp60E->tag=2U,_tmp60E->f1=tv,_tmp60E->f2=vd;_tmp60E;});Cyc_Absyn_new_pat(_tmpB5A,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2778
goto _LL0;}}case 397U: _LL315: _LL316:
# 2779 "parse.y"
 if(({struct _fat_ptr _tmpB5B=(struct _fat_ptr)Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_strcmp(_tmpB5B,({const char*_tmp615="alias";_tag_fat(_tmp615,sizeof(char),6U);}));})!= 0)
({void*_tmp616=0U;({unsigned _tmpB5D=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmpB5C=({const char*_tmp617="expecting `alias'";_tag_fat(_tmp617,sizeof(char),18U);});Cyc_Warn_err(_tmpB5D,_tmpB5C,_tag_fat(_tmp616,sizeof(void*),0U));});});{
int _tmp618=((yyyvsp[0]).l).first_line;int location=_tmp618;
struct _fat_ptr err=({const char*_tmp620="";_tag_fat(_tmp620,sizeof(char),1U);});
if(!Cyc_Parse_tvar_ok(Cyc_yyget_String_tok(&(yyyvsp[2]).v),& err))({void*_tmp619=0U;({unsigned _tmpB5F=(unsigned)location;struct _fat_ptr _tmpB5E=err;Cyc_Warn_err(_tmpB5F,_tmpB5E,_tag_fat(_tmp619,sizeof(void*),0U));});});{
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp61F=_cycalloc(sizeof(*_tmp61F));({struct _fat_ptr*_tmpB62=({struct _fat_ptr*_tmp61D=_cycalloc(sizeof(*_tmp61D));({struct _fat_ptr _tmpB61=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp61D=_tmpB61;});_tmp61D;});_tmp61F->name=_tmpB62;}),_tmp61F->identity=- 1,({void*_tmpB60=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp61E=_cycalloc(sizeof(*_tmp61E));_tmp61E->tag=0U,_tmp61E->f1=& Cyc_Tcutil_rk;_tmp61E;});_tmp61F->kind=_tmpB60;});_tmp61F;});
struct Cyc_Absyn_Vardecl*vd=({unsigned _tmpB67=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpB66=({struct _tuple0*_tmp61C=_cycalloc(sizeof(*_tmp61C));_tmp61C->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpB64=({struct _fat_ptr*_tmp61B=_cycalloc(sizeof(*_tmp61B));({struct _fat_ptr _tmpB63=Cyc_yyget_String_tok(&(yyyvsp[5]).v);*_tmp61B=_tmpB63;});_tmp61B;});_tmp61C->f2=_tmpB64;});_tmp61C;});Cyc_Absyn_new_vardecl(_tmpB67,_tmpB66,({
struct _tuple8*_tmpB65=Cyc_yyget_YY37(&(yyyvsp[4]).v);Cyc_Parse_type_name_to_type(_tmpB65,(unsigned)((yyyvsp[4]).l).first_line);}),0);});
yyval=Cyc_YY9(({void*_tmpB68=(void*)({struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*_tmp61A=_cycalloc(sizeof(*_tmp61A));_tmp61A->tag=2U,_tmp61A->f1=tv,_tmp61A->f2=vd;_tmp61A;});Cyc_Absyn_new_pat(_tmpB68,(unsigned)location);}));
# 2789
goto _LL0;}}case 398U: _LL317: _LL318: {
# 2790 "parse.y"
struct _tuple23 _tmp621=*Cyc_yyget_YY10(&(yyyvsp[2]).v);struct _tuple23 _stmttmp2C=_tmp621;struct _tuple23 _tmp622=_stmttmp2C;int _tmp624;struct Cyc_List_List*_tmp623;_LL4AE: _tmp623=_tmp622.f1;_tmp624=_tmp622.f2;_LL4AF: {struct Cyc_List_List*ps=_tmp623;int dots=_tmp624;
yyval=Cyc_YY9(({void*_tmpB69=(void*)({struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*_tmp625=_cycalloc(sizeof(*_tmp625));_tmp625->tag=5U,_tmp625->f1=ps,_tmp625->f2=dots;_tmp625;});Cyc_Absyn_new_pat(_tmpB69,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2793
goto _LL0;}}case 399U: _LL319: _LL31A: {
# 2794 "parse.y"
struct _tuple23 _tmp626=*Cyc_yyget_YY10(&(yyyvsp[2]).v);struct _tuple23 _stmttmp2D=_tmp626;struct _tuple23 _tmp627=_stmttmp2D;int _tmp629;struct Cyc_List_List*_tmp628;_LL4B1: _tmp628=_tmp627.f1;_tmp629=_tmp627.f2;_LL4B2: {struct Cyc_List_List*ps=_tmp628;int dots=_tmp629;
yyval=Cyc_YY9(({void*_tmpB6B=(void*)({struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*_tmp62A=_cycalloc(sizeof(*_tmp62A));_tmp62A->tag=16U,({struct _tuple0*_tmpB6A=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp62A->f1=_tmpB6A;}),_tmp62A->f2=ps,_tmp62A->f3=dots;_tmp62A;});Cyc_Absyn_new_pat(_tmpB6B,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2797
goto _LL0;}}case 400U: _LL31B: _LL31C: {
# 2798 "parse.y"
struct _tuple23 _tmp62B=*Cyc_yyget_YY14(&(yyyvsp[3]).v);struct _tuple23 _stmttmp2E=_tmp62B;struct _tuple23 _tmp62C=_stmttmp2E;int _tmp62E;struct Cyc_List_List*_tmp62D;_LL4B4: _tmp62D=_tmp62C.f1;_tmp62E=_tmp62C.f2;_LL4B5: {struct Cyc_List_List*fps=_tmp62D;int dots=_tmp62E;
struct Cyc_List_List*_tmp62F=({unsigned _tmpB6C=(unsigned)((yyyvsp[2]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmpB6C,Cyc_yyget_YY40(&(yyyvsp[2]).v));});struct Cyc_List_List*exist_ts=_tmp62F;
yyval=Cyc_YY9(({void*_tmpB6F=(void*)({struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_tmp631=_cycalloc(sizeof(*_tmp631));_tmp631->tag=7U,({union Cyc_Absyn_AggrInfo*_tmpB6E=({union Cyc_Absyn_AggrInfo*_tmp630=_cycalloc(sizeof(*_tmp630));({union Cyc_Absyn_AggrInfo _tmpB6D=Cyc_Absyn_UnknownAggr(Cyc_Absyn_StructA,Cyc_yyget_QualId_tok(&(yyyvsp[0]).v),0);*_tmp630=_tmpB6D;});_tmp630;});_tmp631->f1=_tmpB6E;}),_tmp631->f2=exist_ts,_tmp631->f3=fps,_tmp631->f4=dots;_tmp631;});Cyc_Absyn_new_pat(_tmpB6F,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2803
goto _LL0;}}case 401U: _LL31D: _LL31E: {
# 2804 "parse.y"
struct _tuple23 _tmp632=*Cyc_yyget_YY14(&(yyyvsp[2]).v);struct _tuple23 _stmttmp2F=_tmp632;struct _tuple23 _tmp633=_stmttmp2F;int _tmp635;struct Cyc_List_List*_tmp634;_LL4B7: _tmp634=_tmp633.f1;_tmp635=_tmp633.f2;_LL4B8: {struct Cyc_List_List*fps=_tmp634;int dots=_tmp635;
struct Cyc_List_List*_tmp636=({unsigned _tmpB70=(unsigned)((yyyvsp[1]).l).first_line;((struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c)(Cyc_Parse_typ2tvar,_tmpB70,Cyc_yyget_YY40(&(yyyvsp[1]).v));});struct Cyc_List_List*exist_ts=_tmp636;
yyval=Cyc_YY9(({void*_tmpB71=(void*)({struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_tmp637=_cycalloc(sizeof(*_tmp637));_tmp637->tag=7U,_tmp637->f1=0,_tmp637->f2=exist_ts,_tmp637->f3=fps,_tmp637->f4=dots;_tmp637;});Cyc_Absyn_new_pat(_tmpB71,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2808
goto _LL0;}}case 402U: _LL31F: _LL320:
# 2809 "parse.y"
 yyval=Cyc_YY9(({void*_tmpB73=(void*)({struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_tmp638=_cycalloc(sizeof(*_tmp638));_tmp638->tag=6U,({struct Cyc_Absyn_Pat*_tmpB72=Cyc_yyget_YY9(&(yyyvsp[1]).v);_tmp638->f1=_tmpB72;});_tmp638;});Cyc_Absyn_new_pat(_tmpB73,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 403U: _LL321: _LL322:
# 2811 "parse.y"
 yyval=Cyc_YY9(({void*_tmpB77=(void*)({struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_tmp63A=_cycalloc(sizeof(*_tmp63A));_tmp63A->tag=6U,({struct Cyc_Absyn_Pat*_tmpB76=({void*_tmpB75=(void*)({struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_tmp639=_cycalloc(sizeof(*_tmp639));_tmp639->tag=6U,({struct Cyc_Absyn_Pat*_tmpB74=Cyc_yyget_YY9(&(yyyvsp[1]).v);_tmp639->f1=_tmpB74;});_tmp639;});Cyc_Absyn_new_pat(_tmpB75,(unsigned)((yyyvsp[0]).l).first_line);});_tmp63A->f1=_tmpB76;});_tmp63A;});Cyc_Absyn_new_pat(_tmpB77,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 404U: _LL323: _LL324:
# 2813 "parse.y"
 yyval=Cyc_YY9(({void*_tmpB7E=(void*)({struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_tmp63D=_cycalloc(sizeof(*_tmp63D));_tmp63D->tag=3U,({struct Cyc_Absyn_Vardecl*_tmpB7D=({unsigned _tmpB7C=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpB7B=({struct _tuple0*_tmp63C=_cycalloc(sizeof(*_tmp63C));_tmp63C->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpB7A=({struct _fat_ptr*_tmp63B=_cycalloc(sizeof(*_tmp63B));({struct _fat_ptr _tmpB79=Cyc_yyget_String_tok(&(yyyvsp[1]).v);*_tmp63B=_tmpB79;});_tmp63B;});_tmp63C->f2=_tmpB7A;});_tmp63C;});Cyc_Absyn_new_vardecl(_tmpB7C,_tmpB7B,Cyc_Absyn_void_type,0);});_tmp63D->f1=_tmpB7D;}),({
# 2815
struct Cyc_Absyn_Pat*_tmpB78=Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,(unsigned)((yyyvsp[1]).l).first_line);_tmp63D->f2=_tmpB78;});_tmp63D;});
# 2813
Cyc_Absyn_new_pat(_tmpB7E,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2817
goto _LL0;case 405U: _LL325: _LL326:
# 2818 "parse.y"
 if(({struct _fat_ptr _tmpB7F=(struct _fat_ptr)Cyc_yyget_String_tok(&(yyyvsp[2]).v);Cyc_strcmp(_tmpB7F,({const char*_tmp63E="as";_tag_fat(_tmp63E,sizeof(char),3U);}));})!= 0)
({void*_tmp63F=0U;({unsigned _tmpB81=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmpB80=({const char*_tmp640="expecting `as'";_tag_fat(_tmp640,sizeof(char),15U);});Cyc_Warn_err(_tmpB81,_tmpB80,_tag_fat(_tmp63F,sizeof(void*),0U));});});
yyval=Cyc_YY9(({void*_tmpB88=(void*)({struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_tmp643=_cycalloc(sizeof(*_tmp643));_tmp643->tag=3U,({struct Cyc_Absyn_Vardecl*_tmpB87=({unsigned _tmpB86=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpB85=({struct _tuple0*_tmp642=_cycalloc(sizeof(*_tmp642));_tmp642->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpB84=({struct _fat_ptr*_tmp641=_cycalloc(sizeof(*_tmp641));({struct _fat_ptr _tmpB83=Cyc_yyget_String_tok(&(yyyvsp[1]).v);*_tmp641=_tmpB83;});_tmp641;});_tmp642->f2=_tmpB84;});_tmp642;});Cyc_Absyn_new_vardecl(_tmpB86,_tmpB85,Cyc_Absyn_void_type,0);});_tmp643->f1=_tmpB87;}),({
# 2822
struct Cyc_Absyn_Pat*_tmpB82=Cyc_yyget_YY9(&(yyyvsp[3]).v);_tmp643->f2=_tmpB82;});_tmp643;});
# 2820
Cyc_Absyn_new_pat(_tmpB88,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2824
goto _LL0;case 406U: _LL327: _LL328: {
# 2825 "parse.y"
void*_tmp644=({struct _fat_ptr _tmpB89=Cyc_yyget_String_tok(&(yyyvsp[2]).v);Cyc_Parse_id2type(_tmpB89,Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_ik));});void*tag=_tmp644;
yyval=Cyc_YY9(({void*_tmpB90=(void*)({struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*_tmp647=_cycalloc(sizeof(*_tmp647));_tmp647->tag=4U,({struct Cyc_Absyn_Tvar*_tmpB8F=Cyc_Parse_typ2tvar((unsigned)((yyyvsp[2]).l).first_line,tag);_tmp647->f1=_tmpB8F;}),({
struct Cyc_Absyn_Vardecl*_tmpB8E=({unsigned _tmpB8D=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpB8C=({struct _tuple0*_tmp646=_cycalloc(sizeof(*_tmp646));_tmp646->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpB8B=({struct _fat_ptr*_tmp645=_cycalloc(sizeof(*_tmp645));({struct _fat_ptr _tmpB8A=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp645=_tmpB8A;});_tmp645;});_tmp646->f2=_tmpB8B;});_tmp646;});Cyc_Absyn_new_vardecl(_tmpB8D,_tmpB8C,
Cyc_Absyn_tag_type(tag),0);});
# 2827
_tmp647->f2=_tmpB8E;});_tmp647;});
# 2826
Cyc_Absyn_new_pat(_tmpB90,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2830
goto _LL0;}case 407U: _LL329: _LL32A: {
# 2831 "parse.y"
struct Cyc_Absyn_Tvar*_tmp648=Cyc_Tcutil_new_tvar(Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_ik));struct Cyc_Absyn_Tvar*tv=_tmp648;
yyval=Cyc_YY9(({void*_tmpB96=(void*)({struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*_tmp64B=_cycalloc(sizeof(*_tmp64B));_tmp64B->tag=4U,_tmp64B->f1=tv,({
struct Cyc_Absyn_Vardecl*_tmpB95=({unsigned _tmpB94=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpB93=({struct _tuple0*_tmp64A=_cycalloc(sizeof(*_tmp64A));_tmp64A->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmpB92=({struct _fat_ptr*_tmp649=_cycalloc(sizeof(*_tmp649));({struct _fat_ptr _tmpB91=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp649=_tmpB91;});_tmp649;});_tmp64A->f2=_tmpB92;});_tmp64A;});Cyc_Absyn_new_vardecl(_tmpB94,_tmpB93,
Cyc_Absyn_tag_type(Cyc_Absyn_var_type(tv)),0);});
# 2833
_tmp64B->f2=_tmpB95;});_tmp64B;});
# 2832
Cyc_Absyn_new_pat(_tmpB96,(unsigned)((yyyvsp[0]).l).first_line);}));
# 2836
goto _LL0;}case 408U: _LL32B: _LL32C:
# 2839 "parse.y"
 yyval=Cyc_YY10(({struct _tuple23*_tmp64C=_cycalloc(sizeof(*_tmp64C));({struct Cyc_List_List*_tmpB97=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_rev)(Cyc_yyget_YY11(&(yyyvsp[0]).v));_tmp64C->f1=_tmpB97;}),_tmp64C->f2=0;_tmp64C;}));
goto _LL0;case 409U: _LL32D: _LL32E:
# 2840 "parse.y"
 yyval=Cyc_YY10(({struct _tuple23*_tmp64D=_cycalloc(sizeof(*_tmp64D));({struct Cyc_List_List*_tmpB98=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_rev)(Cyc_yyget_YY11(&(yyyvsp[0]).v));_tmp64D->f1=_tmpB98;}),_tmp64D->f2=1;_tmp64D;}));
goto _LL0;case 410U: _LL32F: _LL330:
# 2841 "parse.y"
 yyval=Cyc_YY10(({struct _tuple23*_tmp64E=_cycalloc(sizeof(*_tmp64E));_tmp64E->f1=0,_tmp64E->f2=1;_tmp64E;}));
goto _LL0;case 411U: _LL331: _LL332:
# 2846 "parse.y"
 yyval=Cyc_YY11(({struct Cyc_List_List*_tmp64F=_cycalloc(sizeof(*_tmp64F));({struct Cyc_Absyn_Pat*_tmpB99=Cyc_yyget_YY9(&(yyyvsp[0]).v);_tmp64F->hd=_tmpB99;}),_tmp64F->tl=0;_tmp64F;}));
goto _LL0;case 412U: _LL333: _LL334:
# 2848 "parse.y"
 yyval=Cyc_YY11(({struct Cyc_List_List*_tmp650=_cycalloc(sizeof(*_tmp650));({struct Cyc_Absyn_Pat*_tmpB9B=Cyc_yyget_YY9(&(yyyvsp[2]).v);_tmp650->hd=_tmpB9B;}),({struct Cyc_List_List*_tmpB9A=Cyc_yyget_YY11(&(yyyvsp[0]).v);_tmp650->tl=_tmpB9A;});_tmp650;}));
goto _LL0;case 413U: _LL335: _LL336:
# 2853 "parse.y"
 yyval=Cyc_YY12(({struct _tuple24*_tmp651=_cycalloc(sizeof(*_tmp651));_tmp651->f1=0,({struct Cyc_Absyn_Pat*_tmpB9C=Cyc_yyget_YY9(&(yyyvsp[0]).v);_tmp651->f2=_tmpB9C;});_tmp651;}));
goto _LL0;case 414U: _LL337: _LL338:
# 2855 "parse.y"
 yyval=Cyc_YY12(({struct _tuple24*_tmp652=_cycalloc(sizeof(*_tmp652));({struct Cyc_List_List*_tmpB9E=Cyc_yyget_YY41(&(yyyvsp[0]).v);_tmp652->f1=_tmpB9E;}),({struct Cyc_Absyn_Pat*_tmpB9D=Cyc_yyget_YY9(&(yyyvsp[1]).v);_tmp652->f2=_tmpB9D;});_tmp652;}));
goto _LL0;case 415U: _LL339: _LL33A:
# 2858
 yyval=Cyc_YY14(({struct _tuple23*_tmp653=_cycalloc(sizeof(*_tmp653));({struct Cyc_List_List*_tmpB9F=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_rev)(Cyc_yyget_YY13(&(yyyvsp[0]).v));_tmp653->f1=_tmpB9F;}),_tmp653->f2=0;_tmp653;}));
goto _LL0;case 416U: _LL33B: _LL33C:
# 2859 "parse.y"
 yyval=Cyc_YY14(({struct _tuple23*_tmp654=_cycalloc(sizeof(*_tmp654));({struct Cyc_List_List*_tmpBA0=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_rev)(Cyc_yyget_YY13(&(yyyvsp[0]).v));_tmp654->f1=_tmpBA0;}),_tmp654->f2=1;_tmp654;}));
goto _LL0;case 417U: _LL33D: _LL33E:
# 2860 "parse.y"
 yyval=Cyc_YY14(({struct _tuple23*_tmp655=_cycalloc(sizeof(*_tmp655));_tmp655->f1=0,_tmp655->f2=1;_tmp655;}));
goto _LL0;case 418U: _LL33F: _LL340:
# 2865 "parse.y"
 yyval=Cyc_YY13(({struct Cyc_List_List*_tmp656=_cycalloc(sizeof(*_tmp656));({struct _tuple24*_tmpBA1=Cyc_yyget_YY12(&(yyyvsp[0]).v);_tmp656->hd=_tmpBA1;}),_tmp656->tl=0;_tmp656;}));
goto _LL0;case 419U: _LL341: _LL342:
# 2867 "parse.y"
 yyval=Cyc_YY13(({struct Cyc_List_List*_tmp657=_cycalloc(sizeof(*_tmp657));({struct _tuple24*_tmpBA3=Cyc_yyget_YY12(&(yyyvsp[2]).v);_tmp657->hd=_tmpBA3;}),({struct Cyc_List_List*_tmpBA2=Cyc_yyget_YY13(&(yyyvsp[0]).v);_tmp657->tl=_tmpBA2;});_tmp657;}));
goto _LL0;case 420U: _LL343: _LL344:
# 2873 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 421U: _LL345: _LL346:
# 2875 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBA5=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBA4=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_seq_exp(_tmpBA5,_tmpBA4,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 422U: _LL347: _LL348:
# 2880 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 423U: _LL349: _LL34A:
# 2882 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBA8=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Core_Opt*_tmpBA7=Cyc_yyget_YY7(&(yyyvsp[1]).v);struct Cyc_Absyn_Exp*_tmpBA6=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_assignop_exp(_tmpBA8,_tmpBA7,_tmpBA6,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 424U: _LL34B: _LL34C:
# 2884 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBAA=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBA9=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_swap_exp(_tmpBAA,_tmpBA9,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 425U: _LL34D: _LL34E:
# 2888 "parse.y"
 yyval=Cyc_YY7(0);
goto _LL0;case 426U: _LL34F: _LL350:
# 2889 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp658=_cycalloc(sizeof(*_tmp658));_tmp658->v=(void*)Cyc_Absyn_Times;_tmp658;}));
goto _LL0;case 427U: _LL351: _LL352:
# 2890 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp659=_cycalloc(sizeof(*_tmp659));_tmp659->v=(void*)Cyc_Absyn_Div;_tmp659;}));
goto _LL0;case 428U: _LL353: _LL354:
# 2891 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp65A=_cycalloc(sizeof(*_tmp65A));_tmp65A->v=(void*)Cyc_Absyn_Mod;_tmp65A;}));
goto _LL0;case 429U: _LL355: _LL356:
# 2892 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp65B=_cycalloc(sizeof(*_tmp65B));_tmp65B->v=(void*)Cyc_Absyn_Plus;_tmp65B;}));
goto _LL0;case 430U: _LL357: _LL358:
# 2893 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp65C=_cycalloc(sizeof(*_tmp65C));_tmp65C->v=(void*)Cyc_Absyn_Minus;_tmp65C;}));
goto _LL0;case 431U: _LL359: _LL35A:
# 2894 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp65D=_cycalloc(sizeof(*_tmp65D));_tmp65D->v=(void*)Cyc_Absyn_Bitlshift;_tmp65D;}));
goto _LL0;case 432U: _LL35B: _LL35C:
# 2895 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp65E=_cycalloc(sizeof(*_tmp65E));_tmp65E->v=(void*)Cyc_Absyn_Bitlrshift;_tmp65E;}));
goto _LL0;case 433U: _LL35D: _LL35E:
# 2896 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp65F=_cycalloc(sizeof(*_tmp65F));_tmp65F->v=(void*)Cyc_Absyn_Bitand;_tmp65F;}));
goto _LL0;case 434U: _LL35F: _LL360:
# 2897 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp660=_cycalloc(sizeof(*_tmp660));_tmp660->v=(void*)Cyc_Absyn_Bitxor;_tmp660;}));
goto _LL0;case 435U: _LL361: _LL362:
# 2898 "parse.y"
 yyval=Cyc_YY7(({struct Cyc_Core_Opt*_tmp661=_cycalloc(sizeof(*_tmp661));_tmp661->v=(void*)Cyc_Absyn_Bitor;_tmp661;}));
goto _LL0;case 436U: _LL363: _LL364:
# 2903 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 437U: _LL365: _LL366:
# 2905 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBAD=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBAC=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpBAB=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);Cyc_Absyn_conditional_exp(_tmpBAD,_tmpBAC,_tmpBAB,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 438U: _LL367: _LL368:
# 2908
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBAE=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_throw_exp(_tmpBAE,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 439U: _LL369: _LL36A:
# 2911
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBAF=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_New_exp(0,_tmpBAF,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 440U: _LL36B: _LL36C:
# 2913 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBB0=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_New_exp(0,_tmpBB0,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 441U: _LL36D: _LL36E:
# 2915 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBB2=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpBB1=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);Cyc_Absyn_New_exp(_tmpBB2,_tmpBB1,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 442U: _LL36F: _LL370:
# 2917 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBB4=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*_tmpBB3=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);Cyc_Absyn_New_exp(_tmpBB4,_tmpBB3,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 443U: _LL371: _LL372:
# 2921 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 444U: _LL373: _LL374:
# 2925 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 445U: _LL375: _LL376:
# 2927 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBB6=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBB5=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_or_exp(_tmpBB6,_tmpBB5,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 446U: _LL377: _LL378:
# 2931 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 447U: _LL379: _LL37A:
# 2933 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBB8=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBB7=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_and_exp(_tmpBB8,_tmpBB7,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 448U: _LL37B: _LL37C:
# 2937 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 449U: _LL37D: _LL37E:
# 2939 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBBA=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBB9=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitor,_tmpBBA,_tmpBB9,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 450U: _LL37F: _LL380:
# 2943 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 451U: _LL381: _LL382:
# 2945 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBBC=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBBB=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitxor,_tmpBBC,_tmpBBB,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 452U: _LL383: _LL384:
# 2949 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 453U: _LL385: _LL386:
# 2951 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBBE=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBBD=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitand,_tmpBBE,_tmpBBD,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 454U: _LL387: _LL388:
# 2955 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 455U: _LL389: _LL38A:
# 2957 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBC0=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBBF=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_eq_exp(_tmpBC0,_tmpBBF,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 456U: _LL38B: _LL38C:
# 2959 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBC2=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBC1=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_neq_exp(_tmpBC2,_tmpBC1,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 457U: _LL38D: _LL38E:
# 2963 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 458U: _LL38F: _LL390:
# 2965 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBC4=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBC3=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_lt_exp(_tmpBC4,_tmpBC3,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 459U: _LL391: _LL392:
# 2967 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBC6=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBC5=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_gt_exp(_tmpBC6,_tmpBC5,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 460U: _LL393: _LL394:
# 2969 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBC8=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBC7=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_lte_exp(_tmpBC8,_tmpBC7,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 461U: _LL395: _LL396:
# 2971 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBCA=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBC9=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_gte_exp(_tmpBCA,_tmpBC9,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 462U: _LL397: _LL398:
# 2975 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 463U: _LL399: _LL39A:
# 2977 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBCC=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBCB=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitlshift,_tmpBCC,_tmpBCB,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 464U: _LL39B: _LL39C:
# 2979 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBCE=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBCD=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Bitlrshift,_tmpBCE,_tmpBCD,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 465U: _LL39D: _LL39E:
# 2983 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 466U: _LL39F: _LL3A0:
# 2985 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBD0=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBCF=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Plus,_tmpBD0,_tmpBCF,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 467U: _LL3A1: _LL3A2:
# 2987 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBD2=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBD1=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Minus,_tmpBD2,_tmpBD1,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 468U: _LL3A3: _LL3A4:
# 2991 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 469U: _LL3A5: _LL3A6:
# 2993 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBD4=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBD3=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Times,_tmpBD4,_tmpBD3,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 470U: _LL3A7: _LL3A8:
# 2995 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBD6=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBD5=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Div,_tmpBD6,_tmpBD5,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 471U: _LL3A9: _LL3AA:
# 2997 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBD8=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBD7=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_prim2_exp(Cyc_Absyn_Mod,_tmpBD8,_tmpBD7,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 472U: _LL3AB: _LL3AC:
# 3001 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 473U: _LL3AD: _LL3AE: {
# 3003 "parse.y"
void*_tmp662=({struct _tuple8*_tmpBD9=Cyc_yyget_YY37(&(yyyvsp[1]).v);Cyc_Parse_type_name_to_type(_tmpBD9,(unsigned)((yyyvsp[1]).l).first_line);});void*t=_tmp662;
yyval=Cyc_Exp_tok(({void*_tmpBDB=t;struct Cyc_Absyn_Exp*_tmpBDA=Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);Cyc_Absyn_cast_exp(_tmpBDB,_tmpBDA,1,Cyc_Absyn_Unknown_coercion,(unsigned)((yyyvsp[0]).l).first_line);}));
# 3006
goto _LL0;}case 474U: _LL3AF: _LL3B0:
# 3009 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 475U: _LL3B1: _LL3B2:
# 3010 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBDC=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_increment_exp(_tmpBDC,Cyc_Absyn_PreInc,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 476U: _LL3B3: _LL3B4:
# 3011 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBDD=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_increment_exp(_tmpBDD,Cyc_Absyn_PreDec,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 477U: _LL3B5: _LL3B6:
# 3012 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBDE=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_address_exp(_tmpBDE,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 478U: _LL3B7: _LL3B8:
# 3013 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBDF=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_deref_exp(_tmpBDF,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 479U: _LL3B9: _LL3BA:
# 3014 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBE0=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_prim1_exp(Cyc_Absyn_Plus,_tmpBE0,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 480U: _LL3BB: _LL3BC:
# 3015 "parse.y"
 yyval=Cyc_Exp_tok(({enum Cyc_Absyn_Primop _tmpBE2=Cyc_yyget_YY6(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpBE1=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_prim1_exp(_tmpBE2,_tmpBE1,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 481U: _LL3BD: _LL3BE: {
# 3017 "parse.y"
void*_tmp663=({struct _tuple8*_tmpBE3=Cyc_yyget_YY37(&(yyyvsp[2]).v);Cyc_Parse_type_name_to_type(_tmpBE3,(unsigned)((yyyvsp[2]).l).first_line);});void*t=_tmp663;
yyval=Cyc_Exp_tok(Cyc_Absyn_sizeoftype_exp(t,(unsigned)((yyyvsp[0]).l).first_line));
# 3020
goto _LL0;}case 482U: _LL3BF: _LL3C0:
# 3020 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpBE4=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_sizeofexp_exp(_tmpBE4,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 483U: _LL3C1: _LL3C2: {
# 3022 "parse.y"
void*_tmp664=({struct _tuple8*_tmpBE5=Cyc_yyget_YY37(&(yyyvsp[2]).v);Cyc_Parse_type_name_to_type(_tmpBE5,(unsigned)((yyyvsp[2]).l).first_line);});void*t=_tmp664;
yyval=Cyc_Exp_tok(({void*_tmpBE7=t;struct Cyc_List_List*_tmpBE6=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY3(&(yyyvsp[4]).v));Cyc_Absyn_offsetof_exp(_tmpBE7,_tmpBE6,(unsigned)((yyyvsp[0]).l).first_line);}));
# 3025
goto _LL0;}case 484U: _LL3C3: _LL3C4:
# 3027
 yyval=Cyc_Exp_tok(({void*_tmpBE9=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmp665=_cycalloc(sizeof(*_tmp665));_tmp665->tag=34U,(_tmp665->f1).is_calloc=0,(_tmp665->f1).rgn=0,(_tmp665->f1).elt_type=0,({struct Cyc_Absyn_Exp*_tmpBE8=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);(_tmp665->f1).num_elts=_tmpBE8;}),(_tmp665->f1).fat_result=0,(_tmp665->f1).inline_call=0;_tmp665;});Cyc_Absyn_new_exp(_tmpBE9,(unsigned)((yyyvsp[0]).l).first_line);}));
# 3029
goto _LL0;case 485U: _LL3C5: _LL3C6:
# 3030 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpBEC=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmp666=_cycalloc(sizeof(*_tmp666));_tmp666->tag=34U,(_tmp666->f1).is_calloc=0,({struct Cyc_Absyn_Exp*_tmpBEB=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);(_tmp666->f1).rgn=_tmpBEB;}),(_tmp666->f1).elt_type=0,({struct Cyc_Absyn_Exp*_tmpBEA=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);(_tmp666->f1).num_elts=_tmpBEA;}),(_tmp666->f1).fat_result=0,(_tmp666->f1).inline_call=0;_tmp666;});Cyc_Absyn_new_exp(_tmpBEC,(unsigned)((yyyvsp[0]).l).first_line);}));
# 3032
goto _LL0;case 486U: _LL3C7: _LL3C8:
# 3033 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpBEF=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmp667=_cycalloc(sizeof(*_tmp667));_tmp667->tag=34U,(_tmp667->f1).is_calloc=0,({struct Cyc_Absyn_Exp*_tmpBEE=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);(_tmp667->f1).rgn=_tmpBEE;}),(_tmp667->f1).elt_type=0,({struct Cyc_Absyn_Exp*_tmpBED=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);(_tmp667->f1).num_elts=_tmpBED;}),(_tmp667->f1).fat_result=0,(_tmp667->f1).inline_call=1;_tmp667;});Cyc_Absyn_new_exp(_tmpBEF,(unsigned)((yyyvsp[0]).l).first_line);}));
# 3035
goto _LL0;case 487U: _LL3C9: _LL3CA: {
# 3036 "parse.y"
void*_tmp668=({struct _tuple8*_tmpBF0=Cyc_yyget_YY37(&(yyyvsp[6]).v);Cyc_Parse_type_name_to_type(_tmpBF0,(unsigned)((yyyvsp[6]).l).first_line);});void*t=_tmp668;
yyval=Cyc_Exp_tok(({void*_tmpBF3=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmp66A=_cycalloc(sizeof(*_tmp66A));_tmp66A->tag=34U,(_tmp66A->f1).is_calloc=1,(_tmp66A->f1).rgn=0,({void**_tmpBF2=({void**_tmp669=_cycalloc(sizeof(*_tmp669));*_tmp669=t;_tmp669;});(_tmp66A->f1).elt_type=_tmpBF2;}),({struct Cyc_Absyn_Exp*_tmpBF1=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);(_tmp66A->f1).num_elts=_tmpBF1;}),(_tmp66A->f1).fat_result=0,(_tmp66A->f1).inline_call=0;_tmp66A;});Cyc_Absyn_new_exp(_tmpBF3,(unsigned)((yyyvsp[0]).l).first_line);}));
# 3039
goto _LL0;}case 488U: _LL3CB: _LL3CC: {
# 3041
void*_tmp66B=({struct _tuple8*_tmpBF4=Cyc_yyget_YY37(&(yyyvsp[8]).v);Cyc_Parse_type_name_to_type(_tmpBF4,(unsigned)((yyyvsp[8]).l).first_line);});void*t=_tmp66B;
yyval=Cyc_Exp_tok(({void*_tmpBF8=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmp66D=_cycalloc(sizeof(*_tmp66D));_tmp66D->tag=34U,(_tmp66D->f1).is_calloc=1,({struct Cyc_Absyn_Exp*_tmpBF7=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);(_tmp66D->f1).rgn=_tmpBF7;}),({void**_tmpBF6=({void**_tmp66C=_cycalloc(sizeof(*_tmp66C));*_tmp66C=t;_tmp66C;});(_tmp66D->f1).elt_type=_tmpBF6;}),({struct Cyc_Absyn_Exp*_tmpBF5=Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);(_tmp66D->f1).num_elts=_tmpBF5;}),(_tmp66D->f1).fat_result=0,(_tmp66D->f1).inline_call=0;_tmp66D;});Cyc_Absyn_new_exp(_tmpBF8,(unsigned)((yyyvsp[0]).l).first_line);}));
# 3044
goto _LL0;}case 489U: _LL3CD: _LL3CE:
# 3045 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_List_List*_tmpBFA=({struct Cyc_Absyn_Exp*_tmp66E[1U];({struct Cyc_Absyn_Exp*_tmpBF9=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp66E[0]=_tmpBF9;});((struct Cyc_List_List*(*)(struct _fat_ptr))Cyc_List_list)(_tag_fat(_tmp66E,sizeof(struct Cyc_Absyn_Exp*),1U));});Cyc_Absyn_primop_exp(Cyc_Absyn_Numelts,_tmpBFA,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 490U: _LL3CF: _LL3D0:
# 3047 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpBFE=(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_tmp670=_cycalloc(sizeof(*_tmp670));_tmp670->tag=38U,({struct Cyc_Absyn_Exp*_tmpBFD=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp670->f1=_tmpBFD;}),({struct _fat_ptr*_tmpBFC=({struct _fat_ptr*_tmp66F=_cycalloc(sizeof(*_tmp66F));({struct _fat_ptr _tmpBFB=Cyc_yyget_String_tok(&(yyyvsp[4]).v);*_tmp66F=_tmpBFB;});_tmp66F;});_tmp670->f2=_tmpBFC;});_tmp670;});Cyc_Absyn_new_exp(_tmpBFE,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 491U: _LL3D1: _LL3D2:
# 3049 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpC03=(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_tmp672=_cycalloc(sizeof(*_tmp672));_tmp672->tag=38U,({struct Cyc_Absyn_Exp*_tmpC02=({struct Cyc_Absyn_Exp*_tmpC01=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_deref_exp(_tmpC01,(unsigned)((yyyvsp[2]).l).first_line);});_tmp672->f1=_tmpC02;}),({struct _fat_ptr*_tmpC00=({struct _fat_ptr*_tmp671=_cycalloc(sizeof(*_tmp671));({struct _fat_ptr _tmpBFF=Cyc_yyget_String_tok(&(yyyvsp[4]).v);*_tmp671=_tmpBFF;});_tmp671;});_tmp672->f2=_tmpC00;});_tmp672;});Cyc_Absyn_new_exp(_tmpC03,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 492U: _LL3D3: _LL3D4: {
# 3051 "parse.y"
void*_tmp673=({struct _tuple8*_tmpC04=Cyc_yyget_YY37(&(yyyvsp[2]).v);Cyc_Parse_type_name_to_type(_tmpC04,(unsigned)((yyyvsp[2]).l).first_line);});void*t=_tmp673;
yyval=Cyc_Exp_tok(Cyc_Absyn_valueof_exp(t,(unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;}case 493U: _LL3D5: _LL3D6:
# 3054 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpC05=Cyc_yyget_YY58(&(yyyvsp[1]).v);Cyc_Absyn_new_exp(_tmpC05,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 494U: _LL3D7: _LL3D8:
# 3055 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC06=Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);Cyc_Absyn_extension_exp(_tmpC06,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 495U: _LL3D9: _LL3DA:
# 3057 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC07=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_assert_exp(_tmpC07,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 496U: _LL3DB: _LL3DC: {
# 3062 "parse.y"
struct _tuple29*_tmp674=Cyc_yyget_YY59(&(yyyvsp[3]).v);struct _tuple29*_stmttmp30=_tmp674;struct _tuple29*_tmp675=_stmttmp30;struct Cyc_List_List*_tmp678;struct Cyc_List_List*_tmp677;struct Cyc_List_List*_tmp676;_LL4BA: _tmp676=_tmp675->f1;_tmp677=_tmp675->f2;_tmp678=_tmp675->f3;_LL4BB: {struct Cyc_List_List*outlist=_tmp676;struct Cyc_List_List*inlist=_tmp677;struct Cyc_List_List*clobbers=_tmp678;
yyval=Cyc_YY58((void*)({struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*_tmp679=_cycalloc(sizeof(*_tmp679));_tmp679->tag=40U,({int _tmpC09=Cyc_yyget_YY31(&(yyyvsp[0]).v);_tmp679->f1=_tmpC09;}),({struct _fat_ptr _tmpC08=Cyc_yyget_String_tok(&(yyyvsp[2]).v);_tmp679->f2=_tmpC08;}),_tmp679->f3=outlist,_tmp679->f4=inlist,_tmp679->f5=clobbers;_tmp679;}));
goto _LL0;}}case 497U: _LL3DD: _LL3DE:
# 3067 "parse.y"
 yyval=Cyc_YY31(0);
goto _LL0;case 498U: _LL3DF: _LL3E0:
# 3068 "parse.y"
 yyval=Cyc_YY31(1);
goto _LL0;case 499U: _LL3E1: _LL3E2:
# 3072 "parse.y"
 yyval=Cyc_YY59(({struct _tuple29*_tmp67A=_cycalloc(sizeof(*_tmp67A));_tmp67A->f1=0,_tmp67A->f2=0,_tmp67A->f3=0;_tmp67A;}));
goto _LL0;case 500U: _LL3E3: _LL3E4: {
# 3074 "parse.y"
struct _tuple30*_tmp67B=Cyc_yyget_YY60(&(yyyvsp[1]).v);struct _tuple30*_stmttmp31=_tmp67B;struct _tuple30*_tmp67C=_stmttmp31;struct Cyc_List_List*_tmp67E;struct Cyc_List_List*_tmp67D;_LL4BD: _tmp67D=_tmp67C->f1;_tmp67E=_tmp67C->f2;_LL4BE: {struct Cyc_List_List*inlist=_tmp67D;struct Cyc_List_List*clobbers=_tmp67E;
yyval=Cyc_YY59(({struct _tuple29*_tmp67F=_cycalloc(sizeof(*_tmp67F));_tmp67F->f1=0,_tmp67F->f2=inlist,_tmp67F->f3=clobbers;_tmp67F;}));
goto _LL0;}}case 501U: _LL3E5: _LL3E6: {
# 3077 "parse.y"
struct _tuple30*_tmp680=Cyc_yyget_YY60(&(yyyvsp[2]).v);struct _tuple30*_stmttmp32=_tmp680;struct _tuple30*_tmp681=_stmttmp32;struct Cyc_List_List*_tmp683;struct Cyc_List_List*_tmp682;_LL4C0: _tmp682=_tmp681->f1;_tmp683=_tmp681->f2;_LL4C1: {struct Cyc_List_List*inlist=_tmp682;struct Cyc_List_List*clobbers=_tmp683;
yyval=Cyc_YY59(({struct _tuple29*_tmp684=_cycalloc(sizeof(*_tmp684));({struct Cyc_List_List*_tmpC0A=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY62(&(yyyvsp[1]).v));_tmp684->f1=_tmpC0A;}),_tmp684->f2=inlist,_tmp684->f3=clobbers;_tmp684;}));
goto _LL0;}}case 502U: _LL3E7: _LL3E8:
# 3082 "parse.y"
 yyval=Cyc_YY62(({struct Cyc_List_List*_tmp685=_cycalloc(sizeof(*_tmp685));({struct _tuple31*_tmpC0B=Cyc_yyget_YY63(&(yyyvsp[0]).v);_tmp685->hd=_tmpC0B;}),_tmp685->tl=0;_tmp685;}));
goto _LL0;case 503U: _LL3E9: _LL3EA:
# 3083 "parse.y"
 yyval=Cyc_YY62(({struct Cyc_List_List*_tmp686=_cycalloc(sizeof(*_tmp686));({struct _tuple31*_tmpC0D=Cyc_yyget_YY63(&(yyyvsp[2]).v);_tmp686->hd=_tmpC0D;}),({struct Cyc_List_List*_tmpC0C=Cyc_yyget_YY62(&(yyyvsp[0]).v);_tmp686->tl=_tmpC0C;});_tmp686;}));
goto _LL0;case 504U: _LL3EB: _LL3EC:
# 3087 "parse.y"
 yyval=Cyc_YY60(({struct _tuple30*_tmp687=_cycalloc(sizeof(*_tmp687));_tmp687->f1=0,_tmp687->f2=0;_tmp687;}));
goto _LL0;case 505U: _LL3ED: _LL3EE:
# 3089 "parse.y"
 yyval=Cyc_YY60(({struct _tuple30*_tmp688=_cycalloc(sizeof(*_tmp688));_tmp688->f1=0,({struct Cyc_List_List*_tmpC0E=Cyc_yyget_YY61(&(yyyvsp[1]).v);_tmp688->f2=_tmpC0E;});_tmp688;}));
goto _LL0;case 506U: _LL3EF: _LL3F0:
# 3091 "parse.y"
 yyval=Cyc_YY60(({struct _tuple30*_tmp689=_cycalloc(sizeof(*_tmp689));({struct Cyc_List_List*_tmpC10=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY62(&(yyyvsp[1]).v));_tmp689->f1=_tmpC10;}),({struct Cyc_List_List*_tmpC0F=Cyc_yyget_YY61(&(yyyvsp[2]).v);_tmp689->f2=_tmpC0F;});_tmp689;}));
goto _LL0;case 507U: _LL3F1: _LL3F2:
# 3095 "parse.y"
 yyval=Cyc_YY62(({struct Cyc_List_List*_tmp68A=_cycalloc(sizeof(*_tmp68A));({struct _tuple31*_tmpC11=Cyc_yyget_YY63(&(yyyvsp[0]).v);_tmp68A->hd=_tmpC11;}),_tmp68A->tl=0;_tmp68A;}));
goto _LL0;case 508U: _LL3F3: _LL3F4:
# 3096 "parse.y"
 yyval=Cyc_YY62(({struct Cyc_List_List*_tmp68B=_cycalloc(sizeof(*_tmp68B));({struct _tuple31*_tmpC13=Cyc_yyget_YY63(&(yyyvsp[2]).v);_tmp68B->hd=_tmpC13;}),({struct Cyc_List_List*_tmpC12=Cyc_yyget_YY62(&(yyyvsp[0]).v);_tmp68B->tl=_tmpC12;});_tmp68B;}));
goto _LL0;case 509U: _LL3F5: _LL3F6: {
# 3101 "parse.y"
struct Cyc_Absyn_Exp*_tmp68C=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);struct Cyc_Absyn_Exp*pf_exp=_tmp68C;
yyval=Cyc_YY63(({struct _tuple31*_tmp68D=_cycalloc(sizeof(*_tmp68D));({struct _fat_ptr _tmpC15=Cyc_yyget_String_tok(&(yyyvsp[0]).v);_tmp68D->f1=_tmpC15;}),({struct Cyc_Absyn_Exp*_tmpC14=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp68D->f2=_tmpC14;});_tmp68D;}));
goto _LL0;}case 510U: _LL3F7: _LL3F8:
# 3107 "parse.y"
 yyval=Cyc_YY61(0);
goto _LL0;case 511U: _LL3F9: _LL3FA:
# 3108 "parse.y"
 yyval=Cyc_YY61(0);
goto _LL0;case 512U: _LL3FB: _LL3FC:
# 3109 "parse.y"
 yyval=Cyc_YY61(((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY61(&(yyyvsp[1]).v)));
goto _LL0;case 513U: _LL3FD: _LL3FE:
# 3113 "parse.y"
 yyval=Cyc_YY61(({struct Cyc_List_List*_tmp68F=_cycalloc(sizeof(*_tmp68F));({struct _fat_ptr*_tmpC17=({struct _fat_ptr*_tmp68E=_cycalloc(sizeof(*_tmp68E));({struct _fat_ptr _tmpC16=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp68E=_tmpC16;});_tmp68E;});_tmp68F->hd=_tmpC17;}),_tmp68F->tl=0;_tmp68F;}));
goto _LL0;case 514U: _LL3FF: _LL400:
# 3114 "parse.y"
 yyval=Cyc_YY61(({struct Cyc_List_List*_tmp691=_cycalloc(sizeof(*_tmp691));({struct _fat_ptr*_tmpC1A=({struct _fat_ptr*_tmp690=_cycalloc(sizeof(*_tmp690));({struct _fat_ptr _tmpC19=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp690=_tmpC19;});_tmp690;});_tmp691->hd=_tmpC1A;}),({struct Cyc_List_List*_tmpC18=Cyc_yyget_YY61(&(yyyvsp[0]).v);_tmp691->tl=_tmpC18;});_tmp691;}));
goto _LL0;case 515U: _LL401: _LL402:
# 3118 "parse.y"
 yyval=Cyc_YY6(Cyc_Absyn_Bitnot);
goto _LL0;case 516U: _LL403: _LL404:
# 3119 "parse.y"
 yyval=Cyc_YY6(Cyc_Absyn_Not);
goto _LL0;case 517U: _LL405: _LL406:
# 3120 "parse.y"
 yyval=Cyc_YY6(Cyc_Absyn_Minus);
goto _LL0;case 518U: _LL407: _LL408:
# 3125 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 519U: _LL409: _LL40A:
# 3127 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC1C=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_Absyn_Exp*_tmpC1B=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);Cyc_Absyn_subscript_exp(_tmpC1C,_tmpC1B,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 520U: _LL40B: _LL40C:
# 3129 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC1D=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);Cyc_Absyn_unknowncall_exp(_tmpC1D,0,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 521U: _LL40D: _LL40E:
# 3131 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC1F=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_List_List*_tmpC1E=Cyc_yyget_YY4(&(yyyvsp[2]).v);Cyc_Absyn_unknowncall_exp(_tmpC1F,_tmpC1E,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 522U: _LL40F: _LL410:
# 3133 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC22=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct _fat_ptr*_tmpC21=({struct _fat_ptr*_tmp692=_cycalloc(sizeof(*_tmp692));({struct _fat_ptr _tmpC20=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp692=_tmpC20;});_tmp692;});Cyc_Absyn_aggrmember_exp(_tmpC22,_tmpC21,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 523U: _LL411: _LL412:
# 3135 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC25=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct _fat_ptr*_tmpC24=({struct _fat_ptr*_tmp693=_cycalloc(sizeof(*_tmp693));({struct _fat_ptr _tmpC23=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp693=_tmpC23;});_tmp693;});Cyc_Absyn_aggrarrow_exp(_tmpC25,_tmpC24,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 524U: _LL413: _LL414:
# 3137 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC26=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);Cyc_Absyn_increment_exp(_tmpC26,Cyc_Absyn_PostInc,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 525U: _LL415: _LL416:
# 3139 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC27=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);Cyc_Absyn_increment_exp(_tmpC27,Cyc_Absyn_PostDec,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 526U: _LL417: _LL418:
# 3141 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpC29=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmp694=_cycalloc(sizeof(*_tmp694));_tmp694->tag=25U,({struct _tuple8*_tmpC28=Cyc_yyget_YY37(&(yyyvsp[1]).v);_tmp694->f1=_tmpC28;}),_tmp694->f2=0;_tmp694;});Cyc_Absyn_new_exp(_tmpC29,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 527U: _LL419: _LL41A:
# 3143 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpC2C=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmp695=_cycalloc(sizeof(*_tmp695));_tmp695->tag=25U,({struct _tuple8*_tmpC2B=Cyc_yyget_YY37(&(yyyvsp[1]).v);_tmp695->f1=_tmpC2B;}),({struct Cyc_List_List*_tmpC2A=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY5(&(yyyvsp[4]).v));_tmp695->f2=_tmpC2A;});_tmp695;});Cyc_Absyn_new_exp(_tmpC2C,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 528U: _LL41B: _LL41C:
# 3145 "parse.y"
 yyval=Cyc_Exp_tok(({void*_tmpC2F=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmp696=_cycalloc(sizeof(*_tmp696));_tmp696->tag=25U,({struct _tuple8*_tmpC2E=Cyc_yyget_YY37(&(yyyvsp[1]).v);_tmp696->f1=_tmpC2E;}),({struct Cyc_List_List*_tmpC2D=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY5(&(yyyvsp[4]).v));_tmp696->f2=_tmpC2D;});_tmp696;});Cyc_Absyn_new_exp(_tmpC2F,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 529U: _LL41D: _LL41E:
# 3150 "parse.y"
 yyval=Cyc_YY3(({struct Cyc_List_List*_tmp699=_cycalloc(sizeof(*_tmp699));({void*_tmpC32=(void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmp698=_cycalloc(sizeof(*_tmp698));_tmp698->tag=0U,({struct _fat_ptr*_tmpC31=({struct _fat_ptr*_tmp697=_cycalloc(sizeof(*_tmp697));({struct _fat_ptr _tmpC30=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp697=_tmpC30;});_tmp697;});_tmp698->f1=_tmpC31;});_tmp698;});_tmp699->hd=_tmpC32;}),_tmp699->tl=0;_tmp699;}));
goto _LL0;case 530U: _LL41F: _LL420:
# 3153
 yyval=Cyc_YY3(({struct Cyc_List_List*_tmp69B=_cycalloc(sizeof(*_tmp69B));({void*_tmpC35=(void*)({struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*_tmp69A=_cycalloc(sizeof(*_tmp69A));_tmp69A->tag=1U,({unsigned _tmpC34=({unsigned _tmpC33=(unsigned)((yyyvsp[0]).l).first_line;Cyc_Parse_cnst2uint(_tmpC33,Cyc_yyget_Int_tok(&(yyyvsp[0]).v));});_tmp69A->f1=_tmpC34;});_tmp69A;});_tmp69B->hd=_tmpC35;}),_tmp69B->tl=0;_tmp69B;}));
goto _LL0;case 531U: _LL421: _LL422:
# 3155 "parse.y"
 yyval=Cyc_YY3(({struct Cyc_List_List*_tmp69E=_cycalloc(sizeof(*_tmp69E));({void*_tmpC39=(void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmp69D=_cycalloc(sizeof(*_tmp69D));_tmp69D->tag=0U,({struct _fat_ptr*_tmpC38=({struct _fat_ptr*_tmp69C=_cycalloc(sizeof(*_tmp69C));({struct _fat_ptr _tmpC37=Cyc_yyget_String_tok(&(yyyvsp[2]).v);*_tmp69C=_tmpC37;});_tmp69C;});_tmp69D->f1=_tmpC38;});_tmp69D;});_tmp69E->hd=_tmpC39;}),({struct Cyc_List_List*_tmpC36=Cyc_yyget_YY3(&(yyyvsp[0]).v);_tmp69E->tl=_tmpC36;});_tmp69E;}));
goto _LL0;case 532U: _LL423: _LL424:
# 3158
 yyval=Cyc_YY3(({struct Cyc_List_List*_tmp6A0=_cycalloc(sizeof(*_tmp6A0));({void*_tmpC3D=(void*)({struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*_tmp69F=_cycalloc(sizeof(*_tmp69F));_tmp69F->tag=1U,({unsigned _tmpC3C=({unsigned _tmpC3B=(unsigned)((yyyvsp[2]).l).first_line;Cyc_Parse_cnst2uint(_tmpC3B,Cyc_yyget_Int_tok(&(yyyvsp[2]).v));});_tmp69F->f1=_tmpC3C;});_tmp69F;});_tmp6A0->hd=_tmpC3D;}),({struct Cyc_List_List*_tmpC3A=Cyc_yyget_YY3(&(yyyvsp[0]).v);_tmp6A0->tl=_tmpC3A;});_tmp6A0;}));
goto _LL0;case 533U: _LL425: _LL426:
# 3164 "parse.y"
 yyval=Cyc_Exp_tok(({struct _tuple0*_tmpC3E=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);Cyc_Absyn_unknownid_exp(_tmpC3E,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 534U: _LL427: _LL428:
# 3166 "parse.y"
 yyval=Cyc_Exp_tok(({struct _fat_ptr _tmpC3F=Cyc_yyget_String_tok(&(yyyvsp[2]).v);Cyc_Absyn_pragma_exp(_tmpC3F,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 535U: _LL429: _LL42A:
# 3168 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 536U: _LL42B: _LL42C:
# 3170 "parse.y"
 yyval=Cyc_Exp_tok(({struct _fat_ptr _tmpC40=Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_Absyn_string_exp(_tmpC40,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 537U: _LL42D: _LL42E:
# 3172 "parse.y"
 yyval=Cyc_Exp_tok(({struct _fat_ptr _tmpC41=Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_Absyn_wstring_exp(_tmpC41,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 538U: _LL42F: _LL430:
# 3174 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 539U: _LL431: _LL432:
# 3179 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC42=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);Cyc_Absyn_noinstantiate_exp(_tmpC42,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 540U: _LL433: _LL434:
# 3181 "parse.y"
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Exp*_tmpC44=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);struct Cyc_List_List*_tmpC43=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY40(&(yyyvsp[3]).v));Cyc_Absyn_instantiate_exp(_tmpC44,_tmpC43,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 541U: _LL435: _LL436:
# 3184
 yyval=Cyc_Exp_tok(({struct Cyc_List_List*_tmpC45=Cyc_yyget_YY4(&(yyyvsp[2]).v);Cyc_Absyn_tuple_exp(_tmpC45,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 542U: _LL437: _LL438:
# 3187
 yyval=Cyc_Exp_tok(({void*_tmpC49=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmp6A1=_cycalloc(sizeof(*_tmp6A1));_tmp6A1->tag=29U,({struct _tuple0*_tmpC48=Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);_tmp6A1->f1=_tmpC48;}),({struct Cyc_List_List*_tmpC47=Cyc_yyget_YY40(&(yyyvsp[2]).v);_tmp6A1->f2=_tmpC47;}),({struct Cyc_List_List*_tmpC46=((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY5(&(yyyvsp[3]).v));_tmp6A1->f3=_tmpC46;}),_tmp6A1->f4=0;_tmp6A1;});Cyc_Absyn_new_exp(_tmpC49,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 543U: _LL439: _LL43A:
# 3190
 yyval=Cyc_Exp_tok(({struct Cyc_Absyn_Stmt*_tmpC4A=Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);Cyc_Absyn_stmt_exp(_tmpC4A,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 544U: _LL43B: _LL43C:
# 3194 "parse.y"
 yyval=Cyc_YY4(((struct Cyc_List_List*(*)(struct Cyc_List_List*x))Cyc_List_imp_rev)(Cyc_yyget_YY4(&(yyyvsp[0]).v)));
goto _LL0;case 545U: _LL43D: _LL43E:
# 3200 "parse.y"
 yyval=Cyc_YY4(({struct Cyc_List_List*_tmp6A2=_cycalloc(sizeof(*_tmp6A2));({struct Cyc_Absyn_Exp*_tmpC4B=Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);_tmp6A2->hd=_tmpC4B;}),_tmp6A2->tl=0;_tmp6A2;}));
goto _LL0;case 546U: _LL43F: _LL440:
# 3202 "parse.y"
 yyval=Cyc_YY4(({struct Cyc_List_List*_tmp6A3=_cycalloc(sizeof(*_tmp6A3));({struct Cyc_Absyn_Exp*_tmpC4D=Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);_tmp6A3->hd=_tmpC4D;}),({struct Cyc_List_List*_tmpC4C=Cyc_yyget_YY4(&(yyyvsp[0]).v);_tmp6A3->tl=_tmpC4C;});_tmp6A3;}));
goto _LL0;case 547U: _LL441: _LL442:
# 3208 "parse.y"
 yyval=Cyc_Exp_tok(({union Cyc_Absyn_Cnst _tmpC4E=Cyc_yyget_Int_tok(&(yyyvsp[0]).v);Cyc_Absyn_const_exp(_tmpC4E,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 548U: _LL443: _LL444:
# 3209 "parse.y"
 yyval=Cyc_Exp_tok(({char _tmpC4F=Cyc_yyget_Char_tok(&(yyyvsp[0]).v);Cyc_Absyn_char_exp(_tmpC4F,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 549U: _LL445: _LL446:
# 3210 "parse.y"
 yyval=Cyc_Exp_tok(({struct _fat_ptr _tmpC50=Cyc_yyget_String_tok(&(yyyvsp[0]).v);Cyc_Absyn_wchar_exp(_tmpC50,(unsigned)((yyyvsp[0]).l).first_line);}));
goto _LL0;case 550U: _LL447: _LL448: {
# 3212 "parse.y"
struct _fat_ptr _tmp6A4=Cyc_yyget_String_tok(&(yyyvsp[0]).v);struct _fat_ptr f=_tmp6A4;
int l=(int)Cyc_strlen((struct _fat_ptr)f);
int i=1;
if(l > 0){
char c=*((const char*)_check_fat_subscript(f,sizeof(char),l - 1));
if((int)c == (int)'f' ||(int)c == (int)'F')i=0;else{
if((int)c == (int)'l' ||(int)c == (int)'L')i=2;}}
# 3220
yyval=Cyc_Exp_tok(Cyc_Absyn_float_exp(f,i,(unsigned)((yyyvsp[0]).l).first_line));
# 3222
goto _LL0;}case 551U: _LL449: _LL44A:
# 3223 "parse.y"
 yyval=Cyc_Exp_tok(Cyc_Absyn_null_exp((unsigned)((yyyvsp[0]).l).first_line));
goto _LL0;case 552U: _LL44B: _LL44C:
# 3227 "parse.y"
 yyval=Cyc_QualId_tok(({struct _tuple0*_tmp6A6=_cycalloc(sizeof(*_tmp6A6));({union Cyc_Absyn_Nmspace _tmpC53=Cyc_Absyn_Rel_n(0);_tmp6A6->f1=_tmpC53;}),({struct _fat_ptr*_tmpC52=({struct _fat_ptr*_tmp6A5=_cycalloc(sizeof(*_tmp6A5));({struct _fat_ptr _tmpC51=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp6A5=_tmpC51;});_tmp6A5;});_tmp6A6->f2=_tmpC52;});_tmp6A6;}));
goto _LL0;case 553U: _LL44D: _LL44E:
# 3228 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 554U: _LL44F: _LL450:
# 3231
 yyval=Cyc_QualId_tok(({struct _tuple0*_tmp6A8=_cycalloc(sizeof(*_tmp6A8));({union Cyc_Absyn_Nmspace _tmpC56=Cyc_Absyn_Rel_n(0);_tmp6A8->f1=_tmpC56;}),({struct _fat_ptr*_tmpC55=({struct _fat_ptr*_tmp6A7=_cycalloc(sizeof(*_tmp6A7));({struct _fat_ptr _tmpC54=Cyc_yyget_String_tok(&(yyyvsp[0]).v);*_tmp6A7=_tmpC54;});_tmp6A7;});_tmp6A8->f2=_tmpC55;});_tmp6A8;}));
goto _LL0;case 555U: _LL451: _LL452:
# 3232 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 556U: _LL453: _LL454:
# 3237 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 557U: _LL455: _LL456:
# 3238 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 558U: _LL457: _LL458:
# 3241
 yyval=(yyyvsp[0]).v;
goto _LL0;case 559U: _LL459: _LL45A:
# 3242 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 560U: _LL45B: _LL45C:
# 3247 "parse.y"
 goto _LL0;case 561U: _LL45D: _LL45E:
# 3247 "parse.y"
 yylex_buf->lex_curr_pos -=1;
goto _LL0;default: _LL45F: _LL460:
# 3251
 goto _LL0;}_LL0:;}
# 375 "cycbison.simple"
yyvsp_offset -=yylen;
yyssp_offset -=yylen;
# 389 "cycbison.simple"
(*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),++ yyvsp_offset))).v=yyval;
# 392
if(yylen == 0){
struct Cyc_Yystacktype*p=(struct Cyc_Yystacktype*)_check_null(_untag_fat_ptr(_fat_ptr_plus(yyvs,sizeof(struct Cyc_Yystacktype),yyvsp_offset - 1),sizeof(struct Cyc_Yystacktype),2U));
((p[1]).l).first_line=yylloc.first_line;
((p[1]).l).first_column=yylloc.first_column;
((p[1]).l).last_line=((p[0]).l).last_line;
((p[1]).l).last_column=((p[0]).l).last_column;}else{
# 399
({int _tmpC57=((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),(yyvsp_offset + yylen)- 1))).l).last_line;((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),yyvsp_offset))).l).last_line=_tmpC57;});
((((struct Cyc_Yystacktype*)yyvs.curr)[yyvsp_offset]).l).last_column=((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),(yyvsp_offset + yylen)- 1))).l).last_column;}
# 409
yyn=(int)Cyc_yyr1[yyn];
# 411
yystate=({int _tmpC58=(int)*((short*)_check_known_subscript_notnull(Cyc_yypgoto,162U,sizeof(short),yyn - 152));_tmpC58 + (int)*((short*)_check_fat_subscript(yyss,sizeof(short),yyssp_offset));});
if((yystate >= 0 && yystate <= 7949)&&(int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,7950U,sizeof(short),yystate))== (int)((short*)yyss.curr)[yyssp_offset])
yystate=(int)Cyc_yytable[yystate];else{
# 415
yystate=(int)*((short*)_check_known_subscript_notnull(Cyc_yydefgoto,162U,sizeof(short),yyn - 152));}
# 417
goto yynewstate;
# 419
yyerrlab:
# 421
 if(yyerrstatus == 0){
# 424
++ yynerrs;
# 427
yyn=(int)Cyc_yypact[yystate];
# 429
if(yyn > - 32768 && yyn < 7949){
# 431
int sze=0;
struct _fat_ptr msg;
int x;int count;
# 435
count=0;
# 437
for(x=yyn < 0?- yyn: 0;(unsigned)x < 314U / sizeof(char*);++ x){
# 439
if((int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,7950U,sizeof(short),x + yyn))== x)
({unsigned long _tmpC59=Cyc_strlen((struct _fat_ptr)*((struct _fat_ptr*)_check_known_subscript_notnull(Cyc_yytname,314U,sizeof(struct _fat_ptr),x)))+ (unsigned long)15;sze +=_tmpC59;}),count ++;}
msg=({unsigned _tmp6AA=(unsigned)(sze + 15)+ 1U;char*_tmp6A9=({struct _RegionHandle*_tmpC5A=yyregion;_region_malloc(_tmpC5A,_check_times(_tmp6AA,sizeof(char)));});({{unsigned _tmp770=(unsigned)(sze + 15);unsigned i;for(i=0;i < _tmp770;++ i){_tmp6A9[i]='\000';}_tmp6A9[_tmp770]=0;}0;});_tag_fat(_tmp6A9,sizeof(char),_tmp6AA);});
({struct _fat_ptr _tmpC5B=msg;Cyc_strcpy(_tmpC5B,({const char*_tmp6AB="parse error";_tag_fat(_tmp6AB,sizeof(char),12U);}));});
# 444
if(count < 5){
# 446
count=0;
for(x=yyn < 0?- yyn: 0;(unsigned)x < 314U / sizeof(char*);++ x){
# 449
if((int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,7950U,sizeof(short),x + yyn))== x){
# 451
({struct _fat_ptr _tmpC5C=msg;Cyc_strcat(_tmpC5C,(struct _fat_ptr)(count == 0?(struct _fat_ptr)({const char*_tmp6AC=", expecting `";_tag_fat(_tmp6AC,sizeof(char),14U);}):(struct _fat_ptr)({const char*_tmp6AD=" or `";_tag_fat(_tmp6AD,sizeof(char),6U);})));});
# 454
Cyc_strcat(msg,(struct _fat_ptr)*((struct _fat_ptr*)_check_known_subscript_notnull(Cyc_yytname,314U,sizeof(struct _fat_ptr),x)));
({struct _fat_ptr _tmpC5D=msg;Cyc_strcat(_tmpC5D,({const char*_tmp6AE="'";_tag_fat(_tmp6AE,sizeof(char),2U);}));});
++ count;}}}
# 459
Cyc_yyerror((struct _fat_ptr)msg,yystate,yychar);}else{
# 463
({struct _fat_ptr _tmpC5F=({const char*_tmp6AF="parse error";_tag_fat(_tmp6AF,sizeof(char),12U);});int _tmpC5E=yystate;Cyc_yyerror(_tmpC5F,_tmpC5E,yychar);});}}
# 465
goto yyerrlab1;
# 467
yyerrlab1:
# 469
 if(yyerrstatus == 3){
# 474
if(yychar == 0){
int _tmp6B0=1;_npop_handler(0U);return _tmp6B0;}
# 483
yychar=-2;}
# 489
yyerrstatus=3;
# 491
goto yyerrhandle;
# 493
yyerrdefault:
# 503 "cycbison.simple"
 yyerrpop:
# 505
 if(yyssp_offset == 0){int _tmp6B1=1;_npop_handler(0U);return _tmp6B1;}
-- yyvsp_offset;
yystate=(int)*((short*)_check_fat_subscript(yyss,sizeof(short),-- yyssp_offset));
# 521 "cycbison.simple"
yyerrhandle:
 yyn=(int)*((short*)_check_known_subscript_notnull(Cyc_yypact,1137U,sizeof(short),yystate));
if(yyn == -32768)goto yyerrdefault;
# 525
yyn +=1;
if((yyn < 0 || yyn > 7949)||(int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,7950U,sizeof(short),yyn))!= 1)goto yyerrdefault;
# 528
yyn=(int)Cyc_yytable[yyn];
if(yyn < 0){
# 531
if(yyn == -32768)goto yyerrpop;
yyn=- yyn;
goto yyreduce;}else{
# 535
if(yyn == 0)goto yyerrpop;}
# 537
if(yyn == 1136){
int _tmp6B2=0;_npop_handler(0U);return _tmp6B2;}
# 546
({struct Cyc_Yystacktype _tmpC60=({struct Cyc_Yystacktype _tmp771;_tmp771.v=yylval,_tmp771.l=yylloc;_tmp771;});*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),++ yyvsp_offset))=_tmpC60;});
# 551
goto yynewstate;}
# 149 "cycbison.simple"
;_pop_region();}
# 3250 "parse.y"
void Cyc_yyprint(int i,union Cyc_YYSTYPE v){
union Cyc_YYSTYPE _tmp6B6=v;struct Cyc_Absyn_Stmt*_tmp6B7;struct Cyc_Absyn_Exp*_tmp6B8;struct _tuple0*_tmp6B9;struct _fat_ptr _tmp6BA;char _tmp6BB;union Cyc_Absyn_Cnst _tmp6BC;switch((_tmp6B6.Stmt_tok).tag){case 1U: _LL1: _tmp6BC=(_tmp6B6.Int_tok).val;_LL2: {union Cyc_Absyn_Cnst c=_tmp6BC;
({struct Cyc_String_pa_PrintArg_struct _tmp6BF=({struct Cyc_String_pa_PrintArg_struct _tmp774;_tmp774.tag=0U,({struct _fat_ptr _tmpC61=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_cnst2string(c));_tmp774.f1=_tmpC61;});_tmp774;});void*_tmp6BD[1U];_tmp6BD[0]=& _tmp6BF;({struct Cyc___cycFILE*_tmpC63=Cyc_stderr;struct _fat_ptr _tmpC62=({const char*_tmp6BE="%s";_tag_fat(_tmp6BE,sizeof(char),3U);});Cyc_fprintf(_tmpC63,_tmpC62,_tag_fat(_tmp6BD,sizeof(void*),1U));});});goto _LL0;}case 2U: _LL3: _tmp6BB=(_tmp6B6.Char_tok).val;_LL4: {char c=_tmp6BB;
({struct Cyc_Int_pa_PrintArg_struct _tmp6C2=({struct Cyc_Int_pa_PrintArg_struct _tmp775;_tmp775.tag=1U,_tmp775.f1=(unsigned long)((int)c);_tmp775;});void*_tmp6C0[1U];_tmp6C0[0]=& _tmp6C2;({struct Cyc___cycFILE*_tmpC65=Cyc_stderr;struct _fat_ptr _tmpC64=({const char*_tmp6C1="%c";_tag_fat(_tmp6C1,sizeof(char),3U);});Cyc_fprintf(_tmpC65,_tmpC64,_tag_fat(_tmp6C0,sizeof(void*),1U));});});goto _LL0;}case 3U: _LL5: _tmp6BA=(_tmp6B6.String_tok).val;_LL6: {struct _fat_ptr s=_tmp6BA;
({struct Cyc_String_pa_PrintArg_struct _tmp6C5=({struct Cyc_String_pa_PrintArg_struct _tmp776;_tmp776.tag=0U,_tmp776.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp776;});void*_tmp6C3[1U];_tmp6C3[0]=& _tmp6C5;({struct Cyc___cycFILE*_tmpC67=Cyc_stderr;struct _fat_ptr _tmpC66=({const char*_tmp6C4="\"%s\"";_tag_fat(_tmp6C4,sizeof(char),5U);});Cyc_fprintf(_tmpC67,_tmpC66,_tag_fat(_tmp6C3,sizeof(void*),1U));});});goto _LL0;}case 5U: _LL7: _tmp6B9=(_tmp6B6.QualId_tok).val;_LL8: {struct _tuple0*q=_tmp6B9;
({struct Cyc_String_pa_PrintArg_struct _tmp6C8=({struct Cyc_String_pa_PrintArg_struct _tmp777;_tmp777.tag=0U,({struct _fat_ptr _tmpC68=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_qvar2string(q));_tmp777.f1=_tmpC68;});_tmp777;});void*_tmp6C6[1U];_tmp6C6[0]=& _tmp6C8;({struct Cyc___cycFILE*_tmpC6A=Cyc_stderr;struct _fat_ptr _tmpC69=({const char*_tmp6C7="%s";_tag_fat(_tmp6C7,sizeof(char),3U);});Cyc_fprintf(_tmpC6A,_tmpC69,_tag_fat(_tmp6C6,sizeof(void*),1U));});});goto _LL0;}case 7U: _LL9: _tmp6B8=(_tmp6B6.Exp_tok).val;_LLA: {struct Cyc_Absyn_Exp*e=_tmp6B8;
({struct Cyc_String_pa_PrintArg_struct _tmp6CB=({struct Cyc_String_pa_PrintArg_struct _tmp778;_tmp778.tag=0U,({struct _fat_ptr _tmpC6B=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_exp2string(e));_tmp778.f1=_tmpC6B;});_tmp778;});void*_tmp6C9[1U];_tmp6C9[0]=& _tmp6CB;({struct Cyc___cycFILE*_tmpC6D=Cyc_stderr;struct _fat_ptr _tmpC6C=({const char*_tmp6CA="%s";_tag_fat(_tmp6CA,sizeof(char),3U);});Cyc_fprintf(_tmpC6D,_tmpC6C,_tag_fat(_tmp6C9,sizeof(void*),1U));});});goto _LL0;}case 8U: _LLB: _tmp6B7=(_tmp6B6.Stmt_tok).val;_LLC: {struct Cyc_Absyn_Stmt*s=_tmp6B7;
({struct Cyc_String_pa_PrintArg_struct _tmp6CE=({struct Cyc_String_pa_PrintArg_struct _tmp779;_tmp779.tag=0U,({struct _fat_ptr _tmpC6E=(struct _fat_ptr)((struct _fat_ptr)Cyc_Absynpp_stmt2string(s));_tmp779.f1=_tmpC6E;});_tmp779;});void*_tmp6CC[1U];_tmp6CC[0]=& _tmp6CE;({struct Cyc___cycFILE*_tmpC70=Cyc_stderr;struct _fat_ptr _tmpC6F=({const char*_tmp6CD="%s";_tag_fat(_tmp6CD,sizeof(char),3U);});Cyc_fprintf(_tmpC70,_tmpC6F,_tag_fat(_tmp6CC,sizeof(void*),1U));});});goto _LL0;}default: _LLD: _LLE:
({void*_tmp6CF=0U;({struct Cyc___cycFILE*_tmpC72=Cyc_stderr;struct _fat_ptr _tmpC71=({const char*_tmp6D0="?";_tag_fat(_tmp6D0,sizeof(char),2U);});Cyc_fprintf(_tmpC72,_tmpC71,_tag_fat(_tmp6CF,sizeof(void*),0U));});});goto _LL0;}_LL0:;}
# 3262
struct _fat_ptr Cyc_token2string(int token){
if(token <= 0)
return({const char*_tmp6D1="end-of-file";_tag_fat(_tmp6D1,sizeof(char),12U);});
if(token == 367)
return Cyc_Lex_token_string;else{
if(token == 376)
return Cyc_Absynpp_qvar2string(Cyc_Lex_token_qvar);}{
int z=token > 0 && token <= 379?(int)*((short*)_check_known_subscript_notnull(Cyc_yytranslate,380U,sizeof(short),token)): 314;
if((unsigned)z < 314U)
return Cyc_yytname[z];
return _tag_fat(0,0,0);}}
# 3276
struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f){
Cyc_Parse_parse_result=0;{
struct _RegionHandle _tmp6D2=_new_region("yyr");struct _RegionHandle*yyr=& _tmp6D2;_push_region(yyr);
({struct _RegionHandle*_tmpC73=yyr;Cyc_yyparse(_tmpC73,Cyc_Lexing_from_file(f));});{
struct Cyc_List_List*_tmp6D3=Cyc_Parse_parse_result;_npop_handler(0U);return _tmp6D3;}
# 3279
;_pop_region();}}

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
 struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};
# 157 "cycboot.h"
extern int Cyc_printf(struct _fat_ptr,struct _fat_ptr);
# 319 "cycboot.h"
extern void exit(int);
# 43 "warn.h"
void Cyc_Warn_err(unsigned,struct _fat_ptr,struct _fat_ptr);
# 154 "flags.h"
extern int Cyc_Flags_print_parser_state_and_token;
# 29 "parse.h"
struct _fat_ptr Cyc_token2string(int);struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};
# 100 "parse_tab.h"
extern struct Cyc_Yyltype Cyc_yylloc;struct Cyc_ParseErrors_ParseState{int state;int token;struct _fat_ptr msg;};static char _TmpG0[34U]="undeclared type or missing comma ";static char _TmpG1[28U]="type has not been declared ";static char _TmpG2[13U]="missing ';' ";static char _TmpG3[31U]="missing ')' on parameter list ";static char _TmpG4[29U]="expecting IDENTIFIER or '{' ";static char _TmpG5[29U]="expecting IDENTIFIER or '{' ";static char _TmpG6[19U]="expecting PATTERN ";static char _TmpG7[22U]="expecting IDENTIFIER ";static char _TmpG8[22U]="expecting IDENTIFIER ";static char _TmpG9[30U]="expecting '(' EXPRESSION ')' ";static char _TmpGA[19U]="expecting 'union' ";static char _TmpGB[22U]="expecting 'datatype' ";static char _TmpGC[36U]="expecting '((' ATTRIBUTE_LIST '))' ";static char _TmpGD[34U]="expecting '(' PARAMETER_LIST ')' ";static char _TmpGE[16U]="expecting KIND ";static char _TmpGF[16U]="expecting KIND ";static char _TmpG10[22U]="expecting ';' or '{' ";static char _TmpG11[22U]="expecting ';' or '{' ";static char _TmpG12[29U]="expecting '<' or IDENTIFIER ";static char _TmpG13[14U]="missing '};' ";static char _TmpG14[22U]="expecting IDENTIFIER ";static char _TmpG15[15U]="expecting '(' ";static char _TmpG16[22U]="expecting IDENTIFIER ";static char _TmpG17[19U]="expecting PATTERN ";static char _TmpG18[35U]="expecting IDENTIFIER or extra ',' ";static char _TmpG19[22U]="expecting EXPRESSION ";static char _TmpG1A[13U]="missing '}' ";static char _TmpG1B[13U]="missing '}' ";static char _TmpG1C[13U]="missing ';' ";static char _TmpG1D[22U]="expecting EXPRESSION ";static char _TmpG1E[23U]="expecting DECLARATION ";static char _TmpG1F[13U]="missing '}' ";static char _TmpG20[29U]="expecting ']' or EXPRESSION ";static char _TmpG21[22U]="expecting EXPRESSION ";static char _TmpG22[16U]="expecting TYPE ";static char _TmpG23[13U]="missing ')' ";static char _TmpG24[22U]="expecting EXPRESSION ";static char _TmpG25[13U]="missing ')' ";static char _TmpG26[16U]="expecting TYPE ";static char _TmpG27[22U]="expecting EXPRESSION ";static char _TmpG28[13U]="missing '}' ";static char _TmpG29[32U]="expecting TYPE_VARIABLE or '_' ";static char _TmpG2A[45U]="expecting '(' EXPRESSION ')' or '( TYPE ')' ";static char _TmpG2B[13U]="missing ')' ";static char _TmpG2C[22U]="expecting EXPRESSION ";static char _TmpG2D[37U]="expecting INITIALIZER or EXPRESSION ";static char _TmpG2E[29U]="expecting '(' EXPRESSION ') ";static char _TmpG2F[25U]="expecting EXPRESSION ') ";static char _TmpG30[12U]="missing ') ";static char _TmpG31[44U]="expecting '(' EXPRESSION ',' EXPRESSION ') ";static char _TmpG32[40U]="expecting EXPRESSION ',' EXPRESSION ') ";static char _TmpG33[29U]="expecting ',' EXPRESSION ') ";static char _TmpG34[13U]="missing ')' ";static char _TmpG35[15U]="expecting '(' ";static char _TmpG36[22U]="expecting EXPRESSION ";static char _TmpG37[15U]="expecting ',' ";static char _TmpG38[30U]="expecting 'sizeof(' TYPE ')' ";static char _TmpG39[16U]="expecting TYPE ";static char _TmpG3A[13U]="missing ')' ";static char _TmpG3B[15U]="expecting '(' ";static char _TmpG3C[22U]="expecting EXPRESSION ";static char _TmpG3D[15U]="expecting ',' ";static char _TmpG3E[22U]="expecting EXPRESSION ";static char _TmpG3F[15U]="expecting ',' ";static char _TmpG40[20U]="expecting 'sizeof' ";static char _TmpG41[15U]="expecting '(' ";static char _TmpG42[16U]="expecting TYPE ";static char _TmpG43[13U]="missing ')' ";static char _TmpG44[37U]="expecting '(' REGION_EXPRESSION ')' ";static char _TmpG45[13U]="missing ')' ";static char _TmpG46[37U]="expecting INITIALIZER or EXPRESSION ";static char _TmpG47[30U]="expecting '(' EXPRESSION ')' ";static char _TmpG48[26U]="expecting EXPRESSION ')' ";static char _TmpG49[13U]="missing ')' ";static char _TmpG4A[24U]="expecting '(' TYPE ')' ";static char _TmpG4B[20U]="expecting TYPE ')' ";static char _TmpG4C[13U]="missing ')' ";static char _TmpG4D[15U]="expecting '(' ";static char _TmpG4E[22U]="expecting EXPRESSION ";static char _TmpG4F[41U]="expecting '.' IDENTIFIER or missing ')' ";static char _TmpG50[22U]="expecting IDENTIFIER ";static char _TmpG51[46U]="expecting TYPE_LIST, EXPRESSION_LIST, or ')' ";static char _TmpG52[15U]="expecting ')' ";static char _TmpG53[24U]="missing ':' EXPRESSION ";static char _TmpG54[33U]="missing EXPRESSION or extra '?' ";static char _TmpG55[38U]="missing EXPRESSION or extra operator ";static char _TmpG56[38U]="missing EXPRESSION or extra operator ";static char _TmpG57[33U]="missing EXPRESSION or extra '|' ";static char _TmpG58[33U]="missing EXPRESSION or extra '&' ";static char _TmpG59[33U]="missing EXPRESSION or extra '^' ";static char _TmpG5A[33U]="missing EXPRESSION or extra '&' ";static char _TmpG5B[38U]="missing EXPRESSION or extra operator ";static char _TmpG5C[38U]="missing EXPRESSION or extra operator ";static char _TmpG5D[34U]="missing EXPRESSION or extra '>>' ";static char _TmpG5E[34U]="missing EXPRESSION or extra '<<' ";static char _TmpG5F[22U]="expecting EXPRESSION ";static char _TmpG60[22U]="expecting EXPRESSION ";static char _TmpG61[22U]="expecting EXPRESSION ";static char _TmpG62[31U]="extra ',' or missing argument ";static char _TmpG63[13U]="missing ')' ";static char _TmpG64[22U]="expecting IDENTIFIER ";static char _TmpG65[22U]="expecting IDENTIFIER ";static char _TmpG66[48U]="extra space not allowed in empty instantiation ";static char _TmpG67[37U]="expecting '<>' or '<' TYPE_LIST '>' ";
# 10 "parse_errors.cyc"
static struct Cyc_ParseErrors_ParseState Cyc_ParseErrors_msg_table[104U]={{149,387,{(unsigned char*)_TmpG0,(unsigned char*)_TmpG0,(unsigned char*)_TmpG0 + 34U}},{79,387,{(unsigned char*)_TmpG1,(unsigned char*)_TmpG1,(unsigned char*)_TmpG1 + 28U}},{566,- 1,{(unsigned char*)_TmpG2,(unsigned char*)_TmpG2,(unsigned char*)_TmpG2 + 13U}},{367,- 1,{(unsigned char*)_TmpG3,(unsigned char*)_TmpG3,(unsigned char*)_TmpG3 + 31U}},{75,- 1,{(unsigned char*)_TmpG4,(unsigned char*)_TmpG4,(unsigned char*)_TmpG4 + 29U}},{22,- 1,{(unsigned char*)_TmpG5,(unsigned char*)_TmpG5,(unsigned char*)_TmpG5 + 29U}},{27,- 1,{(unsigned char*)_TmpG6,(unsigned char*)_TmpG6,(unsigned char*)_TmpG6 + 19U}},{29,- 1,{(unsigned char*)_TmpG7,(unsigned char*)_TmpG7,(unsigned char*)_TmpG7 + 22U}},{30,- 1,{(unsigned char*)_TmpG8,(unsigned char*)_TmpG8,(unsigned char*)_TmpG8 + 22U}},{41,- 1,{(unsigned char*)_TmpG9,(unsigned char*)_TmpG9,(unsigned char*)_TmpG9 + 30U}},{42,- 1,{(unsigned char*)_TmpGA,(unsigned char*)_TmpGA,(unsigned char*)_TmpGA + 19U}},{43,- 1,{(unsigned char*)_TmpGB,(unsigned char*)_TmpGB,(unsigned char*)_TmpGB + 22U}},{49,- 1,{(unsigned char*)_TmpGC,(unsigned char*)_TmpGC,(unsigned char*)_TmpGC + 36U}},{53,- 1,{(unsigned char*)_TmpGD,(unsigned char*)_TmpGD,(unsigned char*)_TmpGD + 34U}},{127,- 1,{(unsigned char*)_TmpGE,(unsigned char*)_TmpGE,(unsigned char*)_TmpGE + 16U}},{136,- 1,{(unsigned char*)_TmpGF,(unsigned char*)_TmpGF,(unsigned char*)_TmpGF + 16U}},{63,- 1,{(unsigned char*)_TmpG10,(unsigned char*)_TmpG10,(unsigned char*)_TmpG10 + 22U}},{64,- 1,{(unsigned char*)_TmpG11,(unsigned char*)_TmpG11,(unsigned char*)_TmpG11 + 22U}},{35,- 1,{(unsigned char*)_TmpG12,(unsigned char*)_TmpG12,(unsigned char*)_TmpG12 + 29U}},{984,- 1,{(unsigned char*)_TmpG13,(unsigned char*)_TmpG13,(unsigned char*)_TmpG13 + 14U}},{78,- 1,{(unsigned char*)_TmpG14,(unsigned char*)_TmpG14,(unsigned char*)_TmpG14 + 22U}},{108,- 1,{(unsigned char*)_TmpG15,(unsigned char*)_TmpG15,(unsigned char*)_TmpG15 + 15U}},{105,- 1,{(unsigned char*)_TmpG16,(unsigned char*)_TmpG16,(unsigned char*)_TmpG16 + 22U}},{109,- 1,{(unsigned char*)_TmpG17,(unsigned char*)_TmpG17,(unsigned char*)_TmpG17 + 19U}},{261,- 1,{(unsigned char*)_TmpG18,(unsigned char*)_TmpG18,(unsigned char*)_TmpG18 + 35U}},{132,- 1,{(unsigned char*)_TmpG19,(unsigned char*)_TmpG19,(unsigned char*)_TmpG19 + 22U}},{144,- 1,{(unsigned char*)_TmpG1A,(unsigned char*)_TmpG1A,(unsigned char*)_TmpG1A + 13U}},{312,- 1,{(unsigned char*)_TmpG1B,(unsigned char*)_TmpG1B,(unsigned char*)_TmpG1B + 13U}},{149,- 1,{(unsigned char*)_TmpG1C,(unsigned char*)_TmpG1C,(unsigned char*)_TmpG1C + 13U}},{317,- 1,{(unsigned char*)_TmpG1D,(unsigned char*)_TmpG1D,(unsigned char*)_TmpG1D + 22U}},{315,- 1,{(unsigned char*)_TmpG1E,(unsigned char*)_TmpG1E,(unsigned char*)_TmpG1E + 23U}},{165,0,{(unsigned char*)_TmpG1F,(unsigned char*)_TmpG1F,(unsigned char*)_TmpG1F + 13U}},{169,- 1,{(unsigned char*)_TmpG20,(unsigned char*)_TmpG20,(unsigned char*)_TmpG20 + 29U}},{513,- 1,{(unsigned char*)_TmpG21,(unsigned char*)_TmpG21,(unsigned char*)_TmpG21 + 22U}},{375,- 1,{(unsigned char*)_TmpG22,(unsigned char*)_TmpG22,(unsigned char*)_TmpG22 + 16U}},{620,- 1,{(unsigned char*)_TmpG23,(unsigned char*)_TmpG23,(unsigned char*)_TmpG23 + 13U}},{373,- 1,{(unsigned char*)_TmpG24,(unsigned char*)_TmpG24,(unsigned char*)_TmpG24 + 22U}},{618,- 1,{(unsigned char*)_TmpG25,(unsigned char*)_TmpG25,(unsigned char*)_TmpG25 + 13U}},{187,- 1,{(unsigned char*)_TmpG26,(unsigned char*)_TmpG26,(unsigned char*)_TmpG26 + 16U}},{385,- 1,{(unsigned char*)_TmpG27,(unsigned char*)_TmpG27,(unsigned char*)_TmpG27 + 22U}},{193,- 1,{(unsigned char*)_TmpG28,(unsigned char*)_TmpG28,(unsigned char*)_TmpG28 + 13U}},{253,- 1,{(unsigned char*)_TmpG29,(unsigned char*)_TmpG29,(unsigned char*)_TmpG29 + 32U}},{194,- 1,{(unsigned char*)_TmpG2A,(unsigned char*)_TmpG2A,(unsigned char*)_TmpG2A + 45U}},{423,- 1,{(unsigned char*)_TmpG2B,(unsigned char*)_TmpG2B,(unsigned char*)_TmpG2B + 13U}},{197,- 1,{(unsigned char*)_TmpG2C,(unsigned char*)_TmpG2C,(unsigned char*)_TmpG2C + 22U}},{198,- 1,{(unsigned char*)_TmpG2D,(unsigned char*)_TmpG2D,(unsigned char*)_TmpG2D + 37U}},{200,- 1,{(unsigned char*)_TmpG2E,(unsigned char*)_TmpG2E,(unsigned char*)_TmpG2E + 29U}},{398,- 1,{(unsigned char*)_TmpG2F,(unsigned char*)_TmpG2F,(unsigned char*)_TmpG2F + 25U}},{640,- 1,{(unsigned char*)_TmpG30,(unsigned char*)_TmpG30,(unsigned char*)_TmpG30 + 12U}},{201,- 1,{(unsigned char*)_TmpG31,(unsigned char*)_TmpG31,(unsigned char*)_TmpG31 + 44U}},{399,- 1,{(unsigned char*)_TmpG32,(unsigned char*)_TmpG32,(unsigned char*)_TmpG32 + 40U}},{641,- 1,{(unsigned char*)_TmpG33,(unsigned char*)_TmpG33,(unsigned char*)_TmpG33 + 29U}},{814,- 1,{(unsigned char*)_TmpG34,(unsigned char*)_TmpG34,(unsigned char*)_TmpG34 + 13U}},{205,- 1,{(unsigned char*)_TmpG35,(unsigned char*)_TmpG35,(unsigned char*)_TmpG35 + 15U}},{403,- 1,{(unsigned char*)_TmpG36,(unsigned char*)_TmpG36,(unsigned char*)_TmpG36 + 22U}},{645,- 1,{(unsigned char*)_TmpG37,(unsigned char*)_TmpG37,(unsigned char*)_TmpG37 + 15U}},{818,- 1,{(unsigned char*)_TmpG38,(unsigned char*)_TmpG38,(unsigned char*)_TmpG38 + 30U}},{1045,- 1,{(unsigned char*)_TmpG39,(unsigned char*)_TmpG39,(unsigned char*)_TmpG39 + 16U}},{1188,- 1,{(unsigned char*)_TmpG3A,(unsigned char*)_TmpG3A,(unsigned char*)_TmpG3A + 13U}},{207,- 1,{(unsigned char*)_TmpG3B,(unsigned char*)_TmpG3B,(unsigned char*)_TmpG3B + 15U}},{405,- 1,{(unsigned char*)_TmpG3C,(unsigned char*)_TmpG3C,(unsigned char*)_TmpG3C + 22U}},{647,- 1,{(unsigned char*)_TmpG3D,(unsigned char*)_TmpG3D,(unsigned char*)_TmpG3D + 15U}},{820,- 1,{(unsigned char*)_TmpG3E,(unsigned char*)_TmpG3E,(unsigned char*)_TmpG3E + 22U}},{939,- 1,{(unsigned char*)_TmpG3F,(unsigned char*)_TmpG3F,(unsigned char*)_TmpG3F + 15U}},{1047,- 1,{(unsigned char*)_TmpG40,(unsigned char*)_TmpG40,(unsigned char*)_TmpG40 + 20U}},{1123,- 1,{(unsigned char*)_TmpG41,(unsigned char*)_TmpG41,(unsigned char*)_TmpG41 + 15U}},{1190,- 1,{(unsigned char*)_TmpG42,(unsigned char*)_TmpG42,(unsigned char*)_TmpG42 + 16U}},{1223,- 1,{(unsigned char*)_TmpG43,(unsigned char*)_TmpG43,(unsigned char*)_TmpG43 + 13U}},{209,- 1,{(unsigned char*)_TmpG44,(unsigned char*)_TmpG44,(unsigned char*)_TmpG44 + 37U}},{649,- 1,{(unsigned char*)_TmpG45,(unsigned char*)_TmpG45,(unsigned char*)_TmpG45 + 13U}},{822,- 1,{(unsigned char*)_TmpG46,(unsigned char*)_TmpG46,(unsigned char*)_TmpG46 + 37U}},{211,- 1,{(unsigned char*)_TmpG47,(unsigned char*)_TmpG47,(unsigned char*)_TmpG47 + 30U}},{409,- 1,{(unsigned char*)_TmpG48,(unsigned char*)_TmpG48,(unsigned char*)_TmpG48 + 26U}},{651,- 1,{(unsigned char*)_TmpG49,(unsigned char*)_TmpG49,(unsigned char*)_TmpG49 + 13U}},{213,- 1,{(unsigned char*)_TmpG4A,(unsigned char*)_TmpG4A,(unsigned char*)_TmpG4A + 24U}},{411,- 1,{(unsigned char*)_TmpG4B,(unsigned char*)_TmpG4B,(unsigned char*)_TmpG4B + 20U}},{653,- 1,{(unsigned char*)_TmpG4C,(unsigned char*)_TmpG4C,(unsigned char*)_TmpG4C + 13U}},{214,- 1,{(unsigned char*)_TmpG4D,(unsigned char*)_TmpG4D,(unsigned char*)_TmpG4D + 15U}},{412,- 1,{(unsigned char*)_TmpG4E,(unsigned char*)_TmpG4E,(unsigned char*)_TmpG4E + 22U}},{654,- 1,{(unsigned char*)_TmpG4F,(unsigned char*)_TmpG4F,(unsigned char*)_TmpG4F + 41U}},{828,- 1,{(unsigned char*)_TmpG50,(unsigned char*)_TmpG50,(unsigned char*)_TmpG50 + 22U}},{592,- 1,{(unsigned char*)_TmpG51,(unsigned char*)_TmpG51,(unsigned char*)_TmpG51 + 46U}},{662,- 1,{(unsigned char*)_TmpG52,(unsigned char*)_TmpG52,(unsigned char*)_TmpG52 + 15U}},{666,- 1,{(unsigned char*)_TmpG53,(unsigned char*)_TmpG53,(unsigned char*)_TmpG53 + 24U}},{429,- 1,{(unsigned char*)_TmpG54,(unsigned char*)_TmpG54,(unsigned char*)_TmpG54 + 33U}},{446,- 1,{(unsigned char*)_TmpG55,(unsigned char*)_TmpG55,(unsigned char*)_TmpG55 + 38U}},{450,- 1,{(unsigned char*)_TmpG56,(unsigned char*)_TmpG56,(unsigned char*)_TmpG56 + 38U}},{431,- 1,{(unsigned char*)_TmpG57,(unsigned char*)_TmpG57,(unsigned char*)_TmpG57 + 33U}},{433,- 1,{(unsigned char*)_TmpG58,(unsigned char*)_TmpG58,(unsigned char*)_TmpG58 + 33U}},{432,- 1,{(unsigned char*)_TmpG59,(unsigned char*)_TmpG59,(unsigned char*)_TmpG59 + 33U}},{226,- 1,{(unsigned char*)_TmpG5A,(unsigned char*)_TmpG5A,(unsigned char*)_TmpG5A + 33U}},{436,- 1,{(unsigned char*)_TmpG5B,(unsigned char*)_TmpG5B,(unsigned char*)_TmpG5B + 38U}},{441,- 1,{(unsigned char*)_TmpG5C,(unsigned char*)_TmpG5C,(unsigned char*)_TmpG5C + 38U}},{443,- 1,{(unsigned char*)_TmpG5D,(unsigned char*)_TmpG5D,(unsigned char*)_TmpG5D + 34U}},{442,- 1,{(unsigned char*)_TmpG5E,(unsigned char*)_TmpG5E,(unsigned char*)_TmpG5E + 34U}},{463,- 1,{(unsigned char*)_TmpG5F,(unsigned char*)_TmpG5F,(unsigned char*)_TmpG5F + 22U}},{217,- 1,{(unsigned char*)_TmpG60,(unsigned char*)_TmpG60,(unsigned char*)_TmpG60 + 22U}},{470,- 1,{(unsigned char*)_TmpG61,(unsigned char*)_TmpG61,(unsigned char*)_TmpG61 + 22U}},{836,41,{(unsigned char*)_TmpG62,(unsigned char*)_TmpG62,(unsigned char*)_TmpG62 + 31U}},{681,59,{(unsigned char*)_TmpG63,(unsigned char*)_TmpG63,(unsigned char*)_TmpG63 + 13U}},{469,- 1,{(unsigned char*)_TmpG64,(unsigned char*)_TmpG64,(unsigned char*)_TmpG64 + 22U}},{465,- 1,{(unsigned char*)_TmpG65,(unsigned char*)_TmpG65,(unsigned char*)_TmpG65 + 22U}},{684,62,{(unsigned char*)_TmpG66,(unsigned char*)_TmpG66,(unsigned char*)_TmpG66 + 48U}},{472,- 1,{(unsigned char*)_TmpG67,(unsigned char*)_TmpG67,(unsigned char*)_TmpG67 + 37U}}};
# 18 "parse_errors.cyc"
void Cyc_yyerror(struct _fat_ptr s,int state,int token){int _T0;struct Cyc_Int_pa_PrintArg_struct _T1;int _T2;struct Cyc_Int_pa_PrintArg_struct _T3;int _T4;struct _fat_ptr _T5;struct _fat_ptr _T6;struct Cyc_ParseErrors_ParseState*_T7;unsigned _T8;int _T9;struct Cyc_ParseErrors_ParseState _TA;int _TB;int _TC;struct Cyc_ParseErrors_ParseState*_TD;unsigned _TE;int _TF;struct Cyc_ParseErrors_ParseState _T10;int _T11;int _T12;struct Cyc_ParseErrors_ParseState*_T13;unsigned _T14;int _T15;struct Cyc_ParseErrors_ParseState _T16;struct Cyc_ParseErrors_ParseState*_T17;unsigned _T18;int _T19;struct Cyc_ParseErrors_ParseState _T1A;int _T1B;int _T1C;struct Cyc_ParseErrors_ParseState*_T1D;unsigned _T1E;int _T1F;struct Cyc_ParseErrors_ParseState _T20;struct _fat_ptr _T21;unsigned char*_T22;char*_T23;struct Cyc_String_pa_PrintArg_struct _T24;struct Cyc_String_pa_PrintArg_struct _T25;struct Cyc_Yyltype _T26;int _T27;unsigned _T28;struct _fat_ptr _T29;struct _fat_ptr _T2A;struct Cyc_String_pa_PrintArg_struct _T2B;struct Cyc_Yyltype _T2C;int _T2D;unsigned _T2E;struct _fat_ptr _T2F;struct _fat_ptr _T30;_T0=Cyc_Flags_print_parser_state_and_token;
if(!_T0)goto _TL0;{struct Cyc_Int_pa_PrintArg_struct _T31;_T31.tag=1;_T2=state;
_T31.f1=(unsigned long)_T2;_T1=_T31;}{struct Cyc_Int_pa_PrintArg_struct _T31=_T1;{struct Cyc_Int_pa_PrintArg_struct _T32;_T32.tag=1;_T4=token;_T32.f1=(unsigned long)_T4;_T3=_T32;}{struct Cyc_Int_pa_PrintArg_struct _T32=_T3;void*_T33[2];_T33[0]=& _T31;_T33[1]=& _T32;_T5=_tag_fat("parse error: state [%d], token [%d]\n",sizeof(char),37U);_T6=_tag_fat(_T33,sizeof(void*),2);Cyc_printf(_T5,_T6);}}
exit(1);goto _TL1;_TL0: _TL1:{
# 26
unsigned i=0U;_TL5: if(i < 104U)goto _TL3;else{goto _TL4;}
_TL3: _T7=Cyc_ParseErrors_msg_table;_T8=i;_T9=(int)_T8;_TA=_T7[_T9];_TB=_TA.state;_TC=state;if(_TB!=_TC)goto _TL6;_TD=Cyc_ParseErrors_msg_table;_TE=i;_TF=(int)_TE;_T10=_TD[_TF];_T11=_T10.token;_T12=token;if(_T11!=_T12)goto _TL6;_T13=Cyc_ParseErrors_msg_table;_T14=i;_T15=(int)_T14;_T16=_T13[_T15];
# 29
s=_T16.msg;goto _TL4;
# 31
_TL6: _T17=Cyc_ParseErrors_msg_table;_T18=i;_T19=(int)_T18;_T1A=_T17[_T19];_T1B=_T1A.state;_T1C=state;if(_T1B!=_T1C)goto _TL8;_T1D=Cyc_ParseErrors_msg_table;_T1E=i;_T1F=(int)_T1E;_T20=_T1D[_T1F];
s=_T20.msg;goto _TL9;_TL8: _TL9:
# 26
 i=i + 1;goto _TL5;_TL4:;}{
# 34
struct _fat_ptr ts=Cyc_token2string(token);_T21=ts;_T22=_T21.curr;_T23=(char*)_T22;
if(_T23==0)goto _TLA;{struct Cyc_String_pa_PrintArg_struct _T31;_T31.tag=0;
_T31.f1=s;_T24=_T31;}{struct Cyc_String_pa_PrintArg_struct _T31=_T24;{struct Cyc_String_pa_PrintArg_struct _T32;_T32.tag=0;_T32.f1=Cyc_token2string(token);_T25=_T32;}{struct Cyc_String_pa_PrintArg_struct _T32=_T25;void*_T33[2];_T33[0]=& _T31;_T33[1]=& _T32;_T26=Cyc_yylloc;_T27=_T26.first_line;_T28=(unsigned)_T27;_T29=_tag_fat("%s (found %s instead)",sizeof(char),22U);_T2A=_tag_fat(_T33,sizeof(void*),2);Cyc_Warn_err(_T28,_T29,_T2A);}}goto _TLB;
# 38
_TLA:{struct Cyc_String_pa_PrintArg_struct _T31;_T31.tag=0;_T31.f1=s;_T2B=_T31;}{struct Cyc_String_pa_PrintArg_struct _T31=_T2B;void*_T32[1];_T32[0]=& _T31;_T2C=Cyc_yylloc;_T2D=_T2C.first_line;_T2E=(unsigned)_T2D;_T2F=_tag_fat("%s ",sizeof(char),4U);_T30=_tag_fat(_T32,sizeof(void*),1);Cyc_Warn_err(_T2E,_T2F,_T30);}_TLB:;}}

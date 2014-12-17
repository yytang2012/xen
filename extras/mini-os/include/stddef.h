#ifndef __STDDEF_H__
#define __STDDEF_H__

#define NULL ((void *)0)
typedef long unsigned int size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WCHAR_TYPE__ wchar_t;
#define offsetof(st, m) __builtin_offsetof(st, m)

#endif /* !__STDDEF_H__ */

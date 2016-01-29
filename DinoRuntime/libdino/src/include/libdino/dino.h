#include <string.h>
#include <stdarg.h>

#include <libmementos/mementos.h>

#define DINO_FIRST_RECOVERY  0xef7a 
#define DINO_SECOND_RECOVERY 0xef78 

void __dino_task_boundary(unsigned);
void __dino_unset_recovery_bit();
unsigned int __dino_recovery_bit_set();

/* Macros for manual versioning */

/*Primitive types form*/
#define DINO_VERSION_VAL(type,var,label) type DINO_VERSION_ ## label = var;

/*General form*/
#define DINO_VERSION_PTR(var, type) \
    type DINO_VERSION_##var; \
    memcpy((void*)DINO_VERSION_##var, (void*)var, sizeof(type)); \

#define DINO_REVERT_BEGIN()     if( __dino_recovery_bit_set() ){
#define DINO_REVERT_VAL(nm,label)   nm = DINO_VERSION_ ## label
#define DINO_REVERT_PTR(type,nm)    memcpy(nm, DINO_VERSION_##nm, sizeof(type))
#define DINO_REVERT_END()       __dino_unset_recovery_bit(); }

#define DINO_VERSION_NAME(var) DINO_VERSION_##var

/* Makes use of the above macros which must be called after the boundary */
#define DINO_TASK_BOUNDARY_MANUAL(t) __mementos_checkpoint();\

/* Makes use of the recovery method defined using macros below */
#define DINO_TASK_BOUNDARY_SEMIAUTO(t,...) __mementos_checkpoint();\
                                  if( __dino_recovery_bit_set() ){\
                                    __dino_recover(t, __VA_ARGS__);\
                                    __dino_unset_recovery_bit();\
                                  }

/* Macros for automated compiler-inserted versioning */

#define DINO_TASK_BOUNDARY(t,...) __dino_task_boundary(t);\
                                  __mementos_checkpoint();

#define DINO_RESTORE_CHECK() unsigned int addr = __mementos_find_active_bundle();\
                             if( addr != 0xffff ){ \
                               __mementos_restore(addr);\
                             }

#define DINO_RECOVERY_ROUTINE_LIST_BEGIN() void __dino_recover(unsigned int recovery, ... ){\
                                 \
                                   switch(recovery){

#define DINO_RECOVERY_ROUTINE_LIST_END() default:\
                                           break;\
                                   }; \
                                       }

#define DINO_RECOVERY_RTN_BEGIN(n) case n:\
                                     {\
                                       va_list a; va_start(a, recovery);\
                                         

#define DINO_RECOVERY_RTN_END()  va_end(a); break; }

#define DINO_BIND(type) va_arg(a,type);

#define DINO_REVERT(type,nm) type DINO_VERSION_##nm = va_arg(a,type); nm = DINO_VERSION_#nm;

/*We want to be able to provide three levels of recovery for NV-external inconsistency

level 1: manual versioning
DINO_VERSION(unsigned int, x) --> (volatile) unsigned int DINO_VERSION_x = x;
DINO_VERSION_PTR(unsigned int, x) --> (volatile) unsigned int DINO_VERSION_x; memcpy(&DINO_VERSION_x,x,sizeof(x));
DINO_TASK_BOUNDARY(...)

level 2: explicit regions of atomicity for specified NV vars
DINO_ATOMIC(x,y,z) --> DINO_VERSION(x,...); DINO_VERSION(y,...); DINO_VERSION(z,...);
DINO_TASK_BOUNDARY_WITH_ATOMICS(...)  
[in recovery the following code is mostly automatic]
DINO_ATOMIC_BIND(x,...)
DINO_ATOMIC_BIND(y,...)
DINO_ATOMIC_BIND(z,...)
x = VERSION_x
y = VERSION_y
z = VERSION_z

level 3: fully automatic atomicity for all NV data.  Compiler support needed.
DINO_TASK_BOUNDARY(...) --> [for all NV vars x in this task] DINO_VERSION(x)


*/


/* Variable placement in nonvolatile memory; linker puts this in right place */
#define __fram __attribute__((section("FRAMVARS")))

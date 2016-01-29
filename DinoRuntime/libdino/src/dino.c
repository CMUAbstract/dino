#include <libmementos/mementos.h>

#include <libdino/dino.h>

void __dino_task_boundary(unsigned t){
  
}

void __dino_unset_recovery_bit(){
  __mementos_restored = 0;
}

unsigned int __dino_recovery_bit_set(){
  return ((__mementos_restored)==0x1);
}

unsigned int __dino_find_next_recovery( ){

  unsigned int checkpointAddr = __mementos_find_active_bundle();
  if( checkpointAddr == FRAM_FIRST_BUNDLE_SEG ){

    return DINO_SECOND_RECOVERY;

  }else if(checkpointAddr == FRAM_SECOND_BUNDLE_SEG){

    return DINO_FIRST_RECOVERY;

  }else{

    *((unsigned long *)DINO_FIRST_RECOVERY) = 0xBEEF;
    *((unsigned long *)DINO_SECOND_RECOVERY) = 0xBEEF;
    return DINO_FIRST_RECOVERY;

  }

}

unsigned int __dino_find_active_recovery( ){

  unsigned int checkpointAddr = __mementos_find_active_bundle();
  if( checkpointAddr == FRAM_FIRST_BUNDLE_SEG ){

    return DINO_FIRST_RECOVERY;

  }else if(checkpointAddr == FRAM_SECOND_BUNDLE_SEG){

    return DINO_SECOND_RECOVERY;

  }else{

    *((unsigned long *)DINO_FIRST_RECOVERY) = 0xBEEF;
    *((unsigned long *)DINO_SECOND_RECOVERY) = 0xBEEF;
    return DINO_FIRST_RECOVERY;

  }

}

unsigned int __dino_recovery_get( unsigned int recoveryAddr ){

  
  return (*((unsigned int*)__dino_find_active_recovery()));

}

void __dino_recovery_set(unsigned int p){

  (*((unsigned int*)__dino_find_active_recovery())) = p;

}

void __dino_set_next_recovery(unsigned int p){

  (*((unsigned int*)__dino_find_next_recovery())) = p;

}


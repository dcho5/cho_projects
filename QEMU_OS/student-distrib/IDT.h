//defines interactions with IDT table

#ifndef _IDT_H
#define _IDT_H

#include "x86_desc.h"
#include "Linkage.h"
#include "exceptions.h"
#include "keyboard.h"
#include "rtc.h"
#include "types.h"

//function that fills in IDT table
void populate_IDT();



#endif //_IDT_H

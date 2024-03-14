#pragma once

#define PS2_PORT   0x60    //  Read/Write 	Data Port
#define PS2_READ   0x64    //  Read 	    Status Register
#define PS2_WRITE  0x64    //  Write 	    Command Register 

void ps2_init();

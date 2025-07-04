#pragma once

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x2

#define PIC1	            0x20		/* IO base address for master PIC */
#define PIC2	            0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND        PIC1
#define PIC1_DATA           (PIC1+1)
#define PIC2_COMMAND        PIC2
#define PIC2_DATA           (PIC2+1)

#define ICW1_ICW4           0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE         0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4      0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL          0x08		/* Level triggered (edge) mode */
#define ICW1_INIT           0x10		/* Initialization - required! */
 
#define ICW4_8086           0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO           0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE      0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER     0x0C		/* Buffered mode/master */
#define ICW4_SFNM           0x10		/* Special fully nested (not) */


#define DIVISION_ERROR_INTERRUPT 					0
#define DEBUG_INTERRUPT 							1
#define NMI_INTERRUPT 								2
#define BREACKPOINT_INTERRUPT 						3
#define OVERFLOW_INTERRUPT 							4
#define BOUND_RANGE_EXCEEDED_INTERRUPT 				5
#define INVALID_OPCODE_INTERRUPT 					6
#define DEVICE_NOT_AVAILABLE_INTERRUPT 				7
#define DOUBLE_FAULT_INTERRUPT 						8
#define INVALID_TSS_INTERRUPT 						10
#define SEGMENT_NOT_PRESENT_INTERRUPT 				11
#define STACK_SEGMENT_FAULT_INTERRUPT 				12
#define GENERAL_PROTECTION_FAULT_INTERRUPT 			13
#define PAGE_FAULT_INTERRUPT 						14
#define x87_FPE_INTERRUPT 							16	
#define ALIGNMENT_CHECK_INTERRUPT 					17
#define MACHINE_CHECK_INTERRUPT 					18
#define SIMD_FPE_INTERRUPT 							19
#define VIRTUALIZATION_EXCEPTION_INTERRUPT 			20
#define CONTROL_PROTECTION_EXCEPTION_INTERRUPT 		21
#define HYPERVISOR_INJECTION_EXCEPTION_INTERRUPT 	28
#define VMM_COMMUNICATION_EXCEPTION_INTERRUPT 		29
#define SECURITY_EXCEPTION_INTERRUPT 				30

void cpu_pic_init();
int cpu_pic_is_enabled();
void cpu_pic_disable();
void cpu_pic_apic_config();

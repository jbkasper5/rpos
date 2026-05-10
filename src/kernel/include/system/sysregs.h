#ifndef __SYSREGS_H__
#define __SYSREGS_H__

#include "macros.h"

typedef union {
    struct {
        // MMU enable for EL3 translation
        u64 m           : 1;  // [0]

        // EL3 alignment fault checking enable
        u64 a           : 1;  // [1]

        // cacheability for EL3
        u64 c           : 1;  // [2]

        // stack alignment check for EL3
        u64 sa          : 1;  // [3]
        u64 res1        : 2;  // [5:4]

        // alginemnt faults for EL3
        u64 naa         : 1;  // [6]
        u64 res2        : 4;  // [10:7]

        // exit is content synchronizing: leave 0
        u64 eos         : 1;  // [11]

        // Instruction access Cacheability control, doesn't impact EL1/0
        u64 i           : 1;  // [12]

        // enabling of pointer authentication for EL3
        u64 endb        : 1;  // [13]
        u64 res3        : 5;  // [18:14]

        // write implies execute-never
        u64 wxn         : 1;  // [19]
        u64 res4        : 1;  // [20]

        // Implicit Error Synchronization event enable.
        u64 iesb        : 1;  // [21]

        // Exception Entry is Context Synchronizing
        u64 eis         : 1;
        u64 res4        : 2;  // [24:23]

        // Endianness of data accesses at EL3, and stage 1 translation table walks in the EL3 translation regime
        u64 ee          : 1;  // [25]
        u64 res5        : 1;  // [26]

        // enabling of pointer authentication
        u64 enda        : 1;  // [27]
        u64 res5        : 2;  // [29:28]

        // enabling of pointer authentication
        u64 enib        : 1;  // [30]

        // enabling of pointer authentication
        u64 enia        : 1;  // [31]
        u64 res6        : 4;  // [35:32]

        // PAC Branch Type compatibility at EL3
        u64 bt          : 1;  // [36]

        // tag check fault stuff
        u64 itfsb       : 1;  // [37]
        u64 res0_38_39  : 2;  // [39:38]

        // tag check fault for EL3
        u64 tcf         : 2;  // [41:40]
        u64 res0_42     : 1;  // [42]

        // Allocation Tag Access in EL3.
        u64 ata         : 1;  // [43]

        // Default PSTATE.SSBS value on Exception Entry
        u64 dssbs       : 1;  // [44]
        u64 res0_45     : 6;  // [50:45]

        // Forces a trivial implementation of the Transactional Memory Extension at EL3
        u64 tmt         : 1;  // [51]
        u64 res0_52     : 6;  // [52]

        // Enables the Transactional Memory Extension at EL3
        u64 tme         : 1;  // [53]
        u64 res0_54_60  : 7;  // [60:54]

        // Non-maskable Interrupt enable
        u64 nmi         : 1;  // [61]

        // SP Interrupt Mask enable
        u64 spintmask   : 1;  // [62]
        u64 res0_63     : 1;  // [63]
    };
    u64 value;
} sctlr_el3_t;

typedef union {
    struct {
        // Non-secure bit, dictates EL1 + EL0
        u64 ns          : 1;  // [0]

        // physical IRQ routing, set to 0 to not route to EL3
        u64 irq         : 1;  // [1]

        // physical FIQ routing, set to 0 to not route to EL3
        u64 irq         : 1;  // [2]

        // External Abort and SError interrupt routing.
        u64 ea          : 1;  // [3]
        u64 res1_5_4    : 2;  // [5:4]
        u64 res0_6      : 1;  // [6]

        // Disables SMC instructions at EL1 and above
        u64 smd         : 1;  // [7]

        // Enables HVC instructions at EL3 and, if EL2 is enabled in the current Security state, at EL2 and EL1
        u64 hce         : 1;  // [8]

        // When the PE is in Secure state, this bit disables instruction fetch from memory marked in the first stage of translation as being Non-secure.
        u64 sif         : 1;  // [9]

        // Execution state control for lower Exception levels (1 for aarch64)
        u64 rw          : 1;  // [10]

        // Traps Secure EL1 accesses to the Counter-timer Physical Secure timer registers to EL3
        u64 st          : 1;  // [11]

        // Traps EL2, EL1, and EL0 execution of WFI instructions to EL3, from both Security states and both Execution states
        u64 twi         : 1;  // [12]

        // Traps EL2, EL1, and EL0 execution of WFE instructions to EL3, from both Security states and both Execution states
        u64 twi         : 1;  // [13]

        // Traps accesses to the LORSA_EL1, LOREA_EL1, LORN_EL1, LORC_EL1, and LORID_EL1 registers from EL1 and EL2 to EL3
        u64 tlor        : 1;  // [14]

        // Accesses to the RAS ERR* and RAS ERX* registers from EL1 and EL2 to EL3 are trapped
        u64 terr        : 1;  // [15]

        // Trap registers holding "key" values for Pointer Authentication
        u64 apk         : 1;  // [16]

        // Controls the use of the following instructions related to Pointer Authentication
        u64 api         : 1;  // [17]

        // Secure EL2 Enable
        u64 eel2        : 1;  // [18]

        // External aborts to SError interrupt vector
        u64 ease        : 1;  // [19]

        // Non-maskable External Aborts
        u64 nmea        : 1;  // [20]

        // Trap accesses to the registers ERXPFGCDN_EL1, ERXPFGCTL_EL1, and ERXPFGF_EL1 from EL1 and EL2 to EL3
        u64 fien        : 1;  // [21]
        u64 res1_22_24  : 3;  // [24:22]

        // Enable access to the SCXTNUM_EL2, SCXTNUM_EL1, and SCXTNUM_EL0 registers
        u64 enscxt      : 1;  // [25]

        // Controls access at EL2, EL1 and EL0 to Allocation Tags
        u64 ata         : 1;  // [26]

        // Fine-Grained Traps Enable
        u64 fgte        : 1;  // [27]

        //  Enables access to the CNTPOFF_EL2 register
        u64 ecven       : 1;  // [28]

        // TWE Delay Enable. Enables a configurable delayed trap of the WFE* instruction caused by SCR_EL3.TWE.
        u64 tweden      : 1;  // [29]

        // TWE Delay
        u64 twedel      : 4;  // [33:30]
        u64 res0_34     : 1;  // [34]

        // Activity Monitors Virtual Offsets Enable
        u64 amvoffen    : 1;  // [35]

        // Traps execution of an ST64BV0 instruction at EL0, EL1, or EL2 to EL3
        u64 enaso       : 1;  // [36]

        // Enables access to the ACCDATA_EL1 register at EL1 and EL2
        u64 aden        : 1;  // [37]

        // Enables access to the HCRX_EL2 register at EL2 from EL3
        u64 hxen        : 1;  // [38]
        u64 res0_39     : 1;  // [39]

        // Controls trapping of reads of RNDR and RNDRRS. The exception is reported using ESR_ELx.EC value 0x18
        u64 trndr       : 1;  // [40]

        u64 res0_41_63  : 23; // [63:41]
    };
    u64 value;
} scr_el3_t;

typedef union {
    struct {
        // AArch64 Exception level and selected Stack Pointer
        u64 m           : 5;  // [4:0]
        u64 res0_5      : 1;  // [5]

        // FIQ interrupt mask
        u64 f           : 1;  // [6]

        // IRQ interrupt mask
        u64 i           : 1;  // [7]

        // SError interrupt mask
        u64 a           : 1;  // [8]

        // Debug exception mask
        u64 d           : 1;  // [9]

        // Branch Type Indicator
        u64 btype       : 2;  // [11:10]

        // Speculative Store Bypass
        u64 ssbs        : 1;  // [9]
        u64 res0_13_19  : 7;  // [19:13]

        // Illegal Execution state
        u64 il          : 1;  // [20]

        // Software Step
        u64 ss          : 1;  // [21]

        // Privileged Access Never.
        u64 pan         : 1;  // [22]

        // User Access Override
        u64 uao         : 1;  // [23]

        // Data Independent Timing
        u64 dit         : 1;  // [24]

        // Tag Check Override
        u64 tco         : 1;  // [25]
        u64 res0_26_27  : 2;  // [27:26]

        // Overflow Condition flag
        u64 v           : 1;  // [28]

        // Carry Condition flag
        u64 c           : 1;  // [29]

        // Zero Condition flag
        u64 z           : 1;  // [30]

        // Negative Condition flag
        u64 n           : 1;  // [31]


        u64 res0_32_63  : 32;  // [63:32]
    };
    u64 value;
} spsr_el3_t;

typedef union {
    struct {
        // MMU enable for EL2 or EL2/EL0 translation
        u64 m           : 1;  // [0]

        // EL2 alignment fault checking enable
        u64 a           : 1;  // [1]

        // cacheability for EL2
        u64 c           : 1;  // [2]

        // stack alignment check for EL2
        u64 sa0         : 1;  // [3]
        
        // stack alignment check for EL0 when EL2/EL0 mode is enabled
        u64 sa          : 1;  // [4]

        // System instruction memory barrier enable for aarch32
        u64 cp15ben     : 1;  // [5]

        // alginemnt faults for EL2
        u64 naa         : 1;  // [6]

        // IT instruction enable for aarch32
        u64 itd         : 1;  // [7]

        // SETEND instruction disable for aarch32
        u64 sed         : 1;  // [8]

        // Traps EL0 execution of MSR and MRS instructions that access the PSTATE.{D, A, I, F} masks to EL2 from AArch64 state only
        u64 uma         : 1;  // [9]

        // Enable EL0 access to some System instructions
        u64 enrctx      : 1;  // [5]

        // exit is content synchronizing: leave 0
        u64 eos         : 1;  // [11]

        // Instruction access Cacheability control, doesn't impact EL1/0
        u64 i           : 1;  // [12]

        // enabling of pointer authentication for EL3
        u64 endb        : 1;  // [13]

        // Traps execution of the DC instructions at EL0 to EL2, from AArch64 state only
        u64 dze         : 1;  // [14]

        // Traps EL0 accesses to the CTR_EL0 to EL2, from AArch64 state only.
        u64 uct         : 1;  // [15]

        // Traps execution of WFI instructions at EL0 to EL2, from both Execution states.
        u64 ntwi        : 1;  // [16]
        u64 res0_17     : 1;  // [17]

        // Traps execution of WFE instructions at EL0 to EL2, from both Execution states.
        u64 ntwe        : 1;  // [18]

        // write implies execute-never
        u64 wxn         : 1;  // [19]

        // Trap EL0 Access to the SCXTNUM_EL0 register, when EL0 is using AArch64.
        u64 tscxt       : 1;  // [20]

        // Implicit Error Synchronization event enable.
        u64 iesb        : 1;  // [21]

        // Exception Entry is Context Synchronizing
        u64 eis         : 1;  // [22]

        // Set Privileged Access Never, on taking an exception to EL2
        u64 span        : 1;  // [23]

        // Endianness of data accesses at EL0.
        u64 e0e         : 1;  // [24]

        // Endianness of data accesses at EL2, and stage 1 translation table walks in the EL2 translation regime
        u64 ee          : 1;  // [25]

        // Traps execution of cache maintenance instructions at EL0 to EL2, from AArch64 state only,
        u64 uci         : 1;  // [26]

        // enabling of pointer authentication
        u64 enda        : 1;  // [27]

        // No Trap Load Multiple and Store Multiple to Device-nGRE/Device-nGnRE/Device-nGnRnE memory
        u64 ntlsmd      : 1;  // [28]

        // Load Multiple and Store Multiple Atomicity and Ordering Enable.
        u64 lsmaoe      : 1;  // [29]

        // enabling of pointer authentication
        u64 enib        : 1;  // [30]

        // enabling of pointer authentication
        u64 enia        : 1;  // [31]

        // Controls the required permissions for the cache maintenance instructions executed at EL0
        u64 cmow        : 1;  // [32]

        // Memory Copy and Memory Set instructions Enable
        u64 mscen       : 1;  // [33]

        // Enables direct and indirect accesses to FPMR from EL0
        u64 enfpn       : 1;  // [34]

        // Indicates the Branch Type compatibility of the implicit BTI behavior for some instructions at EL0
        u64 bt0         : 1;  // [35]

        // PAC Branch Type compatibility at EL2
        u64 bt          : 1;  // [36]

        // tag check fault stuff
        u64 itfsb       : 1;  // [37]

        // Tag Check Fault in EL0. Controls the effect of Tag Check Faults due to Loads and Stores in EL0
        u64 tcf0        : 2;  // [39:38]

        // tag check fault for EL2
        u64 tcf         : 2;  // [41:40]

        // Allocation Tag Access in EL0
        u64 ata0        : 1;  // [42]

        // Allocation Tag Access in EL2
        u64 ata         : 1;  // [43]

        // Default PSTATE.SSBS value on Exception Entry
        u64 dssbs       : 1;  // [44]

        // TWE Delay Enable. Enables a configurable delayed trap of the WFE* instruction caused by SCTLR_EL2.nTWE
        u64 tweden      : 1;  // [45]

        // TWE Delay.
        u64 twedelc     : 4;  // [49:46]
        u64 res0_50_53  : 1;  // [53:50]

        // Traps execution of an ST64BV instruction at EL0 to EL2
        u64 enasr       : 1;  // [54]

        // Traps execution of an ST64BV0 instruction at EL0 to EL2
        u64 enas0       : 1;  // [55]

        // Traps execution of an LD64B or ST64B instruction at EL0 to EL2
        u64 enals       : 1;  // [56]

        // Enhanced Privileged Access Never
        u64 epan        : 1;  // [57]

        // Tag Checking Store Only in EL0
        u64 tcso0       : 1;  // [58]

        // Tag Checking Store Only
        u64 tcso        : 1;  // [59]

        // Traps instructions executed at EL0 that access TPIDR2_EL0 to EL2 when EL2 is implemented and enabled for the current Security state
        u64 entp2       : 1;  // [60]

        // Non-maskable Interrupt enable
        u64 nmi         : 1;  // [61]

        // SP Interrupt Mask enable
        u64 spintmask   : 1;  // [62]

        // Trap IMPLEMENTATION DEFINED functionality.
        u64 tidcp       : 1;  // [63]
    };
    u64 value;
} sctlr_el2_t;

typedef union {
    struct {

    }; 
    u64 value;
} hcr_el2_t;

typedef union {
    struct {

    }; 
    u64 value;
} spsr_el2_t;

#endif
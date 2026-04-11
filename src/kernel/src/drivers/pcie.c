#define      PCIE_MISC_REVISION  0x406c
#define      PCIE_MISC_CPU_2_PCIE_WM_0 0x4300
#define      PCIE_RGR1_SW_INIT_1 0x4020


/*
SCB From DTS


––––––––– MAIN PERIPHERAL BLOCK –––––––––
Bus_Address_High        = 0x00 
Bus_Address_Lo          = 0x7c000000 
Bus_Address             = 0x7c000000 

CPU_Physical_High       = 0x00 
CPU_Physical_Lo         = 0xfc000000 
CPU_Physical            = 0xfc000000 

Size_High               = 0x00 
Size_Lo                 = 0x3800000 
Size                    = 0x3800000
–––––––––––––––––––––––––––––––––––––––––

––––––––––––––– GIC BLOCK –––––––––––––––
Bus_Address_High        = 0x00
Bus_Address_Lo          = 0x40000000
Bus_Address             = 0x40000000

CPU_Physical_High       = 0x00 
CPU_Physical_Lo         = 0xff800000 
CPU_Physical            = 0xff800000 

Size_High               = 0x00 
Size_Lo                 = 0x800000 
Size                    = 0x800000
–––––––––––––––––––––––––––––––––––––––––

––––––––––– PCIE MEMORY WINDOW ––––––––––
Bus_Address_High        = 0x06
Bus_Address_Lo          = 0x00
Bus_Address             = 0x600000000

CPU_Physical_High       = 0x06
CPU_Physical_Lo         = 0x00
CPU_Physical            = 0x600000000

Size_High               = 0x00 
Size_Lo                 = 0x40000000 
Size                    = 0x40000000
––––––––––––––––––––––––––––––––––––––––––

––––––––––– LEGACY PERIPHERALS –––––––––––
Bus_Address_High        = 0x00
Bus_Address_Lo          = 0x00
Bus_Address             = 0x00

CPU_Physical_High       = 0x00 
CPU_Physical_Lo         = 0x00 
CPU_Physical            = 0x00 

Size_High               = 0x00 
Size_Lo                 = 0xfc000000 
Size                    = 0xfc000000
–––––––––––––––––––––––––––––––––––––––––
*/
#include <stdint.h>

typedef struct {
      char id[16];                          // identification string eg "TT Builtin" 
      uint64_t smem_start;                  // Start of frame buffer mem 
                                            // (physical address) 
      uint32_t smem_len;                    // Length of frame buffer mem 
      uint32_t type;                        // see FB_TYPE_*                
      uint32_t type_aux;                    // Interleave for interleaved Planes 
      uint32_t visual;                      // see FB_VISUAL_*
      uint16_t xpanstep;                    // zero if no hardware panning
      uint16_t ypanstep;                    // zero if no hardware panning
      uint16_t ywrapstep;                   // zero if no hardware ywrap
      uint32_t line_length;                 // length of a line in bytes
      uint64_t mmio_start;                  // Start of Memory Mapped I/O  
                                            // (physical address)
      uint32_t mmio_len;                    // Length of Memory Mapped I/O 
      uint32_t accel;                       // Indicate to driver which  specific chip/card we have 
                                            //  specific chip/card we have
      uint16_t capabilities;                // see FB_CAP_* 
      uint16_t reserved[2];                 // Reserved for future compatibility
} fb_fix_screeninfo;

typedef struct {
    uint32_t offset;                   /* beginning of bitfield        */
    uint32_t length;                   /* length of bitfield           */
    uint32_t msb_right;                /* != 0 : Most significant bit is */
                                    /* right */
} fb_bitfield;

typedef struct {
      uint32_t xres;                     // visible resolution           
      uint32_t yres;
      uint32_t xres_virtual;             // virtual resolution           
      uint32_t yres_virtual;
      uint32_t xoffset;                  // offset from virtual to visible 
      uint32_t yoffset;                  // resolution                   

      uint32_t bits_per_pixel;           // guess what                   
      uint32_t grayscale;                // 0 = color, 1 = grayscale,    
                                      // >1 = FOURCC                  
      fb_bitfield red;         // bitfield in fb mem if true color, 
      fb_bitfield green;       // else only length is significant 
      fb_bitfield blue;
      fb_bitfield transp;      // transparency                 

      uint32_t nonstd;                   // != 0 Non standard pixel format 

      uint32_t activate;                 // see FB_ACTIVATE_*            

      uint32_t height;                   // height of picture in mm    
      uint32_t width;                    // width of picture in mm     

      uint32_t accel_flags;              // (OBSOLETE) see fb_info.flags 

      // Timing: All values in pixclocks, except pixclock (of course) 
      uint32_t pixclock;                 // pixel clock in ps (pico seconds) 
      uint32_t left_margin;              // time from sync to picture    
      uint32_t right_margin;             // time from picture to sync    
      uint32_t upper_margin;             // time from sync to picture    
      uint32_t lower_margin;
      uint32_t hsync_len;                // length of horizontal sync    
      uint32_t vsync_len;                // length of vertical sync      
      uint32_t sync;                     // see FB_SYNC_*                
      uint32_t vmode;                    // see FB_VMODE_*               
      uint32_t rotate;                   // angle we rotate counter clockwise 
      uint32_t colorspace;               // colorspace for FOURCC-based modes 
      uint32_t reserved[4];              // Reserved for future compatibility 
} fb_var_screeninfo;

typedef struct {
    /* --- Reference Counting and Context --- */
    // atomic_t count;
    int node;
    int flags;
    int class_flag;
    uint32_t state;                  /* Hardware state (e.g., suspended) */

    /* --- Locking --- */
    // struct mutex lock;          /* Lock for open/release/ioctl */
    // struct mutex mm_lock;       /* Lock for fb_mmap and smem operations */

    /* --- Display Specifications (The "Standard" Info) --- */
    fb_var_screeninfo var;      /* User-changeable info */
    fb_fix_screeninfo fix;      /* Hardware-fixed info */
    // struct fb_monspecs monspecs;       /* Monitor characteristics */
    // struct fb_cmap cmap;               /* Current colormap */
    // struct list_head modelist;         /* List of supported videomodes */
    // struct fb_videomode *mode;         /* Current video mode */

    /* --- Graphics Hardware Assets --- */
    // struct fb_pixmap pixmap;           /* Image hardware mapper */
    // struct fb_pixmap sprite;           /* Cursor/Sprite hardware mapper */
    // struct work_struct queue;          /* Framebuffer event queue */

    /* --- Function Pointers and Device Links --- */
    // struct fb_ops *fbops;              /* Driver callbacks (read, write, fillrect) */
    // struct device *device;             /* Parent device (e.g., PCI or Platform) */
    // struct device *dev;                /* This specific fb device (for sysfs) */

    /* --- Memory and Screen Buffer --- */
    // char __iomem *screen_base;         /* Virtual address (Kernel space) */
    unsigned long screen_size;         /* Amount of ioremapped VRAM */
    void *pseudo_palette;              /* Fake palette for truecolor modes */
    
    /* --- Private Driver Data --- */
    void *fbcon_par;                   /* fbcon-specific data */
    void *par;                         /* Private hardware-specific data (struct par) */
    // struct apertures_struct *apertures; /* PCI/Hardware aperture info */
} fb_info;
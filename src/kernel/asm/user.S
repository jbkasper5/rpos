.section ".uram.text"
.globl do_user_things_asm
do_user_things_asm:
    ldr x30, =user_pulse
    ret
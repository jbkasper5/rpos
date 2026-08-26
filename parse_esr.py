#!/usr/bin/env python3
# Decode an AArch64 ESR_EL1 (Exception Syndrome Register) value.
#
# Usage:
#   ./decode_esr.py 0x96000047
#   ./decode_esr.py 96000047 0x9200004f      # decode several at once
#   ./decode_esr.py                          # read values interactively
#
# Reference: Arm Architecture Reference Manual (ARM DDI 0487), section on
# ESR_ELx. Covers the exception classes and fault status codes that actually
# show up on a Cortex-A72 (ARMv8.0) baremetal kernel.

import sys
import textwrap

# ---------------------------------------------------------------------------
# EC: Exception Class, ESR bits [31:26]
# ---------------------------------------------------------------------------
EC = {
    0x00: "Unknown reason",
    0x01: "Trapped WFI/WFE",
    0x03: "Trapped MCR/MRC (cp15, AArch32)",
    0x04: "Trapped MCRR/MRRC (cp15, AArch32)",
    0x05: "Trapped MCR/MRC (cp14, AArch32)",
    0x06: "Trapped LDC/STC (AArch32)",
    0x07: "Trapped SVE/Advanced SIMD/FP access (check CPACR_EL1.FPEN / CPTR)",
    0x08: "Trapped VMRS access (AArch32)",
    0x09: "Trapped pointer-authentication instruction",
    0x0A: "Trapped LD64B/ST64B/ST64BV",
    0x0C: "Trapped MRRC (cp14, AArch32)",
    0x0D: "Branch Target Exception (BTI)",
    0x0E: "Illegal Execution state",
    0x11: "SVC from AArch32",
    0x12: "HVC from AArch32",
    0x13: "SMC from AArch32",
    0x15: "SVC from AArch64 (syscall)",
    0x16: "HVC from AArch64",
    0x17: "SMC from AArch64",
    0x18: "Trapped MSR/MRS/system instruction (AArch64)",
    0x19: "Trapped SVE access",
    0x1A: "Trapped ERET/ERETAA/ERETAB",
    0x1C: "Pointer authentication failure (FPAC)",
    0x20: "Instruction Abort from a lower EL",
    0x21: "Instruction Abort from the same EL",
    0x22: "PC alignment fault",
    0x24: "Data Abort from a lower EL",
    0x25: "Data Abort from the same EL",
    0x26: "SP alignment fault",
    0x28: "Trapped FP exception (AArch32)",
    0x2C: "Trapped FP exception (AArch64)",
    0x2F: "SError interrupt",
    0x30: "Breakpoint from a lower EL",
    0x31: "Breakpoint from the same EL",
    0x32: "Software Step from a lower EL",
    0x33: "Software Step from the same EL",
    0x34: "Watchpoint from a lower EL",
    0x35: "Watchpoint from the same EL",
    0x38: "BKPT instruction (AArch32)",
    0x3A: "Vector Catch exception (AArch32)",
    0x3C: "BRK instruction (AArch64)",
}

# Plain-language debugging context per exception class — what it usually means
# on a baremetal kernel, and where to look. Printed under the EC line.
HINTS = {
    0x00: ("PC reached an undefined/unallocated instruction. Almost always a wild "
           "jump: a corrupt or NULL function pointer, a return into a clobbered "
           "stack frame, or data being executed as code. Disassemble the bytes at "
           "ELR_EL1 - if they look like garbage, the bug is whatever set PC, not "
           "the instruction itself. (A NEON/FP op with SIMD disabled is usually "
           "EC=0x07, not this.)"),
    0x07: ("FP/SIMD/NEON instruction executed while access is trapped. Set "
           "CPACR_EL1.FPEN = 0b11 (and CPTR_EL2/EL3 if you boot through them) "
           "before any FP/NEON code runs."),
    0x0E: ("Illegal Execution state - typically a bad SPSR on exception return "
           "(e.g. M[4:0]/nRW mismatch, or returning to a reserved EL). Check the "
           "SPSR you loaded before ERET."),
    0x15: ("Normal syscall entry. The SVC immediate is in ISS[15:0]; your handler "
           "usually ignores it and reads the number from x8."),
    0x18: ("A system register access trapped to a higher EL. ISS encodes the "
           "register (op0/op1/CRn/CRm/op2). Usually means the reg is disabled at "
           "this EL or routed elsewhere (e.g. CPUECTLR access from EL1)."),
    0x20: ("Instruction fetch faulted. The page at FAR isn't mapped executable - "
           "check it's mapped, AF=1, and not PXN/UXN for this EL. With I-cache on, "
           "also confirm the mapping reached the walker."),
    0x21: ("Instruction fetch faulted at the current EL - kernel jumped to an "
           "address that isn't mapped executable. Same checks as a lower-EL "
           "instruction abort; FAR holds the target."),
    0x22: ("PC alignment fault - PC is not 4-byte aligned. Another wild-jump "
           "symptom (garbage function pointer / corrupted return address)."),
    0x24: ("Userspace data access faulted. See FSC for the cause and FAR for the "
           "address. Translation fault = not mapped; permission = wrong AP/PXN; "
           "alignment = unaligned access (always faults on Device memory)."),
    0x25: ("Kernel data access faulted. See FSC + FAR. On this kernel, watch for: "
           "translation fault (PTE write not visible to the walker), address-size "
           "fault (descriptor PA exceeds TCR.IPS), alignment fault (unaligned word "
           "access to Device/MMIO memory)."),
    0x26: ("SP alignment fault - SP is not 16-byte aligned at a point that "
           "requires it. Check stack setup / a push that desynced SP."),
    0x2F: ("Asynchronous SError - usually an external/imprecise abort (bad bus "
           "access, ECC). ELR is approximate; the real cause is earlier. Often a "
           "stray DMA or an MMIO access to a powered-down/unclocked block."),
}

# Exception classes whose ISS is a data/instruction fault (EC -> is it data?)
INSTR_ABORT_EC = {0x20, 0x21}
DATA_ABORT_EC = {0x24, 0x25}

# ---------------------------------------------------------------------------
# (I|D)FSC: Fault Status Code, ISS bits [5:0] for aborts.
# ---------------------------------------------------------------------------
def fsc_str(fsc):
    # Address size / translation / access-flag / permission faults encode the
    # level in the low 2 bits, so handle those as families.
    family = fsc & 0x3C
    level = fsc & 0x03
    if family == 0x00 and fsc <= 0x03:
        return f"Address size fault, level {level}"
    if family == 0x04:
        return f"Translation fault, level {level}"
    if family == 0x08:
        return f"Access flag fault, level {level}"
    if family == 0x0C:
        return f"Permission fault, level {level}"
    if family == 0x14:
        return f"Synchronous External abort on translation table walk, level {level}"
    if family == 0x1C:
        return f"Sync parity/ECC error on translation table walk, level {level}"
    specific = {
        0x10: "Synchronous External abort (not on table walk)",
        0x11: "Synchronous Tag Check fault (MTE)",
        0x18: "Synchronous parity/ECC error (not on table walk)",
        0x21: "Alignment fault",
        0x30: "TLB conflict abort",
        0x31: "Unsupported atomic hardware update fault",
        0x34: "IMPLEMENTATION DEFINED fault (Lockdown)",
        0x35: "IMPLEMENTATION DEFINED fault (Unsupported Exclusive/atomic access)",
        0x3D: "Section/page domain fault (AArch32 only)",
    }
    return specific.get(fsc, f"Unknown fault status code 0x{fsc:02x}")


def decode_abort_iss(iss, is_data):
    """Return a list of human-readable lines for a data/instruction abort ISS."""
    lines = []
    fsc = iss & 0x3F
    lines.append(f"  FSC   [5:0]  = 0x{fsc:02x}  {fsc_str(fsc)}")

    if is_data:
        wnr = (iss >> 6) & 1
        lines.append(f"  WnR   [6]    = {wnr}     ({'WRITE' if wnr else 'READ'})")

    s1ptw = (iss >> 7) & 1
    if s1ptw:
        lines.append("  S1PTW [7]    = 1     (fault occurred on a stage-1 page-table walk)")

    if is_data:
        cm = (iss >> 8) & 1
        if cm:
            lines.append("  CM    [8]    = 1     (fault from a cache-maintenance or address-translation instruction)")

    ea = (iss >> 9) & 1
    if ea:
        lines.append("  EA    [9]    = 1     (external abort type)")

    fnv = (iss >> 10) & 1
    if fnv:
        lines.append("  FnV   [10]   = 1     (FAR is NOT valid - faulting address unknown)")
    else:
        lines.append("  FnV   [10]   = 0     (FAR holds the faulting virtual address)")

    if is_data:
        isv = (iss >> 24) & 1
        if isv:
            sas = (iss >> 22) & 0x3
            sse = (iss >> 21) & 1
            srt = (iss >> 16) & 0x1F
            sf = (iss >> 15) & 1
            ar = (iss >> 14) & 1
            size = {0: "byte", 1: "halfword", 2: "word", 3: "doubleword"}[sas]
            lines.append(f"  ISV   [24]   = 1     (instruction syndrome valid)")
            lines.append(f"    access size   = {size}")
            lines.append(f"    sign-extend   = {sse}")
            lines.append(f"    Xt register   = x{srt}")
            lines.append(f"    64-bit reg    = {sf}")
            lines.append(f"    acquire/rel   = {ar}")
    return lines


def decode_sysreg_iss(iss):
    """Decode the ISS of a trapped MSR/MRS (EC 0x18)."""
    direction = "read (MRS)" if (iss & 1) else "write (MSR)"
    op0 = (iss >> 20) & 0x3
    op2 = (iss >> 17) & 0x7
    op1 = (iss >> 14) & 0x7
    crn = (iss >> 10) & 0xF
    rt = (iss >> 5) & 0x1F
    crm = (iss >> 1) & 0xF
    enc = f"S{op0}_{op1}_C{crn}_C{crm}_{op2}"
    return [
        f"  Direction   = {direction}",
        f"  Register    = {enc}  (op0={op0} op1={op1} CRn={crn} CRm={crm} op2={op2})",
        f"  Xt          = x{rt}",
    ]


def _wrap_hint(text):
    """Wrap a hint string to a readable width, indented under the EC line."""
    wrapped = textwrap.wrap(text, width=72)
    return ["          -> " + wrapped[0]] + ["             " + line for line in wrapped[1:]]


def decode(esr):
    out = []
    ec = (esr >> 26) & 0x3F
    il = (esr >> 25) & 1
    iss = esr & 0x1FFFFFF

    out.append(f"ESR_EL1 = 0x{esr:08x}")
    out.append(f"  EC    [31:26] = 0x{ec:02x}  {EC.get(ec, 'Reserved / unknown EC')}")
    if ec in HINTS:
        out += _wrap_hint(HINTS[ec])
    out.append(f"  IL    [25]    = {il}     ({'32-bit' if il else '16-bit'} trapped instruction)")
    out.append(f"  ISS   [24:0]  = 0x{iss:07x}")

    if ec in DATA_ABORT_EC:
        out += decode_abort_iss(iss, is_data=True)
    elif ec in INSTR_ABORT_EC:
        out += decode_abort_iss(iss, is_data=False)
    elif ec == 0x18:
        out += decode_sysreg_iss(iss)
    elif ec in (0x15, 0x11):
        out.append(f"  Syscall #   = {iss & 0xFFFF}")

    return "\n".join(out)


def parse_value(s):
    s = s.strip().lower()
    if s.startswith("0x"):
        s = s[2:]
    return int(s, 16)


def main():
    args = sys.argv[1:]
    if args:
        for i, a in enumerate(args):
            try:
                print(decode(parse_value(a)))
            except ValueError:
                print(f"Could not parse '{a}' as a hex value", file=sys.stderr)
            if i != len(args) - 1:
                print()
        return

    print("Enter ESR values (hex), one per line. Ctrl-D to quit.")
    try:
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            try:
                print(decode(parse_value(line)))
            except ValueError:
                print(f"Could not parse '{line}' as a hex value", file=sys.stderr)
            print()
    except (EOFError, KeyboardInterrupt):
        pass


if __name__ == "__main__":
    main()

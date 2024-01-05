ENTRY(start)
phys = 0x1000;
program_addr_phys = 0x3004000; /* this is 0x1000 higher... for some reason */
program_addr_virt = 0xF00000;

SECTIONS
{ 
  .text phys : AT(phys) {
    code = .;
    *(.kernel_entry)
    *(EXCLUDE_FILE (*src/stock/tedit.o) .text)
    *(EXCLUDE_FILE (*src/stock/tedit.o) .rodata)
    . = ALIGN(4096);
  }
  .data : AT(phys + (data - code))
  {
    data = .;
    *(EXCLUDE_FILE (*src/stock/test_program.o) .data)
    . = ALIGN(4096);
  }
  .bss : AT(phys + (bss - code))
  {
    bss = .;
    *(EXCLUDE_FILE (*src/stock/tedit.o) .bss)
    . = ALIGN(4096);
  }
  
  .test_program program_addr_virt : AT(((phys + (bss - code) + SIZEOF (.bss) + 4096) & 0xFFFFFFFFF000) - 512)
  {
    test_program = .;
    *(.tedit_header) 
    *(.tedit_code)
    src/stock/tedit.o(*.text *.rodata *.data *.bss)
    . = ALIGN(4096);
  }

  .other_program program_addr_virt : AT(((((phys + (bss - code) + SIZEOF (.bss) + SIZEOF(.test_program) + 4096) & 0xFFFFFFFFF000) - 512 + 4096) & 0xFFFFFFFFF000) - 512)
  {
    test_program = .;
    *(.other_header) 
    *(.other_entry)
    src/stock/other_program.o(*.text *.rodata *.data *.bss)
    . = ALIGN(4096);
  }

  end = .;
}
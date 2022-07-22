# REQUIRES: x86, zstd

# RUN: llvm-mc -filetype=obj -triple=x86_64 --compress-debug-sections=zstd %s -o %t.o

# RUN: ld.lld %t.o -o %t.so -shared
# RUN: llvm-readobj --sections --section-data %t.so | FileCheck -check-prefix=DATA %s

# DATA:      Section {
# DATA:        Index: 6
# DATA:        Name: .debug_str
# DATA-NEXT:   Type: SHT_PROGBITS
# DATA-NEXT:   Flags [
# DATA-NEXT:     SHF_MERGE (0x10)
# DATA-NEXT:     SHF_STRINGS (0x20)
# DATA-NEXT:   ]
# DATA-NEXT:   Address: 0x0
# DATA-NEXT:   Offset:
# DATA-NEXT:   Size: 69
# DATA-NEXT:   Link: 0
# DATA-NEXT:   Info: 0
# DATA-NEXT:   AddressAlignment: 1
# DATA-NEXT:   EntrySize: 1
# DATA-NEXT:   SectionData (
# DATA-NEXT:     0000: 756E7369 676E6564 20696E74 00636861  |unsigned int.cha|
# DATA-NEXT:     0010: 7200756E 7369676E 65642063 68617200  |r.unsigned char.|
# DATA-NEXT:     0020: 73686F72 7420756E 7369676E 65642069  |short unsigned i|
# DATA-NEXT:     0030: 6E74006C 6F6E6720 756E7369 676E6564  |nt.long unsigned|
# DATA-NEXT:     0040: 20696E74 00                          | int.|
# DATA-NEXT:   )
# DATA-NEXT: }

.section .debug_str,"MS",@progbits,1
.LASF2:
 .string "short unsigned int"
.LASF3:
 .string "unsigned int"
.LASF0:
 .string "long unsigned int"
.LASF8:
 .string "char"
.LASF1:
 .string "unsigned char"

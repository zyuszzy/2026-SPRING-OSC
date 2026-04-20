#ifndef FDT_H
#define FDT_H

#include "type.h"

// Token for DTB structure Block
#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE   0x00000002
#define FDT_PROP       0x00000003
#define FDT_NOP        0x00000004
#define FDT_END        0x00000009


// offical FDT header
struct fdt_header {
    uint32_t magic;                 // 0xd00dfeed (Big-Endian)
    uint32_t totalsize;             
    uint32_t off_dt_struct;         
    uint32_t off_dt_strings;        
    uint32_t off_mem_rsvmap;        
    uint32_t version;               
    uint32_t last_comp_version;     
    uint32_t boot_cpuid_phys;       
    uint32_t size_dt_strings;       
    uint32_t size_dt_struct;        
};

typedef struct {
    uint64_t mem_start;
    uint64_t mem_size;
    uint64_t initrd_start;
    uint64_t initrd_end;
    uint64_t dtb_start;
    uint32_t dtb_size;
} boot_info_t;

int name_match(const char *node_name, const char *current_path);
int fdt_path_offset(const void* fdt, const char* path);
const void *fdt_getprop(const void *fdt, int nodeoffset, const char *name, int *lenp);
void fdt_get_boot_info(const void* fdt, boot_info_t* info);
void fdt_additional_reserve_mem(const void* fdt);
int fdt_node_offset_by_compatible(const void* fdt, int startoffset, const char* compatible);

#endif
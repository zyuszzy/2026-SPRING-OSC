# include "fdt.h"
# include "type.h"
# include "uart.h"
# include "string.h"

int name_match(const char *node_name, const char *current_path){
    int i = 0;

    // 先比對到字串結束、碰到 '/' 或碰到 '@'
    while (current_path[i] != '\0' && current_path[i] != '/' && current_path[i] != '@') {
        if (node_name[i] != current_path[i]) 
            return 0;
        i++;
    }

    // 情況 A: current_path 在這裡結束了 (或是碰到 '/')
    // 此時 node_name 必須也結束，或者是剛好碰到 '@' 
    if (current_path[i] == '\0' || current_path[i] == '/') {
        if (node_name[i] == '\0' || node_name[i] == '@') {
            return i;
        }
        return 0;
    }

    // 情況 B: current_path 指定了位址 (例如 uart@d4017000)
    // 此時 node_name 必須也完全一致
    if (current_path[i] == '@') {
        while (node_name[i] != '\0' && current_path[i] != '\0' && current_path[i] != '/') {
            if (node_name[i] != current_path[i])
                return 0;
            i++;
        }
        // 確認兩邊都結束或 current_path 碰到下一個層級
        if ((node_name[i] == '\0') && (current_path[i] == '\0' || current_path[i] == '/')) {
            return i;
        }
    }
    return 0;
}

int fdt_path_offset(const void* fdt, const char* path) {

    struct fdt_header* header = (struct fdt_header*)fdt;
    const char* fdt_base = (const char*)fdt;
    const char* p = fdt_base + bswap32(header->off_dt_struct);      // p目前為struture block開頭

    // special case: root node
    if (strcmp(path, "/") == 0) 
        return (int)(p - fdt_base);

    const char* current_path = path;
    
    // 跳過 root node 的 '/'
    if(*current_path == '/')
        current_path++;

    int current_level = -1;     // 當前所在遍歷的 level
    int skip_until_level = -1;  // 此條路不是要找的，要退回到的level

    while(1){
        uint32_t token = bswap32(*(uint32_t*)p);

        // 現在 p 停在 token 前面
        if(token == FDT_BEGIN_NODE){
             const char* node_name = p + 4;
             current_level++;

             // root node，跳過比對
             if(current_level == 0 && *node_name == '\0'){
                p += 4;
                p = (const char*)align_up(p + 1, 4);
                continue;
             }

             // 開始比對
             if(skip_until_level == -1 && *current_path != '\0'){
                int len = name_match(node_name, current_path);
                if(len > 0){
                    current_path += len;
                    if(*current_path == '/')
                        current_path++;
                    
                    // path查找完 代表找到
                    if(*current_path == '\0')
                        return (int)(p - fdt_base);
                }else{                                  // 此條不是要找的
                    skip_until_level = current_level;
                }
             }

             p += 4;
             p = (const char*)align_up(p + strlen(node_name) + 1, 4);
        }else if(token == FDT_END_NODE){
            if(skip_until_level == current_level)
                skip_until_level = -1;
            current_level--;
            p += 4;
        }else if(token == FDT_PROP){
            p += 4;
            uint32_t prop_len = bswap32(*(uint32_t*)p);
            p += 8;     // 跳過 prop_len 和 prop_name_offset
            p = (const char*)align_up(p + prop_len, 4);
        }else if(token == FDT_END){
            break;
        }else{
            p += 4;
        }
    }
    
    return -1;
}

const void *fdt_getprop(const void *fdt, int nodeoffset, const char *name, int *lenp){

    struct fdt_header* header = (struct fdt_header*)fdt;
    const char* fdt_base = (const char*)fdt;
    const char* p = fdt_base + nodeoffset; 
    const char* string_base = fdt_base + bswap32(header->off_dt_strings);

    // 確認目前指標在BEGINNODE
    if(bswap32(*(uint32_t*)p) != FDT_BEGIN_NODE)
        return NULL;

    // 跳過BEGINNODE & node name
    p += 4;
    p = (const char*)align_up(p + strlen(p) + 1, 4);

    // traverse 該 node 的內容
    while(1){
        uint32_t token = bswap32(*(uint32_t*)p);
        
        if(token == FDT_PROP){
            uint32_t prop_len = bswap32(*(uint32_t*)(p + 4));
            uint32_t name_off = bswap32(*(uint32_t*)(p + 8));
            const char* prop_name = string_base + name_off;

            // 比對 property 名稱 
            if(strcmp(prop_name, name) == 0){
                if(lenp) *lenp = prop_len;  // 如果使用者有提供存放長度的變數（不是傳 NULL），才把長度寫進去。
                return (const void*)(p + 12); // 永遠回傳 Token 往後加 12 的位置
            }

            // 沒找到就跳到下一個 token
            p = (const char*)align_up(p + 12 + prop_len, 4);
        }else if(token == FDT_NOP){
            p += 4;
        }else{
            break;
        }
    }
    return NULL;
}
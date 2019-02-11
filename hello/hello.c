#include <elf.h>
#include <link.h>
#include <stddef.h>

ElfW(auxv_t)* getAUXV(char** endArgv)
{
    while (*endArgv)
        endArgv++;
    endArgv++;
    while (*endArgv)
        endArgv++;
    return (ElfW(auxv_t)*)++endArgv;
}

ElfW(Phdr)* getAuxvEntry(ElfW(auxv_t)* auxv, unsigned type)
{
    while (auxv->a_type != AT_NULL && auxv->a_type != type)
        auxv++;
    return (void*)auxv->a_un.a_val;
}

void* getBasePtr(ElfW(Phdr)* phdr)
{
    return (char*)phdr - phdr->p_vaddr;
}

void* getAbsPtr(char* basePtr, ElfW(Addr) offset)
{
    return basePtr + offset;
}

ElfW(Phdr)* searchPhdrSegment(ElfW(Phdr)* phdr,
                              ElfW(Word) type)
{
    while (phdr->p_type != PT_NULL && phdr->p_type != type)
        phdr++;
    return phdr;
}

void* searchFirstDynEntry(ElfW(Dyn)* dyn,
                          ElfW(Sxword) type,
                          struct link_map* linkMap)
{
    while (dyn->d_tag != type)
        dyn++;
    void* res = (void*)dyn->d_un.d_ptr;
    if (linkMap && res < (void*)linkMap->l_addr)
        return res + linkMap->l_addr;
    return res;
}

int mystrcmp(char* str1, char* str2)
{
    while (*str1 && *str2 && *str1 == *str2)
    {
        str1++;
        str2++;
    }
    return !*str1 && !*str2 && *str1 == *str2;
}

void* searchTab(ElfW(Dyn)* dyn,
               char* symbol,
               void* base,
               struct link_map* linkMap)
{
    char* strtab = searchFirstDynEntry(dyn, DT_STRTAB, NULL);
    ElfW(Sym)* symtab = searchFirstDynEntry(dyn, DT_SYMTAB, linkMap);
    while ((void*)symtab < (void*)strtab)
    {
        if (mystrcmp(symbol, strtab + symtab->st_name) && symtab->st_shndx)
            return (void*)((char*)base + symtab->st_value);
        symtab++;
    }
    return NULL;
}

void* iterMap(struct r_debug* map, char* symbol, void* base)
{
    struct link_map* linkMap = map->r_map;
    while (linkMap->l_next)
    {
        void* ret = searchTab(linkMap->l_ld, symbol, (void*)linkMap->l_addr, linkMap);
        if (ret)
            return ret;
        linkMap = linkMap->l_next;
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    ElfW(auxv_t)* auxv = getAUXV(argv);
    ElfW(Phdr)* phdr = getAuxvEntry(auxv, AT_PHDR);
    void* basePtr = getBasePtr(phdr);
    ElfW(Phdr)* phdrDyn = searchPhdrSegment(phdr, PT_DYNAMIC);

    ElfW(Dyn)* dyn = (ElfW(Dyn)*)((char*)basePtr + phdrDyn->p_vaddr);
    struct r_debug* map = searchFirstDynEntry(dyn, DT_DEBUG, NULL);
    void* ret = iterMap(map, "printf", basePtr);
    if (ret)
    {
        void (*func)() = ret;
        func("Hello World !\n");
    }

    return 0;
}

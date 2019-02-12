#include <elf.h>
#include <link.h>
#include <stddef.h>

ElfW(auxv_t)* getAUXV(char** envp)
{
    for (; *envp; envp++)
        continue;
    return (ElfW(auxv_t)*)++envp;
}

ElfW(Phdr)* searchAuxvEntry(ElfW(auxv_t)* auxv, unsigned type)
{
    for (; auxv->a_type != type; auxv++)
        continue;
    return (void*)auxv->a_un.a_val;
}

ElfW(Phdr)* searchPhdrSegment(ElfW(Phdr)* phdr, ElfW(Word) type, unsigned size)
{
    for (unsigned c = 0; c < size && phdr->p_type != type; phdr++, c++)
        continue;
    return phdr;
}

void* searchFirstDynEntry(ElfW(Dyn)* dyn,
                          ElfW(Sxword) type,
                          struct link_map* linkMap)
{
    for (; dyn->d_tag != type; dyn++)
        continue;

    void* res = (void*)dyn->d_un.d_ptr;
    if (linkMap && res < (void*)linkMap->l_addr)
        return res + linkMap->l_addr;
    return res;
}

int mystrcmp(char* str1, char* str2)
{
    for (; *str1 && *str2 && *str1 == *str2; str1++, str2++)
        continue;
    return !*str1 && !*str2 && *str1 == *str2;
}

void* searchFunc(ElfW(Dyn)* dyn,
               char* symbol,
               void* base,
               struct link_map* lMap)
{
    char* strtab = searchFirstDynEntry(dyn, DT_STRTAB, NULL);
    ElfW(Sym)* symtab = searchFirstDynEntry(dyn, DT_SYMTAB, lMap);

    for (; (void*)symtab < (void*)strtab; symtab++)
    {
        if (mystrcmp(symbol, strtab + symtab->st_name) && symtab->st_shndx)
            return (void*)((char*)base + symtab->st_value);
    }
    return NULL;
}

void* iterMap(struct r_debug* map, char* symbol, void* base)
{
    struct link_map* lMap = map->r_map;
    for (; lMap->l_next; lMap = lMap->l_next)
    {
        void* ret = searchFunc(lMap->l_ld, symbol, (void*)lMap->l_addr, lMap);
        if (ret)
            return ret;
    }
    return NULL;
}

int main(int argc, char* argv[], char* envp[])
{
    ElfW(auxv_t)* auxv = getAUXV(envp);
    ElfW(Phdr)* phdr = searchAuxvEntry(auxv, AT_PHDR);
    unsigned phNum = (size_t)searchAuxvEntry(auxv, AT_PHNUM);
    void* basePtr = (char*)phdr - phdr->p_vaddr; // Calculate the base pointer

    ElfW(Phdr)* phdrDyn = searchPhdrSegment(phdr, PT_DYNAMIC, phNum);

    // Get the Dyn of the segment
    ElfW(Dyn)* dyn = (ElfW(Dyn)*)((char*)basePtr + phdrDyn->p_vaddr);

    struct r_debug* map = searchFirstDynEntry(dyn, DT_DEBUG, NULL);

    // Search and return the "printf" symbol
    void* ret = iterMap(map, "printf", basePtr);
    if (ret)
    {
        void (*func)() = ret;
        func("Hello World !\n");
    }

    return 0;
}

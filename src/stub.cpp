#include "stdafx.h"

typedef uint64_t OFFSET;
#define CRYPTED __attribute__((section(".secure")))

/**
 * @brief The encrypted functions that must be unreadable.
 * 
 */
CRYPTED void stub() {}
CRYPTED int evilMain() {

    printf("\nHahahahah, I'm Evil! ;)\n");
    return 0x666;
}

/**
 * @brief The part where the crypted functions are beeing decrypted.
 * 
 */

/**
 * @brief Reads all section headers into a array.
 */
Elf64_Shdr* readAllSections(FILE* target, Elf64_Ehdr hdr) {
    
    Elf64_Shdr* allSections = new Elf64_Shdr[hdr.e_shnum];
    
    fseek(target, hdr.e_shoff, SEEK_SET);

    for (int i = 0; i < hdr.e_shnum; i++) {
        fread(&allSections[i], hdr.e_shentsize, 1, target);
    }

    return allSections;
}

/**
 * @brief Puts the sections string table into char* array.
 */
char* sectionStringTable(FILE* target, Elf64_Shdr section) {

    char* table = new char[section.sh_size];
    fseek(target, section.sh_offset, SEEK_SET);
    fread(table, section.sh_size, 1, target);
    return table;
}

unsigned char* decryptBytes(unsigned char* bytes, int size) {

    unsigned char* decrypted = new unsigned char[size];
    
    for (int i = 0; i < size; i++) {
        decrypted[i] = bytes[i] -13;
    }

    return decrypted;    
}

/**
 * @brief Decrypts the hidden sections. 
 */
bool decryptSections(FILE* target, Elf64_Shdr secretSection) {

    // First locate the (encrypted) function bytes
    fseek(target, secretSection.sh_offset, SEEK_SET);
    unsigned char functionBytes[secretSection.sh_size];
    fread(functionBytes, secretSection.sh_size, 1, target);

    // Decrypt the section bytes.
    unsigned char* decrypted = decryptBytes(functionBytes, secretSection.sh_size);

    // Now overwrite the function bytes inside the memory page.
    long pageSize = sysconf(_SC_PAGESIZE);
    long pageAddr = (long)&stub-((long)&stub % pageSize);

    // Change the permissions to able to write.
    if (mprotect((void*)pageAddr, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC) < 0) {
        printf("Failed to change permissions to write! Code: %d\n", errno);
        return false;
    }

        // Write the decrypted data to the correct location inside the page.
    memcpy((void*)&stub, decrypted, secretSection.sh_size);

    // Set permissions back.
    if (mprotect ((void*)pageAddr, pageSize, PROT_READ | PROT_EXEC) < 0) {
        printf("Failed to change permissions back! Code: %d\n", errno);
        return false;
    } 

    return true;
}

bool decryptSection(char* progname) {

    Elf64_Ehdr elfHdr;
    Elf64_Shdr* allSections;
    OFFSET sectionHeaderOff;
    long sectionSize;
    int indexStringTable;
    
    FILE* target = fopen(progname, "rb");
    fread(&elfHdr, sizeof(Elf64_Ehdr), 1, target);

    allSections = readAllSections(target, elfHdr);
    char* stringTable = sectionStringTable(target, allSections[elfHdr.e_shstrndx]);

    for (int i = 0; i < elfHdr.e_shnum; i++) {
        if (memcmp(stringTable + allSections[i].sh_name, ".secure\0", 8) == 0) {
            if (!decryptSections(target, allSections[i])) {
                printf("Failed to unpack section!\n\n");
                return false;
            }
        }
        
        // For example (multiple sections for a key or something)
        //if (memcmp(stringTable + allSections[i].sh_name, ".key\0", 8) == 0) {
    }

    return true;    
}

int main(int argc, char* argv[]) {

    printf("Decrypting section...\n");
    if (decryptSection(argv[0]))
        return evilMain();
    return 1;
}
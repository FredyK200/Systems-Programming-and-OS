#include <stdio.h>

typedef struct PartitionTableEntry
{
    unsigned char   bootFlag;
    unsigned char   CHSBegin[3];
    unsigned char   typeCode;
    unsigned char   CHSEnd[3];
    unsigned int    LBABegin;
    unsigned int    LBAEnd;
}__attribute__((__packed__)) PartitionTableEntry;

typedef struct MBRStruct
{
    unsigned char bootCode[446];
    PartitionTableEntry part1;
    PartitionTableEntry part2;
    PartitionTableEntry part3;
    PartitionTableEntry part4;
    unsigned short flag;
}__attribute__((__packed__)) MBR;

typedef struct BPBStruct
{
    unsigned char   jmpBoot[3];     // Jump instruction to boot code
    unsigned char   OEMNane[8];     // 8-Character string (not null terminated)
    unsigned short  BytsPerSec;   // Had BETTER be 512!
    unsigned char   SecPerClus;    // How many sectors make up a cluster?
    unsigned short  RsvdSecCnt;   // # of reserved sectors at the beginning (including the BPB)?
    unsigned char   NumFATs;       // How many copies of the FAT are there? (had better be 2)
    unsigned short  RootEntCnt;   // ZERO for FAT32
    unsigned short  TotSec16;     // ZERO for FAT32
    unsigned char   Media;         // SHOULD be 0xF8 for "fixed", but isn't critical for us
    unsigned short  FATSz16;      // ZERO for FAT32
    unsigned short  SecPerTrk;    // Don't care; we're using LBA; no CHS
    unsigned short  NumHeads;     // Don't care; we're using LBA; no CHS
    unsigned int    HiddSec;        // Don't care ?
    unsigned int    TotSec32;       // Total Number of Sectors on the volume
    unsigned int    FATSz32;        // How many sectors long is ONE Copy of the FAT?
    unsigned short  Flags;        // Flags (see document)
    unsigned short  FSVer;        // Version of the File System
    unsigned int    RootClus;       // Cluster number where the root directory starts (should be 2)
    unsigned short  FSInfo;       // What sector is the FSINFO struct located in? Usually 1
    unsigned short  BkBootSec;    // REALLY should be 6  (sector # of the boot record backup)
    unsigned char   Reserved[12];  // Should be all zeroes -- reserved for future use
    unsigned char   DrvNum;         // Drive number for int 13 access (ignore)
    unsigned char   Reserved1;      // Reserved (should be 0)
    unsigned char   BootSig;        // Boot Signature (must be 0x29)
    unsigned char   VolID[4];       // Volume ID
    unsigned char   VolLab[11];     // Volume Label
    unsigned char   FilSysType[8];  // Must be "FAT32 "
    unsigned char   unused[420];       // Not used
    unsigned char   signature[2];      // MUST BE 0x55 AA
}__attribute__((__packed__)) BPB;

typedef struct DIREntryStrict
{
    unsigned char   Name[8];
    unsigned char   Ext[3];
    unsigned char   Attr;
    unsigned char   NTRes;
    unsigned char   CrtTimeTenth;
    unsigned short  CrtTime;
    unsigned short  CrtDate;
    unsigned short  LstAccDate;
    unsigned short  FstClusHI;
    unsigned short  WrtTime;
    unsigned short  WrtDate;
    unsigned short  FstClusLO;
    unsigned int    FileSize;
}__attribute__((__packed__)) DIREntry;

void PrintDIR();

BPB bpb;
MBR mbr;
DIREntry dir;

    /*==================================
                    Main
    ==================================*/

int main(int argc, char *argv[]) {
    if (argc < 2){ printf("usage: ./FAT32 FILENAME.img\n"); return -1; }
    // Read MBR
    FILE *in = fopen(argv[1], "r");
    fread(&mbr,sizeof(mbr),1,in);
    
    // Read BPB from start of Partition 1
    unsigned int part1Start = mbr.part1.LBABegin;
    fseek(in, part1Start * 512, SEEK_SET);
    fread(&bpb,sizeof(bpb),1,in);
    
    // Find start of FAT and start of RootDIR
    unsigned int FATStart = part1Start + bpb.RsvdSecCnt;            // Sector of FAT
    unsigned int ROOTStart = FATStart + bpb.NumFATs * bpb.FATSz32;  // Sector of ROOT  
    char usrInput;
    while (usrInput != 'q')
    {
        printf(">> ");
        scanf ("%s", &usrInput);
        if (usrInput == 'd')
        {
            fseek(in, ROOTStart * 512, SEEK_SET);
            fread(&dir,sizeof(dir),1,in);
            int FATOffset = bpb.RootClus - 2;
            int curSec = ROOTStart;
            unsigned int fstClusNum = dir.FstClusHI << 16 | dir.FstClusLO;
            // idfk bro

        }
    }

    return 0;
}

void PrintDIR()
{
    // Might move printing of directories here, idk
}


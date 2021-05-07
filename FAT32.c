//FAT32.c - FAT32 File System
//
//Author: Kane Eder
//  Date: April 19, 2021
//Course: EECS 3540 (Operating Systems and Systems Programming)
//
//Purpose:  This program takes a image of a FAT32 drive as command line paraneter
//          It assumes no other Directories besides ROOT and can Display all entries in ROOT
//          and Extract a file from inside the IMG to Outside
//Missing:  Long File Name Support
//
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <uchar.h>

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
    unsigned char   jmpBoot[3];   
    unsigned char   OEMName[8];   
    unsigned short  BytsPerSec;  
    unsigned char   SecPerClus; 
    unsigned short  RsvdSecCnt;   
    unsigned char   NumFATs;     
    unsigned short  RootEntCnt; 
    unsigned short  TotSec16;  
    unsigned char   Media;    
    unsigned short  FATSz16; 
    unsigned short  SecPerTrk;   
    unsigned short  NumHeads;     
    unsigned int    HiddSec;      
    unsigned int    TotSec32;     
    unsigned int    FATSz32;      
    unsigned short  Flags;        
    unsigned short  FSVer;        
    unsigned int    RootClus;     
    unsigned short  FSInfo;       
    unsigned short  BkBootSec;    
    unsigned char   Reserved[12]; 
    unsigned char   DrvNum;       
    unsigned char   Reserved1;    
    unsigned char   BootSig;      
    unsigned char   VolID[4];     
    unsigned char   VolLab[11];   
    unsigned char   FilSysType[8];
    unsigned char   unused[420];  
    unsigned char   signature[2]; 
}__attribute__((__packed__)) BPB;


typedef struct ShortDIRStruct
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
}__attribute__((__packed__)) ShortDIREntry;

typedef struct LongDIRStruct
{
    unsigned char   Ord;
    char16_t        Name1[5];
    unsigned char   Attr;
    unsigned char   Type;
    unsigned char   Chksum;   
    char16_t        Name2[6];
    unsigned short  FstClusLO; 
    char16_t        Name3[2];
}__attribute__((__packed__)) LongDIREntry;

typedef union DIRUnion
{
    ShortDIREntry s;
    LongDIREntry l;
}__attribute__((__packed__)) DIRUnion;

void DisplaySector(unsigned char* sector);
void DisplayRoot();
void ExtractFile(char* input); 
unsigned int FindFirstFileCluster(char* searchingFile);
void DisplayDIRSector(DIRUnion dir[16]);
void PrintFile(DIRUnion dir);

BPB bpb;
MBR mbr;
DIRUnion dir[16];

FILE *in;
unsigned int FATStart;
unsigned int DATAStart;
unsigned char SectorBuffer[512];

    /*==================================
                    Main
    ==================================*/

int main(int argc, char *argv[]) {
    setlocale(LC_NUMERIC, "");  // allows me to output ints with commas (ex 1,002,345)
    // Argument Checking
    if (argc < 2){ printf("usage: ./FAT32 FILENAME.img\n"); return -1; }
    
    // Open File passed by argument 
    // and read the first 512 Bytes
    // into a MBR struct
    //
    in = fopen(argv[1], "r");
    fread(&mbr,sizeof(mbr),1,in);
    
    // Use the MBR to Locate The beging 
    // LBA of the First Partition. Then
    // Read the first block of the partition
    // into a BPB struct
    //
    unsigned int part1Start = mbr.part1.LBABegin;
    fseek(in, part1Start * 512, SEEK_SET);
    fread(&bpb,sizeof(bpb),1,in);
        
    // Calculate the start of FAT and start of RootDIR
    // Knowing the FAT starts right after the RsvdSecCnt
    // and the Root Starts right after the last FAT
    //
    FATStart = part1Start + bpb.RsvdSecCnt;            // Sector of FAT
    DATAStart = FATStart + bpb.NumFATs * bpb.FATSz32;  // Sector of DATA  
    
    // Call Appropriate Function
    // Based on User Input
    //
    char inputString[512];
    char* input = inputString;
    while (strcmp(input, "QUIT") != 0)
    {
        printf(">");
        scanf("%[^\n]%*c",input);
        if (strcmp(input, "DIR") == 0) // DIR 
            DisplayRoot();
        if (strncmp(input,"EXTRACT", 5) == 0) // EXTRACT filename 
            ExtractFile(input);
    }

    return 0;
}

void ExtractFile(char* input)
{
    // Read ROOT DIR and Compare the file names of each entry to the string passed by input.
    // When a match is found, get the data cluster the file starts in.
    // Read each sector into the buffer and save it to a file with the same name.
    // After a full cluster has been read, get the next cluster from the FAT.
    // rinse and repeat until EOF Mark is reached. (0xFFFFFFFF)
    //
    // Extract just the filename from pased input
    //
    char* searchingFile;
    unsigned int ClusNum;
    searchingFile = strtok(input," ");
    searchingFile = strtok(NULL, " ");
 
    ClusNum = FindFirstFileCluster(searchingFile);// Find The File
    if (ClusNum == 0) // If after looking, File not found. Return
    {
        printf("File Not found\n");
        return;
    }
    // If file was found, open a new file in same dir as program
    // and append the data from each sector into that file
    FILE * fp = NULL;
    fp = fopen(searchingFile, "wb");

    while (ClusNum < 0x0FFFFFF8) // If Next Cluster = EOF, break
    {
        // Read and Display Cluster DATA
        fseek(in,(DATAStart + (ClusNum-2)*bpb.SecPerClus)*512,SEEK_SET);
        for(int i = bpb.SecPerClus; i > 0; i--)
        {
            fread(&SectorBuffer,sizeof(SectorBuffer),1,in);
            fwrite(SectorBuffer,1,sizeof(SectorBuffer),fp);
        }
            // After reading a full Cluster, Check the FAT for the next cluster
        fseek(in,FATStart*512 + ClusNum*4,SEEK_SET);
        fread(&ClusNum,sizeof(ClusNum),1,in);
    }
    fclose(fp);
}

unsigned int FindFirstFileCluster(char* searchingFile)
{
    // Read ROOT DIR and Compare the file names of each entry to the string passed by searchingFile.
    // When a match is found, get the data cluster the file starts in. and return
    // If no match is found, return 0
    // 
    unsigned int ClusNum = bpb.RootClus; // Set initial Cluster to the Root Cluster
    while (ClusNum < 0x0FFFFFF8) // If Next Cluster = EOF, break
    {
        // Read Every DIR Entry and compare File Names
        fseek(in,(DATAStart + (ClusNum-2)*bpb.SecPerClus)*512,SEEK_SET);
        for(int i = bpb.SecPerClus; i > 0; i--)
        {
            fread(dir,sizeof(dir),1,in);
            for(int j = 0; j < 16; j++)
            {
                if (strncmp(dir[j].s.Name, searchingFile,8) == 0)   // Compare Short Name 
                {
                    // If matching string is found, calculate inital data cluster
                    ClusNum = dir[j].s.FstClusHI << 16 | dir[j].s.FstClusLO;
                    return ClusNum;
                }
            }
        }
        // After reading a Cluster of ROOT, Check the FAT for the next cluster
        fseek(in,FATStart*512 + ClusNum*4,SEEK_SET);
        fread(&ClusNum,sizeof(ClusNum),1,in);
    }
    return 0;
}


void DisplayRoot()
{
    // Display each entry in ROOT's date and time of creation, size, 
    // and Short + Long FileNames
    //
    // Set initial Cluster to the Root Cluster
    //
    unsigned int ClusNum = bpb.RootClus; 
    while (ClusNum < 0x0FFFFFF8) // If Next Cluster = EOF, break
    {
        // Read and Display Cluster DATA
        fseek(in,(DATAStart + (ClusNum-2)*bpb.SecPerClus)*512,SEEK_SET);
        for(int i = bpb.SecPerClus; i > 0; i--)
        {
            // For each sector of the cluster, read it in
            // and display it
            fread(dir,sizeof(dir),1,in);
            DisplayDIRSector(dir);
        }
        // After reading a full Cluster, Check the FAT for the next cluster
        fseek(in,FATStart*512 + ClusNum*4,SEEK_SET);
        fread(&ClusNum,sizeof(ClusNum),1,in);
    }
}

void DisplayDIRSector(DIRUnion dir[16])
{
    // Displays Sector of DIR, reading data from an
    // array of 32 bit File Entries that can either be structured:
    //      dir[].s or dir[].l
    // depending on if its a short or long named directory
    //
    char16_t longName16[255] = {};
    for(int i = 0; i < 16; i++)
    { 
        if(dir[i].s.Name[0] == 0xE5) // If first bit is 0xE5, the rest of the Sector is blank
            continue;

        if(dir[i].s.Name[0] == 0x00) // If first bit == 0x00, then its empty and doesn't need to be displayed
            break;

        if(dir[i].s.Attr == 0x02)   // Skip Hidden files
            continue;
        
        if(dir[i].s.Attr == 0x04)   // Skip System files
            continue;

        if(dir[i].s.Attr == 0x08)   // Skip Volume ID
            continue;

        if(dir[i].s.Attr == 0x10)   // Skip directory listings
            continue;

        if(dir[i].s.Attr == 0x20)   // Skip Archives 
            continue;

        memset(longName16,0,sizeof(longName16)); // Clear longName string
        if(dir[i].l.Attr == 0x0F) // Long name Entry
        {
            int longNameEntries = (dir[i].l.Ord & 0x3F);
            int index = 0;
            // For each dir entry that corresponds to this
            // set of long file name dirs, Go through each
            // and copy the 16-bit characters into longName16[];
            //
            for(int j = longNameEntries-1; j >= 0; j--)
            {
                longName16[index] = *dir[i+j].l.Name1;
                index += 5;
                longName16[index] = *dir[i+j].l.Name2;
                index += 6;
                longName16[index] = *dir[i+j].l.Name3;
                index += 2;
            }
            i += longNameEntries;
        }
        // Extract Usable Values from WrtDate and WrtTime
        //
        int month, day, year ,hours, minutes;
        char *ToD = "AM";
        day = dir[i].s.WrtDate & 0x1F;
        month = (dir[i].s.WrtDate & 0x1E0) >> 5;
        year = (dir[i].s.WrtDate & 0xFE00 >> 9) + 1980;
        minutes = (dir[i].s.WrtTime & 0x3F0) >> 5;
        hours = (dir[i].s.WrtTime & 0x7C00) >> 11;
        if (hours > 12)
        {
            hours -=12;
            ToD = "PM";
        }
        // Print creation date/time, file size, and file name to screen
        printf("%02i/%02i/%4i   %02i:%02i %s  %'10i  %.8s.%.3s \n", 
                month, day ,year,
                hours, minutes,ToD,
                dir[i].s.FileSize, dir[i].s.Name, dir[i].s.Ext); // longName
    }
}

void DisplaySector(unsigned char* sector)
{
    // Display the contents of sector[] as 16 rows of 32 bytes each
    // Each row is shown as 16 bytes, a "-", and then 16 more bytes
    // The left part of the display is in hex; the right part is in
    // ASCII (if the character is printable; otherwise we display ".".
    //
    for (int i = 0; i < 16; i++)                    // for each of 16 rows
    { //
        for (int j = 0; j < 32; j++)                // for each of 32 values per row
        { //
            printf("%02X ", sector[i * 32 + j]);    // Display the byte in hex
            if (j % 32 == 15) printf("- ");         // At the half-way point? Show divider
        }
        printf(" ");                                // Separator space between hex & ASCII
        for (int j = 0; j < 32; j++)                // For each of those same 32 bytes
        { //
            if (j == 16) printf(" ");                   // Halfway? Add a space for clarity
            int c = (unsigned int)sector[i * 32 + j];   // What IS this char's ASCII value?
            if (c >= 32 && c <= 127) printf("%1c", c);  // IF printable, display it
            else printf(".");                       // Otherwise, display a "."
        }                                           //
        printf("\n");                               // Thats the end of this row
    }
}


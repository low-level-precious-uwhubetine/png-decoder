
#include <stdio.h>
#include <malloc.h>
#include <math.h>

struct Chunk
{
    int length;
    char Type[4];
    char Data;
}__attribute__((packed));

struct IHDRChunk
{
    unsigned int Width;
    unsigned int Height;
    unsigned char BitDepth;
    unsigned char ColorType;
    unsigned char CompressionMethod;
    unsigned char FilterMethod;
    unsigned char InterlaceMethod;
}__attribute__((packed));

struct pHYsChunk
{
    unsigned int XPixels;
    unsigned int YPixels;
    unsigned char UnitSpecifier;
}__attribute__((packed));

struct CompressedDataHeader
{
    unsigned char CompressionMethod : 4;
    unsigned char WindowSize : 4;
    unsigned char CheckBits : 5;
    unsigned char PresetDictionary : 1;
    unsigned char CompressionLevel : 2;
}__attribute__((packed));

struct CompressedBlockHeader
{
    unsigned char Final : 1;
    unsigned char Type : 2;
    unsigned char Rsvd : 5; 
}__attribute__((packed));

struct DynamicHuffmanCodeBlock
{
    unsigned char Final : 1;
    unsigned char Type : 2;
    unsigned char Literals : 5;  
}__attribute__((packed));

struct DynamicHuffmanCodeLengths
{
    unsigned char Final : 1;
    unsigned char Type : 2;
    unsigned char Literals : 5; 
    unsigned short Distances : 5; 
    unsigned short Lengths : 4; 
    unsigned short Rsvd : 7; 
}__attribute__((packed));

unsigned int GetBitsFromStream(unsigned char* stream, int pos, int numBits)
{
    if (numBits > 32) 
    {
        printf("NUMBER OF BITS REQUESTED EXCEEDS MAXIMUM!\n");
        return 0;
    }
    int result = 0;
    int byteOffset = pos / 8;
    int bitOffset = pos % 8;
    int cnt = bitOffset;
    int tmp = 0;

    for (tmp = 0; tmp < numBits; tmp++)
    {
        unsigned char byteData = stream[byteOffset];
        unsigned char bit = (byteData & (int)(pow (2, cnt))) >> cnt;
        result |= ((unsigned int)bit << tmp);
        cnt++;
        if (cnt >= 8)
        {
            cnt = 0;
            byteOffset++;
        }
    }
    return result;
}

void GenerateHuffmanCodes(int numCodes, int* codeLengths, int* codes)
{
    int huffmanCodeCounter = 0;
    int codeLengthCounter = 1;

    int index1 = 0; int pos = 0;
    for (index1; index1 < numCodes; index1++)
    {
        if (codeLengths[index1] > 0) break; 
    }

    for (index1; index1 < numCodes; index1++)
    {
        if (codeLengths[index1] == codeLengthCounter)
        {
            codes[index1] = huffmanCodeCounter;
            huffmanCodeCounter++;
        }
        else
        {
            huffmanCodeCounter <<= 1;
            codeLengthCounter++;
            index1--;
        }
    }
    
}



void DecodeHuffmanCodes(int* sortedValues, int* sortedCodeLengths, int* huffmanCodes, int numCodes, unsigned char* stream, int pos)
{
    int codeLength = 1;
    int cpos = pos;
    int maxCodeLength = sortedCodeLengths[numCodes - 1];
    unsigned int val = 0;
    int matchesFound = 0;
    int xpos = 0;
    
    while(1)
    {
        printf("CODELENGTH: %d\n", codeLength);
        unsigned int tval = GetBitsFromStream(stream, cpos, 1);
        cpos++;
        printf("Got bit %d\n", tval);
        tval <<= xpos;
        val |= tval;

        printf("STREAM BITS: 0x%x\n", val);
        int tmp;
        int done = 0;
        for (tmp = 0; tmp < numCodes; tmp++)
        {
            if (codeLength == sortedCodeLengths[tmp])
            {
                printf("CL Available\n");
                int tmp2;
                for (tmp2 = 0; tmp2 < numCodes; tmp2++)
                {
                    if (sortedCodeLengths[tmp2] == codeLength && val == huffmanCodes[tmp2])
                    {
                        if (sortedValues[tmp2] == 16)
                        {
                            unsigned int tmp3 = GetBitsFromStream(stream, cpos, 2) + 3;
                            cpos += 2;
                            matchesFound += tmp3;
                            printf("Found added match: %d %d %d\n", huffmanCodes[tmp2], sortedValues[tmp2], sortedCodeLengths[tmp2]);
                            done = 1;
                        }
                        else
                        {
                            printf("Found match: %d %d %d\n", huffmanCodes[tmp2], sortedValues[tmp2], sortedCodeLengths[tmp2]);
                            done = 1;
                            matchesFound++;
                        }
                    }
                    if (done == 1) break;
                }
            }
            if (done == 1) break;
        }
        codeLength++;
        xpos++;
        if (done == 1)
        {
            printf("\n\n");
            codeLength = 1;
            val = 0;
            xpos = 0;
        }
        if (codeLength >= maxCodeLength) {printf("Matches Found: %d\n", matchesFound);break;}
    }
}

void Sort(int* arr, int cnt, int* out)
{
    int index;
    int pos = 0;
    for (index = 0; index <= 7; index++)
    {
        int index1 = 0;
        for (index1; index1 < cnt; index1++)
        {
            if (arr[index1] == index) 
            {
                out[pos] = index;
                pos++;
            }
        }
    }
}

unsigned int bigE(unsigned int value)
{
	return ((value << 24) & 0xff000000)
             | ((value << 8) & 0x00ff0000) 
             | ((value >> 8) & 0x0000ff00)  
             | ((value >> 24) & 0x000000ff);
}

void processIHDRChunk(unsigned char* data)
{
    struct IHDRChunk* ihdr = (struct IHDRChunk*)data;
    printf("Processing IHDR Chunk...\n");
    printf("Image Width: %dpx\n", bigE(ihdr->Width));
    printf("Image Height: %dpx\n", bigE(ihdr->Height));
    printf("Bit depth: %d\n", ihdr->BitDepth);
    char* colorTypeStr;
    switch (ihdr->ColorType)
    {
        case 0:
            colorTypeStr = "Grayscale (0)";
            break;
        case 2:
            colorTypeStr = "RGB Triple (2)";
            break;
        case 3:
            colorTypeStr = "Palette (3)";
            break;
        case 4:
            colorTypeStr = "Grayscale with alpha channel (4)";
            break;
        case 6:
            colorTypeStr = "RGB with alpha channel (6)";
            break;
        default:
            colorTypeStr = "Invalid color type";
            break;
    }
    printf("Color Type: %s\n", colorTypeStr);   
    printf("Compression Method: %d\n", ihdr->CompressionMethod);
    printf("Filter Method: %d\n", ihdr->FilterMethod);
    printf("Interlace Method: %d\n\n", ihdr->InterlaceMethod);
}

unsigned long long CrcTable[256];

void MakeTable()
{
    int i, j;
    for (i = 0; i < 256; i++)
    {
        CrcTable[i] = i;
        for (j = 0; j < 8; j++)
        {
            if ((CrcTable[i] & 0x1) == 0)
                CrcTable[i] >>= 1;
            else
                CrcTable[i] = 0xEDB88320L ^ (CrcTable[i] >> 1);
        }
    }
}

unsigned long long CRCRegister;
void CrcByte(unsigned char data)
{
    unsigned int index = (CRCRegister ^ data) & 0xff;
    CRCRegister = CrcTable[index] ^ ((CRCRegister >> 8) & 0x00ffffff);
}

int main(int argc, char* argv[])
{
    FILE* image;
    int pos = 0;

    if (argc != 2) { printf("Usage: png [image name]\nError: Invalid number of arguments.\n\n"); return -1;}

    if (image = fopen(argv[1], "r+b"))
    {
        printf("Loading Image...\n");
        MakeTable();

        char* buffer  = (char*)malloc(200*4096);
        
        pos = 8;
        int idat_started = 0;

        while(1)
        {
            fseek(image, pos, SEEK_SET);
            fread(buffer, 8, 1, image);
            struct Chunk* chunk = (struct Chunk*)buffer;
            printf("Chunk size: %d byte(s)\n", bigE(chunk->length));
            printf("Chunk Name: %s\n", chunk->Type);

            if (strcmp(chunk->Type, "IEND") == 0) break;
            pos += 4;
            //calculate CRC;
            CRCRegister = 0xffffffffl;
            int i; 
            unsigned char* chunk_data = (unsigned char*)malloc(bigE(chunk->length) + 4);
            
            fseek(image, pos, SEEK_SET);
            fread(chunk_data, bigE(chunk->length) + 4, 1, image);
            for (i = 0; i < bigE(chunk->length) + 4; i++) CrcByte(chunk_data[i]);
            CRCRegister = ~CRCRegister;
            
            pos += 4 + bigE(chunk->length);
            
            printf("Calculated CRC: %x\n", CRCRegister);

            unsigned int crc_val[1];

            fseek(image, pos, SEEK_SET);
            fread(crc_val, 4, 1, image);

            printf("Chunk CRC: %x\n", bigE(crc_val[0]));
            if ((unsigned int)CRCRegister == (unsigned int)bigE(crc_val[0]))
            {
                printf("REMARKS: Healthy Chunk.\n\n");
                pos -= bigE(chunk->length);

                if (strcmp(chunk->Type, "IHDR") == 0)
                {
                    processIHDRChunk(chunk_data+4);
                }
                else if (strcmp(chunk->Type, "pHYs") == 0)
                {
                    printf("Processing pHYS chunk...\n");
                    struct pHYsChunk* phys = (struct pHYsChunk*)(chunk_data + 4);
                    printf("Pixels per unit X: %d\n", bigE(phys->XPixels));
                    printf("Pixels per unit Y: %d\n", bigE(phys->YPixels));
                    printf("Unit Specifier: %d\n\n", phys->UnitSpecifier);
                }
                else if (strcmp(chunk->Type, "IDAT") == 0)
                {
                    printf("Processing IDAT chunk...\n");
                    if (idat_started == 0) 
                    {
                        idat_started = 1;
                        struct CompressedDataHeader* cdata = (struct CompressedDataHeader*)(chunk_data+4);
                        printf("Compression Method: %d\n", cdata->CompressionMethod);
                        printf("Window Size: %d\n", cdata->WindowSize);
                        printf("Check bits: %d\n", cdata->CheckBits);
                        printf("Preset Dictionary: %d\n", cdata->PresetDictionary);
                        printf("Compression Level: %d\n\n", cdata->CompressionLevel);
                    }

                    unsigned char* CompressedBlock = (unsigned char*)(chunk_data+4+2);
                    printf("Compressed block : %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n\n", 
                                CompressedBlock[0], CompressedBlock[1], CompressedBlock[2], CompressedBlock[3], 
                                CompressedBlock[4], CompressedBlock[5], CompressedBlock[6], CompressedBlock[7], 
                                CompressedBlock[8], CompressedBlock[9], CompressedBlock[10], CompressedBlock[11], 
                                CompressedBlock[12], CompressedBlock[13], CompressedBlock[14], CompressedBlock[15]);
                    
                    struct CompressedBlockHeader* cbh = (struct CompressedBlockHeader*)(chunk_data+4+2);
                    printf("Final: %d\n", cbh->Final);
                    printf("Type: %d\n", cbh->Type);
                    
                    if (cbh->Final == 1)
                        printf("Final Data block.\n");
                    else printf("More Data blocks after this...\n");

                    printf("Processing Data block...\n");
                    switch (cbh->Type)
                    {
                        case 0x0:   printf("Uncompressed block.\n"); break;
                        case 0x1:   printf("Compressed block with fixed huffman codes.\n"); break;
                        case 0x2:   
                        {
                            printf("Compressed block with dynamic huffman codes.\n");

                            unsigned char* bitStream = (unsigned char*)(chunk_data+4+2);

                            unsigned int Literals = GetBitsFromStream(bitStream, 3, 5);
                            unsigned int Distances = GetBitsFromStream(bitStream, 8, 5);
                            unsigned int Lengths = GetBitsFromStream(bitStream, 13, 4);

                            printf("Literals: %d\n", Literals);
                            printf("Distances: %d\n", Distances);
                            printf("Lengths: %d\n", Lengths);
                            
                            int numCodeLengths = Lengths + 4;
                            int cnt = 0;

                            int* codeLengthArray = (int*)malloc(numCodeLengths*sizeof(int));
                            int* codeLengthArray1 = (int*)malloc(numCodeLengths*sizeof(int));
                            int* HuffmanCodes = (int*)malloc(numCodeLengths*sizeof(int));
                            int* arrangedValues = (int*)malloc(19*sizeof(int));
                            int* tick = (int*)malloc(19*sizeof(int));

                            for(cnt = 0; cnt < 19; cnt++) {tick[cnt] = 0; arrangedValues[cnt] = 0;};

                            int values[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

                            printf("CodeLengths: ");
                            for (cnt = 0; cnt < numCodeLengths; cnt++)
                            {
                                unsigned int val = GetBitsFromStream(bitStream, 17 + (cnt*3), 3);
                                codeLengthArray[cnt] = (unsigned char)val;
                                printf("%d ", codeLengthArray[cnt]);
                            }
                            printf("\n");
                            
                            Sort(codeLengthArray, numCodeLengths, codeLengthArray1);
                            for (cnt = 0; cnt < numCodeLengths; cnt++)
                            {
                                printf("%d ", codeLengthArray1[cnt]);
                            }
                            printf("\n");

                            GenerateHuffmanCodes(numCodeLengths, codeLengthArray1, HuffmanCodes);
                            printf("Generated Huffman Codes: \n");
                            for (cnt = 0; cnt < numCodeLengths; cnt++)
                            {
                                printf("%x\n", HuffmanCodes[cnt]);
                            }
                            

                            for (cnt = 0; cnt < numCodeLengths; cnt++)
                            {
                                int cnt1;
                                for (cnt1 = 0; cnt1 < numCodeLengths; cnt1++)
                                {
                                    if (codeLengthArray1[cnt1] == codeLengthArray[cnt] && tick[cnt1] == 0)
                                    {
                                        tick[cnt1] = 1;
                                        arrangedValues[cnt1] = values[cnt];
                                        break;
                                    }
                                }
                            }

                            printf("Sorted VALUES: ");
                            for (cnt = 0; cnt < numCodeLengths; cnt++)
                            {
                                printf("%d ", arrangedValues[cnt]);
                            }
                            printf("\n");
                            
                            
                            int pos = 17 + (numCodeLengths*3);
                            for (cnt = 0; cnt < 50; cnt++)
                            {
                                unsigned int val = GetBitsFromStream(bitStream, pos, 1);
                                pos++;
                                printf("%d", val);
                            }
                            printf("\n");

                            pos = 17 + (numCodeLengths*3);
                            DecodeHuffmanCodes(arrangedValues, codeLengthArray1, HuffmanCodes, numCodeLengths, bitStream, pos);


                            break;
                        }
                        default:    printf("Invalid block!\n"); break;
                    }

                    printf("\n\n");   
                }
                
                
                pos += bigE(chunk->length);
            }
            else printf("REMARKS: Bad Chunk.\n\n");

            pos += 4;
            free(chunk_data);
            
        }

        free(buffer);
    }
    else
    {
        printf("The specified image was not found\n\n");
        return -1;
    }

    fclose(image);

    return 0;
}
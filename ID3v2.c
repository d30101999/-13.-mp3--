#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
typedef enum { false, true } bool;      /// created a new datatype for boolean

struct Frame {
    char frameID[4];                    ///first c4 bytes contains the name of the frame
    unsigned char sizeBytes[4];         /// size of meta info(name of artist,everthing...
    char flagsBytes[2];
};

struct MainHeader {
    char ID3[3];
    char versionBytes[2];
    char flagByte;
    char sizeBytes[4];
};

int getFrameSize(char*);
void showFrames(FILE*);  /// to display current tag values
void showFrame(FILE*, char*); /// shows specific frame
void setVal(FILE*, FILE*, char*, char*);
void setFrameSize(char*, int);
bool isEqual(char*, char*);   /// to check of the strings are same or not

int main(int argc, char* argv[]) {
    char com[3][20];
    char arg[3][50];
    int k = 0;
    int m = 0;
    int j = 0;

    for (int i = 1; i < argc; i++) {
        k = 0;
        for (j = 0; argv[i][j] != '=' && argv[i][j] != '\0'; j++) {
            com[m][k++] = argv[i][j];
        }
        com[m][k] = '\0';

        j++;
        k = 0;
        while (argv[i][j] != '\0') {
            arg[m][k++] = argv[i][j++];
        }
        arg[m][k] = '\0';
        m++;
    }

    FILE* mp3;
    mp3 = fopen(arg[0], "r+b");
    FILE* output;
    output = fopen("theout.mp3", "r+b");
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(com[i], "--show") == 0) {
            showFrames(mp3);
    }
        else if (strcmp(com[i], "--get") == 0) {
            showFrame(mp3, arg[i]);
        }
        else if (strcmp(com[i], "--set") == 0) {
            setVal(mp3, output, arg[i], arg[i + 1]);
        }
    }

    return 0;
}

int getFrameSize(char* byteArr) {
    int size = 0;
    unsigned char ch;
    for (int i = 0; i < 4; i++) {
        ch = byteArr[i];    /// so that what is in byte array put it in character
        size += ch * pow(2, 7 * (3 - i));
    }
    return size;
}

void showFrames(FILE* f) {
    fseek(f, 0, SEEK_SET);
    struct MainHeader header;
    fread(&header, sizeof(header), 1, f);
    long long tagSize = getFrameSize(header.sizeBytes);
    struct Frame frame;
    long long counter = 0;
    while (counter <= tagSize) {
        fread(&frame, sizeof(frame), 1, f);
        long long frameSize = getFrameSize(frame.sizeBytes);
        counter += 10 + frameSize;
        if (frame.frameID[0] == 'T') {
            printf("%s ", frame.frameID);
            while(frameSize--) {
                char ch = fgetc(f);
                if (ch >= 32 && ch <= 126)
                    printf("%c", ch);
            }
            printf("\n");
        }
        else if (isEqual(frame.frameID, "COMM") == true) {  /// comment has 4 byte[=  so we are going to subtract them
            printf("%s ", frame.frameID);
            fseek(f, 4, SEEK_CUR);
            frameSize -= 4;
            while(frameSize--) {
                char ch = fgetc(f);
                if (ch >= 32 && ch <= 126)
                    printf("%c", ch);
            }
            printf("\n");
        }
    }
}

void showFrame(FILE* f, char* frameID) {        /// we need to read the id3 header
    fseek(f, 0, SEEK_SET);
    struct MainHeader header;
    fread(&header, sizeof(header), 1, f);
    int fileSize = getFrameSize(header.sizeBytes);
    struct Frame frame;
    int counter = 0;
    while (counter <= fileSize) {
        fread(&frame, sizeof(frame), 1, f);
        int frameSize = getFrameSize(frame.sizeBytes);
        counter += 10 + frameSize;    /// because the header is 10 bytes
        if (isEqual(frame.frameID, frameID) == true) {
            printf("%s ", frame.frameID);
            while(frameSize--) {
                char ch = fgetc(f);
                if (ch >= 32 && ch <= 126)
                    printf("%c", ch);
            }
        }
        else {
            fseek(f, getFrameSize(frame.sizeBytes), SEEK_CUR);
        }
    }
}

bool isEqual(char* str1, char* str2) {
    for (int i = 0; i < sizeof(str1); i++) {
        if (str1[i] != str2[i]) {
            return false;
        }
    }
    return true;
}

void setVal(FILE* fin, FILE* fout, char* someFrame, char* val) {
    fseek(fin, 0, SEEK_SET);
    struct MainHeader header;
    fread(&header, sizeof(header), 1, fin);
    fwrite(&header, sizeof(header), 1, fout);
    int valSize = strlen(val);
    int fileSize = getFrameSize(header.sizeBytes);              ///filesize ==value sizze// total id3 tag size
    struct Frame frame;
    int counter = 0;
    int frameSize = 0;
    while (counter <= fileSize) {
        fread(&frame, sizeof(frame), 1, fin);
        frameSize = getFrameSize(frame.sizeBytes);
        counter += 10 + frameSize;
        if (isEqual(frame.frameID, someFrame) == false && frameSize >= 0 && frameSize <= 100000) {
            fwrite(&frame, 1, sizeof(frame), fout);
            char info[frameSize];
            fread(&info, frameSize, 1, fin);
            fwrite(info, frameSize, 1, fout);
        }
        else if (isEqual(frame.frameID, someFrame) == true && frameSize >= 0){
            int oldFrameSize = frameSize;
            setFrameSize(header.sizeBytes, getFrameSize(header.sizeBytes) - frameSize + valSize);
            int curPos = ftell(fout);
            fseek(fout, 6, SEEK_SET);
            fwrite(header.sizeBytes, sizeof(header.sizeBytes), 1, fout);
            fseek(fout, curPos, SEEK_SET);
            if (isEqual(frame.frameID, "COMM") == true)
                setFrameSize(frame.sizeBytes, valSize + 4);
            else if (isEqual(frame.frameID, "TXXX") == true)
                setFrameSize(frame.sizeBytes, valSize + 2);
            else
                setFrameSize(frame.sizeBytes, valSize);
            int newFrameSize = getFrameSize(frame.sizeBytes);
            fwrite(&frame, sizeof(frame), 1, fout);
            if (isEqual(frame.frameID, "COMM") == true) {
                for (int i = 0; i < 4; i++) {
                    fputc(fgetc(fin), fout);
                }
                fseek(fin, oldFrameSize - 4, SEEK_CUR);
            }
            else if (isEqual(frame.frameID, "TXXX") == true) {
                for (int i = 0; i < 2; i++) {
                    fputc(fgetc(fin), fout);
                }
                fseek(fin, oldFrameSize - 2, SEEK_CUR);
            }
            else
                fseek(fin, oldFrameSize, SEEK_CUR);
            fwrite(val, valSize, 1, fout);
        }
    }
    int ch;
    while ((ch = fgetc(fin)) != EOF) {
        fputc(ch, fout);
    }
    fseek(fout, 0, SEEK_SET);
    fseek(fin , 0, SEEK_SET);
    while ((ch = getc(fout)) != EOF) {
        fputc(ch, fin);
    }
}

void setFrameSize(char* frameSize, int valSize) {
    for (int i = 0; i < 4; i++) {
        frameSize[i] = valSize / pow(2, 7 * (3 - i));
        valSize -= frameSize[i] * pow(2, 7 * (3 - i));
    }
}

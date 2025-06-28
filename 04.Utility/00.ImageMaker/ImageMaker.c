#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTEOFSECTOR 512

int AdjustInSectorSize(int iFd, int iSourceSize);
void WriteKernelInformation(int iTargetFd, int iKernelSectorCount, int iKernel32SectorCount);
int CopyFile(int iSourceFd, int iTargetFd);

int main(int argc, char *argv[]) {
    int iSourceFd;
    int iTargetFd;
    int iSourceSize;

    if (argc < 4) {
        fprintf(stderr, "[ERROR] ImageMaker BootLoader.bin Kernel32.bin Kernel64.bin\n");
        exit(1);
    }

    // Open Disk.img
    if ((iTargetFd = open("Disk.img", O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1) {
        fprintf(stderr, "[ERROR] Disk.img open fail.\n");
        exit(1);
    }

    // BootLoader
    printf("[INFO] Copy BootLoader to Disk.img\n");
    if ((iSourceFd = open(argv[1], O_RDONLY)) == -1) {
        fprintf(stderr, "[ERROR] %s open fail\n", argv[1]);
        exit(1);
    }

    iSourceSize = CopyFile(iSourceFd, iTargetFd);
    close(iSourceFd);

    int iBootLoaderSize = AdjustInSectorSize(iTargetFd, iSourceSize);
    printf("[INFO] %s size = [%d] and sector count = [%d]\n", argv[1], iSourceSize, iBootLoaderSize);

    // Kernel32
    printf("[INFO] Copy Kernel32 to Disk.img\n");
    if ((iSourceFd = open(argv[2], O_RDONLY)) == -1) {
        fprintf(stderr, "[ERROR] %s open fail\n", argv[2]);
        exit(1);
    }

    iSourceSize = CopyFile(iSourceFd, iTargetFd);
    close(iSourceFd);

    int iKernel32SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);
    printf("[INFO] %s size = [%d] and sector count = [%d]\n", argv[2], iSourceSize, iKernel32SectorCount);

    // Kernel64
    printf("[INFO] Copy Kernel64 to Disk.img\n");
    if ((iSourceFd = open(argv[3], O_RDONLY)) == -1) {
        fprintf(stderr, "[ERROR] %s open fail\n", argv[3]);
        exit(1);
    }

    iSourceSize = CopyFile(iSourceFd, iTargetFd);
    close(iSourceFd);

    int iKernel64SectorCount = AdjustInSectorSize(iTargetFd, iSourceSize);
    printf("[INFO] %s size = [%d] and sector count = [%d]\n", argv[3], iSourceSize, iKernel64SectorCount);


    // Update SectorSize
    printf("[INFO] Start to write kernel information\n");
    WriteKernelInformation(iTargetFd, iKernel32SectorCount + iKernel64SectorCount, iKernel32SectorCount);
    printf("[INFO] Image file create complete\n");

    close(iTargetFd);
    return 0;
}

int AdjustInSectorSize(const int iFd, int iSourceSize) {
    int iAdjustSizeToSector = iSourceSize % BYTEOFSECTOR;
    char cCh = 0x00;

    if (iAdjustSizeToSector != 0) {
        iAdjustSizeToSector = 512 - iAdjustSizeToSector;
        printf("[INFO] File size [%lu] and fill [%u] byte\n", iSourceSize, iAdjustSizeToSector);

        for (int i = 0; i < iAdjustSizeToSector; i++) {
            write(iFd, &cCh, 1);
        }
    } else {
        printf("[INFO] File size is aligned 512 byte\n");
    }

    int iSectorCount = (iSourceSize + iAdjustSizeToSector) / BYTEOFSECTOR;
    return iSectorCount;
}

void WriteKernelInformation(int iTargetFd, int iTotalKernelSectorCount, int iKernel32SectorCount) {
    unsigned short usData;
    long lPosition;

    lPosition = lseek(iTargetFd, (off_t)5, SEEK_SET);
    if (lPosition == -1) {
        fprintf(stderr, "lseek fail. Return value = %d, errno = %d, %d\n", lPosition, errno, SEEK_SET);
        exit(1);
    }

    usData = (unsigned short)iTotalKernelSectorCount;
    write(iTargetFd, &usData, 2);
    usData = (unsigned short)iKernel32SectorCount;
    write(iTargetFd, &usData, 2);

    printf("[INFO] Total sector count except boot loader [%d]\n", iTotalKernelSectorCount);
    printf("[INFO] Total sector count of protected mode kernel [%d]\n", iKernel32SectorCount);
}

int CopyFile(int iSourceFd, int iTargetFd) {
    int iSourceFileSize = 0;
    
    while (1) {
        char vcBuffer[BYTEOFSECTOR];
        int iRead = read(iSourceFd, vcBuffer, sizeof(vcBuffer));
        int iWrite = write(iTargetFd, vcBuffer, iRead);

        if (iRead != iWrite) {
            fprintf(stderr, "[ERROR] iRead != iWrite\n");
            exit(1);
        }
        iSourceFileSize += iRead;

        if (iRead != sizeof(vcBuffer)) {
            break;
        }
    }

    return iSourceFileSize;
}

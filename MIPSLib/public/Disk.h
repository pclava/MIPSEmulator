#ifndef MIPSEMULATOR_DISK_H
#define MIPSEMULATOR_DISK_H

#include "Device.h"
#include "utils.h"
#include <cstdio>

enum DiskOp {
    READ = 1,
    WRITE,
};

enum DiskStatus {
    DSK_OK,
    DSK_BAD_SECTOR,
    DSK_BAD_ADDR,
    DSK_BAD_OP,
    DSK_BAD,
};

class MIPS::Disk : public Device {
private:
    const std::string path;
    std::FILE *diskimg;

    Word disk_sector;
    Word disk_addr;
    Byte disk_ctrl;
    Byte disk_status;

    static constexpr Word DISK_SECTOR = 0xfffffa04;
    static constexpr Word DISK_ADDR = 0xfffffa08;
    static constexpr Word DISK_CTRL = 0xfffffa0c;
    static constexpr Word DISK_STATUS = 0xfffffa0d;

    static constexpr Word SECTOR_SIZE = 512; // bytes in a disk sector/block
    static constexpr Word NBLOCKS = 4096;  // number of blocks in the file system; 2 mb

    bool disk_seek(Word sector);

    void disk_read(Word sector, Word addr, Memory &mem);

    void disk_write(Word sector, Word addr, Memory &mem);

public:
    explicit Disk(const std::string &path);

    void signal(Word addr, Word val, size_t bytes, Memory &mem) override;

    [[nodiscard]] Word query(Word addr, size_t bytes) override;
};

#endif //MIPSEMULATOR_DISK_H
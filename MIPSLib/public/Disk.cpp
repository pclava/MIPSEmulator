#include "Disk.h"

using namespace MIPS;

Disk::Disk(const std::string &path) :
    Device(DISK_START, DISK_LIMIT),
    path(path),
    disk_sector(0),
    disk_addr(0),
    disk_ctrl(0),
    disk_status(0)
{
    diskimg = std::fopen(path.c_str(), "rb+");
    if (diskimg == nullptr) {
        throw std::runtime_error("Could not open disk image");
    }
}

bool Disk::disk_seek(Word sector) {
    // Check bounds
    if (sector >= NBLOCKS) {
        disk_status = DSK_BAD_SECTOR;
        return false;
    }

    // Move file ptr
    Word p = sector * SECTOR_SIZE;
    if (fseek(diskimg, p, SEEK_SET) != 0) {
        disk_status = DSK_BAD_SECTOR;
        return false;
    }

    return true;
}

void Disk::disk_read(Word sector, Word addr, Memory &mem) {
    // Seek
    if (!disk_seek(sector)) return;

    // Read from disk into buffer
    Byte buf[SECTOR_SIZE];
    if (fread(buf, 1, SECTOR_SIZE, diskimg) != SECTOR_SIZE) {
        disk_status = DSK_BAD_SECTOR;
        return;
    }

    // Write from buffer into memory
    try {
        mem.writeN(addr, SECTOR_SIZE, buf);
    } catch (std::exception &) {
        disk_status = DSK_BAD_ADDR;
        return;
    }

}

void Disk::disk_write(Word sector, Word addr, Memory &mem) {
    // Seek
    if (!disk_seek(sector)) return;

    // Read from memory into buffer
    Byte buf[SECTOR_SIZE];
    try {
        mem.readN(addr, SECTOR_SIZE, buf);
    } catch (std::exception &) {
        disk_status = DSK_BAD_ADDR;
        return;
    }

    // Write from buffer into disk
    if (fwrite(buf, 1, SECTOR_SIZE, diskimg) != SECTOR_SIZE) {
        disk_status = DSK_BAD_SECTOR;
    }
    if (fflush(diskimg) != 0) {
        disk_status = DSK_BAD;
    }

}

void Disk::signal(const Word addr, Word val, const size_t, Memory &mem) {

    switch (addr) {
        case DISK_CTRL:
            disk_ctrl = val;
            break;
        case DISK_SECTOR:
            disk_sector = val;
            return;
        case DISK_ADDR:
            disk_addr = val;
            return;
        default: ;
    }

    switch (static_cast<DiskOp>(val)) {
        case READ:
            disk_read(disk_sector, disk_addr, mem);
            return;
        case WRITE:
            disk_write(disk_sector, disk_addr, mem);
            return;
        default:
            disk_status = DSK_BAD_OP;
    }
}

Word Disk::query(const Word addr, const size_t bytes) {
    switch (addr) {
        case DISK_SECTOR:
            if (bytes != 4) {
                return -1;
            }
            return disk_sector;
        case DISK_ADDR:
            if (bytes != 4) {
                return -1;
            }
            return disk_addr;
        case DISK_CTRL:
            if (bytes != 1) {
                return -1;
            }
            return disk_ctrl;
        case DISK_STATUS:
            if (bytes != 1) {
                return -1;
            }
            return disk_status;
        default:
            return -1;
    }
}
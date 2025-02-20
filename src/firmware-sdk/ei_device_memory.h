/* The Clear BSD License
 *
 * Copyright (c) 2025 EdgeImpulse Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 *   * Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef EI_DEVICE_MEMORY_H
#define EI_DEVICE_MEMORY_H

#include <cstdint>
#include <cmath>
#include <cstring>

/**
 * @brief Interface class for all memory type storages in Edge Impulse compatible devices.
 * The memory should be organized in a blocks, because all EI sensor drivers depends on block oranization.
 * Additionaly, at least one or two block of memory at the beginning is used for device configuration
 * (see ei_device_info_lib.h)
 */
class EiDeviceMemory {
protected:
    /**
     * @brief Direct read from memory, should be implemented per device/memory type.
     * 
     * @param data pointer to buffer for data to be read
     * @param address absolute address in the memory (format and value depending on memory chip/type)
     * @param num_bytes number of bytes to be read @refitem data should be at leas num_bytes long
     * @return uint32_t number of bytes actually read. If differs from num_bytes, then some error occured.
     */
    virtual uint32_t read_data(uint8_t *data, uint32_t address, uint32_t num_bytes) = 0;

    /**
     * @brief Direct write to memory, should be implemented per device/memory type.
     * 
     * @param data pointer to bufer with data to write
     * @param address absolute address in the memory (format and value depending on memory chip/type)
     * @param num_bytes number of bytes to write
     * @return uint32_t number of bytes that has been written, if differs from num_bytes some error occured.
     */
    virtual uint32_t write_data(const uint8_t *data, uint32_t address, uint32_t num_bytes) = 0;

    /**
     * @brief Erase memory region
     * 
     * @param address absolute address in memory whwere the erase should begin. Typically block aligned.
     * @param num_bytes number of bytes to be erased, typically multiple of block size.
     * @return uint32_t numer of bytes that has been erased, if differes from num_bytes, then some oerror occured.
     */
    virtual uint32_t erase_data(uint32_t address, uint32_t num_bytes) = 0;

    /**
     * @brief size of device configuration block, device or even firmware specific.
     * 
     */
    const uint32_t config_size;
    /**
     * @brief number of blocks occupied by config. Typically 1, but depending on memory
     * type and config size, it can be multiple blocks.
     * 
     */
    const uint32_t used_blocks;
    /**
     * @brief total number of blocks in the memory
     * 
     */
    const uint32_t memory_blocks;
    /**
     * @brief total size of memory, typically integer multiply of blocks
     * 
     */
    const uint32_t memory_size;

public:
    /**
     * @brief size of the memory block in bytes
     */
    const uint32_t block_size;
    /**
     * @brief Erase time of a single block/sector/page in ms (miliseconds).
     * For RAM it can be set to 0 or 1.
     * For Flash memories take this value from datasheet or measure.
     */
    const uint32_t block_erase_time;

    /**
     * @brief Construct a new Ei Device Memory object, make sure to pass all necessary data
     * from derived class. Usually constructor of the derived class needs to get this data 
     * from user code or read from chip.
     * 
     * @param config_size see property description
     * @param erase_time see property description
     * @param memory_size see property description
     * @param block_size see property description
     */
    EiDeviceMemory(uint32_t config_size, uint32_t erase_time, uint32_t memory_size, uint32_t block_size):
        config_size(config_size),
        used_blocks((config_size < block_size) ? 1 : ceil(float(config_size) / block_size)),
        memory_blocks(memory_size / block_size),
        memory_size(memory_size),
        block_size(block_size),
        block_erase_time(erase_time)
    {

    }

    virtual uint32_t get_available_sample_blocks(void)
    {
        return memory_blocks - used_blocks;
    }

    virtual uint32_t get_available_sample_bytes(void)
    {
        return (memory_blocks - used_blocks) * block_size;
    }

    virtual bool save_config(const uint8_t *config, uint32_t config_size)
    {
        uint32_t used_bytes = this->used_blocks * this->block_size;
        if(this->erase_data(0, used_bytes) != used_bytes) {
            return false;
        }

        if(this->write_data(config, 0, config_size) != config_size) {
            return false;
        }

        return true;
    }

    virtual bool load_config(uint8_t *config, uint32_t config_size)
    {
        if(this->read_data(config, 0, config_size) != config_size) {
            return false;
        }
        return true;
    }

    virtual uint32_t read_sample_data(uint8_t *sample_data, uint32_t address, uint32_t sample_data_size) = 0;
    virtual uint32_t write_sample_data(const uint8_t *sample_data, uint32_t address, uint32_t sample_data_size) = 0;
    virtual uint32_t erase_sample_data(uint32_t address, uint32_t num_bytes) = 0;
};


template <int BLOCK_SIZE = 512, int MEMORY_BLOCKS = 8> class EiDeviceRAM : public EiDeviceMemory {

protected:
    uint8_t ram_memory[MEMORY_BLOCKS * BLOCK_SIZE];

    uint32_t read_data(uint8_t *data, uint32_t address, uint32_t num_bytes)
    {
        if (num_bytes > memory_size - address) {
            num_bytes = memory_size - address;
        }

        memcpy(data, &ram_memory[address], num_bytes);

        return num_bytes;
    }

    uint32_t write_data(const uint8_t *data, uint32_t address, uint32_t num_bytes)
    {
        if (num_bytes > memory_size - address) {
            num_bytes = memory_size - address;
        }

        memcpy(&ram_memory[address], data, num_bytes);

        return num_bytes;
    }

    uint32_t erase_data(uint32_t address, uint32_t num_bytes)
    {
        if (num_bytes > memory_size - address) {
            num_bytes = memory_size - address;
        }

        memset(&ram_memory[address], 0, num_bytes);

        return num_bytes;
    }

public:
    EiDeviceRAM(uint32_t config_size):
        EiDeviceMemory(config_size, 0, BLOCK_SIZE * MEMORY_BLOCKS, BLOCK_SIZE)
    {

    }

    /**
     * @brief for RAM memory, we don't need to care about the blocks and
     * pack data one after another for better memory utilization.
     * 
     */
    uint32_t read_sample_data(uint8_t *sample_data, uint32_t address, uint32_t sample_data_size)
    {
        return this->read_data(sample_data, config_size + address, sample_data_size);
    }

    uint32_t write_sample_data(const uint8_t *sample_data, uint32_t address, uint32_t sample_data_size)
    {
        return this->write_data(sample_data, config_size + address, sample_data_size);
    }

    uint32_t erase_sample_data(uint32_t address, uint32_t num_bytes)
    {
        return this->erase_data(config_size + address, num_bytes);
    }
};

#endif /* EI_DEVICE_MEMORY_H */

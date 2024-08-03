#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <drivers/disks/gpt/gpt.h>

int gpt_read(struct driver_disk disk, struct gpt_info *gpt)
{
    uint8_t buffer[SECTOR_SIZE];
    size_t i, j, lba = 0;
    int r;
    struct gpt_partition_entry *entries;

    // read PMBR and copy from buffer
    disk.read_handler(disk.device, 0, 1, &buffer[0]);
    memcpy(&gpt->pmbr, buffer, sizeof(struct gpt_pmbr));

    // read PTH and copy from buffer
    disk.read_handler(disk.device, 1, 1, &buffer[0]);
    memcpy(&gpt->pth, buffer, sizeof(struct gpt_pth));

    // validate GPT
    if ((r = gpt_validate(gpt)) != GPT_SUCCESS)
        return r;

    // fill empty partitions array in GPT with zeros
    if (gpt->pth.number_of_partitions < GPT_MAX_PARTITIONS)
    {
        memset(
            &gpt->partitions[gpt->pth.number_of_partitions], 0,
            sizeof(struct gpt_partition_entry) * (GPT_MAX_PARTITIONS - gpt->pth.number_of_partitions));
    }

    // parse all partition entries in array
    lba = gpt->pth.starting_lba_of_guid_array;
    for (i = 0; i < gpt->pth.number_of_partitions; i++)
    {
        disk.read_handler(disk.device, lba, 1, &buffer[0]);
        entries = (struct gpt_partition_entry *)&buffer[0];

        for (j = 0; j < SECTOR_SIZE / gpt->pth.size_of_array_entry; j++)
        {
            memcpy(&gpt->partitions[i], &entries[j], sizeof(struct gpt_partition_entry));
            if (++i >= GPT_MAX_PARTITIONS)
                goto end;
        }

        lba++;
    }

end:
    return GPT_SUCCESS;
}

int gpt_validate(struct gpt_info *gpt)
{
    // TODO: implement checksums

    // check the PTH signature
    if (memcmp(&gpt->pth.signature, "EFI PART", 8) != 0)
    {
        return GPT_INVALID;
    }

    return GPT_SUCCESS;
}

int gpt_check_entry(struct gpt_partition_entry *entry)
{
    size_t i;
    // check that partition type is not all zero
    for (i = 0; i < 16; i++)
    {
        if (entry->partition_type[i] != 0x00)
            goto partition_used;
    }

    // type is filled with zeros
    return GPT_UNUSED;

partition_used:
    // TODO.
    return GPT_SUCCESS;
}

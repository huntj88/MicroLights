#include <stdint.h>

/**
 * @brief Enables or disables a specific bit in a register based on the given mask.
 *
 * This function allows you to manipulate individual bits in a register by enabling or disabling
 * them using a mask. You can set a bit to `1` (enable) or `0` (disable) depending on the `enable`
 * parameter. The register is modified directly through a pointer.
 *
 * @param register_ptr A pointer to the register (uint8_t*) that will be modified.
 * @param mask A bitmask that specifies which bit to enable/disable.
 * @param enable Set to `1` to enable the bit, `0` to disable the bit.
 *
 * @example
 * // Initialize the STAT0 register with its default value
 * uint8_t STAT0 = 0x00; // Example initial value (all bits are 0)
 *
 * // Enable bit 2 (binary 00000100 = 0x04)
 * setBit(&STAT0, 0b00000100, 1);
 * setBit(&STAT0, 0x04, 1);
 * // Now STAT0 will be 0x04 (00000100)
 *
 * // Disable bit 5 (binary 00100000 = 0x20)
 * setBit(&STAT0, 0b00100000, 0);
 * setBit(&STAT0, 0x20, 0);
 * // Now STAT0 will still be 0x04 (00000100), since bit 5 is already 0
 *
 * @note The function works with any register and any bitmask. You can use this function to manipulate
 *       individual bits, such as status flags or control bits, in microcontroller registers.
 */
void setBit(uint8_t *register_ptr, uint8_t mask, uint8_t enable)
{
    if (enable)
    {
        // Set the bit: register |= mask (enable the bit)
        *register_ptr |= mask;
    }
    else
    {
        // Clear the bit: register &= ~mask (disable the bit)
        *register_ptr &= ~mask;
    }
}

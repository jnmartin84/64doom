/**
 * @brief Register definition for the AI interface
 * @ingroup lowlevel
 */
typedef struct AI_regs_s {
    /** @brief Pointer to uncached memory buffer of samples to play */
    volatile void * address;
    /** @brief Size in bytes of the buffer to be played.  Should be
     *         number of stereo samples * 2 * sizeof( uint16_t ) 
     */
    uint32_t length;
    /** @brief DMA start register.  Write a 1 to this register to start
     *         playing back an audio sample. */
    uint32_t control;
    /** @brief AI status register.  Bit 31 is the full bit, bit 30 is the busy bit. */
    uint32_t status;
    /** @brief Rate at which the buffer should be played.
     *
     * Use the following formula to calculate the value: ((2 * clockrate / frequency) + 1) / 2 - 1
     */
    uint32_t dacrate;
    /** @brief The size of a single sample in bits. */
    uint32_t samplesize;
} AI_regs_t;

/**
 * @file   example1.c
 * @author Lucas Jadilo
 * @brief  Example 1: Frame encapsulation.
 */


/******************************************************************************/
/*  Includes                                                                  */
/******************************************************************************/


#include <stdio.h>
#include "snap.h"


/******************************************************************************/
/*  Function Declarations                                                     */
/******************************************************************************/


const char *statusToString(snap_status_t status);


/******************************************************************************/
/*  Function Definitions                                                      */
/******************************************************************************/


int main(void)
{
	////////////////////////////////////////////////////////////////////////////
	//  Initialization                                                        //
	////////////////////////////////////////////////////////////////////////////

	uint8_t data[50] = {0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9};

	snap_frame_t frame;

	// Using the same array for frame buffer and data to save memory (it must be large enough to store the complete frame)
	int ret = snap_init(&frame, data, sizeof(data));

	printf("\nsnap_init() = %d\n", ret);

	if(ret > 0)
	{
		printf("Frame initialization succeeded.\n");
	}
	else if(ret == SNAP_ERROR_NULL_FRAME)
	{
		printf("Frame initialization failed. Frame pointer is NULL.\n");
		return -1;
	}
	else if(ret == SNAP_ERROR_NULL_BUFFER)
	{
		printf("Frame initialization failed. Buffer pointer is NULL.\n");
		return -1;
	}
	else if(ret == SNAP_ERROR_SHORT_BUFFER)
	{
		printf("Frame initialization failed. Buffer size is smaller than the minimum allowed.\n");
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	//  Encapsulation                                                         //
	////////////////////////////////////////////////////////////////////////////

	snap_fields_t fields = {
		.header = { .dab = SNAP_HDB2_DAB_1BYTE_DEST_ADDRESS,
					.sab = SNAP_HDB2_SAB_2BYTE_SOURCE_ADDRESS,
					.pfb = SNAP_HDB2_PFB_3BYTE_PROTOCOL_FLAGS,
					.ack = SNAP_HDB2_ACK_NOT_REQUESTED,
					.cmd = SNAP_HDB1_CMD_MODE_DISABLED,
					.edm = SNAP_HDB1_EDM_16BIT_CRC },
		.destAddress = 0xA0,
		.sourceAddress = 0xB0B1,
		.protocolFlags = 0xC0C1C2,
		.data = data,
		.dataSize = 10,
		.paddingAfter = true };

	ret = snap_encapsulate(&frame, &fields);

	printf("snap_encapsulate() = %d\n", ret);

	if(ret == SNAP_STATUS_VALID)
	{
		printf("Frame encapsulation succeeded.\n");
	}
	else if(frame.status == SNAP_STATUS_ERROR_OVERFLOW)	// The return value is equal to the frame status, hence both can be used to check the result
	{
		printf("Frame encapsulation failed. Array does not have enough space to store the complete frame.\n");
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	//  Print                                                                 //
	////////////////////////////////////////////////////////////////////////////

	printf("Frame struct:\n\tstatus = %d (%s)\n\tmaxSize = %u\n\tsize = %u\n\tbuffer = ",
		frame.status, statusToString(frame.status), frame.maxSize, frame.size);

	for(int i = 0; i < frame.size; i++)
	{
		printf("%02X ", frame.buffer[i]);
	}

	printf("\n\n");

	return 0;
}

/**
 * @brief Get a string representation of a status value.
 * @param[in] status Status value.
 * @return Pointer to the string that holds the status name.
 */
const char *statusToString(const snap_status_t status)
{
	switch(status)
	{
		case SNAP_STATUS_IDLE:           return "IDLE";
		case SNAP_STATUS_INCOMPLETE:     return "INCOMPLETE";
		case SNAP_STATUS_VALID:          return "VALID";
		case SNAP_STATUS_ERROR_HASH:     return "ERROR_HASH";
		case SNAP_STATUS_ERROR_OVERFLOW: return "ERROR_OVERFLOW";
		default:                         return "UNKNOWN";
	}
}

/**
 * @brief Dummy function definition to avoid "undefined reference" error when
 *        compiling with `-D SNAP_DISABLE_WEAK` and `-D SNAP_OVERRIDE_USER_HASH`.
 */
uint32_t snap_calculateUserHash(const uint8_t *data, const uint16_t size)
{
	(void)data;
	(void)size;
	return 0;
}

/******************************** END OF FILE *********************************/

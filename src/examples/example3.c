/**
 * @file   example3.c
 * @author Lucas Jadilo
 * @brief  Example 3: User-defined hash function (CRC-24/OPENPGP).
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

	uint8_t buffer[SNAP_MAX_SIZE_FRAME];
	snap_frame_t frame;

	int16_t ret = snap_init(&frame, buffer, sizeof(buffer));

	printf("\nsnap_init() = %d\n", ret);

	if(ret > 0)
	{
		printf("Frame initialization succeeded.\n");
	}
	else
	{
		printf("Frame initialization failed.\n");
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	//  Encapsulation                                                         //
	////////////////////////////////////////////////////////////////////////////

	uint8_t data[] = {0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9};

	snap_fields_t fields = {
		.header = { .dab = SNAP_HDB2_DAB_1BYTE_DEST_ADDRESS,
		            .sab = SNAP_HDB2_SAB_2BYTE_SOURCE_ADDRESS,
		            .pfb = SNAP_HDB2_PFB_3BYTE_PROTOCOL_FLAGS,
		            .ack = SNAP_HDB2_ACK_REQUESTED,
		            .cmd = SNAP_HDB1_CMD_MODE_ENABLED,
		            .edm = SNAP_HDB1_EDM_USER_SPECIFIED },	// SNAP_SIZE_USER_HASH and snap_calculateUserHash() must be defined accordingly
		.destAddress = 0xA0,
		.sourceAddress = 0xB0B1,
		.protocolFlags = 0xC0C1C2,
		.data = data,
		.dataSize = sizeof(data),
		.paddingAfter = false };

	ret = snap_encapsulate(&frame, &fields);

	printf("snap_encapsulate() = %d\n", ret);

	if(frame.status == SNAP_STATUS_VALID)
	{
		printf("Frame encapsulation succeeded.\n");
	}
	else
	{
		printf("Frame encapsulation failed.\n");
		return -1;
	}

	////////////////////////////////////////////////////////////////////////////
	//  Print                                                                 //
	////////////////////////////////////////////////////////////////////////////

	printf("Frame struct:\n\tstatus = %d (%s)\n\tmaxSize = %u\n\tsize = %u\n\tbuffer = ",
		frame.status, statusToString(frame.status), frame.maxSize, frame.size);

	for(uint_fast16_t i = 0; i < frame.size; i++)
	{
		printf("%02X ", frame.buffer[i]);
	}

	printf("\n\n");

	return 0;
}

/**
 * @brief Calculate the 24-bit CRC of a byte array.
 * @details This function overrides the "weak" library implementation. SNAP_SIZE_USER_HASH must be defined and equal to 3.
 *          The algorithm can be described according to the Rocksoft^tm Model CRC Algorithm by Ross Williams:
 *          |       Name     | Width |   Poly   |   Init   | RefIn | RefOut |  XorOut  |  Check   |
 *          |:--------------:|:-----:|:--------:|:--------:|:-----:|:------:|:--------:|:--------:|
 *          | CRC-24/OPENPGP | 24    | 0x864CFB | 0xB704CE | False | False  | 0x000000 | 0x21CF02 |
 * @param[in] data Pointer to the byte array used in the calculation.
 * @param[in] size Number of bytes used in the calculation.
 * @return Result.
 */
uint32_t snap_calculateUserHash(const uint8_t *data, const uint16_t size)
{
	uint32_t crc = 0xB704CE;

	for(uint_fast16_t i = 0; i < size; i++)
	{
		crc ^= (uint32_t)data[i] << 16;
		for(uint_fast8_t j = 0; j < 8; j++)
		{
			crc = (crc & 0x800000) ? (crc << 1) ^ 0x864CFB : crc << 1;
			crc &= 0xFFFFFF;
		}
	}

	return crc;
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

/******************************** END OF FILE *********************************/

/**
 * @file   example2.c
 * @author Lucas Jadilo
 * @brief  Example 2: Frame decoding and decapsulation.
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
void printFrame(const snap_frame_t *frame);


/******************************************************************************/
/*  Function Definitions                                                      */
/******************************************************************************/


int main(void)
{
	////////////////////////////////////////////////////////////////////////////
	//  Initialization                                                        //
	////////////////////////////////////////////////////////////////////////////

	uint8_t buffer[50];
	snap_frame_t frame;

	int16_t ret = snap_init(&frame, buffer, sizeof(buffer));

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
	//  Decoding                                                              //
	////////////////////////////////////////////////////////////////////////////

	const uint8_t inputBytes[] = {
		0x00, 0x11, 0x22,											// Preamble (ignored)
		0x54,														// Sync byte
		0x6C, 0x49,													// Header
		0xA0,														// Destination address
		0xB0, 0xB1,													// Source Address
		0xC0, 0xC1, 0xC2,											// Protocol specific flags
		0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9,	// Data
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,							// Payload padding
		0xE6, 0xEB,													// 16-bit CRC
		0xFF, 0xEE };												// Postamble (ignored)

	for(uint_fast16_t i = 0; i < sizeof(inputBytes); i++)
	{
		ret = snap_decode(&frame, inputBytes[i]);

		printf("inputBytes[%u] = %02X, ", (unsigned)i, inputBytes[i]);
		printFrame(&frame);
	}

	if(ret == SNAP_STATUS_IDLE)
	{
		printf("Frame decoding: No sync byte detected.\n");
	}
	else if(frame.status == SNAP_STATUS_INCOMPLETE)	// The return value is equal to the frame status, hence both can be used to check the result
	{
		printf("Frame decoding: Sync byte detected, but the frame is incomplete.\n");
	}
	else if(frame.status == SNAP_STATUS_VALID)
	{
		printf("Frame decoding: Frame buffer contains a complete and valid frame.\n");
	}
	else if(frame.status == SNAP_STATUS_ERROR_HASH)
	{
		printf("Frame decoding: Frame is complete, but its hash value does not match the value calculated.\n");
	}
	else if(frame.status == SNAP_STATUS_ERROR_OVERFLOW)
	{
		printf("Frame decoding: Frame buffer contains sync and header bytes, but it does not have enough space to store the complete frame.\n");
	}

	////////////////////////////////////////////////////////////////////////////
	//  Decapsulation                                                         //
	////////////////////////////////////////////////////////////////////////////

	snap_header_t header = {0};

	ret = snap_getField(&frame, &header, SNAP_FIELD_HEADER);

	printf("Frame decapsulation:\n");

	if(ret > 0)
	{
		printf("\tHeader: DAB = %u, SAB = %u, PFB = %u, ACK = %u, CMD = %u, EDM = %u, NDB = %u\n",
			header.dab, header.sab, header.pfb, header.ack, header.cmd, header.edm, header.ndb);
	}
	else
	{
		printf("\tFailed to get the header.\n");
	}

	uint32_t destAddress = 0;

	ret = snap_getField(&frame, &destAddress, SNAP_FIELD_DEST_ADDRESS);

	if(ret > 0)
	{
		printf("\tDestination address = 0x%0*X\n", 2*ret, destAddress);
	}
	else
	{
		printf("\tFailed to get the destination address.\n");
	}

	uint32_t sourceAddress = 0;

	ret = snap_getField(&frame, &sourceAddress, SNAP_FIELD_SOURCE_ADDRESS);

	if(ret > 0)
	{
		printf("\tSource address = 0x%0*X\n", 2*ret, sourceAddress);
	}
	else
	{
		printf("\tFailed to get the source address.\n");
	}

	uint32_t protocolFlags = 0;

	ret = snap_getField(&frame, &protocolFlags, SNAP_FIELD_PROTOCOL_FLAGS);

	if(ret > 0)
	{
		printf("\tProtocol flags = 0x%0*X\n", 2*ret, protocolFlags);
	}
	else
	{
		printf("\tFailed to get the protocol flags.\n");
	}

	uint8_t data[sizeof(inputBytes)];

	ret = snap_getField(&frame, data, SNAP_FIELD_DATA);

	if(ret > 0)
	{
		printf("\tPayload = ");

		for(uint_fast16_t i = 0; i < (uint_fast16_t)ret; i++)
		{
			printf("%02X ", data[i]);
		}

		const uint16_t dataSize = snap_removePaddingBytes(data, (uint16_t)ret, true);

		printf("\n\tActual Data = ");

		for(uint_fast16_t i = 0; i < dataSize; i++)
		{
			printf("%02X ", data[i]);
		}

		printf("\n");
	}
	else
	{
		printf("\tFailed to get the data bytes.\n");
	}

	uint32_t hash = 0;

	ret = snap_getField(&frame, &hash, SNAP_FIELD_HASH);

	if(ret > 0)
	{
		printf("\tHash = 0x%0*X\n\n", 2*ret, hash);
	}
	else
	{
		printf("\tFailed to get the hash value.\n\n");
	}

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
 * @brief Print the content of a frame structure.
 * @param[in] frame Pointer to the frame structure.
 */
void printFrame(const snap_frame_t *frame)
{
	printf("status = %d (%s), buffer[%u/%u] = ", frame->status, statusToString(frame->status) , frame->size, frame->maxSize);

	for(uint_fast16_t i = 0; i < frame->size; i++)
	{
		printf("%02X ", frame->buffer[i]);
	}
	
	printf("\n");
}

/******************************** END OF FILE *********************************/

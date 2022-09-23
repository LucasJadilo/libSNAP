/**
 * @file   example4.c
 * @author Lucas Jadilo
 * @brief  Example 4: Function-like macros.
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
	const snap_frame_t frame = {
		.status = SNAP_STATUS_VALID,
		.size = 27,
		.maxSize = 100,
		.buffer = (uint8_t [100]){ 0x54, 0x6C, 0x49, 0xA0, 0xB0, 0xB1, 0xC0,
								   0xC1, 0xC2, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4,
								   0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0x00, 0x00,
								   0x00, 0x00, 0x00, 0x00, 0xE6, 0xEB } };

	////////////////////////////////////////////////////////////////////////////
	//  Frame content                                                         //
	////////////////////////////////////////////////////////////////////////////

	printf("\nFrame content:\n");
	printf("\tsnap_getSync() = 0x%02X\n", snap_getSync());
	printf("\tsnap_getHdb2() = 0x%02X\n", snap_getHdb2(&frame));
	printf("\tsnap_getHdb1() = 0x%02X\n", snap_getHdb1(&frame));
	printf("\tsnap_getDab() = %u\n", snap_getDab(&frame));
	printf("\tsnap_getSab() = %u\n", snap_getSab(&frame));
	printf("\tsnap_getPfb() = %u\n", snap_getPfb(&frame));
	printf("\tsnap_getAck() = %u\n", snap_getAck(&frame));
	printf("\tsnap_getCmd() = %u\n", snap_getCmd(&frame));
	printf("\tsnap_getEdm() = %u\n", snap_getEdm(&frame));
	printf("\tsnap_getNdb() = %u\n", snap_getNdb(&frame));

	int fieldSize = 0;
	snap_header_t header = {0};
	fieldSize = snap_getHeader(&frame, &header);
	printf("\tsnap_getHeader() = %d, header = { dab = %u, sab = %u, pfb = %u, ack = %u, cmd = %u, edm = %u, ndb = %u }\n",
		fieldSize, header.ack, header.sab, header.pfb, header.ack, header.cmd, header.edm, header.ndb);

	uint32_t destAddress = 0;
	fieldSize = snap_getDestAddress(&frame, &destAddress);
	printf("\tsnap_getDestAddress() = %d, destAddress = 0x%0*X\n", fieldSize, (fieldSize > 0) ? (2 * fieldSize) : 1, destAddress);

	uint32_t sourceAddress = 0;
	fieldSize = snap_getSourceAddress(&frame, &sourceAddress);
	printf("\tsnap_getSourceAddress() = %d, sourceAddress = 0x%0*X\n", fieldSize, (fieldSize > 0) ? (2 * fieldSize) : 1, sourceAddress);

	uint32_t protocolFlags = 0;
	fieldSize = snap_getProtocolFlags(&frame, &protocolFlags);
	printf("\tsnap_getProtocolFlags() = %d, protocolFlags = 0x%0*X\n", fieldSize, (fieldSize > 0) ? (2 * fieldSize) : 1, protocolFlags);

	uint8_t data[100] = {0};
	fieldSize = snap_getData(&frame, data);
	printf("\tsnap_getData() = %d, data = { ", fieldSize);
	if(fieldSize > 0)
	{
		for(int i = 0; i < fieldSize; i++)
		{
			printf("%02X ", data[i]);
		}
	}
	printf("}\n");

	uint32_t hash = 0;
	fieldSize = snap_getHash(&frame, &hash);
	printf("\tsnap_getHash() = %d, hash = 0x%0*X\n", fieldSize, (fieldSize > 0) ? (2 * fieldSize) : 1, hash);

	uint8_t *pData = snap_getDataPtr(&frame);
	printf("\tsnap_getDataPtr() = 0x%p, *pData = 0x%02X\n", pData, *pData);

	uint8_t *pFrame = snap_getBufferPtr(&frame);
	printf("\tsnap_getBufferPtr() = 0x%p, *pFrame = 0x%02X\n", pFrame, *pFrame);

	int8_t status = snap_getStatus(&frame);
	printf("\tsnap_getStatus() = %d (%s)\n", status, statusToString(status));

	////////////////////////////////////////////////////////////////////////////
	//  Field indexes                                                         //
	////////////////////////////////////////////////////////////////////////////

	printf("\nField indexes:\n");
	printf("\tsnap_getSyncIndex() = %u\n", snap_getSyncIndex());
	printf("\tsnap_getHdb2Index() = %u\n", snap_getHdb2Index());
	printf("\tsnap_getHdb1Index() = %u\n", snap_getHdb1Index());
	printf("\tsnap_getHeaderIndex() = %u\n", snap_getHeaderIndex());
	printf("\tsnap_getDestAddrIndex() = %u\n", snap_getDestAddrIndex());
	printf("\tsnap_getSourceAddrIndex() = %u\n", snap_getSourceAddrIndex(&frame));
	printf("\tsnap_getProtFlagsIndex() = %u\n", snap_getProtFlagsIndex(&frame));
	printf("\tsnap_getDataIndex() = %u\n", snap_getDataIndex(&frame));
	printf("\tsnap_getHashIndex() = %u\n", snap_getHashIndex(&frame));

	////////////////////////////////////////////////////////////////////////////
	//  Frame and field sizes                                                 //
	////////////////////////////////////////////////////////////////////////////

	printf("\nFrame and field sizes:\n");
	printf("\tsnap_getSyncSize() = %u\n", snap_getSyncSize());
	printf("\tsnap_getHdb2Size() = %u\n", snap_getHdb2Size());
	printf("\tsnap_getHdb1Size() = %u\n", snap_getHdb1Size());
	printf("\tsnap_getHeaderSize() = %u\n", snap_getHeaderSize());
	printf("\tsnap_getDestAddrSize() = %u\n", snap_getDestAddrSize(&frame));
	printf("\tsnap_getSourceAddrSize() = %u\n", snap_getSourceAddrSize(&frame));
	printf("\tsnap_getProtFlagsSize() = %u\n", snap_getProtFlagsSize(&frame));
	printf("\tsnap_getDataSize() = %u\n", snap_getDataSize(&frame));
	printf("\tsnap_getHashSize() = %u\n", snap_getHashSize(&frame));
	printf("\tsnap_getFrameSize() = %u\n", snap_getFrameSize(&frame));
	printf("\tsnap_getBufferSize() = %u\n", snap_getBufferSize(&frame));
	printf("\tsnap_getFullFrameSize() = %u\n\n", snap_getFullFrameSize(&frame));

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

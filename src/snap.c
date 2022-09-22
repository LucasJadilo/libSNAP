/*
Copyright (c) 2022 Lucas Jadilo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @file   snap.c
 * @author Lucas Jadilo
 * @brief  Source file of the libSNAP library. Refer to the library documentation for details.
 */

/**
 * @addtogroup libSNAP
 * @{
 */


/******************************************************************************/
/*  Private Includes                                                          */
/******************************************************************************/


#include <stddef.h>
#include "snap.h"


/******************************************************************************/
/*  Private Macros                                                            */
/******************************************************************************/


#ifdef SNAP_DISABLE_WEAK
	#define SNAP_WEAK
#else
	#define SNAP_WEAK	__attribute__((weak))
#endif


/******************************************************************************/
/*  Public Function Definitions                                               */
/******************************************************************************/


/**
 * @addtogroup pf
 * @{
 */

/**
 * @brief Initialize the frame structure.
 * @details A frame structure should be initialized before passing it to other functions.
 *          On success, store the buffer pointer and size, and clear the other frame variables.
 *          On error, the structure remains unchanged.
 * @param[out] frame   Pointer to the frame structure.
 * @param[in]  buffer  Pointer to the array that will store the frame bytes.
 * @param[in]  maxSize Maximum number of bytes that can be stored in the buffer.
 *                     It must be a value from #SNAP_MIN_SIZE_FRAME to #SNAP_MAX_SIZE_FRAME.
 *                     If necessary, it will be limited to #SNAP_MAX_SIZE_FRAME without generating error.
 * @retval >0                       Return the actual maxSize used.
 * @retval #SNAP_ERROR_NULL_FRAME   Error: Frame pointer is NULL.
 * @retval #SNAP_ERROR_NULL_BUFFER  Error: Buffer pointer is NULL.
 * @retval #SNAP_ERROR_SHORT_BUFFER Error: maxSize is less than the minimum allowed (#SNAP_MIN_SIZE_FRAME).
 */
int16_t snap_init(snap_frame_t *frame, uint8_t *buffer, const uint16_t maxSize)
{
	if(frame == NULL)                 return SNAP_ERROR_NULL_FRAME;
	if(buffer == NULL)                return SNAP_ERROR_NULL_BUFFER;
	if(maxSize < SNAP_MIN_SIZE_FRAME) return SNAP_ERROR_SHORT_BUFFER;

	frame->maxSize = (maxSize > SNAP_MAX_SIZE_FRAME) ? SNAP_MAX_SIZE_FRAME : maxSize;
	frame->buffer = buffer;
	frame->status = SNAP_STATUS_IDLE;
	frame->size = 0;

	return (int16_t)frame->maxSize;
}

/**
 * @brief Reset the frame size and status. The other variables of the frame remain unchanged.
 * @details This function should be called after decoding a frame, prior to decoding
 *          a new frame. After executing this function, the frame will be considered
 *          empty, even though the buffer still holds the previous frame bytes.
 * @param[out] frame Pointer to the frame structure.
 */
void snap_reset(snap_frame_t *frame)
{
	frame->size = 0;
	frame->status = SNAP_STATUS_IDLE;
}

/**
 * @brief Detect, decode, validate and store a frame, one byte at a time.
 * @details All input bytes before a sync byte will be ignored.
 *          When a new byte is inserted into the buffer, the frame size and status are updated accordingly.
 *          All input bytes after a valid frame or any error will be ignored.
 *          Prior to decoding a new frame, the frame status must be #SNAP_STATUS_IDLE. This can be achieved with snap_reset().
 *          See #snap_status_t for more details.
 * @param[in,out] frame   Pointer to the frame structure.
 * @param[in]     newByte Byte to be decoded and inserted into the frame buffer.
 * @return Frame status after the process. It can be any value from #snap_status_t.
 */
int8_t snap_decode(snap_frame_t *frame, const uint8_t newByte)
{
	switch(frame->status)
	{
		case SNAP_STATUS_IDLE:
			if(newByte == SNAP_SYNC)
			{
				frame->buffer[SNAP_INDEX_SYNC] = newByte;
				frame->size = 1;
				frame->status = SNAP_STATUS_INCOMPLETE;
			}
			return frame->status;

		case SNAP_STATUS_INCOMPLETE:
			frame->buffer[frame->size++] = newByte;
			if(frame->size >= SNAP_MIN_SIZE_FRAME)
			{
				const uint8_t hashSize = SNAP_SIZE_HASH(frame->buffer);
				const uint16_t fullFrameSize = (uint16_t)(SNAP_INDEX_HASH(frame->buffer) + hashSize);

				if(frame->maxSize < fullFrameSize)
				{
					frame->status = SNAP_STATUS_ERROR_OVERFLOW;
				}
				else if(frame->size >= fullFrameSize)
				{
					if(hashSize)
					{
						uint32_t expectedHash, actualHash;

						snap_calculateHash(frame, &expectedHash);
						snap_getField(frame, &actualHash, SNAP_FIELD_HASH);

						frame->status = (actualHash == expectedHash) ? SNAP_STATUS_VALID : SNAP_STATUS_ERROR_HASH;
					}
					else
					{
						frame->status = SNAP_STATUS_VALID;
					}
				}
			}
			return frame->status;

		default:	// Valid frame or error
			return frame->status;
	}
}

/**
 * @brief Encapsulate a new frame into the buffer (if there is enough space).
 *        Update the frame status and size according to the result.
 * @param[in,out] frame  Pointer to the frame structure.
 * @param[in,out] fields Pointer to the structure that contains every data needed to build the frame.
 *                       Fields that do not match the frame format will be ignored.
 *                       The NDB value is always ignored because it will be calculated from the data size.
 *                       If data pointer is NULL or data size is zero, the frame will have no payload.
 *                       It is safe to use the same array as data and frame buffer (safe copy).
 * @return Frame status after the process (value from #snap_status_t).
 * @retval #SNAP_STATUS_VALID          Frame created successfully. Frame size is updated to match the new frame.
 * @retval #SNAP_STATUS_ERROR_OVERFLOW Error: Frame does not fit in the buffer. Buffer remains unchanged. Frame size is changed to zero.
 */
int8_t snap_encapsulate(snap_frame_t *frame, snap_fields_t *fields)
{
	if(fields->data == NULL)
	{
		fields->dataSize = 0;
	}

	fields->header.ndb = snap_getNdbFromDataSize(fields->dataSize) & SNAP_HDB1_NDB_MASK;

	const uint_fast16_t payloadSize = snap_getDataSizeFromNdb(fields->header.ndb);
	const uint_fast8_t hashSize = snap_getHashSizeFromEdm(fields->header.edm);
	const uint_fast8_t payloadIndex = (uint_fast8_t)(SNAP_INDEX_DAB + fields->header.dab + fields->header.sab + fields->header.pfb);

	if(frame->maxSize < (payloadIndex + payloadSize + hashSize))
	{
		frame->size = 0;
		frame->status = SNAP_STATUS_ERROR_OVERFLOW;
		return frame->status;
	}

	const uint_fast16_t paddingSize = (uint_fast16_t)(payloadSize - fields->dataSize);
	uint_fast16_t dataIndex, paddingIndex;

	if(fields->paddingAfter)
	{
		dataIndex = payloadIndex;
		paddingIndex = (uint_fast16_t)(payloadIndex + fields->dataSize);
	}
	else
	{
		dataIndex = (uint_fast16_t)(payloadIndex + paddingSize);
		paddingIndex = payloadIndex;
	}

	for(uint_fast16_t i = (uint_fast16_t)(dataIndex + fields->dataSize - 1); i >= dataIndex; i--)
	{
		frame->buffer[i] = fields->data[i - dataIndex];	// Frame buffer and data pointers may point to the same array (safe copy)
	}

	for(uint_fast16_t i = paddingIndex; i < paddingIndex + paddingSize; i++)
	{
		frame->buffer[i] = SNAP_PADDING;
	}

	frame->buffer[SNAP_INDEX_SYNC] = SNAP_SYNC;

	frame->buffer[SNAP_INDEX_HDB2] = (uint8_t)((fields->header.dab << SNAP_HDB2_DAB_POS) |
	                                           (fields->header.sab << SNAP_HDB2_SAB_POS) |
	                                           (fields->header.pfb << SNAP_HDB2_PFB_POS) |
	                                           (fields->header.ack << SNAP_HDB2_ACK_POS));

	frame->buffer[SNAP_INDEX_HDB1] = (uint8_t)((fields->header.cmd << SNAP_HDB1_CMD_POS) |
	                                           (fields->header.edm << SNAP_HDB1_EDM_POS) |
	                                           (fields->header.ndb << SNAP_HDB1_NDB_POS));

	frame->size = SNAP_INDEX_DAB;

	for(uint_fast8_t i = fields->header.dab; i != 0; i--)
	{
		frame->buffer[frame->size++] = (fields->destAddress >> ((i - 1) * 8)) & 0xFF;
	}

	for(uint_fast8_t i = fields->header.sab; i != 0; i--)
	{
		frame->buffer[frame->size++] = (fields->sourceAddress >> ((i - 1) * 8)) & 0xFF;
	}

	for(uint_fast8_t i = fields->header.pfb; i != 0; i--)
	{
		frame->buffer[frame->size++] = (fields->protocolFlags >> ((i - 1) * 8)) & 0xFF;
	}

	frame->size = (uint16_t)(frame->size + payloadSize);

	if(hashSize)
	{
		uint32_t hashValue;
		snap_calculateHash(frame, &hashValue);

		for(uint_fast8_t i = hashSize; i != 0; i--)
		{
			frame->buffer[frame->size++] = (hashValue >> ((i - 1) * 8)) & 0xFF;
		}
	}

	frame->status = SNAP_STATUS_VALID;
	return frame->status;
}

/**
 * @brief Get the content of a selected frame field.
 * @param[in]  frame        Pointer to the frame structure.
 * @param[out] fieldContent Pointer to the variable that will store the field value.
 *                          In case of error, the variable remains unchanged.
 *                          To get the header, the pointer must point to a #snap_header_t.
 *                          To get the destination address, source address, hash value or protocol flags, the pointer must point to a uint32_t.
 *                          To get the data/payload, the pointer must point to an array of uint8_t (it must be large enough to hold all the bytes).
 * @param[in]  fieldType    Selects the field whose content should be returned. It must be a value from #snap_fieldType_t.
 * @retval >0                         Return the size (bytes) of the field.
 * @retval #SNAP_ERROR_UNKNOWN_FORMAT Error: Frame header is not complete.
 * @retval #SNAP_ERROR_FRAME_FORMAT   Error: Frame format does not have the requested field.
 * @retval #SNAP_ERROR_SHORT_FRAME    Error: Frame format has the requested field, but it is incomplete or empty.
 * @retval #SNAP_ERROR_FIELD_TYPE     Error: Invalid field type value.
 */
int16_t snap_getField(const snap_frame_t *frame, void *fieldContent, const uint8_t fieldType)
{
	if(frame->size < SNAP_MIN_SIZE_FRAME)
	{
		return SNAP_ERROR_UNKNOWN_FORMAT;
	}

	uint_fast16_t fieldIndex, fieldSize;

	switch(fieldType)
	{
		case SNAP_FIELD_HEADER:
			((snap_header_t *)fieldContent)->dab = SNAP_HDB2_DAB(frame->buffer);
			((snap_header_t *)fieldContent)->sab = SNAP_HDB2_SAB(frame->buffer);
			((snap_header_t *)fieldContent)->pfb = SNAP_HDB2_PFB(frame->buffer);
			((snap_header_t *)fieldContent)->ack = SNAP_HDB2_ACK(frame->buffer);
			((snap_header_t *)fieldContent)->cmd = SNAP_HDB1_CMD(frame->buffer);
			((snap_header_t *)fieldContent)->edm = SNAP_HDB1_EDM(frame->buffer);
			((snap_header_t *)fieldContent)->ndb = SNAP_HDB1_NDB(frame->buffer);
			return SNAP_SIZE_HEADER;
		case SNAP_FIELD_DEST_ADDRESS:
			fieldIndex = SNAP_INDEX_DAB;
			fieldSize = SNAP_HDB2_DAB(frame->buffer);
			break;
		case SNAP_FIELD_SOURCE_ADDRESS:
			fieldIndex = (uint_fast16_t)SNAP_INDEX_SAB(frame->buffer);
			fieldSize = SNAP_HDB2_SAB(frame->buffer);
			break;
		case SNAP_FIELD_DATA:
			fieldIndex = (uint_fast16_t)SNAP_INDEX_DATA(frame->buffer);
			fieldSize = SNAP_SIZE_DATA(frame->buffer);
			break;
		case SNAP_FIELD_HASH:
			fieldIndex = (uint_fast16_t)SNAP_INDEX_HASH(frame->buffer);
			fieldSize = SNAP_SIZE_HASH(frame->buffer);
			break;
		case SNAP_FIELD_PROTOCOL_FLAGS:
			fieldIndex = (uint_fast16_t)SNAP_INDEX_PFB(frame->buffer);
			fieldSize = SNAP_HDB2_PFB(frame->buffer);
			break;
		default:
			return SNAP_ERROR_FIELD_TYPE;
	}

	if(fieldSize == 0)
	{
		return SNAP_ERROR_FRAME_FORMAT;
	}

	const uint_fast16_t frameSize = (uint_fast16_t)(fieldIndex + fieldSize);

	if(frame->size < frameSize)
	{
		return SNAP_ERROR_SHORT_FRAME;
	}

	if(fieldType == SNAP_FIELD_DATA)
	{
		for(uint_fast16_t i = 0; i < fieldSize; i++)
		{
			((uint8_t *)fieldContent)[i] = frame->buffer[fieldIndex + i];
		}
	}
	else
	{
		*(uint32_t *)fieldContent = 0;
		for(uint_fast16_t i = fieldIndex; i < frameSize; i++)
		{
			*(uint32_t *)fieldContent = (*(uint32_t *)fieldContent << 8) | frame->buffer[i];
		}
	}

	return (int16_t)fieldSize;
}

/**
 * @brief Select the error detection method based on the EDM bits and calculate the hash value of a frame.
 * @param[in]  frame Pointer to the frame structure.
 * @param[out] hash  Pointer to the variable that will store the hash value.
 *                   In case of error, the variable remains unchanged.
 * @retval >0                         Return the size (bytes) of the hash value.
 * @retval #SNAP_ERROR_UNKNOWN_FORMAT Error: Frame header is not complete.
 * @retval #SNAP_ERROR_FRAME_FORMAT   Error: Frame format does not have a hash value.
 * @retval #SNAP_ERROR_SHORT_FRAME    Error: Frame format has a hash value, but it is incomplete or empty.
 */
int8_t snap_calculateHash(const snap_frame_t *frame, uint32_t *hash)
{
	if(frame->size < SNAP_MIN_SIZE_FRAME)
	{
		return SNAP_ERROR_UNKNOWN_FORMAT;
	}

	if(SNAP_SIZE_HASH(frame->buffer) == 0)
	{
		return SNAP_ERROR_FRAME_FORMAT;
	}

	uint16_t frameSize = (uint16_t)SNAP_INDEX_HASH(frame->buffer);

	if(frame->size < frameSize)
	{
		return SNAP_ERROR_SHORT_FRAME;
	}

	frameSize--;	// Hash calculation does not include the sync byte

	switch(SNAP_HDB1_EDM(frame->buffer))
	{
		case SNAP_HDB1_EDM_8BIT_CHECKSUM:
			*hash = snap_calculateChecksum8(&(frame->buffer[SNAP_INDEX_HDB2]), frameSize);
			return 1;
		case SNAP_HDB1_EDM_8BIT_CRC:
			*hash = snap_calculateCrc8(&(frame->buffer[SNAP_INDEX_HDB2]), frameSize);
			return 1;
		case SNAP_HDB1_EDM_16BIT_CRC:
			*hash = snap_calculateCrc16(&(frame->buffer[SNAP_INDEX_HDB2]), frameSize);
			return 2;
		case SNAP_HDB1_EDM_32BIT_CRC:
			*hash = snap_calculateCrc32(&(frame->buffer[SNAP_INDEX_HDB2]), frameSize);
			return 4;
		case SNAP_HDB1_EDM_USER_SPECIFIED:
			*hash = snap_calculateUserHash(&(frame->buffer[SNAP_INDEX_HDB2]), frameSize);
			return SNAP_SIZE_USER_HASH;
		default:
			return SNAP_ERROR_FRAME_FORMAT;	// No error detection, 3 times re-transmission, FEC standard
	}
}

/**
 * @brief Remove the padding bytes (#SNAP_PADDING) of a frame payload (if there are any).
 * @details If the payload has 8 bytes or less, the frame is not supposed to have
 *          padding bytes, hence nothing will be done.
 * @param[out] data         Pointer to the full payload of the frame.
 * @param[in]  size         Number of bytes in the payload (including the padding bytes).
 * @param[in]  paddingAfter Position of the padding bytes in the payload.
 *                          @arg @c true: Padding after data. <b>WARNING:</b> If the actual data ends with padding bytes, they also will be discarded.
 *                          @arg @c false: Padding before data. <b>WARNING:</b> If the actual data starts with padding bytes, they also will be discarded.
 * @return Number of actual data bytes (excluding padding bytes).
 */
uint16_t snap_removePaddingBytes(uint8_t *data, uint16_t size, const bool paddingAfter)
{
	if(size < 9)
	{
		return size;
	}

	if(paddingAfter)
	{
		while((data[size - 1] == SNAP_PADDING) && (--size != 0));
		return size;
	}

	uint_fast16_t paddingSize = 0;

	while((data[paddingSize] == SNAP_PADDING) && (++paddingSize < size));

	size = (uint16_t)(size - paddingSize);

	for(uint_fast16_t i = 0; i < size; i++)
	{
		data[i] = data[paddingSize + i];
	}

	return size;
}

/**
 * @brief Get the NDB bits of the HDB1 byte based on the number of data bytes (with or without padding bytes).
 * @param[in] dataSize Number of data bytes. It must be a value from 0 to 512.
 * @return NDB value (#snap_hdb1_ndb_t). It will be a value from 0 to 14.
 */
uint8_t snap_getNdbFromDataSize(const uint16_t dataSize)
{
	if(dataSize <= 8)   return (uint8_t)dataSize;
	if(dataSize <= 16)  return 9;
	if(dataSize <= 32)  return 10;
	if(dataSize <= 64)  return 11;
	if(dataSize <= 128) return 12;
	if(dataSize <= 256) return 13;
	if(dataSize <= 512) return 14;
	return 0;	// Should not reach this point
}

/**
 * @brief Get the number of data bytes based on the NDB bits of the HDB1 byte.
 * @param[in] ndb NDB value (#snap_hdb1_ndb_t). It must be a value from 0 to 14.
 * @return Number of data bytes (including padding bytes).
 */
uint16_t snap_getDataSizeFromNdb(const uint8_t ndb)
{
	if(ndb <= 8)  return ndb;
	if(ndb <= 14) return (uint16_t)(0x01U << (ndb - 5));
	return 0;	// Should not reach this point (User-defined NDB)
}

/**
 * @brief Get the hash value size of the frame based on the EDM bits of the HDB1 byte.
 * @param[in] edm EDM value (#snap_hdb1_edm_t). Only 3 bits (LSb) will be considered.
 * @return Hash value size. It will be a value from 0 to 4.
 */
uint8_t snap_getHashSizeFromEdm(const uint8_t edm)
{
	static const uint8_t hashSize[8] = {0, 0, 1, 1, 2, 4, 0, SNAP_SIZE_USER_HASH};
	return hashSize[edm & SNAP_HDB1_EDM_MASK];
}

/**
 * @brief Calculate the 8-bit checksum of a byte array.
 * @param[in] data Pointer to the byte array used in the calculation.
 * @param[in] size Number of bytes used in the calculation.
 * @return Result.
 */
uint8_t snap_calculateChecksum8(const uint8_t *data, const uint16_t size)
{
	uint8_t checksum = 0;

	for(uint_fast16_t i = 0; i < size; i++)
	{
		checksum = (uint8_t)(checksum + data[i]);
	}

	return checksum;
}

#ifndef SNAP_OVERRIDE_CRC8

/**
 * @brief Calculate the 8-bit CRC of a byte array.
 * @details This "weak" function can be overridden by a user implementation,
 *          especially when a hardware implementation is available. If the macro
 *          `SNAP_CRC8_TABLE` is defined, this function will use a 256-byte lookup
 *          table to speed up the calculation. If the macro `SNAP_DISABLE_WEAK` is defined,
 *          this function becomes a "strong" definition, so the only way to override it
 *          is to define the macro `SNAP_OVERRIDE_CRC8`. The algorithm can be described
 *          according to the Rocksoft^tm Model CRC Algorithm by Ross Williams:
 *          | Name            | Width | Poly | Init | RefIn | RefOut | XorOut | Check |
 *          |:---------------:|:-----:|:----:|:----:|:-----:|:------:|:------:|:-----:|
 *          | CRC-8/MAXIM-DOW | 8     | 0x31 | 0x00 | True  | True   | 0x00   | 0xA1  |
 * @param[in] data Pointer to the byte array used in the calculation.
 * @param[in] size Number of bytes used in the calculation.
 * @return Result.
 */
SNAP_WEAK uint8_t snap_calculateCrc8(const uint8_t *data, const uint16_t size)
{
	uint8_t crc = 0;

#ifdef SNAP_CRC8_TABLE

	static const uint8_t tableCrc8[256] =
	{
		0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83, 0xC2, 0x9C, 0x7E, 0x20,
		0xA3, 0xFD, 0x1F, 0x41, 0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E,
		0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC, 0x23, 0x7D, 0x9F, 0xC1,
		0x42, 0x1C, 0xFE, 0xA0, 0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
		0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D, 0x7C, 0x22, 0xC0, 0x9E,
		0x1D, 0x43, 0xA1, 0xFF, 0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5,
		0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07, 0xDB, 0x85, 0x67, 0x39,
		0xBA, 0xE4, 0x06, 0x58, 0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
		0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6, 0xA7, 0xF9, 0x1B, 0x45,
		0xC6, 0x98, 0x7A, 0x24, 0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B,
		0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9, 0x8C, 0xD2, 0x30, 0x6E,
		0xED, 0xB3, 0x51, 0x0F, 0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
		0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92, 0xD3, 0x8D, 0x6F, 0x31,
		0xB2, 0xEC, 0x0E, 0x50, 0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C,
		0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE, 0x32, 0x6C, 0x8E, 0xD0,
		0x53, 0x0D, 0xEF, 0xB1, 0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
		0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49, 0x08, 0x56, 0xB4, 0xEA,
		0x69, 0x37, 0xD5, 0x8B, 0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4,
		0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16, 0xE9, 0xB7, 0x55, 0x0B,
		0x88, 0xD6, 0x34, 0x6A, 0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
		0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7, 0xB6, 0xE8, 0x0A, 0x54,
		0xD7, 0x89, 0x6B, 0x35
	};

	for(uint_fast16_t i = 0; i < size; i++)
	{
		crc = tableCrc8[data[i] ^ crc];
	}

#else

	for(uint_fast16_t i = 0; i < size; i++)
	{
		crc ^= data[i];
		for(uint_fast8_t j = 0; j < 8; j++)
		{
			crc = (crc & 1) ? ((crc >> 1) ^ 0x8C) : (crc >> 1);
		}
	}

#endif

	return crc;
}

#endif	// SNAP_OVERRIDE_CRC8

#ifndef SNAP_OVERRIDE_CRC16

/**
 * @brief Calculate the 16-bit CRC of a byte array.
 * @details This "weak" function can be overridden by a user implementation,
 *          especially when a hardware implementation is available. If the macro
 *          `SNAP_CRC16_TABLE` is defined, this function will use a 512-byte lookup
 *          table to speed up the calculation. If the macro `SNAP_DISABLE_WEAK` is defined,
 *          this function becomes a "strong" definition, so the only way to override it
 *          is to define the macro `SNAP_OVERRIDE_CRC16`. The algorithm can be described
 *          according to the Rocksoft^tm Model CRC Algorithm by Ross Williams:
 *          | Name          | Width | Poly   | Init   | RefIn | RefOut | XorOut | Check  |
 *          |:-------------:|:-----:|:------:|:------:|:-----:|:------:|:------:|:------:|
 *          | CRC-16/XMODEM | 16    | 0x1021 | 0x0000 | False | False  | 0x0000 | 0x31C3 |
 * @param[in] data Pointer to the byte array used in the calculation.
 * @param[in] size Number of bytes used in the calculation.
 * @return Result.
 */
SNAP_WEAK uint16_t snap_calculateCrc16(const uint8_t *data, const uint16_t size)
{
	uint16_t crc = 0;

#ifdef SNAP_CRC16_TABLE

	static const uint16_t tableCrc16[256] =
	{
		0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108,
		0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210,
		0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B,
		0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401,
		0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE,
		0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6,
		0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D,
		0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
		0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5,
		0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC,
		0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 0x6CA6, 0x7C87, 0x4CE4,
		0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD,
		0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13,
		0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A,
		0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E,
		0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
		0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1,
		0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB,
		0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0,
		0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8,
		0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657,
		0x7676, 0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9,
		0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882,
		0x28A3, 0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
		0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E,
		0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07,
		0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D,
		0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74,
		0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
	};

	for(uint_fast16_t i = 0; i < size; i++)
	{
		crc = (uint16_t)(crc << 8) ^ tableCrc16[(crc >> 8) ^ data[i]];
	}

#else

	for(uint_fast16_t i = 0; i < size; i++)
	{
		crc ^= (uint16_t)(data[i] << 8);
		for(uint_fast8_t j = 0; j < 8; j++)
		{
			crc = (uint16_t)((crc & 0x8000) ? ((crc << 1) ^ 0x1021) : (crc << 1));
		}
	}

#endif

	return crc;
}

#endif	// SNAP_OVERRIDE_CRC16

#ifndef SNAP_OVERRIDE_CRC32

/**
 * @brief Calculate the 32-bit CRC of a byte array.
 * @details This "weak" function can be overridden by a user implementation,
 *          especially when a hardware implementation is available. If the macro
 *          `SNAP_CRC32_TABLE` is defined, this function will use a 1024-byte lookup
 *          table to speed up the calculation. If the macro `SNAP_DISABLE_WEAK` is defined,
 *          this function becomes a "strong" definition, so the only way to override it
 *          is to define the macro `SNAP_OVERRIDE_CRC32`. The algorithm can be described
 *          according to the Rocksoft^tm Model CRC Algorithm by Ross Williams:
 *          | Name            | Width | Poly       | Init       | RefIn | RefOut | XorOut     | Check      |
 *          |:---------------:|:-----:|:----------:|:----------:|:-----:|:------:|:----------:|:----------:|
 *          | CRC-32/ISO-HDLC | 32    | 0x04C11DB7 | 0xFFFFFFFF | True  | True   | 0xFFFFFFFF | 0xCBF43926 |
 * @param[in] data Pointer to the byte array used in the calculation.
 * @param[in] size Number of bytes used in the calculation.
 * @return Result.
 */
SNAP_WEAK uint32_t snap_calculateCrc32(const uint8_t *data, const uint16_t size)
{
	uint32_t crc = 0xFFFFFFFF;

#ifdef SNAP_CRC32_TABLE

	static const uint32_t tableCrc32[256] =
	{
		0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
		0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
		0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
		0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
		0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
		0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
		0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
		0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
		0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
		0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
		0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
		0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
		0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
		0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
		0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
		0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
		0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
		0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
		0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
		0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
		0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
		0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
		0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
		0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
		0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
		0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
		0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
		0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
		0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
		0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
		0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
		0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
		0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
		0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
		0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
		0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
		0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
		0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
		0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
		0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
		0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
		0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
		0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
	};

	for(uint_fast16_t i = 0; i < size; i++)
	{
		crc = (crc >> 8) ^ tableCrc32[(crc ^ data[i]) & 0xFF];
	}

#else

	for(uint_fast16_t i = 0; i < size; i++)
	{
		crc = crc ^ data[i];
		for(uint_fast8_t j = 0; j < 8; j++)
		{
			crc = (crc & 1) ? ((crc >> 1) ^ 0xEDB88320) : (crc >> 1);
		}
	}

#endif

	return ~crc;
}

#endif	// SNAP_OVERRIDE_CRC32

#ifndef SNAP_OVERRIDE_USER_HASH

/**
 * @brief Calculate the hash value of a byte array using a user-defined algorithm.
 * @details This "weak" function is supposed to be overridden by a user implementation.
 *          It will be selected and called automatically by the function snap_calculateHash()
 *          when the EDM value is equal to #SNAP_HDB1_EDM_USER_SPECIFIED. If the macro
 *          `SNAP_DISABLE_WEAK` is defined, this function becomes a "strong" definition,
 *          so the only way to override it is to define the macro `SNAP_OVERRIDE_USER_HASH`.
 * @param[in] data Pointer to the byte array used in the calculation.
 * @param[in] size Number of bytes used in the calculation.
 * @return Result.
 */
SNAP_WEAK uint32_t snap_calculateUserHash(const uint8_t *data, const uint16_t size)
{
	(void)data;
	(void)size;
	return 0;
}

#endif	// SNAP_OVERRIDE_USER_HASH

/**
 * @}
 * @}
 */

/******************************** END OF FILE *********************************/

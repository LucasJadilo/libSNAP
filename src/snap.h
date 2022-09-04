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
 * @file   snap.h
 * @author Lucas Jadilo
 * @brief  Header file of the libSNAP library. Refer to the library documentation for details.
 */

#ifndef SNAP_H_
#define SNAP_H_

#ifdef __cplusplus
	extern "C" {
#endif

/**
 * @defgroup libSNAP libSNAP
 * @author   Lucas Jadilo
 * @version  1.0.0
 * @brief    Top-level module of the libSNAP library. It enclosures all the macros, types and functions of the library.
 * @{
 */


/******************************************************************************/
/*  Public Includes                                                           */
/******************************************************************************/


#include <stdint.h>
#include <stdbool.h>


/******************************************************************************/
/*  Public Macros                                                             */
/******************************************************************************/


/**
 * @defgroup pm Public Macros
 * @brief Macros visible to all files that include the library header file. Most macros are
 *        for naming constant values, but some are wrappers for functions and other macros.
 * @{
 * @name Error codes returned from functions
 * @{
 */

#define SNAP_ERROR_NULL_FRAME		(-1)	/**< @brief Null frame pointer. */
#define SNAP_ERROR_NULL_BUFFER		(-2)	/**< @brief Null buffer pointer. */
#define SNAP_ERROR_SHORT_BUFFER		(-3)	/**< @brief Buffer size is smaller than the minimum frame size allowed. */
#define SNAP_ERROR_UNKNOWN_FORMAT	(-4)	/**< @brief Frame header is not complete, hence it is impossible to know the frame format. */
#define SNAP_ERROR_FRAME_FORMAT		(-5)	/**< @brief Frame format does not have the requested field. Check the frame format (header bytes). */
#define SNAP_ERROR_SHORT_FRAME		(-6)	/**< @brief Frame format has the requested field, but it is incomplete or empty. */
#define SNAP_ERROR_FIELD_TYPE		(-7)	/**< @brief Invalid field type value. It must be a value from #snap_fieldType_t. */

/**
 * @}
 * @name Bit mask and position of HDB2 fields
 * @{
 */

#define SNAP_HDB2_DAB_MASK	(0x03U)	/**< @brief Bit mask of the DAB bits in the HDB2 byte. */
#define SNAP_HDB2_DAB_POS	(6U)	/**< @brief Position of the DAB bits (LSb) in the HDB2 byte. */

#define SNAP_HDB2_SAB_MASK	(0x03U)	/**< @brief Bit mask of the SAB bits in the HDB2 byte. */
#define SNAP_HDB2_SAB_POS	(4U)	/**< @brief Position of the SAB bits (LSb) in the HDB2 byte. */

#define SNAP_HDB2_PFB_MASK	(0x03U)	/**< @brief Bit mask of the PFB bits in the HDB2 byte. */
#define SNAP_HDB2_PFB_POS	(2U)	/**< @brief Position of the PFB bits (LSb) in the HDB2 byte. */

#define SNAP_HDB2_ACK_MASK	(0x03U)	/**< @brief Bit mask of the ACK bits in the HDB2 byte. */
#define SNAP_HDB2_ACK_POS	(0U)	/**< @brief Position of the ACK bits (LSb) in the HDB2 byte. */

/**
 * @}
 * @name Bit mask and position of HDB1 fields
 * @{
 */

#define SNAP_HDB1_CMD_MASK	(0x01U)	/**< @brief Bit mask of the CMD bit in the HDB1 byte. */
#define SNAP_HDB1_CMD_POS	(7U)	/**< @brief Position of the CMD bit in the HDB1 byte. */

#define SNAP_HDB1_EDM_MASK	(0x07U)	/**< @brief Bit mask of the EDM bits in the HDB1 byte. */
#define SNAP_HDB1_EDM_POS	(4U)	/**< @brief Position of the EDM bits (LSb) in the HDB1 byte. */

#define SNAP_HDB1_NDB_MASK	(0x0FU)	/**< @brief Bit mask of the NDB bits in the HDB1 byte. */
#define SNAP_HDB1_NDB_POS	(0U)	/**< @brief Position of the NDB bits (LSb) in the HDB1 byte. */

/**
 * @}
 * @name Header content
 * @{
 */

#define SNAP_GET_BITS(integer, mask, pos)	(((unsigned int)(integer) >> (pos)) & (mask))	/**< @brief Get a subset of bits from an integer. @param integer Source of the bits. @param mask Bit mask. @param pos Position of the first bit (LSb). */

#define SNAP_HDB2(pByteArray)	((pByteArray)[SNAP_INDEX_HDB2])	/**< @brief Get the HDB2 byte of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_HDB1(pByteArray)	((pByteArray)[SNAP_INDEX_HDB1])	/**< @brief Get the HDB1 byte of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */

#define SNAP_HDB2_DAB(pByteArray)	(SNAP_GET_BITS(SNAP_HDB2(pByteArray), SNAP_HDB2_DAB_MASK, SNAP_HDB2_DAB_POS))	/**< @brief Get the DAB bits of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_HDB2_SAB(pByteArray)	(SNAP_GET_BITS(SNAP_HDB2(pByteArray), SNAP_HDB2_SAB_MASK, SNAP_HDB2_SAB_POS))	/**< @brief Get the SAB bits of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_HDB2_PFB(pByteArray)	(SNAP_GET_BITS(SNAP_HDB2(pByteArray), SNAP_HDB2_PFB_MASK, SNAP_HDB2_PFB_POS))	/**< @brief Get the PFB bits of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_HDB2_ACK(pByteArray)	(SNAP_GET_BITS(SNAP_HDB2(pByteArray), SNAP_HDB2_ACK_MASK, SNAP_HDB2_ACK_POS))	/**< @brief Get the ACK bits of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */

#define SNAP_HDB1_CMD(pByteArray)	(SNAP_GET_BITS(SNAP_HDB1(pByteArray), SNAP_HDB1_CMD_MASK, SNAP_HDB1_CMD_POS))	/**< @brief Get the CMD bit of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_HDB1_EDM(pByteArray)	(SNAP_GET_BITS(SNAP_HDB1(pByteArray), SNAP_HDB1_EDM_MASK, SNAP_HDB1_EDM_POS))	/**< @brief Get the EDM bits of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_HDB1_NDB(pByteArray)	(SNAP_GET_BITS(SNAP_HDB1(pByteArray), SNAP_HDB1_NDB_MASK, SNAP_HDB1_NDB_POS))	/**< @brief Get the NDB bits of a frame. @param pByteArray Pointer to the array of bytes that contains the frame. */

/**
 * @}
 * @name Frame and field sizes
 * @{
 */

#define SNAP_SIZE_SYNC				(1U)													/**< @brief Size of the sync field. */
#define SNAP_SIZE_HDB2				(1U)													/**< @brief Size of the HDB2 field. */
#define SNAP_SIZE_HDB1				(1U)													/**< @brief Size of the HDB1 field. */
#define SNAP_SIZE_HEADER			(SNAP_SIZE_HDB2 + SNAP_SIZE_HDB1)						/**< @brief Size of the header field. */
#define SNAP_SIZE_DATA(pByteArray)	(snap_getDataSizeFromNdb(SNAP_HDB1_NDB(pByteArray)))	/**< @brief Size of the data field based on the NDB bits. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_SIZE_HASH(pByteArray)	(snap_getHashSizeFromEdm(SNAP_HDB1_EDM(pByteArray)))	/**< @brief Size of the hash field based on the EDM bits. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_MIN_SIZE_FRAME			(SNAP_SIZE_SYNC + SNAP_SIZE_HEADER)						/**< @brief Minimum frame size allowed = 1 (sync) + 2 (header). */
#define SNAP_MAX_SIZE_FRAME			(528U)													/**< @brief Maximum frame size allowed = 1 (sync) + 2 (header) + 3 (destination address) + 3 (source address) + 3 (flags) + 512 (data) + 4 (hash). */

#ifdef SNAP_SIZE_USER_HASH
	#if (SNAP_SIZE_USER_HASH < 0) || (SNAP_SIZE_USER_HASH > 4)
		#error Invalid user hash size! It must be an integer from 0 to 4 (bytes).
	#endif
#else
	#define SNAP_SIZE_USER_HASH	(0U)	/**< @brief Size of the user hash field (0 to 4 bytes). It is supposed to be defined by the user in the compilation command. */
#endif

/**
 * @}
 * @name Field indexes
 * @{
 */

#define SNAP_INDEX_SYNC				(0U)														/**< @brief Index of the sync byte. */
#define SNAP_INDEX_HDB2				(1U)														/**< @brief Index of the HDB2 byte. */
#define SNAP_INDEX_HDB1				(2U)														/**< @brief Index of the HDB1 byte. */
#define SNAP_INDEX_DAB				(3U)														/**< @brief Index of the first (MSB) destination address byte (if there are any). */
#define SNAP_INDEX_SAB(pByteArray)	(SNAP_INDEX_DAB + SNAP_HDB2_DAB(pByteArray))				/**< @brief Index of the first (MSB) source address byte (if there are any), based on the frame header. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_INDEX_PFB(pByteArray)	(SNAP_INDEX_SAB(pByteArray) + SNAP_HDB2_SAB(pByteArray))	/**< @brief Index of the first (MSB) protocol flags byte (if there are any), based on the frame header. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_INDEX_DATA(pByteArray)	(SNAP_INDEX_PFB(pByteArray) + SNAP_HDB2_PFB(pByteArray))	/**< @brief Index of the first data byte (if there are any), based on the frame header. @param pByteArray Pointer to the array of bytes that contains the frame. */
#define SNAP_INDEX_HASH(pByteArray)	(SNAP_INDEX_DATA(pByteArray) +  SNAP_SIZE_DATA(pByteArray))	/**< @brief Index of the first (MSB) hash byte (if there are any), based on the frame header. @param pByteArray Pointer to the array of bytes that contains the frame. */

/**
 * @}
 * @name Miscellaneous values
 * @{
 */

#define SNAP_BROADCAST_ADDRESS	(0U)	/**< @brief All nodes in the network should accept a packet that has this value as its destination address. No node should have this value as its local address. */
#define SNAP_SYNC				(0x54U)	/**< @brief Value of the sync byte. It is the first byte of every frame. */
#define SNAP_PADDING			(0x00U)	/**< @brief Value of the byte used in payload padding (when the payload size is greater than the actual data size). */

/**
 * @}
 * @defgroup uc User Convenience
 * @brief Function-like macros defined just for user convenience. Most of them are wrappers for functions and other macros.
 * @note Only functions actually check the frame for errors (e.g. incomplete frame, wrong frame format, etc).
 *       Before using macros that are not wrappers of the function snap_getField(), be sure the operation is valid
 *       (i.e. the frame has a valid header, the frame format has the requested field, etc).
 * @{
 * @name Get frame content
 * @{
 */

#define snap_getSync()								(SNAP_SYNC)														/**< @brief Get the value of the sync byte. */
#define snap_getHdb2(pFrame)						(SNAP_HDB2((pFrame)->buffer))									/**< @brief Get the HDB2 byte of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getHdb1(pFrame)						(SNAP_HDB1((pFrame)->buffer))									/**< @brief Get the HDB1 byte of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getDab(pFrame)							(SNAP_HDB2_DAB((pFrame)->buffer))								/**< @brief Get the DAB bits of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getSab(pFrame)							(SNAP_HDB2_SAB((pFrame)->buffer))								/**< @brief Get the SAB bits of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getPfb(pFrame)							(SNAP_HDB2_PFB((pFrame)->buffer))								/**< @brief Get the PFB bits of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getAck(pFrame)							(SNAP_HDB2_ACK((pFrame)->buffer))								/**< @brief Get the ACK bits of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getCmd(pFrame)							(SNAP_HDB1_CMD((pFrame)->buffer))								/**< @brief Get the CMD bit of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getEdm(pFrame)							(SNAP_HDB1_EDM((pFrame)->buffer))								/**< @brief Get the EDM bits of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getNdb(pFrame)							(SNAP_HDB1_NDB((pFrame)->buffer))								/**< @brief Get the NDB bits of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getHeader(pFrame, pHeader)				(snap_getField(pFrame, pHeader, SNAP_FIELD_HEADER))				/**< @brief Get the header of a frame. @param pFrame Pointer to the frame structure (*#snap_frame_t*). @param pHeader Pointer to the variable that will store the header (#snap_header_t*). */
#define snap_getDestAddress(pFrame, pDestAddr)		(snap_getField(pFrame, pDestAddr, SNAP_FIELD_DEST_ADDRESS))		/**< @brief Get the destination address of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). @param pDestAddr Pointer to the variable that will store the address (uint32_t*). */
#define snap_getSourceAddress(pFrame, pSourceAddr)	(snap_getField(pFrame, pSourceAddr, SNAP_FIELD_SOURCE_ADDRESS))	/**< @brief Get the source address of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). @param pSourceAddr Pointer to the variable that will store the address (uint32_t*). */
#define snap_getProtocolFlags(pFrame, pFlags)		(snap_getField(pFrame, pFlags, SNAP_FIELD_PROTOCOL_FLAGS))		/**< @brief Get the protocol flags of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). @param pFlags Pointer to the variable that will store the flags (uint32_t*). */
#define snap_getData(pFrame, pData)					(snap_getField(pFrame, pData, SNAP_FIELD_DATA))					/**< @brief Get the data bytes of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). @param pData Pointer to the byte array that will store the data (uint8_t*). */
#define snap_getHash(pFrame, pHash)					(snap_getField(pFrame, pHash, SNAP_FIELD_HASH))					/**< @brief Get the hash value of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). @param pHash Pointer to the variable that will store the hash value (uint32_t*). */
#define snap_getDataPtr(pFrame)						((pFrame)->buffer + SNAP_INDEX_DATA((pFrame)->buffer))			/**< @brief Get the pointer to the first data byte of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getBufferPtr(pFrame)					((pFrame)->buffer)												/**< @brief Get the pointer to the first byte of a frame. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getStatus(pFrame)						((pFrame)->status)												/**< @brief Get the frame status (it can be any value from #snap_status_t). @param pFrame Pointer to the frame structure (#snap_frame_t*). */

/**
 * @}
 * @name Get field indexes
 * @{
 */

#define snap_getSyncIndex()				(SNAP_INDEX_SYNC)					/**< @brief Get the index of the sync byte. */
#define snap_getHdb2Index()				(SNAP_INDEX_HDB2)					/**< @brief Get the index of the HDB2 byte. */
#define snap_getHdb1Index()				(SNAP_INDEX_HDB1)					/**< @brief Get the index of the HDB1 byte. */
#define snap_getHeaderIndex()			(SNAP_INDEX_HDB2)					/**< @brief Get the index of the first header byte (HDB2). */
#define snap_getDestAddrIndex()			(SNAP_INDEX_DAB)					/**< @brief Get the index of the first (MSB) destination address byte (if there are any). */
#define snap_getSourceAddrIndex(pFrame)	(SNAP_INDEX_SAB((pFrame)->buffer))	/**< @brief Get the index of the first (MSB) source address byte (if there are any). @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getProtFlagsIndex(pFrame)	(SNAP_INDEX_PFB((pFrame)->buffer))	/**< @brief Get the index of the first (MSB) protocol flags byte (if there are any). @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getDataIndex(pFrame)		(SNAP_INDEX_DATA((pFrame)->buffer))	/**< @brief Get the index of the first data byte (if there are any). @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getHashIndex(pFrame)		(SNAP_INDEX_HASH((pFrame)->buffer))	/**< @brief Get the index of the first (MSB) data byte (if there are any). @param pFrame Pointer to the frame structure (#snap_frame_t*). */

/**
 * @}
 * @name Get frame and field sizes
 * @{
 */

#define snap_getSyncSize()				(SNAP_SIZE_SYNC)														/**< @brief Get the size of the sync field. */
#define snap_getHdb2Size()				(SNAP_SIZE_HDB2)														/**< @brief Get the size of the HDB2 field. */
#define snap_getHdb1Size()				(SNAP_SIZE_HDB1)														/**< @brief Get the size of the HDB1 field. */
#define snap_getHeaderSize()			(SNAP_SIZE_HEADER)														/**< @brief Get the size of the header field. */
#define snap_getDestAddrSize(pFrame)	(SNAP_HDB2_DAB((pFrame)->buffer))										/**< @brief Get the size of the destination address. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getSourceAddrSize(pFrame)	(SNAP_HDB2_SAB((pFrame)->buffer))										/**< @brief Get the size of the source address. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getProtFlagsSize(pFrame)	(SNAP_HDB2_PFB((pFrame)->buffer))										/**< @brief Get the size of the protocol flags. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getDataSize(pFrame)		(SNAP_SIZE_DATA((pFrame)->buffer))										/**< @brief Get the size of the data field. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getHashSize(pFrame)		(SNAP_SIZE_HASH((pFrame)->buffer))										/**< @brief Get the size of the hash field. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getFrameSize(pFrame)		((pFrame)->size)														/**< @brief Get the current size of a frame (it may be incomplete). @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getBufferSize(pFrame)		((pFrame)->maxSize)														/**< @brief Get the maximum number of bytes that can be stored in the buffer. @param pFrame Pointer to the frame structure (#snap_frame_t*). */
#define snap_getFullFrameSize(pFrame)	(SNAP_INDEX_HASH((pFrame)->buffer) + SNAP_SIZE_HASH((pFrame)->buffer))	/**< @brief Get the size of a frame as if it were complete (based on the header). @param pFrame Pointer to the frame structure (#snap_frame_t*). */

/**
 * @}
 * @}
 * @}
 */


/******************************************************************************/
/*  Public Types                                                              */
/******************************************************************************/


/**
 * @defgroup pt Public Types
 * @brief Typedefs of enums and structs visible to all files that include the library header file.
 * @{
 */

/**
 * @brief Values that identify each field of a frame.
 */
typedef enum snap_fieldType_t
{
	SNAP_FIELD_HEADER         = 0,	/**< Header (HDB2 and HDB1 bytes). */
	SNAP_FIELD_DEST_ADDRESS   = 1,	/**< Destination address. */
	SNAP_FIELD_SOURCE_ADDRESS = 2,	/**< Source address. */
	SNAP_FIELD_PROTOCOL_FLAGS = 3,	/**< Protocol specific flags. */
	SNAP_FIELD_DATA           = 4,	/**< Data (payload). */
	SNAP_FIELD_HASH           = 5	/**< Hash value (checksum, CRC-8, CRC-16, etc). */
} snap_fieldType_t;

/**
 * @brief Values for the DAB field of the HDB2 header byte. It defines the number of destination address bytes.
 */
typedef enum snap_hdb2_dab_t
{
	SNAP_HDB2_DAB_NO_DEST_ADDRESS    = 0,	/**< Frame has no destination address. */
	SNAP_HDB2_DAB_1BYTE_DEST_ADDRESS = 1,	/**< Frame has a 1-byte destination address. */
	SNAP_HDB2_DAB_2BYTE_DEST_ADDRESS = 2,	/**< Frame has a 2-byte destination address. */
	SNAP_HDB2_DAB_3BYTE_DEST_ADDRESS = 3	/**< Frame has a 3-byte destination address. */
} snap_hdb2_dab_t;

/**
 * @brief Values for the SAB field of the HDB2 header byte. It defines the number of source address bytes.
 */
typedef enum snap_hdb2_sab_t
{
	SNAP_HDB2_SAB_NO_SOURCE_ADDRESS    = 0,	/**< Frame has no source address. */
	SNAP_HDB2_SAB_1BYTE_SOURCE_ADDRESS = 1,	/**< Frame has a 1-byte source address. */
	SNAP_HDB2_SAB_2BYTE_SOURCE_ADDRESS = 2,	/**< Frame has a 2-byte source address. */
	SNAP_HDB2_SAB_3BYTE_SOURCE_ADDRESS = 3	/**< Frame has a 3-byte source address. */
} snap_hdb2_sab_t;

/**
 * @brief Values for the PFB field of the HDB2 header byte. It defines the number of protocol specific flags bytes.
 */
typedef enum snap_hdb2_pfb_t
{
	SNAP_HDB2_PFB_NO_PROTOCOL_FLAGS    = 0,	/**< Frame has no protocol specific flags. */
	SNAP_HDB2_PFB_1BYTE_PROTOCOL_FLAGS = 1,	/**< Frame has 1 byte of protocol specific flags. */
	SNAP_HDB2_PFB_2BYTE_PROTOCOL_FLAGS = 2,	/**< Frame has 2 bytes of protocol specific flags. */
	SNAP_HDB2_PFB_3BYTE_PROTOCOL_FLAGS = 3	/**< Frame has 3 bytes of protocol specific flags. */
} snap_hdb2_pfb_t;

/**
 * @brief Values for the ACK field of the HDB2 header byte. It defines if the sending node requests an ACK/NACK packet in return.
 *        It also acts as the actual ACK/NACK response sent from the receiving node.
 */
typedef enum snap_hdb2_ack_t
{
	SNAP_HDB2_ACK_NOT_REQUESTED = 0,	/**< Sending node does not request an ACK/NACK packet in return. */
	SNAP_HDB2_ACK_REQUESTED     = 1,	/**< Sending node requests an ACK/NACK packet in return. */
	SNAP_HDB2_ACK_RESPONSE_ACK  = 2,	/**< Receiving node sends an ACK response. */
	SNAP_HDB2_ACK_RESPONSE_NACK = 3		/**< Receiving node sends an NACK response. */
} snap_hdb2_ack_t;

/**
 * @brief Values for the CMD field of the HDB1 header byte. It indicates whether the command mode is enabled or not.
 * @details A node that implements this functionality can respond to requests from other nodes,
 *          even when it does not support the format of the received packet.
 *          If the command mode is enabled, the first data byte contains a command byte (request or response).
 *          Commands in the range 0~127 are requests, and commands in the range 128~255 are responses.
 *          The commands 0/128 are for discovering if the node supports the command mode.
 *          The commands 1/129 are for discovering the preferred packet structure of the node.
 *          The other command values are reserved but not yet defined.
 */
typedef enum snap_hdb1_cmd_t
{
	SNAP_HDB1_CMD_MODE_DISABLED = 0,	/**< Command mode is disabled. */
	SNAP_HDB1_CMD_MODE_ENABLED  = 1		/**< Command mode is enabled. */
} snap_hdb1_cmd_t;

/**
 * @brief Values for the EDM field of the HDB1 header byte. It defines the error detection method used on the frame.
 * @note Some options are not fully supported by this library (see the notes).
 */
typedef enum snap_hdb1_edm_t
{
	SNAP_HDB1_EDM_NO_ERROR_DETECTION = 0,	/**< Frame does not contain any information about error detection/correction. */
	SNAP_HDB1_EDM_3_RETRANSMISSION   = 1,	/**< The sending node must send the same frame 3 times, and the receiving node should compare the frames in order to detect errors. @note Not supported by this library. It produces the same result as EDM = 0. */
	SNAP_HDB1_EDM_8BIT_CHECKSUM      = 2,	/**< Frame has an 8-bit checksum at the end. */
	SNAP_HDB1_EDM_8BIT_CRC           = 3,	/**< Frame has an 8-bit CRC at the end. */
	SNAP_HDB1_EDM_16BIT_CRC          = 4,	/**< Frame has a 16-bit CRC at the end. */
	SNAP_HDB1_EDM_32BIT_CRC          = 5,	/**< Frame has a 32-bit CRC at the end. */
	SNAP_HDB1_EDM_FEC                = 6,	/**< Specific FEC (Forward Error Correction) standard to be determined. @note Not supported by this library. It produces the same result as EDM = 0. */
	SNAP_HDB1_EDM_USER_SPECIFIED     = 7	/**< Error detection method defined by the user. @note This library only supports methods that append a hash value of up to 4 bytes into the end of the frame (like the CRC options).
												 The user can only define the hash function (by overriding snap_calculateUserHash()) and the hash value size (by overriding #SNAP_SIZE_USER_HASH). */
} snap_hdb1_edm_t;

/**
 * @brief Values for the NDB field of the HDB1 header byte. It defines the number of data/payload bytes.
 */
typedef enum snap_hdb1_ndb_t
{
	SNAP_HDB1_NDB_NO_DATA        =  0,	/**< Frame has no data bytes. */
	SNAP_HDB1_NDB_1BYTE_DATA     =  1,	/**< Frame has 1 data byte. */
	SNAP_HDB1_NDB_2BYTE_DATA     =  2,	/**< Frame has 2 data bytes. */
	SNAP_HDB1_NDB_3BYTE_DATA     =  3,	/**< Frame has 3 data bytes. */
	SNAP_HDB1_NDB_4BYTE_DATA     =  4,	/**< Frame has 4 data bytes. */
	SNAP_HDB1_NDB_5BYTE_DATA     =  5,	/**< Frame has 5 data bytes. */
	SNAP_HDB1_NDB_6BYTE_DATA     =  6,	/**< Frame has 6 data bytes. */
	SNAP_HDB1_NDB_7BYTE_DATA     =  7,	/**< Frame has 7 data bytes. */
	SNAP_HDB1_NDB_8BYTE_DATA     =  8,	/**< Frame has 8 data bytes. */
	SNAP_HDB1_NDB_16BYTE_DATA    =  9,	/**< Frame has 16 data bytes (it might contain padding bytes). */
	SNAP_HDB1_NDB_32BYTE_DATA    = 10,	/**< Frame has 32 data bytes (it might contain padding bytes). */
	SNAP_HDB1_NDB_64BYTE_DATA    = 11,	/**< Frame has 64 data bytes (it might contain padding bytes). */
	SNAP_HDB1_NDB_128BYTE_DATA   = 12,	/**< Frame has 128 data bytes (it might contain padding bytes). */
	SNAP_HDB1_NDB_256BYTE_DATA   = 13,	/**< Frame has 256 data bytes (it might contain padding bytes). */
	SNAP_HDB1_NDB_512BYTE_DATA   = 14,	/**< Frame has 512 data bytes (it might contain padding bytes). */
	SNAP_HDB1_NDB_USER_SPECIFIED = 15	/**< Number of data bytes is defined by the user. @note Not supported by this library. It produces the same result as NDB = 0. */
} snap_hdb1_ndb_t;

/**
 * @brief Values for the frame status. The frame structure will always be in one of these states.
 */
typedef enum snap_status_t
{
	SNAP_STATUS_IDLE           =  0,	/**< Frame is considered empty (it has not received the sync byte yet). The frame structure must be in this state before decoding a new frame. */
	SNAP_STATUS_INCOMPLETE     =  1,	/**< Frame has received the sync byte, but it is not complete yet. This state is used only during the decoding process. */
	SNAP_STATUS_VALID          =  2,	/**< Frame buffer contains a complete and valid frame. */
	SNAP_STATUS_ERROR_HASH     = -1,	/**< The hash value received does not match the value calculated. This state is used only during the decoding process. */
	SNAP_STATUS_ERROR_OVERFLOW = -2		/**< Frame buffer does not have enough space to store the complete frame. */
} snap_status_t;

/**
 * @brief Bit field that stores every field of the frame header. It defines the format of the frame.
 */
typedef struct snap_header_t
{
	// HDB2
	unsigned int dab : 2;	/**< @brief DAB (Destination Address Bytes). It can assume any value from #snap_hdb2_dab_t. */
	unsigned int sab : 2;	/**< @brief SAB (Source Address Bytes). It can assume any value from #snap_hdb2_sab_t. */
	unsigned int pfb : 2;	/**< @brief PFB (Protocol specific Flag Bytes). It can assume any value from #snap_hdb2_pfb_t. */
	unsigned int ack : 2;	/**< @brief ACK (ACK/NACK bits). It can assume any value from #snap_hdb2_ack_t. */
	// HDB1
	unsigned int cmd : 1;	/**< @brief CMD (Command mode bit). It can assume any value from #snap_hdb1_cmd_t. */
	unsigned int edm : 3;	/**< @brief EDM (Error Detection Method). It can assume any value from #snap_hdb1_edm_t. */
	unsigned int ndb : 4;	/**< @brief NDB (Number of Data Bytes). It can assume any value from #snap_hdb1_ndb_t. */
} snap_header_t;

/**
 * @brief Structure that stores every data necessary to build a new frame (i.e. every field value and size, except the hash bytes).
 */
typedef struct snap_fields_t
{
	uint8_t       *data;			/**< @brief Pointer to the array that holds the frame data/payload. */
	uint32_t      destAddress;		/**< @brief Destination address (up to 0xFFFFFF). */
	uint32_t      sourceAddress;	/**< @brief Source address (up to 0xFFFFFF). */
	uint32_t      protocolFlags;	/**< @brief Protocol specific flags (up to 0xFFFFFF). */
	snap_header_t header;			/**< @brief Header bytes. */
	uint16_t      dataSize;			/**< @brief Number of bytes in the data array. */
	bool          paddingAfter;		/**< @brief Position of the padding bytes in the payload (if there are any). true = padding after data, false = padding before data. */
} snap_fields_t;

/**
 * @brief This is the main structure of the library, used in frame decoding, encapsulation, and decapsulation.
 */
typedef struct snap_frame_t
{
	uint8_t  *buffer;	/**< @brief Pointer to the array that stores all the bytes of the frame. */
	uint16_t maxSize;	/**< @brief Maximum number of bytes that can be stored in the buffer. */
	uint16_t size;		/**< @brief Current size of the frame (it may be incomplete). */
	int8_t   status;	/**< @brief Status of the frame, used primarily in the decoding process. It can assume any value from #snap_status_t. */
} snap_frame_t;

/**
 * @}
 */


/******************************************************************************/
/*  Public Function Declarations                                              */
/******************************************************************************/


/**
 * @defgroup pf Public Functions
 * @brief Functions visible to all files that include the library header file.
 * @{
 * @name Functions that require a frame structure (snap_frame_t)
 * @{
 */

int16_t snap_init(snap_frame_t *frame, uint8_t *buffer, uint16_t maxSize);

void snap_reset(snap_frame_t *frame);

int8_t snap_decode(snap_frame_t *frame, uint8_t newByte);

int8_t snap_encapsulate(snap_frame_t *frame, snap_fields_t *fields);

int16_t snap_getField(const snap_frame_t *frame, void *fieldContent, uint8_t fieldType);

int8_t snap_calculateHash(const snap_frame_t *frame, uint32_t *hash);

/**
 * @}
 * @name Functions that do NOT require a frame structure
 * @{
 */

uint16_t snap_removePaddingBytes(uint8_t *data, uint16_t size, bool paddingAfter);

uint8_t snap_getNdbFromDataSize(uint16_t dataSize);

uint16_t snap_getDataSizeFromNdb(uint8_t ndb);

uint8_t snap_getHashSizeFromEdm(uint8_t edm);

uint8_t snap_calculateChecksum8(const uint8_t *data, uint16_t size);

uint8_t snap_calculateCrc8(const uint8_t *data, uint16_t size);

uint16_t snap_calculateCrc16(const uint8_t *data, uint16_t size);

uint32_t snap_calculateCrc32(const uint8_t *data, uint16_t size);

uint32_t snap_calculateUserHash(const uint8_t *data, uint16_t size);

/**
 * @}
 * @}
 * @}
 */

#ifdef __cplusplus
	}
#endif

#endif	// SNAP_H_

/******************************** END OF FILE *********************************/

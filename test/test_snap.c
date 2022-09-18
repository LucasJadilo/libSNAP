/**
 * @file   test_snap.c
 * @author Lucas Jadilo
 * @brief  Unit tests for the libSNAP library.
 */


/******************************************************************************/
/*  Includes                                                                  */
/******************************************************************************/


#include <stdio.h>
#include "unity_fixture.h"
#include "snap.h"


/******************************************************************************/
/*  Macros                                                                    */
/******************************************************************************/


#define SIZEOF(array)												(sizeof(array)/sizeof(array[0]))
#define TEST_ASSERT_EQUAL_FRAME(pExpectedFrame, pActualFrame)		assertFrame((pExpectedFrame), (pActualFrame), __LINE__)
#define TEST_ASSERT_EQUAL_HEADER(pExpectedHeader, pActualHeader)	assertHeader((pExpectedHeader), (pActualHeader), __LINE__)


/******************************************************************************/
/*  Function Definitions                                                      */
/******************************************************************************/


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

void printFrame(const snap_frame_t *frame)
{
	printf("status=%d, buffer[%u/%u]=", frame->status, frame->size, frame->maxSize);

	if(frame->buffer == NULL)
	{
		printf("NULL");
	}
	else
	{
		for(uint_fast16_t i = 0; i < frame->size; i++)
		{
			printf("%02X ", frame->buffer[i]);
		}
	}

	printf("\n");
}

static void assertFrame(const snap_frame_t *expectedFrame,
						const snap_frame_t *actualFrame,
						const unsigned int lineNumber)
{
	UNITY_TEST_ASSERT_EQUAL_INT8  (expectedFrame->status,  actualFrame->status,  lineNumber, "(frame.status)");
	UNITY_TEST_ASSERT_EQUAL_UINT16(expectedFrame->maxSize, actualFrame->maxSize, lineNumber, "(frame.maxSize)");
	UNITY_TEST_ASSERT_EQUAL_UINT16(expectedFrame->size,    actualFrame->size,    lineNumber, "(frame.size)");

	if(expectedFrame->size)
	{
		UNITY_TEST_ASSERT_EQUAL_HEX8_ARRAY(expectedFrame->buffer, actualFrame->buffer, expectedFrame->size, lineNumber, "(frame.buffer[])");
	}
}

static void assertHeader(const snap_header_t *expectedHeader,
						 const snap_header_t *actualHeader,
						 const unsigned int lineNumber)
{
	UNITY_TEST_ASSERT_EQUAL_UINT(expectedHeader->dab, actualHeader->dab, lineNumber, "(header.dab)");
	UNITY_TEST_ASSERT_EQUAL_UINT(expectedHeader->sab, actualHeader->sab, lineNumber, "(header.sab)");
	UNITY_TEST_ASSERT_EQUAL_UINT(expectedHeader->pfb, actualHeader->pfb, lineNumber, "(header.pfb)");
	UNITY_TEST_ASSERT_EQUAL_UINT(expectedHeader->ack, actualHeader->ack, lineNumber, "(header.ack)");
	UNITY_TEST_ASSERT_EQUAL_UINT(expectedHeader->cmd, actualHeader->cmd, lineNumber, "(header.cmd)");
	UNITY_TEST_ASSERT_EQUAL_UINT(expectedHeader->edm, actualHeader->edm, lineNumber, "(header.edm)");
	UNITY_TEST_ASSERT_EQUAL_UINT(expectedHeader->ndb, actualHeader->ndb, lineNumber, "(header.ndb)");
}

static void test_encapsulate(snap_fields_t *fields,
							 const snap_frame_t *expectedFrame)
{
	uint8_t actualBuffer[SNAP_MAX_SIZE_FRAME] = {0};
	snap_frame_t actualFrame = {.buffer = actualBuffer, .maxSize = expectedFrame->maxSize, .status = SNAP_STATUS_IDLE, .size = 0};

	TEST_ASSERT_EQUAL_INT8_MESSAGE(expectedFrame->status, snap_encapsulate(&actualFrame, fields), "(return value)");
	TEST_ASSERT_EQUAL_FRAME(expectedFrame, &actualFrame);
}

static void test_decode_StatusIncomplete(const uint8_t *frameBytes,
										 const uint16_t frameSize,
										 const int8_t finalStatus,
										 const uint16_t maxSize)
{
	uint8_t actualBuffer[SNAP_MAX_SIZE_FRAME] = {0};
	snap_frame_t actualFrame = {.buffer = actualBuffer, .maxSize = maxSize, .status = SNAP_STATUS_IDLE, .size = 0};

	uint8_t expectedBuffer[sizeof(actualBuffer)] = {0};
	snap_frame_t expectedFrame = {.buffer = expectedBuffer, .maxSize = maxSize, .status = SNAP_STATUS_INCOMPLETE, .size = 0};

	for(uint_fast16_t i = 0; i < frameSize; i++)
	{
		expectedFrame.buffer[expectedFrame.size++] = frameBytes[i];

		if(i == frameSize - 1U)
		{
			expectedFrame.status = finalStatus;
		}

		TEST_ASSERT_EQUAL_INT8_MESSAGE(expectedFrame.status, snap_decode(&actualFrame, frameBytes[i]), "(return value)");
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}
}

static void test_decode_StatusIdle(const uint8_t *preambleBytes,
								   const uint8_t preambleSize)
{
	uint8_t actualBuffer[10] = {0};
	snap_frame_t actualFrame = {.buffer = actualBuffer, .maxSize = sizeof(actualBuffer), .status = SNAP_STATUS_IDLE, .size = 0};

	uint8_t expectedBuffer[sizeof(actualBuffer)] = {0};
	snap_frame_t expectedFrame = {.buffer = expectedBuffer, .maxSize = sizeof(expectedBuffer), .status = SNAP_STATUS_IDLE, .size = 0};

	TEST_ASSERT_EQUAL_INT8_MESSAGE(SNAP_STATUS_IDLE, actualFrame.status, "(frame.status before test)");

	for(uint_fast8_t i = 0; i < preambleSize; i++)
	{
		TEST_ASSERT_EQUAL_INT8_MESSAGE(SNAP_STATUS_IDLE, snap_decode(&actualFrame, preambleBytes[i]), "(return value)");
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}

	TEST_ASSERT_EQUAL_INT8_MESSAGE(SNAP_STATUS_INCOMPLETE, snap_decode(&actualFrame, SNAP_SYNC), "(return value)");

	expectedFrame.buffer[0] = SNAP_SYNC;
	expectedFrame.status = SNAP_STATUS_INCOMPLETE;
	expectedFrame.size = 1;

	TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
}

static void test_decode_StatusValidOrError(uint8_t *frameBytes,
										   const uint16_t frameSize,
										   const uint8_t *postambleBytes,
										   const uint16_t postambleSize,
										   const int8_t finalStatus,
										   const uint16_t maxSize)
{
	uint8_t actualBuffer[SNAP_MAX_SIZE_FRAME] = {0};
	snap_frame_t actualFrame = {.buffer = actualBuffer, .maxSize = maxSize, .status = SNAP_STATUS_IDLE, .size = 0};

	for(uint_fast16_t i = 0; i < frameSize; i++)
	{
		snap_decode(&actualFrame, frameBytes[i]);
	}

	snap_frame_t expectedFrame = {.buffer = frameBytes, .maxSize = maxSize, .status = finalStatus, .size = frameSize};
	TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);

	for(uint_fast16_t i = 0; i < postambleSize; i++)
	{
		TEST_ASSERT_EQUAL_INT8_MESSAGE(expectedFrame.status, snap_decode(&actualFrame, postambleBytes[i]), "(return value)");
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}
}


/******************************************************************************/
/*  TEST GROUP: miscFunctions                                                 */
/******************************************************************************/


TEST_GROUP(miscFunctions);

TEST_SETUP(miscFunctions) {}

TEST_TEAR_DOWN(miscFunctions) {}

TEST_GROUP_RUNNER(miscFunctions)
{
	RUN_TEST_CASE(miscFunctions, removePaddingBytes);
	RUN_TEST_CASE(miscFunctions, getNdbFromDataSize);
	RUN_TEST_CASE(miscFunctions, getDataSizeFromNdb);
	RUN_TEST_CASE(miscFunctions, getHashSizeFromEdm);
	RUN_TEST_CASE(miscFunctions, calculateChecksum8);
	RUN_TEST_CASE(miscFunctions, calculateCrc8);
	RUN_TEST_CASE(miscFunctions, calculateCrc16);
	RUN_TEST_CASE(miscFunctions, calculateCrc32);
	RUN_TEST_CASE(miscFunctions, calculateUserHash);
}

TEST(miscFunctions, removePaddingBytes)
{
	// size = 0

	{
		uint8_t data[1];
		TEST_ASSERT_EQUAL_UINT16(0, snap_removePaddingBytes(data, 0, true));
	}
	{
		uint8_t data[1];
		TEST_ASSERT_EQUAL_UINT16(0, snap_removePaddingBytes(data, 0, false));
	}

	// 0 < size < 8

	{
		uint8_t data[7] = {0x00, 0x01, 0x02, 0x03};
		const uint8_t expected[7] = {0x00, 0x01, 0x02, 0x03};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, 7, true));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}
	{
		uint8_t data[5] = {0x00, 0x00, 0x11};
		const uint8_t expected[5] = {0x00, 0x00, 0x11};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, 5, false));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}

	// size = 8

	{
		uint8_t data[8] = {0x00, 0x01, 0x02, 0x03};
		const uint8_t expected[8] = {0x00, 0x01, 0x02, 0x03};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, sizeof(data), true));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}
	{
		uint8_t data[8] = {0x00, 0x00, 0x11};
		const uint8_t expected[8] = {0x00, 0x00, 0x11};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, sizeof(data), false));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}

	// size > 8

	{
		uint8_t data[16] = {0x00, 0x01, 0x02, 0x03};
		const uint8_t expected[4] = {0x00, 0x01, 0x02, 0x03};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, sizeof(data), true));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}
	{
		uint8_t data[16] = {0x00, 0x00, 0x11, 0x22, 0x33};
		const uint8_t expected[14] = {0x11, 0x22, 0x33};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, sizeof(data), false));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}
	{
		uint8_t data[64] = {0x00, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03};
		const uint8_t expected[10] = {0x00, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03, 0x01, 0x02, 0x03};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, sizeof(data), true));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}
	{
		uint8_t data[128] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 0x33, 0xFF, 0x0F};
		const uint8_t expected[123] = {0x11, 0x22, 0x33, 0xFF, 0x0F};
		TEST_ASSERT_EQUAL_UINT16(sizeof(expected), snap_removePaddingBytes(data, sizeof(data), false));
		TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, data, sizeof(expected));
	}

	// size = paddingSize

	{
		uint8_t data[16] = {0x00};
		TEST_ASSERT_EQUAL_UINT16(0, snap_removePaddingBytes(data, sizeof(data), true));
	}
	{
		uint8_t data[32] = {0x00};
		TEST_ASSERT_EQUAL_UINT16(0, snap_removePaddingBytes(data, sizeof(data), false));
	}
}

TEST(miscFunctions, getNdbFromDataSize)
{
	const uint16_t dataSize[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 16, 17, 25, 32, 33, 50, 64, 65, 90, 128, 129, 200, 256, 257, 350, 512, 513, 1000, UINT16_MAX};
	const uint8_t ndb[]       = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,  9,  9, 10, 10, 10, 11, 11, 11, 12, 12,  12,  13,  13,  13,  14,  14,  14,   0,    0,          0};

	for(uint_fast8_t i = 0; i < SIZEOF(dataSize); i++)
	{
		TEST_ASSERT_EQUAL_UINT8(ndb[i], snap_getNdbFromDataSize(dataSize[i]));
	}
}

TEST(miscFunctions, getDataSizeFromNdb)
{
	const uint8_t ndb[]       = {0, 1, 2, 3, 4, 5, 6, 7, 8,  9, 10, 11,  12,  13,  14, 15, 16, 100, UINT8_MAX};
	const uint16_t dataSize[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128, 256, 512,  0,  0,   0,         0};

	for(uint_fast8_t i = 0; i < SIZEOF(ndb); i++)
	{
		TEST_ASSERT_EQUAL_UINT16(dataSize[i], snap_getDataSizeFromNdb(ndb[i]));
	}
}

TEST(miscFunctions, getHashSizeFromEdm)
{
	const uint8_t edm[]      = {0, 1, 2, 3, 4, 5, 6,                   7};
	const uint8_t hashSize[] = {0, 0, 1, 1, 2, 4, 0, SNAP_SIZE_USER_HASH};

	for(uint_fast8_t i = 0; i < SIZEOF(edm); i++)
	{
		TEST_ASSERT_EQUAL_UINT8(hashSize[i], snap_getHashSizeFromEdm(edm[i]));
	}
}

TEST(miscFunctions, calculateChecksum8)
{
	const char input1[] = "snap";
	const char input2[] = "SNAP";
	const uint8_t input3[] = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87, 0x78, 0x69, 0x5A, 0x4B, 0x3C, 0x2D, 0x1E, 0x0F};

	TEST_ASSERT_EQUAL_HEX8(0xB2, snap_calculateChecksum8((uint8_t *)input1, sizeof(input1) - 1));
	TEST_ASSERT_EQUAL_HEX8(0x32, snap_calculateChecksum8((uint8_t *)input2, sizeof(input2) - 1));
	TEST_ASSERT_EQUAL_HEX8(0xF8, snap_calculateChecksum8(input3, sizeof(input3)));
}

TEST(miscFunctions, calculateCrc8)
{
	const char input1[] = "snap";
	const char input2[] = "SNAP";
	const uint8_t input3[] = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87, 0x78, 0x69, 0x5A, 0x4B, 0x3C, 0x2D, 0x1E, 0x0F};

	TEST_ASSERT_EQUAL_HEX8(0x17, snap_calculateCrc8((uint8_t *)input1, sizeof(input1) - 1));
	TEST_ASSERT_EQUAL_HEX8(0x11, snap_calculateCrc8((uint8_t *)input2, sizeof(input2) - 1));
	TEST_ASSERT_EQUAL_HEX8(0xD8, snap_calculateCrc8(input3, sizeof(input3)));
}

TEST(miscFunctions, calculateCrc16)
{
	const char input1[] = "snap";
	const char input2[] = "SNAP";
	const uint8_t input3[] = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87, 0x78, 0x69, 0x5A, 0x4B, 0x3C, 0x2D, 0x1E, 0x0F};

	TEST_ASSERT_EQUAL_HEX16(0x1F4F, snap_calculateCrc16((uint8_t *)input1, SIZEOF(input1) - 1));
	TEST_ASSERT_EQUAL_HEX16(0x8C43, snap_calculateCrc16((uint8_t *)input2, SIZEOF(input2) - 1));
	TEST_ASSERT_EQUAL_HEX16(0xD214, snap_calculateCrc16(input3, SIZEOF(input3)));
}

TEST(miscFunctions, calculateCrc32)
{
	const char input1[] = "snap";
	const char input2[] = "SNAP";
	const uint8_t input3[] = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87, 0x78, 0x69, 0x5A, 0x4B, 0x3C, 0x2D, 0x1E, 0x0F};

	TEST_ASSERT_EQUAL_HEX32(0x36641D9E, snap_calculateCrc32((uint8_t *)input1, sizeof(input1) - 1));
	TEST_ASSERT_EQUAL_HEX32(0x00F1F02A, snap_calculateCrc32((uint8_t *)input2, sizeof(input2) - 1));
	TEST_ASSERT_EQUAL_HEX32(0x2B21D32F, snap_calculateCrc32(input3, sizeof(input3)));
}

TEST(miscFunctions, calculateUserHash)
{
	const char input1[] = "snap";
	const char input2[] = "SNAP";
	const uint8_t input3[] = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5, 0x96, 0x87, 0x78, 0x69, 0x5A, 0x4B, 0x3C, 0x2D, 0x1E, 0x0F};

	TEST_ASSERT_EQUAL_HEX32(0xDD91A8, snap_calculateUserHash((uint8_t *)input1, sizeof(input1) - 1));
	TEST_ASSERT_EQUAL_HEX32(0x4EA35C, snap_calculateUserHash((uint8_t *)input2, sizeof(input2) - 1));
	TEST_ASSERT_EQUAL_HEX32(0x42A8A4, snap_calculateUserHash(input3, sizeof(input3)));
}


/******************************************************************************/
/*  TEST GROUP: init                                                          */
/******************************************************************************/


TEST_GROUP(init);

TEST_SETUP(init) {}

TEST_TEAR_DOWN(init) {}

TEST_GROUP_RUNNER(init)
{
	RUN_TEST_CASE(init, should_ReturnMaxSize_and_InitializeFrameVariables_if_PointersAreNotNull_and_MaxSizeIsValid);
	RUN_TEST_CASE(init, should_ReturnErrorNullFrame_if_FramePointerIsNull);
	RUN_TEST_CASE(init, should_ReturnErrorNullBuffer_and_NotModifyFrameVariables_if_BufferPointerIsNull);
	RUN_TEST_CASE(init, should_ReturnErrorShortBuffer_and_NotModifyFrameVariables_if_MaxSizeIsTooShort);
	RUN_TEST_CASE(init, should_LimitMaxBufferSize);
}

TEST(init, should_ReturnMaxSize_and_InitializeFrameVariables_if_PointersAreNotNull_and_MaxSizeIsValid)
{
	{	// maxSize = minimum frame size allowed
		uint8_t buffer[10];
		const snap_frame_t expectedFrame = {.buffer = buffer, .maxSize = SNAP_MIN_SIZE_FRAME, .status = SNAP_STATUS_IDLE, .size = 0};
		snap_frame_t initialFrame = {.buffer = (uint8_t *)76, .maxSize = 54, .status = 32, .size = 15};

		TEST_ASSERT_EQUAL_INT16(SNAP_MIN_SIZE_FRAME, snap_init(&initialFrame, buffer, SNAP_MIN_SIZE_FRAME));
		TEST_ASSERT_EQUAL_PTR(expectedFrame.buffer, initialFrame.buffer);
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &initialFrame);
	}
	{	// maxSize = maximum frame size allowed
		uint8_t buffer[10];
		const snap_frame_t expectedFrame = {.buffer = buffer, .maxSize = SNAP_MAX_SIZE_FRAME, .status = SNAP_STATUS_IDLE, .size = 0};
		snap_frame_t initialFrame = {.buffer = (uint8_t *)NULL, .maxSize = 1, .status = 2, .size = 3};

		TEST_ASSERT_EQUAL_INT16(SNAP_MAX_SIZE_FRAME, snap_init(&initialFrame, buffer, SNAP_MAX_SIZE_FRAME));
		TEST_ASSERT_EQUAL_PTR(expectedFrame.buffer, initialFrame.buffer);
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &initialFrame);
	}
	{	// maxSize = arbitrary value between the valid limits
		uint8_t buffer[10];
		const snap_frame_t expectedFrame = {.buffer = buffer, .maxSize = 100, .status = SNAP_STATUS_IDLE, .size = 0};
		snap_frame_t initialFrame = {.buffer = (uint8_t *)0xFFFFFFFF, .maxSize = 10, .status = 20, .size = 30};

		TEST_ASSERT_EQUAL_INT16(100, snap_init(&initialFrame, buffer, 100));
		TEST_ASSERT_EQUAL_PTR(expectedFrame.buffer, initialFrame.buffer);
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &initialFrame);
	}
}

TEST(init, should_ReturnErrorNullFrame_if_FramePointerIsNull)
{
	uint8_t buffer[10];
	TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_NULL_FRAME, snap_init(NULL, buffer, sizeof(buffer)));
}

TEST(init, should_ReturnErrorNullBuffer_and_NotModifyFrameVariables_if_BufferPointerIsNull)
{
	uint8_t buffer[10];
	snap_frame_t actualFrame = {.buffer = (uint8_t *)0x3A, .maxSize = 0x2B, .status = 0x1C, .size = 0x0D};
	const snap_frame_t expectedFrame = actualFrame;

	TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_NULL_BUFFER, snap_init(&actualFrame, NULL, sizeof(buffer)));
	TEST_ASSERT_EQUAL_PTR(expectedFrame.buffer, actualFrame.buffer);
	TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
}

TEST(init, should_ReturnErrorShortBuffer_and_NotModifyFrameVariables_if_MaxSizeIsTooShort)
{
	{	// maxSize = zero
		uint8_t buffer[10];
		snap_frame_t actualFrame = {.buffer = (uint8_t *)12, .maxSize = 34, .status = 56, .size = 78};
		const snap_frame_t expectedFrame = actualFrame;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_BUFFER, snap_init(&actualFrame, buffer, 0));
		TEST_ASSERT_EQUAL_PTR(expectedFrame.buffer, actualFrame.buffer);
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}
	{	// maxSize = 1 byte shorter than the minimum allowed
		uint8_t buffer[10];
		snap_frame_t actualFrame = {.buffer = (uint8_t *)99, .maxSize = 88, .status = 77, .size = 66};
		const snap_frame_t expectedFrame = actualFrame;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_BUFFER, snap_init(&actualFrame, buffer, SNAP_MIN_SIZE_FRAME - 1));
		TEST_ASSERT_EQUAL_PTR(expectedFrame.buffer, actualFrame.buffer);
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}
}

TEST(init, should_LimitMaxBufferSize)
{
	const uint16_t actualMaxSize[]   = {SNAP_MIN_SIZE_FRAME, 100, 200, SNAP_MAX_SIZE_FRAME,                1000,                2000,          UINT16_MAX};
	const uint16_t expectedMaxSize[] = {SNAP_MIN_SIZE_FRAME, 100, 200, SNAP_MAX_SIZE_FRAME, SNAP_MAX_SIZE_FRAME, SNAP_MAX_SIZE_FRAME, SNAP_MAX_SIZE_FRAME};

	for(uint_fast8_t i = 0; i < SIZEOF(actualMaxSize); i++)
	{
		uint8_t buffer[10];
		snap_frame_t actualFrame;
		const snap_frame_t expectedFrame = {.buffer = buffer, .maxSize = expectedMaxSize[i], .status = SNAP_STATUS_IDLE, .size = 0};

		TEST_ASSERT_EQUAL_INT16(expectedMaxSize[i], snap_init(&actualFrame, buffer, actualMaxSize[i]));
		TEST_ASSERT_EQUAL_PTR(expectedFrame.buffer, actualFrame.buffer);
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}
}


/******************************************************************************/
/*  TEST GROUP: reset                                                         */
/******************************************************************************/


TEST_GROUP(reset);

TEST_SETUP(reset) {}

TEST_TEAR_DOWN(reset) {}

TEST_GROUP_RUNNER(reset)
{
	RUN_TEST_CASE(reset, should_ResetFrameStatusAndSize);
}

TEST(reset, should_ResetFrameStatusAndSize)
{
	snap_frame_t actualFrame = {.buffer = (uint8_t [10]){1, 2, 3}, .maxSize = 10, .status = 11, .size = 12};
	const snap_frame_t expectedFrame = {.buffer = (uint8_t [10]){1, 2, 3}, .maxSize = 10, .status = SNAP_STATUS_IDLE, .size = 0};

	snap_reset(&actualFrame);
	TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
}


/******************************************************************************/
/*  TEST GROUP: decode                                                        */
/******************************************************************************/


TEST_GROUP(decode);

TEST_SETUP(decode) {}

TEST_TEAR_DOWN(decode) {}

TEST_GROUP_RUNNER(decode)
{
	RUN_TEST_CASE(decode, if_StatusIdle_should_StoreSyncByte_and_ChangeStatusToIncomplete_when_ReceiveSyncByte);
	RUN_TEST_CASE(decode, if_StatusIncomplete_should_StoreBytes_and_ChangeStatusToValid_when_ReceiveValidFrame);
	RUN_TEST_CASE(decode, if_StatusIncomplete_should_StoreBytes_and_ChangeStatusToErrorOverflow_when_ReceiveHdb1_and_BufferIsTooShort);
	RUN_TEST_CASE(decode, if_StatusIncomplete_should_StoreBytes_and_ChangeStatusToErrorHash_when_ReceiveInvalidHashValue);
	RUN_TEST_CASE(decode, if_StatusValid_should_NotChangeAnyVariables_and_IgnoreAllInputBytes);
	RUN_TEST_CASE(decode, if_StatusErrorHash_should_NotChangeAnyVariables_and_IgnoreAllInputBytes);
	RUN_TEST_CASE(decode, if_StatusErrorOverflow_should_NotChangeAnyVariables_and_IgnoreAllInputBytes);
}

TEST(decode, if_StatusIdle_should_StoreSyncByte_and_ChangeStatusToIncomplete_when_ReceiveSyncByte)
{
	test_decode_StatusIdle(NULL, 0);
	test_decode_StatusIdle((uint8_t []){0x69}, 1);
	test_decode_StatusIdle((uint8_t []){0x00, 0x10, 0x20, 0x30, 0x53, 0x55, 0xFF}, 7);
}

TEST(decode, if_StatusIncomplete_should_StoreBytes_and_ChangeStatusToValid_when_ReceiveValidFrame)
{
// Frames with sync, header ////////////////////////////////////////////////////

	// DAB=0, SAB=0, PFB=0, ACK=1, CMD=0, EDM=0, NDB=0
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x01, 0x00},
								 3, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=0, SAB=0, PFB=0, ACK=3, CMD=1, EDM=0, NDB=0
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x03, 0x80},
								 3, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab ///////////////////////////////////////////////

	// DAB=1, SAB=0, PFB=0, ACK=0, CMD=1, EDM=1, NDB=0, dAddr=0x05
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x40, 0x90, 0x05},
								 4, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=2, SAB=0, PFB=0, ACK=2, CMD=0, EDM=6, NDB=0, dAddr=0xA5B6
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x82, 0x60, 0xA5, 0xB6},
								 5, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=3, SAB=0, PFB=0, ACK=2, CMD=1, EDM=0, NDB=0, dAddr=0x000000
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xC2, 0x80, 0x00, 0x00, 0x00},
								 6, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, sab ///////////////////////////////////////////////

	// DAB=0, SAB=3, PFB=0, ACK=1, CMD=0, EDM=6, NDB=0, sAddr=0x0FFFFF
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x31, 0x60, 0x0F, 0xFF, 0xFF},
								 6, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=0, SAB=1, PFB=0, ACK=2, CMD=0, EDM=1, NDB=0, sAddr=0x80
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x12, 0x10, 0x80},
								 4, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, pfb, hash (user) //////////////////////////////////

	// DAB=0, SAB=0, PFB=3, ACK=1, CMD=0, EDM=7, NDB=0, flags=0x000009, hash=0x624627
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x0D, 0x70, 0x00, 0x00, 0x09, 0x62, 0x46, 0x27},
								 9, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, data //////////////////////////////////////////////

	// DAB=0, SAB=0, PFB=0, ACK=2, CMD=0, EDM=0, NDB=13, data[256]=0x00 00 00...
	test_decode_StatusIncomplete((uint8_t [259]){SNAP_SYNC, 0x02, 0x0D},
								 259, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, hash //////////////////////////////////////////////

	// DAB=0, SAB=0, PFB=0, ACK=0, CMD=0, EDM=4, NDB=0, hash=0x48C4
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x00, 0x40, 0x48, 0xC4},
								 5, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, sab //////////////////////////////////////////

	// DAB=2, SAB=1, PFB=0, ACK=3, CMD=0, EDM=1, NDB=0, dAddr=0x8000, sAddr=0x7F
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x93, 0x10, 0x80, 0x00, 0x7F},
								 6, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=1, SAB=3, PFB=0, ACK=0, CMD=0, EDM=0, NDB=0, dAddr=0x12, sAddr=0xFEDCBA
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x70, 0x00, 0x12, 0xFE, 0xDC, 0xBA},
								 7, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=3, SAB=2, PFB=0, ACK=0, CMD=1, EDM=1, NDB=0, dAddr=0xABCDEF, sAddr=0x3210
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xE0, 0x90, 0xAB, 0xCD, 0xEF, 0x32, 0x10},
								 8, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, pfb //////////////////////////////////////////

	// DAB=3, SAB=0, PFB=2, ACK=1, CMD=1, EDM=0, NDB=0, dAddr=0xCF9900, flags=0xFFFF
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xC9, 0x80, 0xCF, 0x99, 0x00, 0xFF, 0xFF},
								 8, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, sab, pfb /////////////////////////////////////

	// DAB=3, SAB=3, PFB=1, ACK=3, CMD=0, EDM=1, NDB=0, dAddr=0x010203, sAddr=0x040506, flags=0x55
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xF7, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x55},
								 10, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=1, SAB=1, PFB=3, ACK=0, CMD=0, EDM=6, NDB=0, dAddr=0xA0, sAddr=0xA0, flags=0xFFFFFF
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x5C, 0x60, 0xA0, 0xA0, 0xFF, 0xFF, 0xFF},
								 8, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, pfb, data ////////////////////////////////////

	// DAB=1, SAB=0, PFB=1, ACK=3, CMD=0, EDM=1, NDB=1, dAddr=0xF1, flags=0xF2, data[1]=0x69
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x47, 0x11, 0xF1, 0xF2, 0x69},
								 6, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=3, SAB=0, PFB=2, ACK=2, CMD=0, EDM=6, NDB=14, dAddr=0xFFFFFF, flags=0x0000, data[512]=0x0A 0B 0C 0D 0E 0F 00 00 00...
	test_decode_StatusIncomplete((uint8_t [520]){SNAP_SYNC, 0xCA, 0x6E, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F},
								 520, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, sab, pfb, data ////////////////////////////////////

	// DAB=0, SAB=2, PFB=1, ACK=1, CMD=0, EDM=6, NDB=10, sAddr=0xA0B1, flags=0xC2, data[32]=0xFF 01 80 00 00 00...
	test_decode_StatusIncomplete((uint8_t [38]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0xFF, 0x01, 0x80},
								 38, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, sab, data, hash (user) ///////////////////////

	// DAB=1, SAB=3, PFB=0, ACK=1, CMD=1, EDM=7, NDB=11, dAddr=0x09, sAddr=0x664700, hash=0xAAC097, data[64]=0x1F 2E 3D 00 00 00...
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x71, 0xFB, 0x09, 0x66, 0x47, 0x00, 0x1F, 0x2E, 0x3D, [71] = 0xAA, 0xC0, 0x97},
								 74, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, sab, pfb, data ///////////////////////////////

	// DAB=1, SAB=2, PFB=3, ACK=3, CMD=1, EDM=1, NDB=3, dAddr=0x01, sAddr=0x0202, flags=0x030303, data[3]=0x77 88 99
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x6F, 0x93, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x77, 0x88, 0x99},
								 12, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, sab, data, hash //////////////////////////////

	// DAB=3, SAB=2, PFB=0, ACK=1, CMD=0, EDM=2, NDB=5, dAddr=0x998877, sAddr=0xFEDC, hash=0xCC, data[5]=0xBA 62 63 51 84
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xE1, 0x25, 0x99, 0x88, 0x77, 0xFE, 0xDC, 0xBA, 0x62, 0x63, 0x51, 0x84, 0xCC},
								 14, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	// DAB=1, SAB=1, PFB=2, ACK=2, CMD=0, EDM=3, NDB=9, dAddr=0xA1, sAddr=0xB1, flags=0xC1C2, hash=0x4E, data[16]=0xD1 D2 D3 00 00 00...
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3, [23] = 0x4E},
								 24, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=2, SAB=2, PFB=2, ACK=0, CMD=0, EDM=5, NDB=12, dAddr=0x0001, sAddr=0x0002, flags=0x0003, hash=0x895817A7, data[128]=0xFF FF FF 00 00 00...
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xA8, 0x5C, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0xFF, 0xFF, 0xFF, [137] = 0x89, 0x58, 0x17, 0xA7},
								 141, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);
}

TEST(decode, if_StatusIncomplete_should_StoreBytes_and_ChangeStatusToErrorOverflow_when_ReceiveHdb1_and_BufferIsTooShort)
{
	// DAB=0, SAB=1, PFB=0, ACK=2, CMD=0, EDM=0, NDB=15, size=4, maxSize=3
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x12, 0x0F},
								 3, SNAP_STATUS_ERROR_OVERFLOW, SNAP_MIN_SIZE_FRAME);

	// DAB=3, SAB=3, PFB=3, ACK=0, CMD=0, EDM=5, NDB=14, size=528, maxSize=527
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xFC, 0x5E},
								 3, SNAP_STATUS_ERROR_OVERFLOW, SNAP_MAX_SIZE_FRAME - 1);

	// DAB=2, SAB=1, PFB=0, ACK=1, CMD=0, EDM=5, NDB=12, size=138, maxSize=137
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x91, 0x5C},
								 3, SNAP_STATUS_ERROR_OVERFLOW, 137);

	// DAB=0, SAB=0, PFB=1, ACK=0, CMD=1, EDM=7, NDB=13, size=263, maxSize=262
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x04, 0xFD},
								 3, SNAP_STATUS_ERROR_OVERFLOW, 262);
}

TEST(decode, if_StatusIncomplete_should_StoreBytes_and_ChangeStatusToErrorHash_when_ReceiveInvalidHashValue)
{
	// DAB=0, SAB=0, PFB=0, ACK=0, CMD=0, EDM=4, NDB=0, hash=0x48C5(!=0x48C4)
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x00, 0x40, 0x48, 0xC5},
								 5, SNAP_STATUS_ERROR_HASH, SNAP_MAX_SIZE_FRAME);

	// DAB=1, SAB=1, PFB=2, ACK=2, CMD=0, EDM=3, NDB=9, dAddr=0xA1, sAddr=0xB1, flags=0xC1C2, hash=0x4F(!=0x4E), data[16]=0xD1 D2 D3 00 00 00...
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3, [23] = 0x4F},
								 24, SNAP_STATUS_ERROR_HASH, SNAP_MAX_SIZE_FRAME);

	// DAB=3, SAB=2, PFB=0, ACK=1, CMD=0, EDM=2, NDB=5, dAddr=0x998877, sAddr=0xFEDC, hash=0xCD(!=0xCC), data[5]=0xBA 62 63 51 84
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xE1, 0x25, 0x99, 0x88, 0x77, 0xFE, 0xDC, 0xBA, 0x62, 0x63, 0x51, 0x84, 0xCD},
								 14, SNAP_STATUS_ERROR_HASH, SNAP_MAX_SIZE_FRAME);

	// DAB=2, SAB=2, PFB=2, ACK=0, CMD=0, EDM=5, NDB=12, dAddr=0x0001, sAddr=0x0002, flags=0x0003, hash=0x895817A8(!=0x895817A7), data[128]=0xFF FF FF 00 00 00...
	test_decode_StatusIncomplete((uint8_t []){SNAP_SYNC, 0xA8, 0x5C, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0xFF, 0xFF, 0xFF, [137] = 0x89, 0x58, 0x17, 0xA8},
								 141, SNAP_STATUS_ERROR_HASH, SNAP_MAX_SIZE_FRAME);
}

TEST(decode, if_StatusValid_should_NotChangeAnyVariables_and_IgnoreAllInputBytes)
{
	// DAB=0, SAB=0, PFB=0, ACK=1, CMD=0, EDM=0, NDB=0
	test_decode_StatusValidOrError((uint8_t []){SNAP_SYNC, 0x01, 0x00}, 3,
								   (uint8_t [7]){0x01, 0x02, 0x03, 0x04, 0x05}, 7, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);

	// DAB=0, SAB=2, PFB=1, ACK=1, CMD=0, EDM=6, NDB=10, sAddr=0xA0B1, flags=0xC2, data[32]=0xFF 01 80 00 00 00...
	test_decode_StatusValidOrError((uint8_t [38]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0xFF, 0x01, 0x80}, 38,
								   (uint8_t [30]){0x11, 0x22, 0x33}, 30, SNAP_STATUS_VALID, SNAP_MAX_SIZE_FRAME);
}

TEST(decode, if_StatusErrorHash_should_NotChangeAnyVariables_and_IgnoreAllInputBytes)
{
	// DAB=1, SAB=1, PFB=2, ACK=2, CMD=0, EDM=3, NDB=9, dAddr=0xA1, sAddr=0xB1, flags=0xC1C2, hash=0x4F(!=0x4E), data[16]=0xD1 D2 D3 00 00 00...
	test_decode_StatusValidOrError((uint8_t []){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3, [23] = 0x4F}, 24,
								   (uint8_t [10]){0xFF}, 10, SNAP_STATUS_ERROR_HASH, SNAP_MAX_SIZE_FRAME);

	// DAB=0, SAB=0, PFB=0, ACK=0, CMD=0, EDM=4, NDB=0, hash=0x48C5(!=0x48C4)
	test_decode_StatusValidOrError((uint8_t []){SNAP_SYNC, 0x00, 0x40, 0x48, 0xC5}, 5,
								   (uint8_t [5]){0x54, 0x55, 0x56}, 5, SNAP_STATUS_ERROR_HASH, SNAP_MAX_SIZE_FRAME);
}

TEST(decode, if_StatusErrorOverflow_should_NotChangeAnyVariables_and_IgnoreAllInputBytes)
{
	// DAB=3, SAB=3, PFB=3, ACK=0, CMD=0, EDM=5, NDB=14, size=528, maxSize=527
	test_decode_StatusValidOrError((uint8_t []){SNAP_SYNC, 0xFC, 0x5E}, 3,
								   (uint8_t []){0x01, 0x02, 0x54, 0x04, 0x05}, 5, SNAP_STATUS_ERROR_OVERFLOW, SNAP_MAX_SIZE_FRAME - 1);

	// DAB=2, SAB=1, PFB=0, ACK=1, CMD=0, EDM=5, NDB=12, size=138, maxSize=137
	test_decode_StatusValidOrError((uint8_t []){SNAP_SYNC, 0x91, 0x5C}, 3,
								   (uint8_t [20]){0xFF, 0xEE, 0x54}, 20, SNAP_STATUS_ERROR_OVERFLOW, 137);
}


/******************************************************************************/
/*  TEST GROUP: encapsulate                                                   */
/******************************************************************************/


TEST_GROUP(encapsulate);

TEST_SETUP(encapsulate) {}

TEST_TEAR_DOWN(encapsulate) {}

TEST_GROUP_RUNNER(encapsulate)
{
	RUN_TEST_CASE(encapsulate, should_BuildFrame_and_ChangeStatusToValid_if_BufferHasEnoughSpace);
	RUN_TEST_CASE(encapsulate, should_BuildFrame_and_ChangeStatusToValid_if_BufferHasEnoughSpace_and_UseSameArrayAsDataAndFrameBuffer);
	RUN_TEST_CASE(encapsulate, should_ChangeStatusToErrorOverflow_if_BufferDoesNotHaveEnoughSpace);
}

TEST(encapsulate, should_BuildFrame_and_ChangeStatusToValid_if_BufferHasEnoughSpace)
{
// Frames with sync, header ////////////////////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 0, .pfb = 0, .ack = 1, .cmd = 0, .edm = 0},
									  .dataSize = 0},
					 &(snap_frame_t){.size = 3, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x01, 0x00}});

// Frames with sync, header, dab ///////////////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 0, .pfb = 0, .ack = 0, .cmd = 1, .edm = 1},
									  .dataSize = 0, .destAddress = 0x05},
					 &(snap_frame_t){.size = 4, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x40, 0x90, 0x05}});

// Frames with sync, header, sab ///////////////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 3, .pfb = 0, .ack = 1, .cmd = 0, .edm = 6},
									  .dataSize = 0, .sourceAddress = 0x0FFFFF},
					 &(snap_frame_t){.size = 6, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x31, 0x60, 0x0F, 0xFF, 0xFF}});

// Frames with sync, header, pfb, hash (user) //////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 0, .pfb = 3, .ack = 1, .cmd = 0, .edm = 7},
									  .dataSize = 0, .protocolFlags = 0x000009},
					 &(snap_frame_t){.size = 9, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x0D, 0x70, 0x00, 0x00, 0x09, 0x62, 0x46, 0x27}});

// Frames with sync, header, data //////////////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 0, .pfb = 0, .ack = 2, .cmd = 0, .edm = 0},
									  .dataSize = 200, .data = (uint8_t [200]){0x01}, .paddingAfter = true},
					 &(snap_frame_t){.size = 259, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t [259]){SNAP_SYNC, 0x02, 0x0D, 0x01}});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 0, .pfb = 0, .ack = 2, .cmd = 0, .edm = 0},
									  .dataSize = 200, .data = (uint8_t [200]){0x01}, .paddingAfter = false},
					 &(snap_frame_t){.size = 259, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t [259]){SNAP_SYNC, 0x02, 0x0D, [59] = 0x01}});

// Frames with sync, header, hash //////////////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 0, .pfb = 0, .ack = 0, .cmd = 0, .edm = 4},
									  .dataSize = 0},
					 &(snap_frame_t){.size = 5, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x00, 0x40, 0x48, 0xC4}});

// Frames with sync, header, dab, sab //////////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 2, .sab = 1, .pfb = 0, .ack = 3, .cmd = 0, .edm = 1},
									  .dataSize = 0, .destAddress = 0x8000, .sourceAddress = 0x7F},
					 &(snap_frame_t){.size = 6, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x93, 0x10, 0x80, 0x00, 0x7F}});

// Frames with sync, header, dab, pfb //////////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 3, .sab = 0, .pfb = 2, .ack = 1, .cmd = 1, .edm = 0},
									  .dataSize = 0, .destAddress = 0xCF9900, .protocolFlags = 0xFFFF},
					 &(snap_frame_t){.size = 8, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0xC9, 0x80, 0xCF, 0x99, 0x00, 0xFF, 0xFF}});

// Frames with sync, header, dab, sab, pfb /////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 3, .sab = 3, .pfb = 1, .ack = 3, .cmd = 0, .edm = 1},
									  .dataSize = 0, .destAddress = 0x010203, .sourceAddress = 0x040506, .protocolFlags = 0x55},
					 &(snap_frame_t){.size = 10, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0xF7, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x55}});

// Frames with sync, header, dab, pfb, data ////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 0, .pfb = 1, .ack = 3, .cmd = 0, .edm = 1},
									  .destAddress = 0xF1, .protocolFlags = 0xF2, .dataSize = 1, .data = (uint8_t []){0x69}, .paddingAfter = true},
					 &(snap_frame_t){.size = 6, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x47, 0x11, 0xF1, 0xF2, 0x69}});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 0, .pfb = 1, .ack = 3, .cmd = 0, .edm = 1},
									  .destAddress = 0xF1, .protocolFlags = 0xF2, .dataSize = 1, .data = (uint8_t []){0x69}, .paddingAfter = false},
					 &(snap_frame_t){.size = 6, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x47, 0x11, 0xF1, 0xF2, 0x69}});

// Frames with sync, header, sab, pfb, data ////////////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 2, .pfb = 1, .ack = 1, .cmd = 0, .edm = 6},
									  .sourceAddress = 0xA0B1, .protocolFlags = 0xC2, .dataSize = 31, .data = (uint8_t [31]){0xFF, 0x01, 0x80}, .paddingAfter = true},
					 &(snap_frame_t){.size = 38, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t [38]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0xFF, 0x01, 0x80}});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 2, .pfb = 1, .ack = 1, .cmd = 0, .edm = 6},
									  .sourceAddress = 0xA0B1, .protocolFlags = 0xC2, .dataSize = 31, .data = (uint8_t [31]){0xFF, 0x01, 0x80}, .paddingAfter = false},
					 &(snap_frame_t){.size = 38, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t [38]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0x00, 0xFF, 0x01, 0x80}});

// Frames with sync, header, dab, sab, data, hash (user) ///////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 3, .pfb = 0, .ack = 1, .cmd = 1, .edm = 7},
									  .destAddress = 0x09, .sourceAddress = 0x664700, .dataSize = 60, .data = (uint8_t [60]){0x1F, 0x2E, 0x3D}, .paddingAfter = true},
					 &(snap_frame_t){.size = 74, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x71, 0xFB, 0x09, 0x66, 0x47, 0x00, 0x1F, 0x2E, 0x3D, [71] = 0xAA, 0xC0, 0x97}});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 3, .pfb = 0, .ack = 1, .cmd = 1, .edm = 7},
									  .destAddress = 0x09, .sourceAddress = 0x664700, .dataSize = 60, .data = (uint8_t [60]){0x1F, 0x2E, 0x3D}, .paddingAfter = false},
					 &(snap_frame_t){.size = 74, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x71, 0xFB, 0x09, 0x66, 0x47, 0x00, [11] = 0x1F, 0x2E, 0x3D, [71] = 0x61, 0x14, 0xBB}});

// Frames with sync, header, dab, sab, pfb, data ///////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 2, .pfb = 3, .ack = 3, .cmd = 1, .edm = 1},
									  .destAddress = 0x01, .sourceAddress = 0x0202, .protocolFlags = 0x030303, .dataSize = 3, .data = (uint8_t []){0x77, 0x88, 0x99}, .paddingAfter = true},
					 &(snap_frame_t){.size = 12, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x6F, 0x93, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x77, 0x88, 0x99}});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 2, .pfb = 3, .ack = 3, .cmd = 1, .edm = 1},
									  .destAddress = 0x01, .sourceAddress = 0x0202, .protocolFlags = 0x030303, .dataSize = 3, .data = (uint8_t []){0x77, 0x88, 0x99}, .paddingAfter = false},
					 &(snap_frame_t){.size = 12, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x6F, 0x93, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x77, 0x88, 0x99}});

// Frames with sync, header, dab, sab, data, hash //////////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 3, .sab = 2, .pfb = 0, .ack = 1, .cmd = 0, .edm = 2},
									  .destAddress = 0x998877, .sourceAddress = 0xFEDC, .dataSize = 5, .data = (uint8_t []){0xBA, 0x62, 0x63, 0x51, 0x84}, .paddingAfter = true},
					 &(snap_frame_t){.size = 14, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0xE1, 0x25, 0x99, 0x88, 0x77, 0xFE, 0xDC, 0xBA, 0x62, 0x63, 0x51, 0x84, 0xCC}});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 3, .sab = 2, .pfb = 0, .ack = 1, .cmd = 0, .edm = 2},
									  .destAddress = 0x998877, .sourceAddress = 0xFEDC, .dataSize = 5, .data = (uint8_t []){0xBA, 0x62, 0x63, 0x51, 0x84}, .paddingAfter = false},
					 &(snap_frame_t){.size = 14, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0xE1, 0x25, 0x99, 0x88, 0x77, 0xFE, 0xDC, 0xBA, 0x62, 0x63, 0x51, 0x84, 0xCC}});

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 1, .pfb = 2, .ack = 2, .cmd = 0, .edm = 3},
									  .destAddress = 0xA1, .sourceAddress = 0xB1, .protocolFlags = 0xC1C2, .dataSize = 9, .data = (uint8_t [9]){0xD1, 0xD2, 0xD3}, .paddingAfter = true},
					 &(snap_frame_t){.size = 24, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3, [23] = 0x4E}});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 1, .sab = 1, .pfb = 2, .ack = 2, .cmd = 0, .edm = 3},
									  .destAddress = 0xA1, .sourceAddress = 0xB1, .protocolFlags = 0xC1C2, .dataSize = 9, .data = (uint8_t [9]){0xD1, 0xD2, 0xD3}, .paddingAfter = false},
					 &(snap_frame_t){.size = 24, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
									 .buffer = (uint8_t []){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, [14] = 0xD1, 0xD2, 0xD3, [23] = 0x50}});
}

TEST(encapsulate, should_BuildFrame_and_ChangeStatusToValid_if_BufferHasEnoughSpace_and_UseSameArrayAsDataAndFrameBuffer)
{
// Frames with sync, header, data //////////////////////////////////////////////

	{
		uint8_t data[259] = {0x11};

		snap_fields_t fields = {.header = {.dab = 0, .sab = 0, .pfb = 0, .ack = 2, .cmd = 0, .edm = 0},
								.dataSize = 200, .data = data, .paddingAfter = true};

		const snap_frame_t expectedFrame = {.size = 259, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
											.buffer = (uint8_t [259]){SNAP_SYNC, 0x02, 0x0D, 0x11}};

		snap_frame_t actualFrame = {.buffer = data, .maxSize = expectedFrame.maxSize, .status = SNAP_STATUS_IDLE, .size = 0};

		TEST_ASSERT_EQUAL_INT8(expectedFrame.status, snap_encapsulate(&actualFrame, &fields));
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}

// Frames with sync, header, sab, pfb, data ////////////////////////////////////

	{
		uint8_t data[38] = {0xFF, 0x01, 0x80};

		snap_fields_t fields = {.header = {.dab = 0, .sab = 2, .pfb = 1, .ack = 1, .cmd = 0, .edm = 6},
								.sourceAddress = 0xA0B1, .protocolFlags = 0xC2, .dataSize = 31, .data = data, .paddingAfter = false};

		const snap_frame_t expectedFrame = {.size = 38, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
											.buffer = (uint8_t [38]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0x00, 0xFF, 0x01, 0x80}};

		snap_frame_t actualFrame = {.buffer = data, .maxSize = expectedFrame.maxSize, .status = SNAP_STATUS_IDLE, .size = 0};

		TEST_ASSERT_EQUAL_INT8(expectedFrame.status, snap_encapsulate(&actualFrame, &fields));
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	{
		uint8_t data[SNAP_MAX_SIZE_FRAME] = {0xD1, 0xD2, 0xD3};

		snap_fields_t fields = {.header = {.dab = 1, .sab = 1, .pfb = 2, .ack = 2, .cmd = 0, .edm = 3},
								.destAddress = 0xA1, .sourceAddress = 0xB1, .protocolFlags = 0xC1C2, .dataSize = 9, .data = data, .paddingAfter = true};

		const snap_frame_t expectedFrame = {.size = 24, .status = SNAP_STATUS_VALID, .maxSize = SNAP_MAX_SIZE_FRAME,
											.buffer = (uint8_t []){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3, [23] = 0x4E}};

		snap_frame_t actualFrame = {.buffer = data, .maxSize = expectedFrame.maxSize, .status = SNAP_STATUS_IDLE, .size = 0};

		TEST_ASSERT_EQUAL_INT8(expectedFrame.status, snap_encapsulate(&actualFrame, &fields));
		TEST_ASSERT_EQUAL_FRAME(&expectedFrame, &actualFrame);
	}
}

TEST(encapsulate, should_ChangeStatusToErrorOverflow_if_BufferDoesNotHaveEnoughSpace)
{
	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 1, .pfb = 0, .ack = 2, .cmd = 0, .edm = 0}, .dataSize = 0},
					 &(snap_frame_t){.size = 0, .status = SNAP_STATUS_ERROR_OVERFLOW, .maxSize = SNAP_MIN_SIZE_FRAME});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 3, .sab = 3, .pfb = 3, .ack = 0, .cmd = 0, .edm = 5}, .dataSize = 512, .data = (uint8_t [512]){0x01}, .paddingAfter = true},
					 &(snap_frame_t){.size = 0, .status = SNAP_STATUS_ERROR_OVERFLOW, .maxSize = SNAP_MAX_SIZE_FRAME - 1});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 2, .sab = 1, .pfb = 0, .ack = 1, .cmd = 0, .edm = 5}, .dataSize = 100, .data = (uint8_t [100]){0x02}, .paddingAfter = false},
					 &(snap_frame_t){.size = 0, .status = SNAP_STATUS_ERROR_OVERFLOW, .maxSize = 137});

	test_encapsulate(&(snap_fields_t){.header = {.dab = 0, .sab = 0, .pfb = 1, .ack = 0, .cmd = 1, .edm = 7}, .dataSize = 250, .data = (uint8_t [250]){0x03}, .paddingAfter = true},
					 &(snap_frame_t){.size = 0, .status = SNAP_STATUS_ERROR_OVERFLOW, .maxSize = 262});
}


/******************************************************************************/
/*  TEST GROUP: getField                                                      */
/******************************************************************************/


TEST_GROUP(getField);

TEST_SETUP(getField) {}

TEST_TEAR_DOWN(getField) {}

TEST_GROUP_RUNNER(getField)
{
	RUN_TEST_CASE(getField, should_ReturnFieldSizeAndValue_if_FrameHasRequestedField);
	RUN_TEST_CASE(getField, should_ReturnErrorUnknownFormat_and_NotChangeFieldValue_if_FrameDoesNotHaveCompleteHeader);
	RUN_TEST_CASE(getField, should_ReturnErrorFieldType_and_NotChangeFieldValue_if_FieldTypeIsInvalid);
	RUN_TEST_CASE(getField, should_ReturnErrorFrameFormat_and_NotChangeFieldValue_if_FrameFormatDoesNotHaveField);
	RUN_TEST_CASE(getField, should_ReturnErrorShortFrame_and_NotChangeFieldValue_if_FrameFormatHasFieldButFrameDoesNotHaveEnoughBytes);
}

TEST(getField, should_ReturnFieldSizeAndValue_if_FrameHasRequestedField)
{
// Frames with sync, header, dab ///////////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 4, .status = SNAP_STATUS_VALID, .maxSize = 10,
									.buffer = (uint8_t [10]){SNAP_SYNC, 0x40, 0x90, 0x05}};
		snap_header_t header = {0};
		uint32_t destAddress = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 1, .sab = 0, .pfb = 0, .ack = 0, .cmd = 1, .edm = 1, .ndb = 0}), &header);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &destAddress, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0x05, destAddress);
	}

// Frames with sync, header, sab ///////////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 6, .status = SNAP_STATUS_VALID, .maxSize = 11,
									.buffer = (uint8_t [11]){SNAP_SYNC, 0x31, 0x60, 0x0F, 0xFF, 0xFF}};
		snap_header_t header = {0};
		uint32_t sourceAddress = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 0, .sab = 3, .pfb = 0, .ack = 1, .cmd = 0, .edm = 6, .ndb = 0}), &header);

		TEST_ASSERT_EQUAL_INT16(3, snap_getField(&frame, &sourceAddress, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0x0FFFFF, sourceAddress);
	}

// Frames with sync, header, pfb, hash (user) //////////////////////////////////

	{
		const snap_frame_t frame = {.size = 9, .status = SNAP_STATUS_VALID, .maxSize = 20,
									.buffer = (uint8_t [20]){SNAP_SYNC, 0x0D, 0x70, 0x00, 0x00, 0x09, 0x62, 0x46, 0x27}};
		snap_header_t header = {0};
		uint32_t protocolFlags = 0;
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 0, .sab = 0, .pfb = 3, .ack = 1, .cmd = 0, .edm = 7, .ndb = 0}), &header);

		TEST_ASSERT_EQUAL_INT16(3, snap_getField(&frame, &protocolFlags, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0x000009, protocolFlags);

		TEST_ASSERT_EQUAL_INT16(3, snap_getField(&frame, &hash, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0x624627, hash);
	}

// Frames with sync, header, hash //////////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 5, .status = SNAP_STATUS_VALID, .maxSize = 13,
									.buffer = (uint8_t [13]){SNAP_SYNC, 0x00, 0x40, 0x48, 0xC4}};
		snap_header_t header = {0};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 0, .sab = 0, .pfb = 0, .ack = 0, .cmd = 0, .edm = 4, .ndb = 0}), &header);

		TEST_ASSERT_EQUAL_INT16(2, snap_getField(&frame, &hash, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0x48C4, hash);
	}

// Frames with sync, header, dab, sab //////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 6, .status = SNAP_STATUS_VALID, .maxSize = 14,
									.buffer = (uint8_t [14]){SNAP_SYNC, 0x93, 0x10, 0x80, 0x00, 0x7F}};
		snap_header_t header = {0};
		uint32_t destAddress = 0, sourceAddress = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 2, .sab = 1, .pfb = 0, .ack = 3, .cmd = 0, .edm = 1, .ndb = 0}), &header);

		TEST_ASSERT_EQUAL_INT16(2, snap_getField(&frame, &destAddress, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0x8000, destAddress);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &sourceAddress, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0x7F, sourceAddress);
	}

// Frames with sync, header, dab, pfb //////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 8, .status = SNAP_STATUS_VALID, .maxSize = 15,
									.buffer = (uint8_t [15]){SNAP_SYNC, 0xC9, 0x80, 0xCF, 0x99, 0x00, 0xFF, 0xFF}};
		snap_header_t header = {0};
		uint32_t destAddress = 0, protocolFlags = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 3, .sab = 0, .pfb = 2, .ack = 1, .cmd = 1, .edm = 0, .ndb = 0}), &header);

		TEST_ASSERT_EQUAL_INT16(3, snap_getField(&frame, &destAddress, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0xCF9900, destAddress);

		TEST_ASSERT_EQUAL_INT16(2, snap_getField(&frame, &protocolFlags, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0xFFFF, protocolFlags);
	}

// Frames with sync, header, dab, sab, pfb /////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 10, .status = SNAP_STATUS_VALID, .maxSize = 16,
									.buffer = (uint8_t [16]){SNAP_SYNC, 0xF7, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x55}};
		snap_header_t header = {0};
		uint32_t destAddress = 0, sourceAddress = 0, protocolFlags = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 3, .sab = 3, .pfb = 1, .ack = 3, .cmd = 0, .edm = 1, .ndb = 0}), &header);

		TEST_ASSERT_EQUAL_INT16(3, snap_getField(&frame, &destAddress, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0x010203, destAddress);

		TEST_ASSERT_EQUAL_INT16(3, snap_getField(&frame, &sourceAddress, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0x040506, sourceAddress);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &protocolFlags, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0x55, protocolFlags);
	}

// Frames with sync, header, sab, pfb, data ////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 38, .status = SNAP_STATUS_VALID, .maxSize = 50,
									.buffer = (uint8_t [50]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0xFF, 0x01, 0x80}};
		snap_header_t header = {0};
		uint32_t sourceAddress = 0, protocolFlags = 0;
		uint8_t data[50] = {0};

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 0, .sab = 2, .pfb = 1, .ack = 1, .cmd = 0, .edm = 6, .ndb = 10}), &header);

		TEST_ASSERT_EQUAL_INT16(2, snap_getField(&frame, &sourceAddress, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0xA0B1, sourceAddress);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &protocolFlags, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0xC2, protocolFlags);

		TEST_ASSERT_EQUAL_INT16(32, snap_getField(&frame, data, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX8_ARRAY(((uint8_t [32]){0xFF, 0x01, 0x80}), data, 32);
	}

// Frames with sync, header, dab, sab, data, hash //////////////////////////////

	{
		const snap_frame_t frame = {.size = 14, .status = SNAP_STATUS_VALID, .maxSize = 20,
									.buffer = (uint8_t [20]){SNAP_SYNC, 0xE1, 0x25, 0x99, 0x88, 0x77, 0xFE, 0xDC, 0xBA, 0x62, 0x63, 0x51, 0x84, 0xCC}};
		snap_header_t header = {0};
		uint32_t destAddress = 0, sourceAddress = 0, hash = 0;
		uint8_t data[20] = {0};

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 3, .sab = 2, .pfb = 0, .ack = 1, .cmd = 0, .edm = 2, .ndb = 5}), &header);

		TEST_ASSERT_EQUAL_INT16(3, snap_getField(&frame, &destAddress, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0x998877, destAddress);

		TEST_ASSERT_EQUAL_INT16(2, snap_getField(&frame, &sourceAddress, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0xFEDC, sourceAddress);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &hash, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0xCC, hash);

		TEST_ASSERT_EQUAL_INT16(5, snap_getField(&frame, data, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX8_ARRAY(((uint8_t []){0xBA, 0x62, 0x63, 0x51, 0x84}), data, 5);
	}

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	{
		const snap_frame_t frame = {.size = 24, .status = SNAP_STATUS_VALID, .maxSize = 30,
									.buffer = (uint8_t [30]){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3, [23] = 0x4E}};
		snap_header_t header = {0};
		uint32_t destAddress = 0, sourceAddress = 0, protocolFlags = 0, hash = 0;
		uint8_t data[30] = {0};

		TEST_ASSERT_EQUAL_INT16(SNAP_SIZE_HEADER, snap_getField(&frame, &header, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEADER((&(snap_header_t){.dab = 1, .sab = 1, .pfb = 2, .ack = 2, .cmd = 0, .edm = 3, .ndb = 9}), &header);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &destAddress, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0xA1, destAddress);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &sourceAddress, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0xB1, sourceAddress);

		TEST_ASSERT_EQUAL_INT16(2, snap_getField(&frame, &protocolFlags, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0xC1C2, protocolFlags);

		TEST_ASSERT_EQUAL_INT16(1, snap_getField(&frame, &hash, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0x4E, hash);

		TEST_ASSERT_EQUAL_INT16(16, snap_getField(&frame, data, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX8_ARRAY(((uint8_t [16]){0xD1, 0xD2, 0xD3}), data, 16);
	}
}

TEST(getField, should_ReturnErrorUnknownFormat_and_NotChangeFieldValue_if_FrameDoesNotHaveCompleteHeader)
{
	{	// Empty frame
		const snap_frame_t frame = {.size = 0, .status = SNAP_STATUS_IDLE, .maxSize = 5, .buffer = (uint8_t [5]){0}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_UNKNOWN_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_UNKNOWN_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_UNKNOWN_FORMAT, snap_getField(&frame, &field, 0xFF));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}
	{	// Incomplete header
		const snap_frame_t frame = {.size = 2, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 10, .buffer = (uint8_t [10]){SNAP_SYNC, 0x12}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_UNKNOWN_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HEADER));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_UNKNOWN_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_UNKNOWN_FORMAT, snap_getField(&frame, &field, 0xFF));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}
}

TEST(getField, should_ReturnErrorFieldType_and_NotChangeFieldValue_if_FieldTypeIsInvalid)
{
	const snap_frame_t frame = {.size = 24, .status = SNAP_STATUS_VALID, .maxSize = 30,
								.buffer = (uint8_t [30]){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3, [23] = 0x4E}};
	uint32_t field = 0;

	TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FIELD_TYPE, snap_getField(&frame, &field, SNAP_FIELD_HASH + 1));
	TEST_ASSERT_EQUAL_HEX32(0, field);

	TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FIELD_TYPE, snap_getField(&frame, &field, 0xFF));
	TEST_ASSERT_EQUAL_HEX32(0, field);

	TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FIELD_TYPE, snap_getField(&frame, &field, 10));
	TEST_ASSERT_EQUAL_HEX32(0, field);

	TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FIELD_TYPE, snap_getField(&frame, &field, 100));
	TEST_ASSERT_EQUAL_HEX32(0, field);
}

TEST(getField, should_ReturnErrorFrameFormat_and_NotChangeFieldValue_if_FrameFormatDoesNotHaveField)
{
// Frames with sync, header, dab ///////////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 3, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 10,
									.buffer = (uint8_t [10]){SNAP_SYNC, 0x40, 0x90}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}

// Frames with sync, header, dab, sab //////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 6, .status = SNAP_STATUS_VALID, .maxSize = 11,
									.buffer = (uint8_t [11]){SNAP_SYNC, 0x93, 0x10, 0x80, 0x00, 0x7F}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}

// Frames with sync, header, dab, sab, pfb /////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 9, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 12,
									.buffer = (uint8_t [12]){SNAP_SYNC, 0xF7, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}

// Frames with sync, header, sab ///////////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 6, .status = SNAP_STATUS_VALID, .maxSize = 13,
									.buffer = (uint8_t [13]){SNAP_SYNC, 0x31, 0x60, 0x0F, 0xFF, 0xFF}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}

// Frames with sync, header, sab, pfb, data ////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 38, .status = SNAP_STATUS_VALID, .maxSize = 50,
									.buffer = (uint8_t [50]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0xFF, 0x01, 0x80}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_FRAME_FORMAT, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}
}

TEST(getField, should_ReturnErrorShortFrame_and_NotChangeFieldValue_if_FrameFormatHasFieldButFrameDoesNotHaveEnoughBytes)
{
// Frames with sync, header, dab ///////////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 3, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 15,
									.buffer = (uint8_t [15]){SNAP_SYNC, 0x40, 0x90}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}

// Frames with sync, header, dab, sab //////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 4, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 25,
									.buffer = (uint8_t [25]){SNAP_SYNC, 0x93, 0x10, 0x80}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_DEST_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}

// Frames with sync, header, dab, sab, pfb /////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 8, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 20,
									.buffer = (uint8_t [20]){SNAP_SYNC, 0xF7, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	{
		const snap_frame_t frame = {.size = 4, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 15,
									.buffer = (uint8_t [10]){SNAP_SYNC, 0x5A, 0x39, 0xA1}};
		uint32_t field = 0;

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_SOURCE_ADDRESS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_PROTOCOL_FLAGS));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_DATA));
		TEST_ASSERT_EQUAL_HEX32(0, field);

		TEST_ASSERT_EQUAL_INT16(SNAP_ERROR_SHORT_FRAME, snap_getField(&frame, &field, SNAP_FIELD_HASH));
		TEST_ASSERT_EQUAL_HEX32(0, field);
	}
}


/******************************************************************************/
/*  TEST GROUP: calculateHash                                                 */
/******************************************************************************/


TEST_GROUP(calculateHash);

TEST_SETUP(calculateHash) {}

TEST_TEAR_DOWN(calculateHash) {}

TEST_GROUP_RUNNER(calculateHash)
{
	RUN_TEST_CASE(calculateHash, should_ReturnHashSizeAndValue_if_FrameFormatHasHash_and_FrameHasEnoughBytes);
	RUN_TEST_CASE(calculateHash, should_ReturnErrorUnknownFormat_and_NotChangeHashValue_if_FrameDoesNotHaveCompleteHeader);
	RUN_TEST_CASE(calculateHash, should_ReturnErrorFrameFormat_and_NotChangeHashValue_if_FrameFormatDoesNotHaveHash);
	RUN_TEST_CASE(calculateHash, should_ReturnErrorShortFrame_and_NotChangeHashValue_if_FrameFormatHasHashButFrameDoesNotHaveEnoughBytes);
}

TEST(calculateHash, should_ReturnHashSizeAndValue_if_FrameFormatHasHash_and_FrameHasEnoughBytes)
{
// Frames with sync, header, dab, sab, data, hash //////////////////////////////

	{	// 8-bit checksum
		const snap_frame_t frame = {.size = 14, .status = SNAP_STATUS_VALID, .maxSize = 20,
									.buffer = (uint8_t [20]){SNAP_SYNC, 0xE1, 0x25, 0x99, 0x88, 0x77, 0xFE, 0xDC, 0xBA, 0x62, 0x63, 0x51, 0x84, 0xCC}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(1, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0xCC, hash);
	}

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	{	// 8-bit CRC
		const snap_frame_t frame = {.size = 23, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 30,
									.buffer = (uint8_t [30]){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(1, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0x4E, hash);
	}

// Frames with sync, header, hash //////////////////////////////////////////////

	{	// 16-bit CRC
		const snap_frame_t frame = {.size = 3, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 10,
									.buffer = (uint8_t [10]){SNAP_SYNC, 0x00, 0x40}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(2, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0x48C4, hash);
	}

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	{	// 32-bit CRC
		const snap_frame_t frame = {.size = 141, .status = SNAP_STATUS_VALID, .maxSize = 150,
									.buffer = (uint8_t [150]){SNAP_SYNC, 0xA8, 0x5C, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0xFF, 0xFF, 0xFF, [137] = 0x89, 0x58, 0x17, 0xA7}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(4, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0x895817A7, hash);
	}

// Frames with sync, header, pfb, hash (user) //////////////////////////////////

	{	// 24-bit CRC (user)
		const snap_frame_t frame = {.size = 9, .status = SNAP_STATUS_VALID, .maxSize = 15,
									.buffer = (uint8_t [15]){SNAP_SYNC, 0x0D, 0x70, 0x00, 0x00, 0x09, 0x62, 0x46, 0x27}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(3, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0x624627, hash);
	}
}

TEST(calculateHash, should_ReturnErrorUnknownFormat_and_NotChangeHashValue_if_FrameDoesNotHaveCompleteHeader)
{
	{	// Empty frame
		const snap_frame_t frame = {.size = 0, .status = SNAP_STATUS_IDLE, .maxSize = 5, .buffer = (uint8_t [5]){0}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_UNKNOWN_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_UNKNOWN_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_UNKNOWN_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}
	{	// Incomplete header
		const snap_frame_t frame = {.size = 2, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 10, .buffer = (uint8_t [10]){SNAP_SYNC, 0x12}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_UNKNOWN_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_UNKNOWN_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_UNKNOWN_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}
}

TEST(calculateHash, should_ReturnErrorFrameFormat_and_NotChangeHashValue_if_FrameFormatDoesNotHaveHash)
{
// Frames with sync, header, dab ///////////////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 4, .status = SNAP_STATUS_VALID, .maxSize = 10,
									.buffer = (uint8_t [10]){SNAP_SYNC, 0x40, 0x90, 0x05}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_FRAME_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}

// Frames with sync, header, dab, sab, pfb /////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 10, .status = SNAP_STATUS_VALID, .maxSize = 16,
									.buffer = (uint8_t [16]){SNAP_SYNC, 0xF7, 0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x55}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_FRAME_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}

// Frames with sync, header, sab, pfb, data ////////////////////////////////////

	{
		const snap_frame_t frame = {.size = 38, .status = SNAP_STATUS_VALID, .maxSize = 50,
									.buffer = (uint8_t [50]){SNAP_SYNC, 0x25, 0x6A, 0xA0, 0xB1, 0xC2, 0xFF, 0x01, 0x80}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_FRAME_FORMAT, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}
}

TEST(calculateHash, should_ReturnErrorShortFrame_and_NotChangeHashValue_if_FrameFormatHasHashButFrameDoesNotHaveEnoughBytes)
{
// Frames with sync, header, dab, sab, data, hash //////////////////////////////

	{	// 8-bit checksum
		const snap_frame_t frame = {.size = 12, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 20,
									.buffer = (uint8_t [20]){SNAP_SYNC, 0xE1, 0x25, 0x99, 0x88, 0x77, 0xFE, 0xDC, 0xBA, 0x62, 0x63, 0x51}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_SHORT_FRAME, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	{	// 8-bit CRC
		const snap_frame_t frame = {.size = 22, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 30,
									.buffer = (uint8_t [30]){SNAP_SYNC, 0x5A, 0x39, 0xA1, 0xB1, 0xC1, 0xC2, 0xD1, 0xD2, 0xD3}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_SHORT_FRAME, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}

// Frames with sync, header, sab, hash /////////////////////////////////////////

	{	// 16-bit CRC
		const snap_frame_t frame = {.size = 4, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 10,
									.buffer = (uint8_t [10]){SNAP_SYNC, 0x20, 0x40, 0x33}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_SHORT_FRAME, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}

// Frames with sync, header, dab, sab, pfb, data, hash /////////////////////////

	{	// 32-bit CRC
		const snap_frame_t frame = {.size = 136, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 150,
									.buffer = (uint8_t [150]){SNAP_SYNC, 0xA8, 0x5C, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0xFF, 0xFF}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_SHORT_FRAME, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}

// Frames with sync, header, pfb, hash (user) //////////////////////////////////

	{	// 24-bit CRC (user)
		const snap_frame_t frame = {.size = 5, .status = SNAP_STATUS_INCOMPLETE, .maxSize = 15,
									.buffer = (uint8_t [15]){SNAP_SYNC, 0x0D, 0x70, 0x00, 0x00}};
		uint32_t hash = 0;

		TEST_ASSERT_EQUAL_INT8(SNAP_ERROR_SHORT_FRAME, snap_calculateHash(&frame, &hash));
		TEST_ASSERT_EQUAL_HEX32(0, hash);
	}
}


/******************************************************************************/
/*  MAIN                                                                      */
/******************************************************************************/


static void runAllTests(void)
{
	RUN_TEST_GROUP(miscFunctions);
	RUN_TEST_GROUP(init);
	RUN_TEST_GROUP(reset);
	RUN_TEST_GROUP(decode);
	RUN_TEST_GROUP(encapsulate);
	RUN_TEST_GROUP(getField);
	RUN_TEST_GROUP(calculateHash);
}

int main(int argc, const char **argv)
{
	return UnityMain(argc, argv, runAllTests);
}

/******************************** END OF FILE *********************************/

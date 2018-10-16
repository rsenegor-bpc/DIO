/*****************************************************************************/
/*                    Single digital input example                           */
/*                                                                           */
/*  This example shows how to use the powerdaq API to perform a single       */
/*  digital acquisition with a PD2-DIO boards.                               */
/*  The acquisition is performed in a software timed fashion that is         */
/*  appropriate for slow speed acquisition (up to 500Hz).                    */
/*  This example only works on PD2-DIO boards.                               */
/*  For the PDx-MFx and PD2-AO boards look at the SingleDI and SingleDO      */
/*  examples.                                                                */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/*      Copyright (C) 2001 United Electronic Industries, Inc.                */
/*      All rights reserved.                                                 */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/*****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>

#include "pwrdaq32.h"
#include "pwrdaq.h"
#include "pdfw_def.h" 
#include "pd_hcaps.h"
#include "ParseParams.h"


typedef enum _state
{
	closed,
	unconfigured,
	configured,
	running
} tState;

typedef struct _singleDioData
{
	int board;                    // board number to be used for the AI operation
	int nbOfBoards;             // number of boards installed
	HANDLE handle;                // board handle
	HANDLE driver;
	int nbOfPorts;                // nb of ports of 16 lines to use
	unsigned char OutPorts;       // mask for ports of 16 lines to use for output
	int nbOfSamplesPerPort;       // number of samples per port
	unsigned long portList[64];
	double scanRate;              // sampling frequency on each port
	tState state;                 // state of the acquisition session
	int Error;
} tSingleDioData;


int InitSingleDIO(tSingleDioData *pDioData);
int SingleDIO(tSingleDioData *pDioData);
void CleanUpSingleDIO(tSingleDioData *pDioData);


static tSingleDioData G_DioData;

// exit handler
void SingleDIOExitHandler(int status, void *arg)
{
	CleanUpSingleDIO((tSingleDioData *)arg);
}


int InitSingleDIO(tSingleDioData *pDioData)
{
	Adapter_Info adaptInfo;
	DWORD numAdapters;
	int retVal = 0;


	retVal = PdDriverOpen(&pDioData->driver, &pDioData->Error, &numAdapters);
	if (!retVal)
	{
		printf("SingleDIO: __PdGetAdapterInfo error %d\n", pDioData->Error);
		exit(EXIT_FAILURE);
	}

	// get adapter type
	retVal = _PdGetAdapterInfo(pDioData->board, &pDioData->Error, &adaptInfo);
	if (!retVal)
	{
		printf("SingleDIO: __PdGetAdapterInfo error %d\n", pDioData->Error);
		exit(EXIT_FAILURE);
	}

	if (adaptInfo.atType & atPD2DIO)
		printf("This is a PD2-DIO board\n");
	else
	{
		printf("This board is not a PD2-DIO\n");
		exit(EXIT_FAILURE);
	}

	retVal = _PdAdapterOpen(pDioData->board, &pDioData->Error, &pDioData->handle);

	if (!retVal)
	{
		printf("SingleDIO: __PdGetAdapterInfo error %d\n", pDioData->Error);
		exit(EXIT_FAILURE);
	}

	retVal = PdAdapterAcquireSubsystem(pDioData->handle, &pDioData->Error, DigitalIn, 1);

	retVal = PdAdapterAcquireSubsystem(pDioData->handle, &pDioData->Error, DigitalOut, 1);

	if (!retVal)
	{
		printf("SingleDIO: PdAcquireSubsystem failed\n");
		exit(EXIT_FAILURE);
	}

	pDioData->state = unconfigured;

	retVal = _PdDIOReset(pDioData->handle, &pDioData->Error);
	if (!retVal)
	{
		printf("SingleDIO: PdDInReset error %d\n", pDioData->Error);
		exit(EXIT_FAILURE);
	}

	return 0;
}


int SingleDIO(tSingleDioData *pDioData)
{
	int retVal;
	int i, j;
	DWORD readVal, writeVal;
	//int count = 0;
	UINT16 count = 0;
	retVal = _PdDIOEnableOutput(pDioData->handle, &pDioData->Error, pDioData->OutPorts);
	if (!retVal)
	{
		printf("SingleDIO: _PdDioEnableOutput failed\n");
		exit(EXIT_FAILURE);
	}



	pDioData->state = configured;


	pDioData->state = running;
	for (i = 0; i<pDioData->nbOfSamplesPerPort; i++)


	{
		for (j = 0; j < pDioData->nbOfPorts; j++)
		{

			if (count < 0xff) {
				writeVal = count;
				retVal = _PdDIOWrite(pDioData->handle, &pDioData->Error, j, writeVal);
				if (!retVal)
				{
					printf("SingleDIO: _PdDIOWrite error: %d\n", pDioData->Error);
					exit(EXIT_FAILURE);
				}
				printf("Value Write on Port %d: 0x%x\n", j, writeVal);
			}
			else {
				count = 0;
			}
			Sleep(100);
			retVal = _PdDIORead(pDioData->handle, &pDioData->Error, j, &readVal);
			if (!retVal)
			{
				printf("SingleDIO: _PdDIORead error: %d\n", pDioData->Error);
				exit(EXIT_FAILURE);
			}

			printf("Value Read on Port %d: 0x%x\n", j, readVal);

		}

		printf("\n");
		count++;
		Sleep((DWORD)(1.0E3 / pDioData->scanRate));
	}

	return retVal;
}


void CleanUpSingleDIO(tSingleDioData *pDioData)
{
	int retVal;

	if (pDioData->state == running)
	{
		pDioData->state = configured;
	}

	if (pDioData->state == configured)
	{
		retVal = _PdDIOReset(pDioData->handle, &pDioData->Error);
		if (!retVal)
			printf("SingleDI: _PdDIOReset error %d\n", pDioData->Error);

		pDioData->state = unconfigured;
	}

	if (pDioData->handle > 0 && pDioData->state == unconfigured)
	{
		retVal = PdAdapterAcquireSubsystem(pDioData->handle, &pDioData->Error, DigitalIn, 0);
		retVal = PdAdapterAcquireSubsystem(pDioData->handle, &pDioData->Error, DigitalOut, 0);
		if (!retVal)
			printf("SingleDI: PdReleaseSubsystem error %d\n", pDioData->Error);

		_PdAdapterClose(pDioData->handle, &pDioData->Error);

		PdDriverClose(pDioData->driver, &pDioData->Error);
	}

	pDioData->state = closed;
}


int main(int argc, char *argv[])
{
	int i;
	PD_PARAMS params = { 0, 8,{0,1,2,3,4,5,6,7}, 10.0, 0 };

	ParseParameters(argc, argv, &params);

	// initializes acquisition session parameters

	/*typedef struct _singleDioData
	{
		int board;                    // board number to be used for the AI operation
		int nbOfBoards;             // number of boards installed
		HANDLE handle;                // board handle
		HANDLE driver;
		int nbOfPorts;                // nb of ports of 16 lines to use
		unsigned char OutPorts;       // mask for ports of 16 lines to use for output
		int nbOfSamplesPerPort;       // number of samples per port
		unsigned long portList[64];
		double scanRate;              // sampling frequency on each port
		tState state;                 // state of the acquisition session
		int Error;
	} tSingleDioData;*/

	G_DioData.board = params.board;
	G_DioData.nbOfPorts = params.nb_channels;
	for (i = 0; i < params.nb_channels; i++)
	{
		G_DioData.portList[i] = params.channels[i];
	}
	G_DioData.handle = 0;
	G_DioData.OutPorts = 0x4; 
	G_DioData.nbOfSamplesPerPort = 50;
	G_DioData.scanRate = params.frequency;
	G_DioData.state = closed;


	// initializes acquisition session 
	InitSingleDIO(&G_DioData);

	// run the acquisition
	SingleDIO(&G_DioData);

	// Cleanup acquisition
	CleanUpSingleDIO(&G_DioData);

	return 0;
}


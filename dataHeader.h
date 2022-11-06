//
// Created by think on 11/4/22.
//

typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
//typedef signed __int64      INT64, *PINT64;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
//typedef unsigned __int64    UINT64, *PUINT64;
typedef float               FLOAT;
typedef float REAL32;
typedef double REAL64;

typedef struct SeaCommunication
{
    UINT32 PacketHead;
    UINT32 PayloadSize;
    UINT16 ChunkNum;
    UINT16 ChunkID;
    UINT32 DataTime;
    FLOAT Longitude;
    FLOAT Latitude;
    UINT16 SystemID;
    UINT16 DataType;
}SeaCommunication;

typedef struct BDCommunication
{
    UINT8 PacketHead;
    UINT16 SystemID;
    UINT16 DataType;
}BDCommunication;

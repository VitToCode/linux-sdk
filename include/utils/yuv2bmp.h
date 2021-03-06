/*************************************************************************
	> Filename: yuv2bmp.h
	>   Author: Qiuwei.wang
	>    Email: qiuwei.wang@ingenic.com
	> Datatime: Tue 29 Nov 2016 11:28:48 AM CST
 ************************************************************************/

#ifndef _YUV2BMP_H
#define _YUV2BMP_H
#include <types.h>

#define FAR
#define DWORD uint32_t
#define WORD  uint16_t
#ifndef LONG
#define LONG  uint64_t
#endif
#define BYTE  uint8_t

#ifndef PACKED
#define PACKED
#endif
#ifndef GCC_PACKED
#define GCC_PACKED __attribute__((packed))
#endif

/* structures for defining DIBs */
typedef PACKED struct tagBITMAPCOREHEADER
{
	DWORD   bcSize;                 /* used to get to color table */
	WORD    bcWidth;
	WORD    bcHeight;
	WORD    bcPlanes;
	WORD    bcBitCount;
} GCC_PACKED BITMAPCOREHEADER, FAR *LPBITMAPCOREHEADER, *PBITMAPCOREHEADER;

typedef PACKED struct tagBITMAPINFOHEADER
{
	DWORD      biSize;
	LONG       biWidth;
	LONG       biHeight;
	WORD       biPlanes;
	WORD       biBitCount;
	DWORD      biCompression;
	DWORD      biSizeImage;
	LONG       biXPelsPerMeter;
	LONG       biYPelsPerMeter;
	DWORD      biClrUsed;
	DWORD      biClrImportant;
} GCC_PACKED BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef PACKED struct tagRGBQUAD
{
	BYTE    rgbBlue;
	BYTE    rgbGreen;
	BYTE    rgbRed;
	BYTE    rgbReserved;
} GCC_PACKED RGBQUAD;
typedef RGBQUAD FAR* LPRGBQUAD;

typedef PACKED struct tagRGBTRIPLE
{
	BYTE    rgbtBlue;
	BYTE    rgbtGreen;
	BYTE    rgbtRed;
} GCC_PACKED RGBTRIPLE;


typedef PACKED struct tagBITMAPINFO
{
	BITMAPINFOHEADER    bmiHeader;
	RGBQUAD             bmiColors[1];
} GCC_PACKED BITMAPINFO, FAR *LPBITMAPINFO, *PBITMAPINFO;

typedef PACKED struct tagBITMAPCOREINFO
{
	BITMAPCOREHEADER    bmciHeader;
	RGBTRIPLE           bmciColors[1];
} GCC_PACKED BITMAPCOREINFO, FAR *LPBITMAPCOREINFO, *PBITMAPCOREINFO;

typedef PACKED struct tagBITMAPFILEHEADER
{
	WORD    bfType;
	DWORD   bfSize;
	WORD    bfReserved1;
	WORD    bfReserved2;
	DWORD   bfOffBits;
}GCC_PACKED BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;

/*
 * Extern functions
 */
int yuv2rgb(uint8_t *yuv, uint8_t *rgb, uint32_t width, uint32_t height);
int rgb2bmp(char *filename, uint32_t width, uint32_t height,\
        int iBitCount, uint8_t *rgbbuf);

#endif /* _YUV2BMP_H */

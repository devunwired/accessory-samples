#ifdef ADK_INTERNAL
#ifndef _SD_H_
#define _SD_H_


#define SD_BLOCK_SIZE	512

typedef struct{
	
	uint32_t numSec;
	uint8_t  HC	: 1;
	uint8_t  inited	: 1;
	uint8_t  SD	: 1;
	
}SD;

char sdInit(SD* sd);
uint32_t sdGetNumSec(SD* sd);
char sdSecRead(SD* sd, uint32_t sec, void* buf);
char sdSecWrite(SD* sd, uint32_t sec, const void* buf);

//stream mode
char sdReadStart(SD* sd, uint32_t sec);
void sdNextSec(SD* sd);
void sdSecReadStop(SD* sd);
uint8_t* sdStreamSec(uint8_t* buf);  //returns ptr to end of buffer...

#endif
#endif

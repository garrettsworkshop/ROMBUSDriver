#ifndef SDMMC_H
#define SDMMC_H


/* Status of Disk Functions */
typedef char sdstatus_t;

#define STA_NOINIT		0x01	/* Drive not initialized */
#define STA_NODISK		0x02	/* No medium in the drive */


/* Results of Disk Functions */
typedef enum sdmmc_result_e {
	RES_OK = 0,		/* 0: Function succeeded */
	RES_ERROR,		/* 1: Disk error */
	RES_NOTRDY,		/* 2: Not ready */
	RES_PARERR		/* 3: Invalid parameter */
} sdresult_t;


/*---------------------------------------*/
/* Prototypes for disk control functions */
/*---------------------------------------*/
sdstatus_t sdmmc_init();
sdresult_t sdmmc_readp(Ptr buf, long sector, uint offset, uint count);
sdresult_t sdmmc_writep(const Ptr buf, long sc);


#endif

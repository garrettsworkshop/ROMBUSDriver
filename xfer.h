#ifndef RDISK_H
#define RDISK_H

#pragma parameter xfer_s_256(__A0, __A1)
void xfer_s_256(Ptr srcreg, Ptr destmem);

#pragma parameter xfer_s(__D0, __A0, __A1)
void xfer_s(uint8_t numBytes, Ptr srcreg, Ptr destmem);


#endif

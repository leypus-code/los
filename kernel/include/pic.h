#ifndef LOS_PIC_H
#define LOS_PIC_H

void pic_remap(void);
void pic_send_eoi(unsigned char irq);

#endif

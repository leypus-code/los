#ifndef LOS_SERIAL_H
#define LOS_SERIAL_H

void serial_initialize(void);
int serial_is_ready(void);
int serial_read_char(void);
int serial_read_char_nonblocking(char *out);
void serial_write_char(char c);
void serial_write_string(const char *s);

#endif

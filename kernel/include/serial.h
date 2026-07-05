#ifndef LOS_SERIAL_H
#define LOS_SERIAL_H

void serial_initialize(void);
int serial_is_ready(void);

void serial_write_char(char c);
void serial_write_string(const char *s);
void serial_write_line(const char *s);
int serial_read_char_nonblocking(char *out);

#endif

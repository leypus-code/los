#ifndef LOS_MODEL_H
#define LOS_MODEL_H

void model_initialize(void);
void model_status(void);
void model_list(void);
int model_import(const char *name);
int model_load(const char *name);

#endif

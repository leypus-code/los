#ifndef LOS_WORKSPACE_BUILDER_H
#define LOS_WORKSPACE_BUILDER_H

void workspace_builder_initialize(void);
void workspace_builder_list(void);
int workspace_builder_open(const char *name);
int workspace_builder_create(const char *name, const char *kind);
int workspace_builder_template(const char *kind, const char *name);
int workspace_builder_set_title(const char *name, const char *title);
int workspace_builder_add_block(const char *name, const char *type, const char *title, const char *content);
int workspace_builder_add_button(const char *name, const char *title, const char *label, const char *action);
int workspace_builder_add_node(const char *name, const char *kind, const char *orientation, const char *weight);
int workspace_builder_end_node(const char *name);
int workspace_builder_list_blocks(const char *name);
int workspace_builder_remove_block(const char *name, const char *title);
int workspace_builder_replace_block(const char *name, const char *old_title, const char *type, const char *new_title, const char *content);
int workspace_builder_set_block_action(const char *name, const char *title, const char *action);

#endif

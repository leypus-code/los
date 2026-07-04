#ifndef LOS_AI_BRIDGE_H
#define LOS_AI_BRIDGE_H

int ai_bridge_ask(const char *prompt, char *out, int max);
int ai_bridge_execute(const char *prompt);
int ai_bridge_web(const char *query);
int ai_bridge_talk(const char *text);
void ai_bridge_show_transcript(void);
void ai_bridge_set_host_enabled(int enabled);
int ai_bridge_host_enabled(void);
void ai_bridge_status(void);
void ai_bridge_context_set_workspace(const char *name);
void ai_bridge_context_set_last_intent(const char *intent);
void ai_bridge_context_show(void);
void ai_bridge_model_status(void);
void ai_bridge_model_set_local(void);
void ai_bridge_model_set_host(void);
void ai_bridge_packet_show(void);
int ai_bridge_apply_ui_patch(const char *patch);
void ai_bridge_ui_patch_help(void);

#endif

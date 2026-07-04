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

#endif

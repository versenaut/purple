/*
 * 
*/

extern void	graph_init(void);

extern void	graph_method_send_creates(uint32 avatar, uint8 group_id);

extern void	graph_method_send_call_create(VNodeID node, VLayerID buffer, const char *name);
extern void	graph_method_send_call_rename(uint32 id, const char *name);
extern void	graph_method_send_call_destroy(uint32 id);
extern void	graph_method_send_call_mod_create(uint32 graph_id, uint32 plugin_id);
extern void	graph_method_send_call_mod_input_set(uint32 graph_id, uint32 mod_id, uint32 index, PInputType vtype, ...);

extern void	graph_method_receive_create(uint8 id, const char *name);
extern void	graph_method_receive_call(uint8 id, const void *param);

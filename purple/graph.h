/*
 * 
*/

extern void	graph_init(void);

extern void	graph_method_send_creates(uint32 avatar, uint8 group_id);

extern void	graph_method_send_call_create(VNodeID node, VLayerID buffer, const char *name);
extern void	graph_method_send_call_destroy(uint32 id);

extern void	graph_method_receive_create(uint8 id, const char *name);
extern void	graph_method_receive_call(uint8 id, const void *param);

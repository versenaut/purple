/*
 * Graph editing functionality. Graphs are very opaque.
*/

/* Initialize graph module. Must be called before module is used. */
extern void	graph_init(void);

/* Send Verse method creation commands for the group editing API on the given avatar. */
extern void	graph_method_send_creates(uint32 avatar, uint8 group_id);

/* These are used for local, "console", testing. In an actual production version of Purple,
 * they might be exluded since the calls are expected to come in from dedicated UI clients.
*/
extern void	graph_method_send_call_create(VNodeID node, VLayerID buffer, const char *name);
extern void	graph_method_send_call_destroy(uint32 id);
extern void	graph_method_send_call_mod_create(uint32 graph_id, uint32 plugin_id);
extern void	graph_method_send_call_mod_destroy(uint32 graph_id, uint32 module_id);
extern void	graph_method_send_call_mod_input_set(uint32 graph_id, uint32 mod_id, uint32 index,
						     PValueType type, const PValue *value);
extern void	graph_method_send_call_mod_input_clear(uint32 graph_id, uint32 mod_id, uint32 input);

/* Check if the graph editing API has been created yet. Called from nodedb notification callback. */
extern void	graph_method_check_created(NodeObject *node);

/* A method call was received, check if it's one of the graph editing calls, and if so take action. */
extern void	graph_method_receive_call(uint8 id, const void *param);

/* Output API uses this to set the output port. Gives us a chance to notify graph. */
extern void	graph_port_output_set(PPOutput port, PValueType type, ...);

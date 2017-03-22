#ifndef __XDD__LOOP__HH___
#define __XDD__LOOP__HH___


struct xdd_loop_ctrl_handle {
    int fd;
};

int loop_ctrl_open(struct xdd_loop_ctrl_handle* loop_if);
void loop_ctrl_close(struct xdd_loop_ctrl_handle* loop_if);
int loop_ctrl_next_available_dev(struct xdd_loop_ctrl_handle* loop_if, char** out_dev);

int loop_dev_bind(const char* device, const char* filename, const char* mode);
int loop_dev_unbind(const char* device);
int loop_dev_filename(const char* device, char** filename);

#endif /* __XDD__LOOP__HH___ */

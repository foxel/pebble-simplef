#ifndef STATUS_H
#define STATUS_H
 
void status_init(Window* window);
void status_deinit(void);
void status_set_style(bool inverse);
void status_update(void);

#endif /* STATUS_H */
